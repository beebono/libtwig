#include "twig.h"
#include "twig_dec.h"
#include "twig_regs.h"

#define FRAME_TYPE_PROGRESSIVE       0
#define FRAME_TYPE_INTERLACED_FRAME  1  
#define FRAME_TYPE_INTERLACED_FIELD  2
#define FIELD_TOP_REF                0x1
#define FIELD_BOTTOM_REF             0x2
#define FIELD_BOTH_REF               0x3

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

    pool->short_count = 0;
    pool->long_count = 0;
    pool->max_long_term_frame_idx = -1;
    pool->prev_frame_num = -1;
    pool->max_frame_num = 0;

    memset(pool->short_refs, 0, sizeof(pool->short_refs));
    memset(pool->long_refs, 0, sizeof(pool->long_refs));

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
        if (!frame->buffer)
            return NULL;

        int extra_buf_size = 327680;
        if (pool->frame_width >= 2048) {
            extra_buf_size += (pwimm1 + 32) * 192;
            extra_buf_size = (extra_buf_size + 4095) & ~4095;
            extra_buf_size += (pwimm1 + 64) * 80;
        }
        frame->extra_data = twig_alloc_mem(cedar, extra_buf_size);
        if (!frame->extra_data) {
            twig_free_mem(cedar, frame->buffer);
            return NULL;
        }

        frame->state = FRAME_STATE_DECODER_HELD;
        frame->frame_num = -1;
        frame->poc = 0;
        frame->is_reference = 0;
        frame->is_long_term = 0;
        frame->long_term_idx = -1;

        pool->allocated_count++;
        return frame;
    }

    for (int i = 0; i < pool->allocated_count; i++) {
        if (!pool->frames[i].is_reference && pool->frames[i].state == FRAME_STATE_DECODER_HELD) {
            pool->frames[i].state = FRAME_STATE_DECODER_HELD;
            return &pool->frames[i];
        }
    }
    return NULL;
}

int twig_parse_mmco_commands(void *regs, twig_mmco_cmd_t *mmco_list, int *mmco_count) {
    *mmco_count = 0;
    int adaptive_mode = twig_get_1bit_hw(regs);
    if (!adaptive_mode)
        return 0;

    do {
        twig_mmco_cmd_t *cmd = &mmco_list[*mmco_count];
        cmd->memory_management_control_operation = twig_get_ue_golomb_hw(regs);
        switch (cmd->memory_management_control_operation) {
            case 0:
                return 0;
            case 1:
                cmd->difference_of_pic_nums_minus1 = twig_get_ue_golomb_hw(regs);
                break;
            case 2: 
                cmd->long_term_pic_num = twig_get_ue_golomb_hw(regs);
                break;
            case 3:
                cmd->difference_of_pic_nums_minus1 = twig_get_ue_golomb_hw(regs);
                cmd->long_term_frame_idx = twig_get_ue_golomb_hw(regs);
                break;
            case 4:
                cmd->max_long_term_frame_idx_plus1 = twig_get_ue_golomb_hw(regs);
                break;
            case 5:
                break;
            case 6:
                cmd->long_term_frame_idx = twig_get_ue_golomb_hw(regs);
                break;
            default:
                return -1;
        }
        (*mmco_count)++;
    } while (*mmco_count < 32);

    return 0;
}

static void twig_mmco_short_to_unused(twig_frame_pool_t *pool, int difference_of_pic_nums_minus1, int current_frame_num) {
    int target_frame_num = current_frame_num - (difference_of_pic_nums_minus1 + 1);
    for (int i = 0; i < pool->short_count; i++) {
        if (pool->short_refs[i]->frame_num == target_frame_num) {
            twig_mark_frame_unref(pool, pool->short_refs[i]);
            for (int j = i; j < pool->short_count - 1; j++) {
                pool->short_refs[j] = pool->short_refs[j + 1];
            }
        }
    }
}

static void twig_mmco_long_to_unused(twig_frame_pool_t *pool, int long_term_pic_num) {
    for (int i = 0; i < pool->long_count; i++) {
        if (pool->long_refs[i]->long_term_idx == long_term_pic_num) {
            twig_mark_frame_unref(pool, pool->long_refs[i]);
            for (int j = i; j < pool->long_count - 1; j++) {
                pool->long_refs[j] = pool->long_refs[j + 1];
            }
        }
    }
}

static void twig_mmco_short_to_long(twig_frame_pool_t *pool, int difference_of_pic_nums_minus1, int long_term_frame_idx, int current_frame_num) {
    int target_frame_num = current_frame_num - (difference_of_pic_nums_minus1 + 1);
    for (int i = 0; i < pool->short_count; i++) {
        if (pool->short_refs[i]->frame_num == target_frame_num) {
            twig_frame_t *frame = pool->short_refs[i];
            for (int j = i; j < pool->short_count - 1; j++) {
                pool->short_refs[j] = pool->short_refs[j + 1];
            }
            pool->short_count--;
            twig_mmco_long_to_unused(pool, long_term_frame_idx);
            frame->is_long_term = 1;
            frame->long_term_idx = long_term_frame_idx;
            pool->long_refs[pool->long_count++] = frame;
        }
    }
}

static void twig_mmco_set_max_long_term(twig_frame_pool_t *pool, int max_long_term_frame_idx_plus1) {
    pool->max_long_term_frame_idx = max_long_term_frame_idx_plus1 - 1;
    for (int i = pool->long_count - 1; i >= 0; i--) {
        if (pool->long_refs[i]->long_term_idx > pool->max_long_term_frame_idx) {
            twig_mark_frame_unref(pool, pool->long_refs[i]);
            for (int j = i; j < pool->long_count - 1; j++) {
                pool->long_refs[j] = pool->long_refs[j + 1];
            }
        }
    }
}

static void twig_mmco_reset_all(twig_frame_pool_t *pool) {
    for (int i = 0; i < pool->short_count; i++) {
        twig_mark_frame_unref(pool, pool->short_refs[i]);
    }
    for (int i = 0; i < pool->long_count; i++) {
        twig_mark_frame_unref(pool, pool->long_refs[i]);
    }
    pool->short_count = 0;
    pool->long_count = 0;
    pool->max_long_term_frame_idx = -1;
}

static void twig_mmco_current_to_long(twig_frame_pool_t *pool, twig_frame_t *current_frame, int long_term_frame_idx) {
    if (pool->max_long_term_frame_idx >= 0 && long_term_frame_idx > pool->max_long_term_frame_idx)
        return;

    twig_mmco_long_to_unused(pool, long_term_frame_idx);
    current_frame->is_long_term = 1;
    current_frame->long_term_idx = long_term_frame_idx;
    if (pool->long_count < 16)
        pool->long_refs[pool->long_count++] = current_frame;
}

void twig_execute_mmco_commands(twig_h264_decoder_t *decoder, twig_frame_t *current_frame) {
    twig_frame_pool_t *pool = &decoder->frame_pool;
    for (int i = 0; i < decoder->mmco_count; i++) {
        twig_mmco_cmd_t *cmd = &decoder->mmco_commands[i];
        switch (cmd->memory_management_control_operation) {
            case 1:
                twig_mmco_short_to_unused(pool, cmd->difference_of_pic_nums_minus1, current_frame->frame_num);
                break;
            case 2:
                twig_mmco_long_to_unused(pool, cmd->long_term_pic_num);
                break;
            case 3:
                twig_mmco_short_to_long(pool, cmd->difference_of_pic_nums_minus1, cmd->long_term_frame_idx, current_frame->frame_num);
                break;
            case 4:
                twig_mmco_set_max_long_term(pool, cmd->max_long_term_frame_idx_plus1);
                break;
            case 5:
                twig_mmco_reset_all(pool);
                break;
            case 6:
                twig_mmco_current_to_long(pool, current_frame, cmd->long_term_frame_idx);
                break;
            default:
                break;
        }
    }
}

void twig_add_short_term_ref(twig_frame_pool_t *pool, twig_frame_t *frame) {
    int insert_pos = 0;
    while (insert_pos < pool->short_count && pool->short_refs[insert_pos]->frame_num > frame->frame_num) {
        insert_pos++;
    }

    for (int i = pool->short_count; i > insert_pos; i--) {
        pool->short_refs[i] = pool->short_refs[i-1];
    }

    pool->short_refs[insert_pos] = frame;
    pool->short_count++;
    frame->is_reference = 1;
    frame->is_long_term = 0;

    int max_refs = 16;
    if (pool->short_count > max_refs) {
        twig_frame_t *oldest = pool->short_refs[pool->short_count - 1];
        twig_mark_frame_unref(pool, oldest);
    }
}

void twig_remove_short_term_ref(twig_frame_pool_t *pool, twig_frame_t *frame) {
    for (int i = 0; i < pool->short_count; i++) {
        if (pool->short_refs[i] == frame) {
            for (int j = i; j < pool->short_count - 1; j++) {
                pool->short_refs[j] = pool->short_refs[j + 1];
            }
            pool->short_count--;
            return;
        }
    }
}

void twig_add_long_term_ref(twig_frame_pool_t *pool, twig_frame_t *frame) {
    for (int i = 0; i < pool->long_count; i++) {
        if (pool->long_refs[i]->long_term_idx == frame->long_term_idx) {
            twig_mark_frame_unref(pool, pool->long_refs[i]);
            break;
        }
    }

    if (pool->long_count < 16) {
        pool->long_refs[pool->long_count++] = frame;
        frame->is_reference = 1;
        frame->is_long_term = 1;
    }
}

void twig_remove_long_term_ref(twig_frame_pool_t *pool, twig_frame_t *frame) {
    for (int i = 0; i < pool->long_count; i++) {
        if (pool->long_refs[i] == frame) {
            for (int j = i; j < pool->long_count - 1; j++) {
                pool->long_refs[j] = pool->long_refs[j + 1];
            }
            pool->long_count--;
            return;
        }
    }
}

void twig_mark_frame_unref(twig_frame_pool_t *pool, twig_frame_t *frame) {
    if (!frame || !frame->is_reference)
        return;

    if (frame->is_long_term)
        twig_remove_long_term_ref(pool, frame);
    else
        twig_remove_short_term_ref(pool, frame);

    frame->is_reference = 0;
    frame->is_long_term = 0;
    frame->long_term_idx = -1;
}

void twig_remove_stale_frames(twig_frame_pool_t *pool) {
    for (int i = 0; i < 16; i++) {
        if (pool->frames[i].state == FRAME_STATE_DECODER_HELD && pool->frames[i].is_reference == 0)
            pool->frames[i].state = FRAME_STATE_FREE;
    }
}

void twig_frame_pool_cleanup(twig_frame_pool_t *pool, twig_dev_t *cedar) {
    if (!pool || !cedar)
        return;

    for (int i = 0; i < pool->allocated_count; i++) {
        if (pool->frames[i].is_reference) {
            twig_mark_frame_unref(pool, &pool->frames[i]);
        }
    }

    for (int i = 0; i < pool->allocated_count; i++) {
        if (pool->frames[i].buffer) {
            twig_free_mem(cedar, pool->frames[i].buffer);
            pool->frames[i].buffer = NULL;
        }
        if (pool->frames[i].extra_data) {
            twig_free_mem(cedar, pool->frames[i].extra_data);
            pool->frames[i].extra_data = NULL;
        }
    }

    pool->allocated_count = 0;
}

static void twig_build_list0(twig_frame_pool_t *pool, twig_frame_t **list0, int *l0_count, int current_poc) {
    twig_frame_t *before_refs[16];
    int before_count = 0;
    for (int i = 0; i < pool->short_count; i++) {
        if (pool->short_refs[i]->poc < current_poc)
            before_refs[before_count++] = pool->short_refs[i];
    }

    for (int i = 0; i < before_count - 1; i++) {
        for (int j = i + 1; j < before_count; j++) {
            if (before_refs[i]->poc < before_refs[j]->poc) {
                twig_frame_t *temp = before_refs[i];
                before_refs[i] = before_refs[j];
                before_refs[j] = temp;
            }
        }
    }

    for (int i = 0; i < before_count && *l0_count < 16; i++) {
        list0[(*l0_count)++] = before_refs[i];
    }

    twig_frame_t *after_refs[16];
    int after_count = 0;
    for (int i = 0; i < pool->short_count; i++) {
        if (pool->short_refs[i]->poc > current_poc)
            after_refs[after_count++] = pool->short_refs[i];
    }

    for (int i = 0; i < after_count - 1; i++) {
        for (int j = i + 1; j < after_count; j++) {
            if (after_refs[i]->poc > after_refs[j]->poc) {
                twig_frame_t *temp = after_refs[i];
                after_refs[i] = after_refs[j];
                after_refs[j] = temp;
            }
        }
    }

    for (int i = 0; i < after_count && *l0_count < 16; i++) {
        list0[(*l0_count)++] = after_refs[i];
    }

    for (int i = 0; i < pool->long_count && *l0_count < 16; i++) {
        list0[(*l0_count)++] = pool->long_refs[i];
    }
}

static void twig_build_list1(twig_frame_pool_t *pool, twig_frame_t **list1, int *l1_count, int current_poc) {
    twig_frame_t *after_refs[16];
    int after_count = 0;
    for (int i = 0; i < pool->short_count; i++) {
        if (pool->short_refs[i]->poc > current_poc) {
            after_refs[after_count++] = pool->short_refs[i];
        }
    }

    for (int i = 0; i < after_count - 1; i++) {
        for (int j = i + 1; j < after_count; j++) {
            if (after_refs[i]->poc > after_refs[j]->poc) {
                twig_frame_t *temp = after_refs[i];
                after_refs[i] = after_refs[j];
                after_refs[j] = temp;
            }
        }
    }

    for (int i = 0; i < after_count && *l1_count < 16; i++) {
        list1[(*l1_count)++] = after_refs[i];
    }

    twig_frame_t *before_refs[16];
    int before_count = 0;
    for (int i = 0; i < pool->short_count; i++) {
        if (pool->short_refs[i]->poc < current_poc)
            before_refs[before_count++] = pool->short_refs[i];
    }

    for (int i = 0; i < before_count - 1; i++) {
        for (int j = i + 1; j < before_count; j++) {
            if (before_refs[i]->poc < before_refs[j]->poc) {
                twig_frame_t *temp = before_refs[i];
                before_refs[i] = before_refs[j];
                before_refs[j] = temp;
            }
        }
    }

    for (int i = 0; i < before_count && *l1_count < 16; i++) {
        list1[(*l1_count)++] = before_refs[i];
    }

    for (int i = 0; i < pool->long_count && *l1_count < 16; i++) {
        list1[(*l1_count)++] = pool->long_refs[i];
    }
}

void twig_build_ref_lists(twig_frame_pool_t *pool, twig_slice_type_t slice_type,
                            twig_frame_t **list0, int *l0_count,
                            twig_frame_t **list1, int *l1_count,
                            int current_poc) {    
    *l0_count = 0;
    *l1_count = 0;

    if (slice_type == SLICE_TYPE_I)
        return;

    twig_build_list0(pool, list0, l0_count, current_poc);
    if (slice_type == SLICE_TYPE_B) {
        twig_build_list1(pool, list1, l1_count, current_poc);
        if (*l1_count > 1 && *l0_count == *l1_count) {
            int identical = 1;
            for (int i = 0; i < *l0_count; i++) {
                if (list0[i] != list1[i]) {
                    identical = 0;
                    break;
                }
            }
            if (identical) {
                twig_frame_t *temp = list1[0];
                list1[0] = list1[1];
                list1[1] = temp;
            }
        }
    }
}

void twig_write_framebuffer_list(twig_dev_t *cedar, void *ve_regs, twig_frame_pool_t *pool,
                                    twig_frame_t *output_frame, int output_poc) {
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
            } else if (frame->state == FRAME_STATE_DECODER_HELD && frame->is_reference) {
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
    twig_writel(h264_base, H264_OUTPUT_FRAME_INDEX, output_frame->frame_idx);
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