# libtwig
Streamlined CedarX variant library, pruned for H.264 decoding with easy-to-use buffers

## Architecture Overview

### 1. Main Device Layer (`twig.h`)
- Cedar VE hardware device management
- Memory allocator abstraction (ION/IOMMU)
- Direct access to buffers via `twig_mem_t` struct

### 2. H.264 Decoding Layer (`twig_dec.h`)
- TBD

### 3. Register Definitions and Access (`twig_regs.h`)
- Helper functions for writing to and reading from Cedar VE registers
- Comprehensive list of relevant register bases and offsets

### 4. Software Bitreader (`twig_bits.h`)
- Allows reading SPS/PPS information before buffer requests
- Tracks bitstream position per `twig_bitreader_t` instance
- Convenience functions for single bit reads/skips
- Utility function `twig_show_bits` dumps bit values w/o advancing

## Usage Example

```c
#include "twig.h"

// 1. Open Cedar device
twig_dev_t *device = twig_open();

// TODO: Complete usage example

// X. Cleanup
twig_close(device);
```

## Build

```bash
mkdir ./build && cd build
cmake ../
cmake --build . --target install
```