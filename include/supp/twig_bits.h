/*
 * libtwig - A streamlined CedarX variant library
 * Pruned for H.264 decoding with easy-to-use buffers
 * 
 * Supplemental software bitreader for small chunks of data
 *
 * Copyright (C) 2025 Noxwell(Beebono)
 * Based on CedarX framework by Allwinner Technology Co. Ltd.
 */

#ifndef TWIG_BITS_H_
#define TWIG_BITS_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    const uint8_t *data;
    size_t size;
    int bit_pos;
    int bits_left;
} twig_bitreader_t;

static inline void twig_bitreader_init(twig_bitreader_t *br, const uint8_t *data, size_t size) {
    br->data = data;
    br->size = size;
    br->bit_pos = 0;
    br->bits_left = size * 8;
}

static inline int twig_bitreader_bits_left(twig_bitreader_t *br) {
    return br->bits_left;
}

static inline uint32_t twig_get_bits(twig_bitreader_t *br, int n) {
    if (n <= 0 || n > 32 || br->bits_left < n)
        return 0;
    
    uint32_t result = 0;
    for (int i = 0; i < n; i++) {
        int byte_pos = br->bit_pos / 8;
        int bit_in_byte = 7 - (br->bit_pos % 8);
        uint8_t bit = (br->data[byte_pos] >> bit_in_byte) & 1;
        result = (result << 1) | bit;
        
        br->bit_pos++;
        br->bits_left--;
    }
    
    return result;
}

static inline uint32_t twig_get_1bit(twig_bitreader_t *br) {
    return twig_get_bits(br, 1);
}

static inline uint32_t twig_show_bits(twig_bitreader_t *br, int n) {
    if (n <= 0 || n > 32 || br->bits_left < n)
        return 0;

    int saved_pos = br->bit_pos;
    int saved_bits_left = br->bits_left;
    uint32_t result = twig_get_bits(br, n);
    
    br->bit_pos = saved_pos;
    br->bits_left = saved_bits_left;
    
    return result;
}

static inline void twig_skip_bits(twig_bitreader_t *br, int n) {
    if (n <= 0 || br->bits_left < n)
        return;
    
    br->bit_pos += n;
    br->bits_left -= n;
}

static inline void twig_skip_1bit(twig_bitreader_t *br) {
    twig_skip_bits(br, 1);
}

static inline uint32_t twig_get_ue_golomb(twig_bitreader_t *br) {
    int leading_zeros = 0;

    while (twig_get_bit(br) == 0) {
        leading_zeros++;
        if (leading_zeros > 31)
            return 0;
    }
    
    if (leading_zeros == 0)
        return 0;
    
    uint32_t value = twig_get_bits(br, leading_zeros);
    return (1 << leading_zeros) - 1 + value;
}

static inline int32_t twig_get_se_golomb(twig_bitreader_t *br) {
    uint32_t ue_val = twig_get_ue_golomb(br);
    
    if (ue_val == 0)
        return 0;
    
    if (ue_val & 1)
        return (int32_t)((ue_val + 1) / 2);
    else
        return -((int32_t)(ue_val / 2));
}

static inline void twig_byte_align(twig_bitreader_t *br) {
    int bits_to_skip = (8 - (br->bit_pos % 8)) % 8;
    twig_skip_bits(br, bits_to_skip);
}

#endif // TWIG_BITS_H_