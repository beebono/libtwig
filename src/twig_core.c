#include "twig.h"
#include "twig_priv.h"
#include "twig_regs.h"

EXPORT twig_dev_t *twig_open(void) {
    twig_dev_t ve = {
        .fd = -1,
        .regs = NULL,
        .allocator = NULL,
    };

    ve.fd = open(DEVICE, O_RDWR);
    if (ve.fd == -1) {
        return NULL;
    }

    if (ioctl(ve.fd, IOCTL_ENGINE_REQ, 0) < 0)
        goto err_close;
    
    ioctl(ve.fd, IOCTL_ENABLE_VE, 0);
    ioctl(ve.fd, IOCTL_SET_VE_FREQ, 180);
    ioctl(ve.fd, IOCTL_RESET_VE, 0);

    ve.regs = mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, ve.fd, VE_BASE_ADDR);
    if (ve.regs == MAP_FAILED)
        goto err_release;

    ve.allocator = twig_allocator_ion_create();
    if (!ve.allocator)
        goto err_unmap;

    return &ve;

err_unmap:
    munmap(ve.regs, 2048);
    ve.regs = NULL;

err_release:
    ioctl(ve.fd, IOCTL_ENGINE_REL, 0);

err_close:
    close(ve.fd);
    ve.fd = -1;
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

EXPORT void twig_free_mem(twig_mem_t *mem) {
    if (!mem || !ve.allocator)
        return;
    
    ve.allocator->mem_free(ve.allocator, mem);
}

EXPORT void twig_flush_cache(twig_mem_t *mem) {
    if (!mem || !ve.allocator)
        return;
    
    ve.allocator->mem_flush(ve.allocator, mem);
}