#include "supp/twig_regs.h"
#include "priv/twig_video_priv.h"

#define H264_START_DEC_FRAME    1
#define H264_END_DEC_FRAME      2
#define H264_VBV_BUFFER_SIZE    1048576
#define H264_EXTENDED_DATA_LEN  32

static int twig_h264_parse_extra_data(twig_h264_ctx_t *ctx, uint8_t *extra_data, int size) {
    if (!ctx || !extra_data || size < 7) {
        fprintf(stderr, "Invalid extra data parameters\n");
        return -1;
    }

    if (extra_data[0] == 0x01) {
        printf("AVC format detected\n");
        ctx->is_avc_format = 1;
        ctx->nal_length_size = (extra_data[4] & 0x03) + 1;
        printf("NAL length size: %d bytes\n", ctx->nal_length_size);

        ctx->need_find_sps = 1;
        ctx->need_find_pps = 1;
    } else {
        printf("Annex-B format detected\n");
        ctx->is_avc_format = 0;
        ctx->nal_length_size = 0;
        ctx->need_find_sps = 1;
        ctx->need_find_pps = 1;
    }
    
    return 0;
}

static int twig_h264_allocate_auxiliary_buffers(twig_video_engine_t *engine)
{
    twig_h264_ctx_t *ctx = engine->h264_ctx;
    int mb_width = ctx->mb_width;
    int mb_height = ctx->mb_height;

    int field_mv_col_size = mb_height * (2 - ctx->frame_mbs_only_flag);
    field_mv_col_size = (field_mv_col_size + 1) / 2;
    field_mv_col_size = mb_width * field_mv_col_size * 32;
    
    if (!ctx->direct_8x8_inference_flag)
        field_mv_col_size *= 2;
    
    size_t total_mv_size = field_mv_col_size * engine->video_info.max_ref_frames * 2;
    
    ctx->mv_col_buf = twig_alloc_mem(engine->device, total_mv_size);
    if (!ctx->mv_col_buf) {
        fprintf(stderr, "Failed to allocate MV colocated buffer (%zu bytes)\n", total_mv_size);
        return -1;
    }
    printf("Allocated MV colocated buffer: %zu bytes\n", total_mv_size);
    
    size_t mb_field_size = mb_width * mb_height * 4;
    ctx->mb_field_intra_buf = twig_alloc_mem(engine->device, mb_field_size);
    if (!ctx->mb_field_intra_buf) {
        fprintf(stderr, "Failed to allocate MB field intra buffer\n");
        return -1;
    }

    size_t mb_neighbor_size = mb_width * mb_height * 16;
    ctx->mb_neighbor_buf = twig_alloc_mem(engine->device, mb_neighbor_size);
    if (!ctx->mb_neighbor_buf) {
        fprintf(stderr, "Failed to allocate MB neighbor buffer\n");
        return -1;
    }

    if (engine->video_info.width > 2048) {
        printf("Large frame detected, allocating DRAM buffers\n");
        
        size_t deblock_size = (mb_width + 31) * 16 * 12;
        ctx->deblock_dram_buf = twig_alloc_mem(engine->device, deblock_size);
        if (!ctx->deblock_dram_buf) {
            fprintf(stderr, "Failed to allocate deblock DRAM buffer\n");
            return -1;
        }
        
        size_t intra_pred_size = (mb_width + 63) * 16 * 5;
        ctx->intra_pred_dram_buf = twig_alloc_mem(engine->device, intra_pred_size);
        if (!ctx->intra_pred_dram_buf) {
            fprintf(stderr, "Failed to allocate intra prediction DRAM buffer\n");
            return -1;
        }
        
        printf("Allocated large frame DRAM buffers\n");
    }
    
    return 0;
}

static int twig_h264_allocate_vbv_buffer(twig_video_engine_t *engine) {
    twig_h264_ctx_t *ctx = engine->h264_ctx;

    twig_mem_t *vbv_mem = twig_alloc_mem(engine->device, H264_VBV_BUFFER_SIZE);
    if (!vbv_mem) {
        fprintf(stderr, "Failed to allocate VBV buffer\n");
        return -1;
    }
    
    ctx->vbv_buffer = (uint8_t*)vbv_mem->virt;
    ctx->vbv_buffer_end = ctx->vbv_buffer + H264_VBV_BUFFER_SIZE - 1;
    ctx->vbv_buffer_size = H264_VBV_BUFFER_SIZE;
    ctx->read_ptr = ctx->vbv_buffer;
    ctx->vbv_data_size = 0;
    
    printf("Allocated VBV buffer: %d bytes\n", H264_VBV_BUFFER_SIZE);
    return 0;
}

int twig_h264_init(twig_video_engine_t *engine) {
    if (!engine || !engine->h264_ctx) {
        fprintf(stderr, "Invalid H264 init parameters\n");
        return -1;
    }
    
    twig_h264_ctx_t *ctx = engine->h264_ctx;
    
    ctx->frame_width = engine->video_info.width;
    ctx->frame_height = engine->video_info.height;
    ctx->mb_width = (engine->video_info.width + 15) / 16;
    ctx->mb_height = (engine->video_info.height + 15) / 16;
    ctx->max_ref_frames = engine->video_info.max_ref_frames;
    ctx->frame_mbs_only_flag = 1;
    ctx->direct_8x8_inference_flag = 1;
    
    printf("H264 decoder init: %dx%d (%dx%d MBs), %d ref frames\n",
           ctx->frame_width, ctx->frame_height,
           ctx->mb_width, ctx->mb_height, ctx->max_ref_frames);
    
    ctx->decode_step = H264_STEP_CONFIG_VBV;
    ctx->frame_decode_status = 0;
    ctx->cur_slice_num = 0;
    ctx->need_find_sps = 1;
    ctx->need_find_pps = 1;
    ctx->need_find_iframe = 1;
    ctx->decoded_frames = 0;
    ctx->cur_picture = NULL;
    ctx->ve_active = 0;

    if (engine->video_info.extra_data && engine->video_info.extra_data_size > 0) {
        if (twig_h264_parse_extra_data(ctx, engine->video_info.extra_data, engine->video_info.extra_data_size) < 0) {
            fprintf(stderr, "Failed to parse extra data\n");
            return -1;
        }
    } else {
        ctx->is_avc_format = 0;
        ctx->need_find_sps = 1;
        ctx->need_find_pps = 1;
    }
    
    if (twig_h264_allocate_vbv_buffer(engine) < 0) {
        fprintf(stderr, "Failed to allocate VBV buffer\n");
        return -1;
    }
    
    if (twig_h264_allocate_auxiliary_buffers(engine) < 0) {
        fprintf(stderr, "Failed to allocate auxiliary buffers\n");
        return -1;
    }
    
    printf("H264 decoder initialized successfully\n");
    return 0;
}

void twig_h264_destroy(twig_h264_ctx_t *ctx) {
    if (!ctx)
        return;

    free(ctx);
}

static void twig_h264_configure_registers(twig_video_engine_t *engine) {
    twig_h264_ctx_t *ctx = engine->h264_ctx;
    void *regs = ctx->ve_regs;
    
    if (!regs) {
        fprintf(stderr, "VE registers not available\n");
        return;
    }

    h264_seq_hdr_reg_t seq_hdr = {0};
    seq_hdr.pic_width_in_mbs_minus1 = ctx->mb_width - 1;
    seq_hdr.pic_height_in_map_units_minus1 = ctx->mb_height - 1;
    seq_hdr.frame_mbs_only_flag = ctx->frame_mbs_only_flag;
    seq_hdr.direct_8x8_inference_flag = ctx->direct_8x8_inference_flag;
    seq_hdr.chroma_format_idc = 1;
    twig_writel(seq_hdr.val, regs + H264_SEQ_HDR);
    
    h264_pic_mbsize_reg_t pic_size = {0};
    pic_size.horizontal_macroblock_count_y = ctx->mb_width;
    pic_size.vertical_macroblock_count_y = ctx->mb_height;
    twig_writel(pic_size.val, regs + H264_PIC_MBSIZE);
    
    if (engine->video_info.width > 2048) {
        ve_ipd_dblk_buf_ctrl_reg_t buf_ctrl = {0};
        buf_ctrl.deblock_ctrl = 2;
        buf_ctrl.ipred_ctrl = 2;
        twig_writel(buf_ctrl.val, regs + VE_IPD_DBLK_BUF_CTRL);
        
        if (ctx->deblock_dram_buf)
            twig_writel(ctx->deblock_dram_buf->iommu_addr, regs + VE_DBLK_BUF);

        if (ctx->intra_pred_dram_buf)
            twig_writel(ctx->intra_pred_dram_buf->iommu_addr, regs + VE_IPD_BUF);
    } else {
        ve_ipd_dblk_buf_ctrl_reg_t buf_ctrl = {0};
        buf_ctrl.deblock_ctrl = 0;
        buf_ctrl.ipred_ctrl = 0;
        twig_writel(buf_ctrl.val, regs + VE_IPD_DBLK_BUF_CTRL);
    }
    
    if (ctx->mb_field_intra_buf)
        twig_writel(ctx->mb_field_intra_buf->iommu_addr, regs + H264_FIELD_INTRA_INFO_BUF);

    if (ctx->mb_neighbor_buf)
        twig_writel(ctx->mb_neighbor_buf->iommu_addr, regs + H264_NEIGHBOR_INFO_BUF);
    
    printf("H264 hardware registers configured\n");
}

int twig_h264_init_hardware_context(twig_video_engine_t *engine) {
    if (!engine || !engine->h264_ctx || !engine->h264_ctx->ve_active) {
        fprintf(stderr, "Invalid hardware context\n");
        return -1;
    }
    
    twig_h264_configure_registers(engine);
    
    printf("H264 hardware context initialized\n");
    return 0;
}