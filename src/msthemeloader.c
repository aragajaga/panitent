#include "precomp.h"

#include "msthemeloader.h"
#include <windowsx.h>
#include <tchar.h>
#include "pntini.h"
#include "util/rbhashmap.h"

void MSTheme_Init(MSTheme* pMSTheme, PTSTR pszPath);

MSTheme* MSTheme_Create(PTSTR pszPath)
{
    MSTheme* pMSTheme = (MSTheme*)malloc(sizeof(MSTheme));

    if (pMSTheme)
    {
        memset(pMSTheme, 0, sizeof(MSTheme));
        MSTheme_Init(pMSTheme, pszPath);
    }

    return pMSTheme;
}

int PntINI_Callback(void* user, PCTSTR section, PCTSTR name, PCTSTR value, int lineno)
{
    MSTheme* pMSTheme = (MSTheme*)user;

    TCHAR szString[1024] = L"";
    _stprintf_s(szString, 1024, _T("%s = %s"), name, value);

    if (!_tcscmp(section, _T("Window.Caption")))
    {
        if (!_tcscmp(name, _T("ImageFile")))
        {
            TCHAR szValue[256];
            size_t i;
            for (i = 0; i < _tcslen(value); ++i)
            {
                if (!_istalnum(value[i]))
                {
                    szValue[i] = _T('_');
                    continue;
                }

                szValue[i] = _totupper(value[i]);
            }
            szValue[i] = _T('\0');


            HBITMAP hBitmap = LoadBitmap(pMSTheme->hModule, szValue);

            BITMAP bm;
            GetObject(hBitmap, sizeof(bm), &bm);

            HDC hdc = GetDC(HWND_DESKTOP);
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOldBitmap = SelectObject(hdcMem, hBitmap);

            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hOldBitmap);
            DeleteBitmap(hBitmap);
            DeleteDC(hdcMem);

            // MessageBox(NULL, szValue, section, NULL);
        }
        
    }   
}

static const WCHAR g_kIniExtension[] = L".ini";

BOOL HasIniExtension(PWSTR pszString)
{
    size_t len = wcslen(pszString);
    pszString += len - wcslen(g_kIniExtension);
    
    /* Case-insensitive comparison */
    if (!_wcsicmp(pszString, g_kIniExtension))
    {
        return TRUE;
    }

    return FALSE;
}

void MSTheme_Init(MSTheme* pMSTheme, PTSTR pszPath)
{
    HMODULE hModule = LoadLibraryEx(pszPath, 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule)
    {
        return;
    }

    pMSTheme->hModule = hModule;

    HRSRC hFileResNames = FindResource(hModule, MAKEINTRESOURCE(1), _T("FILERESNAMES"));
    if (hFileResNames)
    {
        HGLOBAL hFileResNamesResource = LoadResource(hModule, hFileResNames);
        if (hFileResNamesResource)
        {
            PWSTR data = (char*)LockResource(hFileResNamesResource);
            WCHAR buffer[256];

            PWSTR ptr = data;
            while (*ptr != L'\0')
            {
                wcscpy_s(buffer, 256, ptr);

                /* Find the length of current resource name */
                int len = wcslen(buffer);

                if (!wcscmp(buffer, L"NORMALBLUE_INI"))
                {

                    HRSRC hNormalBlue = FindResource(hModule, buffer, _T("TEXTFILE"));
                    if (hNormalBlue)
                    {
                        HGLOBAL hNormalBlueResource = LoadResource(hModule, hNormalBlue);
                        if (hNormalBlueResource)
                        {
                            // DWORD dwSize = SizeofResource(hModule, hNormalBlue);
                            void* data = LockResource(hNormalBlueResource);

                            // PntINI_ParseString(data, &PntINI_Callback, (void*)pMSTheme);

                            RBHashMapContext mapCtx = {
                                .pfnKeyComparator = _tcscmp,
                                .pfnKeyDeleter = free,
                                .pfnValueDeleter = free
                            };
                            RBHashMap* map = RBHashMap_Create(&mapCtx);

                            PntINIContextBased* pPntINIContext = PntINIContextBased_Create();
                            PntINIContextBased_LoadString(pPntINIContext, data);

                            while (!PntINIContextBased_ParseNext(pPntINIContext))
                            {
                                size_t len;

                                len = _tcslen(pPntINIContext->szValue);
                                PTSTR pszValue = (PTSTR)malloc((len + 1) * sizeof(TCHAR));
                                _tcscpy_s(pszValue, len + 1, pPntINIContext->szValue);

                                static const TCHAR szFormat[] = _T("%s//%s");
                                
                                /* Disable CRT warning */
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif  /* _MSC_VER */
                                len = _sntprintf(NULL, 0, szFormat, pPntINIContext->szSection, pPntINIContext->szName);
#ifdef _MSC_VER
#pragma warning( pop )
#endif  /* _MSC_VER */
                                PTSTR pszKey = (PTSTR)malloc((len + 1) * sizeof(TCHAR));
                                _stprintf_s(pszKey, len + 1, szFormat, pPntINIContext->szSection, pPntINIContext->szName);

                                // MessageBox(NULL, pszValue, pszKey, MB_OK);

                                RBHashMap_Insert(map, pszKey, pszValue);

                                
                            }

                            void* val = RBHashMap_Lookup(map, _T("Window.Caption//ImageFile"));

                            MessageBox(NULL, (PTSTR)val, NULL, MB_OK);
                        }
                    }

                    MessageBox(NULL, buffer, NULL, MB_OK);
                }

                ptr += len + 1;
            }
        }
    }

#if 0
    HRSRC hOrigFileNames = FindResource(hModule, MAKEINTRESOURCE(1), _T("ORIGFILENAMES"));
    if (hOrigFileNames)
    {
        HGLOBAL hOrigFileNamesResource = LoadResource(hModule, hOrigFileNames);
        if (hOrigFileNamesResource)
        {
            PWSTR data = (char*)LockResource(hOrigFileNamesResource);
            WCHAR buffer[256];

            PWSTR ptr = data;
            while (*ptr != L'\0')
            {
                wcscpy_s(buffer, 256, ptr);

                /* Find length of current filename */
                int len = wcslen(buffer);

                /* Workaround XP msstyles bug */
                if (HasIniExtension(buffer))
                {                    
                    size_t nBaseNameLen = len - (ARRAYSIZE(g_kIniExtension) - 1);
                    
                    /* Detect possibly repeated part */
                    int nRepeatPos = nBaseNameLen / 2 + 1;
                    if (!wcsncmp(&buffer[1], &buffer[nRepeatPos], nRepeatPos - 1))
                    {
                        buffer[nRepeatPos] = L'\0';
                        wcscat_s(buffer, 256, &buffer[nBaseNameLen]);
                    }
                }

                ptr += len + 1;
            }

        }
        
    }
#endif

    pMSTheme->hModule = hModule;
}

void MSTheme_Destroy(MSTheme* pMSTheme)
{
    if (pMSTheme->hModule)
    {
        FreeLibrary(pMSTheme->hModule);
        pMSTheme->hModule = NULL;
    }

    free(pMSTheme);
}

BOOL MSTheme_IsValid(MSTheme* pMSTheme)
{
    return pMSTheme->hModule ? TRUE : FALSE;
}

void MSTheme_DrawCloseBtn(MSTheme* pMSTheme, HDC hDC, int x, int y)
{
    if (!pMSTheme || !MSTheme_IsValid(pMSTheme))
    {
        return;
    }

    HBITMAP hBitmap = LoadBitmap(pMSTheme->hModule, _T("BLUE_CAPTIONBUTTON_BMP"));

    BITMAP bm = { 0 };
    GetObject(hBitmap, sizeof(bm), &bm);

    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hOldBitmap = SelectBitmap(hMemDC, hBitmap);

    BitBlt(hDC, x, y, bm.bmWidth, bm.bmWidth, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
}

/*
HBITMAP CopyToDIBitmap(HBITMAP hBmSource)
{
    BITMAP bm = { 0 };
    GetObject(hBmSource, sizeof(bm), &bm);

    HDC hScreenDC = GetDC(HWND_DESKTOP);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(bmi);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pData;
    HBITMAP hBmTarget = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, &pData, NULL, 0);

    HDC hSourceDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hOldBitmap1 = SelectBitmap(hSourceDC, hBmSource);

    HDC hResultDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hOldBitmap2 = SelectBitmap(hResultDC, hBmTarget);

    BitBlt(hResultDC, 0, 0, bm.bmWidth, bm.bmHeight, hSourceDC, 0, 0, SRCCOPY);

    SelectBitmap(hSourceDC, hOldBitmap1);
    DeleteDC(hSourceDC);
    SelectBitmap(hResultDC, hOldBitmap2);
    DeleteDC(hResultDC);

    ReleaseDC(HWND_DESKTOP, hScreenDC);

    return hBmTarget;
}
*/

void MSTheme_DrawAnything(MSTheme* pMSTheme, HDC hDC, PTSTR pszResourceName, int x, int y)
{
    if (!pMSTheme || !MSTheme_IsValid(pMSTheme))
    {
        return;
    }

    HBITMAP hBitmap = LoadImage(pMSTheme->hModule, pszResourceName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE);

    BITMAP bm = { 0 };
    GetObject(hBitmap, sizeof(bm), &bm);

    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hOldBitmap = SelectBitmap(hMemDC, hBitmap);

    /* Premultiply image if it is 32bpp */
    if (bm.bmBitsPixel == 32)
    {
        for (size_t y = 0; y < bm.bmHeight; ++y)
        {
            for (size_t x = 0; x < bm.bmWidth; ++x)
            {
                COLORREF* color = &((uint8_t*)bm.bmBits)[y * bm.bmWidthBytes + x * 4];

                uint8_t alpha = (*color) >> 24;
                uint8_t red = GetRValue(*color) * alpha / 255;
                uint8_t green = GetGValue(*color) * alpha / 255;
                uint8_t blue = GetBValue(*color) * alpha / 255;

                *color = red | green << 8 | blue << 16 | alpha << 24;
            }
        }

        BLENDFUNCTION blendFunc = {
        .BlendOp = AC_SRC_OVER,
        .BlendFlags = 0,
        .SourceConstantAlpha = 0xFF,
        .AlphaFormat = AC_SRC_ALPHA
        };

        AlphaBlend(hDC, x, y, bm.bmWidth, bm.bmWidth, hMemDC, 0, 0, bm.bmWidth, bm.bmWidth, blendFunc);
    }
    else
    {
        TransparentBlt(hDC, x, y, bm.bmWidth, bm.bmWidth, hMemDC, 0, 0, bm.bmWidth, bm.bmWidth, RGB(255, 0, 255));
    }

    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
}

void MSTheme_DrawCloseGlyph(MSTheme* pMSTheme, HDC hDC, int x, int y)
{
    if (!pMSTheme || !MSTheme_IsValid(pMSTheme))
    {
        return;
    }

    HBITMAP hBitmap = LoadImage(pMSTheme->hModule, _T("BLUE_CLOSEGLYPH_BMP"), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE);

    BITMAP bm = { 0 };
    GetObject(hBitmap, sizeof(bm), &bm);

    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hOldBitmap = SelectBitmap(hMemDC, hBitmap);

    /* Premultiply image if it is 32bpp */
    if (bm.bmBitsPixel == 32)
    {
        for (size_t y = 0; y < bm.bmHeight; ++y)
        {
            for (size_t x = 0; x < bm.bmWidth; ++x)
            {
                COLORREF* color = &((uint8_t*)bm.bmBits)[y * bm.bmWidthBytes + x * 4];

                uint8_t alpha = (*color) >> 24;
                uint8_t red = GetRValue(*color) * alpha / 255;
                uint8_t green = GetGValue(*color) * alpha / 255;
                uint8_t blue = GetBValue(*color) * alpha / 255;

                *color = red | green << 8 | blue << 16 | alpha << 24;
            }
        }

        BLENDFUNCTION blendFunc = {
        .BlendOp = AC_SRC_OVER,
        .BlendFlags = 0,
        .SourceConstantAlpha = 0xFF,
        .AlphaFormat = AC_SRC_ALPHA
        };

        AlphaBlend(hDC, x, y, bm.bmWidth, bm.bmWidth, hMemDC, 0, 0, bm.bmWidth, bm.bmWidth, blendFunc);
    }
    else if (bm.bmBitsPixel == 24)
    {
        TransparentBlt(hDC, x, y, bm.bmWidth, bm.bmWidth, hMemDC, 0, 0, bm.bmWidth, bm.bmWidth, RGB(255, 0, 255));
    }

    // BitBlt(hDC, x, y, bm.bmWidth, bm.bmWidth, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
}
