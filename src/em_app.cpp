#include "em_app.h"


void EmApp::setup_() {
    m_runningInterfaces.set(*this, false);
    beforeInterfacesSetup();
    loop();  // This will call the 'setup' method of each interface until
             // interface returns tro to 'isInitialized' method
    afterInterfacesSetup();
}

void EmApp::loop_() {
    beforeInterfacesLoop();

    struct LoopContext {
        EmApp* self;
        EmIntOperationResult res;
    };
    LoopContext context = { this, EmIntOperationResult::canContinue };

    m_runningInterfaces.forEach<LoopContext>([](EmAppInterface& interface, bool, bool, LoopContext* pCtx) -> EmIterResult {
            bool failed = false;
            pCtx->res = interface.loopStep_(failed);
            switch (pCtx->res) {
                case EmIntOperationResult::stopInterface:
                    return EmIterResult::removeMoveNext;
                case EmIntOperationResult::restartApp:
                    return EmIterResult::stopFailed;
                case EmIntOperationResult::stopApp:
                    return EmIterResult::stopFailed;
                default:
                    break; // Just to keep compiler happy
            }
            // You can now access the EmApp instance via pCtx->self
            return EmIterResult::moveNext;
        }, &context);

    if (context.res == EmIntOperationResult::restartApp ||
        context.res == EmIntOperationResult::stopApp) {
        stop_(context.res);
    }
    
    afterInterfacesLoop();
}

void EmApp::stop_(EmIntOperationResult reason) {
    struct LoopContext {
        EmApp* self;
        EmIntOperationResult reason;
    };
    LoopContext context = { this, reason };

    // No more running interfaces
    m_runningInterfaces.clear();
    // Notify all interfaces about stop event
    forEach<LoopContext>(
        [](EmAppInterface& interface, bool, bool, LoopContext* pCtx) -> EmIterResult {
            interface.onStop(pCtx->reason);
            interface.setInitialized(false);
            if (pCtx->reason == EmIntOperationResult::stopApp) {
                return EmIterResult::removeMoveNext;
            } else if (pCtx->reason == EmIntOperationResult::restartApp) {
                pCtx->self->m_runningInterfaces.appendUnowned(interface);
                return EmIterResult::moveNext;
            }
            return EmIterResult::moveNext;
        }, &context);
    // On stop event
    onStop(reason);
}
