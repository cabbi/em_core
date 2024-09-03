#ifndef __INTERFACE__H_
#define __INTERFACE__H_

#include "em_timeout.h"
#include "em_log.h"
#include "em_sync_value.h"

enum EmInterfaceStatus {
    is_None        = 0x0000,
    is_Initialized = 0x0001, // Correctly initialized
    is_Running     = 0x0002, // Running or Blocked (whachdog)
    is_Warning     = 0x0004, // Has any warning
    is_Error       = 0x0008, // Has any error
};

inline EmInterfaceStatus operator~ (EmInterfaceStatus a) { return static_cast<EmInterfaceStatus>(~static_cast<int>(a)); }
inline EmInterfaceStatus operator|(EmInterfaceStatus a, EmInterfaceStatus b) { return static_cast<EmInterfaceStatus>(static_cast<int>(a) | static_cast<int>(b)); }
inline EmInterfaceStatus operator&(EmInterfaceStatus a, EmInterfaceStatus b) { return static_cast<EmInterfaceStatus>(static_cast<int>(a) & static_cast<int>(b)); }
inline EmInterfaceStatus& operator|=(EmInterfaceStatus& a, EmInterfaceStatus b) { return (EmInterfaceStatus&)((int&)(a) |= static_cast<int>(b)); }
inline EmInterfaceStatus& operator&=(EmInterfaceStatus& a, EmInterfaceStatus b) { return (EmInterfaceStatus&)((int&)(a) &= static_cast<int>(b)); }

#define MAX_INTERFACE_MSG_LEN 60

// This is the base interface class.
// Each interface should implement these methods
class EmInterface: public EmLog {
public:
    EmInterface(bool logEnabled=false)
     : EmLog(logEnabled),
       m_InterfaceStatus(is_None),
       m_RunningTimeout((uint32_t)60*1000)
    { 
        memset(m_WarningMsg, 0, sizeof(m_WarningMsg));
        memset(m_ErrorMsg, 0, sizeof(m_ErrorMsg));
    }
    
    virtual void Setup(EmUpdateableValue* syncValues)=0;
    virtual void Loop()=0;
    
    // Status handling
    virtual bool IsInitialized() const { return GetStatusFlag(is_Initialized); }
    virtual bool IsRunning()     const { return GetStatusFlag(is_Running) && !m_RunningTimeout.IsElapsed(false); }
    virtual bool HasWarning()    const { return GetStatusFlag(is_Warning); }
    virtual bool HasError()      const { return GetStatusFlag(is_Error); }
    // Initialized, running and no errors
    virtual bool IsOk()          const { return IsInitialized() && IsRunning() && !HasError(); }

    virtual void SetInitialized(bool value)
        { SetStatusFlag(is_Initialized, value); }
    
    virtual void SetRunning(bool value)
        { if (value) m_RunningTimeout.Restart();
          SetStatusFlag(is_Running, value); }
    
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

#endif