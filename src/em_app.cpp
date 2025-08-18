#include "em_app.h"

void EmApp::run(uint32_t loopDelayMillis) {
    bool running = true;
    bool restart = false;
    EmAppInterfaces runningInterfaces;  

    // Create a copy of the running interfaces list just in case some interfaces and
    // removed and then app is asked to restart -> need all original interfaces list!
    runningInterfaces.append(m_interfaces);

    // The app running loop 
    while (running) {
        // Iteration of each interface
        bool iterResult = runningInterfaces.forEach([&restart, &running, &runningInterfaces](EmAppInterface& interface) -> EmIterResult {
            EmIntOperationResult res;
            if (interface.isInitialized()) {
                res = interface.canCallLoop() ? interface.loop() : EmIntOperationResult::canContinue;
            } else {
                res = interface.setup();
                if (res == EmIntOperationResult::canContinue) {
                    interface.setInitialized(true);
                }
            }
            switch (res) {
                case EmIntOperationResult::removeInterface:
                    return EmIterResult::removeMoveNext;
                case EmIntOperationResult::restartApp:
                    restart = true;
                    return EmIterResult::stopFailed;
                case EmIntOperationResult::exitApp:
                    running = false;
                    return EmIterResult::stopFailed;
                case EmIntOperationResult::canContinue:
                    break; // Just to keep compiler happy
            }
            return EmIterResult::moveNext;
        });  
        // Restart application requested?
        if (restart) {
            // Restore all application interfaces and set them as "uninitialized"
            restart = false;
            runningInterfaces.clear();
            m_interfaces.forEach([](EmAppInterface& interface) -> EmIterResult {
                interface.setInitialized(false);
                runningInterfaces.Append(interface);
                return EmIterResult::moveNext;
            });
        } 
        // We do wait next iteration in case of iteration success
        else if (loopDelayMillis && iterResult) {      
            delay(loopDelayMillis);
        }
    }
}