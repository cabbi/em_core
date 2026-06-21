#ifndef __EM_APP_TASK_INTERFACE__H_
#define __EM_APP_TASK_INTERFACE__H_

#include "em_defs.h"

#if defined(EM_MULTITHREAD) 

#include "em_task.h"
#include "em_app_interface.h"

// This interface runs in its own task/thread.
//
// Assign your own interface and 'EmAppTaskInterface' instance to your 
// application interfaces list.
// By setting 'runSetupOnTask' to true, the owned interface 'setup' 
// method will be called within the task context. 
class EmAppTaskInterface: public EmAppInterface {
public:
    /// @brief Interface constructor
    /// @param name The interface name
    /// @param taskInterface The interface to run within the task 
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param taskCoreId The core ID on which the task will run
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppTaskInterface(const char* name, 
                       EmAppInterface& taskInterface,
                       bool runSetupOnTask,
                       EmCoreId taskCoreId = EmCoreId::coreUserTask,
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterface(name, taskInterface, runSetupOnTask, 
                           taskCoreId, 8192, 1, blockedTimeout, logLevel) {}

    /// @brief  Interface constructor
    /// @param name The interface name 
    /// @param taskInterface The interface to run within the task 
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param taskCoreId The core ID on which the task will run
    /// @param taskStackSize The stack size of the task
    /// @param taskPriority The priority of the task
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time
    /// @param logLevel The log level of this interface
    EmAppTaskInterface(const char* name,
                       EmAppInterface& taskInterface,
                       bool runSetupOnTask,
                       EmCoreId taskCoreId,
                       uint16_t taskStackSize,
                       uint8_t taskPriority,   
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       EmLogLevel logLevel=EmLogLevel::global) : 
       EmAppInterface(name, blockedTimeout, logLevel),
       m_task(this, EmAppTaskInterface::loop_, 
               taskCoreId,
               taskStackSize,
               taskPriority),
       m_taskInterface(taskInterface),
       m_runSetupOnTask(runSetupOnTask),
       m_taskOperationResult(EmIntOperationResult::canContinue) {}

    /// @brief Interface constructor
    /// @param name The interface name
    /// @param taskInterface The interface to run within the task 
    /// @param appInterfaces Add this instance to the application interfaces 
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param taskCoreId The core ID on which the task will run
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppTaskInterface(const char* name,
                       EmAppInterface& taskInterface,
                       bool runSetupOnTask,  
                       EmAppInterfaces& appInterfaces,
                       EmCoreId taskCoreId = EmCoreId::coreUserTask,
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterface(name, taskInterface, runSetupOnTask, 
                           taskCoreId, blockedTimeout, logLevel) {
        appInterfaces.appendUnowned(*this);
    }

    /// @brief Interface constructor     
    /// @param name The interface name
    /// @param taskInterface The interface to run within the task 
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param appInterfaces Add this instance to the application interfaces 
    /// @param taskCoreId The core ID on which the task will run
    /// @param taskStackSize The stack size of the task
    /// @param taskPriority The priority of the task
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time
    /// @param logLevel The log level of this interface
    EmAppTaskInterface(const char* name,
                       EmAppInterface& taskInterface,
                       bool runSetupOnTask,  
                       EmAppInterfaces& appInterfaces,
                       EmCoreId taskCoreId,
                       uint16_t taskStackSize,
                       uint8_t taskPriority,   
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterface(name, taskInterface, runSetupOnTask, 
                           taskCoreId, taskStackSize, taskPriority, 
                           blockedTimeout, logLevel) {
        appInterfaces.appendUnowned(*this);
    }   

    virtual ~EmAppTaskInterface() {
        m_task.kill();
    }

    virtual const char* name() const { 
        return m_taskInterface.name(); 
    }

    // Setup teh task interfaces
    virtual EmIntOperationResult setup() {
        bool failed = false;
        EmIntOperationResult res = EmIntOperationResult::canContinue;
        if (!m_runSetupOnTask) {
            res = m_taskInterface.loopStep_(failed);
            res = setFailure(res, failed);
        }
        if (!failed) {
            // Starting the background task if setup was not requested ot successful
            m_task.start();
        }   
        return res;
    }

    // Real loop of each task interface is performed in the task.
    // Application calls this loop only to check the overall operation result.
    virtual EmIntOperationResult loop() { 
        return m_taskOperationResult; 
    }

    // Called if interface needs to stop for one of the following reasons
    // 'EmIntOperationResult::stopInterface', 'EmIntOperationResult::restartApp' or 'EmIntOperationResult::stopApp'
    virtual void onStop(EmIntOperationResult reason) { 
        m_taskInterface.onStop(reason);
    }

protected:
    static EmTaskFuncRes loop_(EmAppTaskInterface* self) {
        bool failed = false;
        // Optimized to have 'atomic' writing but not reading of 'm_taskOperationResult'
        EmIntOperationResult res = self->loopStep_(failed);
        self->m_taskOperationResult = res;
        return res == EmIntOperationResult::canContinue ? 
                        EmTaskFuncRes::continueTask : EmTaskFuncRes::pauseTask;
    }

    // Member vars
    EmTask<EmAppTaskInterface> m_task;
    const bool m_runSetupOnTask;
    EmAppInterface& m_taskInterface;
    std::atomic<EmIntOperationResult> m_taskOperationResult;
};

// This interface runs in its own task/thread and can contain more interfaces running within the same task. 
//
// Assign your own interfaces and 'EmAppTaskInterface' instance to your 
// application interfaces list.
// By setting 'runSetupOnTask' to true, the owned interfaces 'setup' 
// method will be called within the task context. 
class EmAppTaskInterfaces: public EmAppInterface, 
                           public EmAppInterfaces {
public:

    /// @brief Interface constructor
    /// @param name The interface name
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param coreId The core ID on which the task will run
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time
    /// @param logLevel The log level of this interface
    EmAppTaskInterfaces(const char* name,
                        bool runSetupOnTask = false,
                        EmCoreId coreId = EmCoreId::coreUserTask,
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterfaces(name, runSetupOnTask, *this, 
                            coreId, 8192, 1, blockedTimeout, logLevel) {}

    /// @brief Interface constructor
    /// @param name The interface name
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param appInterfaces Add this instance to the application interfaces
    /// @param coreId The core ID on which the task will run
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time
    /// @param logLevel The log level of this interface                            
    EmAppTaskInterfaces(const char* name,
                        bool runSetupOnTask,
                        EmCoreId taskCoreId,
                        uint16_t taskStackSize,
                        uint8_t taskPriority,   
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppInterface(name, blockedTimeout, logLevel),
        EmAppInterfaces(),
        m_task(this, EmAppTaskInterfaces::loop_, 
               taskCoreId,
               taskStackSize,
               taskPriority),
        m_runSetupOnTask(runSetupOnTask),
        m_taskOperationResult(EmIntOperationResult::canContinue) {}

    /// @brief Interface constructor  
    /// @param name The interface name 
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param appInterfaces Add this instance to the application interfaces
    /// @param coreId The core ID on which the task will run
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time
    /// @param logLevel The log level of this interface
    EmAppTaskInterfaces(const char* name,
                        bool runSetupOnTask,
                        EmAppInterfaces& appInterfaces,
                        EmCoreId coreId = EmCoreId::coreUserTask,
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterfaces(name, runSetupOnTask, appInterfaces, 
                            coreId, 8192, 1, blockedTimeout, logLevel) {}

    /// @brief Interface constructor     
    /// @param name The interface name
    /// @param runSetupOnTask Whether to run the setup method within the task context
    /// @param appInterfaces Add this instance to the application interfaces
    /// @param coreId The core ID on which the task will run
    /// @param taskStackSize The stack size of the task
    /// @param taskPriority The priority of the task
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time
    /// @param logLevel The log level of this interface
    EmAppTaskInterfaces(const char* name,
                        bool runSetupOnTask,
                        EmAppInterfaces& appInterfaces,
                        EmCoreId taskCoreId,
                        uint16_t taskStackSize,
                        uint8_t taskPriority,   
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterfaces(name, runSetupOnTask, taskCoreId, taskStackSize, 
                            taskPriority, blockedTimeout, logLevel) {
        // TODO: verify if we need to add "this" to the appInterfaces list!?!?
        //appInterfaces.appendUnowned(*this); 
    }

    virtual ~EmAppTaskInterfaces() {
        m_task.stop();
        clear();
        m_runningInterfaces.clear();
    }

    // Add an interface to the task
    virtual void addInterface(EmAppInterface& interface) {
        appendUnowned(interface);
    }

    // Add multiple interfaces to the task using a variable argument list. 
    // NOTE: last argument MUST be a nullptr.
    virtual void addInterfaces(EmAppInterface* interface, ...) {
        va_list args;
        va_start(args, interface);
        extend(false, interface, args);
        va_end(args);
    }

    virtual EmIntOperationResult setup() {
        EmIntOperationResult res = EmIntOperationResult::canContinue;
        m_runningInterfaces.set(*this, false);
        if (!m_runSetupOnTask) {
            // Setup by calling the task loop function the first time
            loop_(this);
            res = m_taskOperationResult;
        } 
        // Starting the background task in any case, even if some interfaces failed setup
        m_task.start();
        return res;
    }

    // Real loop of each task interface is performed in the task.
    // Application calls this loop only to check the overall operation result.
    virtual EmIntOperationResult loop() { 
        return m_taskOperationResult; 
    }

    // Called if interface needs to stop for one of the following reasons
    // 'EmIntOperationResult::stopInterface', 'EmIntOperationResult::restartApp' or 'EmIntOperationResult::stopApp'
    virtual void onStop(EmIntOperationResult reason) { 
        // Stop all running task interfaces
        m_runningInterfaces.forEach<EmIntOperationResult>(
            [](EmAppInterface& interface, bool, bool, EmIntOperationResult* pReason) -> EmIterResult {
                interface.onStop(*pReason);
                return EmIterResult::removeMoveNext;
            }, &reason);
    }

protected:
    static EmTaskFuncRes loop_(EmAppTaskInterfaces* self) {
        EmIntOperationResult opRes = EmIntOperationResult::canContinue;
        // Loop each running task interface
        self->m_runningInterfaces.forEach<EmIntOperationResult>(
            [](EmAppInterface& interface, bool, bool, EmIntOperationResult* pOpRes) -> EmIterResult {
                bool failed = false;
                *pOpRes = interface.loopStep_(failed);
                tDelay(1); // Signal the task watchdog to avoid device reboot
                if (*pOpRes == EmIntOperationResult::stopInterface) {
                    return EmIterResult::removeMoveNext;
                } else if (*pOpRes != EmIntOperationResult::canContinue) {
                    return failed ? EmIterResult::stopFailed : EmIterResult::stopSucceed;
                }
                return EmIterResult::moveNext;
            }, &opRes);
        self->m_taskOperationResult.store(opRes);
        tDelay(1); // Signal the task watchdog to avoid device reboot
        return opRes == EmIntOperationResult::canContinue ? 
                        EmTaskFuncRes::continueTask : EmTaskFuncRes::pauseTask;
    }

    // Member vars
    EmTask<EmAppTaskInterfaces> m_task;
    bool m_runSetupOnTask;
    EmAppInterfaces m_runningInterfaces;
    std::atomic<EmIntOperationResult> m_taskOperationResult;
};

#endif // EM_MULTITHREAD
#endif // __EM_APP_TASK_INTERFACE__H_