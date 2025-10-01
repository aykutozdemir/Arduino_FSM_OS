#include "FsmOS.h"

/* ================== Global OS Instance ================== */
Scheduler OS;

/* ================== Reset Info ================== */
// This struct will be placed in the .noinit section, so it survives a reset.
__attribute__((section(".noinit")))
ResetInfo reset_info;


/* ================== Scheduler Implementation ================== */

void Scheduler::begin() {
#if defined(__AVR__)
  // Read and clear MCUSR register early
  reset_info.reset_reason = MCUSR;
  MCUSR = 0;
#else
  // For non-AVR, we can't determine reset cause this way.
  reset_info.reset_reason = 0;
#endif

  // Initialize the linked list
  task_list = nullptr;
  task_count = 0;
  next_task_id = 0;
  ms = millis();
  watchdog_enabled = false;
}

uint8_t Scheduler::add(Task* t) {
  uint8_t new_id = next_task_id++;
  if (next_task_id == 0) {  // Wrapped around
    next_task_id = 1;  // Skip 0 for next time
  }

  TaskNode* new_node = new TaskNode(t, new_id);
  t->id = new_id;
  t->state = Task::ACTIVE;
  t->next_due = ms; // Run as soon as possible

  // Add to front of list
  new_node->next = task_list;
  task_list = new_node;
  task_count++;
  
  t->on_start();
  return new_id;
}

bool Scheduler::remove(uint8_t task_id) {
  if (!task_list) return false;

  TaskNode** curr = &task_list;
  while (*curr) {
    if ((*curr)->id == task_id) {
      TaskNode* to_delete = *curr;
      *curr = to_delete->next;
      delete to_delete;
      task_count--;
      return true;
    }
    curr = &((*curr)->next);
  }
  return false;
}

bool Scheduler::post(uint8_t type, uint8_t src_id, uint8_t topic, uint16_t arg, void* ptr, bool is_dynamic) {
  MsgData* data = new MsgData();
  data->type = type;
  data->src_id = src_id;
  data->topic = topic;
  data->arg = arg;
  data->ptr = ptr;
  data->is_dynamic = is_dynamic;
  
  // Count subscribers
  uint8_t target_count = 0;
  if (topic == 0) {
    // Direct message
    TaskNode* target_node = find_task_node(src_id);
    if (target_node && target_node->task) target_count = 1;
  } else {
    // Topic-based message
    TaskNode* curr = task_list;
    while (curr) {
      if (curr->task && curr->task->is_subscribed_to(topic)) target_count++;
      curr = curr->next;
    }
  }
  
  data->ref_count = target_count;
  if (target_count == 0) {
    delete data;
    return false;
  }
  
  return message_queue.push(SharedMsg(data));
}

void Scheduler::loop_once() {
  // 1. Update time
  uint32_t now = millis();
  ms = now;

  // 2. Deliver all messages from the global bus
  deliver();

  // 3. Execute due tasks and handle cleanup
  TaskNode** curr = &task_list;
  while (*curr) {
    TaskNode* node = *curr;
    Task* task = node->task;
    bool delete_task = false;

    if (task) {
      if (task->is_inactive()) {
        // Mark for deletion
        delete_task = true;
      }
      else if (task->is_active() && (int32_t)(now - task->next_due) >= 0) {
        // Only execute if task is active (not suspended or inactive)
        // WDT & Profiling Start
        reset_info.last_task_id = node->id;
        uint32_t start_us = micros();

        task->step();

        // Profiling End
        uint32_t exec_time = micros() - start_us;
        node->stats.total_exec_time_us += exec_time;
        if (exec_time > node->stats.max_exec_time_us) {
          node->stats.max_exec_time_us = exec_time;
        }
        node->stats.run_count++;

        // Schedule next run
        task->next_due += task->get_period();
        
        // Handle missed deadlines
        if ((int32_t)(task->next_due - now) < 0) {
          task->next_due = now + task->get_period();
        }
      }
    }

    if (delete_task) {
      // Remove and delete the task
      *curr = node->next;
      delete task;       // This will call on_terminate()
      delete node;
      task_count--;
    } else {
      curr = &(node->next);
    }
  }
  
  // 4. Pet the watchdog
  if (watchdog_enabled) {
    wdt_reset();
  }
}

void Scheduler::deliver() {
  // Process messages for each task
  TaskNode* curr = task_list;
  while (curr) {
    if (curr->task && curr->task->is_active()) {
      curr->task->process_messages();
    }
    curr = curr->next;
  }
}

SharedMsg Scheduler::get_next_message(uint8_t task_id) {
  SharedMsg msg;
  if (message_queue.empty()) return msg;
  
  // Check the next message
  SharedMsg peek;
  if (!message_queue.pop(peek)) return msg;
  
  if (peek->topic == 0) {
    // Direct message
    if (peek->src_id == task_id) {
      return peek;
    }
  } else {
    // Topic-based message
    TaskNode* target_node = find_task_node(task_id);
    if (target_node && target_node->task && target_node->task->is_subscribed_to(peek->topic)) {
      return peek;
    }
  }
  
  // Put it back if not for this task
  message_queue.push(peek);
  return msg;
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

TaskNode* Scheduler::find_task_node(uint8_t task_id) const {
    TaskNode* curr = task_list;
    while (curr) {
        if (curr->id == task_id) {
            return curr;
        }
        curr = curr->next;
    }
    return nullptr;
}

Task* Scheduler::get_task(uint8_t task_id) const {
    TaskNode* node = find_task_node(task_id);
    return node ? node->task : nullptr;
}

bool Scheduler::get_task_stats(uint8_t task_id, TaskStats& stats) const {
    TaskNode* node = find_task_node(task_id);
    if (node) {
        stats = node->stats;
        return true;
    }
    return false;
}

/* ================== Task Implementation ================== */

bool Task::tell(uint8_t dst_task_id, uint8_t type, uint16_t arg, void* ptr, bool is_dynamic) {
  return OS.post(type, dst_task_id, 0, arg, ptr, is_dynamic);
}

bool Task::publish(uint8_t topic, uint8_t type, uint16_t arg, void* ptr, bool is_dynamic) {
  if (topic == 0) return false; // Topic 0 is reserved for direct messages
  return OS.post(type, id, topic, arg, ptr, is_dynamic);
}

void Task::subscribe(uint8_t topic) {
  if (topic > 0 && subscription_count < subscription_capacity) {
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

void Task::activate() {
  if (state == SUSPENDED) {
    on_resume();
    // Will process any queued message on next scheduler loop
  }
  state = ACTIVE;
  next_due = OS.now() + period_ms;
}

void Task::suspend() {
  if (state == ACTIVE) {
    on_suspend();
    state = SUSPENDED;
    // Keep any queued message for when we resume
  }
}

void Task::terminate() {
  state = INACTIVE;
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

void Task::set_queue_messages_while_suspended(bool queue_messages) {
  queue_messages_while_suspended = queue_messages;
}

bool Task::get_queue_messages_while_suspended() const {
  return queue_messages_while_suspended;
}

void Task::process_messages() {
  if (state == ACTIVE) {
    // First process any queued messages from suspended state
    SharedMsg queued;
    while (suspended_msg_queue.pop(queued)) {
      on_msg(*queued.get());
    }
    
    // Then process new messages
    SharedMsg msg = OS.get_next_message(id);
    while (msg.valid()) {
      on_msg(*msg.get());
      msg = OS.get_next_message(id);
    }
  } 
  else if (state == SUSPENDED && queue_messages_while_suspended) {
    // Queue new messages while suspended
    SharedMsg msg = OS.get_next_message(id);
    while (msg.valid()) {
      suspended_msg_queue.push(msg);
      msg = OS.get_next_message(id);
    }
  }
  // If inactive or not queueing while suspended, messages are dropped
}