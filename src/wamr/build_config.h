/*
 * WAMR Build Configuration for ESP32 Arduino
 *
 * This file centralizes all WAMR build configuration flags.
 * These settings configure WAMR for minimal interpreter-only build
 * suitable for ESP32/ESP32-S3 with Arduino framework.
 */

#ifndef _WAMR_BUILD_CONFIG_H
#define _WAMR_BUILD_CONFIG_H

/* Platform configuration */
#ifndef BH_PLATFORM_ESP_IDF
#define BH_PLATFORM_ESP_IDF 1
#endif

/* Architecture configuration */
#ifndef BUILD_TARGET_XTENSA
#define BUILD_TARGET_XTENSA 1
#endif

/* Interpreter configuration */
#ifndef WASM_ENABLE_INTERP
#define WASM_ENABLE_INTERP 1
#endif

#ifndef WASM_ENABLE_FAST_INTERP
#define WASM_ENABLE_FAST_INTERP 1
#endif

/* Library configuration */
#ifndef WASM_ENABLE_LIBC_BUILTIN
#define WASM_ENABLE_LIBC_BUILTIN 1
#endif

/* Features */
#ifndef WASM_ENABLE_BULK_MEMORY
#define WASM_ENABLE_BULK_MEMORY 1
#endif

#ifndef WASM_ENABLE_REF_TYPES
#define WASM_ENABLE_REF_TYPES 1
#endif

/* Memory management */
#ifndef BH_MALLOC
#define BH_MALLOC wasm_runtime_malloc
#endif

#ifndef BH_FREE
#define BH_FREE wasm_runtime_free
#endif

/* Runtime configuration constants */
#ifndef WASM_STACK_GUARD_SIZE
#define WASM_STACK_GUARD_SIZE (1024 * 3)  // 3KB for ESP32
#endif

#ifndef BLOCK_ADDR_CACHE_SIZE
#define BLOCK_ADDR_CACHE_SIZE 64
#endif

#ifndef BLOCK_ADDR_CONFLICT_SIZE
#define BLOCK_ADDR_CONFLICT_SIZE 2
#endif

#ifndef WASM_CONST_EXPR_STACK_SIZE
#define WASM_CONST_EXPR_STACK_SIZE 4
#endif

#ifndef WASM_TABLE_MAX_SIZE
#define WASM_TABLE_MAX_SIZE 1024
#endif

/* Disabled features (not needed for minimal build) */
#define WASM_ENABLE_AOT 0
#define WASM_ENABLE_JIT 0
#define WASM_ENABLE_LAZY_JIT 0
#define WASM_ENABLE_LIBC_WASI 0
#define WASM_ENABLE_MULTI_MODULE 0
#define WASM_ENABLE_LIB_PTHREAD 0
#define WASM_ENABLE_LIB_WASI_THREADS 0
#define WASM_ENABLE_SHARED_MEMORY 0
#define WASM_ENABLE_THREAD_MGR 0
#define WASM_ENABLE_TAIL_CALL 0
#define WASM_ENABLE_SIMD 0
#define WASM_ENABLE_EXCE_HANDLING 0
#define WASM_ENABLE_GC 0
#define WASM_ENABLE_MEMORY_PROFILING 0
#define WASM_ENABLE_PERF_PROFILING 0
#define WASM_ENABLE_DUMP_CALL_STACK 0
#define WASM_ENABLE_DEBUG_INTERP 0
#define WASM_ENABLE_MINI_LOADER 0

#endif /* _WAMR_BUILD_CONFIG_H */
