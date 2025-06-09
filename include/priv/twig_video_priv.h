/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_VIDEO_PRIV_H_
#define TWIG_VIDEO_PRIV_H_

#include "twig_video.h"

#define SBM_FRAME_FIFO_SIZE 64
#define MAX_FRAME_BUFFERS 32

typedef struct {
    twig_stream_frame_t frames[SBM_FRAME_FIFO_SIZE];
    int max_frame_num;
    int valid_frame_num;
    int unread_frame_num;
    int read_pos;
    int write_pos;
    int flush_pos;
} twig_stream_fifo_t;

struct twig_sbm_t {
    twig_mem_t *stream_buffer;
    uint8_t *write_addr;
    uint8_t *buffer_end;
    size_t buffer_size;
    size_t valid_data_size;
    twig_stream_fifo_t frame_fifo;
    pthread_mutex_t mutex;
};

typedef struct {
    twig_picture_t picture;
    int is_used;
    int is_reference;
    int poc;
    int frame_num;
    int ref_count;
} twig_frame_info_t;

struct twig_fbm_t {
    twig_frame_info_t frames[MAX_FRAME_BUFFERS];
    int max_frame_count;
    int empty_frame_count;
    int y_buffer_size;
    int c_buffer_size;
    int y_stride;
    int c_stride;
    pthread_mutex_t mutex;
};

typedef enum {
    H264_STEP_CONFIG_VBV = 0,
    H264_STEP_UPDATE_DATA = 1,
    H264_STEP_SEARCH_NALU = 2,
    H264_STEP_SEARCH_AVC_NALU = 3,
    H264_STEP_PROCESS_DECODE_RESULT = 4
} twig_h264_step_t;

typedef enum {
    NAL_SLICE = 1,
    NAL_DPA = 2,
    NAL_DPB = 3,
    NAL_DPC = 4,
    NAL_IDR_SLICE = 5,
    NAL_SEI = 6,
    NAL_SPS = 7,
    NAL_PPS = 8,
    NAL_AUD = 9,
    NAL_END_SEQUENCE = 10,
    NAL_END_STREAM = 11,
    NAL_FILLER_DATA = 12
} twig_nal_type_t;

typedef struct {
    void *ve_regs;
    int ve_active;
    uint8_t *read_ptr;
    uint8_t *vbv_buffer;
    uint8_t *vbv_buffer_end;
    size_t vbv_buffer_size;
    size_t vbv_data_size;
    twig_h264_step_t decode_step;
    int frame_decode_status;
    int cur_slice_num;
    int need_find_sps;
    int need_find_pps;
    int need_find_iframe;
    int nal_ref_idc;
    twig_nal_type_t nal_unit_type;
    int is_avc_format;
    int nal_length_size;
    int mb_width;
    int mb_height;
    int frame_width;
    int frame_height;
    int max_ref_frames;
    int frame_mbs_only_flag;
    int direct_8x8_inference_flag;
    twig_mem_t *mv_col_buf;
    twig_mem_t *mb_field_intra_buf;
    twig_mem_t *mb_neighbor_buf;
    twig_mem_t *deblock_dram_buf;
    twig_mem_t *intra_pred_dram_buf;
    twig_frame_info_t *cur_picture;
    int decoded_frames;
    int keyframes_only;
} twig_h264_ctx_t;

struct twig_video_engine_t {
    twig_dev_t *device;
    twig_video_config_t config;
    twig_video_info_t video_info;
    twig_sbm_t *sbm;
    twig_fbm_t *fbm;
    twig_h264_ctx_t *h264_ctx;
    pthread_mutex_t mutex;
};

int twig_sbm_create(twig_video_engine_t *engine, twig_sbm_config_t *config);
void twig_sbm_destroy(twig_sbm_t *sbm);

int twig_fbm_create(twig_video_engine_t *engine, twig_fbm_config_t *config);
void twig_fbm_destroy(twig_fbm_t *fbm);

int twig_h264_init(twig_video_engine_t *engine);
void twig_h264_destroy(twig_h264_ctx_t *ctx);
twig_result_t twig_h264_decode(twig_video_engine_t *engine, int end_of_stream, int decode_key_frames_only,
                                int skip_b_frames_if_delay, int64_t current_time_us);

#endif // TWIG_VIDEO_PRIV_H_