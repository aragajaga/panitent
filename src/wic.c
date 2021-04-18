#include <wincodec.h>
#include <assert.h>
#include "wic.h"

#define DIB_WIDTHBYTES(bits) ((((bits) + 31)>>5)<<2)

#define SAFE_RELEASE(obj) \
if ((obj) != NULL) { \
  (obj)->lpVtbl->Release(obj); \
  (obj) = NULL; \
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

  MessageBox(NULL, L"Decoded lol", NULL, MB_OK);

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
