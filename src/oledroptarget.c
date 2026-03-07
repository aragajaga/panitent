#include "precomp.h"

#include "oledroptarget.h"

typedef struct OleFileDropTarget OleFileDropTarget;
struct OleFileDropTarget
{
    IDropTarget iface;
    LONG cRef;
    HWND hWnd;
    void* pContext;
    OleFileDropTargetOnDropFiles pfnOnDropFiles;
    BOOL fCanAccept;
};

static HRESULT STDMETHODCALLTYPE OleFileDropTarget_QueryInterface(IDropTarget* pInterface, REFIID riid, void** ppvObject);
static ULONG STDMETHODCALLTYPE OleFileDropTarget_AddRef(IDropTarget* pInterface);
static ULONG STDMETHODCALLTYPE OleFileDropTarget_Release(IDropTarget* pInterface);
static HRESULT STDMETHODCALLTYPE OleFileDropTarget_DragEnter(IDropTarget* pInterface, IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
static HRESULT STDMETHODCALLTYPE OleFileDropTarget_DragOver(IDropTarget* pInterface, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
static HRESULT STDMETHODCALLTYPE OleFileDropTarget_DragLeave(IDropTarget* pInterface);
static HRESULT STDMETHODCALLTYPE OleFileDropTarget_Drop(IDropTarget* pInterface, IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
static BOOL OleFileDropTarget_CanAcceptDataObject(IDataObject* pDataObject);

static IDropTargetVtbl g_oleFileDropTargetVtbl = {
    OleFileDropTarget_QueryInterface,
    OleFileDropTarget_AddRef,
    OleFileDropTarget_Release,
    OleFileDropTarget_DragEnter,
    OleFileDropTarget_DragOver,
    OleFileDropTarget_DragLeave,
    OleFileDropTarget_Drop
};

static OleFileDropTarget* OleFileDropTarget_FromInterface(IDropTarget* pInterface)
{
    return (OleFileDropTarget*)pInterface;
}

static BOOL OleFileDropTarget_CanAcceptDataObject(IDataObject* pDataObject)
{
    FORMATETC formatEtc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    if (!pDataObject)
    {
        return FALSE;
    }

    return SUCCEEDED(pDataObject->lpVtbl->QueryGetData(pDataObject, &formatEtc));
}

HRESULT OleFileDropTarget_Register(HWND hWnd, OleFileDropTargetOnDropFiles pfnOnDropFiles, void* pContext, IDropTarget** ppDropTarget)
{
    OleFileDropTarget* pDropTarget = NULL;
    HRESULT hr = E_INVALIDARG;

    if (ppDropTarget)
    {
        *ppDropTarget = NULL;
    }

    if (!hWnd || !IsWindow(hWnd) || !pfnOnDropFiles || !ppDropTarget)
    {
        return E_INVALIDARG;
    }

    pDropTarget = (OleFileDropTarget*)calloc(1, sizeof(OleFileDropTarget));
    if (!pDropTarget)
    {
        return E_OUTOFMEMORY;
    }

    pDropTarget->iface.lpVtbl = &g_oleFileDropTargetVtbl;
    pDropTarget->cRef = 1;
    pDropTarget->hWnd = hWnd;
    pDropTarget->pContext = pContext;
    pDropTarget->pfnOnDropFiles = pfnOnDropFiles;
    pDropTarget->fCanAccept = FALSE;

    hr = RegisterDragDrop(hWnd, &pDropTarget->iface);
    if (FAILED(hr))
    {
        pDropTarget->iface.lpVtbl->Release(&pDropTarget->iface);
        return hr;
    }

    *ppDropTarget = &pDropTarget->iface;
    return S_OK;
}

void OleFileDropTarget_Revoke(HWND hWnd, IDropTarget** ppDropTarget)
{
    if (!ppDropTarget || !*ppDropTarget)
    {
        return;
    }

    if (hWnd && IsWindow(hWnd))
    {
        RevokeDragDrop(hWnd);
    }

    (*ppDropTarget)->lpVtbl->Release(*ppDropTarget);
    *ppDropTarget = NULL;
}

static HRESULT STDMETHODCALLTYPE OleFileDropTarget_QueryInterface(IDropTarget* pInterface, REFIID riid, void** ppvObject)
{
    if (!ppvObject)
    {
        return E_POINTER;
    }

    *ppvObject = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDropTarget))
    {
        *ppvObject = pInterface;
        OleFileDropTarget_AddRef(pInterface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE OleFileDropTarget_AddRef(IDropTarget* pInterface)
{
    OleFileDropTarget* pDropTarget = OleFileDropTarget_FromInterface(pInterface);
    return (ULONG)InterlockedIncrement(&pDropTarget->cRef);
}

static ULONG STDMETHODCALLTYPE OleFileDropTarget_Release(IDropTarget* pInterface)
{
    OleFileDropTarget* pDropTarget = OleFileDropTarget_FromInterface(pInterface);
    ULONG cRef = (ULONG)InterlockedDecrement(&pDropTarget->cRef);
    if (cRef == 0)
    {
        free(pDropTarget);
    }
    return cRef;
}

static HRESULT STDMETHODCALLTYPE OleFileDropTarget_DragEnter(IDropTarget* pInterface, IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    OleFileDropTarget* pDropTarget = OleFileDropTarget_FromInterface(pInterface);
    UNREFERENCED_PARAMETER(grfKeyState);
    UNREFERENCED_PARAMETER(pt);

    pDropTarget->fCanAccept = OleFileDropTarget_CanAcceptDataObject(pDataObject);
    if (pdwEffect)
    {
        *pdwEffect = pDropTarget->fCanAccept ? DROPEFFECT_COPY : DROPEFFECT_NONE;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleFileDropTarget_DragOver(IDropTarget* pInterface, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    OleFileDropTarget* pDropTarget = OleFileDropTarget_FromInterface(pInterface);
    UNREFERENCED_PARAMETER(grfKeyState);
    UNREFERENCED_PARAMETER(pt);

    if (pdwEffect)
    {
        *pdwEffect = pDropTarget->fCanAccept ? DROPEFFECT_COPY : DROPEFFECT_NONE;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleFileDropTarget_DragLeave(IDropTarget* pInterface)
{
    OleFileDropTarget* pDropTarget = OleFileDropTarget_FromInterface(pInterface);
    pDropTarget->fCanAccept = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE OleFileDropTarget_Drop(IDropTarget* pInterface, IDataObject* pDataObject, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    OleFileDropTarget* pDropTarget = OleFileDropTarget_FromInterface(pInterface);
    FORMATETC formatEtc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgMedium = { 0 };
    BOOL fHandled = FALSE;

    UNREFERENCED_PARAMETER(grfKeyState);
    UNREFERENCED_PARAMETER(pt);

    if (!pDropTarget->fCanAccept && !OleFileDropTarget_CanAcceptDataObject(pDataObject))
    {
        if (pdwEffect)
        {
            *pdwEffect = DROPEFFECT_NONE;
        }
        return S_OK;
    }

    if (FAILED(pDataObject->lpVtbl->GetData(pDataObject, &formatEtc, &stgMedium)))
    {
        pDropTarget->fCanAccept = FALSE;
        if (pdwEffect)
        {
            *pdwEffect = DROPEFFECT_NONE;
        }
        return S_OK;
    }

    if (pDropTarget->pfnOnDropFiles)
    {
        fHandled = pDropTarget->pfnOnDropFiles(pDropTarget->pContext, (HDROP)stgMedium.hGlobal);
    }

    ReleaseStgMedium(&stgMedium);
    pDropTarget->fCanAccept = FALSE;

    if (pdwEffect)
    {
        *pdwEffect = fHandled ? DROPEFFECT_COPY : DROPEFFECT_NONE;
    }

    return S_OK;
}
