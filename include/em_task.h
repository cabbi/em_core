#ifndef __EM_TASK_H
#define __EM_TASK_H

#include "em_defs.h"

#ifdef EM_MULTITHREAD
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "em_duration.h"
#include "em_timeout.h"
#include "em_threading.h"

// Possible return codes from the task function
enum class EmTaskFuncRes: uint8_t {
    continueTask = 0,  // Task continue normal execution
    pauseTask,         // Task to be paused (i.e. task not deleted)
    stopTask           // Task to be stopped (i.e. task deleted)
};

// Task statuses
enum class EmTaskStatus: uint8_t {
    running = 0,    // Task is running
    pausing,        // Task is pausing, waiting to be paused
    paused,         // Task is paused
    stopping,       // Task is stopping, waiting to be stopped
    stopped         // Task is stopped or killed (i.e. no background task)
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
// - Pausing or stopping the task will not suspend or delete the background task, it will wait 
//   the task function to exit (i.e. function execution not suspended or killed in the middle).
// - Paused task will still consume some CPU resources due to the delay in the task loop.
// - Difference between Pausing or Stopping the task is that Stopping will delete the task.
//   Call stop if you do not want to use the task anymore, pause if you want to start it again.
// - Calling 'kill' will stop execution of the task function immediately.
template <typename TParam>
class EmTask {
public:
    EmTask(TParam* pParam, 
           TaskFunctionType<TParam> taskFunction,
           EmCoreId coreId=EmCoreId::coreUserTask,
           uint16_t stackSize=8192,
           uint8_t priority=1) :
        m_pParam(pParam),
        m_taskFunction(taskFunction),
        m_coreId(coreId),
        m_stackSize(stackSize),
        m_priority(priority),
        m_taskHandle(nullptr),
        m_status(EmTaskStatus::stopped) {
    }

    // Starts the task if it is not already running.
    // Returns false if task is not stopped.
    bool start() {
        // Is task in pause mode?
        if (status() == EmTaskStatus::paused) {
            m_status.store(EmTaskStatus::running);
            return true;
        }
        // We will start a new task so previous task, if any, must be stopped
        if (status() != EmTaskStatus::stopped) {
            return false;
        }
        TaskHandle_t taskHandle;
        BaseType_t res = xTaskCreatePinnedToCore(EmTask::taskLoop_,
                                                 "",
                                                 m_stackSize,
                                                 this,
                                                 static_cast<UBaseType_t>(m_priority),
                                                 &taskHandle,
                                                 static_cast<BaseType_t>(m_coreId));
        if (res == pdPASS) {
            m_status.store(EmTaskStatus::running);
            m_taskHandle.store(taskHandle);
        }     
        return true;
    }

    // Triggers the task to be paused and exits immediately.
    // Returns false if task is not running.
    bool pause() {
        if (isNotRunning()) {
            return false;
        }
        m_status.store(EmTaskStatus::pausing);
        return true;
    }

    // Waits for the task to be paused up to 'timeoutMs' milliseconds duration.
    // Returns true if the task is paused, false if task is not running or timeout occurred.
    bool pause(uint32_t timeoutMs) {
        return pause(EmDuration(timeoutMs));
    }

    // Waits for the task to be paused up to 'timeout' duration.
    // Returns true if the task is paused, false if task is not running or timeout occurred.
    bool pause(EmDuration timeout) {
        if (!pause()) {
            return false;
        }
        EmTimeout timeout_(timeout);
        while (status()==EmTaskStatus::pausing && !timeout_.isElapsed(true)) {
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }        
        return status() == EmTaskStatus::paused;
    }

    // Triggers the task to be stopped and exits immediately.
    // Returns false if task is not running.
    bool stop() {
        if (isNotRunning()) {
            return false;
        }
        m_status.store(EmTaskStatus::stopping);
        return true;
    }

    // Waits for the task to be stopped up to 'timeoutMs' milliseconds duration.
    // Returns true if the task is stopped, false if task is not running or timeout occurred.
    bool stop(uint32_t timeoutMs) {
        return stop(EmDuration(timeoutMs));
    }

    // Waits for the task to be stopped up to 'timeout' duration.
    // Returns true if the task is stopped, false if task is not running or timeout occurred.
    bool stop(EmDuration timeout) {
        if (!stop()) {
            return false;
        }
        EmTimeout timeout_(timeout);
        while (status()==EmTaskStatus::stopping && !timeout_.isElapsed(true)) {
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }        
        return status() == EmTaskStatus::stopped;
    }

    // Brutally kills the task immediately. 
    // Task function execution is stopped immediately.
    void kill() {
        TaskHandle_t handle = m_taskHandle.load();
        if (handle != nullptr) {
            kill_(handle);
        }
    }

    EmTaskStatus status() const {
        return m_status.load();
    }

    bool isRunning() const {
        return status() == EmTaskStatus::running;
    }

    bool isNotRunning() const {
        return !isRunning();
    }

    bool isPaused() const {
        return status() == EmTaskStatus::paused;
    }

    bool isNotPaused() const {
        return !isPaused();
    }

    bool isStopped() const {
        return status() == EmTaskStatus::stopped;
    }

    bool isNotStopped() const {
        return !isStopped();
    }

protected:
    void kill_(TaskHandle_t taskHandle) {
        m_taskHandle.store(nullptr);
        // NOTE: keep setting stopped status after setting handle to nullptr!
        m_status.store(EmTaskStatus::stopped);
        // NOTE: keep the task deleting at end because it will not do anything after this line!
        vTaskDelete(taskHandle);
    }

    static void taskLoop_(void* taskParam) {
        EmTask* pThis = static_cast<EmTask*>(taskParam);
        if (pThis != nullptr && pThis->m_taskFunction != nullptr) {
            while (true) {
                // Perform user task and check requested action
                if (pThis->isRunning()) {
                    EmTaskFuncRes res = pThis->m_taskFunction(pThis->m_pParam);
                    if (res == EmTaskFuncRes::pauseTask) {
                        pThis->pause();
                    }
                    if (res == EmTaskFuncRes::stopTask) {
                        pThis->stop();
                    }
                } 
                // Check task status. 
                // This might have changed during task function execution or user request.
                if (pThis->status() == EmTaskStatus::pausing) {
                    pThis->m_status.store(EmTaskStatus::paused);
                }
                if (pThis->status() == EmTaskStatus::stopping) {
                    pThis->kill_(nullptr);
                }
                // Relax the paused task
                if (pThis->isPaused()) {
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
            }
        }
    }

    // Member vars
    TParam* m_pParam;
    TaskFunctionType<TParam> m_taskFunction;
    EmCoreId m_coreId;
    uint16_t m_stackSize;
    uint8_t m_priority;
    std::atomic<TaskHandle_t> m_taskHandle;
    std::atomic<EmTaskStatus> m_status;
};

#endif // EM_MULTITHREAD
#endif // __EM_TASK_H