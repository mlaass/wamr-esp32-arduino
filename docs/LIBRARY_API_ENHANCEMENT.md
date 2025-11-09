# WAMR ESP32 Arduino Library API Enhancement

## Problem Statement

The current WAMR library requires WASM function calls to execute within a pthread context. When called from Arduino's main task (which runs in FreeRTOS but not as a pthread), it triggers the assertion:
```
assert failed: pthread_self pthread.c:477 (false && "Failed to find current thread ID!")
```

This creates a poor developer experience as users must manually wrap all WASM calls in pthreads.

## Proposed Solution

Enhance the `WamrModule` class to provide both:
1. **Safe API** - Automatically handles pthread context (default, user-friendly)
2. **Raw API** - Direct calls for advanced users managing their own threads

## API Design

### Current API (Raw/Unsafe)
```cpp
class WamrModule {
public:
  // Direct call - REQUIRES pthread context
  bool callFunction(const char* func_name, uint32_t argc, uint32_t* argv);
};
```

### Enhanced API
```cpp
class WamrModule {
public:
  // SAFE: Automatically wraps in pthread (recommended for most users)
  bool callFunction(const char* func_name, uint32_t argc, uint32_t* argv);
  
  // RAW: Direct call without pthread wrapper (for advanced use cases)
  // Must be called from within a pthread context
  bool callFunctionRaw(const char* func_name, uint32_t argc, uint32_t* argv);
  
  // Configuration: Set pthread stack size for safe calls (default: 32KB)
  static void setThreadStackSize(size_t stack_size);
  
private:
  bool callFunctionInternal(const char* func_name, uint32_t argc, uint32_t* argv);
  static size_t thread_stack_size;
};
```

## Implementation Details

### 1. Rename Current Implementation
```cpp
bool WamrModule::callFunctionRaw(const char* func_name, uint32_t argc, uint32_t* argv) {
  // Current implementation - direct WASM call
  // No pthread wrapper
  // User must ensure they're in a pthread context
}
```

### 2. Add Safe Wrapper as Default
```cpp
struct WasmCallContext {
  WamrModule* module;
  const char* func_name;
  uint32_t argc;
  uint32_t* argv;
  bool success;
  char error_buf[256];
};

static void* wasm_thread_wrapper(void* arg) {
  WasmCallContext* ctx = (WasmCallContext*)arg;
  ctx->success = ctx->module->callFunctionRaw(
    ctx->func_name, ctx->argc, ctx->argv
  );
  return nullptr;
}

bool WamrModule::callFunction(const char* func_name, uint32_t argc, uint32_t* argv) {
  WasmCallContext ctx;
  ctx.module = this;
  ctx.func_name = func_name;
  ctx.argc = argc;
  ctx.argv = argv;
  ctx.success = false;
  
  pthread_t thread;
  pthread_attr_t attr;
  
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, thread_stack_size);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  
  if (pthread_create(&thread, &attr, wasm_thread_wrapper, &ctx) != 0) {
    snprintf(error_buf, sizeof(error_buf), 
             "Failed to create pthread for WASM execution");
    pthread_attr_destroy(&attr);
    return false;
  }
  
  pthread_join(thread, nullptr);
  pthread_attr_destroy(&attr);
  
  return ctx.success;
}
```

### 3. Add Configuration
```cpp
size_t WamrModule::thread_stack_size = 32 * 1024; // Default 32KB

void WamrModule::setThreadStackSize(size_t stack_size) {
  thread_stack_size = stack_size;
}
```

## Usage Examples

### Example 1: Simple Usage (Safe API - Recommended)
```cpp
#include <Arduino.h>
#include <WAMR.h>

WamrModule module;

void setup() {
  Serial.begin(115200);
  
  WamrRuntime::begin(128 * 1024);
  module.load(wasm_bytes, wasm_len, 16 * 1024, 32 * 1024);
  
  // Safe call - automatically wrapped in pthread
  uint32_t args[2] = {42, 58};
  if (module.callFunction("add", 2, args)) {
    Serial.printf("Result: %u\n", args[0]);
  }
}
```

### Example 2: Custom Thread Stack Size
```cpp
void setup() {
  Serial.begin(115200);
  
  // Configure larger stack for complex WASM functions
  WamrModule::setThreadStackSize(64 * 1024); // 64KB
  
  WamrRuntime::begin(128 * 1024);
  module.load(wasm_bytes, wasm_len, 16 * 1024, 32 * 1024);
  
  uint32_t args[2] = {42, 58};
  module.callFunction("add", 2, args); // Uses 64KB stack
}
```

### Example 3: Advanced - Managing Own Threads (Raw API)
```cpp
void* my_wasm_thread(void* arg) {
  WamrModule* module = (WamrModule*)arg;
  
  // We're already in a pthread, use raw API
  uint32_t args[2] = {42, 58};
  if (module->callFunctionRaw("add", 2, args)) {
    Serial.printf("Result: %u\n", args[0]);
  }
  
  return nullptr;
}

void setup() {
  Serial.begin(115200);
  
  WamrRuntime::begin(128 * 1024);
  module.load(wasm_bytes, wasm_len, 16 * 1024, 32 * 1024);
  
  // Create custom pthread with specific configuration
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 128 * 1024); // Custom stack
  
  pthread_create(&thread, &attr, my_wasm_thread, &module);
  pthread_join(thread, nullptr);
  pthread_attr_destroy(&attr);
}
```

## Migration Guide

### For Existing Code
The change is **backwards compatible** if we keep the current behavior:

**Option A: Make safe API the default (RECOMMENDED)**
- `callFunction()` → Safe (pthread wrapper)
- `callFunctionRaw()` → Unsafe (direct call)
- Existing code continues to work, just becomes safer

**Option B: Keep current behavior (less safe)**
- `callFunction()` → Unsafe (current behavior)
- `callFunctionSafe()` → Safe (pthread wrapper)
- Existing code unchanged but still has pthread issues

**Recommendation: Choose Option A** - Make the safe API the default. This is a breaking change but improves safety and user experience.

## Performance Considerations

### Overhead of Safe API
- **Thread creation**: ~1-2ms per call on ESP32
- **Context switching**: Minimal (microseconds)
- **Memory**: Stack size per call (default 32KB)

### When to Use Raw API
- High-frequency calls (>100 Hz) where thread overhead matters
- Already managing threads in application
- Need fine-grained control over thread attributes
- Calling from existing pthread context

### Optimization: Thread Pool (Future Enhancement)
For high-performance scenarios, consider adding a thread pool:
```cpp
class WamrModule {
public:
  // Use a persistent worker thread instead of creating new threads
  bool callFunctionPooled(const char* func_name, uint32_t argc, uint32_t* argv);
  
  static void initThreadPool(size_t num_threads = 1);
  static void shutdownThreadPool();
};
```

## Testing Requirements

1. **Unit Tests**
   - Safe API from Arduino main task
   - Raw API from pthread context
   - Thread stack size configuration
   - Error handling (thread creation failure)

2. **Integration Tests**
   - Multiple sequential calls
   - Concurrent calls from different tasks
   - Memory leak detection
   - Stack overflow detection

3. **Performance Tests**
   - Measure overhead of safe vs raw API
   - Stress test with rapid calls
   - Memory usage profiling

## Documentation Updates

### README.md
- Add "Quick Start" section using safe API
- Add "Advanced Usage" section for raw API
- Include performance comparison table

### API Reference
- Document both `callFunction()` and `callFunctionRaw()`
- Explain when to use each
- Document `setThreadStackSize()`

### Examples
- `examples/basic/` - Using safe API
- `examples/advanced_threading/` - Using raw API
- `examples/performance/` - Benchmarking both approaches

## Implementation Checklist

- [ ] Refactor current `callFunction()` to `callFunctionInternal()`
- [ ] Implement pthread wrapper in new `callFunction()`
- [ ] Add `callFunctionRaw()` as alias to internal implementation
- [ ] Add `setThreadStackSize()` static method
- [ ] Add thread stack size configuration
- [ ] Update error handling to capture pthread errors
- [ ] Write unit tests
- [ ] Update documentation
- [ ] Create example sketches
- [ ] Performance benchmarking
- [ ] Update CHANGELOG.md

## Open Questions

1. **Should we add a compile-time flag to disable safe API?**
   - For users who want zero overhead and manage threads themselves
   - `#define WAMR_DISABLE_SAFE_API`

2. **Should we detect if already in pthread context?**
   - Could skip wrapper if `pthread_self()` succeeds
   - Adds complexity but improves performance

3. **Should we add async API?**
   - `callFunctionAsync()` that returns immediately
   - Callback when complete
   - Useful for non-blocking operations

## Conclusion

This enhancement significantly improves the developer experience by making WAMR "just work" out of the box on Arduino, while still providing power users with direct access for performance-critical scenarios.

**Recommended Priority: HIGH** - This fixes a critical usability issue that causes crashes for new users.
