/**
 * @file FsmOS.h
 * @brief A lightweight cooperative task scheduler for Arduino
 * @author Aykut Ozdemir
 * @date 2025-10-02
 * 
 * FsmOS provides a simple, memory-efficient task scheduler for Arduino,
 * supporting cooperative multitasking, message passing, and system monitoring.
 * 
 * Key features:
 * - Cooperative task scheduling with configurable periods
 * - Inter-task communication through messages and events
 * - Dynamic task creation and deletion
 * - Memory management and monitoring
 * - System diagnostics and profiling
 * - Logging system with multiple levels
 * 
 * @note This library is designed for AVR-based Arduino boards but includes
 * partial support for other architectures.
 */

#pragma once

/* ================== Core Types and Dependencies ================== */
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
/**
 * @brief Message data structure for inter-task communication
 * 
 * This structure represents a message that can be passed between tasks.
 * It supports both direct messaging (task-to-task) and publish/subscribe
 * patterns through topics.
 * 
 * Messages can carry:
 * - Small payloads (16-bit argument)
 * - Large payloads (pointer to data)
 * - Dynamic memory that's automatically managed
 */
struct __attribute__((packed)) MsgData {
  uint8_t type;         ///< User-defined event type
  uint8_t src_id;       ///< Scheduler-assigned ID of the source task
  uint8_t topic;        ///< Topic ID (0=direct message, 1-255=pub/sub topics)
  uint8_t ref_count: 7; ///< Number of tasks to process message (max 127)
  bool is_dynamic: 1;   ///< Whether ptr points to dynamically allocated data
  uint16_t arg;         ///< Small payload
  void* ptr;            ///< Optional pointer to larger data
  uint16_t dynamic_size;///< Size of dynamically allocated data
  
  /** @brief Initialize an empty message */
  MsgData() : type(0), src_id(0), topic(0), ref_count(0), is_dynamic(0), arg(0), ptr(nullptr) {}
};

// Smart pointer-like class for message reference counting
class SharedMsg {
  MsgData* data;

public:
  SharedMsg() : data(nullptr) {}
  
  explicit SharedMsg(MsgData* msg) : data(msg) {
    if (data) {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { data->ref_count++; }
    }
  }
  
  SharedMsg(const SharedMsg& other) : data(other.data) {
    if (data) {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { data->ref_count++; }
    }
  }
  
  SharedMsg& operator=(const SharedMsg& other) {
    if (this != &other) {
      release();
      data = other.data;
      if (data) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { data->ref_count++; }
      }
    }
    return *this;
  }
  
  ~SharedMsg() { release(); }
  
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
        }
        delete data;
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
      if (!head) { head = tail = new_node; }
      else { tail->next = new_node; tail = new_node; }
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
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { is_empty = (head == nullptr); }
    return is_empty;
  }

  inline uint8_t size() const { return count; }
};

/* ================== Profiling & Reset Info ================== */
/**
 * @brief Task execution statistics
 * 
 * Tracks performance metrics for individual tasks including:
 * - Maximum execution time
 * - Total execution time
 * - Number of executions
 */
struct __attribute__((packed)) TaskStats {
  uint32_t max_exec_time_us = 0;    ///< Longest single execution time
  uint32_t total_exec_time_us = 0;  ///< Total execution time
  uint16_t run_count = 0;           ///< Number of times task has run
};

/**
 * @brief System reset information
 * 
 * Stores information about system resets and crashes including:
 * - Last task that was running
 * - Reason for reset (e.g., watchdog, brown-out)
 * 
 * This struct is placed in .noinit section to survive resets
 */
struct ResetInfo {
  uint8_t last_task_id;  ///< ID of task running during reset
  uint8_t reset_reason;   ///< MCU status register value at reset
};

/* ================== Memory Monitoring ================== */
struct __attribute__((packed)) TaskMemoryInfo {
  uint16_t task_struct_size;      // Size of task object
  uint16_t subscription_size;      // Size of subscription array
  uint16_t queue_size;            // Size of message queue
  uint16_t total_allocated;       // Total memory allocated by task
};

/**
 * @brief Comprehensive system memory information
 * 
 * Provides detailed breakdown of memory usage across different
 * memory regions and subsystems. Useful for:
 * - Memory leak detection
 * - Resource usage optimization
 * - System health monitoring
 */
struct __attribute__((packed)) SystemMemoryInfo {
  // Static Memory
  uint16_t total_ram;         ///< Total RAM available on device
  uint16_t static_data_size;  ///< Size of static/global variables
  uint16_t heap_size;         ///< Total size of heap region
  
  // Dynamic Memory
  uint16_t free_ram;          ///< Currently available RAM
  uint16_t heap_fragments;    ///< Number of fragmented heap blocks
  uint16_t largest_block;     ///< Size of largest contiguous block
  
  // Task Memory
  uint8_t total_tasks;        ///< Number of active tasks
  uint16_t task_memory;       ///< Total memory used by task objects
  
  // Message Memory
  uint8_t active_messages;    ///< Number of pending messages
  uint16_t message_memory;    ///< Memory used by message system
  
  // Stack Memory
  uint16_t stack_size;        ///< Total allocated stack size
  uint16_t stack_used;        ///< Currently used stack space
  uint16_t stack_free;        ///< Remaining stack space
  
  // Program Memory
  uint32_t flash_used;        ///< Used flash memory space
  uint32_t flash_free;        ///< Available flash memory
};

/* ================== Logging ================== */
/**
 * @brief Log message severity levels
 * 
 * Defines different severity levels for log messages
 * to help with filtering and formatting output.
 */
enum LogLevel : uint8_t {
  LOG_DEBUG = 0,    ///< Detailed debug information
  LOG_INFO,         ///< General information
  LOG_WARNING,      ///< Warning conditions
  LOG_ERROR,        ///< Error conditions
  LOG_NONE = 0xFF   ///< Disable logging
};

#ifndef FSMOS_LOG_LEVEL
#define FSMOS_LOG_LEVEL LOG_INFO
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
/**
 * @brief Core scheduler and task manager for FsmOS
 * 
 * The Scheduler class is the heart of FsmOS, managing task execution,
 * message delivery, and system resources. It provides:
 * - Task lifecycle management
 * - Message routing and delivery
 * - System timing and scheduling
 * - Memory monitoring and diagnostics
 * - Task statistics and profiling
 */
class Scheduler {
public:
    /**
     * @brief Initialize the scheduler
     * Sets up internal data structures and timing system
     */
    void begin();

    /**
     * @brief Initialize with logging capabilities
     * Currently an alias for begin(), reserved for future use
     */
    void begin_with_logger();

    /**
     * @brief Add a new task to the scheduler
     * @param t Pointer to the task to add
     * @return Task ID (255 if failed)
     */
    uint8_t add(Task* t);

    /**
     * @brief Remove a task from the scheduler
     * @param task_id ID of the task to remove
     * @return true if task was found and removed
     */
    bool remove(uint8_t task_id);

    /**
     * @brief Post a message to the system
     * @param type User-defined message type
     * @param src_id Source task ID
     * @param topic Topic ID (0 for direct, 1-255 for pub/sub)
     * @param arg Optional 16-bit payload
     * @param ptr Optional pointer to larger data
     * @param is_dynamic Whether ptr is dynamically allocated
     * @return true if message was queued successfully
     */
    bool post(uint8_t type, uint8_t src_id, uint8_t topic, 
              uint16_t arg = 0, void* ptr = nullptr, bool is_dynamic = false);

    /**
     * @brief Execute one iteration of the scheduler
     * Processes messages and runs due tasks
     */
    void loop_once();

    /**
     * @brief System tick handler
     * Updates internal time counter
     */
    inline void on_tick() { ms++; }

    /**
     * @brief Get current system time in milliseconds
     * @return Current time since boot
     */
    uint32_t now() const { return ms; }

    /**
     * @brief Enable the watchdog timer
     * @param timeout Watchdog timeout period
     */
    void enable_watchdog(uint8_t timeout = WDTO_1S);

    /**
     * @brief Enable stack usage monitoring
     * Installs stack canaries for overflow detection
     */
    void enable_stack_monitoring();

    /**
     * @brief Get remaining free stack space
     * @return Number of free bytes in stack
     */
    int get_free_stack() const;

    /**
     * @brief Get execution statistics for a task
     * @param task_id ID of the task
     * @param stats Reference to store statistics
     * @return true if task was found
     */
    bool get_task_stats(uint8_t task_id, TaskStats& stats) const;

    /**
     * @brief Get total number of active tasks
     * @return Number of tasks in the system
     */
    uint8_t get_task_count() const;

    /**
     * @brief Get pointer to a task by ID
     * @param task_id ID of the task to find
     * @return Pointer to task or nullptr if not found
     */
    Task* get_task(uint8_t task_id) const;

    /**
     * @brief Get information about last system reset
     * @param info Reference to store reset information
     * @return true if information was available
     */
    bool get_reset_info(ResetInfo& info);

    // Logging API
    void logMessage(Task* task, LogLevel level, const __FlashStringHelper* message);
    void logFormatted(Task* task, LogLevel level, const __FlashStringHelper* fmt, ...);

    // Diagnostics (public)
    bool get_task_memory_info(uint8_t task_id, TaskMemoryInfo& info) const;
    bool get_system_memory_info(SystemMemoryInfo& info) const;
    uint16_t get_free_memory() const;
    uint16_t get_largest_block() const;
    uint8_t get_heap_fragmentation() const;
    uint8_t count_heap_fragments() const;

    // Message processing
    SharedMsg get_next_message(uint8_t task_id);
    void process_message(SharedMsg& msg);

private:
    void _print_log_prefix(Task* task, LogLevel level);
    void deliver();
    TaskNode* find_task_node(uint8_t task_id) const;
    
    LinkedQueue<SharedMsg> message_queue;
    TaskNode* task_list;
    uint8_t task_count;
    volatile uint32_t ms;
    bool watchdog_enabled;
    uint8_t next_task_id;

public:
  Scheduler() = default;
  ~Scheduler() {
    while (task_list) {
      TaskNode* next = task_list->next;
      delete task_list;
      task_list = next;
    }
  }
  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;
};

extern Scheduler OS;

/* ================== Task base class ================== */
/**
 * @brief Base class for all tasks in FsmOS
 * 
 * The Task class provides the foundation for implementing cooperative
 * multitasking. Each task represents an independent unit of work that
 * can run periodically, respond to messages, and manage its own lifecycle.
 * 
 * Key features:
 * - Periodic execution with configurable intervals
 * - Message handling through callbacks
 * - Lifecycle management (start, suspend, resume, terminate)
 * - Resource cleanup on termination
 * - Topic-based message subscription
 */
class Task {
public:
  /** @brief Possible states a task can be in */
  enum TaskState : uint8_t { 
    ACTIVE = 0,    ///< Task is running normally
    SUSPENDED = 1, ///< Task is temporarily paused
    INACTIVE = 2   ///< Task is terminated and ready for cleanup
  };

  /** @brief Called when the task is first added to the scheduler */
  virtual void on_start() {}
  
  /** 
   * @brief Called when a message is received
   * @param m The received message
   */
  virtual void on_msg(const MsgData& m) { (void)m; }
  
  /** 
   * @brief Main task function, called periodically by the scheduler
   * @note Must be implemented by derived classes
   */
  virtual void step() = 0;
  
  /** @brief Called before the task enters suspended state */
  virtual void on_suspend() {}
  
  /** @brief Called when task resumes from suspended state */
  virtual void on_resume() {}
  
  /** 
   * @brief Called when task is being terminated
   * Cleans up resources and pending messages
   */
  virtual void on_terminate() {
    on_terminate();
    SharedMsg msg;
    while (suspended_msg_queue.pop(msg)) { msg.release(); }
    delete[] subscriptions;
  }

  /**
   * @brief Activate or resume the task
   * Task will resume normal execution in next scheduler cycle
   */
  void activate();

  /**
   * @brief Temporarily suspend task execution
   * Task can be resumed later with activate()
   */
  void suspend();

  /**
   * @brief Permanently terminate the task
   * Task will be removed and cleaned up by scheduler
   */
  void terminate();

  /**
   * @brief Get current task state
   * @return Current TaskState
   */
  TaskState get_state() const { return static_cast<TaskState>(state); }

  /**
   * @brief Check if task is in a specific state
   * @param s State to check against
   * @return true if task is in specified state
   */
  bool check_state(TaskState s) const { return state == s; }

  /**
   * @brief Check if task is currently active
   * @return true if task is in ACTIVE state
   */
  bool is_active() const { return state == ACTIVE; }

  /**
   * @brief Check if task is terminated
   * @return true if task is in INACTIVE state
   */
  bool is_inactive() const { return state == INACTIVE; }

  /**
   * @brief Send a direct message to another task
   * @param dst_task_id ID of the destination task
   * @param type User-defined message type
   * @param arg Optional 16-bit argument
   * @param ptr Optional pointer to larger data
   * @param is_dynamic Whether ptr points to dynamically allocated data
   * @return true if message was successfully queued
   */
  bool tell(uint8_t dst_task_id, uint8_t type, uint16_t arg = 0, void* ptr = nullptr, bool is_dynamic = false);

  /**
   * @brief Publish a message to all subscribers of a topic
   * @param topic Topic ID (1-255, 0 is reserved for direct messages)
   * @param type User-defined message type
   * @param arg Optional 16-bit argument
   * @param ptr Optional pointer to larger data
   * @param is_dynamic Whether ptr points to dynamically allocated data
   * @return true if message was successfully queued
   */
  bool publish(uint8_t topic, uint8_t type, uint16_t arg = 0, void* ptr = nullptr, bool is_dynamic = false);

  /**
   * @brief Subscribe to messages on a specific topic
   * @param topic Topic ID to subscribe to (1-255)
   * @note Maximum 15 subscriptions per task
   */
  void subscribe(uint8_t topic);

  /**
   * @brief Check if task is subscribed to a topic
   * @param topic Topic ID to check
   * @return true if task is subscribed to the topic
   */
  bool is_subscribed_to(uint8_t topic);

  /**
   * @brief Get the task's unique identifier
   * @return Task ID assigned by scheduler
   */
  uint8_t get_id() const;

  /**
   * @brief Set the task's execution period
   * @param period Time in milliseconds between step() calls
   */
  void set_period(uint16_t period);

  /**
   * @brief Get the task's current execution period
   * @return Time in milliseconds between step() calls
   */
  uint16_t get_period() const;

  /**
   * @brief Configure message queueing during suspension
   * @param queue_messages true to queue messages, false to discard
   */
  void set_queue_messages_while_suspended(bool queue_messages);

  /**
   * @brief Check message queueing configuration
   * @return true if messages are queued during suspension
   */
  bool get_queue_messages_while_suspended() const;

  /**
   * @brief Set the task's name (stored in flash memory)
   * @param name Flash string pointer to task name
   */
  void set_name(const __FlashStringHelper* name) { task_name = name; }

  /**
   * @brief Get the task's name
   * @return Flash string pointer to task name or "Unknown"
   */
  const __FlashStringHelper* get_name() const { return task_name ? task_name : F("Unknown"); }

  /**
   * @brief Log a message with specified severity level
   * @param level Message severity level
   * @param msg Message text (stored in flash)
   */
  inline void log(LogLevel level, const __FlashStringHelper* msg) { OS.logMessage(this, level, msg); }

  /**
   * @brief Log a debug message
   * @param msg Debug message text (stored in flash)
   */
  inline void log_debug(const __FlashStringHelper* msg) { OS.logMessage(this, LOG_DEBUG, msg); }

  /**
   * @brief Log an informational message
   * @param msg Info message text (stored in flash)
   */
  inline void log_info(const __FlashStringHelper* msg) { OS.logMessage(this, LOG_INFO, msg); }

  /**
   * @brief Log a warning message
   * @param msg Warning message text (stored in flash)
   */
  inline void log_warn(const __FlashStringHelper* msg) { OS.logMessage(this, LOG_WARNING, msg); }

  /**
   * @brief Log an error message
   * @param msg Error message text (stored in flash)
   */
  inline void log_error(const __FlashStringHelper* msg) { OS.logMessage(this, LOG_ERROR, msg); }

  /**
   * @brief Process pending messages for this task
   * @note Called automatically by scheduler, rarely needs direct use
   */
  void process_messages();

private:
  friend class Scheduler;

  uint32_t next_due = 0;
  uint16_t period_ms = 1;
  uint8_t id = 255;
  TaskState state = ACTIVE;
  uint8_t subscription_count = 0;
  uint8_t subscription_capacity = 0;
  const __FlashStringHelper* task_name = nullptr;
  bool queue_messages_while_suspended = true;
  uint8_t queue_msgs = 1;
  uint8_t* subscriptions = nullptr;
  LinkedQueue<SharedMsg> suspended_msg_queue;

public:
  explicit Task(const __FlashStringHelper* name = nullptr, uint8_t max_subscriptions = 2) {
    task_name = name;
    subscriptions = new uint8_t[max_subscriptions];
    subscription_count = 0;
    subscription_capacity = min(max_subscriptions, uint8_t(15));
    state = ACTIVE;
    queue_messages_while_suspended = true;
    queue_msgs = 1;
  }
};

/* ================== Utility: soft-timer for FSMs ================== */
/**
 * @brief Lightweight timer for time-based state machines
 * 
 * Timer provides a simple way to manage time-based events in tasks.
 * It supports:
 * - One-shot timing
 * - Duration checking
 * - Automatic time overflow handling
 */
struct __attribute__((packed)) Timer {
  uint32_t start_ms = 0;     ///< Timer start timestamp
  uint32_t duration_ms = 0;   ///< Timer duration in milliseconds

  /**
   * @brief Start the timer
   * @param d Duration in milliseconds
   */
  inline void start(uint32_t d) { start_ms = OS.now(); duration_ms = d; }

  /**
   * @brief Check if timer has expired
   * @return true if timer duration has elapsed
   */
  inline bool expired() const { 
    return duration_ms == 0 || (int32_t)(OS.now() - start_ms) >= (int32_t)duration_ms; 
  }
};
