#ifndef __APP_INTERFACE__H_
#define __APP_INTERFACE__H_

#include <string.h>

#include "em_log.h"
#include "em_list.h"
#include "em_duration.h"
#include "em_timeout.h"
#include "em_defs.h"

class EmAppInterface;

enum class EmInterfaceStatus: int8_t {
    isNone        = 0x0000,
    isInitialized = 0x0001, // Correctly initialized
    isRunning     = 0x0002, // Running or Blocked (in case running timeout is elapsed!)
    isWarning     = 0x0004, // Has any warning
    isError       = 0x0008, // Has any error
};

enum class EmIntOperationResult: int8_t {
    canContinue = 0,
    removeInterface = 1,
    restartApp = 2,
    exitApp = 3,
};

inline EmInterfaceStatus operator~ (EmInterfaceStatus a) { return static_cast<EmInterfaceStatus>(~static_cast<int>(a)); }
inline EmInterfaceStatus operator|(EmInterfaceStatus a, EmInterfaceStatus b) { return static_cast<EmInterfaceStatus>(static_cast<int>(a) | static_cast<int>(b)); }
inline EmInterfaceStatus operator&(EmInterfaceStatus a, EmInterfaceStatus b) { return static_cast<EmInterfaceStatus>(static_cast<int>(a) & static_cast<int>(b)); }
inline EmInterfaceStatus& operator|=(EmInterfaceStatus& a, EmInterfaceStatus b) { return (EmInterfaceStatus&)((int&)(a) |= static_cast<int>(b)); }
inline EmInterfaceStatus& operator&=(EmInterfaceStatus& a, EmInterfaceStatus b) { return (EmInterfaceStatus&)((int&)(a) &= static_cast<int>(b)); }

#define MAX_INTERFACE_MSG_LEN 60

// TODO: add multithreading sync!

// This is the base interface class.
// Each interface should implement 'Name', 'Setup' & 'Loop' methods
class EmAppInterface: public EmLog {
public:
    EmAppInterface(const EmDuration& runningTimeout = EmDuration(0, 1, 0), 
                   EmLogLevel logLevel=EmLogLevel::global)
     : EmLog("AppInt", logLevel),
       m_interfaceStatus(EmInterfaceStatus::isNone),
       m_runningTimeout(runningTimeout)
    { 
        memset(m_warningMsg, 0, sizeof(m_warningMsg));
        memset(m_errorMsg, 0, sizeof(m_errorMsg));
    }
    
    virtual ~EmAppInterface() {}

    static bool match(const EmAppInterface& int1, const EmAppInterface& int2) {
        return 0==strcmp(int1.name(), int2.name());
    }

    virtual const char* name() const=0;
    virtual EmIntOperationResult setup()=0;
    virtual EmIntOperationResult loop()=0;

    // Override this in case app should not call interface 'Loop' method all the times
    virtual bool canCallLoop() { return true; }
    
    // Status handling
    virtual bool isInitialized() const { return getStatusFlag(EmInterfaceStatus::isInitialized); }
    virtual bool isRunning()     const { return getStatusFlag(EmInterfaceStatus::isRunning) && !isBlocked(); }
    virtual bool hasWarning()    const { return getStatusFlag(EmInterfaceStatus::isWarning); }
    virtual bool hasError()      const { return getStatusFlag(EmInterfaceStatus::isError); }
    virtual bool isBlocked()     const { return m_runningTimeout.isElapsed(false); }

    // Initialized, running and no errors
    virtual bool isOk()          const { return isInitialized() && isRunning() && !hasError(); }

    virtual void setInitialized(bool value)
        { setStatusFlag(EmInterfaceStatus::isInitialized, value); }
    
    virtual void setRunning(bool value)
        { if (value) m_runningTimeout.restart();
          setStatusFlag(EmInterfaceStatus::isRunning, value); }
    
    virtual void setWarning(bool value, const char* msg="");
    virtual void setError(bool value, const char* msg="");

    virtual const char* getErrorMsg() const { return m_errorMsg; }
    virtual const char* getWarningMsg() const { return m_warningMsg; }

    virtual EmInterfaceStatus getStatus() const
        { return m_interfaceStatus; }
    virtual bool getStatusFlag(EmInterfaceStatus statusFlags) const
        { return statusFlags == (m_interfaceStatus & statusFlags); }
    virtual void setStatusFlag(EmInterfaceStatus statusFlags, bool value) 
        { if (value) m_interfaceStatus |= statusFlags;
                else m_interfaceStatus &= ~statusFlags; }
    
protected:
    EmInterfaceStatus m_interfaceStatus; 

private:
    mutable EmTimeout m_runningTimeout;
    char m_warningMsg[MAX_INTERFACE_MSG_LEN+1];
    char m_errorMsg[MAX_INTERFACE_MSG_LEN+1];
};

class EmAppInterfaces: public EmList<EmAppInterface> {
public:
    EmAppInterfaces() : EmList<EmAppInterface>(&EmAppInterface::match) {}
};

// This interface has a loop call timeout, app will call the 'Loop' method each time timeout elapses
class EmAppTimeoutInterface: public EmAppInterface {
public:
    EmAppTimeoutInterface(uint32_t loopTimeoutMs, 
                          bool startAsElapsed=true,
                          uint32_t runningTimeoutMs = 60000, 
                          EmLogLevel logLevel=EmLogLevel::none) 
     : EmAppInterface(runningTimeoutMs, logLevel), 
       m_LoopTimeout(EmTimeout(loopTimeoutMs, startAsElapsed)) {}

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