#include <FsmOS.h>

Mutex serial_mutex;
int shared_counter = 0;

class ProducerTask : public Task {
public:
  ProducerTask() : Task(F("Producer")) {
    set_period(1000);
  }
  
  void step() override {
    serial_mutex.lock(get_id());
    
    shared_counter++;
    Serial.print(F("Producer: counter = "));
    Serial.println(shared_counter);
    
    serial_mutex.unlock(get_id());
  }
};

class ConsumerTask : public Task {
public:
  ConsumerTask() : Task(F("Consumer")) {
    set_period(1500);
  }
  
  void step() override {
    serial_mutex.lock(get_id());
    
    if (shared_counter > 0) {
      shared_counter--;
      Serial.print(F("Consumer: counter = "));
      Serial.println(shared_counter);
    }
    
    serial_mutex.unlock(get_id());
  }
};

ProducerTask producer;
ConsumerTask consumer;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  OS.begin();
  OS.add(&producer);
  OS.add(&consumer);
  
  Serial.println(F("Mutex example started"));
}

void loop() {
  OS.loop_once();
}
