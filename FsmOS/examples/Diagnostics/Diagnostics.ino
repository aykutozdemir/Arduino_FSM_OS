#include <FsmOS.h>

// This task periodically prints diagnostics to the Serial monitor.
class StatsDisplayTask : public Task {
public:
  StatsDisplayTask() {
    set_period(2000); // Print stats every 2 seconds
  }

  void step() override {
    Serial.println(F("--- System Diagnostics ---"));

    // 1. Print Free Stack
#if defined(__AVR__)
    Serial.print(F("Free Stack: "));
    Serial.print(OS.get_free_stack());
    Serial.println(F(" bytes"));
#else
    Serial.println(F("Stack monitoring not supported on this board."));
#endif

    // 2. Print Task CPU Stats
    Serial.println(F("Task CPU Usage:"));
    for (uint8_t i = 0; i < OS.get_task_count(); i++) {
      Task* t = OS.get_task(i);
      if (t) { // Check if task exists
        TaskStats stats;
        OS.get_task_stats(i, stats);
        
        Serial.print(F("  Task "));
        Serial.print(i);
        Serial.print(F(": Max Time: "));
        Serial.print(stats.max_exec_time_us);
        Serial.print(F("us, Avg Time: "));
        if (stats.run_count > 0) {
          Serial.print(stats.total_exec_time_us / stats.run_count);
        }
        else {
          Serial.print(0);
        }
        Serial.println(F("us"));
      }
    }
    Serial.println(F("------------------------"));
  }
};

// This task simulates doing a variable amount of work to demonstrate the profiler.
class HeavyWorkTask : public Task {
  int work_load = 100;
public:
  HeavyWorkTask() {
    set_period(500); // Run every 500ms
  }

  void step() override {
    // Simulate work by delaying for a variable number of microseconds
    delayMicroseconds(work_load);

    // Increase the work for next time, then wrap around
    work_load += 100;
    if (work_load > 1000) {
      work_load = 100;
    }
  }
};

// This task waits for a message and then enters an infinite loop to test the watchdog.
class RogueTask : public Task {
public:
  RogueTask() {
    set_period(1000); // Check for messages every second
  }
  
  void on_msg(const Msg& m) override {
    if (m.type == 123) { // Special message type to trigger the hang
      Serial.println(F("RogueTask: Received message to hang. Infinite loop starting..."));
      delay(20); // Allow serial to print before hanging
      while(1); // This will trigger the watchdog timer and reset the MCU
    }
  }

  void step() override {
    // This task does nothing in its step, it only acts on messages.
  }
};

// Create instances of our tasks
StatsDisplayTask stats_task;
HeavyWorkTask work_task;
RogueTask rogue_task;

void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(500); // Wait a bit for serial monitor to open

  // --- Check for Watchdog Reset ---
  // This section runs first to see if we are recovering from a crash.
#if defined(__AVR__)
  ResetInfo info;
  if (OS.get_reset_info(info)) {
    Serial.println(F("*********************************"));
    Serial.println(F("SYSTEM RECOVERED FROM WATCHDOG RESET"));
    Serial.print(F("Last running task ID was: "));
    Serial.println(info.last_task_id);
    Serial.println(F("*********************************"));
  }
#endif

  // --- Enable New Features ---
  Serial.println(F("--- FsmOS Diagnostics Example ---"));

  // 1. Enable Stack Monitoring. Call this FIRST in setup().
  OS.enable_stack_monitoring();

  // 2. Enable the Watchdog Timer with a 2-second timeout.
  OS.enable_watchdog(WDTO_2S);
  Serial.println(F("Watchdog enabled (2s timeout)."));

  // Initialize the OS with capacity for 4 tasks and a message queue of size 32
  // (larger queue for diagnostics messages)
  OS.begin(4, 32);

  // Add tasks to the scheduler
  OS.add(&stats_task);
  OS.add(&work_task);
  OS.add(&rogue_task);

  Serial.println(F("\nTo test the watchdog, send 't' via the serial monitor."));
}

void loop() {
  // OS.loop_once() must be called continuously.
  OS.loop_once();

  // Check for user input to trigger the test
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 't') {
      Serial.println(F("Sending message to RogueTask to trigger a hang..."));
      // Tell the rogue task to hang the system. Its ID will be captured by the OS.
      rogue_task.tell(rogue_task.get_id(), 123);
    }
  }
}