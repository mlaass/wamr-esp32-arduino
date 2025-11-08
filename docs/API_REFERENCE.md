# WAMR-ESP32-Arduino API Reference

Complete API documentation for the WAMR ESP32 Arduino library.

## Table of Contents

- [WamrRuntime Class](#wamrruntime-class)
- [WamrModule Class](#wamrmodule-class)
- [Constants](#constants)
- [Native Function Registration](#native-function-registration)

## WamrRuntime Class

Global runtime management class. All methods are static.

### `WamrRuntime::begin()`

Initialize the WAMR runtime. Must be called before any other WAMR operations.

```cpp
static bool begin(uint32_t heap_pool_size = WAMR_DEFAULT_HEAP_POOL);
```

**Parameters:**
- `heap_pool_size` - Size of global heap pool in bytes (default: 128KB)

**Returns:**
- `true` if initialization successful
- `false` if initialization failed (check `getError()`)

**Example:**
```cpp
if (!WamrRuntime::begin(256 * 1024)) {  // 256KB heap
  Serial.println(WamrRuntime::getError());
}
```

### `WamrRuntime::end()`

Shutdown WAMR runtime and free all resources.

```cpp
static void end();
```

**Example:**
```cpp
WamrRuntime::end();
```

### `WamrRuntime::isInitialized()`

Check if runtime is initialized.

```cpp
static bool isInitialized();
```

**Returns:**
- `true` if runtime is initialized
- `false` otherwise

### `WamrRuntime::getError()`

Get last runtime error message.

```cpp
static const char* getError();
```

**Returns:**
- Error message string or `nullptr` if no error

### `WamrRuntime::printMemoryUsage()`

Print current memory usage to Serial.

```cpp
static void printMemoryUsage();
```

**Output:**
```
=== ESP32 Memory Status ===
Free heap: 234567 bytes
Largest free block: 110592 bytes
Free PSRAM: 4194304 bytes
===========================
```

## WamrModule Class

Represents a loaded WebAssembly module instance.

### Constructor

```cpp
WamrModule();
```

Creates an unloaded module. Call `load()` to load WASM bytecode.

### Destructor

```cpp
~WamrModule();
```

Automatically calls `unload()` to free resources.

### `load()`

Load WASM module from byte array.

```cpp
bool load(const uint8_t* wasm_bytes, uint32_t size,
          uint32_t stack_size = WAMR_DEFAULT_STACK_SIZE,
          uint32_t heap_size = WAMR_DEFAULT_HEAP_SIZE);
```

**Parameters:**
- `wasm_bytes` - Pointer to WASM binary data
- `size` - Size of WASM binary in bytes
- `stack_size` - Stack size for execution (default: 16KB)
- `heap_size` - Heap size for WASM module (default: 64KB)

**Returns:**
- `true` if load successful
- `false` if load failed (check `getError()`)

**Example:**
```cpp
const uint8_t wasm[] = { ... };
WamrModule module;

if (!module.load(wasm, sizeof(wasm), 32*1024, 128*1024)) {
  Serial.println(module.getError());
}
```

### `callFunction()`

Call an exported WASM function by name.

```cpp
bool callFunction(const char* func_name,
                  uint32_t argc = 0,
                  uint32_t* argv = nullptr);
```

**Parameters:**
- `func_name` - Name of exported function
- `argc` - Number of arguments
- `argv` - Array of arguments (input) and results (output)

**Returns:**
- `true` if call successful
- `false` if call failed (check `getError()`)

**Important:** For functions that return values, the result is stored in `argv[0]` after the call.

**Examples:**

No arguments:
```cpp
module.callFunction("init");
```

With arguments:
```cpp
uint32_t args[2] = {42, 58};
if (module.callFunction("add", 2, args)) {
  uint32_t result = args[0];  // Result in first arg
  Serial.printf("Result: %u\n", result);
}
```

### `getResult()`

Get the result of the last function call.

```cpp
uint32_t getResult();
```

**Returns:**
- Last result value (if function returned i32)

**Note:** This is the same as checking `argv[0]` after `callFunction()`.

### `getError()`

Get last module error message.

```cpp
const char* getError();
```

**Returns:**
- Error message string or `nullptr` if no error

### `isLoaded()`

Check if module is loaded.

```cpp
bool isLoaded() const;
```

**Returns:**
- `true` if module is loaded and ready
- `false` otherwise

### `unload()`

Unload module and free all resources.

```cpp
void unload();
```

**Example:**
```cpp
module.unload();
```

### `getInstance()`

Get raw WAMR module instance (for advanced usage).

```cpp
wasm_module_inst_t getInstance();
```

**Returns:**
- WAMR module instance handle or `nullptr`

## Constants

### Heap Sizes

```cpp
#define WAMR_DEFAULT_HEAP_SIZE    (64 * 1024)   // 64KB
#define WAMR_MIN_HEAP_SIZE        (16 * 1024)   // 16KB
#define WAMR_MAX_HEAP_SIZE        (512 * 1024)  // 512KB
```

### Stack Size

```cpp
#define WAMR_DEFAULT_STACK_SIZE   (16 * 1024)   // 16KB
```

### Heap Pool Size

```cpp
#define WAMR_DEFAULT_HEAP_POOL    (128 * 1024)  // 128KB
```

## Native Function Registration

### NativeSymbol Structure

```cpp
typedef struct NativeSymbol {
    const char* symbol;       // Function name
    void* func_ptr;          // Function pointer
    const char* signature;   // Type signature
    void* attachment;        // Optional data
} NativeSymbol;
```

### Signature Format

Function signatures describe parameter and return types:

**Format:** `"(params)results"`

**Types:**
- `i` = i32 (32-bit integer)
- `I` = i64 (64-bit integer)
- `f` = f32 (float)
- `F` = f64 (double)

**Examples:**
- `"()"` - void func(void)
- `"(i)"` - void func(int32)
- `"(ii)"` - void func(int32, int32)
- `"()i"` - int32 func(void)
- `"(i)i"` - int32 func(int32)
- `"(ii)i"` - int32 func(int32, int32)
- `"(f)f"` - float func(float)

### Registering Native Functions

```cpp
static void my_native_func(wasm_exec_env_t exec_env, int32_t arg) {
  Serial.printf("Called with: %d\n", arg);
}

static NativeSymbol natives[] = {
  {"myFunc", (void*)my_native_func, "(i)", nullptr}
};

void setup() {
  WamrRuntime::begin();

  // Register before loading module
  wasm_runtime_register_natives(
    "env",                                  // Module name
    natives,                               // Symbol array
    sizeof(natives) / sizeof(NativeSymbol) // Count
  );

  module.load(...);
}
```

### Native Function Prototype

All native functions must follow this prototype:

```cpp
void native_func(wasm_exec_env_t exec_env, <params...>);
```

or

```cpp
<return_type> native_func(wasm_exec_env_t exec_env, <params...>);
```

The `exec_env` parameter is required but can be ignored if not needed.

## Error Handling

Always check return values and handle errors:

```cpp
// Runtime initialization
if (!WamrRuntime::begin()) {
  Serial.printf("Runtime error: %s\n", WamrRuntime::getError());
  return;
}

// Module loading
if (!module.load(wasm, len)) {
  Serial.printf("Load error: %s\n", module.getError());
  return;
}

// Function calls
if (!module.callFunction("func")) {
  Serial.printf("Call error: %s\n", module.getError());
  return;
}
```

## Memory Management

### Automatic Management

Both `WamrRuntime` and `WamrModule` handle cleanup automatically:

```cpp
{
  WamrModule module;
  module.load(...);
  // ... use module ...
}  // Automatically unloaded when going out of scope
```

### Manual Management

Or manage explicitly:

```cpp
WamrModule module;
module.load(...);
// ... use module ...
module.unload();  // Explicitly unload
```

Runtime cleanup:

```cpp
void setup() {
  WamrRuntime::begin();
  // ... use WAMR ...
}

void shutdown() {
  WamrRuntime::end();  // Free all runtime resources
}
```

## Performance Tips

1. **Reuse modules**: Load once, call multiple times
2. **Minimize heap**: Use smallest heap size that works
3. **Use PSRAM**: Enable for ESP32-S3 with PSRAM
4. **Batch calls**: Call functions in batches to reduce overhead
5. **Profile first**: Use memory_test example to find optimal sizes

## Advanced Usage

### Getting Module Instance

For direct WAMR API access:

```cpp
wasm_module_inst_t inst = module.getInstance();

// Use WAMR C API directly
wasm_function_inst_t func =
  wasm_runtime_lookup_function(inst, "my_func");
```

### Custom Memory Allocation

The library automatically tries PSRAM first, then falls back to internal RAM. This is handled in `WamrRuntime::begin()`.

To force internal RAM only, modify `WAMR.cpp`:

```cpp
global_heap_buf = (char*)malloc(heap_pool_size);
```

## See Also

- [Building WASM Modules](BUILDING_WASM.md)
- [Troubleshooting Guide](TROUBLESHOOTING.md)
- [Example Sketches](../examples/)
