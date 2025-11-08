/*
 * WAMR Memory Test Example for ESP32
 *
 * This example demonstrates:
 * - Memory allocation and usage
 * - PSRAM detection and usage
 * - Heap size configuration
 * - Memory profiling
 *
 * Useful for understanding memory requirements for your WASM applications.
 */

#include <WAMR.h>
#include <esp_heap_caps.h>

// Test WASM module with memory operations
// Contains functions that allocate and use linear memory

const unsigned char memory_test_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
  0x00, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00,
  0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x1e, 0x02, 0x0b, 0x67, 0x65,
  0x74, 0x5f, 0x6d, 0x65, 0x6d, 0x73, 0x69, 0x7a, 0x65, 0x00, 0x00, 0x09,
  0x66, 0x69, 0x62, 0x6f, 0x6e, 0x61, 0x63, 0x63, 0x69, 0x00, 0x01, 0x0a,
  0x2b, 0x02, 0x07, 0x00, 0x3f, 0x00, 0x41, 0x10, 0x6c, 0x0b, 0x20, 0x00,
  0x20, 0x00, 0x41, 0x02, 0x49, 0x04, 0x40, 0x0f, 0x0b, 0x20, 0x00, 0x41,
  0x01, 0x6b, 0x10, 0x01, 0x20, 0x00, 0x41, 0x02, 0x6b, 0x10, 0x01, 0x6a,
  0x0b
};
const unsigned int memory_test_wasm_len = 93;

// WAT format:
// (module
//   (memory 1)
//   (func (export "get_memsize") (result i32)
//     memory.size
//     i32.const 16
//     i32.mul
//   )
//   (func (export "fibonacci") (param i32) (result i32)
//     local.get 0
//     local.get 0
//     i32.const 2
//     i32.lt_s
//     if
//       return
//     end
//     local.get 0
//     i32.const 1
//     i32.sub
//     call 1
//     local.get 0
//     i32.const 2
//     i32.sub
//     call 1
//     i32.add
//   )
// )

void printMemoryInfo() {
  Serial.println("\n=== ESP32 Memory Info ===");

  // Internal RAM
  size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t total_internal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  Serial.printf("Internal RAM: %u / %u bytes (%.1f%% used)\n",
                total_internal - free_internal, total_internal,
                100.0 * (total_internal - free_internal) / total_internal);

  // PSRAM
  #if CONFIG_SPIRAM_SUPPORT || CONFIG_ESP32_SPIRAM_SUPPORT
  size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  if (total_psram > 0) {
    Serial.printf("PSRAM:        %u / %u bytes (%.1f%% used)\n",
                  total_psram - free_psram, total_psram,
                  100.0 * (total_psram - free_psram) / total_psram);
    Serial.println("PSRAM is available and will be used for WAMR heap");
  } else {
    Serial.println("PSRAM: Not available");
  }
  #else
  Serial.println("PSRAM: Not enabled in build");
  #endif

  // Largest free block
  size_t largest_free = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  Serial.printf("Largest free block: %u bytes\n", largest_free);

  Serial.println("========================\n");
}

void testWithHeapSize(uint32_t runtime_heap, uint32_t module_heap) {
  Serial.printf("\n--- Testing with Runtime Heap: %uKB, Module Heap: %uKB ---\n",
                runtime_heap / 1024, module_heap / 1024);

  // Initialize WAMR with specific heap size
  if (!WamrRuntime::begin(runtime_heap)) {
    Serial.println("Failed to initialize WAMR!");
    Serial.println(WamrRuntime::getError());
    return;
  }

  printMemoryInfo();

  // Create and load module
  WamrModule module;
  uint32_t stack_size = 16 * 1024;  // 16KB stack

  Serial.printf("Loading module (stack: %uKB, heap: %uKB)...\n",
                stack_size / 1024, module_heap / 1024);

  if (!module.load(memory_test_wasm, memory_test_wasm_len,
                   stack_size, module_heap)) {
    Serial.println("Failed to load module!");
    Serial.println(module.getError());
    WamrRuntime::end();
    return;
  }

  Serial.println("✓ Module loaded successfully");

  // Test 1: Get memory size from WASM
  Serial.println("\nTest 1: Query WASM linear memory size");
  if (module.callFunction("get_memsize")) {
    uint32_t mem_size = module.getResult();
    Serial.printf("  WASM linear memory: %u KB\n", mem_size);
  }

  // Test 2: Run computation (Fibonacci)
  Serial.println("\nTest 2: Run Fibonacci calculations");
  uint32_t test_values[] = {10, 15, 20, 25};

  for (int i = 0; i < 4; i++) {
    uint32_t n = test_values[i];
    uint32_t args[1] = {n};

    unsigned long start = micros();
    bool success = module.callFunction("fibonacci", 1, args);
    unsigned long elapsed = micros() - start;

    if (success) {
      Serial.printf("  fib(%2u) = %-10u (took %lu μs)\n",
                    n, args[0], elapsed);
    } else {
      Serial.printf("  fib(%2u) = FAILED\n", n);
    }
  }

  printMemoryInfo();

  // Cleanup
  module.unload();
  WamrRuntime::end();

  Serial.println("✓ Test complete, resources freed\n");
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  WAMR Memory Test");
  Serial.println("========================================");

  printMemoryInfo();

  // Test 1: Small configuration (suitable for ESP32 without PSRAM)
  testWithHeapSize(64 * 1024,   // 64KB runtime heap
                   32 * 1024);  // 32KB module heap

  // Test 2: Medium configuration
  testWithHeapSize(128 * 1024,  // 128KB runtime heap
                   64 * 1024);  // 64KB module heap

  // Test 3: Large configuration (requires PSRAM or large RAM)
  #if CONFIG_SPIRAM_SUPPORT || CONFIG_ESP32_SPIRAM_SUPPORT
  if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0) {
    Serial.println("\n*** PSRAM detected, testing large configuration ***");
    testWithHeapSize(512 * 1024,   // 512KB runtime heap
                     256 * 1024);  // 256KB module heap
  }
  #endif

  Serial.println("\n========================================");
  Serial.println("  All Memory Tests Complete!");
  Serial.println("========================================");
  Serial.println("\nRecommendations:");
  Serial.println("- For ESP32 without PSRAM: Use 64-128KB runtime heap");
  Serial.println("- For ESP32 with PSRAM: Can use 256KB+ runtime heap");
  Serial.println("- Module heap should be based on WASM memory needs");
  Serial.println("- Always check available memory before initialization\n");

  printMemoryInfo();
}

void loop() {
  delay(10000);
}
