#ifndef __EM_APP_INTERFACE__H_
#define __EM_APP_INTERFACE__H_

#include <string.h>

#include "em_defs.h"
#include "em_log.h"
#include "em_list.h"
#include "em_threading.h"
#include "em_duration.h"
#include "em_timeout.h"

class EmAppInterfaces;

// Inteface statuses
namespace EmInterfaceStatusFlag {
    constexpr uint8_t none          = 0x0000;
    constexpr uint8_t isInitialized = 0x0001; // Is correctly initialized (app will call 'setup' instead of 'loop' until this flag is not set)
    constexpr uint8_t hasWarning    = 0x0010; // Has any warning
    constexpr uint8_t hasError      = 0x0020; // Has any error

    using Type = uint8_t;
    using TypeInternal = ts_uint8;
}

// The setup and loop function return codes
enum class EmIntOperationResult: uint8_t {
    canContinue = 0,    // The interface can continue running
    stopInterface = 2,  // The interface should stop running
    restartApp = 3,     // The application (i.e. all interfaces) should restart
    stopApp = 4,        // The application (i.e. all interfaces) should stop
    // Failure flag to indicate function failed 
    failureFlag = 0x80,
};

// Helper to check if operation result has failure flag.
// NOTE: The failure flag will be removed from the returned result.
inline EmIntOperationResult hasFailure(EmIntOperationResult result, bool& hasFailure) {
    hasFailure = (static_cast<int8_t>(result) & 
                  static_cast<int8_t>(EmIntOperationResult::failureFlag)) != 0;
    return static_cast<EmIntOperationResult>(
                static_cast<int8_t>(result) & 
                ~static_cast<int8_t>(EmIntOperationResult::failureFlag));
}

// Helper to add failure flag to operation result.
inline EmIntOperationResult setFailure(EmIntOperationResult result, bool hasFailure) {
    return static_cast<EmIntOperationResult>(
                static_cast<int8_t>(result) | 
                (hasFailure ? static_cast<int8_t>(EmIntOperationResult::failureFlag) : 0));
}

// The max interface warning and error messages length
#define MAX_INTERFACE_MSG_LEN 60

// This is the base interface class.
//
// Each interface should implement 'setup' & 'loop' methods. 
// The application will call 'setup' until it returns 'canContinue' and then will call 'loop'.
// If interface setup should be called again on next loop then it can return 'canContinueFailure'.
//
// Consider overriding 'onStop' in case your application might free resources or restart
// since interfaces can request 'EmIntOperationResult::restartApp'.
class EmAppInterface: public EmLog {
    friend class EmApp;
public:
    /// @brief Interface constructor
    /// @param name The interface name
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppInterface(const char* name,
                   const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                   EmLogLevel logLevel=EmLogLevel::global)
     : EmLog(name, logLevel),
    #ifdef EM_NO_LOG
        m_name(name),
    #endif
       m_interfaceStatus(EmInterfaceStatusFlag::none),
       m_blockedTimeout(blockedTimeout) { 
        clear_();
    }

    /// @brief Interface constructor
    /// @param name The interface name
    /// @param appInterfaces Add this instance to the application interfaces 
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppInterface(const char* name,
                   EmList<EmAppInterface>& appInterfaces,
                   const EmDuration& blockedTimeout = EmDuration(0, 1, 0), 
                   EmLogLevel logLevel=EmLogLevel::global)
        : EmAppInterface(name, blockedTimeout, logLevel) {
        appInterfaces.appendUnowned(*this);
    }
    
    virtual ~EmAppInterface() {
        clear_();
    }

    virtual EmIntOperationResult setup() = 0;
    virtual EmIntOperationResult loop() = 0;

#ifdef EM_NO_LOG
    virtual const char* name() const {
        return m_name;
    };
#else
    virtual const char* name() const {
        return getContext();
    };
#endif


    // Override this in case app should not call interface 'loop' method all the times
    virtual bool canCallLoop() { return true; }

    // Called if interface needs to stop for one of the following reasons
    // 'EmIntOperationResult::stopInterface', 'EmIntOperationResult::restartApp' or 'EmIntOperationResult::stopApp'
    virtual void onStop(EmIntOperationResult /*reason*/) { 
        // Do some cleanup if needed
    }
    
    // Status handling
    virtual bool isInitialized() const { return getStatusFlag_(EmInterfaceStatusFlag::isInitialized); }
    virtual bool hasWarning()    const { return getStatusFlag_(EmInterfaceStatusFlag::hasWarning); }
    virtual bool hasError()      const { return getStatusFlag_(EmInterfaceStatusFlag::hasError); }
    virtual bool isBlocked()     const { return m_blockedTimeout.isExpired(false); }
    // Initialized and no errors
    virtual bool isOk()          const { return isInitialized() && !hasError(); }

    virtual void setInitialized(bool value) { 
        setStatusFlag_(EmInterfaceStatusFlag::isInitialized, value); }
       
    virtual void setWarning(bool value, const char* msg="");
    virtual void setError(bool value, const char* msg="");

    virtual const char* getWarningMsg() const { return m_warningMsg; }
    virtual const char* getErrorMsg() const { return m_errorMsg; }

    virtual void printStatus() const {
        // Override this so any interfaces handler (e.g. an "EmApp' instance") can print the interface status
    }

    // Used to match two interfaces (by name) in a EmList.
    static bool match(const EmAppInterface& int1, const EmAppInterface& int2) {
        return 0==strcmp(int1.name(), int2.name());
    }
  
    // The internal loop step function called by the application
    // NOTE: the resulting EmIntOperationResult will have the failure flag removed.
    virtual EmIntOperationResult loopStep_(bool& failed) {
        EmIntOperationResult res;
        failed = false;
        if (!isInitialized()) {
            res = hasFailure(setup(), failed);
            setInitialized(!failed);
        } else {
            if (canCallLoop()) {
                res = hasFailure(loop(), failed);
            } else {
                res = EmIntOperationResult::canContinue;
            }
        }
        return res;
    }

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

#ifdef EM_NO_LOG
    const char* m_name;
#endif
    EmInterfaceStatusFlag::TypeInternal m_interfaceStatus; 
    mutable EmTimeout m_blockedTimeout;
    char m_warningMsg[MAX_INTERFACE_MSG_LEN+1];
    char m_errorMsg[MAX_INTERFACE_MSG_LEN+1];
};

// The interfaces list class
class EmAppInterfaces: public EmList<EmAppInterface> {
public:
    EmAppInterfaces() : EmList<EmAppInterface>(&EmAppInterface::match) {}
};

// This interface has a loop call timeout, app will call the 'loop' 
// method each time timeout elapses
class EmAppTimeoutInterface: public EmAppInterface {
public:
    /// @brief Interface constructor
    /// @param name The interface name
    /// @param loopTimeout The timeout for each loop call (i.e. how often the loop method should be called). 
    /// @param startAsElapsed Set to true to let the first loop call immediately.
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppTimeoutInterface(const char* name,
                          EmDuration loopTimeout, 
                          bool startAsElapsed=true,
                          EmDuration blockedTimeout = EmDuration(0, 1, 0),
                          EmLogLevel logLevel=EmLogLevel::global)
     : EmAppInterface(name, blockedTimeout, logLevel), 
       m_LoopTimeout(loopTimeout, startAsElapsed) {}

    /// @brief Interface constructor
    /// @param name The interface name
    /// @param loopTimeout The timeout for each loop call (i.e. how often the loop method should be called). 
    /// @param startAsElapsed Set to true to let the first loop call immediately.
    /// @param appInterfaces Add this instance to the application interfaces 
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppTimeoutInterface(const char* name,
                          EmDuration loopTimeout, 
                          EmList<EmAppInterface>& interfaces,
                          bool startAsElapsed=true,
                          EmDuration blockedTimeout = EmDuration(0, 1, 0),
                          EmLogLevel logLevel=EmLogLevel::global) 
     : EmAppInterface(name, interfaces, blockedTimeout, logLevel), 
       m_LoopTimeout(loopTimeout, startAsElapsed) {}

    virtual bool canCallLoop() { return m_LoopTimeout.isExpired(true); }

private:
    mutable EmTimeout m_LoopTimeout;
};

// This interface will update each 'EmUpdatable' object at each 'Loop'.
template <EmUpdatable* updatableObjects[], uint8_t size>
class EmAppUpdaterInterface: public EmAppInterface, 
                             public EmUpdater<updatableObjects, size> {
public:
    /// @brief Interface constructor
    /// @param name The interface name
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppUpdaterInterface(const char* name,
                          const EmDuration& blockedTimeout = EmDuration(0, 1, 0),
                          EmLogLevel logLevel=EmLogLevel::none) : 
        EmAppInterface(name, blockedTimeout, logLevel) {}

    /// @brief Interface constructor
    /// @param name The interface name
    /// @param appInterfaces Add this instance to the application interfaces 
    /// @param blockedTimeout The timeout when to consider this interface blocked if not exiting the loop function in time.
    /// @param logLevel The log level of this interface
    EmAppUpdaterInterface(const char* name,
                          EmAppInterfaces& interfaces,
                          const EmDuration& blockedTimeout = EmDuration(0, 1, 0),
                          EmLogLevel logLevel=EmLogLevel::none) 
     : EmAppInterface(name, interfaces, blockedTimeout, logLevel) {}
    
    virtual EmIntOperationResult setup() {
        return EmIntOperationResult::canContinue;
    }

    virtual EmIntOperationResult loop() {
        EmUpdater<updatableObjects, size>::update();
        return EmIntOperationResult::canContinue;    
    }
};


#endif