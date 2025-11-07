# SafeInterrupts Library

Safe interrupt handling utilities for Arduino AVR with nesting support.

## Features

- **Nesting Support**: Properly handles nested interrupt disable/enable calls
- **Safe Macros**: Provides safe `cli()` and `sei()` macros that replace standard AVR macros
- **RAII Support**: ScopedDisable class for automatic interrupt management
- **Thread-Safe**: Uses atomic operations to prevent race conditions
- **Memory Efficient**: Minimal overhead with bit-packed state tracking

## Installation

1. Download or clone this library into your Arduino `libraries` folder
2. Restart Arduino IDE

**Note**: This library is designed for AVR-based Arduino boards (Uno, Nano, Mega, etc.)

## Usage

### Basic Usage with Macros

```cpp
#include <SafeInterrupts.h>

void criticalSection() {
  cli();  // Disable interrupts (safe version)
  
  // Critical code here
  // Interrupts are disabled
  
  sei();  // Enable interrupts (safe version)
}
```

### Nested Critical Sections

```cpp
#include <SafeInterrupts.h>

void functionA() {
  cli();  // Disable interrupts
  
  // Some code...
  functionB();  // This also disables interrupts
  
  sei();  // Re-enable interrupts (only if this was the outermost call)
}

void functionB() {
  cli();  // Nested disable (just increments counter)
  
  // Critical code
  
  sei();  // Nested enable (just decrements counter)
}
```

### RAII-style Scoped Disable

```cpp
#include <SafeInterrupts.h>

void someFunction() {
  {
    SafeInterrupts::ScopedDisable disable;  // Interrupts disabled
    
    // Critical code here
    // Interrupts are automatically re-enabled when leaving scope
    
  }  // Interrupts re-enabled here
}
```

### Static Methods

```cpp
#include <SafeInterrupts.h>

void criticalSection() {
  SafeInterrupts::disable();  // Disable interrupts
  
  // Critical code
  
  SafeInterrupts::enable();  // Enable interrupts
}
```

## API Reference

### Static Methods

- `SafeInterrupts::disable()` - Disable interrupts with nesting support
- `SafeInterrupts::enable()` - Enable interrupts with nesting support

### Macros

- `cli()` - Disable interrupts (replaces AVR cli macro)
- `sei()` - Enable interrupts (replaces AVR sei macro)

### RAII Class

- `SafeInterrupts::ScopedDisable` - Automatically disables interrupts in constructor and enables in destructor

## How Nesting Works

The library tracks the nesting depth of interrupt disable calls:

1. First `disable()` call: Actually disables interrupts and saves SREG
2. Subsequent `disable()` calls: Only increment nesting counter
3. `enable()` calls: Decrement nesting counter
4. Last `enable()` call: Restores SREG and re-enables interrupts

This ensures that interrupts are only re-enabled when all nested critical sections have completed.

## Example: Protecting Shared Data

```cpp
#include <SafeInterrupts.h>

volatile int counter = 0;

void incrementCounter() {
  SafeInterrupts::ScopedDisable disable;
  counter++;  // Safe increment in critical section
}

void ISR_function() {
  incrementCounter();  // Can be called from ISR
}

void loop() {
  incrementCounter();  // Can be called from main code
}
```

## Notes

- This library is AVR-specific and uses AVR-specific registers (SREG)
- The library automatically undefines standard `cli` and `sei` macros to prevent conflicts
- Maximum nesting depth is 127 levels
- Use ScopedDisable for exception-safe interrupt management

## License

This library is provided as-is for use with the FsmOS framework.

## Author

Aykut ÖZDEMİR

