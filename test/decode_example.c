#include "twig_video.h"

static uint8_t example_sps[] = {
    0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x28, 
    0xD9, 0x00, 0x78, 0x02, 0x27, 0xE5, 0x84, 0x00
};

static uint8_t example_pps[] = {
    0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x05, 0xCB, 0x20
};

static uint8_t example_iframe_header[] = {
    0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00
};

// TODO: Implement real-world test that can use provided .h264 files
int main(int argc, char *argv[])
{
    printf("=== libtwig H264 Decoder Usage Example ===\n\n");
    
    // ===== STEP 1: Device Initialization (like VeInitialize) =====
    printf("Step 1: Opening Cedar device...\n");
    twig_dev_t *device = twig_open();
    if (!device) {
        fprintf(stderr, "Failed to open twig device\n");
        return -1;
    }
    printf("✓ Cedar device opened successfully\n\n");
    
    // ===== STEP 2: Video Engine Creation (like VideoEngineCreate) =====
    printf("Step 2: Creating video engine...\n");
    
    // Configure video stream information
    twig_video_info_t video_info = {
        .width = 1920,
        .height = 1080,
        .fps_num = 30,
        .fps_den = 1,
        .extra_data = NULL,      // No extra data for this example
        .extra_data_size = 0,
        .max_ref_frames = 16     // Typical for H264
    };
    
    // Configure decoder settings
    twig_video_config_t config = {
        .decode_key_frames_only = 0,
        .skip_b_frames_if_delay = 0,
        .max_output_width = 1920,
        .max_output_height = 1080,
        .enable_performance_mode = 0
    };
    
    twig_video_engine_t *engine = twig_video_engine_create(device, &config, &video_info);
    if (!engine) {
        fprintf(stderr, "Failed to create video engine\n");
        twig_close(device);
        return -1;
    }
    printf("✓ Video engine created successfully\n\n");
    
    // ===== STEP 3: Buffer Management Setup =====
    printf("Step 3: Setting up buffer management...\n");
    
    // Configure stream buffer (input bitstream)
    twig_sbm_config_t sbm_config = {
        .buffer_size = 4 * 1024 * 1024,  // 4MB stream buffer
        .max_frame_num = 64
    };
    
    if (twig_video_engine_set_sbm(engine, &sbm_config) < 0) {
        fprintf(stderr, "Failed to setup stream buffer\n");
        goto cleanup;
    }
    printf("✓ Stream buffer manager configured (4MB)\n");
    
    // Get frame buffer manager (output pictures)
    twig_fbm_t *fbm = twig_video_engine_get_fbm(engine);
    if (!fbm) {
        fprintf(stderr, "Failed to get frame buffer manager\n");
        goto cleanup;
    }
    printf("✓ Frame buffer manager created (%d frames)\n", 
           twig_fbm_num_total_frames(engine));
    printf("\n");
    
    // ===== STEP 4: Memory Management Demo =====
    printf("Step 4: Demonstrating memory management...\n");
    
    // Request buffer space for input data
    uint8_t *input_buf;
    int buf_size;
    int total_input_size = sizeof(example_sps) + sizeof(example_pps) + sizeof(example_iframe_header);
    
    if (twig_sbm_request_buffer(engine, total_input_size, &input_buf, &buf_size) < 0) {
        fprintf(stderr, "Failed to request stream buffer\n");
        goto cleanup;
    }
    printf("✓ Requested %d bytes from stream buffer\n", buf_size);
    
    // Copy example data to buffer (in real app, this would be from file/network)
    uint8_t *write_ptr = input_buf;
    memcpy(write_ptr, example_sps, sizeof(example_sps));
    write_ptr += sizeof(example_sps);
    memcpy(write_ptr, example_pps, sizeof(example_pps));
    write_ptr += sizeof(example_pps);
    memcpy(write_ptr, example_iframe_header, sizeof(example_iframe_header));
    
    printf("✓ Copied example H264 data to buffer\n");
    
    // Submit frame data to decoder
    twig_stream_frame_t frame_info = {
        .data = input_buf,
        .size = total_input_size,
        .pts = 0,
        .is_key_frame = 1,       // First frame is I-frame
        .is_first_part = 1,
        .is_last_part = 1
    };
    
    if (twig_sbm_add_stream_frame(engine, &frame_info) < 0) {
        fprintf(stderr, "Failed to add stream frame\n");
        goto cleanup;
    }
    printf("✓ Added frame to stream buffer (size: %d bytes)\n", total_input_size);
    
    // Check buffer status
    printf("Stream buffer status:\n");
    printf("  - Frames queued: %d\n", twig_sbm_stream_frame_num(engine));
    printf("  - Data size: %d bytes\n", twig_sbm_stream_data_size(engine));
    printf("  - Available frames: %d\n", twig_fbm_num_empty_frames(engine));
    printf("\n");
    
    // ===== DEMO: Decode Process (Up to decode call) =====
    printf("Demo: Ready for decode process...\n");
    printf("At this point, the application would call:\n");
    printf("  twig_result_t result = twig_video_engine_decode(engine, 0, 0, 0, 0);\n");
    printf("\n");
    
    printf("The decode function would:\n");
    printf("  1. Parse NAL units from stream buffer\n");
    printf("  2. Process SPS/PPS parameters\n");
    printf("  3. Configure VE hardware registers\n");
    printf("  4. Start hardware decode of slice data\n");
    printf("  5. Wait for completion interrupt\n");
    printf("  6. Return decoded frame or status\n");
    printf("\n");
    
    // Demo frame buffer operations
    printf("Demo: Frame buffer operations...\n");
    
    // In a real decoder, after successful decode, a picture would be available
    printf("After decode, application would call:\n");
    printf("  twig_picture_t *picture = twig_fbm_request_picture(engine, 100);\n");
    printf("  if (picture) {\n");
    printf("    // Display or process picture\n");
    printf("    printf(\"Got decoded picture: %%dx%%d\\n\", picture->width, picture->height);\n");
    printf("    // Return when done\n");
    printf("    twig_fbm_return_picture(engine, picture);\n");
    printf("  }\n");
    printf("\n");
    
    // ===== Architecture Summary =====
    printf("=== Architecture Summary ===\n");
    printf("✓ Hardware abstraction: twig_dev_t manages Cedar device\n");
    printf("✓ Video engine: Coordinates all decoder components\n");
    printf("✓ Stream buffer: Manages input bitstream in circular buffer\n");
    printf("✓ Frame buffer: Manages output picture buffers\n");
    printf("✓ H264 context: Hardware-specific decoder state\n");
    printf("✓ Memory management: ION allocator for DMA-able buffers\n");
    printf("\n");
    
    printf("Memory layout:\n");
    printf("  Stream Buffer: Circular buffer for input H264 data\n");
    printf("  VBV Buffer: Hardware bitstream buffer (ION/IOMMU)\n");
    printf("  Frame Buffers: YUV output buffers (ION/IOMMU)\n");
    printf("  Auxiliary Buffers: MV, neighbor info, etc. (ION/IOMMU)\n");
    printf("\n");
    
    printf("Data flow:\n");
    printf("  App → SBM → VBV → VE Hardware → FBM → App\n");
    printf("\n");
    
    // ===== Demonstration of Reset =====
    printf("Demo: Reset functionality...\n");
    twig_video_engine_reset(engine);
    printf("✓ Video engine reset completed\n");
    printf("  - Stream buffer cleared\n");
    printf("  - Frame buffers released\n");
    printf("  - H264 decoder state reset\n");
    printf("  - Hardware registers reset\n");
    printf("\n");

cleanup:
    printf("=== Cleanup ===\n");
    
    // Clean shutdown
    if (engine) {
        twig_video_engine_destroy(engine);
        printf("✓ Video engine destroyed\n");
    }
    
    if (device) {
        twig_close(device);
        printf("✓ Cedar device closed\n");
    }
    
    printf("\nExample completed successfully!\n");
    return 0;
}
