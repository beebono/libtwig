#include "twig.h"
#include "allwinner/ion.h"
#include "allwinner/cedardev_api.h"

#define ION_IOC_SUNXI_FLUSH_RANGE 5
#define ION_IOC_SUNXI_PHYS_ADDR	  7

struct sunxi_cache_range {
	long start, end;
};

struct sunxi_phys_data {
	int handle;
	unsigned int phys_addr, size;
};

struct user_iommu_param {
    int fd;
    unsigned int iommu_addr;
};

struct ion_mem {
    twig_mem_t pub_mem;
    int handle, dev_fd;
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

twig_mem_t *twig_ion_alloc_mem(int cedar_fd, size_t size) {
    if (size <= 0)
        return NULL;

    struct ion_mem *mem = calloc(1, sizeof(*mem));
	if (!mem)
		return NULL;
    
    static int dev_fd = -1;
    if (dev_fd < 0)
        dev_fd = open("/dev/ion", O_RDWR);

    mem->dev_fd = dev_fd;
    if (mem->dev_fd < 0)
        goto err_free;

    mem->pub_mem.size = (size + 4095) & ~(4095);
    mem->handle = ion_alloc(mem->dev_fd, mem->pub_mem.size);
    if (mem->handle < 0)
        goto err_close;

    mem->pub_mem.phys_addr = ion_get_phys_addr(mem->dev_fd, mem->handle);
    mem->pub_mem.ion_fd = ion_map(mem->dev_fd, mem->handle);
    if (!mem->pub_mem.phys_addr || mem->pub_mem.ion_fd < 0)
        goto err_free2;

    mem->pub_mem.virt = mmap(NULL, mem->pub_mem.size, PROT_READ | PROT_WRITE, MAP_SHARED, mem->pub_mem.ion_fd, 0);
    if (mem->pub_mem.virt == MAP_FAILED)
        goto err_close2;

    mem->pub_mem.iommu_addr = ion_get_iommu_addr(cedar_fd, mem->pub_mem.ion_fd);
    if (!mem->pub_mem.iommu_addr)
        goto err_unmap;

    return &mem->pub_mem;

err_unmap:
    munmap(mem->pub_mem.virt, mem->pub_mem.size);

err_close2:
    close(mem->pub_mem.ion_fd);

err_free2:
    ion_free(mem->dev_fd, mem->handle);

err_close:
    close(mem->dev_fd);

err_free:
    free(mem);
    return NULL;
}

void twig_ion_flush_mem(twig_mem_t *pub_mem) {
    if (!pub_mem)
        return;

    struct ion_mem *mem = (struct ion_mem*)pub_mem;

    struct sunxi_cache_range range = {
        .start = (long)pub_mem->virt,
        .end = (long)pub_mem->virt + pub_mem->size
    };
    
    ioctl(mem->dev_fd, ION_IOC_SUNXI_FLUSH_RANGE, &range);
}

void twig_ion_free_mem(int cedar_fd, twig_mem_t *pub_mem) {
    if (cedar_fd < 0 || !pub_mem)
        return;

    struct ion_mem *mem = (struct ion_mem*)pub_mem;

    ion_free_iommu_addr(cedar_fd, pub_mem->ion_fd);
    munmap(pub_mem->virt, pub_mem->size);
    close(pub_mem->ion_fd);
    ion_free(mem->dev_fd, mem->handle);
    free(mem);
}