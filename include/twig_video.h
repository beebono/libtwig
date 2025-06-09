/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Public video decoding API
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_VIDEO_H_
#define TWIG_VIDEO_H_

#include "twig.h"

typedef struct {
    int width;
    int height;
    int fps_num;
    int fps_den;
    uint8_t *extra_data;
    int extra_data_size;
    int max_ref_frames;
} twig_video_info_t;

typedef struct {
    int decode_key_frames_only;
    int skip_b_frames_if_delay;
    int max_output_width;
    int max_output_height;
    int enable_performance_mode;
} twig_video_config_t;

typedef struct {
    size_t buffer_size;
    int max_frame_num;
} twig_sbm_config_t;

typedef struct {
    int frame_count;
    int width;
    int height;
} twig_fbm_config_t;

typedef struct {
    uint8_t *data;
    size_t size;
    int64_t pts;
    int is_key_frame;
    int is_first_part;
    int is_last_part;
} twig_stream_frame_t;

typedef struct {
    twig_mem_t *y_buf;
    twig_mem_t *c_buf;
    int width;
    int height;
    int y_stride;
    int c_stride;
    int64_t pts;
    int is_key_frame;
    int poc;
    int top_offset;
    int bottom_offset;
    int left_offset;
    int right_offset;
} twig_picture_t;

typedef enum {
    TWIG_RESULT_OK = 0,
    TWIG_RESULT_FRAME_DECODED = 1,
    TWIG_RESULT_KEYFRAME_DECODED = 2,
    TWIG_RESULT_NO_FRAME_BUFFER = 3,
    TWIG_RESULT_NO_BITSTREAM = 4,
    TWIG_RESULT_UNSUPPORTED = -1,
    TWIG_RESULT_FRAME_ERROR = -2,
    TWIG_RESULT_INVALID_PARAM = -3,
    TWIG_RESULT_NO_MEMORY = -4
} twig_result_t;

typedef struct twig_video_engine_t twig_video_engine_t;
typedef struct twig_sbm_t twig_sbm_t;
typedef struct twig_fbm_t twig_fbm_t;

twig_video_engine_t *twig_video_engine_create(twig_dev_t *dev, twig_video_config_t *config, twig_video_info_t *video_info);
void twig_video_engine_destroy(twig_video_engine_t *engine);
void twig_video_engine_reset(twig_video_engine_t *engine);
int twig_video_engine_set_sbm(twig_video_engine_t *engine, twig_sbm_config_t *sbm_config); 
twig_fbm_t *twig_video_engine_get_fbm(twig_video_engine_t *engine);

twig_result_t twig_video_engine_decode(twig_video_engine_t *engine, int end_of_stream, int decode_key_frames_only,
                                        int skip_b_frames_if_delay, int64_t current_time_us);

int twig_sbm_request_buffer(twig_video_engine_t *engine, int require_size, uint8_t **buf, int *buf_size);
int twig_sbm_add_stream_frame(twig_video_engine_t *engine, twig_stream_frame_t *frame_info);
int twig_sbm_stream_frame_num(twig_video_engine_t *engine);
int twig_sbm_stream_data_size(twig_video_engine_t *engine);

twig_picture_t *twig_fbm_request_picture(twig_video_engine_t *engine, int timeout_ms);
int twig_fbm_return_picture(twig_video_engine_t *engine, twig_picture_t *picture);
int twig_fbm_num_total_frames(twig_video_engine_t *engine);
int twig_fbm_num_empty_frames(twig_video_engine_t *engine);

#endif // TWIG_VIDEO_H_