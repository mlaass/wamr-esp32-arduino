# Troubleshooting Guide

Common issues and solutions when using WAMR-ESP32-Arduino.

## Compilation Issues

### "fatal error: wasm_export.h: No such file or directory"

**Problem:** Include paths not set correctly.

**Solution:**
- For PlatformIO: Ensure library is in `lib_deps`
- For Arduino IDE: Restart IDE after installing library
- Check that `src/` directory contains WAMR sources

### "undefined reference to `wasm_runtime_init`"

**Problem:** WAMR source files not being compiled.

**Solution:**
- Check `library.json` build configuration
- Ensure all source files are present in `src/wamr/core/`
- Run `tools/collect_sources.py` to update sources

### "multiple definition of `os_malloc`"

**Problem:** Conflicting memory allocation symbols.

**Solution:**
Add to `platformio.ini`:
```ini
build_flags =
    -DBH_MALLOC=wasm_runtime_malloc
    -DBH_FREE=wasm_runtime_free
```

## Runtime Issues

### "Failed to initialize WAMR runtime"

**Possible causes:**

**1. Insufficient memory:**
```cpp
// Try smaller heap
WamrRuntime::begin(64 * 1024);  // Instead of 128KB
```

**2. Already initialized:**
```cpp
if (WamrRuntime::isInitialized()) {
  WamrRuntime::end();
}
WamrRuntime::begin();
```

**3. Memory allocation failure:**
Check available memory before initializing:
```cpp
Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
if (ESP.getFreeHeap() < 150000) {
  Serial.println("Not enough memory!");
}
```

### "Failed to load module"

**Common errors and solutions:**

**"magic header not detected"**
- WASM binary is corrupted or invalid
- Check that you're passing correct data and size
- Verify WASM file with `wasm-objdump -h module.wasm`

**"WASM module load failed: unknown opcode"**
- Module uses unsupported WebAssembly features
- Recompile with basic WASM features only
- Check WAMR configuration in `build_config.h`

**"allocate memory failed"**
- Increase runtime heap size in `begin()`
- Reduce module complexity
- Use PSRAM if available

### "Failed to instantiate module"

**Possible causes:**

**Stack size too small:**
```cpp
module.load(wasm, len,
  32 * 1024,  // Increase stack size
  64 * 1024   // heap size
);
```

**Heap size too small:**
```cpp
module.load(wasm, len,
  16 * 1024,   // stack
  128 * 1024   // Increase heap size
);
```

**Not enough global memory:**
```cpp
WamrRuntime::begin(256 * 1024);  // Larger runtime heap
```

## Function Call Issues

### "assertion failed: pthread != NULL" or ESP32 crashes on callFunction()

**Problem:** Calling WASM functions from Arduino setup() or loop() without pthread context.

**Cause:**
WAMR requires execution to happen in a pthread context. Arduino's main task is NOT a pthread, so directly calling WASM functions causes an assertion failure or crash:
```
assertion "pthread != NULL" failed: file "esp-idf/espidf_thread.c", line 123
```

**Solution:**

**Use the safe API (recommended):**
```cpp
// This automatically wraps the call in pthread context
module.callFunction("add", 2, args);  // ✓ SAFE
```

**DO NOT use the raw API from Arduino:**
```cpp
// This will CRASH from setup() or loop()!
module.callFunctionRaw("add", 2, args);  // ✗ DANGER
```

**When to use each API:**

| API | Use from | Overhead | Use case |
|-----|----------|----------|----------|
| `callFunction()` | setup(), loop(), any Arduino code | ~100-500μs pthread creation | General use, <100 calls/sec |
| `callFunctionRaw()` | Your own pthread only | None | High-frequency calls, >100/sec |

**Example of using raw API correctly:**
```cpp
void* worker_thread(void* arg) {
  WamrModule* module = (WamrModule*)arg;

  // Now safe to use raw API - we're in a pthread
  for (int i = 0; i < 1000; i++) {
    uint32_t args[2] = {i, i+1};
    module->callFunctionRaw("add", 2, args);  // ✓ OK here
  }
  return nullptr;
}

void setup() {
  // ... init WAMR and load module ...

  pthread_t thread;
  pthread_create(&thread, nullptr, worker_thread, &module);
  pthread_join(thread, nullptr);
}
```

See the `threading` example for detailed demonstrations of both APIs.

### "Function not found"

**Problem:** Function name doesn't exist in module.

**Solution:**
```bash
# Check exported functions
wasm-objdump -x module.wasm | grep -A 5 "Export section"

# Make sure function is exported
clang ... -Wl,--export=my_function ...
```

### "Exception: out of bounds memory access"

**Problem:** WASM code accessing memory outside allocated region.

**Solution:**
1. Increase module heap size
2. Check for buffer overflows in C code
3. Enable bounds checking in compilation:
```bash
clang ... -Wl,--bounds-checks=1 ...
```

### "Exception: call stack exhausted"

**Problem:** Stack overflow, usually from deep recursion.

**Solution:**
```cpp
// Increase stack size
module.load(wasm, len,
  64 * 1024,  // Larger stack
  64 * 1024
);
```

Or fix recursion in code:
```c
// Before: Deep recursion
int fib(int n) {
  return (n <= 1) ? n : fib(n-1) + fib(n-2);
}

// After: Iterative
int fib(int n) {
  int a = 0, b = 1;
  for (int i = 0; i < n; i++) {
    int temp = a + b;
    a = b;
    b = temp;
  }
  return a;
}
```

## Native Function Issues

### "unknown import"

**Problem:** WASM module imports function that wasn't registered.

**Solution:**
```cpp
// Register ALL native functions BEFORE loading module
wasm_runtime_register_natives("env", natives, count);
module.load(...);  // Now load module
```

Check which imports are required:
```bash
wasm-objdump -x module.wasm | grep -A 10 "Import section"
```

### Native function crashes ESP32

**Possible causes:**

**1. Incorrect signature:**
```cpp
// Wrong: Signature says (i)i but function returns void
{"func", (void*)func, "(i)i", nullptr}  // ✗

// Correct:
{"func", (void*)func, "(i)", nullptr}   // ✓
```

**2. Stack overflow in native function:**
```cpp
void native_func(wasm_exec_env_t exec_env, int32_t arg) {
  char buffer[10000];  // Too large for stack!
  // Use heap instead:
  char* buffer = (char*)malloc(10000);
  // ... use buffer ...
  free(buffer);
}
```

**3. Invalid pointer access:**
```cpp
// Wrong: arg is WASM address, not native pointer
void native_read(wasm_exec_env_t exec_env, int32_t addr) {
  char* ptr = (char*)addr;  // ✗ CRASH!
  Serial.println(ptr);
}

// Correct: Validate and translate WASM address
void native_read(wasm_exec_env_t exec_env, int32_t wasm_addr) {
  wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
  char* native_addr = (char*)wasm_runtime_addr_app_to_native(
    inst, wasm_addr
  );
  if (native_addr) {
    Serial.println(native_addr);
  }
}
```

## Memory Issues

### Brownout detector triggered

**Problem:** Power supply can't handle current draw.

**Solution:**
1. Use better power supply (min 1A for ESP32)
2. Reduce clock speed:
```cpp
setCpuFrequencyMhz(160);  // Instead of 240MHz
```
3. Reduce memory usage

### Heap corruption detected

**Problem:** Memory corruption, usually from buffer overflow.

**Solution:**
- Enable stack protection
- Check array bounds in both native and WASM code
- Use smaller heap sizes to detect issues earlier

### Watchdog timer reset

**Problem:** Function taking too long.

**Solution:**
```cpp
// Add yield/delay in long-running code
void long_task() {
  for (int i = 0; i < 1000000; i++) {
    // ... work ...
    if (i % 1000 == 0) {
      yield();  // Let watchdog reset
    }
  }
}
```

Or disable watchdog (not recommended):
```cpp
#include <esp_task_wdt.h>

void setup() {
  esp_task_wdt_init(30, false);  // 30s timeout, no panic
}
```

## Performance Issues

### WASM execution very slow

**Solutions:**

1. **Use optimization flags:**
```bash
clang ... -O3 ...  # or -Os for size
```

2. **Enable fast interpreter** (should be default):
Check `src/wamr/build_config.h`:
```c
#define WASM_ENABLE_FAST_INTERP 1
```

3. **Reduce function call overhead:**
Batch operations instead of many small calls

4. **Profile your code:**
```cpp
unsigned long start = micros();
module.callFunction("func");
unsigned long time = micros() - start;
Serial.printf("Took %lu μs\n", time);
```

### Module loading slow

Normal for large modules. If critical:
1. Reduce module size
2. Pre-compile to AOT (requires enabling AOT support)
3. Cache loaded modules in PSRAM

## Platform-Specific Issues

### ESP32-S3 PSRAM not detected

**Check if PSRAM is enabled:**
```cpp
#if CONFIG_SPIRAM_SUPPORT
  Serial.println("PSRAM support enabled");
#else
  Serial.println("PSRAM not enabled in build!");
#endif
```

**Enable in PlatformIO:**
```ini
[env:esp32-s3]
board_build.arduino.memory_type = qio_opi  # or qio_qspi
```

### ESP32-C3 not supported

Currently this library only supports Xtensa (ESP32, ESP32-S3).

For ESP32-C3 (RISC-V):
1. Modify `build_config.h`:
```c
#define BUILD_TARGET_RISCV32 1
// Remove: #define BUILD_TARGET_XTENSA 1
```
2. Copy RISC-V specific sources
3. Test thoroughly

## Debugging Tips

### Enable verbose output

Modify `WAMR.cpp` to add more Serial.print statements.

### Check WAMR version

```cpp
#include <wamr/WAMR_VERSION.txt>
```

### Inspect WASM module

```bash
# View structure
wasm-objdump -h module.wasm

# View exports
wasm-objdump -x module.wasm

# View disassembly
wasm-objdump -d module.wasm

# Convert to text format
wasm2wat module.wasm -o module.wat
```

### Test module on desktop first

Build and test with desktop WAMR before deploying to ESP32:
```bash
iwasm --function=my_func module.wasm arg1 arg2
```

### Monitor memory usage

```cpp
void loop() {
  Serial.printf("Free: %u, Largest: %u\n",
    ESP.getFreeHeap(),
    heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  delay(1000);
}
```

## Getting Help

If you're still stuck:

1. **Check examples:** Review working examples in `examples/`
2. **Enable debug build:** Compile WASM with `-g` flag
3. **Simplify:** Create minimal reproduction case
4. **Check WAMR docs:** https://wamr.gitbook.io/
5. **Report issue:** Include:
   - ESP32 board type
   - Arduino/PlatformIO version
   - Complete error message
   - Minimal code to reproduce
   - WASM module (if possible)

## Common Error Messages

| Error | Likely Cause | Solution |
|-------|--------------|----------|
| `magic header not detected` | Invalid WASM binary | Verify with wasm-objdump |
| `out of memory` | Heap too small | Increase heap sizes |
| `unknown import` | Missing native function | Register before loading |
| `call stack exhausted` | Stack overflow | Increase stack or reduce recursion |
| `out of bounds` | Memory access error | Check array bounds |
| `Brownout detector` | Power supply issue | Better power supply |
| `Watchdog reset` | Function too slow | Add yield() calls |

## Performance Benchmarks

Typical performance on ESP32 @ 240MHz:

| Operation | Time |
|-----------|------|
| Runtime init | ~50ms |
| Module load (10KB) | ~20ms |
| Module load (100KB) | ~200ms |
| `callFunction()` overhead | ~100-500μs (includes pthread creation) |
| `callFunctionRaw()` overhead | ~10-50μs (no pthread) |
| Simple calculation (add) | ~1μs |
| Complex calculation (fib(20)) | ~500μs |

**API Performance Comparison:**

For 100 function calls:
- `callFunction()`: ~10-50ms total (pthread overhead dominates)
- `callFunctionRaw()` from single pthread: ~1-5ms total (no overhead)

**Recommendation:** Use `callFunction()` for occasional calls (<100 Hz). For high-frequency calls, manage your own pthread and use `callFunctionRaw()`.

*Your results may vary based on module complexity and system load.*
