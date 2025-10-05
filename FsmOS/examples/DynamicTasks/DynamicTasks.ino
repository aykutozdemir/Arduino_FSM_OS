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
    set_period(50);  // Check timer frequently
  }

  void on_start() override {
    log_info(F("Worker #%d started - %d steps of %dms each"),
             get_id(), MAX_STEPS, STEP_TIME);
    work_timer = create_timer_typed<Timer16>(STEP_TIME);
  }

  void step() override {
    if (work_timer.expired()) {
      work_timer.start(STEP_TIME);
      step_count++;
      log_debug(F("Worker #%d: Step %d/%d"),
                get_id(), step_count, MAX_STEPS);

      // Check if work is complete
      if (step_count >= MAX_STEPS) {
        log_info(F("Worker #%d: Work complete, terminating"),
                 get_id());
        terminate();  // Mark for cleanup
      }
    }
  }
  
  void on_terminate() override {
    log_info(F("Worker #%d: Cleaning up resources"),
             get_id());
    Task::on_terminate();
  }
  
  float get_progress() const {
    return (step_count * 100.0f) / MAX_STEPS;
  }
};


/**
 * ProducerTask: Dynamically creates and manages worker tasks
 * 
 * This task demonstrates:
 * - Dynamic task creation with 'new'
 * - Rate-limited task spawning
 * - Task population control
 * - Task tracking and monitoring
 */
class ProducerTask : public Task {
  Timer16 produce_timer;      // Variable spawn time - 4 bytes
  uint8_t worker_count;       // Number of workers created
  const uint8_t MAX_WORKERS;  // Maximum workers to create
  const uint32_t SPAWN_TIME;  // Time between spawn attempts (ms)

public:
  ProducerTask(uint8_t max_workers = 5, uint32_t spawn_time = 5000)
    : Task(F("Producer")),
      worker_count(0),
      MAX_WORKERS(max_workers),
      SPAWN_TIME(spawn_time)
  {
    set_period(100);  // Check frequently for new worker creation
  }

  void on_start() override {
    log_info(F("Producer starting - Will attempt to create %d workers, one every %dms"),
             MAX_WORKERS, SPAWN_TIME);
    produce_timer = create_timer_typed<Timer16>(SPAWN_TIME);
  }

  void step() override {
    // Check if it's time to try creating a new worker
    if (produce_timer.expired()) {
      produce_timer.start(SPAWN_TIME);
      // Only create if we haven't hit our limit
      if (worker_count < MAX_WORKERS) {
        worker_count++;
        log_info(F("Producer: Attempting to create worker #%d"), worker_count);
        
        // Create and configure new worker task
        auto* worker = new WorkerTask(5, 200);
        if (worker) {
          uint8_t task_id = os.add(worker);
          if (task_id != 255) {
            log_info(F("Producer: Successfully created worker #%d with task ID %d"), 
                     worker_count, task_id);
          } else {
            log_error(F("Producer: Failed to add worker #%d - no free slots"), 
                      worker_count);
            delete worker;  // Clean up if we couldn't add it
            worker_count--;  // Roll back counter
          }
        } else {
          log_error(F("Producer: Failed to allocate worker #%d - out of memory"), 
                    worker_count);
          worker_count--;  // Roll back counter
        }
      } else {
        log_info(F("Producer: Reached maximum worker count (%d)"), MAX_WORKERS);
      }
    }
    
    // Monitor active workers if debugging enabled
    #ifdef DEBUG_WORKERS
    if (os.get_task_count() > 1) { // Account for producer task
      Serial.print(F("Active workers: "));
      Serial.println(os.get_task_count() - 1);
    }
    #endif
  }
  
  bool is_complete() const {
    return worker_count >= MAX_WORKERS;
  }
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
    set_period(100);  // Check frequently for terminated tasks
  }

  void on_start() override {
    log_info(F("Cleanup task starting - Will scan every %dms"), CLEANUP_TIME);
    cleanup_timer = create_timer_typed<Timer16>(CLEANUP_TIME);
  }

  void step() override {
    if (cleanup_timer.expired()) {
      cleanup_timer.start(CLEANUP_TIME);
      uint8_t cleaned_this_cycle = 0;
      
      // Scan all task slots for terminated tasks
      for (uint8_t i = 0; i < FSMOS_MAX_TASKS; ++i) {
        Task* t = os.get_task(i);
        
        // Check for terminated tasks that can be cleaned up
        if (t && !t->is_active()) {
          // Protect static tasks from deletion
          if (t != (Task*)producer_task_ptr && t != (Task*)cleanup_task_ptr) {
            log_info(F("Cleanup: Removing terminated task #%d"), t->get_id());
            
            // Free the task's memory and clean up OS resources
            delete os.get_task(i);  // Free heap memory
            os.remove(i);           // Clear task slot
            
            cleaned_this_cycle++;
            total_cleaned++;
          }
        }
      }
      
      // Log cleanup results if anything was done
      if (cleaned_this_cycle > 0) {
        log_info(F("Cleanup: Removed %d task(s) this cycle, %d total"),
                 cleaned_this_cycle, total_cleaned);
      }
      
      #ifdef DEBUG_CLEANUP
      // Report memory statistics in debug mode
      Serial.print(F("Free memory: "));
      Serial.println(os.get_free_memory());
      Serial.print(F("Task slots used: "));
      Serial.print(os.get_task_count());
      Serial.print(F("/"));
      Serial.println(FSMOS_MAX_TASKS);
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
  
  os.begin(MAX_TASKS, MSG_QUEUE_SIZE);
  
  // Add core system tasks
  uint8_t producer_id = os.add(producer_task_ptr);
  uint8_t cleanup_id = os.add(cleanup_task_ptr);
  
  if (producer_id == 255 || cleanup_id == 255) {
    Serial.println(F("ERROR: Failed to add core system tasks!"));
    while (1) {
      // Critical error - halt system
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  
  Serial.println(F("System initialized successfully"));
  Serial.println(F("-----------------------------------"));
}

void loop() {
  os.loop_once();  // Run a single iteration of the task scheduler
  
  // Optional: Add system monitoring here
  #ifdef MONITOR_SYSTEM
  static Timer16 monitor_timer; // Variable monitoring - 4 bytes
  if (monitor_timer.expired()) {
    Serial.print(F("System uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" seconds"));
    monitor_timer.start(5000);  // Update every 5 seconds
  }
  #endif
}