/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Private memory allocation abstraction layer
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_MEM_PRIV_H_
#define TWIG_MEM_PRIV_H_

#include "twig.h"
#include "allwinner/ion.h"
#include "allwinner/cedardev_api.h"

#define ION_IOC_SUNXI_PHYS_ADDR	7

struct sunxi_phys_data {
	int handle;
	unsigned int phys_addr;
	unsigned int size;
};

struct user_iommu_param {
    int fd;
    unsigned int iommu_addr;
};

struct cache_range {
    uint64_t start;
    uint64_t end;
};

struct twig_allocator_t {
    twig_mem_t *(*mem_alloc)(twig_allocator_t *allocator, size_t size);
    void (*mem_flush)(twig_allocator_t *allocator, twig_mem_t *mem);
    void (*mem_free)(twig_allocator_t *allocator, twig_mem_t *mem);
	void (*destroy)(twig_allocator_t *allocator);
    int cedar_fd;
    int dev_fd;
};

twig_allocator_t *twig_allocator_ion_create(twig_dev_t *dev);

#endif // TWIG_PRIV_H_