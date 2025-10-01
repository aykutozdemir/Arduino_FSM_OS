#include <FsmOS.h>

using namespace FsmOS;

// Task that blinks LED with configurable period
class BlinkTask : public Task {
  const uint8_t pin;
  bool state;
  
public:
  BlinkTask(uint8_t pin, uint32_t period) : Task(period), pin(pin), state(false) {
    pinMode(pin, OUTPUT);
  }

protected:
  void step() override {
    state = !state;
    digitalWrite(pin, state);
  }

  void on_suspend() override {
    // Turn off LED when suspended
    digitalWrite(pin, LOW);
  }

  void on_resume() override {
    // Resume from last state
    digitalWrite(pin, state);
  }

  void on_terminate() override {
    // Cleanup: set pin to LOW
    digitalWrite(pin, LOW);
  }
};

// Control task that manages other tasks' lifecycles
class ControlTask : public Task {
  Task* fastBlink;
  Task* slowBlink;
  uint32_t phase;
  
public:
  ControlTask() : Task(5000), phase(0) {
    // Create tasks but don't add them yet
    fastBlink = new BlinkTask(LED_BUILTIN, 200);  // Fast blink: 200ms
    slowBlink = new BlinkTask(LED_BUILTIN + 1, 1000);  // Slow blink: 1s
  }

protected:
  void step() override {
    switch(phase) {
      case 0:
        // Start with fast blink
        Scheduler::add_task(fastBlink);
        Serial.println("Phase 0: Started fast blink");
        break;
        
      case 1:
        // Add slow blink
        Scheduler::add_task(slowBlink);
        Serial.println("Phase 1: Added slow blink");
        break;
        
      case 2:
        // Suspend fast blink
        fastBlink->suspend();
        Serial.println("Phase 2: Suspended fast blink");
        break;
        
      case 3:
        // Resume fast blink
        fastBlink->activate();
        Serial.println("Phase 3: Resumed fast blink");
        break;
        
      case 4:
        // Terminate slow blink (will be auto-deleted)
        slowBlink->terminate();
        Serial.println("Phase 4: Terminated slow blink");
        break;
        
      case 5:
        // Terminate fast blink and self
        fastBlink->terminate();
        this->terminate();
        Serial.println("Phase 5: Terminated all tasks");
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
  
  Serial.println("Task Lifecycle Demo");
  Serial.println("------------------");
  Serial.println("This demo shows task state management:");
  Serial.println("1. Task creation and activation");
  Serial.println("2. Task suspension and resumption");
  Serial.println("3. Task termination and auto-cleanup");
  
  // Add the control task
  Scheduler::add_task(new ControlTask());
}

void loop() {
  Scheduler::loop_once();
}