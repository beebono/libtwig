/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Supplemental Cedar hardware register defintions with additional information
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_REGS_H_
#define TWIG_REGS_H_

static inline void twig_writel(uint32_t value, void *addr) {
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t twig_readl(void *addr) {
    return *(volatile uint32_t *)addr;
}

#define VE_REG_BASE                      0x01c0e000
#define VE_CTRL                          0x0000  // Sub-Engine Select and RAM type select
#define VE_RESET                         0x0004  // Sub-Engines Reset
#define VE_CYCLES_COUNTER                0x0008  // Clock Cycles counter
#define VE_TIMEOUT                       0x000c  // VE Timeout value
#define VE_MMCREQ_WNUM                   0x0010
#define VE_CACHEREG_WNUM                 0x0014
#define VE_STATUS                        0x001c  // Busy status
#define VE_RDDATA_COUNTER                0x0020  // DRAM Read counter
#define VE_WRDATA_COUNTER                0x0024  // DRAM Write counter
#define VE_ANAGLYPH_CTRL                 0x0028  // Anaglyph mode control
#define VE_MAF_CTRL                      0x0030  // Motion adaptive filter config
#define VE_MAF_CLIP_TH                   0x0034
#define VE_MAFREF1_LUMA_BUF              0x0038
#define VE_MAFREF1_CHROMA_BUF            0x003c
#define VE_MAFCUR_ADDR                   0x0040
#define VE_MAFREF1_ADDR                  0x0044
#define VE_MAFREF2_ADDR                  0x0048
#define VE_MAFDIFF_GROUP_MAX             0x004c
#define VE_IPD_DBLK_BUF_CTRL             0x0050  // Required for H264 Decoding
#define VE_IPD_BUF                       0x0054  // Required for H264 Decoding
#define VE_DBLK_BUF                      0x0058  // Required for H264 Decoding
#define VE_ARGB_QUEUE_START              0x005c
#define VE_ARGB_BLK_SRC1_ADDR            0x0060
#define VE_ARGB_BLK_SRC2_ADDR            0x0064
#define VE_ARGB_BLK_DST_ADDR             0x0068
#define VE_ARGB_SRC_STRIDE               0x006c
#define VE_ARGB_DST_STRIDE               0x0070
#define VE_ARGB_BLK_SIZE                 0x0074
#define VE_ARGB_BLK_FILL_VALUE           0x0078
#define VE_ARGB_BLK_CTRL                 0x007c

#define VE_LUMA_HIST_THR(n)              (0x0080 + (n) * 4) // n = 0..3
#define VE_LUMA_HIST_VAL(n)              (0x0090 + (n) * 4) // n = 0..15

#define VE_OUTPUT_CHROMA_OFFSET          0x00c4
#define VE_OUTPUT_STRIDE                 0x00c8
#define VE_EXTRA_OUT_STRIDE              0x00cc
#define VE_ANGL_R_BUF                    0x00d0
#define VE_ANGL_G_BUF                    0x00d4
#define VE_ANGL_B_BUF                    0x00d8
#define VE_EXTRA_OUT_FMT_OFFSET          0x00e8
#define VE_OUTPUT_FORMAT                 0x00ec
#define VE_VERSION                       0x00f0
#define VE_DBG_CTRL                      0x00f8
#define VE_DBG_OUTPUT                    0x00fc

// H.264 Engine Registers                
#define VE_H264_BASE                     VE_REG_BASE + 0x200  // 0x01c0e200
#define H264_SEQ_HDR                     0x0000
#define H264_PIC_HDR                     0x0004
#define H264_SLICE_HDR                   0x0008
#define H264_SLICE_HDR2                  0x000c
#define H264_PRED_WEIGHT                 0x0010
#define H264_VP8_HDR                     0x0014
#define H264_QINDEX                      0x0018
#define H264_QP                          0x001c
#define H264_CTRL                        0x0020
#define H264_TRIG                        0x0024
#define H264_STATUS                      0x0028
#define H264_CUR_MBNUM                   0x002c
#define H264_VLD_ADDR                    0x0030
#define H264_VLD_OFFSET                  0x0034
#define H264_VLD_LEN                     0x0038
#define H264_VLD_END                     0x003c
#define H264_SDROT_CTRL                  0x0040
#define H264_SDROT_LUMA                  0x0044
#define H264_SDROT_CHROMA                0x0048
#define H264_OUTPUT_FRAME_INDEX          0x004c
#define H264_FIELD_INTRA_INFO_BUF        0x0050
#define H264_NEIGHBOR_INFO_BUF           0x0054
#define H264_PIC_MBSIZE                  0x0058
#define H264_PIC_BOUNDARYSIZE            0x005c
#define H264_MB_ADDR                     0x0060
#define H264_MB_NB1                      0x0064
#define H264_MB_NB2                      0x0068
#define H264_MB_NB3                      0x006c
#define H264_MB_NB4                      0x0070
#define H264_MB_NB5                      0x0074
#define H264_MB_NB6                      0x0078
#define H264_MB_NB7                      0x007c
#define H264_MB_NB8                      0x0080
#define H264_MB_QP                       0x0090
#define H264_REC_LUMA                    0x00ac
#define H264_FWD_LUMA                    0x00b0
#define H264_BACK_LUMA                   0x00b4
#define H264_ERROR                       0x00b8
#define H264_REC_CHROMA                  0x00d0
#define H264_FWD_CHROMA                  0x00d4
#define H264_BACK_CHROMA                 0x00d8
#define H264_BASIC_BITS_DATA             0x00dc
#define H264_RAM_WRITE_PTR               0x00e0
#define H264_RAM_WRITE_DATA              0x00e4
#define H264_ALT_LUMA                    0x00e8
#define H264_ALT_CHROMA                  0x00ec
#define H264_SEG_MB_LV0                  0x00f0
#define H264_SEG_MB_LV1                  0x00f4
#define H264_REF_LF_DELTA                0x00f8
#define H264_MODE_LF_DELTA               0x00fc

            // Structs below provided for reference only, write to or read from registers directly!
#if 0
/*
 * Detailed Bitfield Definitions for Cedar VE General Registers
 */

// MACC_VE_CTRL Register @ offset 0x0000
typedef union {
    uint32_t val;
    struct {
        uint32_t engine_select       : 4;  // 0–3: Engine selector (Use 0x7 for Idle and 0x1 for H264)
        uint32_t reserved0           : 1;
        uint32_t jpeg_enable         : 1;  // 5: JPEG decoder enable
        uint32_t isp_enable          : 1;  // 6: ISP enable
        uint32_t avc_enc_enable      : 1;  // 7: AVC encoder enable
        uint32_t unknown1            : 2;  // 8–9
        uint32_t reserved1           : 6;  // 10–15
        uint32_t mem_type            : 2;  // 16–17: Memory type
        uint32_t reserved2           : 2;  // 18–19
        uint32_t unknown2            : 1;  // 20
        uint32_t wide_pic_enable     : 1;  // 21: Required for >2048px width
        uint32_t reserved3           : 2;  // 22–23
        uint32_t unknown3            : 2;  // 24–25
        uint32_t reserved4           : 2;  // 26–27
        uint32_t unknown4            : 1;  // 28
        uint32_t reserved5           : 3;  // 29–31
    };
} ve_ctrl_reg_t;

// MACC_VE_RESET Register @ offset 0x0004
typedef union {
    uint32_t val;
    struct {
        uint32_t reset_sub_engines   : 1;  // 0
        uint32_t reserved0           : 3;  // 1–3
        uint32_t unknown             : 1;  // 4
        uint32_t reserved1           : 27; // 5–31
    };
} ve_reset_reg_t;

// MACC_VE_CYCLES_COUNTER @ 0x0008
typedef union {
    uint32_t val;
    struct {
        uint32_t count               : 31; // Cycles from enable time
        uint32_t enable              : 1;  // 31
    };
} ve_cycles_counter_reg_t;

// MACC_VE_TIMEOUT @ 0x000c
typedef union {
    uint32_t val;
    struct {
        uint32_t reserved0           : 8;
        uint32_t overtime_val        : 23;
        uint32_t reserved1           : 1;
    };
} ve_timeout_reg_t;

// MACC_VE_STATUS @ 0x001c
typedef union {
    uint32_t val;
    struct {
        uint32_t reserved0           : 9;
        uint32_t status              : 6;  // 0 = Ready, 3 = Busy
        uint32_t reserved1           : 17;
    };
} ve_status_reg_t;

// MACC_VE_RDDATA_COUNTER @ 0x0020
typedef union {
    uint32_t val;
    struct {
        uint32_t read_words          : 31; // Data read from DRAM in 64 bit words
        uint32_t enable              : 1;  // 31
    };
} ve_read_counter_reg_t;

// MACC_VE_WRDATA_COUNTER @ 0x0024
typedef union {
    uint32_t val;
    struct {
        uint32_t write_words         : 31; // Data written to DRAM in 64 bit words
        uint32_t enable              : 1;  // 31
    };
} ve_write_counter_reg_t;

// MACC_VE_ANAGLYPH_CTRL @ 0x0028
typedef union {
    uint32_t val;
    struct {
        uint32_t src_mode            : 2;
        uint32_t reserved0           : 1;
        uint32_t side                : 1;  // 0 = left, 1 = right
        uint32_t mode                : 3;  // 0 = red/blue, ..., 6 = yellow/blue
        uint32_t reserved1           : 1;
        uint32_t colorspace          : 2;  // 0 = YCC, 1 = BT601, 2 = BT709
        uint32_t reserved2           : 21;
        uint32_t enable              : 1;
    };
} ve_anaglyph_ctrl_reg_t;

// MACC_VE_IPD_DBLK_BUF_CTRL @ 0x0050
typedef union {
    uint32_t val;
    struct {
        uint32_t deblock_ctrl        : 2; // 0 = SRAM, 1 = Left 1280 px SRAM, DRAM after, 2 = DRAM
        uint32_t ipred_ctrl          : 2; // 0 = SRAM, 1 = Left 1280 px SRAM, DRAM after, 2 = DRAM
        uint32_t reserved            : 28;
    };
} ve_ipd_dblk_buf_ctrl_reg_t;

// MACC_VE_LUMA_HIST_THRi @ 0x0080 + i*4
typedef union {
    uint32_t val;
    struct {
        uint32_t thr0                : 8;
        uint32_t thr1                : 8;
        uint32_t thr2                : 8;
        uint32_t thr3                : 8;
    };
} ve_luma_hist_thr_reg_t;

// MACC_VE_LUMA_HIST_VALi @ 0x0090 + i*4
typedef union {
    uint32_t val;
    struct {
        uint32_t hist_val            : 20;
        uint32_t reserved            : 12;
    };
} ve_luma_hist_val_reg_t;

// MACC_VE_OUTPUT_CHROMA_OFFSET @ 0x00c4
typedef union {
    uint32_t val;
    struct {
        uint32_t offset              : 28;
        uint32_t reserved            : 4;
    };
} ve_output_chroma_offset_reg_t;

// MACC_VE_OUTPUT_STRIDE @ 0x00c8
typedef union {
    uint32_t val;
    struct {
        uint32_t luma_stride         : 16;
        uint32_t chroma_stride       : 16;
    };
} ve_output_stride_reg_t;

// MACC_VE_EXTRA_OUT_STRIDE @ 0x00cc
typedef union {
    uint32_t val;
    struct {
        uint32_t luma_stride         : 16;
        uint32_t chroma_stride       : 16;
    };
} ve_extra_out_stride_reg_t;

// MACC_VE_EXTRA_OUT_FMT_OFFSET @ 0x00e8
typedef union {
    uint32_t val;
    struct {
        uint32_t offset              : 28;
        uint32_t align               : 2;
        uint32_t format              : 2;
    };
} ve_extra_out_fmt_offset_reg_t;

// MACC_VE_OUTPUT_FORMAT @ 0x00ec
typedef union {
    uint32_t val;
    struct {
        uint32_t extra_format        : 3;
        uint32_t reserved0           : 1;
        uint32_t output_format       : 3; // 0 = 32x32 Tiled, 1 = 128x32 Tiled, 2 = I420, 3 = YV12, 4 = NV12, 5 = NV21
        uint32_t reserved1           : 25;
    };
} ve_output_format_reg_t;

// MACC_VE_VERSION @ 0x00f0
typedef union {
    uint32_t val;
    struct {
        uint32_t version             : 16; // Read this to get version, NOT info struct!
        uint32_t soc_id              : 16;
    };
} ve_version_reg_t;

/*
 * Detailed Bitfield Definitions for H.264 Engine Registers
 */

// MACC_H264_SEQ_HDR Register @ offset 0x0200
typedef union {
    uint32_t val;
    struct {
        uint32_t pic_height_in_map_units_minus1 : 8;  // 7:0
        uint32_t pic_width_in_mbs_minus1        : 8;  // 15:8
        uint32_t direct_8x8_inference_flag      : 1;  // 16
        uint32_t mb_adaptive_frame_field_flag   : 1;  // 17
        uint32_t frame_mbs_only_flag            : 1;  // 18
        uint32_t chroma_format_idc              : 3;  // 21:19
        uint32_t reserved                       : 10; // 31:22
    };
} h264_seq_hdr_reg_t;

// MACC_H264_PIC_HDR Register @ offset 0x0204
typedef union {
    uint32_t val;
    struct {
        uint32_t transform_8x8_mode_flag            : 1;  // 0
        uint32_t constrained_intra_pred_flag        : 1;  // 1
        uint32_t weighted_bipred_idc                : 2;  // 3:2
        uint32_t weighted_pred_flag                 : 1;  // 4
        uint32_t num_ref_idx_l1_default_active_minus1 : 5;  // 9:5
        uint32_t num_ref_idx_l0_default_active_minus1 : 5;  // 14:10
        uint32_t entropy_coding_mode_flag           : 1;  // 15
        uint32_t reserved                           : 16; // 31:16
    };
} h264_pic_hdr_reg_t;

// MACC_H264_SLICE_HDR Register @ offset 0x0208
typedef union {
    uint32_t val;
    struct {
        uint32_t cabac_init_idc                 : 2;  // 1:0
        uint32_t direct_spatial_mv_pred_flag    : 1;  // 2
        uint32_t bottom_field_flag              : 1;  // 3
        uint32_t field_pic_flag                 : 1;  // 4
        uint32_t first_slice_in_picture         : 1;  // 5
        uint32_t reserved0                      : 2;  // 7:6
        uint32_t slice_type                     : 4;  // 11:8
        uint32_t is_reference                   : 1;  // 12
        uint32_t reserved1                      : 3;  // 15:13
        uint32_t first_mb_in_slice_y            : 8;  // 23:16 (first_mb_in_slice / pic_width_in_mbs)
        uint32_t first_mb_in_slice_x            : 8;  // 31:24 (first_mb_in_slice % pic_width_in_mbs)
    };
} h264_slice_hdr_reg_t;

// MACC_H264_SLICE_HDR2 Register @ offset 0x020c
typedef union {
    uint32_t val;
    struct {
        uint32_t slice_beta_offset_div2             : 4;  // 3:0
        uint32_t slice_alpha_c0_offset_div2         : 4;  // 7:4
        uint32_t disable_deblocking_filter_idc      : 2;  // 9:8
        uint32_t reserved0                          : 2;  // 11:10
        uint32_t num_ref_idx_active_override_flag   : 1;  // 12
        uint32_t reserved1                          : 3;  // 15:13
        uint32_t num_ref_idx_l1_active_minus1       : 5;  // 20:16
        uint32_t reserved2                          : 3;  // 23:21
        uint32_t num_ref_idx_l0_active_minus1       : 5;  // 28:24
        uint32_t reserved3                          : 3;  // 31:29
    };
} h264_slice_hdr2_reg_t;

// MACC_H264_PRED_WEIGHT Register @ offset 0x0210
typedef union {
    uint32_t val;
    struct {
        uint32_t luma_log2_weight_denom  : 3;  // 2:0
        uint32_t reserved0               : 1;  // 3
        uint32_t chroma_log2_weight_denom: 3;  // 6:4
        uint32_t reserved1               : 25; // 31:7
    };
} h264_pred_weight_reg_t;

// MACC_H264_VP8_HDR Register @ offset 0x0214
typedef union {
    uint32_t val;
    struct {
        uint32_t no_lpf                           : 1;  // 0 (VP8 version)
        uint32_t filter_type_vp8                  : 1;  // 1 (VP8 version)
        uint32_t use_billenar_mc_filter           : 1;  // 2 (VP8 version)
        uint32_t full_pixel                       : 1;  // 3 (VP8 version)
        uint32_t update_mb_segmentation_map       : 1;  // 4
        uint32_t mb_segment_abs_delta             : 1;  // 5
        uint32_t segmentation_enabled             : 1;  // 6
        uint32_t last_frame_filter_type           : 1;  // 7
        uint32_t sharpness_level                  : 3;  // 10:8
        uint32_t filter_type                      : 1;  // 11
        uint32_t current_frame_filter_level       : 6;  // 17:12
        uint32_t mode_ref_lf_delta_enabled        : 1;  // 18
        uint32_t mode_ref_lf_delta_update         : 1;  // 19
        uint32_t token_partition_number           : 2;  // 21:20
        uint32_t mb_no_coeff_skip                 : 1;  // 22
        uint32_t refresh_entropy_probs            : 1;  // 23
        uint32_t reload_entropy_probs             : 1;  // 24
        uint32_t ref_frame_sign_bias_golden_frame : 1;  // 25
        uint32_t ref_frame_sign_bias_altref_frame : 1;  // 26
        uint32_t last_frame_type                  : 1;  // 27
        uint32_t last_frame_sharpness_level       : 3;  // 30:28
        uint32_t current_frame_type               : 1;  // 31
    };
} h264_vp8_hdr_reg_t;

// MACC_H264_QINDEX Register @ offset 0x0218
typedef union {
    uint32_t val;
    struct {
        uint32_t base_qindex   : 7;  // 6:0
        uint32_t y1dc_delta_q  : 5;  // 11:7
        uint32_t y2dc_delta_q  : 5;  // 16:12
        uint32_t y2ac_delta_q  : 5;  // 21:17
        uint32_t uvdc_delta_q  : 5;  // 26:22
        uint32_t uvac_delta_q  : 5;  // 31:27
    };
} h264_qindex_reg_t;

// MACC_H264_QP Register @ offset 0x021c
typedef union {
    uint32_t val;
    struct {
        uint32_t pic_init_qp_slice_qp_delta       : 6;  // 5:0
        uint32_t reserved0                        : 2;  // 7:6
        uint32_t chroma_qp_index_offset           : 6;  // 13:8
        uint32_t reserved1                        : 2;  // 15:14
        uint32_t second_chroma_qp_index_offset    : 6;  // 21:16
        uint32_t reserved2                        : 2;  // 23:22
        uint32_t default_scaling_matrix           : 1;  // 24
        uint32_t unknown                          : 7;  // 31:25
    };
} h264_qp_reg_t;

// MACC_H264_CTRL Register @ offset 0x0220
typedef union {
    uint32_t val;
    struct {
        uint32_t slice_dec_finish_int_en : 1;  // 0
        uint32_t dec_error_int_en        : 1;  // 1
        uint32_t vld_data_req_int_en     : 1;  // 2
        uint32_t reserved0               : 5;  // 7:3
        uint32_t disable_writing_rec_picture : 1;  // 8
        uint32_t enable_extra_output_picture : 1;  // 9
        uint32_t mcri_cache_enable       : 1;  // 10
        uint32_t reserved1               : 2;  // 12:11
        uint32_t enable_luma_histogram_output : 1; // 13
        uint32_t reserved_hist           : 11; // 24:14
        uint32_t eptb_detection_bypass   : 1;  // 24
        uint32_t enable_start_code_detection : 1; // 25
        uint32_t avs_demulate_enable     : 1;  // 26
        uint32_t reserved4               : 1;  // 27
        uint32_t is_avs                  : 1;  // 28
        uint32_t is_vp8_dec              : 1;  // 29
        uint32_t reserved5               : 2;  // 31:30
    };
} h264_ctrl_reg_t;

// MACC_H264_TRIG Register @ offset 0x0224
typedef union {
    uint32_t val;
    struct {
        uint32_t function  : 4;  // 3:0  Set 2 for bit read, 4 for se, 5 for ue, 7 for reset/init, 8 to start decoding 1 slice
        uint32_t stcd_type : 2;  // 5:4
        uint32_t reserved  : 2;  // 7:6
        uint32_t parameter : 24; // 31:8  Number of bits to read
    };
} h264_trig_reg_t;

// MACC_H264_STATUS Register @ offset 0x0228
typedef union {
    uint32_t val;
    struct {
        uint32_t slice_decode_finish_interrupt : 1;  // 0
        uint32_t decode_error_interrupt        : 1;  // 1
        uint32_t vld_data_req_interrupt      : 1;  // 2
        uint32_t over_time_interrupt         : 1;  // 3
        uint32_t reserved0                   : 4;  // 7:4
        uint32_t vld_busy                    : 1;  // 8
        uint32_t is_busy                     : 1;  // 9
        uint32_t mvp_busy                    : 1;  // 10
        uint32_t iq_it_busy                  : 1;  // 11
        uint32_t mcri_busy                   : 1;  // 12
        uint32_t intra_pred_busy             : 1;  // 13
        uint32_t irec_busy                   : 1;  // 14
        uint32_t dblk_busy                   : 1;  // 15
        uint32_t more_data_flag              : 1;  // 16
        uint32_t vp8_upprob_busy             : 1;  // 17
        uint32_t vp8_busy                    : 1;  // 18
        uint32_t reserved1                   : 1;  // 19
        uint32_t intram_busy                 : 1;  // 20
        uint32_t it_busy                     : 1;  // 21
        uint32_t bs_dma_busy                 : 1;  // 22
        uint32_t wb_busy                     : 1;  // 23
        uint32_t avs_busy                    : 1;  // 24
        uint32_t avs_idct_busy               : 1;  // 25
        uint32_t reserved2                   : 1;  // 26
        uint32_t stcd_busy                   : 1;  // 27
        uint32_t start_code_type             : 3;  // 30:28
        uint32_t start_code_detected         : 1;  // 31
    };
} h264_status_reg_t;

// MACC_H264_CUR_MBNUM Register @ offset 0x022c
typedef union {
    uint32_t val;
    struct {
        uint32_t current_decoded_macroblock_nr : 32; // 31:0
    };
} h264_cur_mbnum_reg_t;

// MACC_H264_VLD_ADDR Register @ offset 0x0230
typedef union {
    uint32_t val;
    struct {
        uint32_t vld_address_hi_bits  : 4;  // 3:0 (for addesses beyond 256 MB)
        uint32_t vld_address_low_bits : 24; // 27:4 (four first bits dropped from address)
        uint32_t slice_data_valid     : 1;  // 28
        uint32_t last_slice_data      : 1;  // 29
        uint32_t first_slice_data     : 1;  // 30
        uint32_t reserved             : 1;  // 31
    };
} h264_vld_addr_reg_t;

// MACC_H264_VLD_OFFSET Register @ offset 0x0234
typedef union {
    uint32_t val;
    struct {
        uint32_t vld_offset_in_bits : 32; // 31:0
    };
} h264_vld_offset_reg_t;

// MACC_H264_VLD_LEN Register @ offset 0x0238
typedef union {
    uint32_t val;
    struct {
        uint32_t vld_length_in_bits : 32; // 31:0
    };
} h264_vld_len_reg_t;

// MAC_H264_VLD_END Register @ offset 0x023c
typedef union {
    uint32_t val;
    struct {
        uint32_t vld_end_address : 32; // 31:0
    };
} h264_vld_end_reg_t;

// MACC_H264_SDROT_CTRL Register @ offset 0x0240
typedef union {
    uint32_t val;
    struct {
        uint32_t rot_angle      : 3;  // 2:0
        uint32_t r1             : 5;  // 7:3
        uint32_t scale_precision: 4;  // 11:8
        uint32_t field_scale_mode: 1; // 12
        uint32_t bottom_field_sel: 1; // 13
        uint32_t r0             : 18; // 31:14
    };
} h264_sdrot_ctrl_reg_t;

// MAC_H264_OUTPUT_FRAME_INDEX Register @ offset 0x024c
typedef union {
    uint32_t val;
    struct {
        uint32_t output_frame_index_in_dpb : 32; // 31:0
    };
} h264_output_frame_index_reg_t;

// MACC_H264_PIC_MBSIZE Register @ offset 0x0258
typedef union {
    uint32_t val;
    struct {
        uint32_t vertical_macroblock_count_y : 8;  // 7:0 (VP8: number of vertical macroblocks (Y dimensions))
        uint32_t horizontal_macroblock_count_y : 8; // 15:8 (VP8: number of horizontal macroblocks (Y dimensions))
        uint32_t unknown                     : 16; // 31:16
    };
} h264_pic_mbsize_reg_t;

// MAC_H264_PIC_BOUNDARYSIZE Register @ offset 0x025c
typedef union {
    uint32_t val;
    struct {
        uint32_t picture_height_in_pixels : 16; // 15:0
        uint32_t picture_width_in_pixels  : 16; // 31:16
    };
} h264_pic_boundarysize_reg_t;

// MACC_H264_MB_ADDR Register @ offset 0x0260
typedef union {
    uint32_t val;
    struct {
        uint32_t current_decoded_macroblock_vertical_position   : 8;  // 7:0
        uint32_t current_decoded_macroblock_horizontal_position : 8;  // 15:8
        uint32_t unknown                                        : 16; // 31:16
    };
} h264_mb_addr_reg_t;

// MACC_H264_REC_LUMA Register @ offset 0x02ac
typedef union {
    uint32_t val;
    struct {
        uint32_t reconstruct_buffer_luma_color_component : 32; // 31:0
    };
} h264_rec_luma_reg_t;

// MACC_H264_FWD_LUMA Register @ offset 0x02b0
typedef union {
    uint32_t val;
    struct {
        uint32_t forward_prediction_buffer_luma_color_component : 32; // 31:0
    };
} h264_fwd_luma_reg_t;

// MACC_H264_BACK_LUMA Register @ offset 0x02b4
typedef union {
    uint32_t val;
    struct {
        uint32_t back_buffer_luma_color_component : 32; // 31:0
    };
} h264_back_luma_reg_t;

// MACC_H264_ERROR Register @ offset 0x02b8
typedef union {
    uint32_t val;
    struct {
        uint32_t no_more_data_error : 1;  // 0
        uint32_t mbh_error          : 1;  // 1
        uint32_t ref_idx_error      : 1;  // 2
        uint32_t block_error        : 1;  // 3
        uint32_t reserved           : 28; // 31:4
    };
} h264_error_reg_t;

// MACC_H264_REC_CHROMA Register @ offset 0x02d0
typedef union {
    uint32_t val;
    struct {
        uint32_t reconstruct_buffer_chroma_color_component : 32; // 31:0
    };
} h264_rec_chroma_reg_t;

// MACC_H264_FWD_CHROMA Register @ offset 0x02d4
typedef union {
    uint32_t val;
    struct {
        uint32_t forward_prediction_buffer_chroma_color_component : 32; // 31:0
    };
} h264_fwd_chroma_reg_t;

// MACC_H264_BACK_CHROMA Register @ offset 0x02d8
typedef union {
    uint32_t val;
    struct {
        uint32_t back_buffer_chroma_color_component : 32; // 31:0
    };
} h264_back_chroma_reg_t;

// MACC_H264_BASIC_BITS_DATA Register @ offset 0x02dc
typedef union {
    uint32_t val;
    struct {
        uint32_t unknown : 32; // 31:0   Maybe used as a bitreader scratch location?
    };
} h264_basic_bits_data_reg_t;

// MACC_H264_RAM_WRITE_PTR Register @ offset 0x02e0
typedef union {
    uint32_t val;
    struct {
        uint32_t relative_address_to_write_to : 32; // 31:0 This increments automatically after a write!
    };
} h264_ram_write_ptr_reg_t;

// MACC_H264_RAM_WRITE_DATA Register @ offset 0x2e4
typedef union {
    uint32_t val;
    struct {
        uint32_t data_to_write_to_ve_sram : 32; // 31:0 
    };
} h264_ram_write_data_reg_t;

/* SRAM Layout map for H264 decoding:
0x000 - 0x2ff: Prediction weight table
0x400 - 0x63f: Framebuffer list
0x640 - ?    : Reference Picture list 0
0x664 - ?    : Reference Picture list 1
0x800 - 0x8df: Scaling lists            

Expected prediction weight tables:
uint32_t luma_l0[32];
uint32_t chroma_l0[32][2];
uint32_t luma_l1[32];
uint32_t chroma_l1[32][2];
each has bit 24:16 = signed offset
         bit 8:0 = signed weight

Expected Framebuffer List:
struct {
   uint32_t top_pic_order_cnt;
   uint32_t bottom_pic_order_cnt;
   uint32_t flags; // bit 0-1: top ref type: 0x0 = short, 0x1 = long, 0x2 = no ref
                   // bit 4-5: bottom ref type:  0x0 = short, 0x1 = long, 0x2 = no ref
                   // bit 8-9: picture type: 0x0 = frame, 0x1 = field, 0x2 = mbaff
   uint32_t luma_addr;
   uint32_t chroma_addr;
   uint32_t extra_buffer_top_addr;    // prediction buffers?
   uint32_t extra_buffer_bottom_addr; // size = pic_width_in_mbs * pic_height_in_mbs * 32
   uint32_t unknown; // = 0x0
} framebuffer_list[18];
 
Expected Reference Frame Array:
uint8_t ref_picture[X]; Where X = (index to framebuffer list) * 2 + (bottom_field ? 1 : 0)

Expected Scaling List Arrays:
uint8_t ScalingList8x8[2][64];
uint8_t ScalingList4x4[6][16];                                                          */

// MACC_H264_ALT_LUMA Register @ offset 0x02e8
typedef union {
    uint32_t val;
    struct {
        uint32_t alternative_buffer_luma_color_component : 32; // 31:0
    };
} h264_alt_luma_reg_t;

// MACC_H264_ALT_CHROMA Register @ offset 0x02ec
typedef union {
    uint32_t val;
    struct {
        uint32_t alternative_buffer_chroma_color_component : 32; // 31:0
    };
} h264_alt_chroma_reg_t;

// MACC_H264_SEG_MB_LV0 Register @ offset 0x02f0
typedef union {
    uint32_t val;
    struct {
        uint32_t segment_feature_data_0_0 : 8;  // 7:0
        uint32_t segment_feature_data_0_1 : 8;  // 15:8
        uint32_t segment_feature_data_0_2 : 8;  // 23:16
        uint32_t segment_feature_data_0_3 : 8;  // 31:24
    };
} h264_seg_mb_lv0_reg_t;

// MACC_H264_SEG_MB_LV1 Register @ offset 0x02f4
typedef union {
    uint32_t val;
    struct {
        uint32_t segment_feature_data_1_0 : 8;  // 7:0
        uint32_t segment_feature_data_1_1 : 8;  // 15:8
        uint32_t segment_feature_data_1_2 : 8;  // 23:16
        uint32_t segment_feature_data_1_3 : 8;  // 31:24
    };
} h264_seg_mb_lv1_reg_t;

// MACC_H264_REF_LF_DELTA Register @ offset 0x02f8
typedef union {
    uint32_t val;
    struct {
        uint32_t ref_lf_deltas_0 : 7;  // 6:0
        uint32_t ref_lf_deltas_1 : 7;  // 14:8
        uint32_t ref_lf_deltas_2 : 8;  // 22:16
        uint32_t ref_lf_deltas_3 : 7;  // 30:24
        uint32_t reserved        : 1;  // 31
    };
} h264_ref_lf_delta_reg_t;

// MACC_H264_MODE_LF_DELTA Register @ offset 0x02fc
typedef union {
    uint32_t val;
    struct {
        uint32_t mode_lf_deltas_0 : 7;  // 6:0
        uint32_t mode_lf_deltas_1 : 7;  // 14:8
        uint32_t mode_lf_deltas_2 : 8;  // 22:16
        uint32_t mode_lf_deltas_3 : 7;  // 30:24
        uint32_t reserved         : 1;  // 31
    };
} h264_mode_lf_delta_reg_t;
#endif

#endif // TWIG_REGS_H_