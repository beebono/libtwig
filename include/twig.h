/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_H_
#define TWIG_H_

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

typedef enum { 
    TWIG_MEM_ANY = 0,
    TWIG_MEM_VE = 1,
    TWIG_MEM_ION = 2,
    TWIG_MEM_BOTH = 3,
} twig_mem_type_t;

typedef struct {
    void *virt;
	uint32_t phys;
	size_t size;
    int ion_fd;
    twig_mem_type_t type;
} twig_mem_t;

typedef struct twig_allocator twig_allocator_t;
typedef struct twig_dev twig_dev_t;

twig_dev_t* twig_open(twig_mem_type_t mem_type);    
void twig_close(twig_dev_t *dev);
unsigned int twig_get_ve_version(twig_dev_t *dev);
int twig_wait_for_ve(twig_dev_t *dev);
void *twig_get_ve_regs(twig_dev_t *dev);
void twig_put_ve_regs(twig_dev_t *dev);

twig_mem_t* twig_alloc_mem(twig_dev_t *dev, size_t size);
void twig_free_mem(twig_mem_t *mem);
void twig_flush_cache(twig_mem_t *mem);

static inline uint32_t twig_get_phys_addr(twig_mem_t *mem) {
    return mem ? mem->phys : 0;
}

static inline void* twig_get_virt_addr(twig_mem_t *mem) {
    return mem ? mem->virt : NULL;
}

static inline int twig_get_ion_fd(twig_mem_t *mem) {
    return mem ? mem->ion_fd : -1;
}

#endif