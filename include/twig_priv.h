/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_PRIV_H_
#define TWIG_PRIV_H_

#include "twig.h"
#include "allwinner/cedardev_api.h"

#define DEVICE          "/dev/cedar_dev"
#define VE_BASE_ADDR    0x01c0e000

#define EXPORT __attribute__((visibility ("default")))

struct cache_range {
    uint64_t start;
    uint64_t end;
};

struct user_iommu_param {
    int fd;
    unsigned int iommu_addr;
};

struct twig_allocator_t {
    twig_mem_t *(*mem_alloc)(twig_allocator_t *allocator, size_t size);
    void (*mem_free)(twig_allocator_t *allocator, twig_mem_t *mem);
	void (*destroy)(twig_allocator_t *allocator);
    int dev_fd;
};

struct twig_dev_t {
	int fd;
	void *regs;
    twig_allocator_t *allocator;
};

twig_allocator_t *twig_allocator_ion_create(void);

#endif