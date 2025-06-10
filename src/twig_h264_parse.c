#include "twig.h"
#include "supp/twig_bits.h"

typedef struct {
    uint8_t profile_idc;
    uint8_t level_idc;
    uint32_t seq_parameter_set_id;

    uint32_t chroma_format_idc;
    uint8_t separate_colour_plane_flag;
    uint32_t bit_depth_luma_minus8;
    uint32_t bit_depth_chroma_minus8;
    uint8_t qpprime_y_zero_transform_bypass_flag;
    uint8_t seq_scaling_matrix_present_flag;
    uint8_t seq_scaling_list_present_flag[12];
    uint8_t scaling_list_4x4[6][16];
    uint8_t scaling_list_8x8[6][64];

    uint32_t log2_max_frame_num_minus4;
    uint32_t pic_order_cnt_type;
    
    uint32_t log2_max_pic_order_cnt_lsb_minus4;

    uint8_t delta_pic_order_always_zero_flag;
    int32_t offset_for_non_ref_pic;
    int32_t offset_for_top_to_bottom_field;
    uint32_t num_ref_frames_in_pic_order_cnt_cycle;
    int32_t offset_for_ref_frame[256];
    
    uint32_t max_num_ref_frames;
    uint8_t gaps_in_frame_num_value_allowed_flag;
    uint32_t pic_width_in_mbs_minus1;
    uint32_t pic_height_in_map_units_minus1;
    uint8_t frame_mbs_only_flag;
    uint8_t mb_adaptive_frame_field_flag;
    uint8_t direct_8x8_inference_flag;
    uint8_t frame_cropping_flag;
    uint32_t frame_crop_left_offset;
    uint32_t frame_crop_right_offset;
    uint32_t frame_crop_top_offset;
    uint32_t frame_crop_bottom_offset;
} twig_h264_sps_t;

typedef struct {
    uint32_t pic_parameter_set_id;
    uint32_t seq_parameter_set_id;
    uint8_t entropy_coding_mode_flag;
    uint8_t bottom_field_pic_order_in_frame_present_flag;
    uint32_t num_slice_groups_minus1;

    uint32_t slice_group_map_type;
    uint32_t run_length_minus1[8];
    uint32_t top_left[8];
    uint32_t bottom_right[8];
    uint8_t slice_group_change_direction_flag;
    uint32_t slice_group_change_rate_minus1;
    uint32_t pic_size_in_map_units_minus1;
    uint8_t *slice_group_id;
    
    uint32_t num_ref_idx_l0_default_active_minus1;
    uint32_t num_ref_idx_l1_default_active_minus1;
    uint8_t weighted_pred_flag;
    uint8_t weighted_bipred_idc;
    int32_t pic_init_qp_minus26;
    int32_t pic_init_qs_minus26;
    int32_t chroma_qp_index_offset;
    uint8_t deblocking_filter_control_present_flag;
    uint8_t constrained_intra_pred_flag;
    uint8_t redundant_pic_cnt_present_flag;

    uint8_t transform_8x8_mode_flag;
    uint8_t pic_scaling_matrix_present_flag;
    uint8_t pic_scaling_list_present_flag[12];
    uint8_t scaling_list_4x4[6][16];
    uint8_t scaling_list_8x8[6][64];
    int32_t second_chroma_qp_index_offset;

    uint8_t has_extended_fields;
} twig_h264_pps_t;

typedef struct {
    twig_h264_sps_t *sps_list[32];
    twig_h264_pps_t *pps_list[256];
    twig_h264_sps_t cur_sps;
    twig_h264_pps_t cur_pps;
    uint8_t prev_sps_id;
    uint8_t prev_pps_id;
    
    uint8_t nal_ref_idc;
    uint8_t nal_unit_type;
} twig_h264_parser_t;

static int twig_find_start_code(const uint32_t *data, int size, int start) {
	int pos, leading_zeros = 0;
	for (pos = start; pos < size; pos++) {
		if (data[pos] == 0x00)
			leading_zeros++;
		else if (data[pos] == 0x01 && leading_zeros >= 2)
			return pos - 2;
		else
			zeros = 0;
	}
	return -1;
}