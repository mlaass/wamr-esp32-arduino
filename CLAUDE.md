# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

WAMR-ESP32-Arduino is a standalone Arduino/PlatformIO library that packages WebAssembly Micro Runtime (WAMR) for ESP32 and ESP32-S3 microcontrollers. This library provides an Arduino-friendly API wrapper around WAMR's core functionality.

## Repository Structure

```
wamr-esp32-arduino/
├── src/
│   ├── WAMR.h                    # Main Arduino API header
│   ├── WAMR.cpp                  # Arduino API implementation
│   └── wamr/                     # WAMR core sources
│       ├── build_config.h        # Build configuration flags
│       ├── WAMR_VERSION.txt      # WAMR version info
│       └── core/                 # WAMR source files
│           ├── iwasm/            # WASM interpreter and libraries
│           │   ├── include/      # Public WAMR headers
│           │   ├── common/       # Common runtime code
│           │   ├── interpreter/  # WASM interpreter
│           │   └── libraries/    # Built-in libraries
│           └── shared/           # Shared utilities
│               ├── platform/esp-idf/  # ESP32 platform layer
│               ├── mem-alloc/    # Memory allocators
│               └── utils/        # Utility functions
├── examples/                     # Arduino example sketches
│   ├── basic_wasm/              # Basic usage example
│   ├── native_functions/        # Native function export example
│   └── memory_test/             # Memory profiling example
├── docs/                        # Documentation
│   ├── BUILDING_WASM.md        # Guide to compiling WASM
│   ├── API_REFERENCE.md        # Complete API documentation
│   ├── TROUBLESHOOTING.md      # Common issues and solutions
│   └── SOURCE_FILES.md         # List of included WAMR files
├── tools/                       # Build and maintenance tools
│   ├── collect_sources.py      # Script to update WAMR sources
│   └── wasm_examples/          # Example WASM source code
├── library.json                 # PlatformIO library manifest
├── library.properties           # Arduino library manifest
└── README.md                    # Main documentation
```

## Architecture

### Arduino Wrapper Layer

**Files:** `src/WAMR.h`, `src/WAMR.cpp`

Provides two main classes:
- `WamrRuntime`: Static class for runtime initialization and management
- `WamrModule`: Instance class for loading and executing WASM modules

The wrapper handles:
- PSRAM detection and allocation
- Error handling and reporting
- Serial debugging output
- Arduino-style initialization

### WAMR Core Layer

**Directory:** `src/wamr/core/`

Contains selected WAMR sources configured for minimal interpreter build:
- Fast interpreter only (no AOT, no JIT)
- libc-builtin only (no WASI)
- ESP-IDF platform layer
- Xtensa architecture support

### Build Configuration

**File:** `src/wamr/build_config.h`

Centralized configuration for WAMR features. Current settings:
- `BUILD_TARGET_XTENSA=1` - Xtensa architecture (ESP32/ESP32-S3)
- `WASM_ENABLE_FAST_INTERP=1` - Fast interpreter mode
- `WASM_ENABLE_LIBC_BUILTIN=1` - Built-in libc
- `WASM_ENABLE_AOT=0` - AOT disabled (can be enabled)
- `WASM_ENABLE_LIBC_WASI=0` - WASI disabled (can be enabled)

To add features: modify this file and ensure required source files are present.

## Development Workflows

### Updating WAMR Sources

When updating to a newer WAMR version:

```bash
# From repository root
cd tools
python3 collect_sources.py /path/to/wamr-micro-runtime

# This will:
# 1. Copy required sources to src/wamr/core/
# 2. Generate docs/SOURCE_FILES.md
# 3. Write version to src/wamr/WAMR_VERSION.txt
```

The script (`tools/collect_sources.py`) knows which files are needed based on `SOURCE_PATTERNS` dictionary. Modify this if adding new features.

### Adding New Features

To enable additional WAMR features (e.g., AOT, WASI):

1. **Modify build configuration:**
   Edit `src/wamr/build_config.h` to enable feature flags

2. **Add required sources:**
   Update `tools/collect_sources.py` SOURCE_PATTERNS to include needed files

3. **Run collection script:**
   ```bash
   python3 tools/collect_sources.py /path/to/wamr
   ```

4. **Update library manifests:**
   Add any new build flags to `library.json` if needed

5. **Test thoroughly:**
   Test on real hardware with examples

6. **Update documentation:**
   Document new features in README.md and API_REFERENCE.md

### Testing Changes

1. **Test compilation:**
   ```bash
   # PlatformIO
   pio lib install file://.
   pio ci --board=esp32dev examples/basic_wasm/basic_wasm.ino
   ```

2. **Test on hardware:**
   - Upload each example to ESP32
   - Verify Serial output
   - Check memory usage
   - Test error conditions

3. **Test with different boards:**
   - ESP32 (no PSRAM)
   - ESP32 (with PSRAM)
   - ESP32-S3 (with PSRAM)

## Common Development Tasks

### Adding a New Example

1. Create directory in `examples/`
2. Create `.ino` file with same name as directory
3. Include comprehensive comments
4. Test on hardware
5. Add to `library.json` examples list
6. Document in README.md

### Modifying Arduino API

**Files to modify:**
- `src/WAMR.h` - Add new methods/classes
- `src/WAMR.cpp` - Implement functionality
- `docs/API_REFERENCE.md` - Document new API
- Create example demonstrating usage

### Debugging Build Issues

**Common issues:**

1. **Missing source files:**
   - Check `docs/SOURCE_FILES.md` for list
   - Run `collect_sources.py` again
   - Verify file paths in error messages

2. **Undefined references:**
   - Feature enabled in `build_config.h` but source files missing
   - Check WAMR source file dependencies
   - May need additional source files

3. **Conflicting symbols:**
   - Usually BH_MALLOC/BH_FREE redefinition
   - Ensure flags set correctly in `library.json`

## Memory Management

### ESP32 Memory Layout

- **Internal RAM**: ~520KB SRAM, shared between heap and stack
- **PSRAM** (ESP32-S3): Up to 8MB external PSRAM
- **Flash**: Code storage

### WAMR Memory Usage

Three main memory pools:
1. **Runtime heap** (`WamrRuntime::begin(size)`): Global pool for WAMR runtime
2. **Module heap** (`module.load(..., heap_size)`): Per-module WASM linear memory
3. **Stack** (`module.load(..., stack_size, ...)`): Execution stack

### PSRAM Strategy

The wrapper (`WAMR.cpp`) automatically:
1. Tries to allocate from PSRAM (MALLOC_CAP_SPIRAM)
2. Falls back to internal RAM if PSRAM unavailable
3. Prints memory source to Serial

## Platform Support

### Current: ESP32/ESP32-S3 (Xtensa)

**Architecture:** Xtensa dual-core
**Assembly:** `core/iwasm/common/arch/invokeNative_xtensa.s`
**Platform:** ESP-IDF layer compatible with Arduino-ESP32

### Planned: ESP32-C3/C6 (RISC-V)

To add RISC-V support:
1. Change `build_config.h`: `BUILD_TARGET_RISCV32=1`
2. Update `collect_sources.py` to include RISC-V assembly
3. Test native function calling
4. Update README.md platform support section

## Build System Integration

### PlatformIO

Library is discovered via:
- `library.json` manifest
- Build flags in `build.flags`
- Source filter in `build.srcFilter`

### Arduino IDE

Library is discovered via:
- `library.properties` manifest
- `keywords.txt` for syntax highlighting
- All sources in `src/` compiled automatically

## Documentation Maintenance

When making changes, update relevant docs:

- **README.md** - High-level changes, new features, compatibility
- **API_REFERENCE.md** - API changes, new methods, signatures
- **BUILDING_WASM.md** - Compilation changes, new requirements
- **TROUBLESHOOTING.md** - New issues, solutions, workarounds
- **CLAUDE.md** - Architecture changes, workflow updates

## Version Management

Version is tracked in multiple places:
- `library.json` - PlatformIO version
- `library.properties` - Arduino version
- `src/wamr/WAMR_VERSION.txt` - WAMR source version

Keep these synchronized when releasing.

## Release Process

1. Update version in `library.json` and `library.properties`
2. Test all examples on real hardware
3. Update CHANGELOG (if exists) or README
4. Tag release: `git tag v1.0.0`
5. Push with tags: `git push --tags`
6. Create GitHub release with notes
7. Submit to PlatformIO Library Registry (optional)

## Testing Checklist

Before committing significant changes:

- [ ] All examples compile without warnings
- [ ] basic_wasm example runs on ESP32
- [ ] basic_wasm example runs on ESP32-S3
- [ ] Memory usage is reasonable (<200KB for basic example)
- [ ] No memory leaks (test load/unload cycle)
- [ ] Error handling works (test invalid WASM)
- [ ] Documentation updated
- [ ] CLAUDE.md updated if architecture changed

## Useful References

- [WAMR GitHub](https://github.com/bytecodealliance/wasm-micro-runtime)
- [WAMR Documentation](https://wamr.gitbook.io/)
- [Arduino-ESP32](https://github.com/espressif/arduino-esp32)
- [PlatformIO Library Docs](https://docs.platformio.org/en/latest/librarymanager/)
- [WebAssembly Spec](https://webassembly.github.io/spec/)

## Key Files for Future Modifications

| Task | Files to Modify |
|------|-----------------|
| Add WAMR feature | `build_config.h`, `collect_sources.py` |
| Change API | `WAMR.h`, `WAMR.cpp`, `API_REFERENCE.md` |
| Add example | `examples/*/`, `library.json`, `README.md` |
| Fix platform issue | `WAMR.cpp`, platform-specific sources |
| Update WAMR | Run `collect_sources.py` |
| Add architecture | `build_config.h`, architecture-specific assembly |

## Code Style

- Follow existing Arduino-style naming (camelCase for methods)
- Use C++11 features (Arduino-ESP32 supports it)
- Keep Serial debugging optional but helpful
- Comment non-obvious WAMR API usage
- Provide examples for new functionality
