#include <FsmOS.h>

using namespace FsmOS;

// Example task that generates various log messages
class WorkerTask : public Task {
  uint8_t counter;
  Timer delay_timer;
  
public:
  WorkerTask() : Task(F("Worker")), counter(0) {
    set_period(2000);  // Generate log messages every 2 seconds
  }

  void on_start() override {
    log_message(this, LOG_INFO, F("Worker task started"));
    delay_timer.start(10000);  // Set timer for 10 seconds
  }

protected:
  void step() override {
    counter++;
    
    // Generate different types of log messages
    switch (counter % 4) {
      case 0:
        log_message(this, LOG_DEBUG, F("Processing routine data"));
        break;
        
      case 1:
        log_message(this, LOG_INFO, F("Data processing complete"));
        break;
        
      case 2:
        log_message(this, LOG_WARNING, F("High memory usage detected"));
        break;
        
      case 3:
        log_message(this, LOG_ERROR, F("Failed to process data"));
        break;
    }
    
    // Example of checking conditions and logging
    if (delay_timer.expired()) {
      log_message(this, LOG_WARNING, F("Operation taking longer than expected"));
      delay_timer.start(10000);  // Reset timer
    }
  }
  
  void on_suspend() override {
    log_message(this, LOG_INFO, F("Worker task suspended"));
  }
  
  void on_resume() override {
    log_message(this, LOG_INFO, F("Worker task resumed"));
  }
  
  void on_terminate() override {
    log_message(this, LOG_INFO, F("Worker task terminated"));
  }
};

// Task that controls worker lifecycle
class ControlTask : public Task {
  WorkerTask* worker;
  uint8_t phase;
  
public:
  ControlTask() : Task(F("Control")), phase(0) {
    worker = new WorkerTask();
    set_period(8000);  // Change worker state every 8 seconds
  }

protected:
  void step() override {
    switch(phase) {
      case 0:
        log_message(this, LOG_INFO, F("Starting worker task"));
        OS.add(worker);
        break;
        
      case 1:
        log_message(this, LOG_INFO, F("Suspending worker task"));
        worker->suspend();
        break;
        
      case 2:
        log_message(this, LOG_INFO, F("Resuming worker task"));
        worker->activate();
        break;
        
      case 3:
        log_message(this, LOG_INFO, F("Terminating worker task"));
        worker->terminate();
        log_message(this, LOG_INFO, F("Control task terminating"));
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
  
  // Initialize OS with built-in logger
  OS.begin_with_logger();
  
  // Add control task
  OS.add(new ControlTask());
}

void loop() {
  OS.loop_once();
}