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

  hr = CoInitialize(NULL);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoInitialize failed");
    goto fail;
  }

  IWICImagingFactory* pFactory = NULL;
  hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
      &IID_IWICImagingFactory, (LPVOID)&pFactory);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoCreateInstance failed");
    goto fail;
  }

  IWICStream* pStream = NULL;
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

  IWICBitmapEncoder* pEncoder = NULL;
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

  IWICBitmapFrameEncode* pBitmapFrame = NULL;
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

  WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;
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
      uWidth, uHeight, ib.size, cbBufferSize);
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
  void* imageBits = NULL;
  HRESULT hr = S_OK;

  hr = CoInitialize(NULL);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoInitialize failed");
    goto fail;
  }

  IWICImagingFactory* pFactory = NULL;
  hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
      &IID_IWICImagingFactory, (LPVOID)&pFactory);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"CoCreateInstance WICImagingFactory failed");
    goto fail;
  }

  IWICBitmapDecoder* pDecoder = NULL;
  hr = pFactory->lpVtbl->CreateDecoderFromFilename(pFactory, szFilePath, NULL,
      GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICImagingFactory::CreateDecoderFromFilename failed");
    goto fail;
  }

  IWICBitmapFrameDecode* pFrame = NULL;
  hr = pDecoder->lpVtbl->GetFrame(pDecoder, 0, &pFrame);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapDecoder::GetFrame failed");
    goto fail;
  }

  UINT nWidth;
  UINT nHeight;
  hr = pFrame->lpVtbl->GetSize(pFrame, &nWidth, &nHeight);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapDecoder::GetSize");
    goto fail;
  }

  IWICFormatConverter* pConverter = NULL;
  hr = pFactory->lpVtbl->CreateFormatConverter(pFactory, &pConverter);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICBitmapDecoder::CreateFormatConverter failed");
    goto fail;
  }

  hr = pConverter->lpVtbl->Initialize(pConverter, (IWICBitmapSource*)pFrame,
      &GUID_WICPixelFormat32bppBGR,
      WICBitmapDitherTypeNone,
      NULL,
      0.f,
      WICBitmapPaletteTypeCustom);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICFormatConverter::Initialize failed");
    goto fail;
  }

  hr = pConverter->lpVtbl->GetSize(pConverter, &nWidth, &nHeight);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICFormatConverter::GetSize failed");
    goto fail;
  }

  UINT nStride = DIB_WIDTHBYTES(nWidth * 32);
  UINT nImage = nStride * nWidth;
  imageBits = malloc(nImage);

  hr = pConverter->lpVtbl->CopyPixels(pConverter, NULL, nStride, nImage,
      (LPVOID)imageBits);
  if (FAILED(hr))
  {
    assert(SUCCEEDED(hr));
    OutputDebugString(L"WICFormatConverter::CopyPixels failed");
    goto fail;
  }

  ImageBuffer ib = {0};
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
