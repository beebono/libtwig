#include "supp/twig_regs.h"
#include "priv/twig_video_priv.h"

static const twig_video_config_t default_video_config = {
    .decode_key_frames_only = 0,
    .skip_b_frames_if_delay = 0,
    .max_output_width = 1920,
    .max_output_height = 1080,
    .enable_performance_mode = 0
};

static const twig_sbm_config_t default_sbm_config = {
    .buffer_size = 2 * 1024 * 1024,
    .max_frame_num = SBM_FRAME_FIFO_SIZE
};

static int twig_video_engine_init_hardware(twig_video_engine_t *engine) {
    if (!engine || !engine->device) {
        fprintf(stderr, "Invalid engine or device\n");
        return -1;
    }

    void *regs = twig_get_ve_regs(engine->device, engine->video_info.width >= 2048 ? 1 : 0);
    if (!regs) {
        fprintf(stderr, "Failed to get VE registers\n");
        return -1;
    }

    uint32_t version = twig_readl(regs + 0xf0);
    printf("VE hardware version: 0x%x\n", version >> 16);

    twig_writel(0x00130001 | (engine->video_info.width >= 2048 ? 0x00200000 : 0), regs + VE_CTRL);

    twig_writel(0x00130007, regs + VE_RESET);
    usleep(1000);
    twig_writel(0x00130001 | (engine->video_info.width >= 2048 ? 0x00200000 : 0), regs + VE_CTRL);
    
    engine->h264_ctx->ve_regs = regs;
    engine->h264_ctx->ve_active = 1;
    
    return 0;
}

static void twig_video_engine_shutdown_hardware(twig_video_engine_t *engine) {
    if (!engine || !engine->h264_ctx || !engine->h264_ctx->ve_active)
        return;

    twig_put_ve_regs(engine->device);
    engine->h264_ctx->ve_active = 0;
    engine->h264_ctx->ve_regs = NULL;
}

EXPORT twig_video_engine_t *twig_video_engine_create(twig_dev_t *dev, twig_video_config_t *config,
                                                    twig_video_info_t *video_info) {
    if (!dev || !video_info) {
        fprintf(stderr, "Invalid parameters\n");
        return NULL;
    }

    if (video_info->width <= 0 || video_info->height <= 0 ||
        video_info->width > 4096 || video_info->height > 2304) {
        fprintf(stderr, "Invalid video dimensions: %dx%d\n", 
                video_info->width, video_info->height);
        return NULL;
    }
    
    twig_video_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) {
        fprintf(stderr, "Failed to allocate video engine\n");
        return NULL;
    }

    if (pthread_mutex_init(&engine->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize mutex\n");
        free(engine);
        return NULL;
    }

    engine->device = dev;
    engine->config = config ? *config : default_video_config;
    engine->video_info = *video_info;
    
    engine->h264_ctx = calloc(1, sizeof(twig_h264_ctx_t));
    if (!engine->h264_ctx) {
        fprintf(stderr, "Failed to allocate H264 context\n");
        goto err_cleanup;
    }

    if (twig_video_engine_init_hardware(engine) < 0) {
        fprintf(stderr, "Failed to initialize VE hardware\n");
        goto err_cleanup;
    }
    
    if (twig_h264_init(engine) < 0) {
        fprintf(stderr, "Failed to initialize H264 decoder\n");
        goto err_cleanup;
    }

    if (twig_sbm_create(engine, &default_sbm_config) < 0) {
        fprintf(stderr, "Failed to create stream buffer manager\n");
        goto err_cleanup;
    }
    
    printf("Video engine created successfully for %dx%d H264\n",
           video_info->width, video_info->height);
    
    return engine;
    
err_cleanup:
    twig_video_engine_destroy(engine);
    return NULL;
}

EXPORT void twig_video_engine_destroy(twig_video_engine_t *engine) {
    if (!engine)
        return;
        
    pthread_mutex_lock(&engine->mutex);

    twig_video_engine_shutdown_hardware(engine);
    
    if (engine->h264_ctx) {
        twig_h264_destroy(engine->h264_ctx);
        engine->h264_ctx = NULL;
    }
    
    if (engine->sbm) {
        twig_sbm_destroy(engine->sbm);
        engine->sbm = NULL;
    }
    
    if (engine->fbm) {
        twig_fbm_destroy(engine->fbm);
        engine->fbm = NULL;
    }
    
    // Free extra data copy
    if (engine->video_info.extra_data) {
        free(engine->video_info.extra_data);
        engine->video_info.extra_data = NULL;
    }
    
    pthread_mutex_unlock(&engine->mutex);
    pthread_mutex_destroy(&engine->mutex);
    
    free(engine);
    printf("Video engine destroyed\n");
}

EXPORT void twig_video_engine_reset(twig_video_engine_t *engine) {
    if (!engine)
        return;
        
    pthread_mutex_lock(&engine->mutex);

    if (engine->sbm) {
        engine->sbm->write_addr = (uint8_t*)engine->sbm->stream_buffer->virt;
        engine->sbm->valid_data_size = 0;
        engine->sbm->frame_fifo.valid_frame_num = 0;
        engine->sbm->frame_fifo.unread_frame_num = 0;
        engine->sbm->frame_fifo.read_pos = 0;
        engine->sbm->frame_fifo.write_pos = 0;
        engine->sbm->frame_fifo.flush_pos = 0;
    }

    if (engine->h264_ctx) {
        engine->h264_ctx->decode_step = H264_STEP_CONFIG_VBV;
        engine->h264_ctx->frame_decode_status = 0;
        engine->h264_ctx->cur_slice_num = 0;
        engine->h264_ctx->need_find_sps = 1;
        engine->h264_ctx->need_find_pps = 1;
        engine->h264_ctx->need_find_iframe = 1;
        engine->h264_ctx->decoded_frames = 0;
        engine->h264_ctx->cur_picture = NULL;
    }
    
    if (engine->fbm) {
        for (int i = 0; i < engine->fbm->max_frame_count; i++) {
            engine->fbm->frames[i].is_used = 0;
            engine->fbm->frames[i].is_reference = 0;
            engine->fbm->frames[i].ref_count = 0;
        }
        engine->fbm->empty_frame_count = engine->fbm->max_frame_count;
    }
    
    if (engine->h264_ctx && engine->h264_ctx->ve_active) {
        void *regs = engine->h264_ctx->ve_regs;
        twig_writel(0x00130007, regs + VE_RESET);
        usleep(1000);
        twig_writel(0x00130001 | (engine->video_info.width >= 2048 ? 0x00200000 : 0), regs + VE_CTRL);
    }
    
    pthread_mutex_unlock(&engine->mutex);
    
    printf("Video engine reset completed\n");
}

EXPORT int twig_video_engine_set_sbm(twig_video_engine_t *engine, twig_sbm_config_t *sbm_config) {
    if (!engine || !sbm_config)
        return -1;
        
    pthread_mutex_lock(&engine->mutex);

    if (engine->sbm) {
        twig_sbm_destroy(engine->sbm);
        engine->sbm = NULL;
    }

    int ret = twig_sbm_create(engine, sbm_config);
    
    pthread_mutex_unlock(&engine->mutex);
    
    return ret;
}

EXPORT twig_fbm_t *twig_video_engine_get_fbm(twig_video_engine_t *engine) {
    if (!engine)
        return NULL;

    if (!engine->fbm) {
        pthread_mutex_lock(&engine->mutex);
        
        if (!engine->fbm) {
            twig_fbm_config_t fbm_config = {
                .frame_count = engine->video_info.max_ref_frames + 8,
                .width = engine->video_info.width,
                .height = engine->video_info.height
            };
            
            if (twig_fbm_create(engine, &fbm_config) < 0) {
                fprintf(stderr, "Failed to create frame buffer manager\n");
                pthread_mutex_unlock(&engine->mutex);
                return NULL;
            }
        }
        
        pthread_mutex_unlock(&engine->mutex);
    }
    
    return engine->fbm;
}

EXPORT twig_result_t twig_video_engine_decode(twig_video_engine_t *engine,
                                                int end_of_stream, int decode_key_frames_only,
                                                int skip_b_frames_if_delay, int64_t current_time_us) {
    if (!engine) {
        fprintf(stderr, "Invalid engine\n");
        return TWIG_RESULT_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&engine->mutex);
    
    engine->h264_ctx->keyframes_only = decode_key_frames_only;

    twig_result_t result = twig_h264_decode(engine, end_of_stream, decode_key_frames_only,
                                            skip_b_frames_if_delay, current_time_us);
    
    pthread_mutex_unlock(&engine->mutex);
    
    return result;
}