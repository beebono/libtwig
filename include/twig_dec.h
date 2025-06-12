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

#define MAX_FRAME_POOL_SIZE          20

typedef enum {
    SLICE_TYPE_P,
    SLICE_TYPE_B,
    SLICE_TYPE_I,
    SLICE_TYPE_SP,
    SLICE_TYPE_SI
} twig_slice_type_t;

typedef enum {
    FRAME_STATE_FREE,
    FRAME_STATE_DECODER_HELD,
    FRAME_STATE_APP_HELD
} twig_frame_state_t;

typedef struct {
    twig_mem_t *buffer;
    twig_mem_t *extra_data;
    twig_frame_state_t state;
    int frame_idx;
    int frame_num;
    int poc;
    int is_reference;
    int is_long_term;
} twig_frame_t;

typedef struct {
    twig_frame_t frames[MAX_FRAME_POOL_SIZE];
    int allocated_count;
    int frame_width;
    int frame_height;
    size_t frame_size;
} twig_frame_pool_t;

typedef struct twig_dev_t twig_dev_t;
typedef struct twig_h264_decoder_t twig_h264_decoder_t;

twig_h264_decoder_t *twig_h264_decoder_init(twig_dev_t *cedar, int max_width);
twig_mem_t *twig_h264_decode_frame(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf);
int twig_get_frame_res(twig_h264_decoder_t *decoder, int *width, int *height);
void twig_h264_return_frame(twig_h264_decoder_t *decoder, twig_mem_t *output_buf);
void twig_h264_decoder_destroy(twig_h264_decoder_t* decoder);

#endif // TWIG_DEC_H_