#include "em_defs.h"

#ifdef  ESP_PLATFORM
// This enables the Arduino style setup() and loop() application 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern void setup();
extern void loop();

void vLoopTask(void *pvParameters) {
    setup();
    while (true) {
        loop();           
        vTaskDelay(1); 
    }
}

extern "C" {
    void app_main(void) {
    #ifdef EM_MULTICORE
        xTaskCreatePinnedToCore(
            vLoopTask, 
            "main_loop_task", 
            8192, 
            nullptr, 
            1, // Priority 1
            nullptr, 
            static_cast<BaseType_t>(EmCoreId::coreUserTask)
        );
    #else
        vLoopTask(nullptr);
    #endif
    }
}      

#endif