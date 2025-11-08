# Building WebAssembly Modules for ESP32

This guide explains how to compile C/C++ code to WebAssembly (WASM) modules that can run on ESP32 with this library.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Compiling with WASI SDK](#compiling-with-wasi-sdk)
- [Advanced Compilation Options](#advanced-compilation-options)
- [Embedding WASM in Arduino Sketches](#embedding-wasm-in-arduino-sketches)
- [Using Native Functions](#using-native-functions)
- [Tips and Best Practices](#tips-and-best-practices)

## Prerequisites

### Option 1: WASI SDK (Recommended)

WASI SDK is the easiest way to compile C/C++ to WASM.

**Linux/macOS:**
```bash
# Download WASI SDK
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-21/wasi-sdk-21.0-linux.tar.gz

# Extract
tar xf wasi-sdk-21.0-linux.tar.gz

# Add to PATH (optional)
export PATH=$PATH:$(pwd)/wasi-sdk-21.0/bin
```

**Windows:**
Download from [WASI SDK Releases](https://github.com/WebAssembly/wasi-sdk/releases) and extract.

### Option 2: Emscripten

Emscripten is more feature-rich but has a larger footprint.

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Tools

You'll also want these tools:

- **wat2wasm** / **wasm2wat** - WebAssembly text format tools
- **wasm-objdump** - Inspect WASM modules
- **xxd** - Convert binary to C array

```bash
# Install wabt (WebAssembly Binary Toolkit)
sudo apt install wabt  # Ubuntu/Debian
brew install wabt      # macOS
```

## Quick Start

### Simple Function

**add.c:**
```c
int add(int a, int b) {
    return a + b;
}
```

**Compile:**
```bash
wasi-sdk-21.0/bin/clang --target=wasm32 \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export=add \
    -O3 \
    -o add.wasm add.c
```

**Verify:**
```bash
wasm2wat add.wasm  # See text format
wasm-objdump -x add.wasm  # See details
```

## Compiling with WASI SDK

### Basic Compilation

```bash
clang --target=wasm32 \
    -O3 \                      # Optimization level
    -nostdlib \                # Don't link standard library
    -Wl,--no-entry \           # No main() function required
    -Wl,--export=FUNCTION \    # Export function
    -o output.wasm input.c
```

### With Multiple Functions

**math.c:**
```c
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

**Compile:**
```bash
clang --target=wasm32 \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export=add \
    -Wl,--export=multiply \
    -Wl,--export=fibonacci \
    -O3 \
    -o math.wasm math.c
```

### With Memory

If your WASM module needs memory:

```c
// Allocate 1 page (64KB) of memory
__attribute__((export_name("memory")))
unsigned char memory[65536];

void write_data(int offset, int value) {
    memory[offset] = value;
}

int read_data(int offset) {
    return memory[offset];
}
```

Or let the linker configure it:

```bash
clang --target=wasm32 \
    -Wl,--initial-memory=65536 \      # 64KB initial
    -Wl,--max-memory=131072 \         # 128KB maximum
    ...
```

## Advanced Compilation Options

### Size Optimization

```bash
clang --target=wasm32 \
    -Os \                              # Optimize for size
    -flto \                            # Link-time optimization
    -Wl,--strip-all \                  # Remove debug info
    -Wl,--gc-sections \                # Remove unused sections
    -ffunction-sections \              # Separate functions
    -fdata-sections \                  # Separate data
    -o optimized.wasm input.c
```

### With libc-builtin

This library includes WAMR's libc-builtin which provides basic C functions:

```c
#include <string.h>  // memcpy, memset, etc.
#include <stdlib.h>  // abs, labs, etc.
#include <stdio.h>   // sprintf (limited)

int string_length(const char* str) {
    return strlen(str);
}

void copy_memory(char* dest, const char* src, int n) {
    memcpy(dest, src, n);
}
```

Compile without linking standard library:

```bash
clang --target=wasm32 \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export=string_length \
    -Wl,--export=copy_memory \
    -o libc_example.wasm libc_example.c
```

### Debug Build

```bash
clang --target=wasm32 \
    -g \                               # Include debug info
    -O0 \                              # No optimization
    -Wl,--export-all \                 # Export everything
    -o debug.wasm input.c
```

## Embedding WASM in Arduino Sketches

### Method 1: Binary to C Array with xxd

```bash
# Compile your WASM
clang --target=wasm32 -o module.wasm module.c ...

# Convert to C array
xxd -i module.wasm > module_wasm.h
```

**module_wasm.h:**
```c
unsigned char module_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, ...
};
unsigned int module_wasm_len = 1234;
```

**In your sketch:**
```cpp
#include "module_wasm.h"

void setup() {
  module.load(module_wasm, module_wasm_len);
}
```

### Method 2: Load from File System

If using SPIFFS or LittleFS:

```cpp
#include <FS.h>
#include <SPIFFS.h>

void setup() {
  SPIFFS.begin();

  File f = SPIFFS.open("/module.wasm", "r");
  size_t size = f.size();
  uint8_t* buffer = (uint8_t*)malloc(size);
  f.read(buffer, size);
  f.close();

  module.load(buffer, size);
  free(buffer);
}
```

### Method 3: Download via HTTP

```cpp
#include <HTTPClient.h>

void setup() {
  HTTPClient http;
  http.begin("http://example.com/module.wasm");

  int httpCode = http.GET();
  if (httpCode == 200) {
    size_t len = http.getSize();
    uint8_t* buffer = (uint8_t*)malloc(len);

    WiFiClient* stream = http.getStreamPtr();
    stream->readBytes(buffer, len);

    module.load(buffer, len);
    free(buffer);
  }
  http.end();
}
```

## Using Native Functions

Export Arduino functions to WASM so they can be called from WASM code.

### 1. Define Native Function

**In Arduino sketch:**
```cpp
#include <WAMR.h>

// Native function called from WASM
static void native_print(wasm_exec_env_t exec_env, int32_t value) {
  Serial.printf("WASM says: %d\n", value);
}

static int32_t native_random(wasm_exec_env_t exec_env, int32_t max) {
  return random(max);
}
```

### 2. Register Functions

```cpp
static NativeSymbol native_symbols[] = {
  {"print", (void*)native_print, "(i)", nullptr},
  {"random", (void*)native_random, "(i)i", nullptr}
};

void setup() {
  WamrRuntime::begin();

  // Register BEFORE loading module
  wasm_runtime_register_natives("env", native_symbols,
                                 sizeof(native_symbols) / sizeof(NativeSymbol));

  module.load(...);
}
```

### 3. Declare in C Code

**wasm_code.c:**
```c
// Declare external functions (provided by Arduino)
extern void print(int value);
extern int random(int max);

void my_function() {
    int r = random(100);
    print(r);
}
```

### 4. Compile

```bash
clang --target=wasm32 \
    -nostdlib \
    -Wl,--no-entry \
    -Wl,--export=my_function \
    -Wl,--allow-undefined \       # Allow external functions
    -o wasm_code.wasm wasm_code.c
```

### Function Signatures

The signature string describes parameter and return types:

| Signature | Meaning |
|-----------|---------|
| `"()"` | void func(void) |
| `"(i)"` | void func(int32) |
| `"(ii)"` | void func(int32, int32) |
| `"()i"` | int32 func(void) |
| `"(i)i"` | int32 func(int32) |
| `"(ii)i"` | int32 func(int32, int32) |
| `"(f)"` | void func(float) |
| `"(ff)f"` | float func(float, float) |

Types:
- `i` = i32 (32-bit integer)
- `I` = i64 (64-bit integer)
- `f` = f32 (float)
- `F` = f64 (double)

## Tips and Best Practices

### 1. Keep Modules Small

- Target < 100KB for WASM binary
- Use `-Os` for size optimization
- Remove unused functions with `--gc-sections`

### 2. Minimize Memory Usage

- Be conservative with stack usage
- Use module heap for dynamic allocation
- Consider PSRAM for large modules

### 3. Export Only What's Needed

```bash
# Don't do this:
-Wl,--export-all

# Do this instead:
-Wl,--export=function1 \
-Wl,--export=function2
```

### 4. Test on Desktop First

Use WAMR's `iwasm` tool to test modules before deploying to ESP32:

```bash
# Download iwasm
git clone https://github.com/bytecodealliance/wasm-micro-runtime
cd wasm-micro-runtime/product-mini/platforms/linux
mkdir build && cd build
cmake .. && make

# Test your module
./iwasm --function=add module.wasm 10 20
```

### 5. Profile Performance

Use the memory_test example to benchmark your modules:

- Measure execution time
- Monitor memory usage
- Test different optimization levels

### 6. Handle Errors Gracefully

Always check for errors when calling functions:

```cpp
if (!module.callFunction("my_func", argc, argv)) {
  Serial.printf("Error: %s\n", module.getError());
}
```

## Common Issues

### "Unknown import"
You forgot to register a native function. Check that all external functions declared in your C code are registered before loading the module.

### "Out of memory"
Increase heap size when initializing runtime or module:
```cpp
WamrRuntime::begin(256 * 1024);  // Larger heap
module.load(wasm, len, 32*1024, 128*1024);  // stack, heap
```

### Module too large
Use size optimization flags or split functionality into multiple modules.

### Undefined symbols
Make sure you're using `-Wl,--allow-undefined` when compiling WASM code that calls native functions.

## Resources

- [WebAssembly Specification](https://webassembly.github.io/spec/)
- [WASI SDK Documentation](https://github.com/WebAssembly/wasi-sdk)
- [WAMR Documentation](https://wamr.gitbook.io/)
- [WebAssembly Binary Toolkit](https://github.com/WebAssembly/wabt)
