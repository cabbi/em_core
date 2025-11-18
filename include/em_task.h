#include "em_defs.h"

#ifdef EM_MULTITHREAD
#include <FreeRTOS.h>
#include <task.h>
#include "em_threading.h"

// Possible return codes from the task function
enum class EmTaskFuncRes: uint8_t {
    continueTask = 0,  // Task continue normal execution
    pauseTask,         // Task to be paused
    killTask           // Task to be killed
};

// Type definition for the task function
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
// - A killed task cannot be resumed!
template <typename TParam>
class EmTask {
public:
    EmTask(TParam* pParam, 
           TaskFunctionType<TParam> taskFunction,
           EmCoreId coreId=EmCoreId::coreUserTask,
           uint16_t stackSize=8192,
           uint8_t priority=1,
           const char* taskName="EmTask") :
        m_pParam(pParam),
        m_handleMutex(),
        m_taskHandle(nullptr),
        m_isRunning(false) {
        BaseType_t res = xTaskCreatePinnedToCore(EmTask::taskLoop_,
                                                 taskName,
                                                 stackSize,
                                                 this,
                                                 static_cast<UBaseType_t>(priority),
                                                 &m_taskHandle,
                                                 static_cast<BaseType_t>(coreId));
        if (res != pdPASS) {
            m_taskHandle = nullptr;
        }                                                    
    }
    
    bool start() {
        if (isKilled()) {
            return false;
        }
        m_isRunning = true;
        return true;
    }

    bool pause() {
        if (isKilled()) {
            return false;
        }
        m_isRunning = false;
        return true;
    }

    bool kill() {
        // TODO: add a timeout to try to pause it and the kill the task
        if (isNotKilled()) {
            vTaskDelete(getTaskHandle_());
            setTaskHandle_(nullptr);
            m_isRunning = false;
            return true;
        }
        return false;
    }

    bool isRunning() const {
        return m_isRunning;
    }

    bool isNotRunning() const {
        return !isRunning();
    }

    bool isKilled() const {
        return getTaskHandle_() == nullptr;
    }

    bool isNotKilled() const {
        return !isKilled();
    }

protected:
    TaskHandle_t getTaskHandle_() const {
        EmMutexLock lock(m_handleMutex);
        return m_taskHandle;
    }

    void setTaskHandle_(TaskHandle_t handle) {
        EmMutexLock lock(m_handleMutex);
        m_taskHandle = handle;
    }   

    void killInternal_() {
        // NOTE: killing from within the task loop will get stuck if using vTaskDelete(m_taskHandle);
        setTaskHandle_(nullptr);
        vTaskDelete(nullptr);
        m_isRunning = false;
    }

    static void taskLoop_(void* taskParam) {
        EmTask* pThis = static_cast<EmTask*>(taskParam);
        if (pThis != nullptr && pThis->m_taskFunction != nullptr) {
            while (true) {
                if (pThis->isRunning()) {
                    // Perform user task 
                    EmTaskFuncRes res = pThis->m_taskFunction(pThis->m_pParam);
                    if (res == EmTaskFuncRes::pauseTask) {
                        pThis->pause();
                    }
                    if (res == EmTaskFuncRes::killTask) {
                        pThis->killInternal_();
                    }
                } else {
                    // Relax the paused task
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
            }
        }
    }

    // Member vars
    TParam* m_pParam;
    TaskFunctionType<TParam> m_taskFunction;
    mutable EmMutex m_handleMutex;
    TaskHandle_t m_taskHandle;
    ts_bool m_isRunning;
};

#endif // EM_MULTITHREAD