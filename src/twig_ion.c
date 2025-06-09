#include "twig_mem_priv.h"

struct ion_mem {
    twig_mem_t pub_mem;
    int handle;
};

static int ion_alloc(int dev_fd, size_t size) {
    if (dev_fd < 0 || size <= 0)
        return -1;

    struct ion_allocation_data alloc = {
        .len = size,
        .align = 4096,
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

    struct sunxi_phys_data phys_data = {
		.handle = handle
	};
	struct ion_custom_data custom_data = {
		.cmd = ION_IOC_SUNXI_PHYS_ADDR,
		.arg = (unsigned long)(&phys_data)
	};

	int ret = ioctl(dev_fd, ION_IOC_CUSTOM, &custom_data);
    if (ret < 0)
		return 0x0;

    return phys_data.phys_addr;
}

static uint32_t ion_get_iommu_addr(int cedar_fd, int ion_fd) {
    if (cedar_fd < 0 || ion_fd < 0)
        return 0x0;

    struct user_iommu_param iommu_param = {
        .fd = ion_fd
    };

    int ret = ioctl(cedar_fd, IOCTL_GET_IOMMU_ADDR, &iommu_param);
    if (ret < 0)
        return 0x0;

    return iommu_param.iommu_addr;
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
		.handle = handle
	};

    ioctl(dev_fd, ION_IOC_FREE, &handle_data);
}

static void ion_free_iommu_addr(int cedar_fd, int ion_fd) {
    if (cedar_fd < 0 || ion_fd < 0)
        return;

    struct user_iommu_param iommu_param = {
        .fd = ion_fd,
        .iommu_addr = 0,
    };

    ioctl(cedar_fd, IOCTL_FREE_IOMMU_ADDR, &iommu_param);
}

static twig_mem_t *twig_allocator_ion_mem_alloc(twig_allocator_t *allocator, size_t size) {
    if (!allocator || size <= 0)
        return NULL;

    struct ion_mem *mem = calloc(1, sizeof(*mem));
	if (!mem)
		return NULL;
    
    size = (size + 4095) & ~(4095);
    mem->handle = ion_alloc(allocator->dev_fd, size);
    if (mem->handle < 0)
        goto err_free;

    mem->pub_mem.size = size;

    mem->pub_mem.phys = ion_get_phys_addr(allocator->dev_fd, mem->handle);
    if (!mem->pub_mem.phys)
        goto err_ion_free;
    
    mem->pub_mem.ion_fd = ion_map(allocator->dev_fd, mem->handle);
    if (mem->pub_mem.ion_fd < 0)
        goto err_ion_free;

    mem->pub_mem.virt = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem->pub_mem.ion_fd, 0);
    if (mem->pub_mem.virt == MAP_FAILED)
        goto err_close;

    mem->pub_mem.iommu_addr = ion_get_iommu_addr(allocator->cedar_fd, mem->pub_mem.ion_fd);
    if (!mem->pub_mem.iommu_addr)
        goto err_unmap;

    return &mem->pub_mem;

err_unmap:
    munmap(mem->pub_mem.virt, size);

err_close:
    close(mem->pub_mem.ion_fd);

err_ion_free:
    ion_free(allocator->dev_fd, mem->handle);

err_free:
    free(mem);
    return NULL;
}

static void twig_allocator_ion_mem_flush(twig_allocator_t *allocator, twig_mem_t *pub_mem) {
    if (!allocator || !pub_mem)
        return;

    struct user_iommu_param iommu_param = {
        .fd = pub_mem->ion_fd,
        .iommu_addr = pub_mem->iommu_addr,
    };
    
    ioctl(allocator->cedar_fd, IOCTL_FLUSH_CACHE_RANGE, &iommu_param);
}

static void twig_allocator_ion_mem_free(twig_allocator_t *allocator, twig_mem_t *pub_mem) {
    if (!allocator || !pub_mem)
        return;

    struct ion_mem *mem = (struct ion_mem*)pub_mem;

    ion_free_iommu_addr(allocator->cedar_fd, pub_mem->ion_fd);
    munmap(pub_mem->virt, pub_mem->size);
    close(pub_mem->ion_fd);
    ion_free(allocator->dev_fd, mem->handle);
    free(mem);
}

static void twig_allocator_ion_destroy(twig_allocator_t *allocator) {
    if (!allocator)
        return;

    if (allocator->dev_fd != -1) {
        close(allocator->dev_fd);
    }
    free(allocator);
}

twig_allocator_t *twig_allocator_ion_create(twig_dev_t *cedar) {
    twig_allocator_t *allocator = calloc(1, sizeof(*allocator));
    if (!allocator)
        return NULL;

    allocator->dev_fd = open("/dev/ion", O_RDONLY);
    if (allocator->dev_fd == -1) {
        free(allocator);
        return NULL;
    }

    allocator->cedar_fd = cedar->fd;
    allocator->mem_alloc = twig_allocator_ion_mem_alloc;
    allocator->mem_flush = twig_allocator_ion_mem_flush;
    allocator->mem_free = twig_allocator_ion_mem_free;
    allocator->destroy = twig_allocator_ion_destroy;

    return allocator;
}