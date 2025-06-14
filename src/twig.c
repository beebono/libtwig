#include "twig.h"
#include "twig_regs.h"
#include "allwinner/cedardev_api.h"

#define EXPORT __attribute__((visibility ("default")))

struct twig_dev_t {
	int fd, active;
	void *regs;
};

twig_mem_t *twig_ion_alloc_mem(int cedar_fd, size_t size);
void twig_ion_flush_mem(twig_mem_t *pub_mem);
void twig_ion_free_mem(int cedar_fd, twig_mem_t *pub_mem);

EXPORT twig_dev_t *twig_open(void) {
    twig_dev_t *cedar = calloc(1, sizeof(*cedar));
    if (!cedar)
        return NULL;

    cedar->fd = open("/dev/cedar_dev", O_RDWR);
    if (cedar->fd == -1)
        goto err_free;

    cedar->regs = mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, cedar->fd, VE_BASE);
    if (cedar->regs == MAP_FAILED)
        goto err_close;

    if(twig_readl((uintptr_t)cedar->regs, VE_CTRL) & 0x00000001) {
        fprintf(stderr, "WARNING: Cedar VE is still in H.264 mode, but twig_open was called again!\n");
        fprintf(stderr, "         Forcing Refcount to 0 in case the previous instance crashed!\n")
        ioctl(cedar->fd, IOCTL_SET_REFCOUNT, 0);
    }

    if (ioctl(cedar->fd, IOCTL_ENABLE_VE, 0) < 0)
        goto err_unmap;

    if (ioctl(cedar->fd, IOCTL_ENGINE_REQ, 0) < 0)
        goto err_disable;

    cedar->active = 0;
    return cedar;

err_disable:
    ioctl(cedar->fd, IOCTL_DISABLE_VE, 0);
err_unmap:
    munmap(cedar->regs, 2048);
	cedar->regs = NULL;
err_close:
    close(cedar->fd);
    cedar->fd = -1;
err_free:   
    free(cedar);
    return NULL;
}

void *twig_get_ve_regs(twig_dev_t *cedar) {
    if (!cedar)
        return NULL;

    if (cedar->active == 0) {
        twig_writel((uintptr_t)cedar->regs, VE_CTRL, 0x00130001);
        cedar->active = 1;
    }
    return cedar->regs;
}

int twig_wait_for_ve(twig_dev_t *cedar) {
    if (!cedar)
        return -1;

    int ret = ioctl(cedar->fd, IOCTL_WAIT_VE_DE, 1);
    if (ret < 0)
        return -1;

    return 0;
}

void twig_put_ve_regs(twig_dev_t *cedar) {
    if (!cedar)
        return;

    twig_writel((uintptr_t)cedar->regs, VE_CTRL, 0x00130007);
    cedar->active = 0;
}

EXPORT twig_mem_t *twig_alloc_mem(twig_dev_t *cedar, size_t size) {
    if (!cedar || cedar->fd < 0 || size <= 0)
        return NULL;
    
    return twig_ion_alloc_mem(cedar->fd, size);
}

EXPORT void twig_flush_mem(twig_mem_t *mem) {
    if (!mem)
        return;
    
    twig_ion_flush_mem(mem);
}

EXPORT void twig_free_mem(twig_dev_t *cedar, twig_mem_t *mem) {
    if (!cedar || cedar->fd < 0 || !mem)
        return;
    
    twig_ion_free_mem(cedar->fd, mem);
}

EXPORT void twig_close(twig_dev_t *cedar) {
	if (cedar->fd == -1)
		return;

    if (cedar->active == 1)
        twig_put_ve_regs(cedar);

	munmap(cedar->regs, 2048);
	cedar->regs = NULL;

    ioctl(cedar->fd, IOCTL_ENGINE_REL, 0);
    ioctl(cedar->fd, IOCTL_DISABLE_VE, 0);

	close(cedar->fd);
	cedar->fd = -1;
}