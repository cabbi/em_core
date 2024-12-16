#ifndef __APP_INTERFACE__H_
#define __APP_INTERFACE__H_

#include <string.h>

#include "em_log.h"
#include "em_list.h"
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
    EmAppInterface(uint32_t runningTimeoutMs = 60000, EmLogLevel logLevel=EmLogLevel::none)
     : EmLog("AppInt", logLevel),
       m_InterfaceStatus(EmInterfaceStatus::isNone),
       m_RunningTimeout(runningTimeoutMs)
    { 
        memset(m_WarningMsg, 0, sizeof(m_WarningMsg));
        memset(m_ErrorMsg, 0, sizeof(m_ErrorMsg));
    }
    
    virtual ~EmAppInterface() {}

    static bool Match(const EmAppInterface& int1, const EmAppInterface& int2) {
        return 0==strcmp(int1.Name(), int2.Name());
    }

    virtual const char* Name() const=0;
    virtual EmIntOperationResult Setup()=0;
    virtual EmIntOperationResult Loop()=0;

    // Override this in case app should not call interface 'Loop' method all the times
    virtual bool CanCallLoop() { return true; }
    
    // Status handling
    virtual bool IsInitialized() const { return GetStatusFlag(EmInterfaceStatus::isInitialized); }
    virtual bool IsRunning()     const { return GetStatusFlag(EmInterfaceStatus::isRunning) && !IsBlocked(); }
    virtual bool HasWarning()    const { return GetStatusFlag(EmInterfaceStatus::isWarning); }
    virtual bool HasError()      const { return GetStatusFlag(EmInterfaceStatus::isError); }
    virtual bool IsBlocked()     const { return m_RunningTimeout.IsElapsed(false); }

    // Initialized, running and no errors
    virtual bool IsOk()          const { return IsInitialized() && IsRunning() && !HasError(); }

    virtual void SetInitialized(bool value)
        { SetStatusFlag(EmInterfaceStatus::isInitialized, value); }
    
    virtual void SetRunning(bool value)
        { if (value) m_RunningTimeout.Restart();
          SetStatusFlag(EmInterfaceStatus::isRunning, value); }
    
    virtual void SetWarning(bool value, const char* msg="");
    virtual void SetError(bool value, const char* msg="");

    virtual const char* GetErrorMsg() const { return m_ErrorMsg; }
    virtual const char* GetWarningMsg() const { return m_WarningMsg; }

    virtual EmInterfaceStatus GetStatus() const
        { return m_InterfaceStatus; }
    virtual bool GetStatusFlag(EmInterfaceStatus statusFlags) const
        { return statusFlags == (m_InterfaceStatus & statusFlags); }
    virtual void SetStatusFlag(EmInterfaceStatus statusFlags, bool value) 
        { if (value) m_InterfaceStatus |= statusFlags;
                else m_InterfaceStatus &= ~statusFlags; }
    
protected:
    EmInterfaceStatus m_InterfaceStatus; 

private:
    mutable EmTimeout m_RunningTimeout;
    char m_WarningMsg[MAX_INTERFACE_MSG_LEN+1];
    char m_ErrorMsg[MAX_INTERFACE_MSG_LEN+1];
};

class EmAppInterfaces: public EmList<EmAppInterface> {
public:
    EmAppInterfaces() : EmList(EmAppInterface::Match) {}
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

    virtual bool CanCallLoop() { return m_LoopTimeout.IsElapsed(true); }

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

    virtual const char* Name() const override {
        return "EmUpdater";
    }

    virtual EmIntOperationResult Setup() {
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult Loop() {
        EmUpdater<updatableObjects, size>::Update();
        return EmIntOperationResult::canContinue;    
    }
};


#endif