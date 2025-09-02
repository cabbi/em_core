#ifndef __EM_APP_INTERFACE__H_
#define __EM_APP_INTERFACE__H_

#include <string.h>

#include "em_defs.h"
#include "em_log.h"
#include "em_list.h"
#include "em_threading.h"
#include "em_duration.h"
#include "em_timeout.h"

class EmAppInterface;

namespace EmInterfaceStatusFlag {
    constexpr uint8_t none          = 0x0000;
    constexpr uint8_t isInitialized = 0x0001; // Is correctly initialized (app will call 'setup' instead of 'loop' until this flag is not set)
    constexpr uint8_t hasWarning    = 0x0010; // Has any warning
    constexpr uint8_t hasError      = 0x0020; // Has any error

    using Type = uint8_t;
    using TypeInternal = ts_uint8;
}

enum class EmIntOperationResult: int8_t {
    canContinue = 0,
    stopInterface = 1,
    restartApp = 2,
    stopApp = 3,
};

#define MAX_INTERFACE_MSG_LEN 60

// TODO: add multithreading sync!

// This is the base interface class.
//
// Each interface should implement 'name', 'setup' & 'loop' methods. 
// Override 'dispose' In case your application might restart (i.e. any interface returning 'EmIntOperationResult::restartApp')
class EmAppInterface: public EmLog {
    friend class EmApp;
public:
    EmAppInterface(const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                   EmLogLevel logLevel=EmLogLevel::global)
     : EmLog("AppInt", logLevel),
       m_interfaceStatus(EmInterfaceStatusFlag::none),
       m_blockedTimeout(blockedTimeout) { 
        clear_();
    }
    
    virtual ~EmAppInterface() {}

    static bool match(const EmAppInterface& int1, const EmAppInterface& int2) {
        return 0==strcmp(int1.name(), int2.name());
    }

    // NOTE: these methods should be overridden.
    // Those are NOT set as pure virtual since EmApp interfaces list requires concrete classes.
    virtual const char* name() const { return ""; }
    virtual EmIntOperationResult setup() { return EmIntOperationResult::canContinue; }
    virtual EmIntOperationResult loop() { return EmIntOperationResult::stopApp; }

    // Called if interface needs to stop for one of the following reasons
    // 'EmIntOperationResult::stopInterface', 'EmIntOperationResult::restartApp' or 'EmIntOperationResult::stopApp'
    virtual void onStop(EmIntOperationResult /*reason*/) { 
        // Do some cleanup if needed
    }

    // Override this in case app should not call interface 'loop' method all the times
    virtual bool canCallLoop() { return true; }
    
    // Status handling
    virtual bool isInitialized() const { return getStatusFlag_(EmInterfaceStatusFlag::isInitialized); }
    virtual bool hasWarning()    const { return getStatusFlag_(EmInterfaceStatusFlag::hasWarning); }
    virtual bool hasError()      const { return getStatusFlag_(EmInterfaceStatusFlag::hasError); }
    virtual bool isBlocked()     const { return m_blockedTimeout.isElapsed(false); }
    // Initialized and no errors
    virtual bool isOk()          const { return isInitialized() && !hasError(); }

    virtual void setInitialized(bool value) { 
        setStatusFlag_(EmInterfaceStatusFlag::isInitialized, value); }
    
   
    virtual void setWarning(bool value, const char* msg="");
    virtual void setError(bool value, const char* msg="");

    virtual const char* getErrorMsg() const { return m_errorMsg; }
    virtual const char* getWarningMsg() const { return m_warningMsg; }
   
protected:
    virtual bool getStatusFlag_(EmInterfaceStatusFlag::Type statusFlags) const
        { return statusFlags == (static_cast<EmInterfaceStatusFlag::Type>(m_interfaceStatus) & statusFlags); }
    virtual void setStatusFlag_(EmInterfaceStatusFlag::Type statusFlags, bool value) 
        { if (value) m_interfaceStatus |= statusFlags;
                else m_interfaceStatus &= ~statusFlags; }



    void clear_(){ 
        m_interfaceStatus = EmInterfaceStatusFlag::none;
        memset(m_warningMsg, 0, sizeof(m_warningMsg));
        memset(m_errorMsg, 0, sizeof(m_errorMsg));
    }

    EmInterfaceStatusFlag::TypeInternal m_interfaceStatus; 
    mutable EmTimeout m_blockedTimeout;
    char m_warningMsg[MAX_INTERFACE_MSG_LEN+1];
    char m_errorMsg[MAX_INTERFACE_MSG_LEN+1];
};

class EmAppInterfaces: public EmList<EmAppInterface> {
public:
    EmAppInterfaces() : EmList<EmAppInterface>(&EmAppInterface::match) {}
};

// This interface has a loop call timeout, app will call the 'loop' 
// method each time timeout elapses
class EmAppTimeoutInterface: public EmAppInterface {
public:
    EmAppTimeoutInterface(EmDuration loopTimeout, 
                          bool startAsElapsed=true,
                          EmDuration blockedTimeout = EmDuration(0, 1, 0),
                          EmLogLevel logLevel=EmLogLevel::global) 
     : EmAppInterface(blockedTimeout, logLevel), 
       m_LoopTimeout(loopTimeout, startAsElapsed) {}

    virtual bool canCallLoop() { return m_LoopTimeout.isElapsed(true); }

private:
    mutable EmTimeout m_LoopTimeout;
};

// This interface will update each 'EmUpdatable' object at each 'Loop'.
template <EmUpdatable* updatableObjects[], uint8_t size>
class EmAppUpdaterInterface: public EmAppInterface, 
                             public EmUpdater<updatableObjects, size> {
public:
    EmAppUpdaterInterface(uint32_t runningTimeoutMs = 60000, 
                          EmLogLevel logLevel=EmLogLevel::none) 
     : EmAppInterface(runningTimeoutMs, logLevel) {}

    virtual const char* name() const override {
        return "EmUpdater";
    }

    virtual EmIntOperationResult setup() {
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult loop() {
        EmUpdater<updatableObjects, size>::update();
        return EmIntOperationResult::canContinue;    
    }
};


#endif