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
    SLICE_TYPE_P,
    SLICE_TYPE_B,
    SLICE_TYPE_I,
    SLICE_TYPE_SP,
    SLICE_TYPE_SI
} twig_slice_type_t;

typedef enum {
    FRAME_STATE_FREE,
    FRAME_STATE_DECODER_HELD,
    FRAME_STATE_APP_HELD
} twig_frame_state_t;

typedef struct {
    twig_mem_t *buffer;
    twig_mem_t *extra_data;
    twig_frame_state_t state;
    int frame_idx;
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
        pool->frames[i].frame_idx = i;
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

void twig_build_ref_lists_from_pool(twig_frame_pool_t *pool, twig_slice_type_t slice_type, twig_frame_t **list0, int *l0_count,
                                    twig_frame_t **list1, int *l1_count, int current_poc) {    
    *l0_count = 0;
    *l1_count = 0;
    
    if (slice_type == SLICE_TYPE_I)
        return;

    twig_frame_t *short_refs[MAX_FRAME_POOL_SIZE];
    twig_frame_t *long_refs[MAX_FRAME_POOL_SIZE];
    int short_count = 0, long_count = 0;
    
    for (int i = 0; i < pool->allocated_count; i++) {
        twig_frame_t *frame = &pool->frames[i];
        
        if (frame->is_reference && frame->state == FRAME_STATE_DECODER_HELD) {
            if (frame->is_long_term)
                long_refs[long_count++] = frame;
            else
                short_refs[short_count++] = frame;
        }
    }

    if (slice_type == SLICE_TYPE_P || slice_type == SLICE_TYPE_SP) {
        for (int i = 0; i < short_count && *l0_count < 16; i++) {
            if (short_refs[i]->poc < current_poc)
                list0[(*l0_count)++] = short_refs[i];
        }
        for (int i = 0; i < *l0_count - 1; i++) {
            for (int j = i + 1; j < *l0_count; j++) {
                if (list0[i]->poc < list0[j]->poc) {
                    twig_frame_t *tmp = list0[i];
                    list0[i] = list0[j];
                    list0[j] = tmp;
                }
            }
        }

        int temp_count = *l0_count;
        for (int i = 0; i < short_count && *l0_count < 16; i++) {
            if (short_refs[i]->poc > current_poc)
                list0[(*l0_count)++] = short_refs[i];
        }
        for (int i = temp_count; i < *l0_count - 1; i++) {
            for (int j = i + 1; j < *l0_count; j++) {
                if (list0[i]->poc > list0[j]->poc) {
                    twig_frame_t *tmp = list0[i];
                    list0[i] = list0[j];
                    list0[j] = tmp;
                }
            }
        }
    } else if (slice_type == SLICE_TYPE_B) {
        for (int i = 0; i < short_count && *l0_count < 16; i++) {
            if (short_refs[i]->poc < current_poc)
                list0[(*l0_count)++] = short_refs[i];
        }
        for (int i = 0; i < *l0_count - 1; i++) {
            for (int j = i + 1; j < *l0_count; j++) {
                if (list0[i]->poc < list0[j]->poc) {
                    twig_frame_t *tmp = list0[i];
                    list0[i] = list0[j];
                    list0[j] = tmp;
                }
            }
        }
        
        int temp_count = *l0_count;
        for (int i = 0; i < short_count && *l0_count < 16; i++) {
            if (short_refs[i]->poc > current_poc)
                list0[(*l0_count)++] = short_refs[i];
        }
        for (int i = temp_count; i < *l0_count - 1; i++) {
            for (int j = i + 1; j < *l0_count; j++) {
                if (list0[i]->poc > list0[j]->poc) {
                    twig_frame_t *tmp = list0[i];
                    list0[i] = list0[j];
                    list0[j] = tmp;
                }
            }
        }

        for (int i = 0; i < short_count && *l1_count < 16; i++) {
            if (short_refs[i]->poc > current_poc)
                list1[(*l1_count)++] = short_refs[i];
        }
        for (int i = 0; i < *l1_count - 1; i++) {
            for (int j = i + 1; j < *l1_count; j++) {
                if (list1[i]->poc > list1[j]->poc) {
                    twig_frame_t *tmp = list1[i];
                    list1[i] = list1[j];
                    list1[j] = tmp;
                }
            }
        }
        
        temp_count = *l1_count;
        for (int i = 0; i < short_count && *l1_count < 16; i++) {
            if (short_refs[i]->poc < current_poc)
                list1[(*l1_count)++] = short_refs[i];
        }
        for (int i = temp_count; i < *l1_count - 1; i++) {
            for (int j = i + 1; j < *l1_count; j++) {
                if (list1[i]->poc < list1[j]->poc) {
                    twig_frame_t *tmp = list1[i];
                    list1[i] = list1[j];
                    list1[j] = tmp;
                }
            }
        }
    }

    for (int i = 0; i < long_count && *l0_count < 16; i++) {
        list0[(*l0_count)++] = long_refs[i];
    }
    if (slice_type == SLICE_TYPE_B) {
        for (int i = 0; i < long_count && *l1_count < 16; i++) {
            list1[(*l1_count)++] = long_refs[i];
        }
    }
}

void twig_write_framebuffer_list(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t *output_frame, int output_poc) {
    if (!cedar || !ve_regs || !pool || !output_frame)
        return;
        
    uintptr_t ve_base = (uintptr_t)ve_regs;
    uintptr_t h264_base = ve_base + H264_OFFSET;

    twig_writel(h264_base, H264_RAM_WRITE_PTR, VE_SRAM_H264_FRAMEBUFFER_LIST);
    
    int output_placed = 0;

    for (int i = 0; i < 18; i++) {
        twig_frame_t *frame = NULL;
        int is_output = 0;
        if (i < pool->allocated_count) {
            frame = &pool->frames[i];
            if (frame->state == FRAME_STATE_FREE && !output_placed) {
                frame = output_frame;
                is_output = 1;
                output_placed = 1;
            } else {
                frame = NULL;
            }
        }
        
        if (!frame) {
            for (int j = 0; j < 8; j++) {
                twig_writel(h264_base, H264_RAM_WRITE_DATA, 0);
            }
        } else {
            uint32_t luma_addr = frame->buffer->iommu_addr;
            uint32_t luma_size = pool->frame_width * pool->frame_height;
            uint32_t chroma_addr = luma_addr + luma_size;
            uint32_t extra_addr = frame->extra_data->iommu_addr;
            uint32_t extra_size = frame->extra_data->size;
            int frame_poc = is_output ? output_poc : frame->poc;

            twig_writel(h264_base, H264_RAM_WRITE_DATA, (uint16_t)frame_poc);
            twig_writel(h264_base, H264_RAM_WRITE_DATA, (uint16_t)frame_poc);
            twig_writel(h264_base, H264_RAM_WRITE_DATA, 0 << 8);
            twig_writel(h264_base, H264_RAM_WRITE_DATA, luma_addr);
            twig_writel(h264_base, H264_RAM_WRITE_DATA, chroma_addr); 
            twig_writel(h264_base, H264_RAM_WRITE_DATA, extra_addr);
            twig_writel(h264_base, H264_RAM_WRITE_DATA, extra_addr + extra_size);
            twig_writel(h264_base, H264_RAM_WRITE_DATA, 0);
        }
    }
    twig_writel(h264_base, H264_OUTPUT_FRAME_IDX, output_frame->frame_idx);
}


void twig_write_ref_list0_registers(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t **ref_list0, int l0_count) {
    if (!cedar || !ve_regs || !pool || !ref_list0)
        return;
        
    uintptr_t ve_base = (uintptr_t)ve_regs;
    uintptr_t h264_base = ve_base + H264_OFFSET;

    twig_writel(h264_base, H264_RAM_WRITE_PTR, VE_SRAM_H264_REF_LIST0);

    for (int i = 0; i < l0_count; i += 4) {
        uint32_t list_word = 0;
        for (int j = 0; j < 4; j++) {
            if (i + j < l0_count && ref_list0[i + j]) {
                uint32_t packed_idx = ref_list0[i + j]->frame_idx * 2;
                list_word |= (packed_idx << (j * 8));
            }
        }
        twig_writel(h264_base, H264_RAM_WRITE_DATA, list_word);
    }
}

void twig_write_ref_list1_registers(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool, twig_frame_t **ref_list1, int l1_count) {
    if (!cedar || !ve_regs || !pool || !ref_list1)
        return;
        
    uintptr_t ve_base = (uintptr_t)ve_regs;
    uintptr_t h264_base = ve_base + H264_OFFSET;
 
    twig_writel(h264_base, H264_RAM_WRITE_PTR, VE_SRAM_H264_REF_LIST1);

    for (int i = 0; i < l1_count; i += 4) {
        uint32_t list_word = 0;
        for (int j = 0; j < 4; j++) {
            if (i + j < l1_count && ref_list1[i + j]) {
                uint32_t packed_idx = ref_list1[i + j]->frame_idx * 2;
                list_word |= (packed_idx << (j * 8));
            }
        }
        twig_writel(h264_base, H264_RAM_WRITE_DATA, list_word);
    }
}