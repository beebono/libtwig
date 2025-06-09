/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Private Cedar direct hardware access abstraction layer
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_PRIV_H_
#define TWIG_PRIV_H_

#include "twig.h"

#define DEVICE                  "/dev/cedar_dev"
#define VE_BASE_ADDR            0x01c0e000

struct twig_dev_t {
	int fd, active;
	void *regs;
    twig_allocator_t *allocator;
};

#endif // TWIG_PRIV_H_