# Arduino_FSM_OS

A lightweight, cooperative, finite-state-machine-based operating system for Arduino and other AVR-based microcontrollers. It provides a structured way to manage multiple tasks, communication, and real-time events without the complexity of a full preemptive RTOS.

## Features

*   **Enhanced Task State Management:**
    * Active/Suspended/Inactive states
    * Automatic cleanup of inactive tasks
    * Configurable message queueing during suspension
*   **Memory-Optimized Design:**
    * Linked-list based message queues
    * Dynamic memory management
    * Smart pointer message handling
    * Reference counting for shared resources

*   **Cooperative Multitasking:** Run multiple independent tasks on a single-core MCU.
*   **Periodic & One-Shot Tasks:** Schedule tasks to run at regular intervals or just once.
*   **Finite State Machine (FSM) Oriented:** Design tasks as state machines using the `step()` function.
*   **Inter-Task Communication:**
    *   **Publish/Subscribe:** Broadcast messages to topics for decoupled communication.
    *   **Direct Messaging:** Send messages directly from one task to another.
*   **Software Timers:** Simple millisecond-level timers for managing delays within tasks.
*   **Diagnostics & Profiling:**
    *   Measure CPU execution time for each task.
    *   Monitor free stack space (AVR only).
*   **Watchdog Timer Integration:** Automatically recover from tasks that hang or get stuck in infinite loops.
*   **Dynamic Tasks:** Create and destroy tasks at runtime to manage memory and resources efficiently.
*   **Interrupt-Safe Queues:** Robust message passing even when posting from ISRs.

## Core Concepts

### The Scheduler (`OS`)

The global `OS` object is the heart of the system. It manages the task list, message queues, and system clock. You must initialize it with `OS.begin()` and call `OS.loop_once()` repeatedly in your main `loop()` function.

### Tasks (`Task` class)

A `Task` is an independent unit of work. You create your own tasks by inheriting from the `Task` base class and implementing its virtual methods.

*   `on_start()`: Called once when the task is added to the scheduler. Use it for initialization (e.g., `pinMode`).
*   `step()`: Called by the scheduler. This is where you implement your task's main logic or state machine.
*   `on_msg(const Msg& m)`: Called when the task receives a message.

### Messages (`Msg` struct)

Tasks communicate by sending `Msg` objects. A message contains:

*   `type`: A user-defined integer identifying the event (e.g., `EVT_BUTTON_PRESSED`).
*   `src_id`: The ID of the task that sent the message (set automatically).
*   `topic`: An optional topic for publish/subscribe communication.
*   `arg`: A 16-bit integer for small data payloads.
*   `ptr`: A pointer for larger data payloads.

## Basic Usage (BasicBlink)

This example shows how to create a simple task that blinks the built-in LED every 500ms.

```cpp
#include <FsmOS.h>

class BlinkTask : public Task {
public:
  BlinkTask() {
    // Set the task to run every 500 milliseconds.
    set_period(500);
  }

  void on_start() override {
    pinMode(LED_BUILTIN, OUTPUT);
  }

  void step() override {
    // Toggle the LED state.
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
};

BlinkTask blinker;

void setup() {
  OS.begin();
  OS.add(&blinker);
}

void loop() {
  OS.loop_once();
}
```

## Inter-Task Communication (ButtonLed)

This example demonstrates how a `ButtonTask` can publish events that a `LedTask` subscribes to.

### `ButtonTask.ino`
```cpp
// This task debounces a button and publishes press/release events.
class ButtonTask : public Task {
  // ... (debounce logic) ...
  void step() override {
    // ...
    if (state == LOW) {
      // Button was pressed
      publish(TOPIC_UI, EVT_BUTTON_PRESSED);
    } else {
      // Button was released
      publish(TOPIC_UI, EVT_BUTTON_RELEASED);
    }
    // ...
  }
};
```

### `LedTask.ino`
```cpp
// This task controls an LED and listens for button events.
class LedTask : public Task {
public:
  void on_start() override {
    // ...
    subscribe(TOPIC_UI); // Listen to events on the UI topic
  }

  void on_msg(const Msg& m) override {
    switch (m.type) {
      case EVT_BUTTON_PRESSED:
        is_blinking = !is_blinking;
        // ...
        break;
    }
  }
  // ...
};
```

## Diagnostics & Debugging (Diagnostics)

FsmOS includes powerful tools for debugging and profiling your application.

*   **CPU Usage Profiling:** Find out how much time each task is taking.
*   **Stack Monitoring:** Keep an eye on memory usage to prevent stack overflows.
*   **Watchdog Timer:** Recover from fatal errors and identify the problematic task.

```cpp
#include <FsmOS.h>

void setup() {
  Serial.begin(115200);

  // --- Check for Watchdog Reset ---
#if defined(__AVR__)
  ResetInfo info;
  if (OS.get_reset_info(info)) {
    Serial.println(F("SYSTEM RECOVERED FROM WATCHDOG RESET"));
    Serial.print(F("Last running task ID was: "));
    Serial.println(info.last_task_id);
  }
#endif

  // --- Enable Features ---
  OS.enable_stack_monitoring();
  OS.enable_watchdog(WDTO_2S);

  OS.begin();
  // ... add tasks
}

// A task to print the stats
class StatsDisplayTask : public Task {
public:
  StatsDisplayTask() { set_period(2000); }

  void step() override {
    Serial.print(F("Free Stack: "));
    Serial.println(OS.get_free_stack());

    Serial.println(F("Task CPU Usage:"));
    for (uint8_t i = 0; i < OS.get_task_count(); i++) {
      TaskStats stats;
      if (OS.get_task_stats(i, stats)) {
        Serial.print(F("  Task "));
        Serial.print(i);
        Serial.print(F(": Max Time: "));
        Serial.print(stats.max_exec_time_us);
        Serial.print(F("us, Avg Time: "));
        if (stats.run_count > 0) {
          Serial.print(stats.total_exec_time_us / stats.run_count);
        }
        Serial.println(F("us"));
      }
    }
  }
};
```

## Dynamic Tasks (DynamicTasks)

For more complex applications, you can create and destroy tasks at runtime. This is useful for managing resources like network connections or for tasks that only need to run temporarily.

```cpp
// ProducerTask: Creates new WorkerTasks periodically.
class ProducerTask : public Task {
public:
  ProducerTask() { set_period(5000); }

  void step() override {
    // Create the task dynamically on the heap
    WorkerTask* new_worker = new WorkerTask();

    // Add it to the scheduler.
    uint8_t task_id = OS.add(new_worker);

    if (task_id == 255) {
      // IMPORTANT: delete the object if it couldn't be added.
      delete new_worker;
    }
  }
};

// WorkerTask: A one-shot task that does some work and then stops itself.
class WorkerTask : public Task {
  void step() override {
    // ... do work ...
    if (work_is_done) {
      this->stop(); // Stop myself. Another task can now delete me.
    }
  }
};
```

## API Reference

### `Scheduler OS`

*   `void begin()`: Initializes the scheduler and system timer.
*   `uint8_t add(Task* t)`: Adds a task to the scheduler.
*   `void loop_once()`: Runs one iteration of the scheduler loop.
*   `uint32_t now() const`: Returns the system time in milliseconds.
*   `void enable_watchdog(uint8_t timeout)`: Enables the hardware watchdog.
*   `void enable_stack_monitoring()`: Enables stack usage tracking.
*   `int get_free_stack() const`: Returns the number of free bytes on the stack.
*   `bool get_task_stats(uint8_t task_id, TaskStats& stats) const`: Gets profiling data for a task.
*   `bool get_reset_info(ResetInfo& info)`: Checks if the system was reset by the watchdog.

### `Task`

*   `void set_period(uint16_t period)`: Sets how often the `step()` function is called (in ms).
*   `bool tell(uint8_t dst_task_id, ...)`: Sends a direct message to another task.
*   `bool publish(uint8_t topic, ...)`: Publishes a message to a topic.
*   `void subscribe(uint8_t topic)`: Subscribes the task to a topic.
*   `void stop()`: Deactivates the task. It will no longer be scheduled.
*   `void resume()`: Reactivates a stopped task.
*   `uint8_t get_id() const`: Returns the task's unique ID.

### `Timer`

*   `void start(uint32_t duration_ms)`: Starts the timer for a given duration.
*   `bool expired() const`: Returns `true` if the timer has finished.

## How to Install

1.  Download this repository as a ZIP file.
2.  In the Arduino IDE, go to `Sketch > Include Library > Add .ZIP Library...`
3.  Select the downloaded ZIP file.
4.  You can now find the examples in `File > Examples > Arduino_FSM_OS`.