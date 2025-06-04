#include "twig.h"
#include "twig_priv.h"
#include "twig_regs.h"

#define DEVICE          "/dev/cedar_dev"
#define VE_MODE_SELECT  0x00
#define VE_RESET	    0x04

#define EXPORT __attribute__ ((visibility ("default")))

static struct twig_dev {
	int fd;
	void *regs;
	unsigned int version;
    twig_mem_type_t mem_type;
    struct twig_allocator *allocator_ve;
	struct twig_allocator *allocator_ion;
    pthread_mutex_t register_lock;
} ve = { .fd = -1, .register_lock = PTHREAD_MUTEX_INITIALIZER };

struct cedarv_env_infomation info;

void reset_ve(void) {
    ioctl(ve.fd, IOCTL_RESET_VE, 0);
    pthread_mutex_lock(&ve.register_lock);
    volatile vetop_reg_mode_sel_t* pVeModeSelect = (vetop_reg_mode_sel_t*)(info.address_macc + VE_MODE_SELECT);
    pVeModeSelect->ddr_mode = 3;
	pVeModeSelect->rec_wr_mode = 1;
    pVeModeSelect->mode = 7;
    pthread_mutex_unlock(&ve.register_lock);
}

EXPORT struct twig_dev *twig_open(twig_mem_type_t mem_type) {
    if (ve.fd != -1)
		return NULL;

    ve.fd = open(DEVICE, O_RDWR);
    if (ve.fd == -1) {
        return NULL;
    }

    ioctl(ve.fd, IOCTL_SET_REFCOUNT, 0);
    if (ioctl(ve.fd, IOCTL_ENGINE_REQ, 0) < 0)
        goto err_close;
        
    ioctl(ve.fd, IOCTL_GET_ENV_INFO, (unsigned long)&info);
    info.address_macc = (unsigned int)mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, ve.fd, (int)info.address_macc);
    ve.regs = (void*)info.address_macc;
    if (ve.regs == MAP_FAILED)
        goto err_release;

    volatile unsigned int value = *((unsigned int*)((char *)info.address_macc + 0xf0));
    ve.version = (value >> 16);

    reset_ve();

    if (mem_type == TWIG_MEM_ANY) {
        ve.allocator_ve = twig_allocator_ve_create(ve.fd, &info);
        if (!ve.allocator_ve) {
            ve.allocator_ion = twig_allocator_ion_create();
            if (!ve.allocator_ion)
                goto err_unmap;
        }
    }

    if (mem_type == TWIG_MEM_VE || mem_type == TWIG_MEM_BOTH) {
        ve.allocator_ve = twig_allocator_ve_create(ve.fd, &info);
        if (!ve.allocator_ve) 
            goto err_unmap;
    }
    if (mem_type == TWIG_MEM_ION || mem_type == TWIG_MEM_BOTH) {
        ve.allocator_ion = twig_allocator_ion_create();
        if (!ve.allocator_ion) {
            if (ve.allocator_ve)
                goto err_destroy_ve_alloc;

            goto err_unmap;
        }
    }

    ve.mem_type = mem_type;

    return &ve;

err_destroy_ve_alloc:
    ve.allocator_ve->destroy(ve.allocator_ve);
    ve.allocator_ve = NULL;

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

    if (dev->allocator_ion) {
	    dev->allocator_ion->destroy(dev->allocator_ion);
        dev->allocator_ion = NULL;
    }

    if (dev->allocator_ve) {
        dev->allocator_ve->destroy(dev->allocator_ve);
        dev->allocator_ve = NULL;
    }

	close(dev->fd);
	dev->fd = -1;
}

EXPORT unsigned int twig_get_ve_version(struct twig_dev *dev) {
    if (!dev)
        return 0x0;

    return dev->version;
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