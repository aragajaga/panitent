#pragma once

#include "precomp.h"

#include <oleidl.h>
#include <shellapi.h>

typedef BOOL (*OleFileDropTargetOnDropFiles)(void* pContext, HDROP hDrop);

HRESULT OleFileDropTarget_Register(HWND hWnd, OleFileDropTargetOnDropFiles pfnOnDropFiles, void* pContext, IDropTarget** ppDropTarget);
void OleFileDropTarget_Revoke(HWND hWnd, IDropTarget** ppDropTarget);
