#ifndef PANITENT_APPCMD_H
#define PANITENT_APPCMD_H

#include "util/hashmap.h"

typedef struct PanitentApp PanitentApp;

typedef void (PFNAppCmdCallback)(PanitentApp*);

typedef struct AppCmd AppCmd;
typedef struct AppCmd {
    HashMap* pCmdMap;
};

/* Public Interface */
void AppCmd_Init(AppCmd* pAppCmd);
void AppCmd_AddCommand(AppCmd* pAppCmd, UINT cmdId, PFNAppCmdCallback* pfnCallback);
void AppCmd_Execute(AppCmd* pAppCmd, UINT cmdId, PanitentApp* pPanitentApp);
void AppCmd_Destroy(AppCmd* pAppCmd);

#endif  /* PANITENT_APPCMD_H */
