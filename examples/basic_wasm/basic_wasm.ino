/*
 * WAMR Basic Example for ESP32
 *
 * This example demonstrates:
 * - Initializing WAMR runtime
 * - Loading a simple WASM module
 * - Calling WASM functions (using safe pthread-wrapped API)
 * - Cleanup
 *
 * The WASM module contains a simple function that adds two numbers.
 *
 * Note: This example uses the safe callFunction() API which automatically
 * handles pthread context. This is the recommended approach for Arduino code.
 * See the 'threading' example for advanced pthread management.
 */

#include <WAMR.h>

// Simple WASM module that exports an 'add' function
// This is the compiled output of:
//
//   int add(int a, int b) {
//       return a + b;
//   }
//
// Compiled with: wat2wasm add.wat -o add.wasm
// Then converted to C array: xxd -i add.wasm

const unsigned char add_wasm[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
    0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
    0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
    0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b};

const unsigned int add_wasm_len = 41;

// WAT (WebAssembly Text) format of the above binary:
// (module
//   (func $add (export "add") (param $a i32) (param $b i32) (result i32)
//     local.get $a
//     local.get $b
//     i32.add
//   )
// )

WamrModule wasmModule;

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial to be ready

  Serial.println("\n========================================");
  Serial.println("  WAMR Basic Example");
  Serial.println("========================================\n");

  // Step 1: Initialize WAMR runtime
  Serial.println("Step 1: Initializing WAMR runtime...");
  if (!WamrRuntime::begin(128 * 1024)) { // 128KB heap pool
    Serial.println("FATAL: Failed to initialize WAMR runtime!");
    Serial.println(WamrRuntime::getError());
    while (1)
      delay(1000);
  }
  Serial.println("✓ Runtime initialized\n");

  // Step 2: Load WASM module
  Serial.println("Step 2: Loading WASM module...");
  if (!wasmModule.load(add_wasm, add_wasm_len)) {
    Serial.println("FATAL: Failed to load WASM module!");
    Serial.println(wasmModule.getError());
    while (1)
      delay(1000);
  }
  Serial.println("✓ Module loaded\n");

  // Step 3: Call WASM function (Safe API - pthread wrapped)
  Serial.println("Step 3: Calling 'add(42, 58)'...");
  Serial.println("Note: Using safe callFunction() - automatically wraps in pthread\n");

  uint32_t args[2] = {42, 58}; // Arguments for add function
  if (!wasmModule.callFunction("add", 2, args)) {
    Serial.println("ERROR: Function call failed!");
    Serial.println(wasmModule.getError());
  } else {
    uint32_t result = args[0]; // Result is stored in first argument
    Serial.printf("✓ Result: %u\n", result);
    Serial.printf("  Expected: 100, Got: %u %s\n\n", result,
                  result == 100 ? "✓" : "✗");
  }

  // Step 4: Try more calculations
  Serial.println("Step 4: Testing more calculations...");

  struct TestCase {
    int a, b, expected;
  } tests[] = {{10, 20, 30}, {100, 200, 300}, {-5, 5, 0}, {999, 1, 1000}};

  for (int i = 0; i < 4; i++) {
    args[0] = tests[i].a;
    args[1] = tests[i].b;

    if (wasmModule.callFunction("add", 2, args)) {
      uint32_t result = args[0];
      bool passed = (int32_t)result == tests[i].expected;
      Serial.printf("  add(%d, %d) = %d %s\n", tests[i].a, tests[i].b,
                    (int32_t)result, passed ? "✓" : "✗");
    }
  }

  Serial.println("\n========================================");
  Serial.println("  Example Complete!");
  Serial.println("========================================\n");

  WamrRuntime::printMemoryUsage();
}

void loop() {
  // Nothing to do in loop
  delay(10000);
}
