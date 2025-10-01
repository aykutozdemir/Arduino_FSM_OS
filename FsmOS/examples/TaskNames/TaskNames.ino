#include <FsmOS.h>

using namespace FsmOS;

// Task that blinks LED with configurable period
class BlinkTask : public Task {
  const uint8_t pin;
  bool state;
  
public:
  BlinkTask(uint8_t pin, uint32_t period) 
    : Task(F("Blinker")), pin(pin), state(false) {  // Name stored in PROGMEM
    pinMode(pin, OUTPUT);
    set_period(period);
  }

protected:
  void step() override {
    state = !state;
    digitalWrite(pin, state);
  }

  void on_suspend() override {
    Serial.print(F("Task '")); 
    Serial.print(get_name());  // Uses task name from PROGMEM
    Serial.println(F("' suspended, LED off"));
    digitalWrite(pin, LOW);
  }

  void on_resume() override {
    Serial.print(F("Task '"));
    Serial.print(get_name());
    Serial.println(F("' resumed"));
    digitalWrite(pin, state);
  }

  void on_terminate() override {
    Serial.print(F("Task '"));
    Serial.print(get_name());
    Serial.println(F("' terminated"));
    digitalWrite(pin, LOW);
  }
};

// Stats monitor task that uses task names
class MonitorTask : public Task {
public:
  MonitorTask() : Task(F("Monitor")) {  // Name stored in PROGMEM
    set_period(5000);  // Report every 5 seconds
  }

protected:
  void step() override {
    Serial.println(F("\nTask Statistics:"));
    Serial.println(F("---------------"));
    
    // Iterate through all tasks
    for (uint8_t i = 0; i < OS.get_task_count(); i++) {
      Task* task = OS.get_task(i);
      if (task) {
        TaskStats stats;
        OS.get_task_stats(i, stats);
        
        // Print task info using stored name
        Serial.print(F("Task '"));
        Serial.print(task->get_name());  // Uses task name from PROGMEM
        Serial.print(F("' (ID:"));
        Serial.print(task->get_id());
        Serial.print(F("): "));
        
        // Print state
        switch(task->get_state()) {
          case Task::ACTIVE:
            Serial.print(F("ACTIVE"));
            break;
          case Task::SUSPENDED:
            Serial.print(F("SUSPENDED"));
            break;
          case Task::INACTIVE:
            Serial.print(F("INACTIVE"));
            break;
        }
        
        // Print stats
        Serial.print(F(", Runs: "));
        Serial.print(stats.run_count);
        Serial.print(F(", Max Time: "));
        Serial.print(stats.max_exec_time_us);
        Serial.println(F("us"));
      }
    }
  }
};

// Control task that manages other tasks' lifecycles
class ControlTask : public Task {
  Task* fast_blink;
  Task* slow_blink;
  uint32_t phase;
  
public:
  ControlTask() : Task(F("Controller")), phase(0) {  // Name stored in PROGMEM
    // Create tasks with descriptive names
    fast_blink = new BlinkTask(LED_BUILTIN, 200);     // Fast blink: 200ms
    slow_blink = new BlinkTask(LED_BUILTIN + 1, 1000); // Slow blink: 1s
    set_period(5000);  // Phase changes every 5 seconds
  }

protected:
  void step() override {
    switch(phase) {
      case 0:
        // Start with fast blink
        OS.add(fast_blink);
        Serial.print(F("Added task '"));
        Serial.print(fast_blink->get_name());
        Serial.println(F("'"));
        break;
        
      case 1:
        // Add slow blink
        OS.add(slow_blink);
        Serial.print(F("Added task '"));
        Serial.print(slow_blink->get_name());
        Serial.println(F("'"));
        break;
        
      case 2:
        // Suspend fast blink
        fast_blink->suspend();
        break;
        
      case 3:
        // Resume fast blink
        fast_blink->activate();
        break;
        
      case 4:
        // Terminate slow blink
        slow_blink->terminate();
        break;
        
      case 5:
        // Terminate fast blink and self
        fast_blink->terminate();
        this->terminate();
        break;
    }
    
    phase++;
  }
};

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println(F("Task Names Demo"));
  Serial.println(F("--------------"));
  
  // Add the monitor task first so it can observe others
  OS.add(new MonitorTask());
  
  // Add the control task
  OS.add(new ControlTask());
}

void loop() {
  OS.loop_once();
}