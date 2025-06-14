/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Private Cedar hardware accelerated H.264 decoding API
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_DEC_H_
#define TWIG_DEC_H_

#define NAL_SPS 7
#define NAL_PPS 8
#define NAL_IDR_SLICE 5
#define NAL_SLICE 1

#define MAX_FRAME_POOL_SIZE 20

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
    int long_term_idx;
} twig_frame_t;

typedef struct {
    twig_frame_t frames[MAX_FRAME_POOL_SIZE];
    int allocated_count;
    int frame_width;
    int frame_height;
    size_t frame_size;
    twig_frame_t *short_refs[16];
    twig_frame_t *long_refs[16];
    int short_count;
    int long_count;
    int max_long_term_frame_idx;
    int prev_frame_num;
    int max_frame_num; 
} twig_frame_pool_t;

typedef struct {
    uint8_t profile_idc;
    uint8_t level_idc;
    uint8_t chroma_format_idc;
    uint8_t bit_depth_luma_minus8;
    uint8_t bit_depth_chroma_minus8;
    uint16_t pic_width_in_mbs_minus1;
    uint16_t pic_height_in_map_units_minus1;
    uint16_t pic_height_in_mbs_minus1;
    uint8_t frame_mbs_only_flag;
    uint8_t mb_adaptive_frame_field_flag;
    uint8_t direct_8x8_inference_flag;
    uint8_t frame_cropping_flag;
    uint16_t frame_crop_left_offset;
    uint16_t frame_crop_right_offset;
    uint16_t frame_crop_top_offset;
    uint16_t frame_crop_bottom_offset;
    uint8_t log2_max_frame_num_minus4;
    uint8_t pic_order_cnt_type;
    uint8_t log2_max_pic_order_cnt_lsb_minus4;
    uint8_t delta_pic_order_always_zero_flag;
    uint8_t max_num_ref_frames;
    uint8_t gaps_in_frame_num_value_allowed_flag;
} twig_h264_sps_t;

typedef struct {
    uint8_t pic_parameter_set_id;
    uint8_t seq_parameter_set_id;
    uint8_t entropy_coding_mode_flag;
    uint8_t bottom_field_pic_order_in_frame_present_flag;
    uint8_t num_slice_groups_minus1;
    uint8_t num_ref_idx_l0_default_active_minus1;
    uint8_t num_ref_idx_l1_default_active_minus1;
    uint8_t weighted_pred_flag;
    uint8_t weighted_bipred_idc;
    int8_t pic_init_qp_minus26;
    int8_t pic_init_qs_minus26;
    int8_t chroma_qp_index_offset;
    uint8_t deblocking_filter_control_present_flag;
    uint8_t constrained_intra_pred_flag;
    uint8_t redundant_pic_cnt_present_flag;
    uint8_t transform_8x8_mode_flag;
    uint8_t pic_scaling_matrix_present_flag;
    int8_t second_chroma_qp_index_offset;
} twig_h264_pps_t;

typedef struct {
    uint8_t nal_unit_type;
    uint16_t first_mb_in_slice;
    uint8_t slice_type;
    uint8_t pic_parameter_set_id;
    uint16_t frame_num;
    uint8_t field_pic_flag;
    uint8_t bottom_field_pic_flag;
    uint16_t idr_pic_id;
    uint32_t pic_order_cnt_lsb;
    int32_t delta_pic_order_cnt_bottom;
    int32_t delta_pic_order_cnt[2];
    uint8_t redundant_pic_cnt;
    uint8_t direct_spatial_mv_pred_flag;
    uint8_t num_ref_idx_active_override_flag;
    uint8_t num_ref_idx_l0_active_minus1;
    uint8_t num_ref_idx_l1_active_minus1;
    uint8_t cabac_init_idc;
    int8_t slice_qp_delta;
    uint8_t sp_for_switch_flag;
    int8_t slice_qs_delta;
    uint8_t disable_deblocking_filter_idc;
    int8_t slice_alpha_c0_offset_div2;
    int8_t slice_beta_offset_div2;
    uint8_t first_slice_in_pic;
} twig_h264_hdr_t;

typedef struct {
    int prev_poc_msb;
    int prev_poc_lsb;
    int prev_frame_num;
    int frame_num_offset;
} twig_ref_state_t;

typedef struct {
    int memory_management_control_operation;
    int difference_of_pic_nums_minus1;
    int long_term_pic_num;  
    int long_term_frame_idx;
    int max_long_term_frame_idx_plus1;
} twig_mmco_cmd_t;

struct twig_h264_decoder_t {
    twig_dev_t *cedar;
    void *ve_regs;
    twig_mem_t *extra_buf;
    twig_h264_hdr_t *hdr;
    twig_h264_sps_t *sps;
    twig_h264_pps_t *pps;
    uint16_t coded_width, coded_height;
    uint16_t last_width, last_height;
    int is_default_scaling;
    twig_frame_pool_t frame_pool;
    int pool_initialized;
    twig_ref_state_t ref_state;
    twig_mmco_cmd_t mmco_commands[32];
    int mmco_count;
    int long_term_reference_flag;
};

void *twig_get_ve_regs(twig_dev_t *cedar, int flag);
int twig_wait_for_ve(twig_dev_t *cedar);
void twig_put_ve_regs(twig_dev_t *cedar);

uint32_t twig_get_bits_hw(void *regs, int num);
uint32_t twig_get_1bit_hw(void *regs);
uint32_t twig_get_ue_golomb_hw(void *regs);
int32_t twig_get_se_golomb_hw(void *regs);

int twig_frame_pool_init(twig_frame_pool_t *pool, int width, int height);
twig_frame_t *twig_frame_pool_get(twig_frame_pool_t *pool, twig_dev_t *cedar, uint16_t pwimm1);
void twig_mark_frame_unref(twig_frame_pool_t *pool, twig_frame_t *frame);
void twig_remove_stale_frames(twig_frame_pool_t *pool);
void twig_frame_pool_cleanup(twig_frame_pool_t *pool, twig_dev_t *cedar);

int twig_parse_mmco_commands(void *regs, twig_mmco_cmd_t *mmco_list, int *mmco_count);
void twig_execute_mmco_commands(twig_h264_decoder_t *decoder, twig_frame_t *current_frame);
void twig_write_framebuffer_list(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t *output_frame, int output_poc);
void twig_build_ref_lists_from_pool(twig_frame_pool_t *pool, twig_slice_type_t slice_type, twig_frame_t **list0, int *l0_count,
                                    twig_frame_t **list1, int *l1_count, int current_poc);
void twig_write_ref_list0_registers(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t **ref_list0, int l0_count);
void twig_write_ref_list1_registers(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t **ref_list1, int l1_count);

#endif // TWIG_DEC_H_