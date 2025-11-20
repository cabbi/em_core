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
    EmAppTaskInterface(EmAppInterface& taskInterface,
                       bool runSetupOnTask,
                       EmCoreId coreId = EmCoreId::coreUserTask,
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       const char* logContext=nullptr,
                       EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterface(taskInterface, runSetupOnTask, nullptr, 
                           coreId, blockedTimeout, logContext, logLevel) {}

    EmAppTaskInterface(EmAppInterface& taskInterface,
                       bool runSetupOnTask,  
                       EmList<EmAppInterface>* appInterfaces,
                       EmCoreId coreId = EmCoreId::coreUserTask,
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       const char* logContext=nullptr,
                       EmLogLevel logLevel=EmLogLevel::global) : 
       EmAppInterface(appInterfaces, blockedTimeout, logContext ? logContext : taskInterface.name(), logLevel),
       m_task(this, EmAppTaskInterface::loop_),
       m_taskInterface(taskInterface),
       m_runSetupOnTask(runSetupOnTask),
       m_taskOperationResult(EmIntOperationResult::canContinue) {

    }

    virtual ~EmAppTaskInterface() {
        m_task.kill();
    }

    virtual const char* name() const { 
        return m_taskInterface.name(); 
    }

    // Setup teh task interfaces
    virtual EmIntOperationResult setup() {
        EmIntOperationResult res = EmIntOperationResult::canContinue;
        if (!m_runSetupOnTask) {
            // Setup by calling the task loop function the first time
            loop_(this);
            res = m_taskOperationResult;
        } 
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
        m_taskInterface.onStop(reason);
    }

protected:
    static EmTaskFuncRes loop_(EmAppTaskInterface* self) {
        EmIntOperationResult opRes;
        if (!self->m_taskInterface.isInitialized()) {
            opRes = self->m_taskInterface.setup();
            if (opRes == EmIntOperationResult::canContinue) {
                self->m_taskInterface.setInitialized(true);
            }   
        } else {
            opRes = self->m_taskInterface.loop();
        }
        self->m_taskOperationResult = opRes;
        return opRes == EmIntOperationResult::canContinue ? 
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
    EmAppTaskInterfaces(const char* name,
                        bool runSetupOnTask = false,
                        EmCoreId coreId = EmCoreId::coreUserTask,
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        const char* logContext=nullptr,
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterfaces(name, runSetupOnTask, nullptr, coreId, blockedTimeout, logContext, logLevel) {}

    EmAppTaskInterfaces(const char* name,
                        bool runSetupOnTask,
                        EmList<EmAppInterface>* appInterfaces,
                        EmCoreId coreId = EmCoreId::coreUserTask,
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        const char* logContext=nullptr,
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterfaces(name, runSetupOnTask, appInterfaces, 
                            coreId, 8192, 1, blockedTimeout, logContext, logLevel) {}

    EmAppTaskInterfaces(const char* name,
                        bool runSetupOnTask,
                        EmList<EmAppInterface>* appInterfaces,
                        EmCoreId taskCoreId,
                        uint16_t taskStackSize,
                        uint8_t taskPriority,
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        const char* logContext=nullptr,
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppInterface(appInterfaces, blockedTimeout, logContext ? logContext : name, logLevel),
        EmAppInterfaces(),
        m_name(name),
        m_task(this, EmAppTaskInterfaces::loop_, 
               taskCoreId,
               taskStackSize,
               taskPriority),
        m_runSetupOnTask(runSetupOnTask),
        m_taskOperationResult(EmIntOperationResult::canContinue) {}

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

    virtual const char* name() const { return m_name; }

    virtual EmIntOperationResult setup() {
        EmIntOperationResult res = EmIntOperationResult::canContinue;
        m_runningInterfaces.set(*this, false);
        if (!m_runSetupOnTask) {
            // Setup by calling the task loop function the first time
            loop_(this);
            res = m_taskOperationResult;
        } 
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
                if (!interface.isInitialized()) {
                    *pOpRes = interface.setup();
                    if (*pOpRes == EmIntOperationResult::canContinue) {
                        interface.setInitialized(true);
                    }
                } else {
                    *pOpRes = interface.loop();
                }
                if (*pOpRes == EmIntOperationResult::stopInterface) {
                    return EmIterResult::removeMoveNext;
                } else if (*pOpRes != EmIntOperationResult::canContinue) {
                    return EmIterResult::stopSucceed;
                }
                return EmIterResult::moveNext;
            }, &opRes);
        self->m_taskOperationResult = opRes;
        return opRes == EmIntOperationResult::canContinue ? 
                        EmTaskFuncRes::continueTask : EmTaskFuncRes::pauseTask;
    }

    // Member vars
    const char* m_name;
    EmTask<EmAppTaskInterfaces> m_task;
    bool m_runSetupOnTask;
    EmAppInterfaces m_runningInterfaces;
    std::atomic<EmIntOperationResult> m_taskOperationResult;
};

#endif // EM_MULTITHREAD
#endif // __EM_APP_TASK_INTERFACE__H_