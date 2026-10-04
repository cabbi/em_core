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
    #ifndef EM_MAIN_TASK_STACK_SIZE
        #define EM_MAIN_TASK_STACK_SIZE 8192    
    #endif        
    void app_main(void) {
    #ifdef EM_MULTICORE
        xTaskCreatePinnedToCore(
            vLoopTask, 
            "main_loop_task", 
            EM_MAIN_TASK_STACK_SIZE, 
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