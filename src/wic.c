#include <wincodec.h>
#include <assert.h>
#include "wic.h"
#include <strsafe.h>

#define DIB_WIDTHBYTES(bits) ((((bits) + 31)>>5)<<2)

#define SAFE_RELEASE(obj) \
if ((obj) != NULL) { \
  (obj)->lpVtbl->Release(obj); \
  (obj) = NULL; \
}

void ImageFileWriter(LPWSTR szFilePath, ImageBuffer ib)
{
  HRESULT hr = S_OK;

  UINT uWidth = ib.width;
  UINT uHeight = ib.height;

  IWICImagingFactory* pFactory = NULL;
  IWICStream* pStream = NULL;
  IWICBitmapEncoder* pEncoder = NULL;
  IWICBitmapFrameEncode* pBitmapFrame = NULL;
  WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;

  hr = CoInitialize(NULL);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoInitialize failed");
    goto fail;
  }

  hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
      &IID_IWICImagingFactory, (LPVOID)&pFactory);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoCreateInstance failed");
    goto fail;
  }

  hr = pFactory->lpVtbl->CreateStream(pFactory, &pStream);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICImagingFactory::CreateStream failed");
    goto fail;
  }

  hr = pStream->lpVtbl->InitializeFromFilename(pStream, szFilePath,
      GENERIC_WRITE);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICImagingFactory::InitializeFromFilename failed");
    goto fail;
  }

  hr = pFactory->lpVtbl->CreateEncoder(pFactory, &GUID_ContainerFormatPng,
      NULL, &pEncoder);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICImagingFactory::CreateEncoder failed");
    goto fail;
  }

  hr = pEncoder->lpVtbl->Initialize(pEncoder, (IStream*)pStream, WICBitmapEncoderNoCache);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapEncoder::Initialize failed");
    goto fail;
  }

  /* IPropertyBag2* pPropertyBag = NULL; */
  hr = pEncoder->lpVtbl->CreateNewFrame(pEncoder, &pBitmapFrame, NULL /* &pPropertyBag */);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapEncoder::CreateNewFrame failed");
    goto fail;
  }

  /*
  PROPBAG2 option;
  VARIANT varValue;

  option.pstrName = L"InterlaceOption";
  VariantInit(&varValue);
  varValue.vt = VT_BOOL;
  varValue.boolVal = FALSE;

  hr = pPropertyBag->lpVtbl->Write(pPropertyBag, 1, &option, &varValue);
  if (FAILED(hr))
  {
    DWORD dwCode= GetLastError();
    WCHAR szError[80];
    StringCchPrintf(szError, 80, L"Error: %d, HRESULT: %d", dwCode, hr);

    MessageBox(NULL, szError, NULL, MB_OK);
    assert(SUCCEEDED(hr));
    OutputDebugString(L"PropertyBag2::Write failed");
    goto fail;
  }

  option.pstrName = L"FilterOption";
  VariantInit(&varValue);
  varValue.vt = VT_UI1;
  varValue.bVal = WICPngFilterUnspecified;

  hr = pPropertyBag->lpVtbl->Write(pPropertyBag, 1, &option, &varValue);
  if (FAILED(hr))
  {
    DWORD dwCode= GetLastError();
    WCHAR szError[80];
    StringCchPrintf(szError, 80, L"Error: %d, HRESULT: %d", dwCode, hr);

    MessageBox(NULL, szError, NULL, MB_OK);
    assert(SUCCEEDED(hr));
    OutputDebugString(L"PropertyBag2::Write failed");
    goto fail;
  }
  */

  hr = pBitmapFrame->lpVtbl->Initialize(pBitmapFrame, NULL /* pPropertyBag */);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapFrame::Initialize failed");
    goto fail;
  }

  hr = pBitmapFrame->lpVtbl->SetSize(pBitmapFrame, uWidth, uHeight);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapFrame::SetSize failed");
    goto fail;
  }

  hr = pBitmapFrame->lpVtbl->SetPixelFormat(pBitmapFrame, &formatGUID);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapFrame::SetPixelFormat failed");
    goto fail;
  }

  hr = IsEqualGUID(&formatGUID, &GUID_WICPixelFormat32bppBGRA) ? S_OK : E_FAIL; 
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"IsEqualGUID failed");
    goto fail;
  }

  UINT cbStride = (uWidth * 32 + 7) / 8;
  UINT cbBufferSize = uHeight * cbStride;

  hr = pBitmapFrame->lpVtbl->WritePixels(pBitmapFrame, uHeight, cbStride,
      cbBufferSize, ib.bits);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapFrame::WritePixels failed");
    goto fail;
  }

  WCHAR szImageInfo[80];
  StringCchPrintf(szImageInfo, 80, L"W: %d\nH: %d\nSize: %d\nBufsize: %d",
      uWidth, uHeight, (int)ib.size, cbBufferSize);
  MessageBox(NULL, szImageInfo, L"Info", MB_OK);

  hr = pBitmapFrame->lpVtbl->Commit(pBitmapFrame);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapFrame::Commit failed");
    goto fail;
  }

  hr = pEncoder->lpVtbl->Commit(pEncoder);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapEncoder::Commit failed");
    goto fail;
  }

fail:
  SAFE_RELEASE(pFactory)
  SAFE_RELEASE(pEncoder)
  SAFE_RELEASE(pBitmapFrame)
  /* SAFE_RELEASE(pPropertyBag) */
  SAFE_RELEASE(pStream)
}

ImageBuffer ImageFileReader(LPWSTR szFilePath)
{
  HRESULT hr = S_OK;
  IWICImagingFactory* pFactory = NULL;
  IWICBitmapDecoder* pDecoder = NULL;
  IWICBitmapFrameDecode* pFrame = NULL;
  IWICFormatConverter* pConverter = NULL;
  ImageBuffer ib = {0};
  void* imageBits = NULL;
  UINT nWidth;
  UINT nHeight;
  UINT nStride;
  UINT nImage;

  hr = CoInitialize(NULL);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"CoInitialize failed");
    goto fail;
  }

  hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
      &IID_IWICImagingFactory, (LPVOID)&pFactory);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"CoCreateInstance WICImagingFactory failed");
    goto fail;
  }

  hr = pFactory->lpVtbl->CreateDecoderFromFilename(pFactory, szFilePath, NULL,
      GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"WICImagingFactory::CreateDecoderFromFilename failed");
    goto fail;
  }

  hr = pDecoder->lpVtbl->GetFrame(pDecoder, 0, &pFrame);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"WICBitmapDecoder::GetFrame failed");
    goto fail;
  }

  hr = pFrame->lpVtbl->GetSize(pFrame, &nWidth, &nHeight);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"WICBitmapDecoder::GetSize");
    goto fail;
  }

  hr = pFactory->lpVtbl->CreateFormatConverter(pFactory, &pConverter);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"WICBitmapDecoder::CreateFormatConverter failed");
    goto fail;
  }

  hr = pConverter->lpVtbl->Initialize(pConverter, (IWICBitmapSource*)pFrame,
      &GUID_WICPixelFormat32bppBGR, WICBitmapDitherTypeNone, NULL, 0.f,
      WICBitmapPaletteTypeCustom);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"WICFormatConverter::Initialize failed");
    goto fail;
  }

  hr = pConverter->lpVtbl->GetSize(pConverter, &nWidth, &nHeight);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"WICFormatConverter::GetSize failed");
    goto fail;
  }

  nStride = DIB_WIDTHBYTES(nWidth * 32);
  nImage = nStride * nHeight;
  imageBits = malloc(nImage);

  hr = pConverter->lpVtbl->CopyPixels(pConverter, NULL, nStride, nImage,
      (LPVOID)imageBits);

  assert(SUCCEEDED(hr));
  if (FAILED(hr))
  {
    OutputDebugString(L"WICFormatConverter::CopyPixels failed");
    goto fail;
  }

  ib.width  = nWidth;
  ib.height = nHeight;
  ib.stride = nStride;
  ib.size   = nImage;
  ib.bits   = imageBits;

fail:
  SAFE_RELEASE(pFrame);
  SAFE_RELEASE(pDecoder);
  SAFE_RELEASE(pFactory);

  return ib;
}
