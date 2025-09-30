#include "FsmOS.h"

/* ================== Global OS Instance ================== */
Scheduler OS;

/* ================== Reset Info ================== */
// This struct will be placed in the .noinit section, so it survives a reset.
__attribute__((section(".noinit")))
ResetInfo reset_info;

// Helper to get reset reason from MCUSR
#if defined(__AVR__)
void get_mcusr() {
  // MCUSR must be read and cleared early in the startup sequence.
  // A copy is saved to reset_info.reset_reason.
  reset_info.reset_reason = MCUSR;
  MCUSR = 0;
}
#endif

/* ================== Scheduler Implementation ================== */

void Scheduler::begin() {
#if defined(__AVR__)
  // This function is called by the bootloader before global constructors.
  // We save the MCUSR register early.
  __asm__ __volatile__("call get_mcusr");
#else
  // For non-AVR, we can't determine reset cause this way.
  reset_info.reset_reason = 0;
#endif
  ms = millis();
}

uint8_t Scheduler::add(Task* t) {
  if (task_count >= FSMOS_MAX_TASKS) {
    return 255; // Error: Task limit reached
  }
  tasks[task_count] = t;
  t->id = task_count;
  t->state = Task::ACTIVE;
  t->next_due = ms; // Run as soon as possible
  task_stats[task_count] = {0, 0, 0};
  task_count++;
  t->on_start();
  return t->id;
}

bool Scheduler::post(Msg m) {
  return bus.push(m);
}

void Scheduler::loop_once() {
  // 1. Update time
  uint32_t now = millis();
  ms = now;

  // 2. Deliver all messages from the global bus
  deliver();

  // 3. Execute due tasks
  for (uint8_t i = 0; i < task_count; ++i) {
    if (tasks[i] && tasks[i]->is_active() && (int32_t)(now - tasks[i]->next_due) >= 0) {
      
      // WDT & Profiling Start
      reset_info.last_task_id = i; // Mark current task before running
      uint32_t start_us = micros();

      tasks[i]->step();

      // Profiling End
      uint32_t exec_time = micros() - start_us;
      task_stats[i].total_exec_time_us += exec_time;
      if (exec_time > task_stats[i].max_exec_time_us) {
        task_stats[i].max_exec_time_us = exec_time;
      }
      task_stats[i].run_count++;

      // Schedule next run
      tasks[i]->next_due += tasks[i]->get_period();
      
      // If a task takes too long, it might miss its deadline.
      // This ensures it schedules for the next available slot from NOW.
      if ((int32_t)(tasks[i]->next_due - now) < 0) {
          tasks[i]->next_due = now + tasks[i]->get_period();
      }
    }
  }
  
  // 4. Pet the watchdog
  if (watchdog_enabled) {
    wdt_reset();
  }
}

void Scheduler::deliver() {
  Msg m;
  while (bus.pop(m)) {
    if (m.topic == 0) { // Direct message
      if (m.src_id < task_count && tasks[m.src_id]) {
        tasks[m.src_id]->inbox.push(m);
      }
    } else { // Topic-based (publish/subscribe)
      for (uint8_t i = 0; i < task_count; ++i) {
        if (tasks[i] && tasks[i]->is_subscribed_to(m.topic)) {
          tasks[i]->inbox.push(m);
        }
      }
    }
  }
}

void Scheduler::enable_watchdog(uint8_t timeout) {
#if defined(__AVR__)
  wdt_enable(timeout);
  watchdog_enabled = true;
#endif
}

bool Scheduler::get_reset_info(ResetInfo& info) {
  info = reset_info;
  // Clear last task ID after reading to avoid stale info on next reset
  reset_info.last_task_id = 255; 
  return true;
}

uint8_t Scheduler::get_task_count() const {
  return task_count;
}

Task* Scheduler::get_task(uint8_t task_id) const {
  if (task_id < task_count) {
    return tasks[task_id];
  }
  return nullptr;
}

bool Scheduler::get_task_stats(uint8_t task_id, TaskStats& stats) const {
    if (task_id < task_count) {
        stats = task_stats[task_id];
        return true;
    }
    return false;
}

/* ================== Task Implementation ================== */

bool Task::tell(uint8_t dst_task_id, uint8_t type, uint16_t arg, void* ptr) {
  Msg m = {type, id, 0, arg, ptr};
  // We need to find the destination task to post to its queue directly
  // For simplicity, we post to the global bus and let the scheduler deliver.
  // A more direct implementation would be:
  // return OS.get_task(dst_task_id)->inbox.push(m);
  // But this is safer for now.
  m.src_id = dst_task_id; // Repurposing src_id for destination
  return OS.post(m);
}

bool Task::publish(uint8_t topic, uint8_t type, uint16_t arg, void* ptr) {
  if (topic == 0) return false; // Topic 0 is reserved for direct messages
  Msg m = {type, id, topic, arg, ptr};
  return OS.post(m);
}

void Task::subscribe(uint8_t topic) {
  if (topic > 0 && subscription_count < FSMOS_TASK_QUEUE_CAP) {
    // Avoid duplicate subscriptions
    for (uint8_t i = 0; i < subscription_count; ++i) {
      if (subscriptions[i] == topic) return;
    }
    subscriptions[subscription_count++] = topic;
  }
}

bool Task::is_subscribed_to(uint8_t topic) {
  for (uint8_t i = 0; i < subscription_count; ++i) {
    if (subscriptions[i] == topic) return true;
  }
  return false;
}

void Task::stop() {
  state = INACTIVE;
}

void Task::resume() {
  state = ACTIVE;
  next_due = OS.now() + period_ms;
}

bool Task::is_active() const {
  return state == ACTIVE;
}

uint8_t Task::get_id() const {
  return id;
}

void Task::set_period(uint16_t period) {
  period_ms = period;
}

uint16_t Task::get_period() const {
  return period_ms;
}