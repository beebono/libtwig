# libtwig
Streamlined CedarX variant library, pruned for H.264 decoding with easy-to-use buffers

## Architecture Overview

### 1. Device Layer (`twig_dev_t`)
- Cedar VE hardware device management
- Memory allocator abstraction (ION/IOMMU)
- Register access and interrupt handling
- Direct access to buffers via `twig_get_ion_fd` after successful decode

### 2. Video Engine (`twig_video_engine_t`)
- Main coordinator for all decoder components
- Hardware initialization and configuration

### 3. Stream Buffer Manager (SBM)
- Circular buffer for input H.264 bitstream
- Frame boundary management

### 4. Frame Buffer Manager (FBM)  
- Output picture buffer management
- Reference frame tracking

### 5. H.264 Decoder Context
- Hardware-specific decoder state
- NAL unit parsing and processing
- VE register configuration

## Usage Example

```c
#include "twig_video.h"

// 1. Open Cedar device
twig_dev_t *device = twig_open();

// 2. Create video engine
twig_video_info_t video_info = {
    .width = 1920, .height = 1080,
    .max_ref_frames = 16
};
twig_video_engine_t *engine = twig_video_engine_create(device, NULL, &video_info);

// 3. Setup stream buffer
twig_sbm_config_t sbm_config = { .buffer_size = 2*1024*1024 };
twig_video_engine_set_sbm(engine, &sbm_config);

// 4. Feed H.264 data
uint8_t *buf; int size;
twig_sbm_request_buffer(engine, frame_size, &buf, &size);
memcpy(buf, h264_data, frame_size);

twig_stream_frame_t frame = { .data = buf, .size = frame_size };
twig_sbm_add_stream_frame(engine, &frame);

// 5. TODO: Implement decode loop function here

// 6. Get output picture
twig_picture_t *pic = twig_fbm_request_picture(engine, 100);
if (pic) {
    // Process picture...
    twig_fbm_return_picture(engine, pic);
}

// 7. Cleanup
twig_video_engine_destroy(engine);
twig_close(device);
```

## Build

```bash
mkdir ./build && cd build
cmake ../
cmake --build . --target install
```