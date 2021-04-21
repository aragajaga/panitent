#include "precomp.h"

#include <shobjidl.h>

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

#include "file_open.h"

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

void FileOpenPng(LPWSTR pszPath)
{
  /*
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
  */
}

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

crefptr_t init_open_file_dialog()
{
  HRESULT hr = S_OK;
  crefptr_t s = NULL;

  hr = CoInitialize(NULL);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoInitialize failed");
    goto fail;
  }

  IFileDialog* pfd = NULL;
  hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
      &IID_IFileDialog, (LPVOID)&pfd);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoCreateInstance FileOpenDialog failed");
    goto fail;
  }

  hr = pfd->lpVtbl->SetFileTypes(pfd, ARRAYSIZE(c_rgReadTypes), c_rgReadTypes);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"FileOpenDialog::SetFileType failed");
    goto fail;
  }

  hr = pfd->lpVtbl->Show(pfd, NULL);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"FileOpenDialog::Show failed");
    goto fail;
  }

  IShellItem* psiResult = NULL;
  hr = pfd->lpVtbl->GetResult(pfd, &psiResult);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"FileOpenDialog::GetResult failed");
    goto fail;
  }
  
  LPWSTR pszFilePath = NULL;
  hr = psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH,
      &pszFilePath);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"GetResult failed");
    goto fail;
  }

  s = crefptr_new((LPVOID)pszFilePath, CoStringDtor);

fail:
  SAFE_RELEASE(psiResult)
  SAFE_RELEASE(pfd)

  return s;
}

crefptr_t init_save_file_dialog()
{
  HRESULT hr = S_OK;
  crefptr_t s = NULL;

  hr = CoInitialize(NULL);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoInitialize failed");
    goto fail;
  }

  IFileDialog* pfd = NULL;
  hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
      &IID_IFileDialog, (LPVOID)&pfd);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoCreateInstance FileSaveDialog failed");
    goto fail;
  }

  hr = pfd->lpVtbl->SetFileTypes(pfd, ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"FileSaveDialog::SetFileTypes failed");
    goto fail;
  }

  hr = pfd->lpVtbl->Show(pfd, NULL);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"FileSaveDialog::Show failed");
    goto fail;
  }

  IShellItem* psiResult = NULL;
  hr = pfd->lpVtbl->GetResult(pfd, &psiResult);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"FileSaveDialog::GetResult failed");
    goto fail;
  }

  LPWSTR pszFilePath = NULL;
  hr = psiResult->lpVtbl->GetDisplayName(psiResult, SIGDN_FILESYSPATH,
      &pszFilePath);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"FileSaveDialog::GetResult failed");
    goto fail;
  }

  s = crefptr_new((LPVOID)pszFilePath, CoStringDtor);

fail:
  SAFE_RELEASE(psiResult)
  SAFE_RELEASE(pfd)

  return s;
}
