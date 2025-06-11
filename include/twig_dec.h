/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Public Cedar hardware accelerated H.264 decoding API
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_DEC_H_
#define TWIG_DEC_H_

typedef struct twig_dev_t twig_dev_t;
typedef struct twig_mem_t twig_mem_t;
typedef struct twig_h264_decoder_t twig_h264_decoder_t;

twig_h264_decoder_t *twig_h264_decoder_init(twig_dev_t *cedar, int max_width);
twig_mem_t *twig_h264_decode_frame(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf);
void twig_h264_return_frame(twig_h264_decoder_t *decoder, twig_mem_t *output_buf);
void twig_h264_decoder_destroy(twig_h264_decoder_t* decoder);

#endif // TWIG_DEC_H_