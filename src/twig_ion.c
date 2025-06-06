#include "twig.h"
#include "twig_priv.h"
#include "allwinner/ion.h"

#define PAGE_SIZE                   4096
#define MEM_OFFSET	                0x40000000UL
#define ION_IOC_SUNXI_FLUSH_RANGE	5
#define ION_IOC_SUNXI_FLUSH_ALL		6
#define ION_IOC_SUNXI_PHYS_ADDR		7
#define ION_IOC_SUNXI_DMA_COPY		8
#define ION_IOC_SUNXI_DUMP			9
#define ION_IOC_SUNXI_POOL_FREE		10

typedef struct {
	long start;
	long end;
} sunxi_cache_range;

typedef struct {
	int handle;
	unsigned int phys_addr;
	unsigned int size;
} sunxi_phys_data;

struct ion_mem {
    twig_mem_t pub_mem;
    int handle;
};

static int ion_alloc(int dev_fd, size_t size) {
    if (dev_fd < 0 || size <= 0)
        return -1;

    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    struct ion_allocation_data alloc = {
        .len = size,
        .align = PAGE_SIZE,
        .heap_id_mask = ION_HEAP_TYPE_DMA_MASK,
        .flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC,
    };

    int ret = ioctl(dev_fd, ION_IOC_ALLOC, &alloc);
    if (ret < 0) 
        return -1;

    return alloc.handle;
}

static uint32_t ion_get_phys_addr(int dev_fd, int handle) {
    if (dev_fd < 0 || handle < 0)
        return 0x0;

    sunxi_phys_data phys_data = {
		.handle = handle,
	};
	struct ion_custom_data custom_data = {
		.cmd = ION_IOC_SUNXI_PHYS_ADDR,
		.arg = (unsigned long)(&phys_data),
	};

	int ret = ioctl(dev_fd, ION_IOC_CUSTOM, &custom_data);
    if (ret < 0)
		return 0x0;

    return phys_data.phys_addr - MEM_OFFSET;
}

static int ion_map(int dev_fd, int handle) {
    if (dev_fd < 0 || handle < 0)
        return -1;

    struct ion_fd_data fd_data = {
        .handle = handle,
    };

    int ret = ioctl(dev_fd, ION_IOC_MAP, &fd_data);
    if (ret < 0)
        return -1;

    return fd_data.fd;
}

static void ion_free(int dev_fd, int handle) {
    if (dev_fd < 0 || handle < 0)
        return;

    struct ion_handle_data handle_data = {
		.handle = handle,
	};

    ioctl(dev_fd, ION_IOC_FREE, &handle_data);
}

static void ion_flush_cache(int dev_fd, void *addr, size_t size) {
    if (dev_fd < 0 || addr == NULL || size <= 0)
        return;

    sunxi_cache_range cache_range = {
		.start = (long)addr,
		.end = (long)addr + size,
	};
    struct ion_custom_data custom_data = {
		.cmd = ION_IOC_SUNXI_FLUSH_RANGE,
		.arg = (unsigned long)(&cache_range),
	};

    ioctl(dev_fd, ION_IOC_CUSTOM, &custom_data);
}

static twig_mem_t *twig_allocator_ion_mem_alloc(twig_allocator_t *allocator, size_t size) {
    struct ion_mem *mem = calloc(1, sizeof(*mem));
	if (!mem)
		return NULL;

    mem->pub_mem.size = size;
    mem->handle = ion_alloc(allocator->dev_fd, size);
    if (mem->handle < 0)
        goto err_free;

    mem->pub_mem.phys = ion_get_phys_addr(allocator->dev_fd, mem->handle);
    if (!mem->pub_mem.phys)
        goto err_ion_free;
    
    mem->pub_mem.ion_fd = ion_map(allocator->dev_fd, mem->handle);
    if (mem->pub_mem.ion_fd < 0)
        goto err_ion_free;

    mem->pub_mem.virt = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem->pub_mem.ion_fd, 0);
    if (mem->pub_mem.virt == MAP_FAILED)
        goto err_close;

    mem->pub_mem.type = TWIG_MEM_ION;
    return &mem->pub_mem;

err_close:
    close(mem->pub_mem.ion_fd);

err_ion_free:
    ion_free(allocator->dev_fd, mem->handle);

err_free:
    free(mem);
    return NULL;
}

static void twig_allocator_ion_mem_free(twig_allocator_t *allocator, twig_mem_t *pub_mem) {
    if (!allocator || !pub_mem)
        return;

    struct ion_mem *mem = (struct ion_mem *)pub_mem;
    munmap(pub_mem->virt, pub_mem->size);
    close(pub_mem->ion_fd);
    ion_free(allocator->dev_fd, mem->handle);
    free(mem);
}

static void twig_allocator_ion_mem_flush(twig_allocator_t *allocator, twig_mem_t *pub_mem) {
    if (!allocator || !pub_mem)
        return;

    struct ion_mem *mem = (struct ion_mem *)pub_mem;
    ion_flush_cache(allocator->dev_fd, pub_mem->virt, pub_mem->size);
}

static void twig_allocator_ion_destroy(twig_allocator_t *allocator) {
    if (!allocator)
        return;

    if (allocator->dev_fd != -1) {
        close(allocator->dev_fd);
    }
    free(allocator);
}

twig_allocator_t *twig_allocator_ion_create(void) {
    struct twig_allocator *allocator = calloc(1, sizeof(*allocator));
    if (!allocator)
        return NULL;

    allocator->dev_fd = open("/dev/ion", O_RDONLY);
    if (allocator->dev_fd == -1) {
        free(allocator);
        return NULL;
    }

    allocator->mem_alloc = twig_allocator_ion_mem_alloc;
    allocator->mem_free = twig_allocator_ion_mem_free;
    allocator->mem_flush = twig_allocator_ion_mem_flush;
    allocator->destroy = twig_allocator_ion_destroy;

    return allocator;
}