#include <FsmOS.h>

using namespace FsmOS;

// Helper function to print memory size in human-readable format
void print_size(const __FlashStringHelper* label, uint32_t size) {
  Serial.print(label);
  if (size < 1024) {
    Serial.print(size);
    Serial.println(F(" bytes"));
  } else if (size < 1024 * 1024) {
    Serial.print(size / 1024);
    Serial.print('.');
    Serial.print((size % 1024) * 10 / 1024);
    Serial.println(F(" KB"));
  } else {
    Serial.print(size / (1024 * 1024));
    Serial.print('.');
    Serial.print((size % (1024 * 1024)) * 10 / (1024 * 1024));
    Serial.println(F(" MB"));
  }
}

// Task that monitors and reports memory usage
class MemoryMonitorTask : public Task {
public:
  MemoryMonitorTask() : Task(F("MemMon")) {
    set_period(5000);  // Report every 5 seconds
  }

protected:
  void step() override {
    SystemMemoryInfo sys_info;
    OS.get_system_memory_info(sys_info);
    
    Serial.println(F("\nSystem Memory Status:"));
    Serial.println(F("==================="));
    
    // RAM Usage
    Serial.println(F("\nRAM:"));
    print_size(F("  Total: "), sys_info.total_ram);
    print_size(F("  Free:  "), sys_info.free_ram);
    print_size(F("  Used:  "), sys_info.total_ram - sys_info.free_ram);
    
    // Heap Status
    Serial.println(F("\nHeap:"));
    print_size(F("  Size:        "), sys_info.heap_size);
    print_size(F("  Largest Free: "), sys_info.largest_block);
    Serial.print(F("  Fragments:   "));
    Serial.println(sys_info.heap_fragments);
    Serial.print(F("  Fragmentation: "));
    Serial.print(OS.get_heap_fragmentation());
    Serial.println(F("%"));
    
    // Stack Usage
    Serial.println(F("\nStack:"));
    print_size(F("  Size:  "), sys_info.stack_size);
    print_size(F("  Used:  "), sys_info.stack_used);
    print_size(F("  Free:  "), sys_info.stack_free);
    
    // Task Memory
    Serial.println(F("\nTasks:"));
    Serial.print(F("  Count:  "));
    Serial.println(sys_info.total_tasks);
    print_size(F("  Memory: "), sys_info.task_memory);
    
    // Message System
    Serial.println(F("\nMessages:"));
    Serial.print(F("  Active: "));
    Serial.println(sys_info.active_messages);
    print_size(F("  Memory: "), sys_info.message_memory);
    
    // Flash Usage
    Serial.println(F("\nProgram Memory:"));
    print_size(F("  Used:  "), sys_info.flash_used);
    print_size(F("  Free:  "), sys_info.flash_free);
    
    Serial.println(F("\nTask Details:"));
    Serial.println(F("============"));
    
    // Print info for each task
    for (uint8_t i = 0; i < OS.get_task_count(); i++) {
      Task* task = OS.get_task(i);
      if (task) {
        TaskMemoryInfo task_info;
        OS.get_task_memory_info(i, task_info);
        
        Serial.print(F("\nTask '"));
        Serial.print(task->get_name());
        Serial.println(F("':"));
        print_size(F("  Structure:    "), task_info.task_struct_size);
        print_size(F("  Subscriptions: "), task_info.subscription_size);
        print_size(F("  Queue:        "), task_info.queue_size);
        print_size(F("  Total:        "), task_info.total_allocated);
      }
    }
    
    Serial.println(F("\n"));
  }
};

// Task that allocates and frees memory to demonstrate monitoring
class MemoryStressTask : public Task {
  uint8_t phase;
  uint8_t* buffer;
  size_t buffer_size;
  
public:
  MemoryStressTask() : Task(F("MemStress")), phase(0), buffer(nullptr), buffer_size(0) {
    set_period(10000);  // Change allocation every 10 seconds
  }
  
  ~MemoryStressTask() {
    delete[] buffer;
  }

protected:
  void step() override {
    // Free old buffer if it exists
    if (buffer) {
      delete[] buffer;
      buffer = nullptr;
    }
    
    // Allocate new buffer with different size
    switch (phase) {
      case 0:
        buffer_size = 100;
        break;
      case 1:
        buffer_size = 200;
        break;
      case 2:
        buffer_size = 50;
        break;
      case 3:
        buffer_size = 300;
        break;
      default:
        buffer_size = 0;
        break;
    }
    
    if (buffer_size > 0) {
      buffer = new uint8_t[buffer_size];
      if (buffer) {
        // Fill buffer with pattern
        for (size_t i = 0; i < buffer_size; i++) {
          buffer[i] = i & 0xFF;
        }
      }
    }
    
    phase = (phase + 1) % 5;
  }
};

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println(F("Memory Monitoring Demo"));
  Serial.println(F("====================="));
  
  // Add monitoring task
  OS.add(new MemoryMonitorTask());
  
  // Add stress test task
  OS.add(new MemoryStressTask());
}

void loop() {
  OS.loop_once();
}