#include "twig.h"
#include "twig_dec.h"

static void print_usage(const char *prog_name) {
    printf("Usage: %s <input.h264> [output.yuv [width_of_input]]\n", prog_name);
    printf("  input.h264      - Raw H.264 elementary stream file\n");
    printf("  output.yuv      - Optional: dump first decoded frame to specified YUV file name\n");
    printf("  width_of_input  - Optional: width of the input file's frames in pixels\n");
    printf("                              (defaults to 1920, will not change actual resolution)\n");
}

static int load_file_to_memory(const char *filename, uint8_t **data, size_t *size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open %s\n", filename);
        return -1;
    }
    
    struct stat st;
    if (fstat(fd, &st) < 0) {
        printf("Failed to get file size\n");
        close(fd);
        return -1;
    }
    
    *size = st.st_size;
    *data = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (*data == MAP_FAILED) {
        printf("Failed to map file into memory\n");
        return -1;
    }
    
    return 0;
}

static void dump_yuv_frame(const char *filename, twig_mem_t *frame_buf, uint16_t width, uint16_t height) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Failed to create YUV output file\n");
        return;
    }
    
    uint8_t *yuv_data = (uint8_t *)frame_buf->virt;

    fwrite(yuv_data, 1, width * height, f);
    fwrite(yuv_data + (width * height), 1, (width * height) / 4, f);
    fwrite(yuv_data + (width * height) + (width * height) / 4, 1, (width * height) / 4, f);
    
    fclose(f);
    printf("First frame dumped to %s (%dx%d YUV420)\n", filename, width, height);
}

static int check_frame_content(twig_mem_t *frame_buf, uint16_t width, uint16_t height) {
    uint8_t *data = (uint8_t *)frame_buf->virt;
    int total_bytes = width * height * 3 / 2;
    int non_zero_count = 0;
    
    for (int i = 0; i < total_bytes; i++) {
        if (data[i] != 0) {
            non_zero_count++;
        }
    }
    
    printf("Frame content check: %d/%d bytes non-zero (%.1f%%)\n", 
           non_zero_count, total_bytes, (100.0 * non_zero_count) / total_bytes);
    
    if (non_zero_count > 0) {
        printf("Sample values: 0x%02x 0x%02x 0x%02x 0x%02x\n", 
               data[0], data[1], data[width], data[width+1]);
    }
    
    return non_zero_count;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *output_file = (argc > 2) ? argv[2] : NULL;
    int file_frame_width = (argc > 3) ? atoi(argv[3]) : 1920;
    
    printf("Twig H.264 Decoder Test\n");
    printf("Input file: %s\n", input_file);
    if (output_file) {
        printf("Output file: %s\n", output_file);
        if (file_frame_width != 1920)
            printf("Provided width: %d\n", file_frame_width);
        else
            printf("Default width: %d\n", 1920);
    }
    
    uint8_t *file_data;
    size_t file_size;
    if (load_file_to_memory(input_file, &file_data, &file_size) < 0)
        return 1;
    
    printf("Loaded %zu bytes from input file\n", file_size);

    twig_dev_t *cedar = twig_open();
    if (!cedar) {
        printf("Failed to initialize Cedar VE\n");
        munmap(file_data, file_size);
        return 1;
    }
    printf("Cedar VE initialized successfully\n");

    twig_h264_decoder_t *decoder = twig_h264_decoder_init(cedar, file_frame_width);
    if (!decoder) {
        printf("Failed to initialize H.264 decoder\n");
        twig_close(cedar);
        munmap(file_data, file_size);
        return 1;
    }
    printf("H.264 decoder initialized successfully\n");

    twig_mem_t *bitstream_buf = twig_alloc_mem(cedar, file_size);
    if (!bitstream_buf) {
        printf("Failed to allocate bitstream buffer\n");
        twig_h264_decoder_destroy(decoder);
        twig_close(cedar);
        munmap(file_data, file_size);
        return 1;
    }
    
    memcpy(bitstream_buf->virt, file_data, bitstream_buf->size);
    printf("Bitstream buffer allocated and filled (%zu bytes)\n", file_size);

    printf("\nStarting decode...\n");
    twig_mem_t *output_frame = twig_h264_decode_frame(decoder, bitstream_buf);
    
    if (output_frame) {
        printf("Got output from decoder, testing results...\n");
        printf("Frame buffer address: %p (virtual), 0x%x (physical), 0x%x (IOMMU)\n", 
               output_frame->virt, output_frame->phys_addr, output_frame->iommu_addr);
        
        int width, height;
        if (twig_get_frame_res(decoder, &width, &height) == 0)
            printf("Frame dimensions: %dx%d\n", width, height);

        if (check_frame_content(output_frame, width, height) == 0)
            printf("WARNING: Frame appears to be all zeros!\n");
        
        if (output_file)
            dump_yuv_frame(output_file, output_frame, width, height);

        twig_h264_return_frame(decoder, output_frame);
    } else {
        printf("Decode FAILED - no output from decoder\n");
    }
    
    twig_free_mem(cedar, bitstream_buf);
    twig_h264_decoder_destroy(decoder);
    twig_close(cedar);
    munmap(file_data, file_size);
    
    printf("\nTest completed\n");
    return output_frame ? 0 : 1;
}