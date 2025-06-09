#include "priv/twig_video_priv.h"

static int sbm_lock(twig_sbm_t *sbm) {
    return pthread_mutex_lock(&sbm->mutex);
}

static void sbm_unlock(twig_sbm_t *sbm) {
    pthread_mutex_unlock(&sbm->mutex);
}

int twig_sbm_create(twig_video_engine_t *engine, twig_sbm_config_t *config) {
    if (!engine || !config || config->buffer_size == 0) {
        fprintf(stderr, "Invalid SBM parameters\n");
        return -1;
    }
    
    twig_sbm_t *sbm = calloc(1, sizeof(*sbm));
    if (!sbm) {
        fprintf(stderr, "Failed to allocate SBM\n");
        return -1;
    }

    if (pthread_mutex_init(&sbm->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize SBM mutex\n");
        free(sbm);
        return -1;
    }

    sbm->stream_buffer = twig_alloc_mem(engine->device, config->buffer_size);
    if (!sbm->stream_buffer) {
        fprintf(stderr, "Failed to allocate %zu byte stream buffer\n", config->buffer_size);
        pthread_mutex_destroy(&sbm->mutex);
        free(sbm);
        return -1;
    }

    sbm->buffer_size = config->buffer_size;
    sbm->write_addr = (uint8_t*)sbm->stream_buffer->virt;
    sbm->buffer_end = (uint8_t*)sbm->stream_buffer->virt + config->buffer_size - 1;
    sbm->valid_data_size = 0;
    
    sbm->frame_fifo.max_frame_num = config->max_frame_num;
    sbm->frame_fifo.valid_frame_num = 0;
    sbm->frame_fifo.unread_frame_num = 0;
    sbm->frame_fifo.read_pos = 0;
    sbm->frame_fifo.write_pos = 0;
    sbm->frame_fifo.flush_pos = 0;
    
    memset(sbm->frame_fifo.frames, 0, sizeof(sbm->frame_fifo.frames));
    
    engine->sbm = sbm;
    
    printf("Stream buffer manager created: %zu bytes\n", config->buffer_size);
    return 0;
}

void twig_sbm_destroy(twig_sbm_t *sbm) {
    if (!sbm)
        return;
        
    if (sbm->stream_buffer)
        sbm->stream_buffer= NULL;
    
    pthread_mutex_destroy(&sbm->mutex);
    free(sbm);
}

EXPORT int twig_sbm_request_buffer(twig_video_engine_t *engine, int require_size,
                                uint8_t **buf, int *buf_size) {
    if (!engine || !engine->sbm || !buf || !buf_size) {
        fprintf(stderr, "Invalid SBM request parameters\n");
        return -1;
    }
    
    twig_sbm_t *sbm = engine->sbm;
    
    if (sbm_lock(sbm) != 0) {
        fprintf(stderr, "Failed to lock SBM\n");
        return -1;
    }

    if (sbm->frame_fifo.valid_frame_num >= sbm->frame_fifo.max_frame_num) {
        fprintf(stderr, "Frame FIFO is full\n");
        sbm_unlock(sbm);
        return -1;
    }

    size_t free_size = sbm->buffer_size - sbm->valid_data_size;
    if (require_size > (int)free_size) {
        fprintf(stderr, "Not enough space: need %d, have %zu\n", require_size, free_size);
        sbm_unlock(sbm);
        return -1;
    }

    *buf = sbm->write_addr;
    *buf_size = require_size;
    
    sbm_unlock(sbm);
    return 0;
}

EXPORT int twig_sbm_add_stream_frame(twig_video_engine_t *engine, twig_stream_frame_t *frame_info) {
    if (!engine || !engine->sbm || !frame_info) {
        fprintf(stderr, "Invalid SBM add frame parameters\n");
        return -1;
    }
    
    twig_sbm_t *sbm = engine->sbm;
    
    if (sbm_lock(sbm) != 0) {
        fprintf(stderr, "Failed to lock SBM\n");
        return -1;
    }

    if (sbm->frame_fifo.valid_frame_num >= sbm->frame_fifo.max_frame_num) {
        fprintf(stderr, "Frame FIFO full\n");
        sbm_unlock(sbm);
        return -1;
    }

    int write_pos = sbm->frame_fifo.write_pos;
    sbm->frame_fifo.frames[write_pos] = *frame_info;
    sbm->frame_fifo.write_pos = (write_pos + 1) % SBM_FRAME_FIFO_SIZE;
    sbm->frame_fifo.valid_frame_num++;
    sbm->frame_fifo.unread_frame_num++;

    sbm->write_addr += frame_info->size;
    if (sbm->write_addr > sbm->buffer_end)
        sbm->write_addr -= sbm->buffer_size;

    sbm->valid_data_size += frame_info->size;
    
    sbm_unlock(sbm);
    return 0;
}

EXPORT int twig_sbm_stream_frame_num(twig_video_engine_t *engine) {
    if (!engine || !engine->sbm)
        return 0;
        
    return engine->sbm->frame_fifo.valid_frame_num;
}

EXPORT int twig_sbm_stream_data_size(twig_video_engine_t *engine) {
    if (!engine || !engine->sbm)
        return 0;
        
    return (int)engine->sbm->valid_data_size;
}