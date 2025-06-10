/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Public Cedar direct hardware access API
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_H_
#define TWIG_H_

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

typedef struct {
    void *virt;
	uint32_t phys;
    uint32_t iommu_addr;
	size_t size;
    int ion_fd;
} twig_mem_t;

typedef struct twig_dev_t twig_dev_t;

twig_dev_t* twig_open(void);    
void twig_close(twig_dev_t *cedar);
int twig_wait_for_ve(twig_dev_t *cedar);
void *twig_get_ve_regs(twig_dev_t *cedar, int flag);
void twig_put_ve_regs(twig_dev_t *cedar);

twig_mem_t* twig_alloc_mem(size_t size);
void twig_flush_mem(int cedar_fd, twig_mem_t *mem);
void twig_free_mem(int cedar_fd, twig_mem_t *mem);

#endif // TWIG_H_