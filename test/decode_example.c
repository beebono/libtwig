#include "twig.h"

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
        fprintf(stderr, "Failed to open /dev/cedar_dev\n");
        fprintf(stderr, "Are you on a legacy kernel? (version should be < 5.4)\n")
        return -1;
    }
    printf("✓ Cedar device opened successfully\n\n");
    
cleanup:
    printf("=== Cleanup ===\n");
    
    if (device) {
        twig_close(device);
        printf("✓ Cedar device closed\n");
    }
    
    printf("\nExample completed successfully!\n");
    return 0;
}
