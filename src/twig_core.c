#include "twig.h"
#include "twig_priv.h"
#include "twig_regs.h"

EXPORT twig_dev_t *twig_open(void) {
    twig_dev_t *dev = calloc(1, sizeof(*dev));
    if (!dev) {
        fprintf(stderr, "twig_open: failed to allocate device structure\n");
        return NULL;
    }

    dev->fd = -1;
    dev->regs = NULL;
    dev->allocator = NULL;

    dev->fd = open(DEVICE, O_RDWR);
    if (dev->fd == -1) {
        return NULL;
    }

    if (ioctl(dev->fd, IOCTL_ENGINE_REQ, 0) < 0)
        goto err_close;
    
    ioctl(dev->fd, IOCTL_ENABLE_VE, 0);
    ioctl(dev->fd, IOCTL_SET_VE_FREQ, 180);
    ioctl(dev->fd, IOCTL_RESET_VE, 0);

    dev->regs = mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, VE_BASE_ADDR);
    if (dev->regs == MAP_FAILED)
        goto err_release;

    dev->allocator = twig_allocator_ion_create();
    if (!dev->allocator)
        goto err_unmap;

    return dev;

err_unmap:
    munmap(dev->regs, 2048);
    dev->regs = NULL;

err_release:
    ioctl(dev->fd, IOCTL_ENGINE_REL, 0);

err_close:
    close(dev->fd);
    dev->fd = -1;
    return NULL;
}

EXPORT void twig_close(twig_dev_t *dev) {
	if (dev->fd == -1)
		return;

	munmap(dev->regs, 2048);
	dev->regs = NULL;

    if (dev->allocator) {
        dev->allocator->destroy(dev->allocator);
        dev->allocator = NULL;
    }

    ioctl(dev->fd, IOCTL_DISABLE_VE, 0);
    ioctl(dev->fd, IOCTL_ENGINE_REL, 0);

	close(dev->fd);
	dev->fd = -1;
}

EXPORT int twig_wait_for_ve(twig_dev_t *dev) {
    if (!dev)
        return -1;

    int ret = ioctl(dev->fd, IOCTL_WAIT_VE_DE, 1);
    if (ret < 0)
        return -1;

    return 0;
}

EXPORT void* twig_get_ve_regs(twig_dev_t *dev) {
    if (!dev)
        return (void*)0x0;

    return (char*)dev->regs;
}

EXPORT twig_mem_t* twig_alloc_mem(twig_dev_t *dev, size_t size) {
    if (!dev || size <= 0 || !dev->allocator)
        return NULL;
    
    return dev->allocator->mem_alloc(dev->allocator, size);
}

EXPORT void twig_free_mem(twig_dev_t *dev, twig_mem_t *mem) {
    if (!mem || !dev->allocator)
        return;
    
    dev->allocator->mem_free(dev->allocator, mem);
}

EXPORT void twig_flush_cache(twig_dev_t *dev, twig_mem_t *mem) {
    if (!mem || !dev)
        return;
    
    // Try different cache flush methods
    struct user_iommu_param iommu_param = {
        .fd = mem->ion_fd,
        .iommu_addr = mem->iommu_addr,
    };
    
    // Try IOCTL_FLUSH_CACHE_RANGE first (kernel 5.4+)
    int ret = ioctl(dev->fd, IOCTL_FLUSH_CACHE_RANGE, &iommu_param);
    if (ret == 0) {
        printf("Cache flushed with IOCTL_FLUSH_CACHE_RANGE\n");
        return;
    }
    
    // Fall back to IOCTL_FLUSH_CACHE_ALL
    ret = ioctl(dev->fd, IOCTL_FLUSH_CACHE_ALL, 0);
    if (ret == 0) {
        printf("Cache flushed with IOCTL_FLUSH_CACHE_ALL\n");
        return;
    }
    
    // Fall back to basic IOCTL_FLUSH_CACHE
    ret = ioctl(dev->fd, IOCTL_FLUSH_CACHE, 0);
    if (ret == 0) {
        printf("Cache flushed with IOCTL_FLUSH_CACHE\n");
        return;
    }
    
    printf("Warning: All cache flush methods failed: %d\n", ret);
}