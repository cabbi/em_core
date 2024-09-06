#include <string.h>
#include "em_app_interface.h"

void EmAppInterface::SetWarning(bool value, const char* msg)
{
    // Any change? (avoid log same stuff many times)
    if (value == GetStatusFlag(EmInterfaceStatus::isWarning) &&
        ((msg == NULL && m_WarningMsg[0]==0) ||
          0==strcmp(msg, m_WarningMsg)))
        return;
    // Set new status
    SetStatusFlag(EmInterfaceStatus::isWarning, value); 
    memset(m_WarningMsg, 0, sizeof(m_WarningMsg));
    if (msg) 
    {
        strncpy(m_WarningMsg, msg, MAX_INTERFACE_MSG_LEN); 
    }
}
    
void EmAppInterface::SetError(bool value, const char* msg) 
{
    // Any change? (avoid log same stuff many times)
    if (value == GetStatusFlag(EmInterfaceStatus::isError) &&
        ((msg == NULL && m_ErrorMsg[0]==0) ||
          0==strcmp(msg, m_ErrorMsg)))
        return;
    // Set new status
    SetStatusFlag(EmInterfaceStatus::isError, value); 
    memset(m_ErrorMsg, 0, sizeof(m_ErrorMsg));
    if (msg) 
    {
        strncpy(m_ErrorMsg, msg, MAX_INTERFACE_MSG_LEN); 
    }
}
