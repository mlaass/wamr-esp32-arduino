/*
 * WAMR Native Functions Example for ESP32
 *
 * This example demonstrates:
 * - Exporting Arduino/ESP32 native functions to WASM
 * - Calling native functions from WASM code
 * - Controlling hardware (LED) from WASM
 *
 * The WASM module can call pinMode() and digitalWrite() to control an LED.
 */

#include <WAMR.h>

#define LED_PIN 2  // Built-in LED on most ESP32 boards

// Native function: pinMode wrapper
static void native_pinMode(wasm_exec_env_t exec_env, int32_t pin, int32_t mode) {
  Serial.printf("[Native] pinMode(%d, %d)\n", pin, mode);
  pinMode(pin, mode);
}

// Native function: digitalWrite wrapper
static void native_digitalWrite(wasm_exec_env_t exec_env, int32_t pin, int32_t value) {
  Serial.printf("[Native] digitalWrite(%d, %d)\n", pin, value);
  digitalWrite(pin, value);
}

// Native function: delay wrapper
static void native_delay(wasm_exec_env_t exec_env, int32_t ms) {
  Serial.printf("[Native] delay(%d)\n", ms);
  delay(ms);
}

// Native function: print to serial
static void native_print(wasm_exec_env_t exec_env, int32_t value) {
  Serial.printf("[WASM Output] %d\n", value);
}

// Define native function signatures
static NativeSymbol native_symbols[] = {
  {
    "pinMode",                    // Name visible to WASM
    (void*)native_pinMode,        // Function pointer
    "(ii)",                       // Signature: (int, int) -> void
    nullptr
  },
  {
    "digitalWrite",
    (void*)native_digitalWrite,
    "(ii)",
    nullptr
  },
  {
    "delay",
    (void*)native_delay,
    "(i)",
    nullptr
  },
  {
    "print",
    (void*)native_print,
    "(i)",
    nullptr
  }
};

// WASM module that uses native functions to blink an LED
// This is compiled from:
//
// extern void pinMode(int pin, int mode);
// extern void digitalWrite(int pin, int value);
// extern void delay(int ms);
// extern void print(int value);
//
// void setup_led() {
//     pinMode(2, 1);  // OUTPUT = 1
//     print(1);       // Signal setup complete
// }
//
// void blink_led() {
//     digitalWrite(2, 1);  // HIGH
//     delay(500);
//     digitalWrite(2, 0);  // LOW
//     delay(500);
// }

const unsigned char blink_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x03, 0x60,
  0x02, 0x7f, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02,
  0x4d, 0x04, 0x03, 0x65, 0x6e, 0x76, 0x07, 0x70, 0x69, 0x6e, 0x4d, 0x6f,
  0x64, 0x65, 0x00, 0x00, 0x03, 0x65, 0x6e, 0x76, 0x0c, 0x64, 0x69, 0x67,
  0x69, 0x74, 0x61, 0x6c, 0x57, 0x72, 0x69, 0x74, 0x65, 0x00, 0x00, 0x03,
  0x65, 0x6e, 0x76, 0x05, 0x64, 0x65, 0x6c, 0x61, 0x79, 0x00, 0x01, 0x03,
  0x65, 0x6e, 0x76, 0x05, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x00, 0x01, 0x03,
  0x03, 0x02, 0x02, 0x02, 0x07, 0x1d, 0x02, 0x09, 0x73, 0x65, 0x74, 0x75,
  0x70, 0x5f, 0x6c, 0x65, 0x64, 0x00, 0x04, 0x09, 0x62, 0x6c, 0x69, 0x6e,
  0x6b, 0x5f, 0x6c, 0x65, 0x64, 0x00, 0x05, 0x0a, 0x1d, 0x02, 0x0b, 0x00,
  0x41, 0x02, 0x41, 0x01, 0x10, 0x00, 0x41, 0x01, 0x10, 0x03, 0x0b, 0x0e,
  0x00, 0x41, 0x02, 0x41, 0x01, 0x10, 0x01, 0x41, 0xf4, 0x03, 0x10, 0x02,
  0x41, 0x02, 0x41, 0x00, 0x10, 0x01, 0x41, 0xf4, 0x03, 0x10, 0x02, 0x0b
};
const unsigned int blink_wasm_len = 168;

WamrModule wasmModule;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  WAMR Native Functions Example");
  Serial.println("========================================\n");

  // Initialize WAMR runtime
  Serial.println("Initializing WAMR runtime...");
  if (!WamrRuntime::begin(128 * 1024)) {
    Serial.println("FATAL: Failed to initialize WAMR!");
    while (1) delay(1000);
  }
  Serial.println("✓ Runtime initialized\n");

  // Register native functions BEFORE loading module
  Serial.println("Registering native functions...");
  if (!wasm_runtime_register_natives("env",
                                     native_symbols,
                                     sizeof(native_symbols) / sizeof(NativeSymbol))) {
    Serial.println("FATAL: Failed to register native functions!");
    while (1) delay(1000);
  }
  Serial.println("✓ Native functions registered:");
  Serial.println("  - pinMode(int, int)");
  Serial.println("  - digitalWrite(int, int)");
  Serial.println("  - delay(int)");
  Serial.println("  - print(int)\n");

  // Load WASM module
  Serial.println("Loading WASM module...");
  if (!wasmModule.load(blink_wasm, blink_wasm_len)) {
    Serial.println("FATAL: Failed to load WASM module!");
    Serial.println(wasmModule.getError());
    while (1) delay(1000);
  }
  Serial.println("✓ Module loaded\n");

  // Call setup_led function from WASM
  Serial.println("Calling WASM function 'setup_led()'...");
  if (!wasmModule.callFunction("setup_led")) {
    Serial.println("ERROR: setup_led() failed!");
    Serial.println(wasmModule.getError());
  }
  Serial.println("✓ LED setup complete\n");

  Serial.println("Now blinking LED from WASM...");
  Serial.println("(The LED on GPIO2 should blink)\n");
}

void loop() {
  // Call WASM function to blink LED
  if (!wasmModule.callFunction("blink_led")) {
    Serial.println("ERROR: blink_led() failed!");
    Serial.println(wasmModule.getError());
    delay(1000);
  }
}
