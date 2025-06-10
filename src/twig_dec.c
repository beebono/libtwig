#include "twig_dec.h"
#include "twig_regs.h"

#define EXPORT __attribute__((visibility ("default")))

void *twig_get_ve_regs(twig_dev_t *cedar, int flag);
int twig_wait_for_ve(twig_dev_t *cedar);
void twig_put_ve_regs(twig_dev_t *cedar);

EXPORT twig_h264_decoder_t *twig_h264_decoder_init(twig_dev_t *cedar) {

}

EXPORT int twig_h264_decode_frame(twig_h264_decoder_t *decoder, twig_mem_t *bitstream_buf,
                            size_t bitstream_size, twig_mem_t *output_buf) {

}

EXPORT void twig_h264_return_frame(twig_h264_decoder_t *decoder, twig_mem_t *output_buf) {

}

EXPORT void twig_h264_decoder_destroy(twig_h264_decoder_t* decoder) {
    
}