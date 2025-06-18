/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Supplemental hardware bitreader functions
 *
 * Garbage code by Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_BITS_H_
#define TWIG_BITS_H_

#include "twig_regs.h"

static uint32_t twig_get_bits(void *regs, int num) {
    twig_writel(regs, H264_TRIGGER, (2 << 0)| (num << 8));

    while (twig_readl(regs, H264_STATUS) & (1 << 8))
        usleep(1);

    return twig_readl(regs, H264_BASIC_BITS);
}

static uint32_t twig_get_1bit(void *regs) {
    return twig_get_bits(regs, 1);
}

static void twig_skip_bits(void *regs, int num) {
	int count = 0;
	while (count < num) {
		int tmp = (num - count <= 32) ? num - count : 32;
		twig_writel(regs, H264_TRIGGER, (3 << 0) | (tmp << 8));
		while (twig_readl(regs, H264_STATUS) & (1 << 8))
			usleep(1);

		count += tmp;
	}   
}

static void twig_skip_1bit(void *regs) {
    twig_skip_bits(regs, 1);
}

static int32_t twig_get_se(void *regs) {
    twig_writel(regs, H264_TRIGGER, (4 << 0));

    while (twig_readl(regs, H264_STATUS) & (1 << 8))
        usleep(1);

    return twig_readl(regs, H264_BASIC_BITS);
}

static uint32_t twig_get_ue(void *regs) {
    twig_writel(regs, H264_TRIGGER, (5 << 0));

    while (twig_readl(regs, H264_STATUS) & (1 << 8))
     usleep(1);

    return twig_readl(regs, H264_BASIC_BITS);
}

#endif // TWIG_BITS_H_