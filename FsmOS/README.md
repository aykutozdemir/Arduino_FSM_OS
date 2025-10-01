# FsmOS - Arduino Finite State Machine Operating System

A lightweight, cooperative task scheduler for Arduino that helps you organize your code into independent tasks and manage communication between them. This library is perfect for projects that need to handle multiple operations without blocking or complex interrupt management.

## Key Features

- **Cooperative Multitasking**: Run multiple tasks without preemption
- **Message Passing**: Inter-task communication with publish/subscribe
- **Memory Efficient**: Optimized for AVR microcontrollers
- **Debug Support**: Built-in logging and diagnostics

## Installation

### Arduino IDE Library Manager (Recommended)
1. Open Arduino IDE
2. Go to `Sketch > Include Library > Manage Libraries...`
3. Search for "FsmOS"
4. Click Install

### Manual Installation
1. Download this repository
2. Copy the `FsmOS` folder to your Arduino libraries directory
3. Restart Arduino IDE

## Quick Start

```cpp
#include <FsmOS.h>

// Define a simple blinking task
class BlinkTask : public Task {
public:
  BlinkTask() : Task(F("Blinker")) {
    set_period(500);  // Run every 500ms
  }

  void on_start() override {
    pinMode(LED_BUILTIN, OUTPUT);
    log_info(F("Blink task started"));
  }

  void step() override {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
};

BlinkTask blinker;

void setup() {
  Serial.begin(9600);
  OS.begin_with_logger();  // Initialize with logging
  OS.add(&blinker);
}

void loop() {
  OS.loop_once();
}
```

## Examples

Check the `examples` folder for more demonstrations:
- `BasicBlink`: Simple LED blinking task
- `ButtonLed`: Inter-task communication
- `Diagnostics`: System monitoring and debugging
- `Logger`: Using the built-in logging system
- `MemoryMonitoring`: Memory usage tracking
- `MessageQueueing`: Message handling during task suspension
- `TaskLifecycle`: Task state management
- `DynamicTasks`: Runtime task creation/deletion
- `MemoryOptimization`: Memory-efficient coding practices
- `TaskNames`: Named tasks and state tracking

## Documentation

Full documentation is available in the repository's main [README.md](../README.md).

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License. See the LICENSE file for details.