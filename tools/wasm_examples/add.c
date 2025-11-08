/*
 * Simple addition function example
 *
 * Compile:
 *   clang --target=wasm32 -nostdlib -Wl,--no-entry \
 *         -Wl,--export=add -O3 -o add.wasm add.c
 *
 * Convert to C array:
 *   xxd -i add.wasm > add_wasm.h
 */

int add(int a, int b) {
    return a + b;
}
