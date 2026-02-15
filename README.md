# ESP32 Memory Management & Diagnostics Program

## Overview

This program is a comprehensive memory testing utility for **ESP32** microcontrollers. It monitors and demonstrates:
- **SRAM (Internal Heap)** — on-chip memory for fast access
- **PSRAM (External SPI RAM)** — additional external memory for large buffers
- Memory allocation/deallocation cycles
- Contiguous memory block availability
- Large block allocation tests (up to 1 MB)

The program runs continuous memory cycles every 10 seconds, displaying real-time statistics and performing allocation stress tests.

## Hardware Requirements

- **Board:** YD-ESP32-S3 N16R8
- **PSRAM:** Optional but recommended (enables external memory testing)
- **Serial Connection:** USB-to-UART for monitoring output (115200 bps)

### Tested Configuration
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.psram_type = opi
board_build.extra_flags = -DBOARD_HAS_PSRAM
monitor_speed = 115200
```

## Features

### 1. **Comprehensive Memory Report**
Displays detailed statistics for both SRAM and PSRAM:
- Total memory size
- Used memory
- Free memory
- Maximum contiguous allocation block
- All values shown in both bytes and KB/MB

### 2. **SRAM Allocation Demo**
- Allocates 50 KB blocks sequentially (up to 6 blocks)
- Shows free/used memory after each allocation
- Demonstrates memory fragmentation
- Frees all blocks and verifies restoration

### 3. **PSRAM Large Block Demo (1 MB blocks)**
- Tests large allocation capacity (up to 3 × 1 MB blocks)
- Useful for buffer-heavy applications (image processing, audio, etc.)
- Shows whether your board can handle multiple large buffers

### 4. **Quick Allocation Tests**
- Tests 1 KB and 10 KB allocations from SRAM
- Tests 100 KB and 1 MB allocations from PSRAM (if available)
- Indicates success/failure with visual indicators (✓/✗)

## Build & Upload

### Prerequisites
- [PlatformIO](https://platformio.org/) installed
- ESP32 board connected via USB

### Commands
```bash
# Build the project
pio run

# Upload to board
pio run -t upload

# Build and upload in one command
pio run -t upload

# Monitor serial output
pio device monitor -b 115200

# Build, upload, and monitor
pio run -t upload && pio device monitor -b 115200
```

## Expected Output

```
===== MEMORY CYCLE #1 =====
========================================
ESP32 Memory Report
========================================

[SRAM - Internal Heap]
  Total:  402748 bytes (393.31 KB)
  Used:   29888 bytes (29.19 KB)
  Free:   372860 bytes (364.12 KB)
  Max contiguous alloc: 335860 bytes (327.99 KB)
  Free internal: 372860 bytes (364.12 KB)

[PSRAM - External SPI RAM]
  Detected: YES ✓
  Total:  8388607 bytes (8.00 MB)
  Free:   8386035 bytes (8.00 MB)

[Allocation Tests]
  ✓ 1 KB from SRAM: SUCCESS
  ✓ 10 KB from SRAM: SUCCESS
  ✓ 100 KB from PSRAM: SUCCESS
  ✓ 1 MB from PSRAM: SUCCESS

========================================


>>> SRAM Memory Allocation Demo <<<

Allocating 50 KB blocks from SRAM:
  Block 1 allocated: FREE = 314.11 KB, USED = 79.20 KB ✓
  Block 2 allocated: FREE = 264.09 KB, USED = 129.22 KB ✓
  Block 3 allocated: FREE = 214.07 KB, USED = 179.23 KB ✓
  Block 4 allocated: FREE = 164.06 KB, USED = 229.25 KB ✓
  Block 5 allocated: FREE = 114.04 KB, USED = 279.27 KB ✓
  Block 6 allocated: FREE = 64.03 KB, USED = 329.28 KB ✓

Total blocks allocated: 6

Freeing all 6 blocks:
  Block 1 freed: FREE = 114.04 KB, USED = 279.27 KB ✓
  Block 2 freed: FREE = 164.06 KB, USED = 229.25 KB ✓
  Block 3 freed: FREE = 214.07 KB, USED = 179.23 KB ✓
  Block 4 freed: FREE = 264.09 KB, USED = 129.22 KB ✓
  Block 5 freed: FREE = 314.11 KB, USED = 79.20 KB ✓
  Block 6 freed: FREE = 364.12 KB, USED = 29.19 KB ✓

All blocks freed. Memory restored.

>>> PSRAM Large Block Demo (1 MB blocks) <<<

Allocating 1.00 MB blocks from PSRAM:
  Block 1 allocated: FREE = 7.00 MB, USED = 1.00 MB ✓
  Block 2 allocated: FREE = 6.00 MB, USED = 2.00 MB ✓
  Block 3 allocated: FREE = 5.00 MB, USED = 3.00 MB ✓
  Block 4 allocated: FREE = 4.00 MB, USED = 4.00 MB ✓
  Block 5 allocated: FREE = 3.00 MB, USED = 5.00 MB ✓
  Block 6 allocated: FREE = 2.00 MB, USED = 6.00 MB ✓

Total blocks allocated: 6

Freeing all 6 blocks:
  Block 1 freed: FREE = 3.00 MB, USED = 5.00 MB ✓
  Block 2 freed: FREE = 4.00 MB, USED = 4.00 MB ✓
  Block 3 freed: FREE = 5.00 MB, USED = 3.00 MB ✓
  Block 4 freed: FREE = 6.00 MB, USED = 2.00 MB ✓
  Block 5 freed: FREE = 7.00 MB, USED = 1.00 MB ✓
  Block 6 freed: FREE = 8.00 MB, USED = 0.00 MB ✓

All blocks freed. Memory restored.
```

## Understanding Memory Metrics

| Term | Meaning |
|------|---------|
| **Total** | Maximum available memory for this heap type |
| **Free** | Currently unused memory |
| **Used** | Currently allocated memory |
| **Max contiguous** | Largest single block that can be allocated (may be less than Free due to fragmentation) |

**Example:** If Free = 400 KB but Max contiguous = 200 KB, the memory is fragmented into smaller chunks. `malloc(300 KB)` would fail even though 400 KB is free.

## Code Structure

- `setup()` — Initialize serial communication
- `loop()` — Main cycle that runs every 10 seconds
- `printMemoryInfo()` — Displays current memory statistics
- `testAllocation()` — Single allocation test
- `demoMemoryCycle()` — Allocate/deallocate small blocks (KB range)
- `demoMemoryCycleLarge()` — Allocate/deallocate large blocks (MB range)

## API Functions Used

All memory operations use `esp_heap_caps.h` functions:

```cpp
heap_caps_get_total_size(MALLOC_CAP_INTERNAL)          // Total SRAM
heap_caps_get_free_size(MALLOC_CAP_INTERNAL)           // Free SRAM
heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)  // Max contiguous SRAM
heap_caps_get_free_size(MALLOC_CAP_SPIRAM)             // Free PSRAM
heap_caps_get_total_size(MALLOC_CAP_SPIRAM)            // Total PSRAM
heap_caps_malloc(size, MALLOC_CAP_INTERNAL/SPIRAM)     // Allocate specific heap
psramFound()                                            // Check PSRAM availability
```

## Troubleshooting

### PSRAM Not Detected
- Verify `board_build.psram_type` is set in `platformio.ini`
- Check `board_build.extra_flags = -DBOARD_HAS_PSRAM`
- Ensure PSRAM module is soldered/connected on your board

### High Memory Usage
- SRAM is limited (~500 KB on ESP32-S3)
- Use PSRAM for large buffers (images, audio, etc.)
- Monitor `Max contiguous alloc` — fragmentation can limit allocations

## Performance Tips

1. **For large data buffers:** Use PSRAM (`ps_malloc()` or `MALLOC_CAP_SPIRAM`)
2. **For performance-critical code:** Use SRAM (faster access)
3. **For long-running apps:** Monitor fragmentation and consider defragmentation strategies
4. **For embedded systems:** The program uses ~30 KB SRAM at idle

## References

- [ESP32 Memory Layout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/mem_alloc.html)
- [Heap Memory Allocation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/heap_alloc.html)
- [ESP32 Architecture](https://www.espressif.com/en/products/microcontrollers/esp32/overview)

## Author

Memory testing utility for ESP32 embedded systems education.

## License

Open source. Use and modify freely.
