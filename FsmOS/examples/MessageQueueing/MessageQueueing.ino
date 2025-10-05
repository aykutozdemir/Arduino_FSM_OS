/*
  MessageQueueing - Task Suspension and Message Handling Example
  
  This example demonstrates FsmOS's message queueing capabilities during task suspension:
  1. How tasks can queue or drop messages while suspended
  2. How to implement task suspension and resumption
  3. Different message handling strategies during task suspension
  
  The sketch creates three tasks:
  - Two LED tasks that respond to messages differently when suspended:
    * LED1: Queues messages while suspended (processes them when resumed)
    * LED2: Drops messages while suspended (ignores them completely)
  - A Generator task that sends periodic messages to both LEDs
  
  Every 5 seconds, both LED tasks are suspended/resumed to demonstrate
  the different message handling behaviors.
  
  The circuit:
  - LED1 connected to built-in LED (usually pin 13)
  - LED2 connected to built-in LED + 1 (usually pin 14)
  
  Created October 2, 2025
  By Aykut Ozdemir
  
  https://github.com/aykutozdemir/Arduino_FSM_OS
*/

#include <FsmOS.h>

/* ================== Message Types ================== */
enum : uint8_t {
  MSG_TOGGLE = 1,  // Toggle LED state
  MSG_COUNT,       // Request message count
  MSG_REPORT      // Request status report
};

/**
 * LedTask: Controls an LED and handles messages during suspension
 * 
 * This task demonstrates:
 * - Message handling during active and suspended states
 * - Lifecycle callbacks (suspend/resume)
 * - Message counting and reporting
 */
class LedTask : public Task {
  const uint8_t pin;          // LED pin number
  bool state;                 // Current LED state
  uint32_t toggle_count;      // Number of times LED has been toggled
  uint32_t dropped_msgs;      // Count of messages dropped while suspended
  const char* task_name;      // Name for this LED task instance
  
public:
  LedTask(uint8_t led_pin, const char* name) 
    : Task(F(name)),
      pin(led_pin), 
      state(false), 
      toggle_count(0),
      dropped_msgs(0),
      task_name(name)
  {
    set_period(100);  // Fast updates to process messages quickly
  }

  void on_start() override {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
    log_info(F("%s initialized on pin %d"), task_name, pin);
  }

  void step() override {
    // This task is purely event-driven via messages
  }
  
  void on_msg(const MsgData& msg) override {
    switch(msg.type) {
      case MSG_TOGGLE:
        // Toggle LED state
        state = !state;
        digitalWrite(pin, state);
        toggle_count++;
        log_debug(F("%s toggled %d times"), task_name, toggle_count);
        break;
        
      case MSG_REPORT:
        // Print status report
        log_info(F("%s Status:"), task_name);
        log_info(F("  Pin: %d"), pin);
        log_info(F("  Toggles: %d"), toggle_count);
        log_info(F("  Dropped msgs: %d"), dropped_msgs);
        log_info(F("  Queue msgs: %s"), 
                get_queue_messages_while_suspended() ? "Yes" : "No");
        break;
    }
  }
  
  void on_suspend() override {
    log_warn(F("%s suspended"), task_name);
    digitalWrite(pin, LOW);  // Turn off LED while suspended
  }
  
  void on_resume() override {
    log_info(F("%s resumed"), task_name);
    digitalWrite(pin, state);  // Restore LED state
  }
  
  void increment_dropped() {
    dropped_msgs++;
  }
};

/**
 * GeneratorTask: Sends periodic messages to LED tasks
 * 
 * This task demonstrates:
 * - Sending messages to other tasks
 * - Controlling task lifecycle (suspend/resume)
 * - Periodic task execution
 */
class GeneratorTask : public Task {
  LedTask* led1;      // LED task that queues messages
  LedTask* led2;      // LED task that drops messages
  uint32_t count;     // Cycle counter
  Timer16 cycle_timer; // 5000ms cycle timing - 4 bytes
  
public:
  GeneratorTask(LedTask* l1, LedTask* l2) 
    : Task(F("Generator")), 
      led1(l1), 
      led2(l2), 
      count(0) 
  {
    set_period(1000);  // Send toggle message every second
  }

  void on_start() override {
    log_info(F("Message generator started"));
    log_info(F("  Sending toggles every 1 second"));
    log_info(F("  Suspend/resume cycle every 5 seconds"));
    cycle_timer = create_timer_typed<Timer16>(5000);  // Start 5-second cycle timer
  }

  void step() override {
    count++;
    
    // Send toggle messages to both LEDs
    send_toggle_messages();
    
    // Check if it's time for a cycle change
    if (cycle_timer.expired()) {
      cycle_timer.start(5000);
      handle_cycle_change();
    }
  }

private:
  void send_toggle_messages() {
    // Send toggle messages and log results
    bool msg1_sent = led1->tell(led1->get_id(), MSG_TOGGLE);
    bool msg2_sent = led2->tell(led2->get_id(), MSG_TOGGLE);
    
    // Log message delivery status
    log_debug(F("Toggle messages: LED1=%s, LED2=%s"),
             msg1_sent ? "sent" : "failed",
             msg2_sent ? "sent" : "failed");
  }
  
  void handle_cycle_change() {
    log_info(F("\n=== Cycle %d Complete ==="), count / 5);
    
    // Request status reports
    led1->tell(led1->get_id(), MSG_REPORT);
    led2->tell(led2->get_id(), MSG_REPORT);
    
    // Toggle suspension state
    if (led1->is_active()) {
      log_info(F("Suspending LED tasks..."));
      led1->suspend();
      led2->suspend();
    } else {
      log_info(F("Resuming LED tasks..."));
      led1->activate();
      led2->activate();
    }
  }
};

/* ================== Application Setup ================== */

// Task instances
LedTask* led1;        // LED that queues messages while suspended
LedTask* led2;        // LED that drops messages while suspended
GeneratorTask* gen;   // Task that generates messages

void setup() {
  // Initialize serial and wait for connection
  Serial.begin(115200);
  while (!Serial) { /* wait for serial port */ }
  
  Serial.println(F("\n=== Message Queueing Demo ==="));
  Serial.println(F("Version: 1.2.0"));
  
  // Initialize the OS with logging enabled
  OS.begin_with_logger();
  
  // Create LED tasks with different message handling strategies
  led1 = new LedTask(LED_BUILTIN, "LED1 (Queuing)");
  led2 = new LedTask(LED_BUILTIN + 1, "LED2 (Dropping)");
  
  // Configure message handling behavior
  led1->set_queue_messages_while_suspended(true);   // Queue messages
  led2->set_queue_messages_while_suspended(false);  // Drop messages
  
  // Create message generator
  gen = new GeneratorTask(led1, led2);
  
  // Add all tasks to scheduler
  OS.add(led1);
  OS.add(led2);
  OS.add(gen);
  
  // Print demo information
  Serial.println(F("\nDemo Features:"));
  Serial.println(F("1. LED1 queues messages while suspended"));
  Serial.println(F("2. LED2 drops messages while suspended"));
  Serial.println(F("3. Both LEDs toggle every 1 second"));
  Serial.println(F("4. Tasks suspend/resume every 5 seconds"));
  Serial.println(F("5. Status reports show message handling differences"));
  Serial.println(F("\nDemo starting...\n"));
}

void loop() {
  OS.loop_once();  // Run the cooperative scheduler
}