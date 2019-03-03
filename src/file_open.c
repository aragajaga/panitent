#include "file_open.h"

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
    {L"Bitmap (*.bmp)",                             L"*.bmp"},
    {L"Portable Network Graphics (*.png)",          L"*.png"},
    {L"Joint Picture Expert Group (*.jpg; *.jpeg)", L"*.jpg;*.jpeg"},
    {L"Truevision TGA (*.tga)",                     L"*.tga"},
    {L"All Files (*.*)",                            L"*.*"}
};

HRESULT __stdcall DialogEventHandler_QueryInterface(DialogEventHandler* This,
        REFIID riid,
        void **ppvObject)
{
    /* static const QITAB qit[] = {
        QITABENT(DialogEventHandler, IFileDialogEvents),
        QITABENT(DialogEventHandler, IFileDialogControlEvents),
        { 0 },
    }; */

    // For unknown reasons MSVC can't use C definition of QITABENT macro
    // It expands it to "static_cast", what cause error with C compiling
    static const QITAB qit[] =
    {
        {
            (IID*) &IID_IFileDialogEvents,
            ((DWORD)(DWORD_PTR)((IFileDialogEvents*)((DialogEventHandler*)8))-8)
        },
        {
            (IID*) &IID_IFileDialogControlEvents,
            ((DWORD)(DWORD_PTR)((IFileDialogControlEvents*)((DialogEventHandler*)8))-8)
        },
        { 0 }
    };
    
    return QISearch(This, qit, riid, ppvObject);
}

ULONG __stdcall DialogEventHandler_AddRef(DialogEventHandler* This)
{
    return InterlockedIncrement((LONG *)&This->_cRef);
}

ULONG __stdcall DialogEventHandler_Release(DialogEventHandler* This)
{
    long cRef = InterlockedDecrement((LONG *)&This->_cRef);
    if (!cRef)
        GlobalFree(This);
    return cRef;
}

HRESULT __stdcall DialogEventHandler_Placeholder1(IFileDialog *pfd) {return S_OK;}
HRESULT __stdcall DialogEventHandler_Placeholder2(IFileDialog *pfd, IShellItem *a) {return S_OK;}
HRESULT __stdcall DialogEventHandler_OnShareViolation(IFileDialog *pfd, IShellItem *a, FDE_SHAREVIOLATION_RESPONSE *b) { return S_OK; }

HRESULT __stdcall DialogEventHandler_OnTypeChange(IFileDialog *pfd)
{
    IFileSaveDialog *pfsd;
    HRESULT hr = pfd->lpVtbl->QueryInterface(pfd, &IID_IFileSaveDialog, (void **)&pfsd);
    if (SUCCEEDED(hr))
    {
        UINT uIndex;
        hr = pfsd->lpVtbl->GetFileTypeIndex(pfsd, &uIndex);
        if (SUCCEEDED(hr))
        {
            IPropertyDescriptionList *pdl = NULL;

            switch (uIndex)
            {
            case INDEX_BMP:
                // Доп. метаданные
                hr = PSGetPropertyDescriptionListFromString(L"prop:System.Title", &IID_IPropertyDescriptionList, (void **)&pdl);
                if (SUCCEEDED(hr))
                {
                    hr = pfsd->lpVtbl->SetCollectedProperties(pfsd, pdl, FALSE);
                    pdl->lpVtbl->Release(pdl);
                }
                break;

            case INDEX_PNG:
                hr = PSGetPropertyDescriptionListFromString(L"prop:System.Keywords", &IID_IPropertyDescriptionList, (void **)&pdl);
                if (SUCCEEDED(hr))
                {
                    hr = pfsd->lpVtbl->SetCollectedProperties(pfsd, pdl, FALSE);
                    pdl->lpVtbl->Release(pdl);
                }
                break;

            case INDEX_JPG:
                hr = PSGetPropertyDescriptionListFromString(L"prop:System.Author", &IID_IPropertyDescriptionList, (void **)&pdl);
                if (SUCCEEDED(hr))
                {
                    hr = pfsd->lpVtbl->SetCollectedProperties(pfsd, pdl, TRUE);
                    pdl->lpVtbl->Release(pdl);
                }
                break;
            }
        }
        pfsd->lpVtbl->Release(pfsd);
    }
    return hr;
}

HRESULT __stdcall DialogEventHandler_OnOverwrite(IFileDialog *pfd, IShellItem *a, FDE_OVERWRITE_RESPONSE * b) { return S_OK; }

HRESULT __stdcall DialogEventHandler_OnItemSelected(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem)
{
    IFileDialog *pfd = NULL;
    HRESULT hr = pfdc->lpVtbl->QueryInterface(pfdc, &IID_IFileDialog, (void **)&pfd);
    if (SUCCEEDED(hr))
    {
        if (dwIDCtl == CONTROL_RADIOBUTTONLIST)
        {
            switch (dwIDItem)
            {
            case CONTROL_RADIOBUTTON1:
                hr = pfd->lpVtbl->SetTitle(pfd, L"Longhorn Dialog");
                break;

            case CONTROL_RADIOBUTTON2:
                hr = pfd->lpVtbl->SetTitle(pfd, L"Vista Dialog");
                break;
            }
        }
        pfd->lpVtbl->Release(pfd);
    }
    return hr;
}

HRESULT __stdcall DialogEventHandler_OnButtonClicked(IFileDialogCustomize *pfdc, DWORD a) { return S_OK; };
HRESULT __stdcall DialogEventHandler_OnCheckButtonToggled(IFileDialogCustomize *pfdc, DWORD a, BOOL b) { return S_OK; };
HRESULT __stdcall DialogEventHandler_OnControlActivating(IFileDialogCustomize *pfdc, DWORD a) { return S_OK; };

static DialogEventHandlerVtbl deh_vtbl = {
    DialogEventHandler_QueryInterface,
    DialogEventHandler_AddRef,
    DialogEventHandler_Release,
    
    DialogEventHandler_Placeholder1,    /* OnFileOk */
    DialogEventHandler_Placeholder1,    /* OnFolderChange */
    DialogEventHandler_Placeholder2,    /* OnFolderChanging */
    DialogEventHandler_Placeholder1,    /* OnHelp */
    DialogEventHandler_Placeholder1,    /* OnSelectionChange */
    DialogEventHandler_OnShareViolation,
    DialogEventHandler_Placeholder1,    /* OnTypeChange */
    DialogEventHandler_OnOverwrite,
    
    DialogEventHandler_OnItemSelected,
    DialogEventHandler_OnButtonClicked,
    DialogEventHandler_OnCheckButtonToggled,
    DialogEventHandler_OnControlActivating
};

int FileOpen(void)
{
    HRESULT hr;
    
    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IFileDialog *pFileDialog = NULL;
        hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFileDialog, (void **)(&pFileDialog));
        
        if (SUCCEEDED(hr))
        {
            /* Создание обработчика событий */
            IFileDialogEvents *pFileDialogEvents = NULL;
            DialogEventHandler *deh;
            deh = (DialogEventHandler *)GlobalAlloc(GMEM_FIXED, sizeof(DialogEventHandler));
            deh->lpVtbl = &deh_vtbl;
            deh->_cRef = 1;
            if (SUCCEEDED(hr)){
                hr = deh->lpVtbl->QueryInterface(deh, &IID_IFileDialogEvents, (void **)&pFileDialogEvents);
                deh->lpVtbl->Release(deh);
                if (SUCCEEDED(hr))
                {
                    /* Привязка обработчика событий к экземпляру */
                    DWORD dwCookie;
                    hr = pFileDialog->lpVtbl->Advise(pFileDialog, pFileDialogEvents, &dwCookie);
                    if (SUCCEEDED(hr))
                    {
                        /* Получение настроек созданного диалога по умолчанию */
                        DWORD dwFlags;
                        hr = pFileDialog->lpVtbl->GetOptions(pFileDialog, &dwFlags);
                        if (SUCCEEDED(hr))
                        {
                            /*  */
                            hr = pFileDialog->lpVtbl->SetOptions(pFileDialog, dwFlags | FOS_FORCEFILESYSTEM);
                            if (SUCCEEDED(hr))
                            {
                                /* Сделать так, чтобы отображались не все файлы, а только
                                определенные типы файлов. Кстати, эта хуйня зачем-то
                                принимает только одиночный массив*/
                                hr = pFileDialog->lpVtbl->SetFileTypes(pFileDialog, ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pFileDialog->lpVtbl->SetFileTypeIndex(pFileDialog, INDEX_BMP);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = pFileDialog->lpVtbl->SetDefaultExtension(pFileDialog, L"bmp");
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = pFileDialog->lpVtbl->Show(pFileDialog, NULL);
                                            if (SUCCEEDED(hr))
                                            {
                                                IShellItem *psiResult;
                                                hr = pFileDialog->lpVtbl->GetResult(pFileDialog, &psiResult);
                                                if (SUCCEEDED(hr))
                                                {
                                                    PWSTR pszFilePath = NULL;
                                                    hr = psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH, &pszFilePath);
                                                    if (SUCCEEDED(hr))
                                                    {
                                                        MessageBoxW(NULL,
                                                            L"CommonFileDialogApp",
                                                            pszFilePath,
                                                            MB_OK);
                                                        CoTaskMemFree(pszFilePath);
                                                    }
                                                    psiResult->lpVtbl->Release(psiResult);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        pFileDialog->lpVtbl->Unadvise(pFileDialog, dwCookie);
                    }
                    pFileDialogEvents->lpVtbl->Release(pFileDialogEvents);
                }
            }
            pFileDialog->lpVtbl->Release(pFileDialog);
        }
    }
    CoUninitialize();
    return 0;
}