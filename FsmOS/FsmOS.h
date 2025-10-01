#pragma once

/* ================== core types ================== */
#include <Arduino.h>
#include <stdint.h>
#include <util/atomic.h> // For ATOMIC_BLOCK

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
  uint8_t ref_count; // Number of tasks that need to process this message
  uint16_t arg;     // Small payload
  void* ptr;        // Optional pointer to larger data
  bool is_dynamic;  // Whether ptr needs to be freed
  
  MsgData() : type(0), src_id(0), topic(0), ref_count(0), arg(0), ptr(nullptr), is_dynamic(false) {}
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
    void* mem = ::operator new(sizeof(Node), std::nothrow);
    if (!mem) return false;
    
    Node* new_node = new(mem) Node(v);
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
  uint32_t max_exec_time_us = 0;
  uint32_t total_exec_time_us = 0;
  uint16_t run_count = 0;  // Reduced from uint32_t as 65535 runs is usually sufficient
};

struct ResetInfo {
  uint8_t last_task_id;
  uint8_t reset_reason; // The value of the MCUSR register on AVR boards
};

/* ================== Task Node ================== */
struct TaskNode {
    Task* task;
    TaskStats stats;
    TaskNode* next;
    uint8_t id;

    TaskNode(Task* t, uint8_t task_id) 
        : task(t), next(nullptr), id(task_id) {
        stats = {0, 0, 0};
    }
};

/* ================== Scheduler ================== */
class Scheduler {
public:
    void begin();
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

    // Message processing
    SharedMsg get_next_message(uint8_t task_id);
    void process_message(SharedMsg& msg);

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
class Task {
public:
  enum TaskState {
    ACTIVE,     // Task is running normally
    SUSPENDED,  // Task doesn't execute but receives messages
    INACTIVE    // Task will be auto-deleted
  };

  virtual void on_start() {}
  virtual void on_msg(const MsgData& m) { (void)m; }
  virtual void step() = 0;
  virtual void on_suspend() {}     // Called when task is suspended
  virtual void on_resume() {}      // Called when task is resumed
  virtual void on_terminate() {}   // Called before task is deleted
  virtual ~Task() { 
    on_terminate();
    delete[] subscriptions; 
  }

  // State management
  void activate();   // Set task to ACTIVE state
  void suspend();    // Set task to SUSPENDED state
  void terminate();  // Set task to INACTIVE state (will be deleted)
  
  // State checks
  bool is_active() const { return state == ACTIVE; }
  bool is_suspended() const { return state == SUSPENDED; }
  bool is_inactive() const { return state == INACTIVE; }
  TaskState get_state() const { return state; }

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
  bool queue_messages_while_suspended = true;  // Whether to queue messages while suspended
  uint8_t* subscriptions = nullptr;
  LinkedQueue<SharedMsg> suspended_msg_queue;  // Queue for messages during suspended state

public:
  explicit Task(uint8_t max_subscriptions = 4) {  // Reduced default as most tasks use fewer topics
    subscriptions = new uint8_t[max_subscriptions];
    subscription_capacity = max_subscriptions;
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
