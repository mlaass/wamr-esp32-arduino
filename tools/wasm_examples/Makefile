# Makefile for compiling WASM examples
#
# Requirements:
#   - WASI SDK installed or clang with wasm32 target
#   - xxd tool for converting to C arrays
#
# Usage:
#   make           # Build all examples
#   make clean     # Remove built files
#   make headers   # Generate C header files

# Compiler setup
WASI_SDK ?= /opt/wasi-sdk
CC = $(WASI_SDK)/bin/clang

# Compilation flags
CFLAGS = --target=wasm32 \
         -nostdlib \
         -Wl,--no-entry \
         -O3 \
         -Wl,--strip-all

# Source files
SOURCES = add.c math.c native_calls.c

# Generated files
WASM_FILES = $(SOURCES:.c=.wasm)
HEADER_FILES = $(SOURCES:.c=_wasm.h)

.PHONY: all clean headers

all: $(WASM_FILES)

headers: $(HEADER_FILES)

# Compile add.c
add.wasm: add.c
	$(CC) $(CFLAGS) \
		-Wl,--export=add \
		-o $@ $<

# Compile math.c
math.wasm: math.c
	$(CC) $(CFLAGS) \
		-Wl,--export=add \
		-Wl,--export=subtract \
		-Wl,--export=multiply \
		-Wl,--export=divide \
		-Wl,--export=fibonacci \
		-o $@ $<

# Compile native_calls.c
native_calls.wasm: native_calls.c
	$(CC) $(CFLAGS) \
		-Wl,--export=setup \
		-Wl,--export=loop \
		-Wl,--export=sensor_read \
		-Wl,--allow-undefined \
		-o $@ $<

# Convert WASM to C header
%_wasm.h: %.wasm
	xxd -i $< > $@

clean:
	rm -f $(WASM_FILES) $(HEADER_FILES)

# Show targets
help:
	@echo "Available targets:"
	@echo "  all      - Build all WASM files"
	@echo "  headers  - Generate C header files"
	@echo "  clean    - Remove generated files"
	@echo ""
	@echo "Generated files:"
	@echo "  WASM:    $(WASM_FILES)"
	@echo "  Headers: $(HEADER_FILES)"
