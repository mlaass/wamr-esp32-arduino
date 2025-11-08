/*
 * Math functions example
 *
 * Compile:
 *   clang --target=wasm32 -nostdlib -Wl,--no-entry \
 *         -Wl,--export=add -Wl,--export=subtract \
 *         -Wl,--export=multiply -Wl,--export=divide \
 *         -Wl,--export=fibonacci -O3 -o math.wasm math.c
 */

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

int divide(int a, int b) {
    if (b == 0) return 0;  // Avoid division by zero
    return a / b;
}

// Recursive fibonacci
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}
