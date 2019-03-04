#include "file_open.h"

COMDLG_FILTERSPEC c_rgSaveTypes[1] = {{L"Portable Network Graphics", L"*.png"}};

int FileOpen()
{
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IFileDialog *pfd = NULL;
        hr = CoCreateInstance(
                &CLSID_FileOpenDialog,
                NULL,
                CLSCTX_INPROC_SERVER,
                &IID_IFileDialog,
                (void **)(&pfd));
        
        if (SUCCEEDED(hr))
        {
            hr = pfd->lpVtbl->SetFileTypes(pfd, ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
            if (SUCCEEDED(hr))
            {
                hr = pfd->lpVtbl->Show(pfd, NULL);
                if (SUCCEEDED(hr))
                {
                    IShellItem *psiResult;
                    hr = pfd->lpVtbl->GetResult(pfd, &psiResult);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath = NULL;
                        hr = psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH, &pszFilePath);
                        if (SUCCEEDED(hr))
                        {
                            MessageBoxW(NULL, pszFilePath, pszFilePath, MB_OK);
                            CoTaskMemFree(pszFilePath);
                        }
                        psiResult->lpVtbl->Release(psiResult);
                    }
                }
            }
            pfd->lpVtbl->Release(pfd);
        }
    }
    
    return 0;
}

int FileSave()
{
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        IFileDialog *pfd = NULL;
        hr = CoCreateInstance(
                &CLSID_FileSaveDialog,
                NULL,
                CLSCTX_INPROC_SERVER,
                &IID_IFileDialog,
                (void **)(&pfd));
        
        if (SUCCEEDED(hr))
        {
            hr = pfd->lpVtbl->SetFileTypes(pfd, ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
            if (SUCCEEDED(hr))
            {
                hr = pfd->lpVtbl->Show(pfd, NULL);
                if (SUCCEEDED(hr))
                {
                    IShellItem *psiResult;
                    hr = pfd->lpVtbl->GetResult(pfd, &psiResult);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath = NULL;
                        hr = psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH, &pszFilePath);
                        if (SUCCEEDED(hr))
                        {
                            MessageBoxW(NULL, pszFilePath, pszFilePath, MB_OK);
                            CoTaskMemFree(pszFilePath);
                        }
                        psiResult->lpVtbl->Release(psiResult);
                    }
                }
            }
            pfd->lpVtbl->Release(pfd);
        }
    }
    
    return 0;
}
