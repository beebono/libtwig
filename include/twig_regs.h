/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
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

#define STARTCODE_DETECT_E                  (1<<25)
#define EPTB_DETECTION_BY_PASS              (1<<24)
#define VLD_BUSY                            (1<<8)

#define N_BITS(n)                           (n<<8)
#define SHOW_BITS                           1
#define GET_BITS                            2
#define FLUSH_BITS                          3
#define GET_VLCSE                           4
#define GET_VLCUE                           5
#define SYNC_BYTE                           6
#define INIT_SWDEC                          7
#define DECODE_SLICE                        8
#define START_CODE_DETECT                   12
#define START_CODE_TERMINATE                13

#define REG_SPS                             0x200
#define REG_PPS                             0x204
#define REG_SHS                             0x208
#define REG_SHS2                            0x20c
#define REG_SHS_WP                          0x210
#define REG_SHS_QP                          0x21c
#define REG_FUNC_CTRL                       0x220
#define REG_TRIGGER_TYPE                    0x224
#define REG_FUNC_STATUS                     0x228
#define REG_CUR_MB_NUM                      0x22c
#define REG_VLD_BITSTREAM_ADDR              0x230
#define REG_VLD_OFFSET                      0x234
#define REG_VLD_BIT_LENGTH                  0x238
#define REG_VLD_END_ADDR                    0x23c
#define REG_SHS_RECONSTRUCT_FRM_BUF_INDEX   0x24c
#define REG_MB_FIELD_INTRA_INFO_ADDR        0x250
#define REG_MB_NEIGHBOR_INFO_ADDR           0x254
#define REG_MB_ADDR                         0x260
#define REG_ERROR_CASE                      0x2b8
#define REG_BASIC_BITS_RETURN_DATA          0x2dc
#define REG_STCD_OFFSET                     0x2f0
#define REG_SRAM_PORT_RW_OFFSET             0x2e0
#define REG_SRAM_PORT_RW_DATA               0x2e4

            // Structs below provided for reference only, write to or read from registers directly!
#if 0
typedef struct {                                                    // 0x00
    volatile unsigned pic_height_mb_minus1                  : 8;	//lsb	PIC_HEIGHT_IN_MAP_UNITS_MINUS1
    volatile unsigned pic_width_mb_minus1                   : 8;	// | 	PIC_WIDTH_IN_MAP_UNITS_MINUS1
    volatile unsigned direct_8x8_inference_flag             : 1;	// |	DIRECT_8X8_INFERENCE_FLAG
    volatile unsigned mb_adaptive_frame_field_flag          : 1;	// |	MB_ADAPTIVE_FRAME_FIELD_FLAG
    volatile unsigned frame_mbs_only_flag                   : 1;	// |	FRAME_MBS_ONLY_FLAG
    volatile unsigned chroma_format_idc                     : 3;	// |	CHROMA_FORMAT_IDC
    volatile unsigned r2                                    : 10;	//msb  Reserved
} twig_reg_sps;

typedef struct {                                                    // 0x04
    volatile unsigned transform_8x8_mode_flag               : 1;	//lsb	TRANSFROM_8X8_MODE_FLAG
    volatile unsigned constrained_intra_pred_flag           : 1;    // |	CONSTRAINED_INTRA_PRED_FLAG
    volatile unsigned weighted_bipred_idc                   : 2;	// | 	WEIGHTED_BIPRED_IDC
    volatile unsigned weighted_pred_idc                     : 1;    // |	WEIGHTED_PRED_FLAG
    volatile unsigned num_ref_idx_l1_active_minus1_pic      : 5;	// |	NUM_REF_IDX_L0_ACTIVE_MINUS1_PIC
    volatile unsigned num_ref_idx_l0_active_minus1_pic      : 5;	// |	NUM_REF_IDX_L1_ACTIVE_MINUS1_PIC
    volatile unsigned entropy_coding_mode_flag              : 1;	// |	ENTROPY_CODING_MODE_FLAG
    volatile unsigned r0                                    : 16;	//msb  Reserved
} twig_reg_pps;

typedef struct {                                                    // 0x08
    volatile unsigned bCabac_init_idc                       : 2;	//lsb	CABAC_INIT_IDC
    volatile unsigned direct_spatial_mv_pred_flag           : 1;    // |	DIRECT_SPATIAL_MV_PRED_FLAG
    volatile unsigned bottom_field_flag                     : 1;	// | 	BOTTOM_FIELD_FLAG
    volatile unsigned field_pic_flag                        : 1;    // |	FIELD_PIC_FLAG
    volatile unsigned first_slice_in_pic                    : 1;	// |	FIRST_SLICE_IN_PIC
    volatile unsigned r0                                    : 2;    // |	Reserved
    volatile unsigned slice_type                            : 4;	// |	NAL_REF_FLAG
    volatile unsigned nal_ref_flag                          : 1;	// |	NAL_REF_FLAG
    volatile unsigned r1                                    : 3;    // |	Reserved
    volatile unsigned first_mb_y                            : 8;	// |	FIRST_MB_Y
    volatile unsigned first_mb_x                            : 8;	//msb	FIRST_MB_x
} twig_reg_shs;

typedef struct {                                                    // 0x0c
    volatile unsigned slice_beta_offset_div2                : 4;	//lsb	SLICE_BETA_OFFSET_DIV2
    volatile unsigned slice_alpha_c0_offset_div2            : 4;    // |	SLICE_ALPHA_C0_OFFSET_DIV2
    volatile unsigned disable_deblocking_filter_idc         : 2;	// | 	DISABLE_DEBLOCKING_FILTER_IDC
    volatile unsigned r0                                    : 2;    // |	Reserved
    volatile unsigned num_ref_idx_active_override_flag      : 1;	// |	NUM_REF_IDX_ACTIVE_OVERRIDE_FLAG
    volatile unsigned r1                                    : 3;    // |	Reserved
    volatile unsigned num_ref_idx_l1_active_minus1_slice    : 5;	// |	NUM_REF_IDX_L1_ACTIVE_MINUS1_SLICE
    volatile unsigned r2                                    : 3;    // |	Reserved
    volatile unsigned num_ref_idx_l0_active_minus1_slice    : 5;	// |	NUM_REF_IDX_L0_ACTIVE_MINUS1_SLICE
    volatile unsigned r3                                    : 3;	//msb  Reserved
} twig_reg_shs2;

typedef struct {                                                    // 0x10
    volatile unsigned luma_log2_weight_denom                : 3;	//lsb	LUMA_LOG2_WEIGHT_DENOM
    volatile unsigned r0                                    : 1;    // |	Reserved
    volatile unsigned chroma_log2_weight_denom              : 3;	// |	LUMA_LOG2_WEIGHT_DENOM
    volatile unsigned r1                                    : 25;	//msb	Reserved
} twig_reg_shs_wp;

typedef struct {                                                    // 0x1c
    volatile unsigned slice_qpy                             : 6;	//lsb	SLICE_QPY
    volatile unsigned r0                                    : 2;	// |	Reserved
    volatile unsigned chroma_qp_index_offset                : 6;	// |	CHROMA_QP_INDEX_OFFSET
    volatile unsigned r1                                    : 2;    // |	Reserved
    volatile unsigned second_chroma_qp_index_offset         : 6;	// |	SECOND_CHROMA_QP_INDEX_OFFSET
    volatile unsigned r2                                    : 2;    // |	Reserved
    volatile unsigned scaling_matix_flat_flag               : 1;	// |	SCALING_MATRIX_FLAT_FLAG
    volatile unsigned r3                                    : 7;	//msb  Reserved
} twig_reg_shs_qp;

typedef struct {                                                    // 0x20
    volatile  unsigned slice_decode_finish_interrupt_enable : 1;	// bit0
    volatile  unsigned decode_error_intrupt_enable          : 1;	// bit1
    volatile  unsigned vld_data_request_interrupt_enable    : 1;	// bit2
    volatile  unsigned r0                                   : 5;	// bit3~7
    volatile  unsigned not_write_recons_pic_flag            : 1;	// bit8
    volatile  unsigned write_scale_rotated_pic_flag         : 1;	// bit9
    volatile  unsigned mcri_cache_enable                    : 1;	// bit10
    volatile  unsigned r1                                   : 13;	// bit11~23
    volatile  unsigned eptb_detection_by_pass               : 1;	// bit24
    volatile  unsigned startcode_detect_enable              : 1;	// bit25
    volatile  unsigned r2                                   : 1;	// bit26
    volatile  unsigned r3                                   : 1;	// bit27
    volatile  unsigned r4                                   : 1;    // bit28
    volatile  unsigned r5                                   : 1;    // bit29
    volatile  unsigned r6                                   : 2;	// bit30~31
} twig_reg_function_ctrl;

typedef struct {                                                    // 0x24
    volatile  unsigned trigger_type_pul                     : 4;    // bit0~3
    volatile  unsigned stcd_type                            : 2;    // bit4~5
    volatile  unsigned r0                                   : 2;    // bit6~7
    volatile  unsigned n_bits                               : 6;    // bit8~13
    volatile  unsigned r1                                   : 2;    // bit14~15
    volatile  unsigned bin_lens                             : 3;    // bit16~18
    volatile  unsigned r2                                   : 5;    // bit19~23
    volatile  unsigned probability                          : 8;    // bit24~31
} twig_reg_trigger_type;

typedef struct {                                                    // 0x28
    volatile  unsigned slice_decode_finish_interrupt        : 1;    // bit0
    volatile  unsigned decode_error_interrupt               : 1;    // bit1
    volatile  unsigned vld_data_req_interrupt               : 1;    // bit2
    volatile  unsigned over_time_interrupt                  : 1;    // bit3
    volatile  unsigned r0                                   : 4;    // bit4~7
    volatile  unsigned vld_busy                             : 1;    // bit8
    volatile  unsigned is_busy                              : 1;    // bit9
    volatile  unsigned mvp_busy                             : 1;    // bit10
    volatile  unsigned iq_it_bust                           : 1;    // bit11
    volatile  unsigned mcri_busy                            : 1;    // bit12
    volatile  unsigned intra_pred_busy                      : 1;    // bit13
    volatile  unsigned irec_busy                            : 1;    // bit14
    volatile  unsigned dblk_busy                            : 1;    // bit15
    volatile  unsigned more_data_flag                       : 1;    // bit16
    volatile  unsigned r1                                   : 1;    // bit17
    volatile  unsigned r2                                   : 1;    // bit18
    volatile  unsigned r3                                   : 1;    // bit19
    volatile  unsigned intram_busy                          : 1;    // bit20
    volatile  unsigned it_busy                              : 1;    // bit21
    volatile  unsigned bs_dma_busy                          : 1;    // bit22
    volatile  unsigned wb_busy                              : 1;    // bit23
    volatile  unsigned r4                                   : 1;    // bit24
    volatile  unsigned r5                                   : 1;    // bit25
    volatile  unsigned r6                                   : 1;    // bit26
    volatile  unsigned stcd_busy                            : 1;    // bit27
    volatile  unsigned startcode_type                       : 3;    // bit28~30
    volatile  unsigned startcode_detected                   : 1;    // bit31
} twig_reg_function_status;

typedef struct {                                                    // 0x2c
    volatile  unsigned cur_mb_number                        : 16;   // bit0~15
    volatile  unsigned r0                                   : 16;   // bit16~31
} twig_reg_cur_mb_num;

typedef struct {                                                    // 0x30
    volatile  unsigned vbv_base_addr_high4                  : 4;    // bit0~3
    volatile  unsigned vbv_base_addr_low24                  : 24;   // bit4~27
    volatile  unsigned slice_data_valid                     : 1;    // bit28
    volatile  unsigned last_slice_data                      : 1;    // bit29
    volatile  unsigned first_slice_data                     : 1;    // bit30
    volatile  unsigned r0                                   : 1;    // bit31
} twig_reg_vld_bitstream_addr;

typedef struct {                                                    // 0x34
    volatile  unsigned  vld_bit_offset                      : 30;   // bit0~29
    volatile  unsigned  r0                                  : 2;    // bit30~31
} twig_reg_vld_offset;

typedef struct {                                                    // 0x38
    volatile  unsigned vld_bit_length                       : 29;   // bit0~28
    volatile  unsigned r0                                   : 3;    // bit29~31
} twig_reg_vld_bit_length;

typedef struct {                                                    // 0x3c
    volatile  unsigned vbv_byte_end_addr                    : 32;
} twig_reg_vld_end_addr;

typedef struct {                                                    // 0x4c 
    volatile  unsigned cur_reconstruct_frame_buf_index      : 5;    // bit0~4
    volatile  unsigned r0                                   : 27;   // bit5~31
} twig_reg_shs_reconstruct_frmbuf_index;                  
                                                                       
typedef struct {                                                    // 0x50                                                   
    volatile  unsigned mb_field_intra_info_addr             : 32;   // bit8~31
} twig_reg_mb_field_intra_info_addr;                      
                                                                           
typedef struct {                                                    // 0x54                                                   
    volatile  unsigned mb_neighbor_info_addr                : 32;   // bit8~31
} twig_reg_mb_neighbor_info_addr;

typedef struct {                                                    // 0x60
    volatile  unsigned mb_y                                 : 7;    // bit0~6
    volatile  unsigned r0                                   : 1;    // bit7
    volatile  unsigned mb_x                                 : 7;    // bit8~14
    volatile  unsigned r1                                   : 17;   // bit15~31
} twig_reg_mb_addr;

typedef struct {                                                    // 0xb8
    volatile unsigned no_more_data_error                    : 1;    // bit0
    volatile unsigned mbh_error                             : 1;    // bit1
    volatile unsigned ref_idx_error                         : 1;    // bit2
    volatile unsigned block_error                           : 1;    // bit3
    volatile unsigned r0                                    : 28;
} twig_reg_error_case;

typedef struct {                                                    // 0xe0
    volatile unsigned sram_addr                              : 32;  // bit2~bit11
} twig_reg_sram_port_rw_offset;

typedef struct {                                                    // 0xe4
    volatile unsigned sram_data                              : 32;  // bit0~bit31               
} twig_reg_sram_port_rw_data;

typedef struct {
    volatile unsigned top_ref_type	    :2;
    volatile unsigned r0				:2;
    volatile unsigned bot_ref_type	    :2;
    volatile unsigned r1				:2;
    volatile unsigned frm_struct		:2;
    volatile unsigned r2				:22;
} twig_reg_frame_struct_ref_info;
#endif

#endif