/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Public Cedar direct hardware access API
 *
 * Garbage code by Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_H_
#define TWIG_H_

#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

typedef struct {
    void *virt_addr;
    uint32_t phys_addr, iommu_addr;
    size_t size;
    int ion_fd;
} twig_mem_t;

typedef struct twig_dev_t twig_dev_t;
typedef struct twig_h264_decoder_t twig_h264_decoder_t;

twig_dev_t *twig_open(void);    
void twig_close(twig_dev_t *cedar);

twig_mem_t *twig_alloc_mem(twig_dev_t *cedar, size_t size);
void twig_flush_mem(twig_mem_t *mem);
void twig_free_mem(twig_dev_t *cedar, twig_mem_t *mem);

twig_h264_decoder_t *twig_h264_decoder_init(twig_dev_t *cedar);
twig_mem_t *twig_h264_decode_frame(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf);
int twig_h264_get_frame_res(twig_h264_decoder_t *decoder, int *width, int *height);
void twig_h264_return_frame(twig_h264_decoder_t *decoder, twig_mem_t *output_buf);
void twig_h264_decoder_destroy(twig_h264_decoder_t* decoder);

#endif // TWIG_H_