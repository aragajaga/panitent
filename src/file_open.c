#include "precomp.h"

#include "file_open.h"

#include <shobjidl.h>

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

#include "win32/window.h"
#include "panitentapp.h"
#include "settings.h"
#include "util.h"

#define SAFE_RELEASE(obj) \
if ((obj) != NULL) { \
  (obj)->lpVtbl->Release(obj); \
  (obj) = NULL; \
}

/*
#include <png.h>

void user_error_fn(png_structp png_ptr, png_const_charp message)
{
  printf("[libpng] We encountered a fatal error.\n");
  if (png_ptr)
    fprintf(stderr, "[libpng] libpng error: %s\n", message);
  exit(1);
}

void user_warning_fn(png_structp png_ptr, png_const_charp message)
{
  printf("[libpng internal] We encountered a warning.\n");
  if (png_ptr)
    fprintf(stderr, "[libpng internal] libpng warning: %s\n", message);
}
*/

/*
void FileOpenPng(LPWSTR pszPath)
{
  FILE* fp = _wfopen(pszPath, L"rb");
  if (!fp)
    exit(1);

  unsigned char header[8];
  int number = 8;
  if (fread(header, 1, number, fp) != number) {
    printf("[FileOpenPng] Failed to read file signature bits\n");
    return;
  } else {
    printf("[FileOpenPng] File signature bits read\n");
  }

  if (png_sig_cmp(header, 0, number)) {
    printf("[FileOpenPng] png_sig_cmp returned that is not PNG file\n");
    return;
  } else {
    printf("[FileOpenPng] PNG signature OK\n");
  }

  png_structp png_ptr;
  png_infop info_ptr;

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                   NULL,
                                   (png_error_ptr)user_error_fn,
                                   (png_error_ptr)user_warning_fn);
  if (!png_ptr) {
    printf("[libpng] Failed to create read struct\n");
    return;
  } else {
    printf("[libpng] Read struct created\n");
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    printf("[libpng] Failed to create info struct\n");
    return;
  } else {
    printf("[libpng] Info struct created\n");
  }
}
*/

PNTMAP_DECLARE_TYPE(ExtensionMap, PWSTR, PWSTR)

typedef struct _tagPNTSAVEPARAMS {
    pntmap(ExtensionMap) m_extensionMap;
    PWSTR pszPath;
} PNTSAVEPARAMS, * PPNTSAVEPARAMS;

typedef struct _tagPNGOPENPARAMS {
    pntmap(ExtensionMap) m_extensionMap;
} PNTOPENPARAMS, * PPNTOPENPARAMS;

COMDLG_FILTERSPEC c_rgReadTypes[2] = {
  {L"Windows/OS2 Bitmap", L"*.bmp"},
  {L"Portable Network Graphics", L"*.png"},
};

COMDLG_FILTERSPEC c_rgSaveTypes[4] = {
  {L"Windows/OS2 Bitmap", L"*.bmp"},
  {L"Joint Picture Expert Group (JPEG)", L"*.jpg"},
  {L"Portable Network Graphics", L"*.png"},
  {L"Raw binary", L"*.*"}
};

void CoStringDtor(void* str)
{
    CoTaskMemFree(str);
}

BOOL Win32LegacyFileOpenDialog(LPWSTR* pszPath)
{
    BOOL bStatus;
    HWND hWnd = Window_GetHWND((Window *)PanitentApp_GetWindow(PanitentApp_Instance()));

    OPENFILENAME ofn = { 0 };

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrFile = *pszPath;
    ofn.lpstrFilter = L"PNG Files\0*.png\0All Files\0*.*\0";
    ofn.nMaxFile = 256;
    ofn.lpstrDefExt = L"zip";
    ofn.Flags = OFN_PATHMUSTEXIST;

    bStatus = GetOpenFileName(&ofn);

    *pszPath = ofn.lpstrFile;

    return bStatus;
}

BOOL COMFileOpenDialog(LPWSTR* pszPath)
{
    BOOL bResult;
    HRESULT hr;
    IFileDialog* pfd = NULL;
    IShellItem* psiResult = NULL;

    bResult = FALSE;
    hr = S_OK;

    hr = CoInitialize(NULL);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"CoInitialize failed");
        goto fail;
    }

    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
        &IID_IFileDialog, (LPVOID)&pfd);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"CoCreateInstance FileOpenDialog failed");
        goto fail;
    }

    hr = pfd->lpVtbl->SetFileTypes(pfd, ARRAYSIZE(c_rgReadTypes), c_rgReadTypes);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"FileOpenDialog::SetFileType failed");
        goto fail;
    }

    hr = pfd->lpVtbl->Show(pfd, NULL);
    assert(SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED));
    if (FAILED(hr)) {
        if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            OutputDebugString(L"FileOpenDialog::Show failed");
        }
        goto fail;
    }

    hr = pfd->lpVtbl->GetResult(pfd, &psiResult);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"FileOpenDialog::GetResult failed");
        goto fail;
    }

    /* CHECKME: POSSIBLY DANGLING UNFREED MEMORY
       What does CoTaskMemFree actually do?
       Should it be applied on COM strings explicitly to free them properly? */
    hr = psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH, pszPath);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"GetResult failed");
        goto fail;
    }

    bResult = TRUE;

fail:
    SAFE_RELEASE(psiResult)
        SAFE_RELEASE(pfd)

        return bResult;
}

BOOL Win32LegacyFileSaveDialog(LPWSTR* pszPath)
{
    BOOL bStatus;
    HWND hWnd = Window_GetHWND((Window*)PanitentApp_GetWindow(PanitentApp_Instance()));

    OPENFILENAME ofn = { 0 };

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrFile = *pszPath;
    ofn.nMaxFile = 256;
    bStatus = GetSaveFileName(&ofn);

    *pszPath = ofn.lpstrFile;

    return bStatus;
}

BOOL COMFileSaveDialog(LPWSTR* pszPath)
{
    HRESULT hr = S_OK;

    IFileDialog* pfd = NULL;
    IShellItem* psiResult = NULL;

    hr = CoInitialize(NULL);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"CoInitialize failed");
        goto fail;
    }

    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
        &IID_IFileDialog, (LPVOID)&pfd);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"CoCreateInstance FileSaveDialog failed");
        goto fail;
    }

    hr = pfd->lpVtbl->SetFileTypes(pfd, ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"FileSaveDialog::SetFileTypes failed");
        goto fail;
    }

    hr = pfd->lpVtbl->Show(pfd, NULL);
    assert(SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED));
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            OutputDebugString(L"FileSaveDialog::Show failed");
        }
        goto fail;
    }

    hr = pfd->lpVtbl->GetResult(pfd, &psiResult);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"FileSaveDialog::GetResult failed");
        goto fail;
    }

    hr = psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH,
        pszPath);
    assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        OutputDebugString(L"FileSaveDialog::GetResult failed");
        goto fail;
    }


fail:
    SAFE_RELEASE(psiResult)
        SAFE_RELEASE(pfd)

        return TRUE;
}

BOOL init_save_file_dialog(LPWSTR* pszPath)
{
    PNTSETTINGS* pSettings;
    BOOL bStatus;

    pSettings = PanitentApp_GetSettings(PanitentApp_Instance());

    if (pSettings->bLegacyFileDialogs == TRUE)
        bStatus = Win32LegacyFileSaveDialog(pszPath);
    else
        bStatus = COMFileSaveDialog(pszPath);

    return bStatus;
}

BOOL init_open_file_dialog(LPWSTR* pszPath)
{
    PNTSETTINGS* pSettings;
    BOOL bStatus;

    pSettings = PanitentApp_GetSettings(PanitentApp_Instance());

    if (pSettings->bLegacyFileDialogs == TRUE)
        bStatus = Win32LegacyFileOpenDialog(pszPath);
    else
        bStatus = COMFileOpenDialog(pszPath);

    return bStatus;
}
