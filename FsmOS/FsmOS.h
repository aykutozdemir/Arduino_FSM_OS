#pragma once

/* ================== config ================== */
#ifndef FSMOS_MAX_TASKS
#define FSMOS_MAX_TASKS 8
#endif

#ifndef FSMOS_GLOBAL_QUEUE_CAP
#define FSMOS_GLOBAL_QUEUE_CAP 32
#endif

#ifndef FSMOS_TASK_QUEUE_CAP
#define FSMOS_TASK_QUEUE_CAP 8
#endif

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

/* Message/Event for inter-task communication */
struct Msg {
  uint8_t type;     // User-defined event type (0-255)
  uint8_t src_id;   // Scheduler-assigned ID of the source task
  uint8_t topic;    // Optional topic for publish/subscribe (0 for direct, 1-255 for topics)
  uint16_t arg;     // Small payload
  void*    ptr;     // Optional pointer to larger data (avoid unless static/const)
};

/*
 * Lightweight, interrupt-safe ring buffer.
 */
template<size_t N, typename T>
struct Ring {
  volatile uint8_t head = 0, tail = 0;
  T buf[N];

  inline bool push(const T &v) {
    bool result = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      uint8_t h = head;
      uint8_t next_h = (uint8_t)(h + 1);
      if (next_h % N != tail) { // Check if not full
        buf[h] = v;
        head = next_h % N;
        result = true;
      }
    }
    return result;
  }

  inline bool pop(T &out) {
    bool result = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      uint8_t t = tail;
      if (t != head) { // Check if not empty
        out = buf[t];
        tail = (uint8_t)(t + 1) % N;
        result = true;
      }
    }
    return result;
  }

  inline bool empty() const {
    bool is_empty;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      is_empty = (head == tail);
    }
    return is_empty;
  }

  inline uint8_t size() const {
    uint8_t current_size;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      current_size = (head - tail + N) % N;
    }
    return current_size;
  }
};

/* ================== Profiling & Reset Info ================== */
struct TaskStats {
  uint32_t max_exec_time_us = 0;
  uint32_t total_exec_time_us = 0;
  uint32_t run_count = 0;
};

struct ResetInfo {
  uint8_t last_task_id;
  uint8_t reset_reason; // The value of the MCUSR register on AVR boards
};

/* ================== Scheduler ================== */
class Scheduler {
public:
  void begin();
  uint8_t add(Task* t);
  bool post(Msg m);
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

private:
  void deliver();

  Ring<FSMOS_GLOBAL_QUEUE_CAP, Msg> bus;
  Task* tasks[FSMOS_MAX_TASKS] = {0};
  uint8_t task_count = 0;
  volatile uint32_t ms = 0;
  TaskStats task_stats[FSMOS_MAX_TASKS];
  bool watchdog_enabled = false;
};

extern Scheduler OS;

/* ================== Task base class ================== */
class Task {
public:
  enum TaskState { ACTIVE, INACTIVE };

  virtual void on_start() {}
  virtual void on_msg(const Msg& m) { (void)m; }
  virtual void step() = 0;
  virtual ~Task() {}

  bool tell(uint8_t dst_task_id, uint8_t type, uint16_t arg = 0, void* ptr = nullptr);
  bool publish(uint8_t topic, uint8_t type, uint16_t arg = 0, void* ptr = nullptr);
  void subscribe(uint8_t topic);
  bool is_subscribed_to(uint8_t topic);
  void stop();
  void resume();
  bool is_active() const;
  uint8_t get_id() const;
  void set_period(uint16_t period);
  uint16_t get_period() const;

protected:
  inline bool recv(Msg &m) { return inbox.pop(m); }

private:
  friend class Scheduler;

  Ring<FSMOS_TASK_QUEUE_CAP, Msg> inbox;
  uint8_t id = 255;
  uint16_t period_ms = 1;
  uint32_t next_due = 0;
  TaskState state = ACTIVE;
  uint8_t subscriptions[FSMOS_TASK_QUEUE_CAP] = {0};
  uint8_t subscription_count = 0;
};

/* ================== Utility: soft-timer for FSMs ================== */
struct Timer {
  uint32_t start_ms = 0;
  uint32_t duration_ms = 0;

  inline void start(uint32_t d) {
    duration_ms = d;
    start_ms = OS.now();
  }

  inline bool expired() const {
    if (duration_ms == 0) return true;
    return (int32_t)(OS.now() - start_ms) >= (int32_t)duration_ms;
  }
};
