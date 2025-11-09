/*
 * WAMR Threading Example for ESP32
 *
 * This example demonstrates:
 * - Safe API: callFunction() with automatic pthread wrapping (recommended)
 * - Raw API: callFunctionRaw() for manual thread management (advanced)
 * - Configuring thread stack size with setThreadStackSize()
 * - Performance comparison between safe and raw APIs
 *
 * Use this example to understand when to use each API variant.
 */

#include <WAMR.h>
#include <pthread.h>

// Simple WASM add function (same as basic example)
const unsigned char add_wasm[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
    0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
    0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
    0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b};
const unsigned int add_wasm_len = 41;

WamrModule wasmModule;

// Context for passing module to pthread
struct ThreadContext {
  WamrModule *module;
  int iterations;
};

// Thread function that uses RAW API
void *raw_api_thread(void *arg) {
  ThreadContext *ctx = (ThreadContext *)arg;

  Serial.println("\n[Raw API Thread] Started");
  Serial.println("[Raw API Thread] Using callFunctionRaw() - direct call");

  unsigned long start = micros();

  for (int i = 0; i < ctx->iterations; i++) {
    uint32_t args[2] = {i, i + 1};

    // RAW API: Direct call without pthread wrapper
    // We're already in a pthread, so this is safe
    if (!ctx->module->callFunctionRaw("add", 2, args)) {
      Serial.printf("[Raw API Thread] ERROR: Call %d failed!\n", i);
      break;
    }

    if (i == 0) {
      Serial.printf("[Raw API Thread] First call result: %u\n", args[0]);
    }
  }

  unsigned long elapsed = micros() - start;
  Serial.printf("[Raw API Thread] Completed %d calls in %lu µs\n",
                ctx->iterations, elapsed);
  Serial.printf("[Raw API Thread] Average: %.2f µs per call\n",
                (float)elapsed / ctx->iterations);

  return nullptr;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  WAMR Threading Example");
  Serial.println("========================================\n");

  // Initialize WAMR
  if (!WamrRuntime::begin(128 * 1024)) {
    Serial.println("FATAL: Failed to initialize WAMR!");
    while (1) delay(1000);
  }

  // Load module
  if (!wasmModule.load(add_wasm, add_wasm_len)) {
    Serial.println("FATAL: Failed to load module!");
    while (1) delay(1000);
  }

  Serial.println("✓ WAMR initialized and module loaded\n");

  // ============================================================================
  // EXAMPLE 1: Safe API (Recommended for Arduino)
  // ============================================================================

  Serial.println("========================================");
  Serial.println("Example 1: Safe API (callFunction)");
  Serial.println("========================================");
  Serial.println("This is the recommended API for Arduino code.");
  Serial.println("Automatically wraps calls in pthread context.\n");

  int iterations = 100;
  unsigned long start = micros();

  for (int i = 0; i < iterations; i++) {
    uint32_t args[2] = {i, i + 1};

    // SAFE API: Automatically wraps in pthread
    // Safe to call from Arduino setup/loop
    if (!wasmModule.callFunction("add", 2, args)) {
      Serial.printf("ERROR: Call %d failed!\n", i);
      break;
    }

    if (i == 0) {
      Serial.printf("First call result: %u\n", args[0]);
    }
  }

  unsigned long elapsed = micros() - start;
  Serial.printf("\nCompleted %d calls in %lu µs\n", iterations, elapsed);
  Serial.printf("Average: %.2f µs per call\n", (float)elapsed / iterations);
  Serial.println("(includes pthread creation overhead)\n");

  delay(1000);

  // ============================================================================
  // EXAMPLE 2: Custom Thread Stack Size
  // ============================================================================

  Serial.println("========================================");
  Serial.println("Example 2: Custom Thread Stack Size");
  Serial.println("========================================");
  Serial.println("Configure pthread stack for complex WASM functions.\n");

  // Set larger stack for complex WASM (default is 32KB)
  WamrModule::setThreadStackSize(64 * 1024);

  uint32_t args[2] = {100, 200};
  if (wasmModule.callFunction("add", 2, args)) {
    Serial.printf("Result with 64KB stack: %u\n\n", args[0]);
  }

  // Reset to default
  WamrModule::setThreadStackSize(32 * 1024);

  delay(1000);

  // ============================================================================
  // EXAMPLE 3: Raw API with Manual Thread Management
  // ============================================================================

  Serial.println("========================================");
  Serial.println("Example 3: Raw API (callFunctionRaw)");
  Serial.println("========================================");
  Serial.println("Advanced: Manual pthread management for performance.");
  Serial.println("Only use if you need fine control over threading.\n");

  ThreadContext ctx;
  ctx.module = &wasmModule;
  ctx.iterations = iterations;

  pthread_t thread;
  pthread_attr_t attr;

  // Create custom pthread with specific settings
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 48 * 1024); // Custom 48KB stack
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Create thread
  if (pthread_create(&thread, &attr, raw_api_thread, &ctx) != 0) {
    Serial.println("ERROR: Failed to create pthread!");
  } else {
    // Wait for thread to complete
    pthread_join(thread, nullptr);
  }

  pthread_attr_destroy(&attr);

  delay(1000);

  // ============================================================================
  // EXAMPLE 4: Performance Comparison
  // ============================================================================

  Serial.println("\n========================================");
  Serial.println("Example 4: Performance Comparison");
  Serial.println("========================================\n");

  // Measure Safe API with new thread per call
  iterations = 10;
  start = micros();
  for (int i = 0; i < iterations; i++) {
    uint32_t args[2] = {42, 58};
    wasmModule.callFunction("add", 2, args);
  }
  unsigned long safe_time = micros() - start;

  Serial.printf("Safe API (callFunction):\n");
  Serial.printf("  %d calls: %lu µs\n", iterations, safe_time);
  Serial.printf("  Average: %.2f µs/call\n", (float)safe_time / iterations);
  Serial.printf("  Overhead: ~%.2f ms for pthread creation\n\n",
                (float)safe_time / 1000.0);

  Serial.println("Raw API comparison:");
  Serial.println("  See Example 3 results above");
  Serial.println("  Raw API avoids pthread creation overhead");
  Serial.println("  Use when calling WASM 100+ times per second\n");

  // ============================================================================
  // Summary
  // ============================================================================

  Serial.println("========================================");
  Serial.println("  Summary & Recommendations");
  Serial.println("========================================\n");

  Serial.println("Use SAFE API (callFunction) when:");
  Serial.println("  • Calling from Arduino setup() or loop()");
  Serial.println("  • Making occasional WASM calls (<100 Hz)");
  Serial.println("  • You want simplicity and safety");
  Serial.println("  • You're new to WAMR or threads\n");

  Serial.println("Use RAW API (callFunctionRaw) when:");
  Serial.println("  • You manage threads yourself");
  Serial.println("  • Making high-frequency calls (>100 Hz)");
  Serial.println("  • You need fine control over thread attributes");
  Serial.println("  • You're already in a pthread context\n");

  Serial.println("⚠️  WARNING: callFunctionRaw() will CRASH");
  Serial.println("   if called from Arduino main task!\n");

  Serial.println("========================================");
  Serial.println("  Example Complete!");
  Serial.println("========================================\n");

  WamrRuntime::printMemoryUsage();
}

void loop() {
  delay(10000);
}
