#include <Arduino.h>

#include "em_task.h"

EmTaskFuncRes task1Func(uint32_t* counter)
{
  *counter = *counter + 1;
  Serial.printf("Task 1: %d\n", static_cast<uint32_t>(*counter));
  if (*counter == 10) {
    return EmTaskFuncRes::pauseTask;
  }
  vTaskDelay(500 / portTICK_PERIOD_MS);
  return EmTaskFuncRes::continueTask;
}

uint32_t task1Counter(0);
EmTask<uint32_t> task1(&task1Counter, task1Func);


void setup() {
    Serial.begin(115200);
    Serial.println("Initializing...");
    task1.start();
}

void loop() {
    delay(1000);
    Serial.println("Looping...");
    if (task1.isPaused()) {
        Serial.println("Resuming Task 1");
        task1.start();
    }
    if (task1Counter >= 20 && task1.isNotStopped()) {
        Serial.printf("Stopping Task 1 [%d]\n", static_cast<uint32_t>(task1.status()));
        task1.stop();
    }
}
