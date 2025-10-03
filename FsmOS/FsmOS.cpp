/**
 * @file FsmOS.cpp
 * @brief Implementation of the FsmOS cooperative task scheduler
 * @author Aykut Ozdemir
 * @date 2025-10-02
 * 
 * This file contains the implementation of the FsmOS scheduler and task
 * management system. It provides:
 * 
 * - Task scheduling and execution
 * - Message passing and event handling
 * - Memory management and monitoring
 * - System diagnostics and profiling
 * - Hardware watchdog integration
 */

#include "FsmOS.h"

/* ================== Memory Tracking ================== */
#if !defined(FSMOS_DISABLE_LEAK_DETECTION)
MemoryStats fsmos_memory_stats = {0, 0, 0, 0};
#endif

/* ================== Global OS Instance ================== */
Scheduler OS;

/* ================== SharedMsg Implementation ================== */
void SharedMsg::release() {
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
      OS.deallocate_message(data);
    }
    data = nullptr;
  }
}

/* ================== Reset Info ================== */
// This struct will be placed in the .noinit section, so it survives a reset.
__attribute__((section(".noinit")))
ResetInfo reset_info;


/* ================== Scheduler Implementation ================== */

/**
 * @brief Initialize the scheduler
 * 
 * Performs initial setup of the scheduler including:
 * - Capturing reset reason (AVR only)
 * - Initializing task management
 * - Setting up system time
 * - Preparing watchdog status
 */
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

// Optional: initialize with logger (alias to begin for now)
void Scheduler::begin_with_logger() {
  begin();
}

void Scheduler::_print_log_prefix(Task* task, LogLevel level) {
#ifndef FSMOS_DISABLE_LOGGING
  static const char PROGMEM levelChars[] = {'D', 'I', 'W', 'E'};
  
  uint32_t t = now();
  Serial.print('[');
  Serial.print(t / 1000);
  Serial.print(':');
  Serial.print(t % 1000);
  Serial.print(F("]["));
  Serial.write(pgm_read_byte(&levelChars[level]));
  Serial.print(F("]["));
  if (task && task->get_name()) {
    Serial.print(task->get_name());
  } else {
    Serial.write('-');
  }
  Serial.print(']');
  Serial.print(' ');
#endif
}

void Scheduler::logMessage(Task* task, LogLevel level, const __FlashStringHelper* msg) {
#ifndef FSMOS_DISABLE_LOGGING
  _print_log_prefix(task, level);
  Serial.println(msg);
#endif
}

void Scheduler::logFormatted(Task* task, LogLevel level, const __FlashStringHelper* fmt, ...) {
#ifndef FSMOS_DISABLE_LOGGING
  _print_log_prefix(task, level);
  char fmt_buf[64];
  uint8_t i = 0;
  const char* p = reinterpret_cast<const char*>(fmt);
  while (i < sizeof(fmt_buf) - 1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    fmt_buf[i++] = c;
  }
  fmt_buf[i] = 0;

  char out_buf[64];
  va_list args;
  va_start(args, fmt);
  vsnprintf(out_buf, sizeof(out_buf), fmt_buf, args);
  va_end(args);
  Serial.println(out_buf);
#endif
}


/**
 * @brief Add a new task to the scheduler
 * 
 * This method:
 * - Assigns a unique ID to the task
 * - Creates a new task node
 * - Initializes task state
 * - Links task into task list
 * - Calls task's start handler
 * 
 * @param t Pointer to the task to add
 * @return Assigned task ID (255 if failed)
 */
uint8_t Scheduler::add(Task* t) {
  uint8_t new_id = next_task_id++;
  if (next_task_id == 0) {
    next_task_id = 1;
  }

  TaskNode* new_node = new TaskNode(t, new_id);
  if (!new_node) {
    return 255;
  }
  
  t->id = new_id;
  t->state = Task::ACTIVE;
  t->next_due = ms;

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
      Task* task = to_delete->task;
      *curr = to_delete->next;
      delete task;
      delete to_delete;
      task_count--;
      return true;
    }
    curr = &((*curr)->next);
  }
  return false;
}

bool Scheduler::post(uint8_t type, uint8_t src_id, uint8_t topic, uint8_t dst_id, uint16_t arg, void* ptr, bool is_dynamic) {
  MsgData* data = msg_pool.allocate();
  if (!data) return false;
  
  data->type = type;
  data->src_id = src_id;
  data->topic = topic;
  data->dst_id = dst_id;
  data->arg = arg;
  data->ptr = ptr;
  data->is_dynamic = is_dynamic;
  data->dynamic_size = 0;
  
  uint8_t target_count = 0;
  if (topic == 0) {
    TaskNode* target_node = find_task_node(dst_id);
    if (target_node && target_node->task) target_count = 1;
  } else {
    TaskNode* curr = task_list;
    while (curr) {
      if (curr->task && curr->task->is_subscribed_to(topic)) target_count++;
      curr = curr->next;
    }
  }
  
  data->ref_count = (target_count > 0) ? (target_count - 1) : 0;
  if (target_count == 0) {
    msg_pool.deallocate(data);
    return false;
  }
  
  return message_queue.push(SharedMsg(data));
}

/**
 * @brief Execute one iteration of the scheduler
 * 
 * This is the core scheduling loop that:
 * 1. Updates system time
 * 2. Delivers pending messages
 * 3. Executes due tasks
 * 4. Handles task cleanup
 * 5. Updates task statistics
 * 6. Manages watchdog
 * 
 * The scheduler ensures:
 * - Tasks run in their configured periods
 * - Messages are delivered promptly
 * - Resources are cleaned up
 * - System remains responsive
 */
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
#if defined(__AVR__)
  if (watchdog_enabled) {
    wdt_reset();
  }
#endif
  
  msg_pool.update_adaptive_limit(now);
}

/**
 * @brief Deliver pending messages to tasks
 * 
 * Iterates through all active tasks and processes their pending
 * messages. For each task:
 * - Checks if task is active
 * - Processes queued messages
 * - Handles message cleanup
 * 
 * This method ensures messages are delivered in a fair manner
 * and maintains system responsiveness.
 */
void Scheduler::deliver() {
  // Process all messages in the queue
  while (!message_queue.empty()) {
    SharedMsg msg;
    if (!message_queue.pop(msg)) break;
    
    if (msg->topic == 0) {
      TaskNode* target_node = find_task_node(msg->dst_id);
      if (target_node && target_node->task && target_node->task->is_active()) {
        target_node->task->on_msg(*msg.get());
      }
    } else {
      // Topic-based message - deliver to all subscribed tasks
      TaskNode* curr = task_list;
      while (curr) {
        if (curr->task && curr->task->is_active() && curr->task->is_subscribed_to(msg->topic)) {
          curr->task->on_msg(*msg.get());
        }
        curr = curr->next;
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

#if defined(__AVR__)
// Memory markers
static const uint8_t HEAP_FREE_MARKER = 0xFF;
static const uint16_t STACK_CANARY = 0xFEED;
#endif

/**
 * @brief Get memory usage information for a task
 * 
 * Calculates memory used by a task including:
 * - Task object size
 * - Subscription array size
 * - Message queue size
 * - Total allocated memory
 * 
 * @param task_id ID of task to analyze
 * @param info Reference to store memory info
 * @return true if task was found and info populated
 */
bool Scheduler::get_task_memory_info(uint8_t task_id, TaskMemoryInfo& info) const {
    TaskNode* node = find_task_node(task_id);
    if (!node || !node->task) return false;

    Task* task = node->task;
    info.task_struct_size = sizeof(Task);
    info.subscription_size = task->subscription_count;
    info.queue_size = sizeof(LinkedQueue<SharedMsg>);
    
    // Calculate total allocated memory
    info.total_allocated = info.task_struct_size + 
                          info.subscription_size +
                          info.queue_size;
    
    return true;
}

bool Scheduler::get_system_memory_info(SystemMemoryInfo& info) const {
#if defined(__AVR__)
    extern int __heap_start, *__brkval;
    extern uint8_t __data_start, __data_end;
    extern uint8_t __bss_start, __bss_end;
    extern uint8_t _etext, __stack;
    
    // Static Memory
    info.total_ram = RAMEND - RAMSTART + 1;
    info.static_data_size = (&__data_end - &__data_start) + (&__bss_end - &__bss_start);
    info.heap_size = (__brkval == 0 ? (uint16_t)&__heap_start : (uint16_t)__brkval) - (uint16_t)&__heap_start;
    
    // Dynamic Memory
    info.free_ram = get_free_memory();
    info.heap_fragments = count_heap_fragments();
    info.largest_block = get_largest_block();
    
    // Task Memory
    info.total_tasks = task_count;
    info.task_memory = task_count * (sizeof(TaskNode) + sizeof(Task));
    
    // Message Memory
    uint8_t msg_count = 0;
    uint16_t msg_mem = 0;
    TaskNode* curr = task_list;
    while (curr) {
        if (curr->task) {
            msg_count += curr->task->suspended_msg_queue.size();
            msg_mem += sizeof(MsgData) * curr->task->suspended_msg_queue.size();
        }
        curr = curr->next;
    }
    info.active_messages = msg_count;
    info.message_memory = msg_mem;
    
    // Stack Memory
    info.stack_size = RAMEND - (uint16_t)&__stack;
    uint16_t* p = (uint16_t*)RAMEND;
    while (p > (uint16_t*)&__stack && *p == STACK_CANARY) {
        p--;
    }
    info.stack_free = (uint16_t)RAMEND - (uint16_t)p;
    info.stack_used = info.stack_size - info.stack_free;
    
    // Program Memory
    info.flash_used = (uint32_t)&_etext;
    info.flash_free = (uint32_t)FLASHEND - info.flash_used;
    
    return true;
#else
    // Non-AVR implementation
    memset(&info, 0, sizeof(info));
    return false;
#endif
}

uint16_t Scheduler::get_free_memory() const {
#if defined(__AVR__)
    extern int __heap_start, *__brkval;
    int v;
    return (uint16_t)&v - (__brkval == 0 ? (uint16_t)&__heap_start : (uint16_t)__brkval);
#else
    return 0;
#endif
}

uint16_t Scheduler::get_largest_block() const {
#if defined(__AVR__)
    // Simplified implementation - return free memory as largest block
    // This is not perfect but works for basic monitoring
    return get_free_memory();
#else
    return 0;
#endif
}

uint8_t Scheduler::get_heap_fragmentation() const {
#if defined(__AVR__)
    uint16_t total_free = get_free_memory();
    uint16_t largest = get_largest_block();
    
    if (total_free == 0) return 100;
    return (uint8_t)(100 - (largest * 100) / total_free);
#else
    return 0;
#endif
}

uint8_t Scheduler::count_heap_fragments() const {
#if defined(__AVR__)
    // Simplified implementation - return 1 for basic monitoring
    // This is not perfect but works for basic monitoring
    return 1;
#else
    return 0;
#endif
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

/**
 * @brief Send a direct message to another task
 * 
 * @param dst_task_id Destination task ID
 * @param type Message type
 * @param arg Optional 16-bit argument
 * @param ptr Optional data pointer
 * @param is_dynamic Whether ptr is dynamically allocated
 * @return true if message was queued successfully
 */
bool Task::tell(uint8_t dst_task_id, uint8_t type, uint16_t arg, void* ptr, bool is_dynamic) {
  return OS.post(type, this->id, 0, dst_task_id, arg, ptr, is_dynamic);
}

/**
 * @brief Publish a message to a topic
 * 
 * @param topic Topic ID (1-255)
 * @param type Message type
 * @param arg Optional 16-bit argument
 * @param ptr Optional data pointer
 * @param is_dynamic Whether ptr is dynamically allocated
 * @return true if message was queued successfully
 */
bool Task::publish(uint8_t topic, uint8_t type, uint16_t arg, void* ptr, bool is_dynamic) {
  if (topic == 0) return false;
  return OS.post(type, id, topic, 0, arg, ptr, is_dynamic);
}

/**
 * @brief Subscribe to a topic
 * 
 * Allows task to receive messages published to the specified topic.
 * 
 * @param topic Topic ID to subscribe to (1-255)
 */
void Task::subscribe(uint8_t topic) {
  if (topic == 0) return; // topic 0 reserved
  // Avoid duplicates
  SubNode* curr = subscription_head;
  while (curr) {
    if (curr->topic == topic) return;
    curr = curr->next;
  }
  // Prepend new subscription
  SubNode* node = new SubNode(topic);
  node->next = subscription_head;
  subscription_head = node;
  if (subscription_count < 255) subscription_count++;
  
}

/**
 * @brief Check if task is subscribed to a topic
 * 
 * @param topic Topic ID to check
 * @return true if task is subscribed to topic
 */
bool Task::is_subscribed_to(uint8_t topic) {
  if (topic == 0) return false;
  SubNode* curr = subscription_head;
  while (curr) {
    if (curr->topic == topic) return true;
    curr = curr->next;
  }
  return false;
}

/**
 * @brief Activate or resume task execution
 * 
 * If task was suspended:
 * - Calls on_resume() handler
 * - Processes queued messages
 * - Schedules next execution
 */
void Task::activate() {
  if (state == SUSPENDED) {
    on_resume();
    // Will process any queued message on next scheduler loop
  }
  state = ACTIVE;
  next_due = OS.now() + period_ms;
}

/**
 * @brief Suspend task execution
 * 
 * When suspending:
 * - Calls on_suspend() handler
 * - Preserves message queue
 * - Stops task execution
 */
void Task::suspend() {
  if (state == ACTIVE) {
    on_suspend();
    state = SUSPENDED;
    // Keep any queued message for when we resume
  }
}

/**
 * @brief Permanently terminate task
 * 
 * Marks task for removal by scheduler.
 * Task will be deleted and resources freed
 * on next scheduler iteration.
 */
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

void Scheduler::enable_stack_monitoring() {
#if defined(__AVR__)
  extern uint8_t __stack;
  uint16_t* p = (uint16_t*)&__stack;
  uint16_t* stack_end = (uint16_t*)RAMEND;
  while (p < stack_end) {
    *p++ = STACK_CANARY;
  }
#endif
}

int Scheduler::get_free_stack() const {
#if defined(__AVR__)
  extern uint8_t __stack;
  uint16_t* p = (uint16_t*)RAMEND;
  while (p > (uint16_t*)&__stack && *p == STACK_CANARY) {
    p--;
  }
  return (int)((uint16_t)RAMEND - (uint16_t)p);
#else
  return 0;
#endif
}

void Task::process_messages() {
  if (!suspended_msg_queue.empty() && is_active()) {
    while (!suspended_msg_queue.empty()) {
      SharedMsg msg;
      if (!suspended_msg_queue.pop(msg)) break;
      on_msg(*msg.get());
    }
  }
}

void Scheduler::deallocate_message(MsgData* data) {
  msg_pool.deallocate(data);
}
