#pragma once

/* ================== core types ================== */
#include <Arduino.h>
#include <stdint.h>
#include <util/atomic.h> // For ATOMIC_BLOCK
#include <stdarg.h>

#if defined(__AVR__)
#include <avr/wdt.h> // For watchdog timer
#endif

// Forward declarations
class Task;
class Scheduler;

/* Message/Event for inter-task communication with reference counting */
struct __attribute__((packed)) MsgData {
  uint8_t type;     // User-defined event type
  uint8_t src_id;   // Scheduler-assigned ID of the source task
  uint8_t topic;    // Optional topic for publish/subscribe (0 for direct, 1-255 for topics)
  uint8_t ref_count: 7; // Number of tasks that need to process this message (max 127)
  bool is_dynamic: 1;   // Whether ptr needs to be freed
  uint16_t arg;     // Small payload
  void* ptr;        // Optional pointer to larger data
  uint16_t dynamic_size; // Size of dynamically allocated data
  
  MsgData() : type(0), src_id(0), topic(0), ref_count(0), is_dynamic(0), arg(0), ptr(nullptr) {}
};

// Smart pointer-like class for message reference counting
class SharedMsg {
  MsgData* data;

public:
  SharedMsg() : data(nullptr) {}
  
  explicit SharedMsg(MsgData* msg) : data(msg) {
    if (data) {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        data->ref_count++;
      }
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
      fsmos_memory_stats.allocations++;
      fsmos_memory_stats.current_bytes += sizeof(MsgData);
      if (fsmos_memory_stats.current_bytes > fsmos_memory_stats.peak_bytes) {
        fsmos_memory_stats.peak_bytes = fsmos_memory_stats.current_bytes;
      }
#endif
    }
  }
  
  SharedMsg(const SharedMsg& other) : data(other.data) {
    if (data) {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        data->ref_count++;
      }
    }
  }
  
  SharedMsg& operator=(const SharedMsg& other) {
    if (this != &other) {
      release();
      data = other.data;
      if (data) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
          data->ref_count++;
        }
      }
    }
    return *this;
  }
  
  ~SharedMsg() {
    release();
  }
  
  void release() {
    if (data) {
      bool should_delete = false;
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        data->ref_count--;
        should_delete = (data->ref_count == 0);
      }
      if (should_delete) {
        if (data->is_dynamic && data->ptr) {
          delete[] static_cast<uint8_t*>(data->ptr);
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
          fsmos_memory_stats.deallocations++;
          fsmos_memory_stats.current_bytes -= data->dynamic_size;
#endif
        }
        delete data;
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
        fsmos_memory_stats.deallocations++;
        fsmos_memory_stats.current_bytes -= sizeof(MsgData);
#endif
      }
      data = nullptr;
    }
  }
  
  MsgData* get() const { return data; }
  MsgData* operator->() const { return data; }
  bool valid() const { return data != nullptr; }
};

/*
 * Lightweight, interrupt-safe linked queue.
 */
template<typename T>
class LinkedQueue {
private:
  struct Node {
    T data;
    Node* next;
    Node(const T& value) : data(value), next(nullptr) {}
  };

  Node* head;
  Node* tail;
  volatile uint8_t count;

public:
  LinkedQueue() : head(nullptr), tail(nullptr), count(0) {}

  ~LinkedQueue() {
    while (head) {
      Node* temp = head;
      head = head->next;
      delete temp;
    }
  }

  // Move constructor
  LinkedQueue(LinkedQueue&& other) noexcept 
    : head(other.head), tail(other.tail), count(other.count) {
    other.head = nullptr;
    other.tail = nullptr;
    other.count = 0;
  }

  // Delete copy constructor to prevent double deletion
  LinkedQueue(const LinkedQueue&) = delete;
  LinkedQueue& operator=(const LinkedQueue&) = delete;

  inline bool push(const T& v) {
    Node* new_node = new Node(v);
    if (!new_node) return false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (!head) {
        head = tail = new_node;
      } else {
        tail->next = new_node;
        tail = new_node;
      }
      count++;
    }
    return true;
  }

  inline bool pop(T& out) {
    bool result = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (head) {
        out = head->data;
        Node* temp = head;
        head = head->next;
        if (!head) tail = nullptr;
        delete temp;
        count--;
        result = true;
      }
    }
    return result;
  }

  inline bool empty() const {
    bool is_empty;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      is_empty = (head == nullptr);
    }
    return is_empty;
  }

  inline uint8_t size() const {
    return count;
  }
};

/* ================== Profiling & Reset Info ================== */
struct __attribute__((packed)) TaskStats {
  uint16_t max_exec_time_ms = 0;     // Max execution time in milliseconds (65.5s max)
  uint16_t avg_exec_time_us = 0;     // Average execution time in microseconds
  uint16_t run_count = 0;            // Number of times task has run
};

struct ResetInfo {
  uint8_t last_task_id;
  uint8_t reset_reason; // The value of the MCUSR register on AVR boards
};

/* ================== Memory Monitoring ================== */
struct __attribute__((packed)) TaskMemoryInfo {
  uint16_t task_struct_size;      // Size of task object
  uint16_t subscription_size;      // Size of subscription array
  uint16_t queue_size;            // Size of message queue
  uint16_t total_allocated;       // Total memory allocated by task
};

struct __attribute__((packed)) SystemMemoryInfo {
  // Static Memory
  uint16_t total_ram;            // Total RAM available
  uint16_t static_data_size;     // Size of static/global variables
  uint16_t heap_size;            // Size of heap
  
  // Dynamic Memory
  uint16_t free_ram;             // Current free RAM
  uint16_t heap_fragments;       // Number of heap fragments
  uint16_t largest_block;        // Largest free block
  
  // Task Memory
  uint8_t total_tasks;           // Number of tasks
  uint16_t task_memory;          // Memory used by tasks
  
  // Message Memory
  uint8_t active_messages;       // Number of messages in flight
  uint16_t message_memory;       // Memory used by messages
  
  // Stack Memory
  uint16_t stack_size;           // Total stack size
  uint16_t stack_used;           // Used stack space
  uint16_t stack_free;           // Free stack space
  
  // Program Memory
  uint32_t flash_used;           // Used program memory
  uint32_t flash_free;           // Free program memory
};

/* ================== Logging ================== */
enum LogLevel : uint8_t {
  LOG_DEBUG = 0,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_NONE = 0xFF  // Disable logging
};

#ifndef FSMOS_LOG_LEVEL
#define FSMOS_LOG_LEVEL LOG_INFO  // Default log level
#endif

/* ================== Task Node ================== */
struct TaskNode {
    Task* task;
    TaskStats stats;
    TaskNode* next;
    uint8_t id;

    TaskNode(Task* t, uint8_t task_id) 
        : task(t), next(nullptr), id(task_id) {
         stats.max_exec_time_us = 0;
         stats.total_exec_time_us = 0;
         stats.run_count = 0;
    }
};

/* ================== Scheduler ================== */
class Scheduler {
public:
    void begin();
    void begin_with_logger();
    uint8_t add(Task* t);
    bool remove(uint8_t task_id);  // New method to remove tasks
    bool post(uint8_t type, uint8_t src_id, uint8_t topic, uint16_t arg = 0, void* ptr = nullptr, bool is_dynamic = false);
    void loop_once();
    inline void on_tick() { ms++; }
    uint32_t now() const { return ms; }

    // --- New Features & Accessors ---
    void enable_watchdog(uint8_t timeout = WDTO_1S);
    void enable_stack_monitoring();
    int get_free_stack() const;
    bool get_task_stats(uint8_t task_id, TaskStats& stats) const;
    uint8_t get_task_count() const;
    Task* get_task(uint8_t task_id) const;
    bool get_reset_info(ResetInfo& info);

    // Logging API
    void log(Task* task, LogLevel level, const __FlashStringHelper* message) {
      #if defined(FSMOS_DISABLE_LOGGING)
        return;
      #else
        if (level >= FSMOS_LOG_LEVEL) {
          _log(task, level, message);
        }
      #endif
    }

    template<typename T>
    void logf(Task* task, LogLevel level, const __FlashStringHelper* fmt, T value) {
      #if defined(FSMOS_DISABLE_LOGGING)
        return;
      #else
        if (level >= FSMOS_LOG_LEVEL) {
          _logf(task, level, fmt, value);
        }
      #endif
    }

private:
    void _log(Task* task, LogLevel level, const __FlashStringHelper* message);
    void _print_log_prefix(Task* task, LogLevel level);
    
    template<typename... Args>
    void _logf(Task* task, LogLevel level, const __FlashStringHelper* format, Args... args) {
      // Print timestamp and prefix
      _print_log_prefix(task, level);
      
      // Format the message with the value
      char fmt_buf[32];  // Small buffer for format string
      uint8_t i = 0;
      const char* p = reinterpret_cast<const char*>(fmt);
      while (i < sizeof(fmt_buf) - 1) {
        char c = pgm_read_byte(p++);
        if (c == 0) break;
        fmt_buf[i++] = c;
      }
      fmt_buf[i] = 0;
      
      // Print formatted message
      char out_buf[32];  // Small buffer for output
      snprintf(out_buf, sizeof(out_buf), fmt_buf, value);
      Serial.println(out_buf);
    }
    
    void _print_log_prefix(Task* task, LogLevel level);

    // Memory monitoring
    bool get_task_memory_info(uint8_t task_id, TaskMemoryInfo& info) const;
    bool get_system_memory_info(SystemMemoryInfo& info) const;
    uint16_t get_free_memory() const;      // Quick access to free RAM
    uint16_t get_largest_block() const;     // Largest allocatable block
    uint8_t get_heap_fragmentation() const; // Fragmentation percentage
    uint8_t count_heap_fragments() const;   // Count heap fragments
    
    // Message processing
    SharedMsg get_next_message(uint8_t task_id);
    void process_message(SharedMsg& msg);
    
    // Memory leak detection
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
    bool check_leaks() const {
      return fsmos_memory_stats.allocations != fsmos_memory_stats.deallocations;
    }
    
    uint16_t get_allocation_count() const {
      return fsmos_memory_stats.allocations;
    }
    
    uint16_t get_deallocation_count() const {
      return fsmos_memory_stats.deallocations;
    }
    
    uint16_t get_current_allocated_bytes() const {
      return fsmos_memory_stats.current_bytes;
    }
    
    uint16_t get_peak_allocated_bytes() const {
      return fsmos_memory_stats.peak_bytes;
    }
#endif

private:
    void deliver();
    TaskNode* find_task_node(uint8_t task_id) const;
    
    LinkedQueue<SharedMsg> message_queue;  // Linked list based message queue
    TaskNode* task_list;           // Head of the linked list
    uint8_t task_count;
    volatile uint32_t ms;
    bool watchdog_enabled;

    uint8_t next_task_id;         // For assigning unique IDs to tasks

public:
  Scheduler() = default;
  
  ~Scheduler() {
    // Clean up the task list
    while (task_list) {
      TaskNode* next = task_list->next;
      delete task_list;
      task_list = next;
    }
  }

  // Delete copy constructor and assignment
  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;
};

extern Scheduler OS;

/* ================== Task base class ================== */
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
struct MemoryStats {
  uint16_t allocations;
  uint16_t deallocations;
  uint16_t current_bytes;
  uint16_t peak_bytes;
};

extern MemoryStats fsmos_memory_stats;
#endif

class Task {
public:
  enum TaskState : uint8_t {
    ACTIVE = 0,    // Task is running normally (00)
    SUSPENDED = 1, // Task doesn't execute but receives messages (01)
    INACTIVE = 2   // Task will be auto-deleted (10)
  };

  virtual void on_start() {}
  virtual void on_msg(const MsgData& m) { (void)m; }
  virtual void step() = 0;
  virtual void on_suspend() {}     // Called when task is suspended
  virtual void on_resume() {}      // Called when task is resumed
  virtual void on_terminate() {}   // Called before task is deleted
  virtual ~Task() { 
    on_terminate();
    // Clear any pending messages in suspended queue
    SharedMsg msg;
    while (suspended_msg_queue.pop(msg)) {
      msg.release();
    }
    delete[] subscriptions;
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
    fsmos_memory_stats.deallocations++;
    fsmos_memory_stats.current_bytes -= sizeof(Task);
#endif
  }

  // State management
  void activate();   // Set task to ACTIVE state
  void suspend();    // Set task to SUSPENDED state
  void terminate();  // Set task to INACTIVE state (will be deleted)
  
  // State management
  TaskState get_state() const { return static_cast<TaskState>(state); }
  bool check_state(TaskState s) const { return state == s; }

  // Message handling
  bool tell(uint8_t dst_task_id, uint8_t type, uint16_t arg = 0, void* ptr = nullptr, bool is_dynamic = false);
  bool publish(uint8_t topic, uint8_t type, uint16_t arg = 0, void* ptr = nullptr, bool is_dynamic = false);
  void subscribe(uint8_t topic);
  bool is_subscribed_to(uint8_t topic);

  // Task management
  uint8_t get_id() const;
  void set_period(uint16_t period);
  uint16_t get_period() const;

  // Configure message handling during suspension
  void set_queue_messages_while_suspended(bool queue_messages);
  bool get_queue_messages_while_suspended() const;

  // Task naming
  void set_name(const __FlashStringHelper* name) { task_name = name; }
  const __FlashStringHelper* get_name() const { return task_name ? task_name : F("Unknown"); }

  // Logging helpers
  inline void log(LogLevel level, const __FlashStringHelper* msg) { OS.logMessage(this, level, msg); }
  inline void log_debug(const __FlashStringHelper* msg) { OS.log(this, LOG_DEBUG, msg); }
  inline void log_info(const __FlashStringHelper* msg) { OS.log(this, LOG_INFO, msg); }
  inline void log_warn(const __FlashStringHelper* msg) { OS.log(this, LOG_WARNING, msg); }
  inline void log_error(const __FlashStringHelper* msg) { OS.log(this, LOG_ERROR, msg); }

  template<typename... Args>
  void log_debug(const __FlashStringHelper* format, Args... args) { OS.logf(this, LOG_DEBUG, format, args...); }
  template<typename... Args>
  void log_info(const __FlashStringHelper* format, Args... args) { OS.logf(this, LOG_INFO, format, args...); }
  template<typename... Args>
  void log_warn(const __FlashStringHelper* format, Args... args) { OS.logf(this, LOG_WARNING, format, args...); }
  template<typename... Args>
  void log_error(const __FlashStringHelper* format, Args... args) { OS.logf(this, LOG_ERROR, format, args...); }

  // Process any pending messages for this task
  void process_messages();

private:
  friend class Scheduler;

  uint32_t next_due = 0;
  uint16_t period_ms = 1;
  uint8_t id = 255;
  TaskState state = ACTIVE;
  uint8_t subscription_count = 0;
  uint8_t subscription_capacity = 0;
  const __FlashStringHelper* task_name = nullptr;  // Task name stored in PROGMEM
  uint8_t flags;                    // Bit flags for task configuration
  uint8_t* subscriptions = nullptr;
  LinkedQueue<SharedMsg> suspended_msg_queue;  // Queue for messages during suspended state

  // Bit definitions for flags
  static const uint8_t FLAG_QUEUE_MSGS = 0x01;
  static const uint8_t FLAG_AUTO_DELETE = 0x02;
  static const uint8_t FLAG_RESERVED1 = 0x04;
  static const uint8_t FLAG_RESERVED2 = 0x08;

public:
  explicit Task(const __FlashStringHelper* name = nullptr, uint8_t max_subscriptions = 2) {
    task_name = name;
    subscriptions = new uint8_t[max_subscriptions];
    subscription_count = 0;
    subscription_capacity = min(max_subscriptions, uint8_t(15));
    state = ACTIVE;
    flags = FLAG_QUEUE_MSGS;  // Default: queue messages while suspended
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
    fsmos_memory_stats.allocations++;
    fsmos_memory_stats.current_bytes += sizeof(Task);
    if (fsmos_memory_stats.current_bytes > fsmos_memory_stats.peak_bytes) {
      fsmos_memory_stats.peak_bytes = fsmos_memory_stats.current_bytes;
    }
#endif
  }
};

/* ================== Utility: soft-timer for FSMs ================== */
struct __attribute__((packed)) Timer {
  uint32_t start_ms = 0;
  uint32_t duration_ms = 0;

  inline void start(uint32_t d) {
    start_ms = OS.now();
    duration_ms = d;
  }

  inline bool expired() const {
    return duration_ms == 0 || (int32_t)(OS.now() - start_ms) >= (int32_t)duration_ms;
  }
};
