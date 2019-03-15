#include "file_open.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
// #include <png.h>

/*void user_error_fn(png_structp png_ptr, png_const_charp message) {
	printf("[libpng] We encountered a fatal error.\n");
	if (png_ptr) fprintf(stderr, "[libpng] libpng error: %s\n", message);
	exit(1);
}

void user_warning_fn(png_structp png_ptr, png_const_charp message) {
	printf("[libpng internal] We encountered a warning.\n");
	if (png_ptr) fprintf(stderr, "[libpng internal] libpng warning: %s\n", message);
} */

void FileOpenPng(LPWSTR pszPath)
{
    /*
    FILE *fp = _wfopen(pszPath, L"rb");
    if (!fp)
        exit(1);
    
    unsigned char header[8];
    int number = 8;
    if (fread(header, 1, number, fp) != number)
    {
        printf("[FileOpenPng] Failed to read file signature bits\n");
        return;
    }
    else {
        printf("[FileOpenPng] File signature bits read\n");
    }
    
    if (png_sig_cmp(header, 0, number))
    {
        printf("[FileOpenPng] png_sig_cmp returned that is not PNG file\n");
        return;
    }
    else {
        printf("[FileOpenPng] PNG signature OK\n");
    }
    
    png_structp png_ptr;
    png_infop info_ptr;
    
    png_ptr = png_create_read_struct(
            PNG_LIBPNG_VER_STRING,
            NULL,
            (png_error_ptr)user_error_fn,
            (png_error_ptr)user_warning_fn);
    if (!png_ptr)
    {
        printf("[libpng] Failed to create read struct\n");
        return;
    }
    else {
        printf("[libpng] Read struct created\n");
    }
    
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        printf("[libpng] Failed to create info struct\n");
        return;
    }
    else {
        printf("[libpng] Info struct created\n");
    }
    */
}

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
                            wprintf(L"[FileOpen] Path: %ls\n", pszFilePath);
                            FileOpenPng(pszFilePath);
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
