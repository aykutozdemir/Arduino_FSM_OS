#include <FsmOS.h>



// Message types
enum {
  MSG_MEMORY_STATUS = 1,
  MSG_CREATE_TASK,
  MSG_CLEANUP
};

// Monitor available memory and system state
class MemoryMonitorTask : public Task {
  const uint32_t report_interval;
  Timer16 reportTimer;  // 5000ms report interval - 4 bytes
  uint16_t min_free_memory;
  
public:
  MemoryMonitorTask(uint32_t interval_ms = 5000) 
    : report_interval(interval_ms), min_free_memory(0xFFFF) {
    setPeriod(100);  // Check frequently but report less often
  }

protected:
  void step() override {
    uint16_t free_mem = getFreeMemory();
    if (free_mem < min_free_memory) {
      min_free_memory = free_mem;
    }
    
    if (reportTimer.isExpired()) {
      reportTimer.startTimer(report_interval);
      
      Serial.println(F("Memory Status:"));
      Serial.print(F("Current Free: ")); Serial.println(free_mem);
      Serial.print(F("Minimum Free: ")); Serial.println(min_free_memory);
      Serial.print(F("Active Tasks: ")); Serial.println(OS.getTaskCount());
      Serial.println();
      
      // Notify other tasks about memory status
      publish(1, MSG_MEMORY_STATUS, free_mem);
    }
  }
  
private:
  // Get available RAM
  uint16_t getFreeMemory() {
    extern int __heap_start, *__brkval;
    int v;
    return (uint16_t)&v - (__brkval == 0 ? (uint16_t)&__heap_start : (uint16_t)__brkval);
  }
};

// Dynamic task that allocates memory and sends messages
class WorkerTask : public Task {
  const uint8_t id;
  uint8_t* buffer;
  uint16_t buffer_size;
  Timer16 workTimer;  // 5000-15000ms work timing - 4 bytes
  
public:
  WorkerTask(uint8_t worker_id, uint16_t mem_size) 
    : id(worker_id), buffer_size(mem_size) {
    // Allocate memory
    buffer = new uint8_t[buffer_size];
    // Random work duration between 5-15 seconds
    workTimer = Timer16();
    workTimer.startTimer(5000 + random(0, 10000));
    setPeriod(100);
  }
  
  ~WorkerTask() {
    delete[] buffer;
  }

protected:
  void step() override {
    // Simulate some work with the buffer
    for (uint16_t i = 0; i < buffer_size; i++) {
      buffer[i] = random(0, 255);
    }
    
    // Check if work time is up
    if (workTimer.isExpired()) {
      Serial.print(F("Worker ")); 
      Serial.print(id);
      Serial.println(F(" finished, cleaning up"));
      terminate();  // Mark for auto-deletion
    }
  }
  
  void on_msg(const MsgData& msg) override {
    if (msg.type == MSG_MEMORY_STATUS) {
      // React to low memory if needed
      if (msg.arg < 500) {  // Less than 500 bytes free
        Serial.print(F("Worker ")); 
        Serial.print(id);
        Serial.println(F(" detected low memory, terminating"));
        terminate();
      }
    }
  }
};

// Creates worker tasks periodically
class TaskManagerTask : public Task {
  uint8_t next_worker_id;
  
public:
  TaskManagerTask() : next_worker_id(1) {
    setPeriod(3000);  // Create new worker every 3 seconds
    subscribe(1);  // Listen for memory status
  }

protected:
  void step() override {
    // Create new worker with random buffer size (100-500 bytes)
    uint16_t buffer_size = 100 + random(0, 400);
    WorkerTask* worker = new WorkerTask(next_worker_id++, buffer_size);
    
    if (OS.add(worker)) {
      Serial.print(F("Created Worker with buffer size "));
      Serial.println(buffer_size);
    } else {
      delete worker;
      Serial.println(F("Failed to create worker, system limit reached"));
    }
  }
  
  void on_msg(const MsgData& msg) override {
    if (msg.type == MSG_MEMORY_STATUS) {
      // Stop creating new tasks if memory is low
      if (msg.arg < 1000) {  // Less than 1KB free
        setPeriod(10000);  // Slow down task creation
      } else {
        setPeriod(3000);   // Resume normal rate
      }
    }
  }
};

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial connection
  }
  
  Serial.println(F("Memory Optimization Demo"));
  Serial.println(F("----------------------"));
  Serial.println(F("This demo shows:"));
  Serial.println(F("1. Dynamic memory allocation"));
  Serial.println(F("2. Automatic task cleanup"));
  Serial.println(F("3. Memory monitoring"));
  Serial.println(F("4. Resource management"));
  
  randomSeed(analogRead(0));
  
  // Add tasks
  OS.add(new MemoryMonitorTask());
  OS.add(new TaskManagerTask());
}

void loop() {
  OS.loopOnce();
}