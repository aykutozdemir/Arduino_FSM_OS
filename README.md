# FsmOS - Arduino Finite State Machine Operating System

A lightweight, cooperative task scheduler for Arduino that helps you organize your code into independent tasks and manage communication between them. This library is perfect for projects that need to handle multiple operations without blocking or complex interrupt management.

## Key Features

- **Cooperative Multitasking**: Run multiple tasks without preemption
- **Message Passing**: Inter-task communication with publish/subscribe (type + arg only)
- **Memory Efficient**: Optimized for AVR microcontrollers with accurate memory reporting
- **Debug Support**: Built-in logging and diagnostics with formatted output
- **Task Budgeting**: Prevent message queue overruns with per-task message budgets
- **Memory Monitoring**: Real-time RAM, stack, heap, flash, and EEPROM usage tracking
- **Stack Canary Protection**: Automatic stack overflow detection
- **Memory Leak Detection**: Built-in memory allocation tracking
- **Task Limit Control**: Configurable maximum task count based on topic bitfield size

## Framework Structure

FsmOS is organized as a framework with the following structure:

```
FsmOS/
├── framework/          # Core FsmOS framework
│   ├── include/       # Public API headers
│   └── src/           # Implementation files
├── libs/              # External integrated libraries
├── examples/          # Example sketches
├── scripts/           # Integration and build scripts
└── docs/              # Documentation
```

### Integrating External Libraries

To integrate an external library from GitHub:

**Using Bash script:**
```bash
./scripts/integrate_library.sh https://github.com/user/library.git [LibraryName]
```

**Using Python script:**
```bash
python3 scripts/integrate_library.py https://github.com/user/library.git [LibraryName]
```

The library will be cloned into the `libs/` directory and can be used alongside FsmOS.

## Installation

### Arduino IDE Library Manager (Recommended)
1. Open Arduino IDE
2. Go to `Sketch > Include Library > Manage Libraries...`
3. Search for "FsmOS"
4. Click Install

### Manual Installation
1. Download this repository
2. Copy the `lib/FsmOS` folder to your Arduino libraries directory
3. Restart Arduino IDE

## Quick Start

```cpp
#include <FsmOS.h>

// Define a simple blinking task
class BlinkTask : public Task
{
public:
    BlinkTask() : Task(F("Blinker"))
    {
        set_period(500);  // Run every 500ms
    }

    uint8_t getMaxMessageBudget() const override { return 0; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

    void on_start() override
    {
        pinMode(LED_BUILTIN, OUTPUT);
        logInfo(F("Blink task started"));
    }

    void step() override
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
};

BlinkTask blinker;

void setup()
{
    Serial.begin(9600);
    OS.begin_with_logger();  // Initialize with logging
    OS.add(&blinker);
}

void loop()
{
    OS.loop_once();
}
```

## Examples

Check the `examples` folder for more demonstrations:
- `BasicBlink`: Simple LED blinking task
- `ButtonLed`: Inter-task communication with publish/subscribe
- `Diagnostics`: System monitoring and debugging
- `Logger`: Using the built-in logging system with different levels
- `MemoryMonitoring`: Comprehensive memory usage tracking
- `MemoryOptimization`: Memory-efficient coding practices
- `MemoryOptimizedTimers`: Timer usage optimization
- `MessageQueueing`: Message handling during task suspension
- `TaskLifecycle`: Task state management
- `DynamicTasks`: Runtime task creation/deletion
- `TaskNames`: Named tasks and state tracking
- `MutexExample`: Mutual exclusion synchronization
- `SemaphoreExample`: Semaphore-based synchronization
- `TaskTimingMonitoring`: Task execution timing analysis

## Key Concepts

### Task Structure
Every task must implement two pure virtual methods:
```cpp
uint8_t getMaxMessageBudget() const override { return X; }  // Max messages per step
uint16_t getTaskStructSize() const override { return sizeof(*this); }  // Memory tracking
```

### Message System
FsmOS uses a simplified message system with only type and argument data:
```cpp
// Publish a message
publish(TOPIC_LED_EVENTS, EVT_LED_ON, 1);  // topic, type, arg

// Handle messages
void on_msg(const MsgData &msg) override
{
    switch (msg.type)
    {
        case EVT_LED_ON:
            digitalWrite(LED_PIN, msg.arg);
            break;
    }
}
```

### Message Budgeting
Tasks declare their maximum message production budget. The scheduler ensures sufficient queue space before execution:
```cpp
// Button task that publishes events
uint8_t getMaxMessageBudget() const override { return 2; }  // Press + Release

// LED task that only receives
uint8_t getMaxMessageBudget() const override { return 1; }  // Minimal
```

### Memory Monitoring
Access comprehensive memory information:
```cpp
SystemMemoryInfo info;
OS.getSystemMemoryInfo(info);
// info.freeRam, info.stackUsed, info.flashUsed, info.eepromUsed, etc.
```

### Formatted Logging
Use memory-efficient formatted logging:
```cpp
logDebugf(F("Value: %d, Status: %s"), value, status);
logInfof(F("Operation %d complete"), operation_id);
logWarnf(F("Low memory: %d bytes"), free_memory);
logErrorf(F("Failed after %d attempts"), attempts);
```

## Configuration Parameters

### Stack Canary Protection
```cpp
#ifndef FSMOS_STACK_CANARY_MARGIN
#define FSMOS_STACK_CANARY_MARGIN 32  // Safety margin in bytes
#endif
```

### Topic Bitfield Size
```cpp
#ifndef TOPIC_BITFIELD_SIZE
#define TOPIC_BITFIELD_SIZE 16  // 8, 16, or 32 topics max
#endif
```

### Message Pool Size
```cpp
#ifndef MAX_MESSAGE_POOL_SIZE
#define MAX_MESSAGE_POOL_SIZE 32  // Maximum messages in pool
#endif
```

### Default Values
```cpp
const uint8_t DEFAULT_TASK_MESSAGE_BUDGET = 1;  // Messages per step
const uint16_t DEFAULT_TASK_PERIOD = 100;       // Default period in ms
```

## Memory Optimization Features

### Stack Canary
- Automatic stack overflow detection
- Configurable safety margin
- Marks entire free RAM region between heap and stack

### Memory Leak Detection
- Tracks all memory allocations and deallocations
- Provides peak usage and current usage statistics
- Always active (no conditional compilation)

### Task Limit Control
- Prevents adding more tasks than `MAX_TOPICS` allows
- Runtime logging and rejection of excess tasks
- Based on `TOPIC_BITFIELD_SIZE` configuration

### Message Data Optimization
- Simplified message structure (type + arg only)
- No dynamic data allocation for messages
- Reduced memory footprint per message

## Platformio Configuration

For optimal performance with Arduino Nano, use these build flags:
```ini
build_flags =
  -Os                    # Size optimization
  -ffunction-sections    # Function sectioning
  -fdata-sections       # Data sectioning
  -fno-exceptions       # Remove exception handling
  -DTOPIC_BITFIELD_SIZE=16  # Topic bitfield size
  -Wl,--gc-sections     # Dead code elimination
  -fno-lto              # Disable LTO
  -DNDEBUG              # Remove debug symbols
  -mmcu=atmega328p      # AVR architecture
  -fno-stack-protector  # Reduce stack usage
  -fpack-struct=1       # Memory alignment
  -DFSMOS_FLASH_SIZE=30720  # Flash size
  -DFSMOS_EEPROM_SIZE=1024  # EEPROM size
```

## Memory Usage

Typical memory usage on Arduino Nano (ATmega328P):
- **RAM**: ~1.3KB (64% of 2KB)
- **Flash**: ~29KB (95% of 30KB)
- **Message Pool**: 32 messages × 5 bytes = 160 bytes
- **Stack Canary**: 32 bytes safety margin

## API Reference

### Core Functions
- `OS.begin()` - Initialize scheduler
- `OS.begin_with_logger()` - Initialize with logging
- `OS.add(task)` - Add task to scheduler
- `OS.loop_once()` - Run one scheduler cycle
- `OS.getTaskCount()` - Get current task count
- `OS.getFreeMemory()` - Get free RAM

### Task Methods
- `set_period(ms)` - Set task period
- `set_priority(level)` - Set task priority
- `publish(topic, type, arg)` - Publish message
- `subscribe(topic)` - Subscribe to topic
- `logInfo(msg)` - Log info message
- `logDebug(msg)` - Log debug message
- `logWarn(msg)` - Log warning message
- `logError(msg)` - Log error message

### Memory Functions
- `OS.getSystemMemoryInfo(info)` - Get comprehensive memory info
- `OS.getMemoryStats()` - Get memory allocation statistics
- `OS.getTaskStats()` - Get task execution statistics

## Troubleshooting

### Common Issues
1. **Task limit reached**: Reduce `TOPIC_BITFIELD_SIZE` or optimize task count
2. **Memory overflow**: Check stack canary warnings and reduce memory usage
3. **Message queue full**: Increase `MAX_MESSAGE_POOL_SIZE` or optimize message budgets
4. **Compilation errors**: Ensure all required methods are implemented

### Debug Commands
Use serial commands for debugging:
- `s` - System statistics
- `mem` - Memory information
- `tl` - Task limit check
- `st` - Task status

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Changelog

### Version 1.3.0
- Removed message data system for memory optimization
- Enhanced stack canary protection
- Added task limit control
- Improved memory leak detection
- Optimized message structure (type + arg only)
- Reduced memory footprint per message by 7 bytes