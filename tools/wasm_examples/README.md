# WASM Example Sources

This directory contains example C source code that can be compiled to WebAssembly for use with the WAMR-ESP32-Arduino library.

## Files

- **add.c** - Simple addition function
- **math.c** - Multiple math operations (add, subtract, multiply, divide, fibonacci)
- **native_calls.c** - Example using native Arduino functions
- **Makefile** - Build script for all examples

## Prerequisites

### Option 1: WASI SDK (Recommended)

```bash
# Download and extract WASI SDK
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-21/wasi-sdk-21.0-linux.tar.gz
tar xf wasi-sdk-21.0-linux.tar.gz
sudo mv wasi-sdk-21.0 /opt/wasi-sdk
```

### Option 2: System Clang

If your system clang supports wasm32 target:
```bash
clang --version  # Check if wasm32 is listed
```

## Building

### Using Make

```bash
# Build all WASM files
make

# Generate C header files
make headers

# Clean build artifacts
make clean
```

### Manual Compilation

**Simple function:**
```bash
clang --target=wasm32 -nostdlib -Wl,--no-entry \
      -Wl,--export=add -O3 -o add.wasm add.c
```

**With native functions:**
```bash
clang --target=wasm32 -nostdlib -Wl,--no-entry \
      -Wl,--export=setup -Wl,--export=loop \
      -Wl,--allow-undefined -O3 \
      -o native_calls.wasm native_calls.c
```

## Converting to C Arrays

To embed WASM in Arduino sketches:

```bash
# Convert to C array
xxd -i add.wasm > add_wasm.h
```

This creates a header file like:
```c
unsigned char add_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, ...
};
unsigned int add_wasm_len = 44;
```

## Using in Arduino

```cpp
#include "add_wasm.h"

WamrModule module;

void setup() {
  WamrRuntime::begin();
  module.load(add_wasm, add_wasm_len);

  uint32_t args[2] = {10, 20};
  module.callFunction("add", 2, args);
  Serial.printf("Result: %u\n", args[0]);  // 30
}
```

## Inspecting WASM

Use WebAssembly Binary Toolkit (wabt):

```bash
# Install wabt
sudo apt install wabt  # Ubuntu/Debian
brew install wabt      # macOS

# Convert to text format
wasm2wat add.wasm -o add.wat

# View module structure
wasm-objdump -x add.wasm

# View disassembly
wasm-objdump -d add.wasm
```

## Example Output

**add.wat (text format):**
```wat
(module
  (func $add (export "add") (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
  )
)
```

## Tips

1. **Keep modules small** - Target <100KB for better performance
2. **Use -O3 or -Os** - Optimize for speed or size
3. **Export only needed functions** - Reduces module size
4. **Test on desktop first** - Use `iwasm` before deploying to ESP32
5. **Check exported functions** - Use `wasm-objdump -x` to verify

## Troubleshooting

**"clang: error: unknown target triple 'wasm32'"**
- Your clang doesn't support WebAssembly
- Install WASI SDK instead

**"undefined symbol: ..."**
- Forgot `-Wl,--allow-undefined` for native function calls

**Module too large:**
- Use `-Os` for size optimization
- Add `-Wl,--strip-all` to remove debug info
- Add `-Wl,--gc-sections` to remove unused code

## See Also

- [Building WASM Guide](../../docs/BUILDING_WASM.md)
- [API Reference](../../docs/API_REFERENCE.md)
- [Arduino Examples](../../examples/)
