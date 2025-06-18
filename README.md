# libtwig
Streamlined CedarX variant library, pruned for H.264 decoding with easy-to-use buffers

## Architecture Overview

### 1. Main Public API (`twig.h`)
- Cedar VE hardware device management
- Memory allocator abstraction (ION/IOMMU)
- Single user-side bitstream buffer decoding
- Direct access to decoded buffers via `twig_mem_t` struct

### 2. Register Definitions and Access (`twig_regs.h`)
- Helper functions for writing to and reading from Cedar VE registers
- Includes list of relevant register bases and offsets

### 3. Hardware Bitreader (`twig_bits.h`)
- Allows reading SPS/PPS information
- Convenience functions for single bit reads/skips

## Usage Example

```c
#include "twig.h"

// 1. Open Cedar device
twig_dev_t *device = twig_open();

// 2. Allocate and fill bitstream buffer
twig_mem_t *bitstream_buffer = twig_alloc_mem(device, frame_size);
memcpy(bitstream_buffer->virt_addr, frame_data, frame_size);

// 3. Create decoder context
twig_h264_decoder_t *decoder = twig_h264_decoder_init(device);

// 4. Send data to decoder and receive decoded data
twig_mem_t *output_buffer = twig_h264_decode_frame(device, bitstream_buffer);

// 4a. (Optional) Read frame resolution and/or process decoded data
int width, height;
twig_h264_get_frame_res(decoder, &width, &height);
printf("Received resolution: %dx%d\n", width, height);

// 5. Return frame to decoder when finished with it
twig_h264_return_frame(decoder, output_buffer);

// 6. Cleanup after completely finished with decoding
twig_h264_decoder_destroy(decoder);
twig_close(device);
```

## Build

```bash
mkdir ./build && cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=./install
cmake --build . --target install
```