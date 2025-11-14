#include "em_defs.h"

#ifdef EM_MULTITHREAD
#include <FreeRTOS.h>
#include <task.h>
#include "em_threading.h"

enum class EmTaskFuncRes: uint8_t {
    shouldContinue = 0, 
    shouldPause,
    shouldKill
};

template <typename TParam>
using TaskFunctionType = EmTaskFuncRes (*)(TParam*);

// A worker task executing 'taskFunction' within a loop. The result of the function
// will determine if the task should continue, pause or be killed.
// Ensure that stack size, priority and core are set appropriately.
// Valid task priority values are from 1 to 24, with higher numbers indicating higher priority.
// 
// NOTES:
// ------
// - Pausing the task will not suspend it, it will only pause the execution of the task function.
//   This ensures task function is always executed till the end and not suspended in the middle.
// - Paused task will still consume some CPU resources due to the delay in the task loop.

template <typename TParam>
class EmTask {
public:
    EmTask(TParam* pParam, 
           TaskFunctionType<TParam> taskFunction,
           EmCoreId coreId=EmCoreId::core0,
           uint16_t stackSize=8192,
           uint8_t priority=1,
           const char* taskName="EmTask") :
        m_pParam(pParam),
        m_taskHandle(nullptr),
        m_taskFunction(taskFunction),
        m_isRunning(false) {
        BaseType_t res = xTaskCreatePinnedToCore(EmTask::taskLoop,
                                                 taskName,
                                                 stackSize,
                                                 pParam,
                                                 static_cast<UBaseType_t>(priority),
                                                 &m_taskHandle,
                                                 static_cast<BaseType_t>(coreId));
        if (res != pdPASS) {
            m_taskHandle = nullptr;
        }                                                    
    }
    
    void start() {
        m_isRunning = true;
    }

    void pause() {
        m_isRunning = false;
    }

    bool kill() {
        // TODO: add a timeout to try to pause it and the kill the task
        if (isNotKilled()) {
            vTaskDelete(m_taskHandle);
            m_taskHandle = nullptr;
            return true;
        }
        return false;
    }

    bool isRunning() const {
        return isNotKilled() && m_isRunning;
    }

    bool isNotRunning() const {
        return !isRunning();
    }

    bool isKilled() const {
        return m_taskHandle == nullptr;
    }

    bool isNotKilled() const {
        return !isKilled();
    }

protected:
    static void taskLoop(void* taskParam) {
        EmTask* pThis = static_cast<EmTask*>(taskParam);
        if (pThis != nullptr && pThis->m_taskFunction != nullptr) {
            while (true) {
                if (pThis->isRunning()) {
                    // Perform user task 
                    EmTaskFuncRes res = pThis->m_taskFunction(pThis->m_pParam);
                    if (res == EmTaskFuncRes::shouldPause) {
                        pThis->pause();
                    }
                    if (res == EmTaskFuncRes::shouldKill) {
                        pThis->kill();
                    }
                } else {
                    // Relax the stopped task
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
            }
        }
    }

    // Member vars
    TParam* m_pParam;
    TaskHandle_t m_taskHandle;
    TaskFunctionType<TParam> m_taskFunction;
    ts_bool m_isRunning;
};

#endif // EM_MULTITHREAD