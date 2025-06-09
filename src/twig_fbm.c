#include "twig_video_priv.h"

static int fbm_lock(twig_fbm_t *fbm) {
    return pthread_mutex_lock(&fbm->mutex);
}

static void fbm_unlock(twig_fbm_t *fbm) {
    pthread_mutex_unlock(&fbm->mutex);
}

static int calculate_buffer_sizes(int width, int height, int *y_size, int *c_size, 
                                    int *y_stride, int *c_stride) {
    int aligned_width = (width + 31) & ~31;
    int aligned_height = (height + 31) & ~31;
    
    *y_stride = aligned_width;
    *c_stride = aligned_width;
    
    *y_size = aligned_width * aligned_height;
    *c_size = aligned_width * aligned_height / 2;
    
    return 0;
}

int twig_fbm_create(twig_video_engine_t *engine, twig_fbm_config_t *config) {
    if (!engine || !config || config->frame_count <= 0) {
        fprintf(stderr, "Invalid FBM parameters\n");
        return -1;
    }
    
    twig_fbm_t *fbm = calloc(1, sizeof(*fbm));
    if (!fbm) {
        fprintf(stderr, "Failed to allocate FBM\n");
        return -1;
    }

    if (pthread_mutex_init(&fbm->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize FBM mutex\n");
        free(fbm);
        return -1;
    }

    calculate_buffer_sizes(config->width, config->height, &fbm->y_buffer_size, &fbm->c_buffer_size,
                            &fbm->y_stride, &fbm->c_stride);
    
    fbm->max_frame_count = config->frame_count;
    fbm->empty_frame_count = config->frame_count;
    
    for (int i = 0; i < config->frame_count; i++) {
        twig_frame_info_t *frame = &fbm->frames[i];

        frame->picture.y_buf = twig_alloc_mem(engine->device, fbm->y_buffer_size);
        if (!frame->picture.y_buf) {
            fprintf(stderr, "Failed to allocate Y buffer %d\n", i);
            goto err_cleanup;
        }

        frame->picture.c_buf = twig_alloc_mem(engine->device, fbm->c_buffer_size);
        if (!frame->picture.c_buf) {
            fprintf(stderr, "Failed to allocate UV buffer %d\n", i);
            twig_free_mem(engine->device, frame->picture.y_buf);
            goto err_cleanup;
        }
        
        frame->picture.width = config->width;
        frame->picture.height = config->height;
        frame->picture.y_stride = fbm->y_stride;
        frame->picture.c_stride = fbm->c_stride;
        frame->picture.pts = -1;
        frame->picture.is_key_frame = 0;
        frame->picture.poc = 0;
        frame->picture.top_offset = 0;
        frame->picture.bottom_offset = 0;
        frame->picture.left_offset = 0;
        frame->picture.right_offset = 0;
        
        frame->is_used = 0;
        frame->is_reference = 0;
        frame->poc = 0;
        frame->frame_num = 0;
        frame->ref_count = 0;
    }
    
    engine->fbm = fbm;
    
    printf("Frame buffer manager created: %d frames, %dx%d\n",
           config->frame_count, config->width, config->height);
    return 0;
    
err_cleanup:
    for (int j = 0; j < i; j++) {
        if (fbm->frames[j].picture.y_buf) {
            twig_free_mem(engine->device, fbm->frames[j].picture.y_buf);
        }
        if (fbm->frames[j].picture.c_buf) {
            twig_free_mem(engine->device, fbm->frames[j].picture.c_buf);
        }
    }
    
    pthread_mutex_destroy(&fbm->mutex);
    free(fbm);
    return -1;
}

void twig_fbm_destroy(twig_fbm_t *fbm) {
    if (!fbm)
        return;

    pthread_mutex_destroy(&fbm->mutex);
    free(fbm);
}

EXPORT twig_picture_t *twig_fbm_request_picture(twig_video_engine_t *engine, int timeout_ms) {
    if (!engine || !engine->fbm) {
        fprintf(stderr, "Invalid FBM request parameters\n");
        return NULL;
    }
    
    twig_fbm_t *fbm = engine->fbm;
    
    if (fbm_lock(fbm) != 0) {
        fprintf(stderr, "Failed to lock FBM\n");
        return NULL;
    }
    
    for (int i = 0; i < fbm->max_frame_count; i++) {
        twig_frame_info_t *frame = &fbm->frames[i];
        if (frame->is_used && frame->ref_count == 0 && !frame->is_reference) {
            frame->ref_count = 1;
            fbm_unlock(fbm);
            return &frame->picture;
        }
    }
    
    fbm_unlock(fbm);
    return NULL;
}

EXPORT int twig_fbm_return_picture(twig_video_engine_t *engine, twig_picture_t *picture) {
    if (!engine || !engine->fbm || !picture) {
        fprintf(stderr, "Invalid FBM return parameters\n");
        return -1;
    }
    
    twig_fbm_t *fbm = engine->fbm;
    
    if (fbm_lock(fbm) != 0) {
        fprintf(stderr, "Failed to lock FBM\n");
        return -1;
    }

    for (int i = 0; i < fbm->max_frame_count; i++) {
        if (&fbm->frames[i].picture == picture) {
            if (fbm->frames[i].ref_count > 0) {
                fbm->frames[i].ref_count--;

                if (fbm->frames[i].ref_count == 0 && !fbm->frames[i].is_reference) {
                    fbm->frames[i].is_used = 0;
                    fbm->empty_frame_count++;
                }
            }
            fbm_unlock(fbm);
            return 0;
        }
    }
    
    fbm_unlock(fbm);
    fprintf(stderr, "Picture not found in FBM\n");
    return -1;
}

EXPORT int twig_fbm_num_total_frames(twig_video_engine_t *engine) {
    if (!engine || !engine->fbm)
        return 0;
        
    return engine->fbm->max_frame_count;
}

EXPORT int twig_fbm_num_empty_frames(twig_video_engine_t *engine) {
    if (!engine || !engine->fbm)
        return 0;
        
    return engine->fbm->empty_frame_count;
}