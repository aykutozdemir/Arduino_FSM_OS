/*
  Logger - Advanced Logging and Task Monitoring Example
  
  This example demonstrates FsmOS's built-in logging system:
  1. Different log levels (DEBUG, INFO, WARNING, ERROR)
  2. Task context in log messages
  3. Automatic timestamps and formatting
  4. Memory-efficient logging with PROGMEM strings
  5. System event logging
  
  The example creates two tasks:
  - WorkerTask: Generates various types of log messages
  - ControlTask: Manages worker lifecycle and logs state changes
  
  The circuit:
  - No additional hardware required
  - Serial Monitor should be set to 115200 baud
  
  Log Format:
  [time][level][task] message
  
  Where:
  - time: milliseconds since start
  - level: D=Debug, I=Info, W=Warning, E=Error
  - task: name of the task that generated the message
  
  Created October 2, 2025
  By Aykut Ozdemir
  
  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/**
 * WorkerTask: Demonstrates various logging features
 * 
 * This task shows:
 * - Different log levels and when to use them
 * - Lifecycle event logging
 * - Condition monitoring and alerts
 * - Performance tracking
 */
class WorkerTask : public Task {
  uint8_t counter;           // Used to cycle through message types
  Timer delay_timer;         // Simulates a long operation
  Timer perf_timer;         // For performance measurements
  const uint32_t OPERATION_TIMEOUT = 10000;  // 10 second timeout
  uint32_t total_operations;  // Track number of operations
  uint32_t failed_operations; // Track failures
  
public:
  WorkerTask() 
    : Task(F("Worker")), 
      counter(0),
      total_operations(0),
      failed_operations(0)
  {
    set_period(2000);  // Run every 2 seconds
  }

  void on_start() override {
    log_info(F("Worker task initialized"));
    log_info(F("Operation timeout: %dms"), OPERATION_TIMEOUT);
    delay_timer.start(OPERATION_TIMEOUT);
  }

protected:
  void step() override {
    counter++;
    total_operations++;
    
    // Start timing this operation
    perf_timer.start(0);
    
    // Simulate different scenarios with logging
    switch (counter % 4) {
      case 0:
        // Debug: Detailed operation information
        log_debug(F("Operation #%d starting..."), total_operations);
        log_debug(F("Memory available: %d bytes"), OS.get_free_memory());
        break;
        
      case 1:
        // Info: Normal operation completion
        log_info(F("Operation #%d complete"), total_operations);
        log_info(F("Processing time: %dms"), OS.now() - perf_timer.start_ms);
        break;
        
      case 2:
        // Warning: Potential issues
        if (OS.get_free_memory() < 1024) {
          log_warn(F("Low memory warning: %d bytes free"), OS.get_free_memory());
        }
        if (perf_timer.expired()) {
          log_warn(F("Operation taking longer than usual"));
        }
        break;
        
      case 3:
        // Error: Operation failures
        failed_operations++;
        log_error(F("Operation #%d failed"), total_operations);
        log_error(F("Failure rate: %.1f%%"), 
                 (failed_operations * 100.0f) / total_operations);
        break;
    }
    
    // Monitor long-running operations
    if (delay_timer.expired()) {
      log_warn(F("Operation timeout - possible deadlock"));
      log_warn(F("Total ops: %d, Failed: %d"), 
               total_operations, failed_operations);
      delay_timer.start(OPERATION_TIMEOUT);
    }
  }
  
  void on_suspend() override {
    log_info(F("Worker suspending - ops in progress: %d"), total_operations);
  }
  
  void on_resume() override {
    log_info(F("Worker resuming - last state: counter=%d"), counter);
  }
  
  void on_terminate() override {
    // Log final statistics
    log_info(F("Worker terminating - Final Statistics:"));
    log_info(F("Total operations: %d"), total_operations);
    log_info(F("Failed operations: %d (%.1f%%)"), 
             failed_operations,
             (failed_operations * 100.0f) / total_operations);
    Task::on_terminate();
  }
};

/**
 * ControlTask: Manages worker lifecycle and demonstrates system logging
 * 
 * This task shows:
 * - System event logging
 * - Task state transition logging
 * - Error handling and recovery
 * - Clean shutdown procedures
 */
class ControlTask : public Task {
  WorkerTask* worker;     // The worker task we're managing
  uint8_t phase;         // Current demo phase
  Timer phase_timer;     // Controls phase timing
  
public:
  ControlTask() 
    : Task(F("Control")), 
      phase(0) 
  {
    worker = new WorkerTask();
    set_period(8000);  // Change phase every 8 seconds
  }

  void on_start() override {
    log_info(F("Control task starting - Demo Sequence:"));
    log_info(F("Phase 0: Start worker"));
    log_info(F("Phase 1: Suspend worker"));
    log_info(F("Phase 2: Resume worker"));
    log_info(F("Phase 3: Clean shutdown"));
    phase_timer.start(get_period());
  }

protected:
  void step() override {
    log_info(F("\n=== Phase %d Starting ==="), phase);
    
    switch(phase) {
      case 0:
        // Initialize worker
        log_info(F("Initializing worker task..."));
        if (!OS.add(worker)) {
          log_error(F("Failed to add worker task!"));
          this->terminate();
          return;
        }
        break;
        
      case 1:
        // Suspend worker
        log_info(F("Suspending worker task..."));
        worker->suspend();
        // Check task state
        log_debug(F("Worker state: %s"), 
                 worker->is_active() ? "ACTIVE" : "SUSPENDED");
        break;
        
      case 2:
        // Resume worker
        log_info(F("Resuming worker task..."));
        worker->activate();
        // Verify task is running
        log_debug(F("Worker state: %s"), 
                 worker->is_active() ? "ACTIVE" : "SUSPENDED");
        break;
        
      case 3:
        // Clean shutdown
        log_info(F("Initiating shutdown sequence..."));
        
        // Stop worker first
        log_debug(F("Terminating worker task..."));
        worker->terminate();
        
        // Stop self
        log_info(F("All tasks terminated"));
        log_info(F("Demo complete - check log output above"));
        this->terminate();
        break;
    }
    
    phase++;
    phase_timer.start(get_period());
  }
  
  void on_terminate() override {
    log_info(F("Control task terminated"));
    Task::on_terminate();
  }
};

/* ================== Application Setup ================== */

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) { /* wait for serial port */ }
  
  Serial.println(F("\n=== FsmOS Logger Demo ==="));
  Serial.println(F("Version: 1.2.0"));
  
  // Print demo information
  Serial.println(F("\nThis demo shows:"));
  Serial.println(F("1. Log Levels"));
  Serial.println(F("   DEBUG   - Detailed debugging information"));
  Serial.println(F("   INFO    - General operational messages"));
  Serial.println(F("   WARNING - Potential issues or concerns"));
  Serial.println(F("   ERROR   - Critical problems and failures"));
  
  Serial.println(F("\n2. Log Format"));
  Serial.println(F("   [time][level][task] message"));
  Serial.println(F("   Example: [1234][I][Worker] Task started"));
  
  Serial.println(F("\n3. Features"));
  Serial.println(F("   - Automatic timestamps"));
  Serial.println(F("   - Task context tracking"));
  Serial.println(F("   - Memory-efficient logging"));
  Serial.println(F("   - System event capture"));
  
  // Initialize the OS with logging enabled
  OS.begin_with_logger();
  
  // Create and add the control task
  OS.add(new ControlTask());
  
  Serial.println(F("\nDemo starting...\n"));
}

void loop() {
  OS.loop_once();  // Run the cooperative scheduler
}