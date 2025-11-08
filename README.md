# WAMR-ESP32-Arduino

WebAssembly Micro Runtime (WAMR) library for ESP32/ESP32-S3 with Arduino and PlatformIO support.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20ESP32--S3-green.svg)](https://www.espressif.com/)

## Overview

This library packages the [WebAssembly Micro Runtime (WAMR)](https://github.com/bytecodealliance/wasm-micro-runtime) from the Bytecode Alliance for easy use with ESP32 and ESP32-S3 in Arduino and PlatformIO projects.

WAMR is a lightweight WebAssembly runtime that allows you to:
- Run sandboxed WebAssembly code on your ESP32
- Dynamically load and execute code without reflashing
- Achieve near-native performance with AOT compilation
- Isolate untrusted code in a safe execution environment
- Update application logic over-the-air

### Features

- ✅ **Fast Interpreter** - Optimized WASM interpreter (~2X faster than classic)
- ✅ **Small Footprint** - Minimal memory overhead (50-100KB + module size)
- ✅ **Arduino-Friendly** - Simple C++ API with Serial debugging
- ✅ **PSRAM Support** - Automatically uses PSRAM when available
- ✅ **Native Functions** - Call Arduino functions from WASM
- ✅ **Built-in libc** - Standard C library functions available to WASM
- ✅ **ESP32 & ESP32-S3** - Supports Xtensa architecture

### Current Configuration

This library is configured for minimal interpreter-only build:
- Fast interpreter mode (enabled)
- AOT runtime (disabled - can be enabled)
- WASI (disabled - libc-builtin only)
- Multi-threading (disabled)
- JIT compilation (disabled)

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
[env:esp32]
platform = espressif32
board = esp32dev  ; or your board
framework = arduino
lib_deps =
    https://github.com/yourname/wamr-esp32-arduino.git
```

### Arduino IDE

1. Download this repository as ZIP
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file
4. Restart Arduino IDE

### Manual Installation

Clone or download this repository to your Arduino libraries folder:

```bash
cd ~/Arduino/libraries/  # or your libraries directory
git clone https://github.com/yourname/wamr-esp32-arduino.git
```

## Quick Start

```cpp
#include <WAMR.h>

// Your WASM module binary (embed or load from file)
const unsigned char my_wasm[] = { /* ... */ };
const unsigned int my_wasm_len = /* ... */;

WamrModule module;

void setup() {
  Serial.begin(115200);

  // Initialize WAMR runtime
  if (!WamrRuntime::begin(128 * 1024)) {  // 128KB heap
    Serial.println("Failed to init WAMR!");
    return;
  }

  // Load WASM module
  if (!module.load(my_wasm, my_wasm_len)) {
    Serial.println("Failed to load module!");
    return;
  }

  // Call WASM function
  uint32_t args[2] = {42, 58};
  if (module.callFunction("add", 2, args)) {
    Serial.printf("Result: %u\n", args[0]);
  }
}

void loop() {
  // Your code here
}
```

## Examples

The library includes three example sketches:

### 1. Basic WASM (`basic_wasm.ino`)
Demonstrates:
- Runtime initialization
- Loading a simple WASM module
- Calling WASM functions with parameters
- Getting results

### 2. Native Functions (`native_functions.ino`)
Demonstrates:
- Exporting Arduino functions to WASM
- Calling native functions from WASM
- Hardware control (LED blinking) from WASM code

### 3. Memory Test (`memory_test.ino`)
Demonstrates:
- Memory allocation strategies
- PSRAM usage
- Heap size configuration
- Performance profiling

## Documentation

- [Building WASM Modules](docs/BUILDING_WASM.md) - How to compile C/C++ to WASM
- [API Reference](docs/API_REFERENCE.md) - Complete API documentation
- [Troubleshooting](docs/TROUBLESHOOTING.md) - Common issues and solutions

## Memory Requirements

### ESP32 (without PSRAM)
- Minimum: 64KB runtime heap + 32KB module heap
- Recommended: 128KB runtime heap + 64KB module heap

### ESP32-S3 (with PSRAM)
- Can use 256KB+ for runtime heap
- PSRAM is automatically detected and used
- Better for larger WASM applications

### Typical Memory Usage
- WAMR Runtime: ~50-100KB (depending on features)
- WASM Module: Varies by module size
- Stack: 16KB per execution (default)
- Heap: Configurable per module

## Supported Platforms

- **ESP32** (Xtensa dual-core)
- **ESP32-S3** (Xtensa dual-core, PSRAM support)

### Planned Support
- ESP32-C3 (RISC-V) - requires RISC-V build configuration
- ESP32-C6 (RISC-V) - requires RISC-V build configuration

## Building WASM Modules

You need the [WASI SDK](https://github.com/WebAssembly/wasi-sdk) or [Emscripten](https://emscripten.org/) to compile C/C++ to WASM.

Simple example:

```bash
# Install WASI SDK
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-21/wasi-sdk-21.0-linux.tar.gz
tar xf wasi-sdk-21.0-linux.tar.gz

# Compile C to WASM
wasi-sdk-21.0/bin/clang --target=wasm32 -O3 \
    -nostdlib -Wl,--no-entry \
    -Wl,--export=add \
    -o add.wasm add.c
```

See [docs/BUILDING_WASM.md](docs/BUILDING_WASM.md) for detailed instructions.

## API Overview

### WamrRuntime Class
```cpp
// Initialize runtime (call once in setup())
WamrRuntime::begin(heap_size);

// Shutdown runtime
WamrRuntime::end();

// Check if initialized
WamrRuntime::isInitialized();

// Print memory usage
WamrRuntime::printMemoryUsage();
```

### WamrModule Class
```cpp
WamrModule module;

// Load WASM module
module.load(wasm_bytes, size, stack_size, heap_size);

// Call function
uint32_t args[] = {arg1, arg2};
module.callFunction("function_name", num_args, args);

// Get result
uint32_t result = args[0];  // Result in first argument

// Get error
const char* error = module.getError();

// Unload module
module.unload();
```

## Performance

Typical performance characteristics:

- Function call overhead: ~10-50μs
- Execution speed: ~50-70% of native C (interpreter mode)
- Memory overhead: ~2X for fast interpreter vs classic
- Startup time: <100ms for small modules

*Note: AOT (Ahead-of-Time) compilation can provide near-native performance but requires the `wamrc` compiler.*

## Limitations

Current build limitations:

- Interpreter only (no AOT/JIT runtime included)
- No WASI support (libc-builtin only)
- No multi-threading support
- Xtensa only (RISC-V not yet configured)

These can be enabled by modifying `src/wamr/build_config.h` and adding the required source files.

## Contributing

Contributions are welcome! Please:

1. Test on real hardware (ESP32/ESP32-S3)
2. Follow the existing code style
3. Update documentation as needed
4. Add examples if introducing new features

## License

This library is licensed under Apache License 2.0 with LLVM Exception, the same as WAMR.

See [LICENSE](LICENSE) for details.

## Acknowledgments

- [WebAssembly Micro Runtime (WAMR)](https://github.com/bytecodealliance/wasm-micro-runtime) by Bytecode Alliance
- [Espressif ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)

## Version

- Library Version: 1.0.0
- WAMR Version: 2.4.1+ (see `src/wamr/WAMR_VERSION.txt`)

## Links

- [WAMR Documentation](https://wamr.gitbook.io/)
- [WAMR GitHub](https://github.com/bytecodealliance/wasm-micro-runtime)
- [WebAssembly Specification](https://webassembly.github.io/spec/)
- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/)
