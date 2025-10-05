#include <FsmOS.h>

Semaphore resource_pool(2);

class WorkerTask : public Task {
private:
  uint8_t worker_id;
  enum State { IDLE, WAITING, WORKING } state;
  Timer16 work_timer;  // 2000ms work timing - 4 bytes
  
public:
  WorkerTask(uint8_t id) : Task(), worker_id(id), state(IDLE) {
    set_period(100);
  }
  
  void on_start() override {
    char name[16];
    snprintf(name, sizeof(name), "Worker%d", worker_id);
    Serial.println(name);
  }
  
  void step() override {
    switch (state) {
      case IDLE:
        Serial.print(F("Worker "));
        Serial.print(worker_id);
        Serial.println(F(" requesting resource..."));
        
        if (resource_pool.try_wait(get_id())) {
          state = WORKING;
          work_timer = create_timer_typed<Timer16>(2000);
          Serial.print(F("Worker "));
          Serial.print(worker_id);
          Serial.println(F(" got resource!"));
        } else {
          resource_pool.wait(get_id());
          state = WAITING;
        }
        break;
        
      case WAITING:
        state = WORKING;
        work_timer = create_timer_typed<Timer16>(2000);
        Serial.print(F("Worker "));
        Serial.print(worker_id);
        Serial.println(F(" reactivated with resource!"));
        break;
        
      case WORKING:
        if (work_timer.expired()) {
          Serial.print(F("Worker "));
          Serial.print(worker_id);
          Serial.println(F(" releasing resource"));
          resource_pool.signal();
          state = IDLE;
          set_period(random(1000, 3000));
        }
        break;
    }
  }
};

WorkerTask worker1(1);
WorkerTask worker2(2);
WorkerTask worker3(3);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  OS.begin();
  OS.add(&worker1);
  OS.add(&worker2);
  OS.add(&worker3);
  
  Serial.println(F("Semaphore example: 3 workers, 2 resources"));
}

void loop() {
  OS.loop_once();
}
