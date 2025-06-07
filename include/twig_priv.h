/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_PRIV_H_
#define TWIG_PRIV_H_

#include "allwinner/cedardev_api.h"

#define DEVICE          "/dev/cedar_dev"
#define VE_BASE_ADDR    0x01c0e000

#define EXPORT __attribute__((visibility ("default")))

typedef struct {
    twig_mem_t *(*mem_alloc)(twig_allocator_t *allocator, size_t size);
    void (*mem_free)(twig_allocator_t *allocator, twig_mem_t *mem);
	void (*mem_flush)(twig_allocator_t *allocator, twig_mem_t *mem);
	void (*destroy)(twig_allocator_t *allocator);
    int dev_fd;
} twig_allocator_t;

typedef struct {
	int fd;
	void *regs;
    twig_allocator_t *allocator;
} twig_dev_t;

twig_allocator_t *twig_allocator_ion_create(void);

#endif