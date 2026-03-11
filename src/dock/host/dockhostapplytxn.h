#pragma once

#include "dockhostapplycore.h"

typedef struct DockHostApplyTxn
{
    DockModelNode* pRollbackModel;
    DockModelNode* pTargetModel;
    DockHostModelApplyContext context;
} DockHostApplyTxn;

BOOL DockHostApplyTxn_Begin(DockHostApplyTxn* pTxn, DockHostWindow* pDockHostWindow, HWND hWndIncludeHidden);
void DockHostApplyTxn_Rollback(DockHostWindow* pDockHostWindow, DockHostApplyTxn* pTxn);
void DockHostApplyTxn_End(DockHostApplyTxn* pTxn);
