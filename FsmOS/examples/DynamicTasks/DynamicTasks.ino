#include <FsmOS.h>

// Forward declare our tasks
class ProducerTask;
class CleanupTask;

// Pointers to the static tasks, so the CleanupTask can avoid deleting them.
ProducerTask* producer_task_ptr;
CleanupTask* cleanup_task_ptr;


// WorkerTask: A one-shot task that does some work and then stops itself.
// These will be created dynamically at runtime using 'new'.
class WorkerTask : public Task {
  Timer work_timer;
  int step_count = 0;
  const int max_steps = 5;

public:
  void on_start() override {
    Serial.print("Worker task #");
    Serial.print(get_id());
    Serial.println(" started.");
    work_timer.start(200); // Set timer for the first step
  }

  void step() override {
    if (work_timer.expired()) {
      step_count++;
      Serial.print("Worker #");
      Serial.print(get_id());
      Serial.print(" doing work step ");
      Serial.println(step_count);

      if (step_count >= max_steps) {
        Serial.print("Worker #");
        Serial.print(get_id());
        Serial.println(" finished. Stopping.");
        this->stop(); // Stop myself. The CleanupTask will delete me later.
      } else {
        work_timer.start(200); // Restart timer for the next step
      }
    }
  }
};


// ProducerTask: Creates new WorkerTasks periodically.
class ProducerTask : public Task {
public:
  ProducerTask() {
    set_period(5000); // Try to create a new worker every 5 seconds
  }

  void step() override {
    Serial.println("Producer: Trying to create a new worker...");
    
    // Create the task dynamically on the heap
    WorkerTask* new_worker = new WorkerTask();

    // Add it to the scheduler. OS.add() will now find an empty slot.
    uint8_t task_id = OS.add(new_worker);

    if (task_id != 255) {
      Serial.print("Producer: Successfully created Worker #");
      Serial.println(task_id);
    } else {
      Serial.println("Producer: FAILED. No free slots available.");
      delete new_worker; // IMPORTANT: delete the object if it couldn't be added.
    }
  }
};


// CleanupTask: Finds inactive tasks and deletes them to free memory.
class CleanupTask : public Task {
public:
  CleanupTask() {
    set_period(2000); // Check for inactive tasks every 2 seconds
  }

  void step() override {
    // Iterate through the whole task array, as empty slots can be anywhere.
    for (uint8_t i = 0; i < FSMOS_MAX_TASKS; ++i) {
      Task* t = OS.get_task(i);

      // Is there a task in this slot, and is it inactive?
      if (t && !t->is_active()) {
        
        // IMPORTANT: Make sure we don't delete the static tasks!
        // In this example, we know ProducerTask and CleanupTask are static.
        if (t != (Task*)producer_task_ptr && t != (Task*)cleanup_task_ptr) {
          Serial.print("Cleanup: Deleting inactive task #");
          Serial.println(t->get_id());
          
          delete OS.get_task(i);      // 1. Free the memory for the task object.
          // Nullify the pointer in the OS array so the slot can be reused.
	  // This is not ideal, a OS.remove(id) would be better.
	  // We can't set OS.tasks[i] = nullptr directly anymore.
        }
      }
    }
  }
};

// Create instances of the static tasks that manage the dynamic ones.
ProducerTask producer_task;
CleanupTask cleanup_task;

void setup() {
  Serial.begin(9600);
  while(!Serial); // Wait for serial monitor to connect
  Serial.println("--- Dynamic Task Deletion Example ---");

  // Assign the global pointers for the CleanupTask to use for safety checks.
  producer_task_ptr = &producer_task;
  cleanup_task_ptr = &cleanup_task;

  // Initialize the OS with capacity for 8 tasks (to allow for dynamic creation)
  // and a message queue of size 32 (for task coordination)
  OS.begin(8, 32);
  OS.add(producer_task_ptr);
  OS.add(cleanup_task_ptr);
}

void loop() {
  OS.loop_once();
}