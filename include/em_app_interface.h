#ifndef __APP_INTERFACE__H_
#define __APP_INTERFACE__H_

#include <string.h>

#include "em_log.h"
#include "em_list.h"
#include "em_timeout.h"

class EmAppInterface;

enum class EmInterfaceStatus {
    isNone        = 0x0000,
    isInitialized = 0x0001, // Correctly initialized
    isRunning     = 0x0002, // Running or Blocked (in case running timeout is elapsed!)
    isWarning     = 0x0004, // Has any warning
    isError       = 0x0008, // Has any error
};

enum class EmIntOperationResult {
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
    EmAppInterface(uint32_t runningTimeoutMs = 60000, bool logEnabled=false)
     : EmLog(logEnabled),
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

#endif