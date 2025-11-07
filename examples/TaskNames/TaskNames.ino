#include <FsmOS.h>



// Task that blinks LED with configurable period
class BlinkTask : public Task {
  const uint8_t pin;
  bool state;
  
public:
  BlinkTask(uint8_t pin, uint32_t period) 
    : Task(F("Blinker")), pin(pin), state(false) {  // Name stored in PROGMEM
    pinMode(pin, OUTPUT);
    setPeriod(period);
  }

protected:
  void step() override {
    state = !state;
    digitalWrite(pin, state);
  }
  
  uint8_t getMaxMessageBudget() const override { return 0; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
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
    setPeriod(5000);  // Phase changes every 5 seconds
  }

protected:
  void step() override {
    switch(phase) {
      case 0:
        // Start with fast blink
        OS.add(fast_blink);
        Serial.print(F("Added task '"));
        Serial.print(F("Task"));
        Serial.println(F("'"));
        break;
        
      case 1:
        // Add slow blink
        OS.add(slow_blink);
        Serial.print(F("Added task '"));
        Serial.print(F("Task"));
        Serial.println(F("'"));
        break;
        
      case 2:
        // Suspend fast blink
        fast_blink->suspend();
        break;
        
      case 3:
        // Resume fast blink
        fast_blink->resume();
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
  
  uint8_t getMaxMessageBudget() const override { return 0; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};

/**
 * MonitorTask: Observes and reports task names
 */
class MonitorTask : public Task {
public:
  MonitorTask() : Task(F("Monitor")) {
    setPeriod(3000);  // Report every 3 seconds
  }
  
protected:
  void step() override {
    Serial.println(F("\n=== Current Tasks ==="));
    Serial.print(F("Total tasks: "));
    Serial.println(OS.getTaskCount());
    Serial.println(F("Task names are stored in PROGMEM to save RAM"));
  }
  
  uint8_t getMaxMessageBudget() const override { return 0; }
  uint16_t getTaskStructSize() const override { return sizeof(*this); }
};

ControlTask controller;

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println(F("Task Names Demo"));
  Serial.println(F("--------------"));
  
  OS.begin();
  
  // Add the monitor task first so it can observe others
  OS.add(new MonitorTask());
  
  // Add the control task
  OS.add(&controller);
}

void loop() {
  OS.loopOnce();
}