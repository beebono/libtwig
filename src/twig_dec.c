#include "twig.h"
#include "twig_bits.h"
#include "twig_dec.h"
#include "twig_regs.h"

#define EXPORT __attribute__((visibility ("default")))

static const uint8_t default_4x4_intra[16] = {
     6, 13, 20, 28,
    13, 20, 28, 32,
    20, 28, 32, 37,
    28, 32, 37, 42
};

static const uint8_t default_4x4_inter[16] = {
    10, 14, 20, 24,
    14, 20, 24, 27,
    20, 24, 27, 30,
    24, 27, 30, 34
};

static const uint8_t default_8x8_intra[64] = {
     6, 10, 13, 16, 18, 23, 25, 27,
    10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31,
    16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36,
    23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40,
    27, 29, 31, 33, 36, 38, 40, 42
};

static const uint8_t default_8x8_inter[64] = {
     9, 13, 15, 17, 19, 21, 22, 24,
    13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27,
    17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30,
    21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33,
    24, 25, 27, 28, 30, 32, 33, 35
};

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

static uint8_t *twig_strip_epb_h264(const uint8_t *data, size_t size, size_t *new_size) {
    uint8_t *out = malloc(size);
    if (!out) return NULL;
    size_t out_pos = 0;
    for (size_t i = 0; i < size; i++) {
        if (i + 2 < size && data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x03) {
            out[out_pos++] = 0x00;
            out[out_pos++] = 0x00;
            i += 2;
        } else {
            out[out_pos++] = data[i];
        }
    }
    *new_size = out_pos;
    return out;
}

uint32_t twig_get_bits_hw(void *regs, int num) {
    twig_writel((uintptr_t)regs, H264_TRIGGER, 0x00000002 | (num << 8));

    while (twig_readl((uintptr_t)regs, H264_STATUS) & (1 << 8));

    return twig_readl((uintptr_t)regs, H264_BASIC_BITS);
}

uint32_t twig_get_1bit_hw(void *regs) {
    return twig_get_bits_hw(regs, 1);
}

uint32_t twig_get_ue_golomb_hw(void *regs) {
    twig_writel((uintptr_t)regs, H264_TRIGGER, 0x00000005);

    while (twig_readl((uintptr_t)regs, H264_STATUS) & (1 << 8));

    return twig_readl((uintptr_t)regs, H264_BASIC_BITS);
}

int32_t twig_get_se_golomb_hw(void *regs) {
    twig_writel((uintptr_t)regs, H264_TRIGGER, 0x00000004);

    while (twig_readl((uintptr_t)regs, H264_STATUS) & (1 << 8));

    return twig_readl((uintptr_t)regs, H264_BASIC_BITS);
}

static int twig_parse_pred_weight_table(void *regs, twig_h264_hdr_t *hdr) {
    int i, j, ChromaArrayType = 1;
    uint8_t luma_log2_weight_denom = twig_get_ue_golomb_hw(regs);
    uint8_t chroma_log2_weight_denom = 0;
    if (ChromaArrayType != 0)
        chroma_log2_weight_denom = twig_get_ue_golomb_hw(regs);

    int8_t luma_weight_l0[32], luma_offset_l0[32];
    int8_t chroma_weight_l0[32][2], chroma_offset_l0[32][2]; 
    int8_t luma_weight_l1[32], luma_offset_l1[32];
    int8_t chroma_weight_l1[32][2], chroma_offset_l1[32][2];
    
    for (i = 0; i < 32; i++) {
        luma_weight_l0[i] = (1 << luma_log2_weight_denom);
        luma_weight_l1[i] = (1 << luma_log2_weight_denom);
        chroma_weight_l0[i][0] = (1 << chroma_log2_weight_denom);
        chroma_weight_l1[i][0] = (1 << chroma_log2_weight_denom);
        chroma_weight_l0[i][1] = (1 << chroma_log2_weight_denom);
        chroma_weight_l1[i][1] = (1 << chroma_log2_weight_denom);
        luma_offset_l0[i] = luma_offset_l1[i] = 0;
        chroma_offset_l0[i][0] = chroma_offset_l0[i][1] = 0;
        chroma_offset_l1[i][0] = chroma_offset_l1[i][1] = 0;
    }

    for (i = 0; i <= hdr->num_ref_idx_l0_active_minus1; i++) {
        int luma_weight_l0_flag = twig_get_1bit_hw(regs);
        if (luma_weight_l0_flag) {
            luma_weight_l0[i] = twig_get_se_golomb_hw(regs);
            luma_offset_l0[i] = twig_get_se_golomb_hw(regs);
        }
        if (ChromaArrayType != 0) {
            int chroma_weight_l0_flag = twig_get_1bit_hw(regs);
            if (chroma_weight_l0_flag) {
                for (j = 0; j < 2; j++) {
                    chroma_weight_l0[i][j] = twig_get_se_golomb_hw(regs);
                    chroma_offset_l0[i][j] = twig_get_se_golomb_hw(regs);
                }
            }
        }
    }

    if (hdr->slice_type == SLICE_TYPE_B) {
        for (i = 0; i <= hdr->num_ref_idx_l1_active_minus1; i++) {
            int luma_weight_l1_flag = twig_get_1bit_hw(regs);
            if (luma_weight_l1_flag) {
                luma_weight_l1[i] = twig_get_se_golomb_hw(regs);
                luma_offset_l1[i] = twig_get_se_golomb_hw(regs);
            }
            if (ChromaArrayType != 0) {
                int chroma_weight_l1_flag = twig_get_1bit_hw(regs);
                if (chroma_weight_l1_flag) {
                    for (j = 0; j < 2; j++) {
                        chroma_weight_l1[i][j] = twig_get_se_golomb_hw(regs);
                        chroma_offset_l1[i][j] = twig_get_se_golomb_hw(regs);
                    }
                }
            }
        }
    }

    uintptr_t h264_base = (uintptr_t)regs + H264_OFFSET;

    twig_writel(h264_base, H264_PRED_WEIGHT, 
                ((chroma_log2_weight_denom & 0xf) << 4) |
                ((luma_log2_weight_denom & 0xf) << 0));

    twig_writel(h264_base, H264_RAM_WRITE_PTR, VE_SRAM_H264_PRED_WEIGHT_TABLE);

    for (i = 0; i < 32; i++)
        twig_writel(h264_base, H264_RAM_WRITE_DATA,
                    ((luma_offset_l0[i] & 0x1ff) << 16) |
                    (luma_weight_l0[i] & 0xff));

    for (i = 0; i < 32; i++)
        for (j = 0; j < 2; j++)
            twig_writel(h264_base, H264_RAM_WRITE_DATA,
                        ((chroma_offset_l0[i][j] & 0x1ff) << 16) |
                        (chroma_weight_l0[i][j] & 0xff));

    for (i = 0; i < 32; i++)
        twig_writel(h264_base, H264_RAM_WRITE_DATA,
                    ((luma_offset_l1[i] & 0x1ff) << 16) |
                    (luma_weight_l1[i] & 0xff));

    for (i = 0; i < 32; i++)
        for (j = 0; j < 2; j++)
            twig_writel(h264_base, H264_RAM_WRITE_DATA,
                        ((chroma_offset_l1[i][j] & 0x1ff) << 16) |
                        (chroma_weight_l1[i][j] & 0xff));

    return 0;
}

static void twig_parse_scaling_list_4x4_sw(twig_bitreader_t *br, uint8_t *scaling_list) {
    int last_scale = 8, next_scale = 8;
    for (int j = 0; j < 16; j++) {
        if (next_scale != 0) {
            int delta_scale = twig_get_se_golomb(br);
            next_scale = (last_scale + delta_scale + 256) % 256;
        }
        scaling_list[j] = (next_scale == 0) ? last_scale : next_scale;
        last_scale = scaling_list[j];
    }
}

static void twig_parse_scaling_list_8x8_sw(twig_bitreader_t *br, uint8_t *scaling_list) {
    int last_scale = 8, next_scale = 8;
    for (int j = 0; j < 64; j++) {
        if (next_scale != 0) {
            int delta_scale = twig_get_se_golomb(br);
            next_scale = (last_scale + delta_scale + 256) % 256;
        }
        scaling_list[j] = (next_scale == 0) ? last_scale : next_scale;
        last_scale = scaling_list[j];
    }
}

static int twig_are_scaling_lists_default(twig_h264_sps_t *sps, twig_h264_pps_t *pps) {
    if (!sps || !pps)
        return 1;

    if (sps->seq_scaling_matrix_present_flag) {
        for (int i = 0; i < 6; i++) {
            if (sps->seq_scaling_list_present_flag[i]) {
                const uint8_t *default_list = (i == 0 || i == 3) ? default_4x4_intra : default_4x4_inter;
                if (memcmp(sps->scaling_list_4x4[i], default_list, 16) != 0)
                    return 0;
            }
        }
        
        if (sps->profile_idc >= 100) {
            for (int i = 6; i < 8; i++) {
                if (sps->seq_scaling_list_present_flag[i]) {
                    const uint8_t *default_list = (i == 6) ? default_8x8_intra : default_8x8_inter;
                    if (memcmp(sps->scaling_list_8x8[i-6], default_list, 64) != 0)
                        return 0;
                }
            }
        }
    }

    if (pps->pic_scaling_matrix_present_flag) {
        int loop_count = 6 + (pps->transform_8x8_mode_flag ? 2 : 0);
        for (int i = 0; i < loop_count; i++) {
            if (pps->pic_scaling_list_present_flag[i]) {
                if (i < 6) {
                    const uint8_t *default_list = (i == 0 || i == 3) ? default_4x4_intra : default_4x4_inter;
                    if (memcmp(pps->scaling_list_4x4[i], default_list, 16) != 0)
                        return 0;
                } else {
                    const uint8_t *default_list = (i == 6) ? default_8x8_intra : default_8x8_inter;
                    if (memcmp(pps->scaling_list_8x8[i-6], default_list, 64) != 0)
                        return 0;
                }
            }
        }
    }

    return 1;
}

static void twig_write_scaling_lists(uintptr_t h264_base, twig_h264_sps_t *sps, twig_h264_pps_t *pps) {
    uint8_t final_4x4[6][16];
    uint8_t final_8x8[2][64];
    
    for (int i = 0; i < 6; i++) {
        const uint8_t *source_list = NULL;
        
        if (pps->pic_scaling_matrix_present_flag && pps->pic_scaling_list_present_flag[i]) {
            source_list = pps->scaling_list_4x4[i];
        } else if (sps->seq_scaling_matrix_present_flag && sps->seq_scaling_list_present_flag[i]) {
            source_list = sps->scaling_list_4x4[i];
        } else {
            source_list = (i == 0 || i == 3) ? default_4x4_intra : default_4x4_inter;
        }
        
        memcpy(final_4x4[i], source_list, 16);
    }
    
    for (int i = 0; i < 2; i++) {
        const uint8_t *source_list = NULL;
        int pps_idx = i + 6;
        int sps_idx = i + 6;
        
        if (pps->pic_scaling_matrix_present_flag && pps->transform_8x8_mode_flag && 
            pps->pic_scaling_list_present_flag[pps_idx]) {
            source_list = pps->scaling_list_8x8[i];
        } else if (sps->seq_scaling_matrix_present_flag && sps->seq_scaling_list_present_flag[sps_idx]) {
            source_list = sps->scaling_list_8x8[i];
        } else {
            source_list = (i == 0) ? default_8x8_intra : default_8x8_inter;
        }
        
        memcpy(final_8x8[i], source_list, 64);
    }
    
    twig_writel(h264_base, H264_RAM_WRITE_PTR, VE_SRAM_H264_SCALING_LISTS);
    
    const uint32_t *sl8 = (const uint32_t *)&final_8x8[0][0];
    for (int i = 0; i < 2 * 64 / 4; i++) {
        twig_writel(h264_base, H264_RAM_WRITE_DATA, sl8[i]);
    }
    
    const uint32_t *sl4 = (const uint32_t *)&final_4x4[0][0];
    for (int i = 0; i < 6 * 16 / 4; i++) {
        twig_writel(h264_base, H264_RAM_WRITE_DATA, sl4[i]);
    }
}

static int twig_parse_sps(const uint8_t *data, size_t size, twig_h264_sps_t *sps) {
    size_t stripped_size;
    uint8_t *stripped_data = twig_strip_epb_h264(data + 1, size - 1, &stripped_size);
    if (!stripped_data)
        return -1;

    twig_bitreader_t br;
    twig_bitreader_init(&br, stripped_data, stripped_size);

    memset(sps, 0, sizeof(twig_h264_sps_t));

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
        if (sps->profile_idc == 100 || sps->profile_idc == 110 || sps->profile_idc == 122 ||
            sps->profile_idc == 244 || sps->profile_idc == 44 || sps->profile_idc == 83 ||
            sps->profile_idc == 86 || sps->profile_idc == 118 || sps->profile_idc == 128) {

            sps->seq_scaling_matrix_present_flag = twig_get_1bit(&br);
            if (sps->seq_scaling_matrix_present_flag) {
                for (int i = 0; i < 8; i++) {
                    sps->seq_scaling_list_present_flag[i] = twig_get_1bit(&br);
                    if (sps->seq_scaling_list_present_flag[i]) {
                        if (i < 6)
                            twig_parse_scaling_list_4x4_sw(&br, sps->scaling_list_4x4[i]);
                        else
                            twig_parse_scaling_list_8x8_sw(&br, sps->scaling_list_8x8[i-6]);
                    }
                }
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
    free(stripped_data);
    return 0;
}

static int twig_parse_pps(const uint8_t *data, size_t size, twig_h264_pps_t *pps) {
    size_t stripped_size;
    uint8_t *stripped_data = twig_strip_epb_h264(data + 1, size - 1, &stripped_size);
    if (!stripped_data)
        return -1;

    twig_bitreader_t br;
    twig_bitreader_init(&br, stripped_data, stripped_size);
    
    memset(pps, 0, sizeof(twig_h264_pps_t));

    pps->pic_parameter_set_id = twig_get_ue_golomb(&br);
    pps->seq_parameter_set_id = twig_get_ue_golomb(&br);
    pps->entropy_coding_mode_flag = twig_get_1bit(&br);
    pps->bottom_field_pic_order_in_frame_present_flag = twig_get_1bit(&br);
    pps->num_slice_groups_minus1 = twig_get_ue_golomb(&br);
    if (pps->num_slice_groups_minus1 > 0) {
        pps->slice_group_map_type = twig_get_ue_golomb(&br);
        
        if (pps->slice_group_map_type == 0) {
            for (int i = 0; i <= pps->num_slice_groups_minus1; i++) {
                pps->run_length_minus1[i] = twig_get_ue_golomb(&br);
            }
        } else if (pps->slice_group_map_type == 2) {
            for (int i = 0; i < pps->num_slice_groups_minus1; i++) {
                pps->top_left[i] = twig_get_ue_golomb(&br);
                pps->bottom_right[i] = twig_get_ue_golomb(&br);
            }
        } else if (pps->slice_group_map_type >= 3 && pps->slice_group_map_type <= 5) {
            pps->slice_group_change_direction_flag = twig_get_1bit(&br);
            pps->slice_group_change_rate_minus1 = twig_get_ue_golomb(&br);
        } else if (pps->slice_group_map_type == 6) {
            pps->pic_size_in_map_units_minus1 = twig_get_ue_golomb(&br);
            int map_units = pps->pic_size_in_map_units_minus1 + 1;
            
            int bits_needed = 1;
            while ((1 << bits_needed) <= pps->num_slice_groups_minus1) bits_needed++;
            
            pps->slice_group_id = malloc(map_units);
            if (!pps->slice_group_id) return -1;
            
            for (int i = 0; i < map_units; i++) {
                pps->slice_group_id[i] = twig_get_bits(&br, bits_needed);
            }
        }
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
            int loop_count = 6 + (pps->transform_8x8_mode_flag ? 2 : 0);
            for (int i = 0; i < loop_count; i++) {
                pps->pic_scaling_list_present_flag[i] = twig_get_1bit(&br);
                if (pps->pic_scaling_list_present_flag[i]) {
                    if (i < 6)
                        twig_parse_scaling_list_4x4_sw(&br, pps->scaling_list_4x4[i]);
                    else
                        twig_parse_scaling_list_8x8_sw(&br, pps->scaling_list_8x8[i-6]);
                }
            }
        }
        pps->second_chroma_qp_index_offset = twig_get_se_golomb(&br);
    } else {
        pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
    }
    free(stripped_data);
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

static int twig_setup_vld_registers(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf, size_t nal_pos, size_t nal_size) {
    uintptr_t h264_base = (uintptr_t)decoder->ve_regs + H264_OFFSET;
    uint32_t bitstream_addr = bitstream_buf->iommu_addr;

    uint32_t vld_ctrl = (0x1 << 25) |
                        (0x1 << 24) |
                        (0x1 << 10) |
                        (0x1 << 8);

    twig_writel(h264_base, H264_CTRL, vld_ctrl);

    twig_writel(h264_base, H264_VLD_LEN, nal_size * 8);
    twig_writel(h264_base, H264_VLD_OFFSET, nal_pos * 8);
    twig_writel(h264_base, H264_VLD_END, bitstream_addr + bitstream_buf->size - 1);
    twig_writel(h264_base, H264_VLD_ADDR, (bitstream_addr & 0x0ffffff0) | (bitstream_addr >> 28) | (0x7 << 28));

    twig_writel(h264_base, H264_TRIGGER, 0x7);

    printf("DEBUG: VLD register setup complete.\n");
    printf("       Wrote the following values to listed register:\n");
    printf("       - H264_CTRL       = 0x%x and read back 0x%x\n", vld_ctrl, twig_readl(h264_base, H264_CTRL));
    printf("       - H264_VLD_LEN    = 0x%lx and read back 0x%x\n", nal_size * 8, twig_readl(h264_base, H264_VLD_LEN));
    printf("       - H264_VLD_OFFSET = 0x%lx and read back 0x%x\n", nal_pos * 8, twig_readl(h264_base, H264_VLD_OFFSET));
    printf("       - H264_VLD_END    = 0x%lx and read back 0x%x\n", bitstream_addr + bitstream_buf->size - 1, twig_readl(h264_base, H264_VLD_END));
    printf("       - H264_VLD_ADDR   = 0x%x and read back 0x%x\n", (bitstream_addr & 0x0ffffff0) | (bitstream_addr >> 28) | (0x7 << 28), twig_readl(h264_base, H264_VLD_ADDR));
    printf("       - H264_TRIGGER    = 0x7 (no read back, trigger automatically cleared)\n");
    return 0;
}

static int twig_validate_slice_groups(twig_h264_pps_t *pps) {
    if (!pps || pps->num_slice_groups_minus1 == 0)
        return 0;
    
    fprintf(stderr, "WARNING: Stream uses slice groups (FMO) - type %d, %d groups\n", 
            pps->slice_group_map_type, pps->num_slice_groups_minus1 + 1);
    fprintf(stderr, "         Slice groups are parsed but not validated. Decode may fail.\n");
    
    return 0;
}

static int twig_parse_hdr_poc(const uint8_t *data, size_t size, twig_h264_decoder_t *decoder) {
    if (!data || size < 2 || !decoder)
        return -1;

    twig_h264_sps_t *sps = decoder->sps;
    twig_h264_pps_t *pps = decoder->pps;
    twig_h264_hdr_t *hdr = decoder->hdr;
    void *regs = decoder->ve_regs;

    memset(hdr, 0, sizeof(twig_h264_hdr_t));

    hdr->first_mb_in_slice = twig_get_ue_golomb_hw(regs);
    hdr->slice_type = twig_get_ue_golomb_hw(regs);
    if (hdr->slice_type > 9)
        hdr->slice_type -= 5;

    hdr->pic_parameter_set_id = twig_get_ue_golomb_hw(regs);
    hdr->frame_num = twig_get_bits_hw(regs, sps->log2_max_frame_num_minus4 + 4);

    if (!sps->frame_mbs_only_flag) {
        hdr->field_pic_flag = twig_get_1bit_hw(regs);
        if (hdr->field_pic_flag)
            hdr->bottom_field_pic_flag = twig_get_1bit_hw(regs);
    } else {
        hdr->field_pic_flag = 0;
        hdr->bottom_field_pic_flag = 0;
    }

    if (sps->pic_order_cnt_type == 0) {
        hdr->pic_order_cnt_lsb = twig_get_bits_hw(regs, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt_bottom = twig_get_se_golomb_hw(regs);
    } else if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
        hdr->delta_pic_order_cnt[0] = twig_get_se_golomb_hw(regs);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt[1] = twig_get_se_golomb_hw(regs);
    }
    return 0;
}

static int twig_parse_hdr(const uint8_t *data, size_t size, twig_h264_decoder_t *decoder) {
    if (!data || size < 2 || !decoder)
        return -1;

    twig_h264_sps_t *sps = decoder->sps;
    twig_h264_pps_t *pps = decoder->pps;
    twig_h264_hdr_t *hdr = decoder->hdr;
    void *regs = decoder->ve_regs;

    hdr->nal_unit_type = data[0] & 0x1f;

    hdr->first_mb_in_slice = twig_get_ue_golomb_hw(regs);
    hdr->first_slice_in_pic = (hdr->first_mb_in_slice == 0) ? 1 : 0;

    uint32_t slice_type = twig_get_ue_golomb_hw(regs);
    if (slice_type > 9)
        return -1;
    hdr->slice_type = (slice_type > 4) ? slice_type - 5 : slice_type;

    hdr->pic_parameter_set_id = twig_get_ue_golomb_hw(regs);
    if (hdr->pic_parameter_set_id >= 256)
        return -1;

    hdr->frame_num = twig_get_bits_hw(regs, sps->log2_max_frame_num_minus4 + 4);

    if (!sps->frame_mbs_only_flag) {
        hdr->field_pic_flag = twig_get_1bit_hw(regs);
        if (hdr->field_pic_flag)
            hdr->bottom_field_pic_flag = twig_get_1bit_hw(regs);
    }

    if (hdr->nal_unit_type == 5)
        hdr->idr_pic_id = twig_get_ue_golomb_hw(regs);

    if (sps->pic_order_cnt_type == 0) {
        hdr->pic_order_cnt_lsb = twig_get_bits_hw(regs, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt_bottom = twig_get_se_golomb_hw(regs);
    }

    if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
        hdr->delta_pic_order_cnt[0] = twig_get_se_golomb_hw(regs);
        if (pps->bottom_field_pic_order_in_frame_present_flag && !hdr->field_pic_flag)
            hdr->delta_pic_order_cnt[1] = twig_get_se_golomb_hw(regs);
    }

    if (pps->redundant_pic_cnt_present_flag)
        hdr->redundant_pic_cnt = twig_get_ue_golomb_hw(regs);

    if (hdr->slice_type == SLICE_TYPE_B)
        hdr->direct_spatial_mv_pred_flag = twig_get_1bit_hw(regs);

    hdr->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
    hdr->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;

    if (hdr->slice_type == SLICE_TYPE_P || hdr->slice_type == SLICE_TYPE_SP || hdr->slice_type == SLICE_TYPE_B) {
        hdr->num_ref_idx_active_override_flag = twig_get_1bit_hw(regs);
        if (hdr->num_ref_idx_active_override_flag) {
            hdr->num_ref_idx_l0_active_minus1 = twig_get_ue_golomb_hw(regs);
            if (hdr->slice_type == SLICE_TYPE_B)
                hdr->num_ref_idx_l1_active_minus1 = twig_get_ue_golomb_hw(regs);
        }
    }

    if (hdr->slice_type != SLICE_TYPE_I && hdr->slice_type != SLICE_TYPE_SI) {
        hdr->ref_pic_list_modification_flag_l0 = twig_get_1bit_hw(regs);
        hdr->modification_count_l0 = 0;
    if (hdr->ref_pic_list_modification_flag_l0) {
        uint32_t modification_of_pic_nums_idc;
        do {
            modification_of_pic_nums_idc = twig_get_ue_golomb_hw(regs);
            hdr->modification_of_pic_nums_idc[hdr->modification_count_l0] = modification_of_pic_nums_idc;
            if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1)
                hdr->abs_diff_pic_num_minus1[hdr->modification_count_l0] = twig_get_ue_golomb_hw(regs);
            else if (modification_of_pic_nums_idc == 2)
                hdr->long_term_pic_num[hdr->modification_count_l0] = twig_get_ue_golomb_hw(regs);

            if (modification_of_pic_nums_idc != 3)
                hdr->modification_count_l0++;
            } while (modification_of_pic_nums_idc != 3 && hdr->modification_count_l0 < 32);
        }
    }

    if (hdr->slice_type == SLICE_TYPE_B) {
        hdr->ref_pic_list_modification_flag_l1 = twig_get_1bit_hw(regs);
        hdr->modification_count_l1 = 0;
        if (hdr->ref_pic_list_modification_flag_l1) {
            uint32_t modification_of_pic_nums_idc;
            do {
                modification_of_pic_nums_idc = twig_get_ue_golomb_hw(regs);
                hdr->modification_of_pic_nums_idc[hdr->modification_count_l1] = modification_of_pic_nums_idc;
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1)
                    hdr->abs_diff_pic_num_minus1[hdr->modification_count_l1] = twig_get_ue_golomb_hw(regs);
                else if (modification_of_pic_nums_idc == 2)
                    hdr->long_term_pic_num[hdr->modification_count_l1] = twig_get_ue_golomb_hw(regs);

                if (modification_of_pic_nums_idc != 3)
                      hdr->modification_count_l1++;
            } while (modification_of_pic_nums_idc != 3 && hdr->modification_count_l1 < 32);
        }
    }

    if ((pps->weighted_pred_flag && (hdr->slice_type == SLICE_TYPE_P || hdr->slice_type == SLICE_TYPE_SP)) ||
        (pps->weighted_bipred_idc == 1 && hdr->slice_type == SLICE_TYPE_B))
        twig_parse_pred_weight_table(regs, hdr);

    if (hdr->nal_unit_type == 5) {
        hdr->long_term_reference_flag = twig_get_1bit_hw(regs);
        decoder->mmco_count = 0;
    } else if ((data[0] >> 5) & 0x3) {
        if (twig_parse_mmco_commands(regs, decoder->mmco_commands, &decoder->mmco_count) < 0)
            return -1;
    } else {
        decoder->mmco_count = 0;
    }

    if (pps->entropy_coding_mode_flag && hdr->slice_type != SLICE_TYPE_I && hdr->slice_type != SLICE_TYPE_SI)
        hdr->cabac_init_idc = twig_get_ue_golomb_hw(regs);

    hdr->slice_qp_delta = twig_get_se_golomb_hw(regs);

    if (hdr->slice_type == SLICE_TYPE_SP || hdr->slice_type == SLICE_TYPE_SI) {
        if (hdr->slice_type == SLICE_TYPE_SP)
            hdr->sp_for_switch_flag = twig_get_1bit_hw(regs);
        hdr->slice_qs_delta = twig_get_se_golomb_hw(regs);
    }

    if (pps->deblocking_filter_control_present_flag) {
        hdr->disable_deblocking_filter_idc = twig_get_ue_golomb_hw(regs);
        if (hdr->disable_deblocking_filter_idc != 1) {
            hdr->slice_alpha_c0_offset_div2 = twig_get_se_golomb_hw(regs);
            hdr->slice_beta_offset_div2 = twig_get_se_golomb_hw(regs);
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

EXPORT twig_h264_decoder_t *twig_h264_decoder_init(twig_dev_t *cedar) {
    if (!cedar)
        return NULL;

    twig_h264_decoder_t *decoder = calloc(1, sizeof(twig_h264_decoder_t));
    if (!decoder)
        return NULL;

    decoder->cedar = cedar;
    decoder->ve_regs = twig_get_ve_regs(cedar);
    if (!decoder->ve_regs) {
        free(decoder);
        return NULL;
    }

    memset(&decoder->ref_state, 0, sizeof(twig_ref_state_t));
    memset(&decoder->mmco_commands, 0, sizeof(decoder->mmco_commands));
    decoder->mmco_count = 0;
    decoder->hdr = NULL;
    decoder->extra_buf = NULL;
    decoder->coded_width = -1;
    decoder->coded_height = -1;
    return decoder;
}

static int twig_decode_params(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf) {
    if (!decoder || !bitstream_buf)
        return -1;

    decoder->sps = calloc(1, sizeof(twig_h264_sps_t));
    decoder->pps = calloc(1, sizeof(twig_h264_pps_t));
    if (!decoder->sps || !decoder->pps)
        goto fail;

    const uint8_t *data = (const uint8_t *)bitstream_buf->virt_addr;
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
                if (twig_parse_pps(data + pos, nal_size, decoder->pps) == 0) {
                    pps_ok = 1;
                    twig_validate_slice_groups(decoder->pps);
                }
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

    twig_flush_mem(bitstream_buf);
    if (twig_decode_params(decoder, bitstream_buf) < 0)
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
        decoder->frame_pool.max_frame_num = 1 << (decoder->sps->log2_max_frame_num_minus4 + 4);
        decoder->pool_initialized = 1;
    }

    if (!decoder->extra_buf) {
        decoder->extra_buf = twig_alloc_mem(decoder->cedar, 1024 * 1024);
        if (!decoder->extra_buf)
            return NULL;
    }

    uintptr_t ve_base = (uintptr_t)decoder->ve_regs;
    uintptr_t h264_base = ve_base + H264_OFFSET;

    if (decoder->coded_width >= 2048) {
        uint32_t ctrl_val = twig_readl(ve_base, VE_CTRL) | 0x00200000;
        twig_writel(ve_base, VE_CTRL, ctrl_val);
    }

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

    decoder->is_default_scaling = twig_are_scaling_lists_default(decoder->sps, decoder->pps);
    if (decoder->is_default_scaling != 1)
        twig_write_scaling_lists(h264_base, decoder->sps, decoder->pps);

    twig_writel(h264_base, H264_SDROT_CTRL, 0x00000000);

    twig_frame_t *output_frame = twig_frame_pool_get(&decoder->frame_pool, decoder->cedar, decoder->sps->pic_width_in_mbs_minus1);
    if (!output_frame)
        return NULL;

    const uint8_t *data = (const uint8_t *)bitstream_buf->virt_addr;
    size_t len = bitstream_buf->size;

    int first_slice_pos = twig_find_first_slice(data, len);
    if (first_slice_pos < 0)
        return NULL;
    
    int next_start = twig_find_start_code(data, len, first_slice_pos + 1);
    size_t slice_size = (next_start > 0) ? (next_start - first_slice_pos) : (len - first_slice_pos);
    twig_setup_vld_registers(decoder, bitstream_buf, first_slice_pos, slice_size);
    if (twig_parse_hdr_poc(data + first_slice_pos, slice_size, decoder) < 0)
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
        twig_setup_vld_registers(decoder, bitstream_buf, pos, nal_size);
        twig_parse_hdr(data + pos, nal_size, decoder);

        if (decoder->mmco_count > 0) {
            twig_execute_mmco_commands(decoder, output_frame);
            twig_remove_stale_frames(&decoder->frame_pool);
        }

        twig_frame_t *ref_list0[16], *ref_list1[16];
        int l0_count = 0, l1_count = 0;
        twig_build_ref_lists(&decoder->frame_pool, decoder->hdr, ref_list0, &l0_count, ref_list1, &l1_count, current_poc);
        if (decoder->hdr->slice_type != SLICE_TYPE_I && decoder->hdr->slice_type != SLICE_TYPE_SI)
            twig_write_ref_list0_registers(decoder->cedar, decoder->ve_regs, &decoder->frame_pool, ref_list0, l0_count);
        if (decoder->hdr->slice_type == SLICE_TYPE_B)
            twig_write_ref_list1_registers(decoder->cedar, decoder->ve_regs, &decoder->frame_pool, ref_list1, l1_count);

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
                  | (((decoder->hdr->first_mb_in_slice / (decoder->sps->pic_width_in_mbs_minus1 + 1)) & 0xff)
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

        twig_writel(h264_base, H264_STATUS, twig_readl(h264_base, H264_STATUS));
        twig_writel(h264_base, H264_CTRL, twig_readl(h264_base, H264_CTRL) | 0x7);
        twig_writel(h264_base, H264_TRIGGER, 0x8);
        twig_wait_for_ve(decoder->cedar);
        printf("DEBUG: Status registers after wait:\n");
        printf("           - VE_STATUS reports:   0x%x\n", twig_readl(ve_base, VE_STATUS));
        printf("           - H264_STATUS reports: 0x%x\n", twig_readl(h264_base, H264_STATUS));
        twig_writel(h264_base, H264_STATUS, twig_readl(h264_base, H264_STATUS));

        pos = next_start - 3;
    }
    twig_put_ve_regs(decoder->cedar);

    output_frame->frame_num = decoder->hdr->frame_num;
    output_frame->poc = current_poc;
    if (nal_ref_idc != 0) {
        if (nal_type == NAL_IDR_SLICE) {
            for (int i = 0; i < decoder->frame_pool.allocated_count; i++) {
                if (decoder->frame_pool.frames[i].is_reference)
                    twig_mark_frame_unref(&decoder->frame_pool, &decoder->frame_pool.frames[i]);
            }
        }
        twig_add_short_term_ref(&decoder->frame_pool, output_frame);
    }

    if (decoder->sps->pic_order_cnt_type == 0) {
        decoder->ref_state.prev_poc_lsb = decoder->hdr->pic_order_cnt_lsb;
        decoder->ref_state.prev_poc_msb = current_poc - decoder->ref_state.prev_poc_lsb;
    }
    output_frame->state = FRAME_STATE_APP_HELD;
    return output_frame->buffer;
}

EXPORT int twig_h264_get_frame_res(twig_h264_decoder_t *decoder, int *width, int *height) {
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

    for (int i = 0; i < decoder->frame_pool.allocated_count; i++) {
        if (decoder->frame_pool.frames[i].buffer == output_buf) {
            twig_frame_t *frame = &decoder->frame_pool.frames[i];            
            if (frame->is_reference)
                frame->state = FRAME_STATE_DECODER_HELD;
            else
                frame->state = FRAME_STATE_FREE;

            return;
        }
    }
    twig_free_mem(decoder->cedar, output_buf);
}

static void twig_cleanup_pps(twig_h264_pps_t *pps) {
    if (pps && pps->slice_group_id) {
        free(pps->slice_group_id);
        pps->slice_group_id = NULL;
    }
}

EXPORT void twig_h264_decoder_destroy(twig_h264_decoder_t* decoder) {
    if (!decoder)
        return;

    if (decoder->ve_regs)
        twig_put_ve_regs(decoder->cedar);

    twig_frame_pool_cleanup(&decoder->frame_pool, decoder->cedar);

    if (decoder->sps)
        free(decoder->sps);
    if (decoder->pps) {
        twig_cleanup_pps(decoder->pps);
        free(decoder->pps);
    }
    if (decoder->hdr)
        free(decoder->hdr);
    if (decoder->extra_buf)
        twig_free_mem(decoder->cedar, decoder->extra_buf);

    free(decoder);
}