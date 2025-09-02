#ifndef __EM_APP__H_
#define __EM_APP__H_

#include "em_defs.h"
#include "em_log.h"
#include "em_app_interface.h"

// This is the application class you can run withing your code.
//
// By using this EmApp object you can manage multiple application interfaces. 
// Each interface will setup and run the main loop. Interfaces can drive the application
// workflow by removing themselves and restarting or stopping the application.
class EmApp: public EmLog
{
public:
    EmApp(const char* logContext = "App", 
          EmLogLevel logLevel = EmLogLevel::global) 
     : EmLog(logContext, logLevel), m_appInterfaces() {};
    
    virtual ~EmApp() {
        m_appInterfaces.clear();
        m_runningInterfaces.clear();
    }

    // Adds an interface object to the application.
    // NOTE that the object will NOT be owned by the application 
    // so it must outlive the application.
    virtual void addInterface(EmAppInterface& interface) {
        m_appInterfaces.appendUnowned(interface);
    }

    virtual void setup() { setup_(); }
    virtual void loop() { loop_(); }

    virtual void beforeInterfacesSetup() {
        // Do some preparation if needed
    }

    virtual void afterInterfacesSetup() {
        // Do some preparation if needed
    }

    // Called before application restarts or stops due to an interface requesting
    // 'EmIntOperationResult::restartApp' or 'EmIntOperationResult::stopApp'
    virtual void onStop(EmIntOperationResult /*reason*/) { 
        // Do some cleanup if needed
    }

    bool isRunning() const {
        return !m_runningInterfaces.isEmpty();
    }
    
protected:
    virtual void setup_();
    virtual void loop_();
    virtual void stop_(EmIntOperationResult reason);

    EmAppInterfaces m_appInterfaces;
    EmAppInterfaces m_runningInterfaces;
};

#endif