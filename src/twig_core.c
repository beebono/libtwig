#include "twig.h"
#include "twig_priv.h"
#include "twig_regs.h"

#define DEVICE          "/dev/cedar_dev"
#define VE_BASE_ADDR    0x01c0e000
#define VE_MODE_SELECT  0x00

#define EXPORT __attribute__ ((visibility ("default")))

static struct twig_dev {
	int fd;
	void *regs;
    struct twig_allocator *allocator;
    pthread_mutex_t register_lock;
} ve = { .fd = -1, .register_lock = PTHREAD_MUTEX_INITIALIZER };

void prepare_ve(void) {
    pthread_mutex_lock(&ve.register_lock);
    volatile vetop_reg_mode_sel_t* pVeModeSelect = (vetop_reg_mode_sel_t*)(ve.regs + VE_MODE_SELECT);
    pVeModeSelect->ddr_mode = 3;
	pVeModeSelect->rec_wr_mode = 1;
    pVeModeSelect->mode = 7;
    pthread_mutex_unlock(&ve.register_lock);
}

EXPORT struct twig_dev *twig_open(void) {
    if (ve.fd != -1)
		return NULL;

    ve.fd = open(DEVICE, O_RDWR);
    if (ve.fd == -1) {
        return NULL;
    }

    ioctl(ve.fd, IOCTL_SET_REFCOUNT, 0);
    if (ioctl(ve.fd, IOCTL_ENGINE_REQ, 0) < 0)
        goto err_close;
    
    ioctl(ve.fd, IOCTL_ENABLE_VE, 0);
    ioctl(ve.fd, IOCTL_SET_VE_FREQ, 180);
    ioctl(ve.fd, IOCTL_RESET_VE, 0);

    ve.regs = mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, ve.fd, VE_BASE_ADDR);
    if (ve.regs == MAP_FAILED)
        goto err_release;

    prepare_ve();

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

EXPORT void twig_close(struct twig_dev *dev) {
	if (dev->fd == -1)
		return;

	ioctl(dev->fd, IOCTL_ENGINE_REL, 0);

	munmap(dev->regs, 2048);
	dev->regs = NULL;

    if (dev->allocator) {
	    dev->allocator->destroy(dev->allocator);
        dev->allocator = NULL;
    }

	close(dev->fd);
	dev->fd = -1;
}

EXPORT int twig_wait_for_ve(struct twig_dev *dev) {
    if (!dev)
        return -1;

    int ret = ioctl(dev->fd, IOCTL_WAIT_VE_DE, 1);
    if (ret < 0)
        return -1;

    return 0;
}

EXPORT void* twig_get_ve_regs(struct twig_dev *dev) {
    if (!dev)
        return (void*)0x0;

    return (char*)dev->regs + 0x200;
}

EXPORT void twig_enable_decoder(struct twig_dev *dev) {
    if (!dev)
        return;

    volatile vetop_reg_mode_sel_t* pVeModeSelect;
	pthread_mutex_lock(&dev->register_lock);
	pVeModeSelect = (vetop_reg_mode_sel_t*)(dev->regs + VE_MODE_SELECT);
    pVeModeSelect->mode = 1;
    pthread_mutex_unlock(&dev->register_lock);	
}

EXPORT void twig_disable_decoder(struct twig_dev *dev) {
    if (!dev)
        return;

    volatile vetop_reg_mode_sel_t* pVeModeSelect;
	pthread_mutex_lock(&dev->register_lock);
	pVeModeSelect = (vetop_reg_mode_sel_t*)(dev->regs + VE_MODE_SELECT);
	pVeModeSelect->mode = 7;
	pthread_mutex_unlock(&dev->register_lock);
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