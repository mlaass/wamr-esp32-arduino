/*
 * Example using native functions
 *
 * Compile:
 *   clang --target=wasm32 -nostdlib -Wl,--no-entry \
 *         -Wl,--export=setup -Wl,--export=loop \
 *         -Wl,--allow-undefined -O3 -o native_calls.wasm native_calls.c
 *
 * Native functions must be registered in Arduino code before loading.
 */

// Declare native functions (provided by Arduino)
extern void pinMode(int pin, int mode);
extern void digitalWrite(int pin, int value);
extern void delay(int ms);
extern int analogRead(int pin);
extern void print(int value);

// Constants matching Arduino
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#define LED_PIN 2

void setup() {
    pinMode(LED_PIN, OUTPUT);
    print(1);  // Signal setup complete
}

void loop() {
    // Blink LED
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
}

void sensor_read() {
    int value = analogRead(34);  // Read from ADC pin
    print(value);
}
