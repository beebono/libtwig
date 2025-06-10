#include "twig.h"
#include "twig_regs.h"

#define EXPORT __attribute__((visibility ("default")))

struct twig_dev_t {
	int fd, active;
	void *regs;
};

EXPORT twig_dev_t *twig_open(void) {
    twig_dev_t *cedar = calloc(1, sizeof(*cedar));
    if (!cedar)
        return NULL;

    cedar->fd = open("/dev/cedar_dev", O_RDWR);
    if (cedar->fd == -1)
        goto err_free;

    ioctl(cedar->fd, IOCTL_ENABLE_VE, 0);

    if (ioctl(cedar->fd, IOCTL_ENGINE_REQ, 0) < 0)
        goto err_close;

    ioctl(cedar->fd, IOCTL_SET_VE_FREQ, 216);
    ioctl(cedar->fd, IOCTL_RESET_VE, 0);

    cedar->regs = mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, cedar->fd, VE_REG_BASE);
    if (cedar->regs == MAP_FAILED) {
        perror("mmap regs: MAP_FAILED");
        goto err_release;
    }

    cedar->active = 0;
    return cedar;

err_release:
    ioctl(cedar->fd, IOCTL_ENGINE_REL, 0);

err_close:
    close(cedar->fd);
    cedar->fd = -1;

err_free:   
    free(cedar);
    return NULL;
}

EXPORT void twig_close(twig_dev_t *cedar) {
	if (cedar->fd == -1)
		return;

    if (cedar->active)
        twig_put_ve_regs(cedar);

	munmap(cedar->regs, 2048);
	cedar->regs = NULL;

    if (cedar->allocator) {
        cedar->allocator->destroy(cedar->allocator);
        cedar->allocator = NULL;
    }

    ioctl(cedar->fd, IOCTL_ENGINE_REL, 0);
    ioctl(cedar->fd, IOCTL_DISABLE_VE, 0);

	close(cedar->fd);
	cedar->fd = -1;
}

void *twig_get_ve_regs(twig_dev_t *cedar, int width_geq2048_flag) {
    if (!cedar)
        return NULL;

    if (cedar->active == 0) {
        twig_writel(cedar->regs, VE_CTRL, 0x00130001 | (width_geq2048_flag ? 0x00200000 : 0x0 ));
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

    twig_writel(cedar->regs, VE_CTRL, 0x00130007);
    cedar->active = 0;
}

EXPORT twig_mem_t *twig_alloc_mem(twig_dev_t *cedar, size_t size) {
    if (!cedar || cedar->fd < 0 || size <= 0)
        return NULL;
    
    return twig_ion_alloc_mem(size);
}

EXPORT void twig_flush_mem(twig_dev_t *cedar, twig_mem_t *mem) {
    if (!cedar || cedar->fd < 0 || !mem)
        return;
    
    twig_ion_flush_mem(cedar->fd, mem);
}

EXPORT void twig_free_mem(twig_dev_t *cedar, twig_mem_t *mem) {
    if (!cedar || cedar->fd < 0 || !mem)
        return;
    
    twig_ion_free_mem(cedar->fd, mem);
}