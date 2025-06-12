#include "twig.h"
#include "twig_dec.h"
#include "twig_regs.h"

#define FRAME_TYPE_PROGRESSIVE       0
#define FRAME_TYPE_INTERLACED_FRAME  1  
#define FRAME_TYPE_INTERLACED_FIELD  2
#define FIELD_TOP_REF                0x1
#define FIELD_BOTTOM_REF             0x2
#define FIELD_BOTH_REF               0x3

#define MAX_FRAME_POOL_SIZE          20

typedef enum {
    FRAME_STATE_FREE,
    FRAME_STATE_DECODER_HELD,
    FRAME_STATE_APP_HELD
} twig_frame_state_t;

typedef struct {
    twig_mem_t *buffer;
    twig_mem_t *extra_data;
    twig_frame_state_t state;
    int frame_num;
    int poc;
    int is_reference;
    int is_long_term;
} twig_frame_t;

typedef struct {
    twig_frame_t frames[MAX_FRAME_POOL_SIZE];
    int allocated_count;
    int frame_width;
    int frame_height;
    size_t frame_size;
} twig_frame_pool_t;

int twig_frame_pool_init(twig_frame_pool_t *pool, int width, int height) {
    if (!pool || width <= 0 || height <= 0)
        return -1;

    pool->frame_width = width;
    pool->frame_height = height;
    pool->frame_size = width * height * 3 / 2;
    pool->allocated_count = 0;

    for (int i = 0; i < MAX_FRAME_POOL_SIZE; i++) {
        pool->frames[i].buffer = NULL;
        pool->frames[i].state = FRAME_STATE_FREE;
        pool->frames[i].frame_num = -1;
        pool->frames[i].poc = 0;
        pool->frames[i].is_reference = 0;
        pool->frames[i].is_long_term = 0;
    }

    return 0;
}

twig_frame_t *twig_frame_pool_get(twig_frame_pool_t *pool, twig_dev_t *cedar, uint16_t pwimm1) {
    if (!pool || !cedar)
        return NULL;

    for (int i = 0; i < pool->allocated_count; i++) {
        if (pool->frames[i].state == FRAME_STATE_FREE) {
            pool->frames[i].state = FRAME_STATE_DECODER_HELD;
            return &pool->frames[i];
        }
    }

    if (pool->allocated_count < MAX_FRAME_POOL_SIZE) {
        twig_frame_t *frame = &pool->frames[pool->allocated_count];
        
        frame->buffer = twig_alloc_mem(cedar, pool->frame_size);
        if (!frame->buffer) {
            fprintf(stderr, "Failed to allocate buffer for frame %d\n", pool->allocated_count);
            return NULL;
        }
        int extra_buf_size = 327680;
        if (pool->frame_width >= 2048) {
            extra_buf_size += (pwimm1 + 32) * 192;
            extra_buf_size = (extra_buf_size + 4095) & ~4095;
            extra_buf_size += (pwimm1 + 64) * 80;
        }
        frame->extra_data = twig_alloc_mem(cedar, extra_buf_size);
        if (!frame->extra_data) {
            fprintf(stderr, "Failed to allocate extra data for frame %d\n", pool->allocated_count);
            twig_free_mem(cedar, frame->buffer);
            return NULL;
        }

        frame->state = FRAME_STATE_DECODER_HELD;
        frame->frame_num = -1;
        frame->poc = 0;
        frame->is_reference = 0;
        frame->is_long_term = 0;

        pool->allocated_count++;
        return frame;
    }

    fprintf(stderr, "WARNING: Frame pool at maximum size (%d frames)\n", MAX_FRAME_POOL_SIZE);
    fprintf(stderr, "Looking for non-reference frames to recycle...\n");    
    for (int i = 0; i < pool->allocated_count; i++) {
        if (!pool->frames[i].is_reference && pool->frames[i].state == FRAME_STATE_DECODER_HELD) {
            fprintf(stderr, "Force recycling non-reference frame slot %d\n", i);
            pool->frames[i].state = FRAME_STATE_DECODER_HELD;
            return &pool->frames[i];
        }
    }

    fprintf(stderr, "ERROR: No frames available for allocation!\n");
    fprintf(stderr, "This may indicate a reference management bug or app not returning frames\n");
    return NULL;
}

twig_frame_t twig_send_frame(twig_frame_t *frame, int frame_num, int poc, int is_reference) {
    if (!frame)
        return NULL;
        
    frame->frame_num = frame_num;
    frame->poc = poc;
    frame->is_reference = is_reference;
    frame->state = FRAME_STATE_APP_HELD;
    return frame;
}

void twig_mark_frame_return(twig_frame_pool_t *pool, twig_mem_t *buffer, twig_dev_t *cedar) {
    if (!pool || !buffer)
        return;

    for (int i = 0; i < pool->allocated_count; i++) {
        if (pool->frames[i].buffer == buffer) {
            twig_frame_t *frame = &pool->frames[i];
            
            if (frame->state != FRAME_STATE_APP_HELD) {
                fprintf(stderr, "WARNING: App returned frame not in APP_HELD state\n");
            }
            
            if (frame->is_reference) {
                frame->state = FRAME_STATE_DECODER_HELD;
            } else {
                frame->state = FRAME_STATE_FREE;
                frame->frame_num = -1;
            }
            return;
        }
    }
    fprintf(stderr, "NOTICE: Returned frame not in current pool, freeing its buffer!\n");
    twig_free_mem(cedar, buffer);
}

void twig_mark_frame_unref(twig_frame_t *frame) {
    if (!frame)
        return;

    frame->is_reference = 0;
    frame->is_long_term = 0;

    if (frame->state == FRAME_STATE_DECODER_HELD) {
        frame->state = FRAME_STATE_FREE;
        frame->frame_num = -1;
    }
}

void twig_frame_pool_cleanup(twig_frame_pool_t *pool, twig_dev_t *cedar) {
    if (!pool || !cedar)
        return;
        
    for (int i = 0; i < pool->allocated_count; i++) {
        if (pool->frames[i].buffer) {
            twig_free_mem(cedar, pool->frames[i].buffer);
            pool->frames[i].buffer = NULL;
        }
    }
    
    pool->allocated_count = 0;
}