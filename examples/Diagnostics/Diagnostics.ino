/*
  Diagnostics - System Monitoring and Debug Example
  
  This example demonstrates FsmOS's diagnostic and debugging capabilities including:
  1. Task CPU usage monitoring and profiling
  2. Stack space monitoring
  3. Watchdog timer integration for crash recovery
  4. Task identification after system resets
  
  The example creates three tasks:
  - StatsDisplayTask: Periodically prints system diagnostics
  - HeavyWorkTask: Simulates varying workloads to show CPU profiling
  - RogueTask: Can be triggered to hang, demonstrating watchdog recovery
  
  To test the watchdog recovery:
  1. Upload the sketch and open Serial Monitor at 115200 baud
  2. Wait for the diagnostics to show normal operation
  3. Send 't' through Serial Monitor to trigger RogueTask
  4. The system will hang and then reset via watchdog
  5. After reset, it will show which task caused the crash
  
  The circuit:
  - No additional hardware required
  - Serial Monitor should be set to 115200 baud
  
  Created October 2, 2025
  By Aykut Ozdemir
  
  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/**
 * StatsDisplayTask: Monitors and displays system diagnostics
 * 
 * This task demonstrates:
 * - How to access task statistics
 * - Stack usage monitoring
 * - How to identify and query tasks by ID
 */
class StatsDisplayTask : public Task {
public:
  StatsDisplayTask() : Task(F("Stats")) {
    setPeriod(2000);  // Print stats every 2 seconds
  }

  void step() override {
    Serial.println(F("\n=== System Diagnostics ==="));

    // 1. Memory Statistics
    print_memory_stats();

    // 2. Task Statistics
    print_task_stats();

    Serial.println(F("=========================\n"));
  }

private:
  void print_memory_stats() {
    Serial.println(F("Memory Statistics:"));
    
    // Stack monitoring
#if defined(__AVR__)
    Serial.print(F("  Free Stack: "));
    Serial.print(OS.getFreeMemory());
    Serial.println(F(" bytes"));
    
    // Get detailed system memory info
    SystemMemoryInfo mem_info;
    if (OS.getSystemMemoryInfo(mem_info)) {
      Serial.print(F("  Heap Free: "));
      Serial.print(mem_info.freeRam);
      Serial.print(F(" bytes ("));
      Serial.print(mem_info.heapFragments);
      Serial.println(F(" fragments)"));
      
      Serial.print(F("  Largest Block: "));
      Serial.print(mem_info.largestBlock);
      Serial.println(F(" bytes"));
    }
#else
    Serial.println(F("  Memory monitoring not supported on this board"));
#endif
  }

  void print_task_stats() {
    Serial.println(F("Task Statistics:"));
    
    for (uint8_t i = 0; i < OS.getTaskCount(); i++) {
      Task* t = OS.getTask(i);
      if (!t) continue;  // Skip if task doesn't exist
      
      TaskStats stats;
      TaskMemoryInfo mem_info;
      
      Serial.print(F("  Task "));
      Serial.print(F("Task"));  // Use task name instead of just ID
      Serial.print(F(" (#"));
      Serial.print(i);
      Serial.println(F("):"));
      
      // Get and print CPU stats
      if (OS.getTaskStats(i, stats)) {
        Serial.print(F("    CPU: Max="));
        Serial.print(stats.maxExecTimeUs);
        Serial.print(F("µs, Avg="));
        Serial.print(stats.runCount > 0 ? stats.totalExecTimeUs / stats.runCount : 0);
        Serial.print(F("µs, Runs="));
        Serial.println(stats.runCount);
      }
      
      // Get and print memory stats
      if (OS.getTaskMemoryInfo(i, mem_info)) {
        Serial.print(F("    Mem: Task="));
        Serial.print(mem_info.task_struct_size);
        Serial.print(F("B, Queue="));
        Serial.print(mem_info.queue_size);
        Serial.print(F("B, Total="));
        Serial.print(mem_info.total_allocated);
        Serial.println(F("B"));
      }
    }
  }
};

/**
 * HeavyWorkTask: Simulates varying CPU workloads
 * 
 * This task demonstrates:
 * - How task execution time affects system performance
 * - How the profiler tracks varying CPU usage
 * - The impact of task timing on overall system behavior
 */
class HeavyWorkTask : public Task {
  int work_load = 100;  // Starting workload in microseconds
  const int MIN_LOAD = 100;
  const int MAX_LOAD = 1000;
  const int LOAD_STEP = 100;

public:
  HeavyWorkTask() : Task(F("HeavyWork")) {
    setPeriod(500);  // Run every 500ms
  }

  void on_start() override {
    logInfof(F("Heavy work task started with %d-%dµs variable load"),
             MIN_LOAD, MAX_LOAD);
  }

  void step() override {
    // Log current workload for monitoring
    logDebugf(F("Processing load: %dµs"), work_load);
    
    // Simulate CPU-intensive work
    delayMicroseconds(work_load);

    // Increase work_load for next iteration
    work_load += LOAD_STEP;
    if (work_load > MAX_LOAD) {
      work_load = MIN_LOAD;
      logInfo(F("Work cycle complete, resetting load"));
    }
  }
};

/**
 * RogueTask: Demonstrates watchdog timer recovery
 * 
 * This task can be triggered to enter an infinite loop, which will:
 * 1. Cause the watchdog timer to trigger
 * 2. Force a system reset
 * 3. Allow the system to identify which task caused the crash
 * 
 * This demonstrates:
 * - How the watchdog protects against hung tasks
 * - How to identify problematic tasks after a crash
 * - The system's automatic recovery capabilities
 */
class RogueTask : public Task {
public:
  static const uint8_t MSG_TRIGGER_HANG = 1;  // Message type to trigger hang

  RogueTask() : Task(F("Rogue")) {
    setPeriod(1000);  // Check messages every second
  }

  void on_start() override {
    logInfo(F("Rogue task started - send 't' to trigger watchdog test"));
  }
  
  void on_msg(const MsgData& m) override {
    if (m.type == MSG_TRIGGER_HANG) {
      logWarn(F("Received command to hang - entering infinite loop"));
      logWarn(F("System should reset via watchdog in ~2 seconds"));
      
      delay(100);  // Allow logs to be printed
      
      // Enter infinite loop - this will trigger the watchdog
      while(1) {
        // The watchdog will eventually reset the system
        // The OS will record this task's ID as the culprit
      }
    }
  }

  void step() override {
    // This task only responds to messages
  }
};

/* ================== Application Setup ================== */

// Create task instances
StatsDisplayTask stats_task;
HeavyWorkTask work_task;
RogueTask rogue_task;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) { /* Wait for serial port */ }
  
  Serial.println(F("\n=== FsmOS Diagnostics Example ==="));
  Serial.println(F("Version: 1.2.0"));

  // Check if we're recovering from a watchdog reset
#if defined(__AVR__)
  ResetInfo reset_info;
  if (OS.getResetInfo(reset_info)) {
    Serial.println(F("\n!!! WATCHDOG RESET DETECTED !!!"));
    Serial.println(F("System recovered from a crash"));
    Serial.print(F("Last active task: "));
    
    // Get the name of the task that caused the crash
    Task* crashed_task = OS.getTask(reset_info.lastTaskId);
    if (crashed_task) {
      Serial.print(F("Task"));
      Serial.print(F(" (#"));
      Serial.print(reset_info.lastTaskId);
      Serial.println(F(")"));
    } else {
      Serial.print(F("#"));
      Serial.println(reset_info.lastTaskId);
    }
    
    Serial.print(F("Reset reason: 0x"));
    Serial.println(reset_info.resetReason, HEX);
    Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"));
  }
#endif

  // Enable diagnostic features
  // OS.enableStackMonitoring();  // Not available in current API
  OS.enableWatchdog(WDTO_2S);

  // Initialize the OS with logging
  OS.begin(); // With logger);

  // Add tasks to the scheduler
  OS.add(&stats_task);
  OS.add(&work_task);
  OS.add(&rogue_task);

  // Print instructions
  Serial.println(F("\nDiagnostics Example Running"));
  Serial.println(F("---------------------------"));
  Serial.println(F("This example demonstrates:"));
  Serial.println(F("1. Task CPU profiling"));
  Serial.println(F("2. Memory monitoring"));
  Serial.println(F("3. Watchdog protection"));
  Serial.println(F("\nCommands:"));
  Serial.println(F("- Send 't' to trigger watchdog test"));
  Serial.println(F("---------------------------\n"));
}

void loop() {
  // Run the scheduler
  OS.loopOnce();

  // Check for user commands
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 't') {
      Serial.println(F("\n>>> Triggering watchdog test <<<"));
      OS.sendMessage(0, RogueTask::MSG_TRIGGER_HANG);
    }
  }
}