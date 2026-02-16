#include <Arduino.h>
#include "esp_heap_caps.h"

void printMemoryInfo();
void testAllocation(size_t size, const char* label, uint32_t caps);
void demoMemoryCycle(uint32_t blockSize, const char* memTypes);
void demoMemoryCycleLarge(uint32_t blockSize, const char* memType);

void setup() {
    Serial.begin(115200);
}

void loop() {
    int cycle = 0;
    cycle++;

    Serial.printf("\n\n===== MEMORY CYCLE #%d =====\n", cycle);

    // Show initial state
    printMemoryInfo();

    // Demo SRAM allocation cycle
    Serial.println("\n>>> SRAM Memory Allocation Demo <<<");
    demoMemoryCycle(50 * 1024, "SRAM");  // 50 KB blocks

    // Demo PSRAM allocation cycle (if available)
    if (psramFound()) {
        // Demo PSRAM with large 1 MB blocks
        delay(2000);
        Serial.println("\n>>> PSRAM Large Block Demo (1 MB blocks) <<<");
        demoMemoryCycleLarge(1024 * 1024, "PSRAM");  // 1 MB blocks
    } else {
        Serial.println("\n>>> PSRAM not detected. Skipping PSRAM demo. <<<");
    }

    delay(10000);  // Wait 10 seconds before next cycle
}

void printMemoryInfo() {
    size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t usedHeap = totalHeap - freeHeap;
    size_t maxAlloc = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    size_t freeInternal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    bool psramPresent = psramFound();
    size_t psramSize = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

    Serial.println("========================================");
    Serial.println("ESP32 Memory Report");
    Serial.println("========================================");

    // SRAM / Internal Heap
    Serial.println("\n[SRAM - Internal Heap]");
    Serial.printf("  Total:  %u bytes (%.2f KB)\n",
                    (unsigned)totalHeap, totalHeap / 1024.0);
    Serial.printf("  Used:   %u bytes (%.2f KB)\n",
                    (unsigned)usedHeap, usedHeap / 1024.0);
    Serial.printf("  Free:   %u bytes (%.2f KB)\n",
                    (unsigned)freeHeap, freeHeap / 1024.0);
    Serial.printf("  Max contiguous alloc: %u bytes (%.2f KB)\n",
                    (unsigned)maxAlloc, maxAlloc / 1024.0);
    Serial.printf("  Free internal: %u bytes (%.2f KB)\n",
                    (unsigned)freeInternal, freeInternal / 1024.0);

    // PSRAM
    Serial.println("\n[PSRAM - External SPI RAM]");
    Serial.printf("  Detected: %s\n", psramPresent ? "YES ✓" : "NO ✗");
    if (psramPresent) {
        Serial.printf("  Total:  %u bytes (%.2f MB)\n",
                        (unsigned)psramSize, psramSize / (1024.0 * 1024.0));
        Serial.printf("  Free:   %u bytes (%.2f MB)\n",
                        (unsigned)freePsram, freePsram / (1024.0 * 1024.0));
    } else {
        Serial.println("  PSRAM not available on this board.");
    }

    // Test allocations
    Serial.println("\n[Allocation Tests]");
    testAllocation(1024, "1 KB from SRAM", MALLOC_CAP_INTERNAL);
    testAllocation(10240, "10 KB from SRAM", MALLOC_CAP_INTERNAL);

    if (psramPresent && freePsram > 1024 * 1024) {
        testAllocation(1024 * 100, "100 KB from PSRAM", MALLOC_CAP_SPIRAM);
        testAllocation(1024 * 1024, "1 MB from PSRAM", MALLOC_CAP_SPIRAM);
    }

    Serial.println("\n========================================\n");
}

void testAllocation(size_t size, const char* label, uint32_t caps) {
    void* ptr = heap_caps_malloc(size, caps);
    if (ptr != NULL) {
        Serial.printf("  ✓ %s: SUCCESS\n", label);
        free(ptr);
    } else {
        Serial.printf("  ✗ %s: FAILED (insufficient memory)\n", label);
    }
}

void demoMemoryCycle(uint32_t blockSize, const char* memType) {
    const int maxBlocks = 6;  // Try to allocate up to 6 blocks
    void* pointers[maxBlocks] = {NULL};
    int allocatedBlocks = 0;

    Serial.printf("\nAllocating %u KB blocks from %s:\n",
                    blockSize / 1024, memType);

    // Allocation phase
     for (int i = 0; i < maxBlocks; i++) {
        //pointers[i] = malloc(blockSize);
        pointers[i] = heap_caps_malloc(blockSize, MALLOC_CAP_INTERNAL);

        if (pointers[i] != NULL) {
            allocatedBlocks++;
            size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            size_t usedMemory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL)
                                                                - freeMemory;

            Serial.printf("  Block %d allocated: FREE = %.2f KB,"
                            " USED = %.2f KB ✓\n", i + 1,
                            freeMemory / 1024.0,
                            usedMemory / 1024.0);
            delay(500);
        } else {
            Serial.printf("  Block %d allocation FAILED: Insufficient memory"
                            " ✗\n", i + 1);

            break;
        }
    }

    Serial.printf("\nTotal blocks allocated: %d\n", allocatedBlocks);
    delay(1500);

    // Deallocation phase
    Serial.printf("\nFreeing all %d blocks:\n", allocatedBlocks);
    for (int i = 0; i < allocatedBlocks; i++) {
        if (pointers[i] != NULL) {
            free(pointers[i]);
            pointers[i] = NULL;

            size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            size_t usedMemory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL) -
                                                                freeMemory;

            Serial.printf("  Block %d freed: FREE = %.2f KB,"
                            " USED = %.2f KB ✓\n", i + 1,
                            freeMemory / 1024.0,
                            usedMemory / 1024.0);
            delay(500);
        }
    }

    Serial.printf("\nAll blocks freed. Memory restored.\n");
}

void demoMemoryCycleLarge(uint32_t blockSize, const char* memType) {
    const int maxBlocks = 6;  // Try to allocate up to 6 large blocks
    void* pointers[maxBlocks] = {NULL};
    int allocatedBlocks = 0;

    Serial.printf("\nAllocating %.2f MB blocks from %s:\n",
                    blockSize / (1024.0 * 1024.0), memType);

    // Allocation phase
    for (int i = 0; i < maxBlocks; i++) {
        pointers[i] = ps_malloc(blockSize);

        if (pointers[i] != NULL) {
            allocatedBlocks++;
            size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            size_t usedMemory = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) -
                                                                freeMemory;

            Serial.printf("  Block %d allocated: FREE = %.2f MB,"
                            " USED = %.2f MB ✓\n", i + 1,
                            freeMemory / (1024.0 * 1024.0),
                            usedMemory / (1024.0 * 1024.0));
            delay(500);
        } else {
            Serial.printf("  Block %d allocation FAILED: Insufficient"
                            " memory ✗\n", i + 1);

            break;
        }
    }

    Serial.printf("\nTotal blocks allocated: %d\n", allocatedBlocks);
    delay(1500);

    // Deallocation phase
    Serial.printf("\nFreeing all %d blocks:\n", allocatedBlocks);
    for (int i = 0; i < allocatedBlocks; i++) {
        if (pointers[i] != NULL) {
            free(pointers[i]);
            pointers[i] = NULL;

            size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            size_t usedMemory = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) -
                                                                freeMemory;

            Serial.printf("  Block %d freed: FREE = %.2f MB,"
                            " USED = %.2f MB ✓\n", i + 1,
                            freeMemory / (1024.0 * 1024.0),
                            usedMemory / (1024.0 * 1024.0));
            delay(500);
        }
    }

    Serial.printf("\nAll blocks freed. Memory restored.\n");
}

