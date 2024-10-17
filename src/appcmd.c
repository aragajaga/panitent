#include "precomp.h"

#include "appcmd.h"
#include "util/assert.h"
#include "panitentapp.h"

int AppCmd_KeyCompare(void* pKey1, void* pKey2) {
    if ((UINT)pKey1 == (UINT)pKey2) {
        return 0;
    }
    else if ((UINT)pKey1 > (UINT)pKey2) {
        return 1;
    }
    
    return -1;
}

void AppCmd_Init(AppCmd* pAppCmd) {
    ASSERT(pAppCmd);
    memset(pAppCmd, 0, sizeof(AppCmd));

    HashMap* pHashMap = HashMap_Create(16, &AppCmd_KeyCompare);
    ASSERT(pHashMap);
    pAppCmd->pCmdMap = pHashMap;
}

void AppCmd_AddCommand(AppCmd* pAppCmd, UINT cmdId, PFNAppCmdCallback* pfnCallback) {
    ASSERT(pAppCmd);
    ASSERT(pfnCallback);
    HashMap_Put(pAppCmd->pCmdMap, (void*)cmdId, (void*)pfnCallback);
}

void AppCmd_Execute(AppCmd* pAppCmd, UINT cmdId, PanitentApp* pPanitentApp) {
    ASSERT(pAppCmd);
    PFNAppCmdCallback *pfnCallback = (PFNAppCmdCallback*)HashMap_Get(pAppCmd->pCmdMap, (void*)cmdId);
    ASSERT(pfnCallback);
    pfnCallback(pPanitentApp);
}

void AppCmd_Destroy(AppCmd* pAppCmd) {
    ASSERT(pAppCmd);
    HashMap_Destroy(pAppCmd->pCmdMap);
    pAppCmd->pCmdMap = NULL;
}
