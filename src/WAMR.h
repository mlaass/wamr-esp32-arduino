/*
 * WAMR Arduino Wrapper Library
 *
 * Copyright (C) 2025 WAMR-ESP32-Arduino Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arduino-friendly C++ wrapper for WebAssembly Micro Runtime (WAMR)
 */

#ifndef _WAMR_ARDUINO_H
#define _WAMR_ARDUINO_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

// Include WAMR build configuration
#include "wamr/build_config.h"

// Include WAMR public API
#include "wamr/iwasm/include/wasm_export.h"

// Default configuration values
#define WAMR_DEFAULT_HEAP_SIZE (64 * 1024)  // 64KB default heap
#define WAMR_MIN_HEAP_SIZE (16 * 1024)      // 16KB minimum
#define WAMR_MAX_HEAP_SIZE (512 * 1024)     // 512KB maximum
#define WAMR_DEFAULT_STACK_SIZE (16 * 1024) // 16KB stack
#define WAMR_DEFAULT_HEAP_POOL (128 * 1024) // 128KB pool for runtime

/**
 * WAMR Module wrapper class
 *
 * Represents a loaded WebAssembly module instance.
 */
class WamrModule {
public:
  WamrModule();
  ~WamrModule();

  /**
   * Load WASM module from byte array
   *
   * @param wasm_bytes Pointer to WASM binary data
   * @param size Size of WASM binary in bytes
   * @param stack_size Stack size for WASM execution (default: 16KB)
   * @param heap_size Heap size for WASM module (default: 64KB)
   * @return true if successful, false otherwise
   */
  bool load(const uint8_t *wasm_bytes, uint32_t size,
            uint32_t stack_size = WAMR_DEFAULT_STACK_SIZE,
            uint32_t heap_size = WAMR_DEFAULT_HEAP_SIZE);

  /**
   * Call a WASM function by name
   *
   * @param func_name Name of the exported function
   * @param argc Number of arguments
   * @param argv Array of arguments (uint32_t values)
   * @return true if call succeeded, false otherwise
   *
   * Note: Check getError() for error details if call fails
   */
  bool callFunction(const char *func_name, uint32_t argc = 0,
                    uint32_t *argv = nullptr);

  /**
   * Get the result of the last function call (if any)
   *
   * @return Result value (for functions returning i32)
   */
  uint32_t getResult();

  /**
   * Get last error message
   *
   * @return Error string, or nullptr if no error
   */
  const char *getError();

  /**
   * Check if module is loaded and valid
   */
  bool isLoaded() const { return module_inst != nullptr; }

  /**
   * Unload the module and free resources
   */
  void unload();

  /**
   * Get module instance (for advanced usage)
   */
  wasm_module_inst_t getInstance() { return module_inst; }

private:
  wasm_module_t module;
  wasm_module_inst_t module_inst;
  wasm_exec_env_t exec_env;
  char error_buf[128];
  uint32_t last_result;
  bool loaded;
};

/**
 * WAMR Runtime wrapper class
 *
 * Manages the WAMR runtime initialization and global settings.
 * You must call begin() before using any WAMR functionality.
 */
class WamrRuntime {
public:
  /**
   * Initialize WAMR runtime
   *
   * @param heap_pool_size Size of global heap pool for runtime (default: 128KB)
   * @return true if initialization successful
   */
  static bool begin(uint32_t heap_pool_size = WAMR_DEFAULT_HEAP_POOL);

  /**
   * Shutdown WAMR runtime and free all resources
   */
  static void end();

  /**
   * Check if runtime is initialized
   */
  static bool isInitialized() { return initialized; }

  /**
   * Get runtime error (if initialization failed)
   */
  static const char *getError() { return error_msg; }

  /**
   * Print memory usage information to Serial
   */
  static void printMemoryUsage();

private:
  static bool initialized;
  static char *global_heap_buf;
  static const char *error_msg;
};

#endif /* _WAMR_ARDUINO_H */
