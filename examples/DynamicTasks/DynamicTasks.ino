/*
  DynamicTasks - Dynamic Task Creation and Cleanup Example
  
  This example demonstrates FsmOS's dynamic task management capabilities:
  1. Dynamic task creation at runtime
  2. Automatic task cleanup and memory management
  3. Task lifecycle monitoring
  4. Safe task deletion and memory reclamation
  
  The example creates three types of tasks:
  - ProducerTask: Creates new worker tasks periodically
  - WorkerTask: Temporary tasks that run for a limited time
  - CleanupTask: Monitors and removes completed worker tasks
  
  Key Concepts Demonstrated:
  - Memory allocation for dynamic tasks
  - Task slot management
  - Safe task deletion
  - Memory leak prevention
  - Task coordination
  
  Created October 2, 2025
  By Aykut Ozdemir
  
  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/* ================== Forward Declarations ================== */
class ProducerTask;
class CleanupTask;

/* ================== Global Task References ================== */
// These pointers help CleanupTask identify which tasks are static
// and should not be deleted
ProducerTask* producer_task_ptr;
CleanupTask* cleanup_task_ptr;


/**
 * WorkerTask: A temporary task that performs a series of steps
 * 
 * This task demonstrates:
 * - Dynamic task creation
 * - Self-termination
 * - Progress tracking
 * - Resource cleanup
 * 
 * Workers are created by the ProducerTask and automatically
 * clean themselves up when their work is complete.
 */
class WorkerTask : public Task {
  Timer16 work_timer;       // 200ms step time - 4 bytes
  uint8_t step_count;       // Current step number
  const uint8_t MAX_STEPS;  // Total steps to perform
  const uint32_t STEP_TIME; // Time per step (ms)
  
public:
  WorkerTask(uint8_t max_steps = 5, uint32_t step_time = 200) 
    : Task(F("Worker")),
      step_count(0),
      MAX_STEPS(max_steps),
      STEP_TIME(step_time)
  {
    setPeriod(50);  // Check timer frequently
  }

  void on_start() override {
    logInfof(F("Worker started - %d steps of %dms each"),
             MAX_STEPS, STEP_TIME);
    work_timer = Timer16();
    work_timer.startTimer(STEP_TIME);
  }

  void step() override {
    if (work_timer.isExpired()) {
      work_timer.startTimer(STEP_TIME);
      step_count++;
      logDebugf(F("Worker: Step %d/%d"),
                step_count, MAX_STEPS);

      // Check if work is complete
      if (step_count >= MAX_STEPS) {
        logInfof(F("Worker: Work complete, terminating"));
        terminate();  // Mark for cleanup
      }
    }
  }
  
  uint8_t getMaxMessageBudget() const override { return 0; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};

/**
 * ProducerTask: Creates new worker tasks periodically
 */
class ProducerTask : public Task {
  Timer16 produce_timer;
  uint8_t worker_count;
  const uint32_t SPAWN_TIME = 2000;  // Create worker every 2 seconds
  const uint8_t MAX_WORKERS = 3;     // Maximum concurrent workers
  
public:
  ProducerTask() 
    : Task(F("Producer")),
      worker_count(0)
  {
    setPeriod(100);  // Check frequently
  }
  
  void on_start() override {
    logInfof(F("Producer starting - Will attempt to create %d workers, one every %dms"), 
             MAX_WORKERS, SPAWN_TIME);
    produce_timer = Timer16();
    produce_timer.startTimer(SPAWN_TIME);
  }
  
  void step() override {
    if (produce_timer.isExpired()) {
      produce_timer.startTimer(SPAWN_TIME);
      
      if (worker_count < MAX_WORKERS) {
        logInfof(F("Producer: Attempting to create worker #%d"), worker_count);
        
        WorkerTask* worker = new WorkerTask(5, 200);
        if (OS.add(worker)) {
          worker_count++;
          logInfof(F("Producer: Successfully created worker"));
        } else {
          logInfof(F("Producer: Failed to add worker - no free slots"));
          delete worker;
        }
      } else {
        logInfof(F("Producer: Reached maximum worker count (%d)"), MAX_WORKERS);
      }
    }
  }
  
  uint8_t getMaxMessageBudget() const override { return 0; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};


/**
 * CleanupTask: Manages task lifecycle and memory reclamation
 * 
 * This task demonstrates:
 * - Safe task deletion
 * - Memory management
 * - Task lifecycle tracking
 * - Protected task handling
 * 
 * The cleanup task periodically scans for terminated tasks and
 * safely removes them from the system, freeing their resources.
 */
class CleanupTask : public Task {
  Timer16 cleanup_timer;        // Variable cleanup time - 4 bytes
  uint32_t total_cleaned;       // Total tasks cleaned up
  const uint32_t CLEANUP_TIME;  // Time between cleanup scans (ms)

public:
  CleanupTask(uint32_t cleanup_time = 2000)
    : Task(F("Cleanup")),
      total_cleaned(0),
      CLEANUP_TIME(cleanup_time)
  {
    setPeriod(100);  // Check frequently for terminated tasks
  }

  void on_start() override {
    logInfof(F("Cleanup task starting - Will scan every %dms"), CLEANUP_TIME);
    cleanup_timer = Timer16();
    cleanup_timer.startTimer(CLEANUP_TIME);
  }

  void step() override {
    if (cleanup_timer.isExpired()) {
      cleanup_timer.startTimer(CLEANUP_TIME);
      uint8_t cleaned_this_cycle = 0;
      
      // Scan all task slots for terminated tasks
      for (uint8_t i = 0; i < OS.getMaxTasks(); ++i) {
        Task* t = OS.getTask(i);
        
        // Check for terminated tasks - use pointer comparison instead of isActive()
        if (t && t != (Task*)producer_task_ptr && t != (Task*)cleanup_task_ptr) {
          // Check if task is terminated by trying to remove it
          if (OS.remove(t)) {
            logInfof(F("Cleanup: Removing terminated task"));
            
            // Free the task's memory
            delete t;
            
            cleaned_this_cycle++;
            total_cleaned++;
          }
        }
      }
      
      // Log cleanup results if anything was done
      if (cleaned_this_cycle > 0) {
        logInfof(F("Cleanup: Removed %d task(s) this cycle, %d total"),
                 cleaned_this_cycle, total_cleaned);
      }
      
      #ifdef DEBUG_CLEANUP
      // Report memory statistics in debug mode
      Serial.print(F("Free memory: "));
      Serial.println(OS.getFreeMemory());
      Serial.print(F("Task slots used: "));
      Serial.print(OS.getTaskCount());
      Serial.print(F("/"));
      Serial.println(MAX_TASKS);
      #endif
    }
  }
  
  uint32_t get_total_cleaned() const {
    return total_cleaned;
  }
};

// Create instances of the static tasks that manage the dynamic ones.
ProducerTask producer_task;
CleanupTask cleanup_task;

// Serial port configuration
const uint32_t SERIAL_BAUD = 9600;
const uint32_t SERIAL_TIMEOUT = 5000;  // Max time to wait for serial (ms)

// OS configuration
const uint8_t MAX_TASKS = 8;           // Maximum number of concurrent tasks
const uint8_t MSG_QUEUE_SIZE = 32;     // Size of message queue for coordination

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  
  // Wait for serial connection (timeout after SERIAL_TIMEOUT ms)
  uint32_t start = millis();
  while (!Serial && (millis() - start < SERIAL_TIMEOUT));
  
  Serial.println(F("=== Dynamic Task Management Example ==="));
  Serial.println(F("Demonstrates task lifecycle management and memory safety"));
  
  // Register static tasks for protection from cleanup
  producer_task_ptr = &producer_task;
  cleanup_task_ptr = &cleanup_task;
  
  // Initialize the operating system
  Serial.print(F("Initializing FsmOS with "));
  Serial.print(MAX_TASKS);
  Serial.print(F(" task slots and "));
  Serial.print(MSG_QUEUE_SIZE);
  Serial.println(F(" message queue size"));
  
  OS.begin();
  
  // Add core system tasks
  OS.add(&producer_task);
  OS.add(&cleanup_task);
  
  Serial.println(F("System initialized successfully"));
  Serial.println(F("-----------------------------------"));
}

void loop() {
  OS.loopOnce();  // Run a single iteration of the task scheduler
  
  // Optional: Add system monitoring here
  #ifdef MONITOR_SYSTEM
  static Timer16 monitor_timer; // Variable monitoring - 4 bytes
  if (monitor_timer.isExpired()) {
    Serial.print(F("System uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" seconds"));
    monitor_timer.startTimer(5000);  // Update every 5 seconds
  }
  #endif
}