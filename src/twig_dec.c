#include "twig.h"
#include "twig_bits.h"
#include "twig_dec.h"
#include "twig_regs.h"

#define EXPORT __attribute__((visibility ("default")))

#define NAL_SPS 7
#define NAL_PPS 8
#define NAL_IDR_SLICE 5
#define NAL_SLICE 1

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
    twig_frame_t *ref_list0[16];
    twig_frame_t *ref_list1[16];
    int l0_count;
    int l1_count;
};

void *twig_get_ve_regs(twig_dev_t *cedar, int flag);
int twig_wait_for_ve(twig_dev_t *cedar);
void twig_put_ve_regs(twig_dev_t *cedar);

int twig_frame_pool_init(twig_frame_pool_t *pool, int width, int height);
twig_frame_t *twig_frame_pool_get(twig_frame_pool_t *pool, twig_dev_t *cedar, uint16_t pwimm1);
twig_mem_t *twig_send_frame(twig_frame_t *frame, int frame_num, int poc, int is_reference);
void twig_mark_frame_return(twig_frame_pool_t *pool, twig_mem_t *buffer, twig_dev_t *cedar);
void twig_mark_frame_unref(twig_frame_t *frame);
void twig_frame_pool_cleanup(twig_frame_pool_t *pool, twig_dev_t *cedar);

void twig_write_framebuffer_list(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t *output_frame, int output_poc);
void twig_build_ref_lists_from_pool(twig_frame_pool_t *pool, twig_slice_type_t slice_type, twig_frame_t **list0, int *l0_count,
                                    twig_frame_t **list1, int *l1_count, int current_poc);
void twig_write_ref_list0_registers(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t **ref_list0, int l0_count);
void twig_write_ref_list1_registers(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t **ref_list1, int l1_count);

static int twig_find_start_code(const uint8_t *data, int size, int start) {
	int pos, leading_zeros = 0;
	for (pos = start; pos < size; pos++) {
		if (data[pos] == 0x00)
			leading_zeros++;
		else if (data[pos] == 0x01 && leading_zeros >= 2)
			return pos - 2;
		else
			leading_zeros = 0;
	}
	return -1;
}

static int twig_parse_sps(const uint8_t *data, size_t size, twig_h264_sps_t *sps) {
    twig_bitreader_t br;

    twig_bitreader_init(&br, data + 1, size - 1);
    
    sps->profile_idc = twig_get_bits(&br, 8);
    twig_skip_bits(&br, 8);
    sps->level_idc = twig_get_bits(&br, 8);
    
    twig_get_ue_golomb(&br);
    
    if (sps->profile_idc >= 100) {
        sps->chroma_format_idc = twig_get_ue_golomb(&br);
        if (sps->chroma_format_idc == 3) {
            twig_skip_1bit(&br);
        }
        sps->bit_depth_luma_minus8 = twig_get_ue_golomb(&br);
        sps->bit_depth_chroma_minus8 = twig_get_ue_golomb(&br);
        twig_skip_1bit(&br);
        
        if (twig_get_1bit(&br)) {
            // TODO: Cedar VE needs the scaling lists if not default, assume default for now
            for (int i = 0; i < 8; i++) {
                if (twig_get_1bit(&br))
                    twig_skip_bits(&br, (i < 6) ? 16 : 64);
            }
        }
    } else {
        sps->chroma_format_idc = 1;
        sps->bit_depth_luma_minus8 = 0;
        sps->bit_depth_chroma_minus8 = 0;
    }
    
    sps->log2_max_frame_num_minus4 = twig_get_ue_golomb(&br);
    sps->pic_order_cnt_type = twig_get_ue_golomb(&br);
    
    if (sps->pic_order_cnt_type == 0) {
        sps->log2_max_pic_order_cnt_lsb_minus4 = twig_get_ue_golomb(&br);
    } else if (sps->pic_order_cnt_type == 1) {
        sps->delta_pic_order_always_zero_flag = twig_get_1bit(&br);
        twig_get_se_golomb(&br);
        twig_get_se_golomb(&br);
        uint32_t num_ref_frames_in_poc_cycle = twig_get_ue_golomb(&br);
        for (uint32_t i = 0; i < num_ref_frames_in_poc_cycle; i++) {
            twig_get_se_golomb(&br);
        }
    }
    
    sps->max_num_ref_frames = twig_get_ue_golomb(&br);
    sps->gaps_in_frame_num_value_allowed_flag = twig_get_1bit(&br);
    sps->pic_width_in_mbs_minus1 = twig_get_ue_golomb(&br);
    sps->pic_height_in_map_units_minus1 = twig_get_ue_golomb(&br);
    sps->frame_mbs_only_flag = twig_get_1bit(&br);

    if (!sps->frame_mbs_only_flag) {
        sps->mb_adaptive_frame_field_flag = twig_get_1bit(&br);
        sps->pic_height_in_mbs_minus1 = (sps->pic_height_in_map_units_minus1 + 1) * 2 - 1;
    } else {
        sps->pic_height_in_mbs_minus1 = sps->pic_height_in_map_units_minus1;
    }
    
    sps->direct_8x8_inference_flag = twig_get_1bit(&br);
    sps->frame_cropping_flag = twig_get_1bit(&br);
    
    if (sps->frame_cropping_flag) {
        sps->frame_crop_left_offset = twig_get_ue_golomb(&br);
        sps->frame_crop_right_offset = twig_get_ue_golomb(&br);
        sps->frame_crop_top_offset = twig_get_ue_golomb(&br);
        sps->frame_crop_bottom_offset = twig_get_ue_golomb(&br);
    }
    
    return 0;
}

static int twig_parse_pps(const uint8_t *data, size_t size, twig_h264_pps_t *pps) {
    twig_bitreader_t br;
    
    twig_bitreader_init(&br, data + 1, size - 1);
    
    pps->pic_parameter_set_id = twig_get_ue_golomb(&br);
    pps->seq_parameter_set_id = twig_get_ue_golomb(&br);
    pps->entropy_coding_mode_flag = twig_get_1bit(&br);
    pps->bottom_field_pic_order_in_frame_present_flag = twig_get_1bit(&br);
    pps->num_slice_groups_minus1 = twig_get_ue_golomb(&br);
    
    if (pps->num_slice_groups_minus1 > 0) {
        // Skip slice group map parameters for now
        return -1; // Not supported yet
    }
    
    pps->num_ref_idx_l0_default_active_minus1 = twig_get_ue_golomb(&br);
    pps->num_ref_idx_l1_default_active_minus1 = twig_get_ue_golomb(&br);
    pps->weighted_pred_flag = twig_get_1bit(&br);
    pps->weighted_bipred_idc = twig_get_bits(&br, 2);
    pps->pic_init_qp_minus26 = twig_get_se_golomb(&br);
    pps->pic_init_qs_minus26 = twig_get_se_golomb(&br);
    pps->chroma_qp_index_offset = twig_get_se_golomb(&br);
    pps->deblocking_filter_control_present_flag = twig_get_1bit(&br);
    pps->constrained_intra_pred_flag = twig_get_1bit(&br);
    pps->redundant_pic_cnt_present_flag = twig_get_1bit(&br);
    
    if (twig_bitreader_bits_left(&br) > 0) {
        pps->transform_8x8_mode_flag = twig_get_1bit(&br);
        pps->pic_scaling_matrix_present_flag = twig_get_1bit(&br);
        if (pps->pic_scaling_matrix_present_flag) {
            // TODO: Cedar VE needs the scaling lists if not default, assume default for now
            int count = 6 + (pps->transform_8x8_mode_flag ? 2 : 0);
            for (int i = 0; i < count; i++) {
                if (twig_get_1bit(&br))
                    twig_skip_bits(&br, (i < 6) ? 16 : 64);
            }
        }
        pps->second_chroma_qp_index_offset = twig_get_se_golomb(&br);
    } else {
        pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
    }
    
    return 0;
}

static int twig_find_first_slice(const uint8_t *data, size_t len) {
    size_t pos = 0;
    while (pos < len) {
        pos = twig_find_start_code(data, len, pos) + 3;
        if (pos < 0 || pos >= len)
            break;
            
        uint8_t nal_header = data[pos];
        uint8_t nal_type = nal_header & 0x1f;
        
        if (nal_type == 1 || nal_type == 5)
            return pos;
        
        pos++;
    }
    return -1;
}

static int twig_parse_slice_header_for_poc(const uint8_t *data, size_t size, twig_h264_decoder_t *decoder) {
    twig_h264_sps_t *sps = decoder->sps;
    twig_h264_pps_t *pps = decoder->pps;
    twig_h264_hdr_t *hdr = decoder->hdr;
    twig_bitreader_t br;

    memset(hdr, 0, sizeof(twig_h264_hdr_t));
    
    twig_bitreader_init(&br, data + 1, size - 1);
    
    hdr->first_mb_in_slice = twig_get_ue_golomb(&br);
    hdr->slice_type = twig_get_ue_golomb(&br);
    if (hdr->slice_type > 9)
        hdr->slice_type -= 5;
    
    hdr->pic_parameter_set_id = twig_get_ue_golomb(&br);
    hdr->frame_num = twig_get_bits(&br, sps->log2_max_frame_num_minus4 + 4);
    
    if (!sps->frame_mbs_only_flag) {
        hdr->field_pic_flag = twig_get_1bit(&br);
        if (hdr->field_pic_flag)
            hdr->bottom_field_pic_flag = twig_get_1bit(&br);
    } else {
        hdr->field_pic_flag = 0;
        hdr->bottom_field_pic_flag = 0;
    }
    
    if (sps->pic_order_cnt_type == 0) {
        hdr->pic_order_cnt_lsb = twig_get_bits(&br, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt_bottom = twig_get_se_golomb(&br);
    } else if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
        hdr->delta_pic_order_cnt[0] = twig_get_se_golomb(&br);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt[1] = twig_get_se_golomb(&br);
    }
    return 0;
}

static int twig_h264_parse_hdr(const uint8_t *data, size_t size, twig_h264_decoder_t *decoder) {
    if (!data || size < 2 || !decoder)
        return -1;

    twig_h264_sps_t *sps = decoder->sps;
    twig_h264_pps_t *pps = decoder->pps;
    twig_h264_hdr_t *hdr = decoder->hdr;
    
    twig_bitreader_t br;
    twig_bitreader_init(&br, data + 1, size - 1);
    
    hdr->nal_unit_type = data[0] & 0x1f;
    
    hdr->first_mb_in_slice = twig_get_ue_golomb(&br);
    hdr->first_slice_in_pic = (hdr->first_mb_in_slice == 0) ? 1 : 0;
    
    uint32_t slice_type = twig_get_ue_golomb(&br);
    if (slice_type > 9)
        return -1;
    hdr->slice_type = (slice_type > 4) ? slice_type - 5 : slice_type;
    
    hdr->pic_parameter_set_id = twig_get_ue_golomb(&br);
    if (hdr->pic_parameter_set_id >= 256)
        return -1;
    
    hdr->frame_num = twig_get_bits(&br, sps->log2_max_frame_num_minus4 + 4);
    
    if (!sps->frame_mbs_only_flag) {
        hdr->field_pic_flag = twig_get_1bit(&br);
        if (hdr->field_pic_flag)
            hdr->bottom_field_pic_flag = twig_get_1bit(&br);
    }
    
    if (hdr->nal_unit_type == 5)
        hdr->idr_pic_id = twig_get_ue_golomb(&br);
    
    if (sps->pic_order_cnt_type == 0) {
        hdr->pic_order_cnt_lsb = twig_get_bits(&br, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt_bottom = twig_get_se_golomb(&br);
    }
    
    if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
        hdr->delta_pic_order_cnt[0] = twig_get_se_golomb(&br);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt[1] = twig_get_se_golomb(&br);
    }
    
    if (pps->redundant_pic_cnt_present_flag)
        hdr->redundant_pic_cnt = twig_get_ue_golomb(&br);
    
    if (hdr->slice_type == SLICE_TYPE_B)
        hdr->direct_spatial_mv_pred_flag = twig_get_1bit(&br);
    
    hdr->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
    hdr->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
    
    if (hdr->slice_type == SLICE_TYPE_P || hdr->slice_type == SLICE_TYPE_SP || hdr->slice_type == SLICE_TYPE_B) {
        hdr->num_ref_idx_active_override_flag = twig_get_1bit(&br);
        if (hdr->num_ref_idx_active_override_flag) {
            hdr->num_ref_idx_l0_active_minus1 = twig_get_ue_golomb(&br);
            if (hdr->slice_type == SLICE_TYPE_B)
                hdr->num_ref_idx_l1_active_minus1 = twig_get_ue_golomb(&br);
        }
    }
    
    // Skip ref pic list modifications for now
    if (hdr->slice_type != SLICE_TYPE_I && hdr->slice_type != SLICE_TYPE_SI) {
        if (twig_get_1bit(&br)) {
            uint32_t modification_of_pic_nums_idc;
            do {
                modification_of_pic_nums_idc = twig_get_ue_golomb(&br);
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1)
                    twig_get_ue_golomb(&br);
                else if (modification_of_pic_nums_idc == 2)
                    twig_get_ue_golomb(&br);
            } while (modification_of_pic_nums_idc != 3);
        }
    }
    
    if (hdr->slice_type == SLICE_TYPE_B) {
        if (twig_get_1bit(&br)) {
            uint32_t modification_of_pic_nums_idc;
            do {
                modification_of_pic_nums_idc = twig_get_ue_golomb(&br);
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1)
                    twig_get_ue_golomb(&br);
                else if (modification_of_pic_nums_idc == 2)
                    twig_get_ue_golomb(&br);
            } while (modification_of_pic_nums_idc != 3);
        }
    }
    
    // Skip weighted prediction tables for now
    if ((pps->weighted_pred_flag && (hdr->slice_type == SLICE_TYPE_P || hdr->slice_type == SLICE_TYPE_SP)) ||
        (pps->weighted_bipred_idc == 1 && hdr->slice_type == SLICE_TYPE_B)) {
        // Skip pred_weight_table() - complex to implement
        fprintf(stderr, "Weighted prediction not implemented\n");
        return -1;
    }
    
    // Skip dec_ref_pic_marking() for now
    if (hdr->nal_unit_type == 5) {
        twig_get_1bit(&br);
        twig_get_1bit(&br);
    } else if ((data[0] >> 5) & 0x3) {
        if (twig_get_1bit(&br)) {
            uint32_t memory_management_control_operation;
            do {
                memory_management_control_operation = twig_get_ue_golomb(&br);
                if (memory_management_control_operation == 1 || memory_management_control_operation == 3)
                    twig_get_ue_golomb(&br);
                if (memory_management_control_operation == 2)
                    twig_get_ue_golomb(&br);
                if (memory_management_control_operation == 3 || memory_management_control_operation == 6)
                    twig_get_ue_golomb(&br);
                if (memory_management_control_operation == 4)
                    twig_get_ue_golomb(&br);
            } while (memory_management_control_operation != 0);
        }
    }
    
    if (pps->entropy_coding_mode_flag && hdr->slice_type != SLICE_TYPE_I && hdr->slice_type != SLICE_TYPE_SI)
        hdr->cabac_init_idc = twig_get_ue_golomb(&br);
    
    hdr->slice_qp_delta = twig_get_se_golomb(&br);
    
    if (hdr->slice_type == SLICE_TYPE_SP || hdr->slice_type == SLICE_TYPE_SI) {
        if (hdr->slice_type == SLICE_TYPE_SP)
            hdr->sp_for_switch_flag = twig_get_1bit(&br);
        hdr->slice_qs_delta = twig_get_se_golomb(&br);
    }
    
    if (pps->deblocking_filter_control_present_flag) {
        hdr->disable_deblocking_filter_idc = twig_get_ue_golomb(&br);
        if (hdr->disable_deblocking_filter_idc != 1) {
            hdr->slice_alpha_c0_offset_div2 = twig_get_se_golomb(&br);
            hdr->slice_beta_offset_div2 = twig_get_se_golomb(&br);
        }
    }
    
    return 0;
}

static int twig_calculate_poc(twig_h264_decoder_t *decoder) {
    twig_h264_sps_t *sps = decoder->sps;
    twig_h264_hdr_t *hdr = decoder->hdr;
    
    switch (sps->pic_order_cnt_type) {
        case 0: {
            int max_poc_lsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
            int poc_msb;
            
            if (hdr->pic_order_cnt_lsb < decoder->ref_state.prev_poc_lsb && (decoder->ref_state.prev_poc_lsb - hdr->pic_order_cnt_lsb) >= (max_poc_lsb / 2))
                poc_msb = decoder->ref_state.prev_poc_msb + max_poc_lsb;
            else if (hdr->pic_order_cnt_lsb > decoder->ref_state.prev_poc_lsb && (hdr->pic_order_cnt_lsb - decoder->ref_state.prev_poc_lsb) > (max_poc_lsb / 2))
                poc_msb = decoder->ref_state.prev_poc_msb - max_poc_lsb;
            else
                poc_msb = decoder->ref_state.prev_poc_msb;
            
            return poc_msb + hdr->pic_order_cnt_lsb;
        }
        case 1: {
            int abs_frame_num = decoder->ref_state.frame_num_offset + hdr->frame_num;
            return abs_frame_num * 2 + (hdr->field_pic_flag ? (hdr->bottom_field_pic_flag ? 1 : 0) : 0);
        }
        case 2:
            return 2 * hdr->frame_num;
        default:
            return -1;
    }
}

static void twig_update_poc_state(twig_h264_decoder_t *decoder, int current_poc) {
    if (decoder->sps->pic_order_cnt_type == 0) {
        decoder->ref_state.prev_poc_lsb = decoder->hdr->pic_order_cnt_lsb;
        decoder->ref_state.prev_poc_msb = current_poc - decoder->ref_state.prev_poc_lsb;
    }
}

EXPORT twig_h264_decoder_t *twig_h264_decoder_init(twig_dev_t *cedar, int max_width) {
    if (!cedar || max_width <= 0)
        return NULL;

    twig_h264_decoder_t *decoder = calloc(1, sizeof(twig_h264_decoder_t));
    if (!decoder)
        return NULL;
    
    decoder->cedar = cedar;
    decoder->ve_regs = twig_get_ve_regs(cedar, (max_width >= 2048) ? 1 : 0);
    if (!decoder->ve_regs) {
        free(decoder);
        return NULL;
    }

    memset(&decoder->ref_state, 0, sizeof(twig_ref_state_t));
    decoder->hdr = NULL;
    decoder->extra_buf = NULL;
    decoder->coded_width = -1;
    decoder->coded_height = -1;
    return decoder;
}

static int twig_h264_decode_params(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf) {
    if (!decoder || !bitstream_buf)
        return -1;

    decoder->sps = calloc(1, sizeof(twig_h264_sps_t));
    decoder->pps = calloc(1, sizeof(twig_h264_pps_t));
    if (!decoder->sps || !decoder->pps)
        goto fail;

    const uint8_t *data = (const uint8_t *)bitstream_buf->virt;
    size_t len = bitstream_buf->size;
    size_t pos = 0;
    int sps_ok = 0;
    int pps_ok = 0;
    while (pos < len) {
        pos = twig_find_start_code(data, len, pos) + 3;
        if (pos < 0 || pos >= len)
            break;

        uint8_t nal_header = data[pos];
        uint8_t nal_type = nal_header & 0x1f;
        int next_start = twig_find_start_code(data, len, pos + 1);
        size_t nal_size = (next_start > 0) ? (next_start - pos) : (len - pos);
        switch (nal_type) {
            case NAL_SPS:
                if (twig_parse_sps(data + pos, nal_size, decoder->sps) == 0) {
                    sps_ok = 1;
                    decoder->coded_width = (decoder->sps->pic_width_in_mbs_minus1 + 1) * 16;
                    decoder->coded_height = (decoder->sps->pic_height_in_mbs_minus1 + 1) * 16;
                }
                break;
            case NAL_PPS:
                if (twig_parse_pps(data + pos, nal_size, decoder->pps) == 0)
                    pps_ok = 1;
                break;
            default:
                if (sps_ok == 1 && pps_ok == 1)
                    return 0;
        }
    }

fail:
    if (decoder->sps)
        free(decoder->sps);
    if (decoder->pps)
        free(decoder->pps);

    return -1;
}

EXPORT twig_mem_t *twig_h264_decode_frame(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf) {
    if (!decoder || !bitstream_buf)
        return NULL;

    if (twig_h264_decode_params(decoder, bitstream_buf) < 0)
        return NULL;

    decoder->hdr = calloc(1, sizeof(twig_h264_hdr_t));
    if (!decoder->hdr)
        return NULL;

    if (decoder->pool_initialized == 0 || decoder->last_width != decoder->coded_width || decoder->last_height != decoder->coded_height) {
        if (decoder->pool_initialized == 1)
            twig_frame_pool_cleanup(&decoder->frame_pool, decoder->cedar);

        if (twig_frame_pool_init(&decoder->frame_pool, decoder->coded_width, decoder->coded_height) < 0)
            return NULL;

        decoder->last_width = decoder->coded_width;
        decoder->last_height = decoder->coded_height;
        decoder->pool_initialized = 1;
    }

    if (!decoder->extra_buf) {
        decoder->extra_buf = twig_alloc_mem(decoder->cedar, 1024 * 1024);
        if (!decoder->extra_buf) {
            fprintf(stderr, "Failed to allocate extra buffer\n");
            return NULL;
        }
    }

    uintptr_t ve_base = (uintptr_t)decoder->ve_regs;
    uintptr_t h264_base = ve_base + H264_OFFSET;

    uint32_t extra_buffer = decoder->extra_buf->iommu_addr;
    twig_writel(h264_base, H264_FIELD_INTRA_INFO_BUF, extra_buffer);
    twig_writel(h264_base, H264_NEIGHBOR_INFO_BUF, extra_buffer + 0x48000);
    if (decoder->coded_width >= 2048) {
        int size = (decoder->sps->pic_width_in_mbs_minus1 + 32) * 192;
        size = (size + 4095) & ~4095;
        twig_writel(h264_base, H264_FIELD_INTRA_INFO_BUF, 0x5);
        twig_writel(h264_base, H264_NEIGHBOR_INFO_BUF, extra_buffer + 0x50000);
        twig_writel(h264_base, H264_PIC_MBSIZE, extra_buffer + 0x50000 + size);
    }

    // TODO: Implement scaling list check, assume default for now
    decoder->is_default_scaling = 1;

    twig_writel(h264_base, H264_SDROT_CTRL, 0x00000000);

    twig_frame_t *output_frame = twig_frame_pool_get(&decoder->frame_pool, decoder->cedar, decoder->sps->pic_width_in_mbs_minus1);
    if (!output_frame)
        return NULL;

    const uint8_t *data = (const uint8_t *)bitstream_buf->virt;
    size_t len = bitstream_buf->size;

    int first_slice_pos = twig_find_first_slice(data, len);
    if (first_slice_pos < 0)
        return NULL;
    
    int next_start = twig_find_start_code(data, len, first_slice_pos + 1);
    size_t slice_size = (next_start > 0) ? (next_start - first_slice_pos) : (len - first_slice_pos);

    if (twig_parse_slice_header_for_poc(data + first_slice_pos, slice_size, decoder) < 0)
        return NULL;

    int current_poc = twig_calculate_poc(decoder);
    twig_write_framebuffer_list(decoder->cedar, decoder->ve_regs, &decoder->frame_pool, output_frame, current_poc);

    uint8_t nal_ref_idc = 0;
    uint8_t nal_type = 0;
    size_t pos = 0;
    while (pos < len) {
        pos = twig_find_start_code(data, len, pos) + 3;
        if (pos < 0 || pos >= len)
            break;

        uint32_t bitstream_addr = bitstream_buf->iommu_addr;
        uint8_t nal_header = data[pos];
        nal_ref_idc = (nal_header >> 5) & 0x3;
        nal_type = nal_header & 0x1f;
        next_start = twig_find_start_code(data, len, pos + 1);
        size_t nal_size = (next_start > 0) ? (next_start - pos) : (len - pos);
        if (nal_type != 1 && nal_type != 5)
            continue;
        
        memset(decoder->hdr, 0, sizeof(twig_h264_hdr_t));    
        twig_h264_parse_hdr(data + pos, nal_size, decoder);

        twig_frame_t *ref_list0[16], *ref_list1[16];
        int l0_count = 0, l1_count = 0;
    
        twig_build_ref_lists_from_pool(&decoder->frame_pool, decoder->hdr->slice_type, ref_list0, &l0_count, ref_list1, &l1_count, current_poc);
        // TODO: Apply ref_pic_list_modification() if present in slice header
        if (decoder->hdr->slice_type != SLICE_TYPE_I && decoder->hdr->slice_type != SLICE_TYPE_SI)
            twig_write_ref_list0_registers(decoder->cedar, decoder->ve_regs, &decoder->frame_pool, ref_list0, l0_count);
        if (decoder->hdr->slice_type == SLICE_TYPE_B)
            twig_write_ref_list1_registers(decoder->cedar, decoder->ve_regs, &decoder->frame_pool, ref_list1, l1_count);

        twig_writel(h264_base, H264_CTRL, (0x1 << 25) | (0x1 << 10));
        twig_writel(h264_base, H264_VLD_LEN, (len - pos) * 8);
        twig_writel(h264_base, H264_VLD_OFFSET, pos * 8);
        twig_writel(h264_base, H264_VLD_END, bitstream_addr + bitstream_buf->size - 1);
        twig_writel(h264_base, H264_VLD_ADDR, (bitstream_addr & 0x0ffffff0) | (bitstream_addr >> 28) | (0x7 << 28));
        twig_writel(h264_base, H264_TRIGGER, 0x7);

        twig_writel(h264_base, H264_SEQ_HDR, (0x1 << 19)
                  | ((decoder->sps->frame_mbs_only_flag & 0x1) << 18)
                  | ((decoder->sps->mb_adaptive_frame_field_flag & 0x1) << 17)
                  | ((decoder->sps->direct_8x8_inference_flag & 0x1) << 16)
                  | ((decoder->sps->pic_width_in_mbs_minus1 & 0xff) << 8)
                  | ((decoder->sps->pic_height_in_mbs_minus1 & 0xff) << 0));

        twig_writel(h264_base, H264_PIC_HDR,
                    ((decoder->pps->entropy_coding_mode_flag & 0x1) << 15)
                  | ((decoder->pps->num_ref_idx_l0_default_active_minus1 & 0x1f) << 10)
                  | ((decoder->pps->num_ref_idx_l1_default_active_minus1 & 0x1f) << 5) 
                  | ((decoder->pps->weighted_pred_flag & 0x1) << 4)
                  | ((decoder->pps->weighted_bipred_idc & 0x3) << 2)
                  | ((decoder->pps->constrained_intra_pred_flag & 0x1) << 1)
                  | ((decoder->pps->transform_8x8_mode_flag & 0x1) << 0));

        twig_writel(h264_base, H264_SLICE_HDR,
                    (((decoder->hdr->first_mb_in_slice % (decoder->sps->pic_width_in_mbs_minus1 + 1)) & 0xff) << 24)
                  | (((decoder->hdr->first_mb_in_slice / (decoder->sps->pic_height_in_mbs_minus1 + 1)) & 0xff)
                      * (decoder->sps->mb_adaptive_frame_field_flag ? 2 : 1) << 16)
                  | (((nal_ref_idc != 0 ? 0x1 : 0x0) & 0x1) << 12)
                  | ((decoder->hdr->slice_type & 0xf) << 8)
                  | ((decoder->hdr->first_slice_in_pic ? 0x1 : 0x0) << 5)
                  | ((decoder->hdr->field_pic_flag & 0x1) << 4)
                  | ((decoder->hdr->bottom_field_pic_flag & 0x1) << 3)
                  | ((decoder->hdr->direct_spatial_mv_pred_flag & 0x1) << 2)
                  | ((decoder->hdr->cabac_init_idc & 0x3) << 0));

        twig_writel(h264_base, H264_SLICE_HDR2,
                    ((decoder->hdr->num_ref_idx_l0_active_minus1 & 0x1f) << 24)
                  | ((decoder->hdr->num_ref_idx_l1_active_minus1 & 0x1f) << 16)
                  | ((decoder->hdr->num_ref_idx_active_override_flag & 0x1) << 12)
                  | ((decoder->hdr->disable_deblocking_filter_idc & 0x3) << 8)
                  | ((decoder->hdr->slice_alpha_c0_offset_div2 & 0xf) << 4)
                  | ((decoder->hdr->slice_beta_offset_div2 & 0xf) << 0));

        twig_writel(h264_base, H264_QP,
                    ((decoder->is_default_scaling & 0x1) << 24)
                  | ((decoder->pps->second_chroma_qp_index_offset & 0x3f) << 16)
                  | ((decoder->pps->chroma_qp_index_offset & 0x3f) << 8)
                  | ((decoder->pps->pic_init_qp_minus26 + 26 + decoder->hdr->slice_qp_delta) & 0x3f) << 0);

        twig_writel(h264_base, H264_STATUS, twig_readl(h264_base, H264_STATUS)); // Clears status by writing the same bits the status already holds
        twig_writel(h264_base, H264_CTRL, twig_readl(h264_base, H264_CTRL) | 0x7); // Enables interrupt on the next start code detected...
        twig_writel(h264_base, H264_TRIGGER, 0x8); // DECODE NOW BABYYYYY          // ...which basically makes it a stop code?
        if (twig_wait_for_ve(decoder->cedar) < 0)
            fprintf(stderr, "VE error or timeout during decode!\n");

        twig_writel(h264_base, H264_STATUS, twig_readl(h264_base, H264_STATUS));
        pos = (twig_readl(h264_base, H264_VLD_OFFSET) / 8) - 3;
    }
    twig_put_ve_regs(decoder->cedar);

    int frame_num = decoder->hdr->frame_num;
    int poc = current_poc;
    int is_reference = (nal_ref_idc != 0);

    twig_update_poc_state(decoder, current_poc);

    return twig_send_frame(output_frame, frame_num, poc, is_reference);
}

EXPORT int twig_get_frame_res(twig_h264_decoder_t *decoder, int *width, int *height) {
    if (width){
        if (decoder->coded_width == -1) {
            fprintf(stderr, "ERROR: No valid frame width yet, but it was requested!\n");
            return -1;
        }
        *width = decoder->coded_width;
    }
    if (height){
        if (decoder->coded_height == -1) {
            fprintf(stderr, "ERROR: No valid frame height yet, but it was requested!\n");
            return -1;
        }
        *height = decoder->coded_height;
    }
    return 0;
}

EXPORT void twig_h264_return_frame(twig_h264_decoder_t *decoder, twig_mem_t *output_buf) {
    if (!decoder || !output_buf)
        return;

    twig_mark_frame_return(&decoder->frame_pool, output_buf, decoder->cedar);
}

EXPORT void twig_h264_decoder_destroy(twig_h264_decoder_t* decoder) {
    if (!decoder)
        return;

    if (decoder->ve_regs)
        twig_put_ve_regs(decoder->cedar);

    twig_frame_pool_cleanup(&decoder->frame_pool, decoder->cedar);

    if (decoder->sps)
        free(decoder->sps);
    if (decoder->pps)
        free(decoder->pps);
    if (decoder->hdr)
        free(decoder->hdr);
    if (decoder->extra_buf)
        twig_free_mem(decoder->cedar, decoder->extra_buf);

    free(decoder);
}