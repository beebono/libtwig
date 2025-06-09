#include "twig.h"
#include "twig_priv.h"
#include "twig_regs.h"

EXPORT twig_dev_t *twig_open(void) {
    twig_dev_t *cedar = calloc(1, sizeof(*cedar));
    if (!cedar)
        return NULL;

    cedar->fd = -1;
    cedar->regs = NULL;
    cedar->allocator = NULL;

    cedar->fd = open(DEVICE, O_RDWR);
    if (cedar->fd == -1) {
        free(cedar);
        return NULL;
    }

    cedar->regs = mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, cedar->fd, VE_BASE_ADDR);
    fprintf(stderr, "VE_H264_STATUS: 0x%08x\n", twig_readl(cedar->regs + H264_STATUS));

    ioctl(cedar->fd, IOCTL_SET_REFCOUNT, 0);

    if (ioctl(cedar->fd, IOCTL_ENGINE_REQ, 0) < 0)
        goto err_close;

    ioctl(cedar->fd, IOCTL_ENABLE_VE, 0);
    ioctl(cedar->fd, IOCTL_RESET_VE, 0);

    
    if (cedar->regs == MAP_FAILED)
        goto err_release;

    cedar->allocator = twig_allocator_ion_create();
    if (!cedar->allocator)
        goto err_unmap;

    return cedar;

err_unmap:
    munmap(cedar->regs, 2048);
    cedar->regs = NULL;

err_release:
    ioctl(cedar->fd, IOCTL_ENGINE_REL, 0);

err_close:
    close(cedar->fd);
    cedar->fd = -1;
    return NULL;
}

EXPORT void twig_close(twig_dev_t *cedar) {
	if (cedar->fd == -1)
		return;

	munmap(cedar->regs, 2048);
	cedar->regs = NULL;

    if (cedar->allocator) {
        cedar->allocator->destroy(cedar->allocator);
        cedar->allocator = NULL;
    }

    ioctl(cedar->fd, IOCTL_DISABLE_VE, 0);
    ioctl(cedar->fd, IOCTL_ENGINE_REL, 0);

	close(cedar->fd);
	cedar->fd = -1;
}

EXPORT int twig_wait_for_ve(twig_dev_t *cedar) {
    if (!cedar)
        return -1;

    int ret = ioctl(cedar->fd, IOCTL_WAIT_VE_DE, 1);
    if (ret < 0)
        return -1;

    return 0;
}

EXPORT void* twig_get_ve_regs(twig_dev_t *cedar) {
    if (!cedar)
        return (void*)0x0;

    return (char*)cedar->regs;
}

EXPORT twig_mem_t* twig_alloc_mem(twig_dev_t *cedar, size_t size) {
    if (!cedar || size <= 0 || !cedar->allocator)
        return NULL;
    
    return cedar->allocator->mem_alloc(cedar->allocator, size);
}

EXPORT void twig_flush_mem(twig_dev_t *cedar, twig_mem_t *mem) {
    if (!mem || !cedar)
        return;
    
    cedar->allocator->mem_flush(cedar->allocatorm, mem);
}

EXPORT void twig_free_mem(twig_dev_t *cedar, twig_mem_t *mem) {
    if (!mem || !cedar->allocator)
        return;
    
    cedar->allocator->mem_free(cedar->allocator, mem);
}