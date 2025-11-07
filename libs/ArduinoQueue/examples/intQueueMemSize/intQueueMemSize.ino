/*
        Author: Vasco Baptista
        email: vascojdb@gmail.com

        An example of using Einar Arnason's queue
        In this example we create a queue of int's
        limited by the maximum amount of items and
        by the maximum size in bytes, limited by
        whatever comes first

        Usage and further info:
        https://github.com/EinarArnason/ArduinoQueue
*/

#include <ArduinoQueue.h>

#define QUEUE_SIZE_ITEMS 10
#define QUEUE_SIZE_BYTES 10

// Queue creation:
ArduinoQueue<int> intQueue(QUEUE_SIZE_ITEMS, QUEUE_SIZE_BYTES);

void printQueueStats()
{
  Serial.println("");
  Serial.print(F("Size of each element:    "));
  Serial.print(intQueue.itemSize());
  Serial.println(F(" bytes"));
  Serial.print(F("Items in queue now:      "));
  Serial.print(intQueue.itemCount());
  Serial.println(F(" items"));
  Serial.print(F("Queue actual max items:  "));
  Serial.print(intQueue.maxQueueSize());
  Serial.println(F(" items"));
  Serial.print(F("Queue actual max memory: "));
  Serial.print(intQueue.maxMemorySize());
  Serial.println(F(" bytes"));
  Serial.println("");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial.println(F("========== Queue example =========="));
  Serial.print(F("Desired max item size:  "));
  Serial.print(QUEUE_SIZE_ITEMS);
  Serial.println(F(" items"));
  Serial.print(F("Desired max queue size: "));
  Serial.print(QUEUE_SIZE_BYTES);
  Serial.println(F(" bytes"));
  Serial.println(F("==================================="));
}

void loop()
{
  printQueueStats();

  // Add elements: (add more than the queue size for demo purposes)
  for (int n = 1; n < QUEUE_SIZE_ITEMS + 5; n++)
  {
    if (!intQueue.isFull())
    {
      Serial.print(F("Adding value: "));
      Serial.println(n);
      intQueue.enqueue(n);
    }
    else
    {
      Serial.println("Queue is full!");
    }
  }

  printQueueStats();

  // Remove elements: (remove more than the queue size for demo purposes)
  for (int n = 1; n < QUEUE_SIZE_ITEMS + 5; n++)
  {
    if (!intQueue.isEmpty())
    {
      int value = intQueue.dequeue();
      Serial.print(F("Removed value: "));
      Serial.println(value);
    }
    else
    {
      Serial.println("Queue is empty!");
    }
  }

  printQueueStats();

  // Loop forever
  while (true)
  {
#ifdef ESP8266
    ESP.wdtFeed();
#endif
  }
}
