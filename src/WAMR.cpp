/*
 * WAMR Arduino Wrapper Library - Implementation
 *
 * Copyright (C) 2024 WAMR-ESP32-Arduino Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "WAMR.h"
#include <esp_heap_caps.h>

// Static member initialization
bool WamrRuntime::initialized = false;
char* WamrRuntime::global_heap_buf = nullptr;
const char* WamrRuntime::error_msg = nullptr;

// ============================================================================
// WamrRuntime Implementation
// ============================================================================

bool WamrRuntime::begin(uint32_t heap_pool_size) {
    if (initialized) {
        Serial.println("WAMR: Runtime already initialized");
        return true;
    }

    Serial.println("WAMR: Initializing runtime...");

    // Validate heap size
    if (heap_pool_size < WAMR_MIN_HEAP_SIZE) {
        error_msg = "Heap pool size too small (min 16KB)";
        Serial.printf("WAMR Error: %s\n", error_msg);
        return false;
    }

    // Allocate global heap buffer
    // Try to allocate from PSRAM first if available
    global_heap_buf = (char*)heap_caps_malloc(heap_pool_size,
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!global_heap_buf) {
        // Fall back to internal RAM
        Serial.println("WAMR: PSRAM not available, using internal RAM");
        global_heap_buf = (char*)malloc(heap_pool_size);
    } else {
        Serial.println("WAMR: Using PSRAM for heap");
    }

    if (!global_heap_buf) {
        error_msg = "Failed to allocate global heap";
        Serial.printf("WAMR Error: %s (%u bytes)\n", error_msg, heap_pool_size);
        return false;
    }

    // Initialize WAMR runtime
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = heap_pool_size;

    if (!wasm_runtime_full_init(&init_args)) {
        error_msg = "Failed to initialize WAMR runtime";
        Serial.printf("WAMR Error: %s\n", error_msg);
        free(global_heap_buf);
        global_heap_buf = nullptr;
        return false;
    }

    initialized = true;
    Serial.printf("WAMR: Runtime initialized with %u bytes heap\n", heap_pool_size);
    printMemoryUsage();

    return true;
}

void WamrRuntime::end() {
    if (!initialized) {
        return;
    }

    Serial.println("WAMR: Shutting down runtime...");

    wasm_runtime_destroy();

    if (global_heap_buf) {
        free(global_heap_buf);
        global_heap_buf = nullptr;
    }

    initialized = false;
    Serial.println("WAMR: Runtime shutdown complete");
}

void WamrRuntime::printMemoryUsage() {
    Serial.println("=== ESP32 Memory Status ===");
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest free block: %u bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    #if CONFIG_SPIRAM_SUPPORT || CONFIG_ESP32_SPIRAM_SUPPORT
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
    #else
    Serial.println("PSRAM: Not available");
    #endif

    Serial.println("===========================");
}

// ============================================================================
// WamrModule Implementation
// ============================================================================

WamrModule::WamrModule()
    : module(nullptr)
    , module_inst(nullptr)
    , exec_env(nullptr)
    , last_result(0)
    , loaded(false)
{
    memset(error_buf, 0, sizeof(error_buf));
}

WamrModule::~WamrModule() {
    unload();
}

bool WamrModule::load(const uint8_t* wasm_bytes, uint32_t size,
                      uint32_t stack_size, uint32_t heap_size) {
    if (!WamrRuntime::isInitialized()) {
        snprintf(error_buf, sizeof(error_buf), "WAMR runtime not initialized");
        Serial.printf("WAMR Error: %s\n", error_buf);
        return false;
    }

    // Unload any existing module
    unload();

    Serial.printf("WAMR: Loading module (%u bytes)...\n", size);

    // Load WASM module
    module = wasm_runtime_load(const_cast<uint8_t*>(wasm_bytes), size,
                               error_buf, sizeof(error_buf));
    if (!module) {
        Serial.printf("WAMR Error: Failed to load module: %s\n", error_buf);
        return false;
    }

    Serial.println("WAMR: Module loaded successfully");

    // Instantiate module
    Serial.printf("WAMR: Instantiating module (stack: %u, heap: %u)...\n",
                  stack_size, heap_size);

    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                           error_buf, sizeof(error_buf));
    if (!module_inst) {
        Serial.printf("WAMR Error: Failed to instantiate module: %s\n", error_buf);
        wasm_runtime_unload(module);
        module = nullptr;
        return false;
    }

    Serial.println("WAMR: Module instantiated successfully");

    // Create execution environment
    exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
    if (!exec_env) {
        snprintf(error_buf, sizeof(error_buf), "Failed to create execution environment");
        Serial.printf("WAMR Error: %s\n", error_buf);
        wasm_runtime_deinstantiate(module_inst);
        wasm_runtime_unload(module);
        module_inst = nullptr;
        module = nullptr;
        return false;
    }

    loaded = true;
    Serial.println("WAMR: Module ready for execution");

    return true;
}

bool WamrModule::callFunction(const char* func_name, uint32_t argc, uint32_t* argv) {
    if (!loaded || !module_inst) {
        snprintf(error_buf, sizeof(error_buf), "Module not loaded");
        Serial.printf("WAMR Error: %s\n", error_buf);
        return false;
    }

    // Look up function
    wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
    if (!func) {
        snprintf(error_buf, sizeof(error_buf), "Function '%s' not found", func_name);
        Serial.printf("WAMR Error: %s\n", error_buf);
        return false;
    }

    Serial.printf("WAMR: Calling function '%s'...\n", func_name);

    // Call function
    if (!wasm_runtime_call_wasm(exec_env, func, argc, argv)) {
        const char* exception = wasm_runtime_get_exception(module_inst);
        if (exception) {
            snprintf(error_buf, sizeof(error_buf), "Exception: %s", exception);
        } else {
            snprintf(error_buf, sizeof(error_buf), "Function call failed");
        }
        Serial.printf("WAMR Error: %s\n", error_buf);
        return false;
    }

    // Store result if function returned a value
    if (argc > 0 && argv) {
        last_result = argv[0];
        Serial.printf("WAMR: Function returned: %u\n", last_result);
    }

    Serial.printf("WAMR: Function '%s' completed successfully\n", func_name);
    return true;
}

uint32_t WamrModule::getResult() {
    return last_result;
}

const char* WamrModule::getError() {
    return error_buf[0] != '\0' ? error_buf : nullptr;
}

void WamrModule::unload() {
    if (exec_env) {
        wasm_runtime_destroy_exec_env(exec_env);
        exec_env = nullptr;
    }

    if (module_inst) {
        wasm_runtime_deinstantiate(module_inst);
        module_inst = nullptr;
    }

    if (module) {
        wasm_runtime_unload(module);
        module = nullptr;
    }

    loaded = false;
    memset(error_buf, 0, sizeof(error_buf));
}
