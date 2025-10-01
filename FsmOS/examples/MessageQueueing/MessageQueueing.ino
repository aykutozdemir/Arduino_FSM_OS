#include <FsmOS.h>

using namespace FsmOS;

// Message types
enum {
  MSG_TOGGLE = 1,
  MSG_COUNT,
  MSG_REPORT
};

// LED task that can be suspended and receives toggle messages
class LedTask : public Task {
  const uint8_t pin;
  bool state;
  uint32_t toggle_count;
  
public:
  LedTask(uint8_t pin) : Task(), pin(pin), state(false), toggle_count(0) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
    set_period(100);  // Fast updates to process messages quickly
  }

protected:
  void step() override {
    // Just handle messages, no automatic toggling
  }
  
  void on_msg(const MsgData& msg) override {
    switch(msg.type) {
      case MSG_TOGGLE:
        state = !state;
        digitalWrite(pin, state);
        toggle_count++;
        break;
        
      case MSG_REPORT:
        static const char PROGMEM report_prefix[] = "LED pin ";
        static const char PROGMEM report_middle[] = " toggled ";
        static const char PROGMEM report_suffix[] = " times";
        
        Serial.print((__FlashStringHelper*)report_prefix);
        Serial.print(pin);
        Serial.print((__FlashStringHelper*)report_middle);
        Serial.print(toggle_count);
        Serial.println((__FlashStringHelper*)report_suffix);
        break;
    }
  }
  
  void on_suspend() override {
    static const char PROGMEM status_prefix[] = "LED pin ";
    static const char PROGMEM suspended_str[] = " suspended";
    
    Serial.print((__FlashStringHelper*)status_prefix);
    Serial.print(pin);
    Serial.println((__FlashStringHelper*)suspended_str);
  }
  
  void on_resume() override {
    static const char PROGMEM resumed_str[] = " resumed";
    
    Serial.print((__FlashStringHelper*)status_prefix);  // Reuse prefix from suspend
    Serial.print(pin);
    Serial.println((__FlashStringHelper*)resumed_str);
  }
};

// Generator task that sends toggle messages to LEDs
class GeneratorTask : public Task {
  Task* led1;
  Task* led2;
  uint32_t count;
  
public:
  GeneratorTask(Task* l1, Task* l2) : Task(), led1(l1), led2(l2), count(0) {
    set_period(1000);  // Send toggle message every second
  }

protected:
  void step() override {
    count++;
    
    // Send toggle messages to both LEDs
    led1->tell(led1->get_id(), MSG_TOGGLE);
    led2->tell(led2->get_id(), MSG_TOGGLE);
    
    if (count % 5 == 0) {  // Every 5 seconds
      // Send report request
      led1->tell(led1->get_id(), MSG_REPORT);
      led2->tell(led2->get_id(), MSG_REPORT);
      
      // Toggle suspension state
      if (led1->is_active()) {
        led1->suspend();
        led2->suspend();
      } else {
        led1->activate();
        led2->activate();
      }
    }
  }
};

// Create tasks
LedTask* led1;
LedTask* led2;
GeneratorTask* gen;

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    ; // Wait for serial port to connect
  }
  
  static const char PROGMEM demo_title[] = "Message Queueing Demo";
  static const char PROGMEM demo_line[] = "-------------------";
  static const char PROGMEM msg_queue_info[] = "LED 1: Messages are queued while suspended";
  static const char PROGMEM msg_drop_info[] = "LED 2: Messages are dropped while suspended";
  static const char PROGMEM timing_info[] = "Every 5 seconds, LEDs will be suspended/resumed";
  
  Serial.println((__FlashStringHelper*)demo_title);
  Serial.println((__FlashStringHelper*)demo_line);
  
  // Create LED tasks
  led1 = new LedTask(LED_BUILTIN);
  led2 = new LedTask(LED_BUILTIN + 1);
  
  // Configure message queueing differently for each LED
  led1->set_queue_messages_while_suspended(true);   // Will queue messages
  led2->set_queue_messages_while_suspended(false);  // Will drop messages
  
  // Create and add generator task
  gen = new GeneratorTask(led1, led2);
  
  // Add all tasks to scheduler
  Scheduler::add_task(led1);
  Scheduler::add_task(led2);
  Scheduler::add_task(gen);
  
  Serial.println((__FlashStringHelper*)msg_queue_info);
  Serial.println((__FlashStringHelper*)msg_drop_info);
  Serial.println((__FlashStringHelper*)timing_info);
}

void loop() {
  Scheduler::loop_once();
}