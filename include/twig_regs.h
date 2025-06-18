/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Supplemental Cedar hardware register defintions and read/write functions
 *
 * Garbage code by Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_REGS_H_
#define TWIG_REGS_H_

#include <stdint.h>

static inline void twig_writel(void *base, uint8_t offset, uint32_t value) {
    *((volatile uint32_t*)(base + offset)) = value;
}

static inline uint32_t twig_readl(void *base, uint8_t offset) {
    return *((volatile uint32_t*)(base + offset));
}

#define VE_BASE 0x01c0e000

typedef enum {
    VE_CTRL                 = 0x00,  // Sub-Engine Select and RAM type select
    VE_RESET                = 0x04,  // Sub-Engines Reset
    VE_CYCLES_COUNTER       = 0x08,  // Clock Cycles counter
    VE_TIMEOUT              = 0x0c,  // VE Timeout value
    VE_MMCREQ_WNUM          = 0x10,
    VE_CACHEREG_WNUM        = 0x14,
    VE_STATUS               = 0x1c,  // Busy status
    VE_RDDATA_COUNTER       = 0x20,  // DRAM Read counter
    VE_WRDATA_COUNTER       = 0x24,  // DRAM Write counter
    VE_ANAGLYPH_CTRL        = 0x28,  // Anaglyph mode control
    VE_MAF_CTRL             = 0x30,  // Motion adaptive filter config
    VE_MAF_CLIP_TH          = 0x34,
    VE_MAFREF1_LUMA_BUF     = 0x38,
    VE_MAFREF1_CHROMA_BUF   = 0x3c,
    VE_MAFCUR_ADDR          = 0x40,
    VE_MAFREF1_ADDR         = 0x44,
    VE_MAFREF2_ADDR         = 0x48,
    VE_MAFDIFF_GROUP_MAX    = 0x4c,
    VE_IPD_DBLK_BUF_CTRL    = 0x50,  // Required for H264 Decoding
    VE_IPD_BUF              = 0x54,  // Required for H264 Decoding
    VE_DBLK_BUF             = 0x58,  // Required for H264 Decoding
    VE_ARGB_QUEUE_START     = 0x5c,
    VE_ARGB_BLK_SRC1_ADDR   = 0x60,
    VE_ARGB_BLK_SRC2_ADDR   = 0x64,
    VE_ARGB_BLK_DST_ADDR    = 0x68,
    VE_ARGB_SRC_STRIDE      = 0x6c,
    VE_ARGB_DST_STRIDE      = 0x70,
    VE_ARGB_BLK_SIZE        = 0x74,
    VE_ARGB_BLK_FILL_VALUE  = 0x78,
    VE_ARGB_BLK_CTRL        = 0x7c,
    VE_OUTPUT_CHROMA_OFFSET = 0xc4,
    VE_OUTPUT_STRIDE        = 0xc8,
    VE_EXTRA_OUT_STRIDE     = 0xcc,
    VE_ANGL_R_BUF           = 0xd0,
    VE_ANGL_G_BUF           = 0xd4,
    VE_ANGL_B_BUF           = 0xd8,
    VE_EXTRA_OUT_FMT_OFFSET = 0xe8,
    VE_OUTPUT_FORMAT        = 0xec,
    VE_VERSION              = 0xf0,
    VE_DBG_CTRL             = 0xf8,
    VE_DBG_OUTPUT           = 0xfc
} VE_Registers;

#define VE_LUMA_HIST_THR(n)  (0x80 + (n) * 4)
#define VE_LUMA_HIST_VAL(n)  (0x90 + (n) * 4)

#define H264_OFFSET 0x200

typedef enum {
    H264_SEQ_HDR               = 0x00,
    H264_PIC_HDR               = 0x04,
    H264_SLICE_HDR             = 0x08,
    H264_SLICE_HDR2            = 0x0c,
    H264_PRED_WEIGHT           = 0x10,
    H264_VP8_HDR               = 0x14,
    H264_QINDEX                = 0x18,
    H264_QP                    = 0x1c,
    H264_CTRL                  = 0x20,
    H264_TRIGGER               = 0x24,
    H264_STATUS                = 0x28,
    H264_CUR_MBNUM             = 0x2c,
    H264_VLD_ADDR              = 0x30,
    H264_VLD_OFFSET            = 0x34,
    H264_VLD_LEN               = 0x38,
    H264_VLD_END               = 0x3c,
    H264_SDROT_CTRL            = 0x40,
    H264_SDROT_LUMA            = 0x44,
    H264_SDROT_CHROMA          = 0x48,
    H264_OUTPUT_FRAME_INDEX    = 0x4c,
    H264_FIELD_INTRA_INFO_BUF  = 0x50,
    H264_NEIGHBOR_INFO_BUF     = 0x54,
    H264_PIC_MBSIZE            = 0x58,
    H264_PIC_BOUNDARYSIZE      = 0x5c,
    H264_MB_ADDR               = 0x60,
    H264_MB_NB1                = 0x64,
    H264_MB_NB2                = 0x68,
    H264_MB_NB3                = 0x6c,
    H264_MB_NB4                = 0x70,
    H264_MB_NB5                = 0x74,
    H264_MB_NB6                = 0x78,
    H264_MB_NB7                = 0x7c,
    H264_MB_NB8                = 0x80,
    H264_MB_QP                 = 0x90,
    H264_REC_LUMA              = 0xac,
    H264_FWD_LUMA              = 0xb0,
    H264_BACK_LUMA             = 0xb4,
    H264_ERROR                 = 0xb8,
    H264_REC_CHROMA            = 0xd0,
    H264_FWD_CHROMA            = 0xd4,
    H264_BACK_CHROMA           = 0xd8,
    H264_BASIC_BITS            = 0xdc,
    H264_RAM_WRITE_PTR         = 0xe0,
    H264_RAM_WRITE_DATA        = 0xe4,
    H264_ALT_LUMA              = 0xe8,
    H264_ALT_CHROMA            = 0xec,
    H264_SEG_MB_LV0            = 0xf0,
    H264_SEG_MB_LV1            = 0xf4,
    H264_REF_LF_DELTA          = 0xf8,
    H264_MODE_LF_DELTA         = 0xfc
} H264_Registers;

#define VE_SRAM_H264_PRED_WEIGHT_TABLE	0x000
#define VE_SRAM_H264_FRAMEBUFFER_LIST	0x400
#define VE_SRAM_H264_REF_LIST0		    0x640
#define VE_SRAM_H264_REF_LIST1		    0x664
#define VE_SRAM_H264_SCALING_LISTS	    0x800

#endif // TWIG_REGS_H_