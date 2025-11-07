#include <FsmOS.h>

Mutex serial_mutex;
int shared_counter = 0;

class ProducerTask : public Task {
public:
  ProducerTask() : Task(F("Producer")) {
    setPeriod(1000);
  }
  
  void step() override {
    if (serial_mutex.tryLock(getId())) {
      shared_counter++;
      Serial.print(F("Producer: counter = "));
      Serial.println(shared_counter);
      serial_mutex.unlock(getId());
    }
  }
};

class ConsumerTask : public Task {
public:
  ConsumerTask() : Task(F("Consumer")) {
    setPeriod(1500);
  }
  
  void step() override {
    if (serial_mutex.tryLock(getId())) {
      if (shared_counter > 0) {
        shared_counter--;
        Serial.print(F("Consumer: counter = "));
        Serial.println(shared_counter);
      }
      serial_mutex.unlock(getId());
    }
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
  OS.loopOnce();
}
