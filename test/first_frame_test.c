#include "twig.h"

static void print_usage(const char *prog_name) {
    printf("Usage: %s <input.h264> [output.yuv]\n", prog_name);
    printf("  input.h264      - Raw H.264 elementary stream file\n");
    printf("  output.yuv      - Optional: dump first decoded frame to specified YUV file name\n");
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

    uint8_t *yuv_data = (uint8_t *)frame_buf->virt_addr;

    fwrite(yuv_data, 1, width * height, f);
    fwrite(yuv_data + (width * height), 1, (width * height) / 4, f);
    fwrite(yuv_data + (width * height) + (width * height) / 4, 1, (width * height) / 4, f);

    fclose(f);
    printf("First frame dumped to %s (%dx%d YUV420)\n", filename, width, height);
}

static int find_start_code(const uint8_t *data, size_t size, size_t start) {
    size_t pos, leading_zeros = 0;
    for (pos = start; pos < size; pos++) {
        if (data[pos] == 0x00)
            leading_zeros++;
        else if (data[pos] == 0x01 && leading_zeros >= 2)
            return pos - 2;
        else
            leading_zeros = 0;
    }
    return -1;
}

static size_t extract_first_frame(const uint8_t *input_data, size_t input_size, uint8_t **output_data) {
    size_t pos = 0;
    int found_sps = 0, found_pps = 0, found_slice = 0;
    size_t frame_end = 0;
    
    printf("Extracting first frame data...\n");
    
    while (pos < input_size) {
        int start_code_pos = find_start_code(input_data, input_size, pos);
        if (start_code_pos < 0)
            break;
        
        pos = start_code_pos + 3;
        if (pos >= input_size)
            break;
        
        uint8_t nal_header = input_data[pos];
        uint8_t nal_type = nal_header & 0x1f;
        switch (nal_type) {
            case 7:
                if (!found_sps) {
                    printf("  Found SPS at position %u\n", start_code_pos);
                    found_sps = 1;
                }
                break;
            case 8:
                if (!found_pps) {
                    printf("  Found PPS at position %u\n", start_code_pos);
                    found_pps = 1;
                }
                break;
            case 1:
            case 5:
                if (!found_slice && found_sps && found_pps) {
                    printf("  Found first slice (%s) at position %u\n", 
                           (nal_type == 5) ? "IDR" : "Non-IDR", start_code_pos);
                    found_slice = 1;
                    int next_start = find_start_code(input_data, input_size, pos + 1);
                    frame_end = (next_start > 0) ? next_start : input_size;
                    goto extract_complete;
                }
                break;
        }
        pos++;
    }

extract_complete:
    if (found_sps && found_pps && found_slice) {
        *output_data = malloc(frame_end);
        if (*output_data) {
            memcpy(*output_data, input_data, frame_end);
            printf("  Extracted first frame: %zu bytes (SPS + PPS + First Slice)\n", frame_end);
            return frame_end;
        }
    }

    printf("  WARNING: Could not find complete first frame data\n");
    printf("  Found: SPS=%s, PPS=%s, Slice=%s\n", 
           found_sps ? "YES" : "NO", 
           found_pps ? "YES" : "NO", 
           found_slice ? "YES" : "NO");
    printf("  Falling back to full input file, decoding may take SIGNIFICANTLY longer...\n");

    *output_data = malloc(input_size);
    if (*output_data) {
        memcpy(*output_data, input_data, input_size);
        return input_size;
    }

    return 0;
}

static int check_frame_content(twig_mem_t *frame_buf, uint16_t width, uint16_t height) {
    uint8_t *data = (uint8_t *)frame_buf->virt_addr;
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

    printf("Twig H.264 Decoder Test\n");
    printf("Input file: %s\n", input_file);
    if (output_file)
        printf("Output file: %s\n", output_file);

    uint8_t *file_data;
    size_t file_size;
    if (load_file_to_memory(input_file, &file_data, &file_size) < 0)
        return 1;

    printf("Loaded %zu bytes from input file\n", file_size);

    uint8_t *first_frame_data;
    size_t first_frame_size = extract_first_frame(file_data, file_size, &first_frame_data);
    if (first_frame_size == 0) {
        printf("Failed to extract first frame data\n");
        munmap(file_data, file_size);
        return 1;
    }

    twig_dev_t *cedar = twig_open();
    if (!cedar) {
        printf("Failed to initialize Cedar VE\n");
        munmap(file_data, file_size);
        return 1;
    }
    printf("Cedar VE initialized successfully\n");

    twig_h264_decoder_t *decoder = twig_h264_decoder_init(cedar);
    if (!decoder) {
        printf("Failed to initialize H.264 decoder\n");
        twig_close(cedar);
        munmap(file_data, file_size);
        return 1;
    }
    printf("H.264 decoder initialized successfully\n");

    twig_mem_t *bitstream_buf = twig_alloc_mem(cedar, first_frame_size);
    if (!bitstream_buf) {
        printf("Failed to allocate bitstream buffer\n");
        twig_h264_decoder_destroy(decoder);
        twig_close(cedar);
        free(first_frame_data);
        munmap(file_data, file_size);
        return 1;
    }

    memcpy(bitstream_buf->virt_addr, first_frame_data, first_frame_size);
    printf("Bitstream buffer allocated and filled (%zu bytes)\n", first_frame_size);
    bitstream_buf->size = first_frame_size;
    printf("\nStarting decode...\n");
    twig_mem_t *output_frame = twig_h264_decode_frame(decoder, bitstream_buf);

    if (output_frame) {
        printf("Got output from decoder, testing results...\n");
        printf("Frame buffer address: %p (virtual), 0x%x (physical), 0x%x (IOMMU)\n", 
               output_frame->virt_addr, output_frame->phys_addr, output_frame->iommu_addr);

        int width, height;
        if (twig_h264_get_frame_res(decoder, &width, &height) == 0)
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
    free(first_frame_data);
    munmap(file_data, file_size);

    printf("\nTest completed\n");
    return output_frame ? 0 : 1;
}