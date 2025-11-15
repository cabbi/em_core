#ifndef __EM_APP_TASK_INTERFACE__H_
#define __EM_APP_TASK_INTERFACE__H_

#include "em_defs.h"

#if defined(EM_MULTITHREAD) 

#include "em_task.h"
#include "em_app_interface.h"

// This interface runs in its own task/thread.
//
// Assign your own interface and 'EmAppTaskInterface' instance to your application interfaces list.
class EmAppTaskInterface: public EmAppInterface {
public:
    EmAppTaskInterface(EmAppInterface& taskInterface,
                       EmCoreId coreId = EmCoreId::core1,
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       const char* logContext=nullptr,
                       EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterface(taskInterface, nullptr, coreId, blockedTimeout, logContext, logLevel) {}

    EmAppTaskInterface(EmAppInterface& taskInterface,
                       EmList<EmAppInterface>* interfaces,
                       EmCoreId coreId = EmCoreId::core1,
                       const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                       const char* logContext=nullptr,
                       EmLogLevel logLevel=EmLogLevel::global) : 
       EmAppInterface(interfaces, blockedTimeout, logContext ? logContext : taskInterface.name(), logLevel),
       m_task(this, EmAppTaskInterface::loop_),
       m_taskInterface(taskInterface),
       m_taskOperationResult(EmIntOperationResult::canContinue) {

    }

    virtual ~EmAppTaskInterface() {
        m_task.kill();
    }

    virtual const char* name() const { 
        return m_taskInterface.name(); 
    }

    // Setup of all task interfaces is done on the application task.
    virtual EmIntOperationResult setup() { 
        return m_taskInterface.setup(); 
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
        EmIntOperationResult opRes = self->m_taskInterface.loop();
        self->m_taskOperationResult = opRes;
        return opRes == EmIntOperationResult::canContinue ? 
                        EmTaskFuncRes::continueTask : EmTaskFuncRes::pauseTask;
    }

    // Member vars
    EmTask<EmAppTaskInterface> m_task;
    EmAppInterface& m_taskInterface;
    std::atomic<EmIntOperationResult> m_taskOperationResult;
};

// This interface runs in its own task/thread and can contain more interfaces running within the same task. 
//
// Assign your own interfaces and 'EmAppTaskInterfaces' instance to your application interfaces list.
class EmAppTaskInterfaces: public EmAppInterface, 
                           public EmAppInterfaces {
public:
    EmAppTaskInterfaces(const char* name,
                        EmCoreId coreId = EmCoreId::core1,
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        const char* logContext=nullptr,
                        EmLogLevel logLevel=EmLogLevel::global) : 
        EmAppTaskInterfaces(name, nullptr, coreId, blockedTimeout, logContext, logLevel) {}

    EmAppTaskInterfaces(const char* name,
                        EmList<EmAppInterface>* interfaces,
                        EmCoreId coreId = EmCoreId::core1,
                        const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                        const char* logContext=nullptr,
                        EmLogLevel logLevel=EmLogLevel::global) : 
       EmAppInterface(interfaces, blockedTimeout, logContext ? logContext : name, logLevel),
       EmAppInterfaces(),
       m_name(name),
       m_task(this, EmAppTaskInterfaces::loop_),
       m_taskOperationResult(EmIntOperationResult::canContinue) {

    }

    virtual ~EmAppTaskInterfaces() {
        m_task.kill();
        m_taskInterfaces.clear();
        m_runningInterfaces.clear();
    }

    // Add an interface to the task
    virtual void addInterface(EmAppInterface& interface) {
        m_taskInterfaces.appendUnowned(interface);
    }

    // Add multiple interfaces to the task using a variable argument list. 
    // NOTE: last argument MUST be a nullptr.
    virtual void addInterfaces(EmAppInterface* interface, ...) {
        va_list args;
        va_start(args, interface);
        m_taskInterfaces.extend(false, interface, args);
        va_end(args);
    }

    virtual const char* name() const { return m_name; }

    // Setup of all task interfaces is done on the application task.
    virtual EmIntOperationResult setup() { 
        // Set all task interfaces as running 
        m_runningInterfaces.set(m_taskInterfaces, false);
        EmIntOperationResult opRes = EmIntOperationResult::canContinue;
        // Setup each task interface
        m_runningInterfaces.forEach<EmIntOperationResult>(
            [](EmAppInterface& interface, bool, bool, EmIntOperationResult* pOpRes) -> EmIterResult {
                *pOpRes = interface.setup();
                if (*pOpRes == EmIntOperationResult::stopInterface) {
                    return EmIterResult::removeMoveNext;
                } else if (*pOpRes != EmIntOperationResult::canContinue) {
                    return EmIterResult::stopSucceed;
                }
                return EmIterResult::moveNext;
            }, &opRes);
        return opRes; 
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
                *pOpRes = interface.loop();
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
    EmAppInterfaces m_taskInterfaces;
    EmAppInterfaces m_runningInterfaces;
    std::atomic<EmIntOperationResult> m_taskOperationResult;
};

#endif // EM_MULTITHREAD
#endif // __EM_APP_TASK_INTERFACE__H_