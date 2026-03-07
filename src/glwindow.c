#include "precomp.h"

#include "win32/window.h"
#include "glwindow.h"

#include "canvas.h"
#include "document.h"
#include "panitentapp.h"

#include <gl/GL.h>

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

static const WCHAR szClassName[] = L"GLWindowClass";
static const UINT_PTR kGLWindowRenderTimerId = 1;
static const UINT kGLWindowRenderIntervalMs = 16;
static const ULONGLONG kGLWindowAutoResumeDelayMs = 3000;
static const float kGLWindowMouseRotateSensitivity = 0.45f;
static const float kGLWindowZoomStep = 1.12f;
static const float kGLWindowZoomMin = 0.35f;
static const float kGLWindowZoomMax = 4.50f;

typedef struct GLVec2 GLVec2;
typedef struct GLVec3 GLVec3;
typedef struct GLVertex GLVertex;
typedef struct GLMesh GLMesh;
typedef struct GLSourceBuffers GLSourceBuffers;
typedef struct GLFaceVertex GLFaceVertex;

struct GLVec2
{
    float u;
    float v;
};

struct GLVec3
{
    float x;
    float y;
    float z;
};

struct GLVertex
{
    GLVec3 position;
    GLVec3 normal;
    GLVec2 texcoord;
};

struct GLMesh
{
    GLVertex* pVertices;
    size_t nVertexCount;
    size_t nVertexCapacity;
    GLVec3 center;
    float fScale;
};

struct GLSourceBuffers
{
    GLVec3* pPositions;
    size_t nPositionCount;
    size_t nPositionCapacity;
    GLVec2* pTexcoords;
    size_t nTexcoordCount;
    size_t nTexcoordCapacity;
    GLVec3* pNormals;
    size_t nNormalCount;
    size_t nNormalCapacity;
};

struct GLFaceVertex
{
    int iPosition;
    int iTexcoord;
    int iNormal;
};

GLWindow* GLWindow_Create();
static void GLWindow_Init(GLWindow* pGLWindow);

static void GLWindow_PreRegister(LPWNDCLASSEX lpwcex);
static void GLWindow_PreCreate(LPCREATESTRUCT lpcs);
static BOOL GLWindow_OnCreate(GLWindow* pGLWindow, LPCREATESTRUCT lpcs);
static void GLWindow_OnDestroy(GLWindow* pGLWindow);
static void GLWindow_OnPaint(GLWindow* pGLWindow);
static void GLWindow_OnSize(GLWindow* pGLWindow, UINT state, int cx, int cy);
static void GLWindow_OnLButtonDown(GLWindow* pGLWindow, int x, int y);
static void GLWindow_OnLButtonUp(GLWindow* pGLWindow, int x, int y);
static void GLWindow_OnMouseMove(GLWindow* pGLWindow, HWND hWnd, int x, int y);
static void GLWindow_OnMouseWheel(GLWindow* pGLWindow, HWND hWnd, short zDelta);
static void GLWindow_OnRButtonUp(GLWindow* pGLWindow, int x, int y);
static void GLWindow_OnContextMenu(GLWindow* pGLWindow, int x, int y);
static LRESULT CALLBACK GLWindow_UserProc(GLWindow* pGLWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static BOOL GLWindow_LoadMesh(GLWindow* pGLWindow);
static BOOL GLWindow_FindMeshPath(WCHAR* pszPath, size_t cchPath);
static BOOL GLWindow_LoadOBJMesh(const WCHAR* pszPath, GLMesh** ppMesh);
static void GLWindow_FreeMesh(GLMesh* pMesh);
static void GLWindow_FinalizeMesh(GLMesh* pMesh);

static BOOL GLWindow_SyncActiveDocumentTexture(GLWindow* pGLWindow);
static void GLWindow_DeleteTexture(GLWindow* pGLWindow);

static void GLWindow_Render(GLWindow* pGLWindow);
static void GLWindow_RenderFallback(void);
static void GLWindow_SetPerspective(float fovyDegrees, float aspect, float zNear, float zFar);

static BOOL GLMesh_EnsureVertexCapacity(GLMesh* pMesh, size_t nRequiredCapacity);
static BOOL GLMesh_AppendVertex(GLMesh* pMesh, const GLVertex* pVertex);

static BOOL GLSource_EnsurePositionCapacity(GLSourceBuffers* pSource, size_t nRequiredCapacity);
static BOOL GLSource_EnsureTexcoordCapacity(GLSourceBuffers* pSource, size_t nRequiredCapacity);
static BOOL GLSource_EnsureNormalCapacity(GLSourceBuffers* pSource, size_t nRequiredCapacity);
static BOOL GLSource_AppendPosition(GLSourceBuffers* pSource, GLVec3 position);
static BOOL GLSource_AppendTexcoord(GLSourceBuffers* pSource, GLVec2 texcoord);
static BOOL GLSource_AppendNormal(GLSourceBuffers* pSource, GLVec3 normal);
static void GLSource_Free(GLSourceBuffers* pSource);

static BOOL GLWindow_TryCandidatePath(WCHAR* pszCandidate, size_t cchCandidate, PCWSTR pszDirectory);
static BOOL GLWindow_ParseFaceVertexToken(char* pszToken, GLFaceVertex* pFaceVertex);
static BOOL GLWindow_IsSpaceChar(char ch);

static GLVec3 GLVec3_Sub(GLVec3 a, GLVec3 b);
static GLVec3 GLVec3_Add(GLVec3 a, GLVec3 b);
static GLVec3 GLVec3_Scale(GLVec3 v, float s);
static GLVec3 GLVec3_Cross(GLVec3 a, GLVec3 b);
static float GLVec3_Length(GLVec3 v);
static GLVec3 GLVec3_Normalize(GLVec3 v);
static GLVec3 GLVec3_Reflect(GLVec3 v, GLVec3 normal);
static GLVec3 GLVec3_RotateY(GLVec3 v, float degrees);
static GLVec3 GLVec3_RotateX(GLVec3 v, float degrees);
static GLVec2 GLWindow_ComputeMatcapTexcoord(const GLWindow* pGLWindow, const GLMesh* pMesh, GLVec3 position, GLVec3 normal);

GLWindow* GLWindow_Create()
{
    GLWindow* pGLWindow = (GLWindow*)malloc(sizeof(GLWindow));
    if (pGLWindow)
    {
        memset(pGLWindow, 0, sizeof(GLWindow));
        GLWindow_Init(pGLWindow);
    }

    return pGLWindow;
}

static void GLWindow_Init(GLWindow* pGLWindow)
{
    Window_Init(&pGLWindow->base);

    pGLWindow->base.szClassName = szClassName;
    pGLWindow->base.OnCreate = (FnWindowOnCreate)GLWindow_OnCreate;
    pGLWindow->base.OnDestroy = (FnWindowOnDestroy)GLWindow_OnDestroy;
    pGLWindow->base.OnPaint = (FnWindowOnPaint)GLWindow_OnPaint;
    pGLWindow->base.OnSize = (FnWindowOnSize)GLWindow_OnSize;
    pGLWindow->base.PreRegister = (FnWindowPreRegister)GLWindow_PreRegister;
    pGLWindow->base.PreCreate = (FnWindowPreCreate)GLWindow_PreCreate;
    pGLWindow->base.UserProc = (FnWindowUserProc)GLWindow_UserProc;
}

static void GLWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
    lpwcex->style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
    lpwcex->hbrBackground = NULL;
    lpwcex->lpszClassName = szClassName;
}

static void GLWindow_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = 0;
    lpcs->lpszClass = szClassName;
    lpcs->lpszName = L"GLWindow";
    lpcs->style = WS_VISIBLE | WS_CAPTION | WS_THICKFRAME | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    lpcs->x = CW_USEDEFAULT;
    lpcs->y = CW_USEDEFAULT;
    lpcs->cx = 320;
    lpcs->cy = 240;
}

static BOOL GLWindow_OnCreate(GLWindow* pGLWindow, LPCREATESTRUCT lpcs)
{
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    UNREFERENCED_PARAMETER(lpcs);

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    pGLWindow->hdcGL = GetDC(pGLWindow->base.hWnd);
    if (!pGLWindow->hdcGL)
    {
        return FALSE;
    }

    {
        int nPixelFormat = ChoosePixelFormat(pGLWindow->hdcGL, &pfd);
        if (nPixelFormat == 0 || !SetPixelFormat(pGLWindow->hdcGL, nPixelFormat, &pfd))
        {
            ReleaseDC(pGLWindow->base.hWnd, pGLWindow->hdcGL);
            pGLWindow->hdcGL = NULL;
            return FALSE;
        }
    }

    pGLWindow->hglrc = wglCreateContext(pGLWindow->hdcGL);
    if (!pGLWindow->hglrc || !wglMakeCurrent(pGLWindow->hdcGL, pGLWindow->hglrc))
    {
        if (pGLWindow->hglrc)
        {
            wglDeleteContext(pGLWindow->hglrc);
            pGLWindow->hglrc = NULL;
        }
        ReleaseDC(pGLWindow->base.hWnd, pGLWindow->hdcGL);
        pGLWindow->hdcGL = NULL;
        return FALSE;
    }

    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (!GLWindow_LoadMesh(pGLWindow))
    {
        OutputDebugStringW(L"GLWindow: failed to load teapot.obj\n");
    }

    {
        RECT rcClient = { 0 };
        GetClientRect(pGLWindow->base.hWnd, &rcClient);
        pGLWindow->cxClient = max(1, rcClient.right - rcClient.left);
        pGLWindow->cyClient = max(1, rcClient.bottom - rcClient.top);
    }

    pGLWindow->uLastAutoTickMs = GetTickCount64();
    pGLWindow->uLastInteractionMs = 0;
    pGLWindow->fUserYaw = 0.0f;
    pGLWindow->fUserPitch = 18.0f;
    pGLWindow->fAutoYaw = 0.0f;
    pGLWindow->fZoom = 1.0f;
    pGLWindow->nRenderTimerId = SetTimer(pGLWindow->base.hWnd, kGLWindowRenderTimerId, kGLWindowRenderIntervalMs, NULL);

    return TRUE;
}

static void GLWindow_OnDestroy(GLWindow* pGLWindow)
{
    if (!pGLWindow)
    {
        return;
    }

    if (pGLWindow->nRenderTimerId)
    {
        KillTimer(pGLWindow->base.hWnd, pGLWindow->nRenderTimerId);
        pGLWindow->nRenderTimerId = 0;
    }

    GLWindow_DeleteTexture(pGLWindow);
    GLWindow_FreeMesh((GLMesh*)pGLWindow->pMesh);
    pGLWindow->pMesh = NULL;

    if (pGLWindow->hglrc)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(pGLWindow->hglrc);
        pGLWindow->hglrc = NULL;
    }

    if (pGLWindow->hdcGL)
    {
        ReleaseDC(pGLWindow->base.hWnd, pGLWindow->hdcGL);
        pGLWindow->hdcGL = NULL;
    }
}

static void GLWindow_OnPaint(GLWindow* pGLWindow)
{
    PAINTSTRUCT ps = { 0 };
    BeginPaint(pGLWindow->base.hWnd, &ps);
    GLWindow_Render(pGLWindow);
    EndPaint(pGLWindow->base.hWnd, &ps);
}

static void GLWindow_OnSize(GLWindow* pGLWindow, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(state);

    pGLWindow->cxClient = max(1, cx);
    pGLWindow->cyClient = max(1, cy);
    InvalidateRect(pGLWindow->base.hWnd, NULL, FALSE);
}

static void GLWindow_OnLButtonDown(GLWindow* pGLWindow, int x, int y)
{
    if (!pGLWindow)
    {
        return;
    }

    pGLWindow->bMouseRotating = TRUE;
    pGLWindow->ptLastMouse.x = x;
    pGLWindow->ptLastMouse.y = y;
    pGLWindow->uLastInteractionMs = GetTickCount64();
    pGLWindow->uLastAutoTickMs = pGLWindow->uLastInteractionMs;
    SetCapture(pGLWindow->base.hWnd);
}

static void GLWindow_OnLButtonUp(GLWindow* pGLWindow, int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (!pGLWindow)
    {
        return;
    }

    if (pGLWindow->bMouseRotating)
    {
        pGLWindow->bMouseRotating = FALSE;
        pGLWindow->uLastInteractionMs = GetTickCount64();
        pGLWindow->uLastAutoTickMs = pGLWindow->uLastInteractionMs;
        if (GetCapture() == pGLWindow->base.hWnd)
        {
            ReleaseCapture();
        }
    }
}

static void GLWindow_OnMouseMove(GLWindow* pGLWindow, HWND hWnd, int x, int y)
{
    if (!pGLWindow || !pGLWindow->bMouseRotating)
    {
        return;
    }

    {
        int dx = x - pGLWindow->ptLastMouse.x;
        int dy = y - pGLWindow->ptLastMouse.y;

        pGLWindow->ptLastMouse.x = x;
        pGLWindow->ptLastMouse.y = y;
        pGLWindow->fUserYaw += (float)dx * kGLWindowMouseRotateSensitivity;
        pGLWindow->fUserPitch += (float)dy * kGLWindowMouseRotateSensitivity;
        if (pGLWindow->fUserPitch > 89.0f)
        {
            pGLWindow->fUserPitch = 89.0f;
        }
        if (pGLWindow->fUserPitch < -89.0f)
        {
            pGLWindow->fUserPitch = -89.0f;
        }

        pGLWindow->uLastInteractionMs = GetTickCount64();
        pGLWindow->uLastAutoTickMs = pGLWindow->uLastInteractionMs;
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

static void GLWindow_OnMouseWheel(GLWindow* pGLWindow, HWND hWnd, short zDelta)
{
    if (!pGLWindow || zDelta == 0)
    {
        return;
    }

    if (zDelta > 0)
    {
        pGLWindow->fZoom *= kGLWindowZoomStep;
    }
    else {
        pGLWindow->fZoom /= kGLWindowZoomStep;
    }

    if (pGLWindow->fZoom < kGLWindowZoomMin)
    {
        pGLWindow->fZoom = kGLWindowZoomMin;
    }
    if (pGLWindow->fZoom > kGLWindowZoomMax)
    {
        pGLWindow->fZoom = kGLWindowZoomMax;
    }

    InvalidateRect(hWnd, NULL, FALSE);
}

static void GLWindow_OnRButtonUp(GLWindow* pGLWindow, int x, int y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);

    if (!pGLWindow)
    {
        return;
    }

    pGLWindow->bMatcapMode = pGLWindow->bMatcapMode ? FALSE : TRUE;
    InvalidateRect(pGLWindow->base.hWnd, NULL, FALSE);
}

static void GLWindow_OnContextMenu(GLWindow* pGLWindow, int x, int y)
{
    UNREFERENCED_PARAMETER(pGLWindow);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

static LRESULT CALLBACK GLWindow_UserProc(GLWindow* pGLWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
        if (wParam == kGLWindowRenderTimerId)
        {
            if (IsWindowVisible(hWnd))
            {
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_LBUTTONDOWN:
        GLWindow_OnLButtonDown(pGLWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

    case WM_RBUTTONUP:
        GLWindow_OnRButtonUp(pGLWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

    case WM_MOUSEMOVE:
        GLWindow_OnMouseMove(pGLWindow, hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEWHEEL:
        GLWindow_OnMouseWheel(pGLWindow, hWnd, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;

    case WM_LBUTTONUP:
        GLWindow_OnLButtonUp(pGLWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

    case WM_CAPTURECHANGED:
        if (pGLWindow)
        {
            pGLWindow->bMouseRotating = FALSE;
            pGLWindow->uLastInteractionMs = GetTickCount64();
            pGLWindow->uLastAutoTickMs = pGLWindow->uLastInteractionMs;
        }
        return 0;

    case WM_CONTEXTMENU:
        GLWindow_OnContextMenu(pGLWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    }

    return Window_UserProcDefault((Window*)pGLWindow, hWnd, message, wParam, lParam);
}

static BOOL GLWindow_LoadMesh(GLWindow* pGLWindow)
{
    WCHAR szPath[MAX_PATH] = L"";
    GLMesh* pMesh = NULL;

    if (!GLWindow_FindMeshPath(szPath, ARRAYSIZE(szPath)))
    {
        return FALSE;
    }

    if (!GLWindow_LoadOBJMesh(szPath, &pMesh))
    {
        return FALSE;
    }

    pGLWindow->pMesh = pMesh;
    return TRUE;
}

static BOOL GLWindow_TryCandidatePath(WCHAR* pszCandidate, size_t cchCandidate, PCWSTR pszDirectory)
{
    if (!pszCandidate || !pszDirectory)
    {
        return FALSE;
    }

    if (FAILED(StringCchCopyW(pszCandidate, cchCandidate, pszDirectory)))
    {
        return FALSE;
    }

    if (!PathAppendW(pszCandidate, L"teapot.obj"))
    {
        return FALSE;
    }

    return PathFileExistsW(pszCandidate);
}

static BOOL GLWindow_FindMeshPath(WCHAR* pszPath, size_t cchPath)
{
    WCHAR szCandidate[MAX_PATH] = L"";
    WCHAR szDirectory[MAX_PATH] = L"";

    if (!pszPath || cchPath == 0)
    {
        return FALSE;
    }

    pszPath[0] = L'\0';

    if (GetCurrentDirectoryW(ARRAYSIZE(szDirectory), szDirectory) > 0 &&
        GLWindow_TryCandidatePath(szCandidate, ARRAYSIZE(szCandidate), szDirectory))
    {
        return SUCCEEDED(StringCchCopyW(pszPath, cchPath, szCandidate));
    }

    if (GetModuleFileNameW(NULL, szDirectory, ARRAYSIZE(szDirectory)) == 0)
    {
        return FALSE;
    }

    if (!PathRemoveFileSpecW(szDirectory))
    {
        return FALSE;
    }

    for (int i = 0; i < 4; ++i)
    {
        if (GLWindow_TryCandidatePath(szCandidate, ARRAYSIZE(szCandidate), szDirectory))
        {
            return SUCCEEDED(StringCchCopyW(pszPath, cchPath, szCandidate));
        }

        if (!PathRemoveFileSpecW(szDirectory))
        {
            break;
        }
    }

    return FALSE;
}

static BOOL GLWindow_LoadOBJMesh(const WCHAR* pszPath, GLMesh** ppMesh)
{
    FILE* pFile = NULL;
    GLMesh* pMesh = NULL;
    GLSourceBuffers source = { 0 };
    char szLine[512];

    if (!pszPath || !ppMesh)
    {
        return FALSE;
    }

    *ppMesh = NULL;
    if (_wfopen_s(&pFile, pszPath, L"rt") != 0 || !pFile)
    {
        return FALSE;
    }

    pMesh = (GLMesh*)calloc(1, sizeof(GLMesh));
    if (!pMesh)
    {
        fclose(pFile);
        return FALSE;
    }

    while (fgets(szLine, sizeof(szLine), pFile))
    {
        if (szLine[0] == 'v' && szLine[1] == ' ')
        {
            GLVec3 position = { 0.0f, 0.0f, 0.0f };
            if (sscanf_s(szLine + 2, "%f %f %f", &position.x, &position.y, &position.z) == 3)
            {
                if (!GLSource_AppendPosition(&source, position))
                {
                    fclose(pFile);
                    GLSource_Free(&source);
                    GLWindow_FreeMesh(pMesh);
                    return FALSE;
                }
            }
        }
        else if (szLine[0] == 'v' && szLine[1] == 't' && szLine[2] == ' ')
        {
            GLVec2 texcoord = { 0.0f, 0.0f };
            if (sscanf_s(szLine + 3, "%f %f", &texcoord.u, &texcoord.v) >= 2)
            {
                if (!GLSource_AppendTexcoord(&source, texcoord))
                {
                    fclose(pFile);
                    GLSource_Free(&source);
                    GLWindow_FreeMesh(pMesh);
                    return FALSE;
                }
            }
        }
        else if (szLine[0] == 'v' && szLine[1] == 'n' && szLine[2] == ' ')
        {
            GLVec3 normal = { 0.0f, 1.0f, 0.0f };
            if (sscanf_s(szLine + 3, "%f %f %f", &normal.x, &normal.y, &normal.z) == 3)
            {
                if (!GLSource_AppendNormal(&source, GLVec3_Normalize(normal)))
                {
                    fclose(pFile);
                    GLSource_Free(&source);
                    GLWindow_FreeMesh(pMesh);
                    return FALSE;
                }
            }
        }
        else if (szLine[0] == 'f' && szLine[1] == ' ')
        {
            GLFaceVertex faceVertices[32] = { 0 };
            int nFaceVertexCount = 0;
            char* pCursor = szLine + 2;

            while (*pCursor)
            {
                GLFaceVertex faceVertex = { 0 };
                char* pTokenStart = NULL;
                char* pTokenEnd = NULL;
                char chRestore = '\0';

                while (*pCursor && GLWindow_IsSpaceChar(*pCursor))
                {
                    ++pCursor;
                }

                if (!*pCursor)
                {
                    break;
                }

                pTokenStart = pCursor;
                while (*pCursor && !GLWindow_IsSpaceChar(*pCursor))
                {
                    ++pCursor;
                }

                pTokenEnd = pCursor;
                chRestore = *pTokenEnd;
                *pTokenEnd = '\0';

                if (nFaceVertexCount < (int)ARRAYSIZE(faceVertices) &&
                    GLWindow_ParseFaceVertexToken(pTokenStart, &faceVertex))
                {
                    faceVertices[nFaceVertexCount++] = faceVertex;
                }

                *pTokenEnd = chRestore;
            }

            for (int i = 1; i + 1 < nFaceVertexCount; ++i)
            {
                GLFaceVertex triangle[3] = {
                    faceVertices[0],
                    faceVertices[i],
                    faceVertices[i + 1]
                };
                GLVertex drawVertices[3] = { 0 };
                GLVec3 faceNormal = { 0.0f, 1.0f, 0.0f };

                for (int j = 0; j < 3; ++j)
                {
                    if (triangle[j].iPosition <= 0 || (size_t)triangle[j].iPosition > source.nPositionCount)
                    {
                        fclose(pFile);
                        GLSource_Free(&source);
                        GLWindow_FreeMesh(pMesh);
                        return FALSE;
                    }

                    drawVertices[j].position = source.pPositions[triangle[j].iPosition - 1];
                    if (triangle[j].iTexcoord > 0 && (size_t)triangle[j].iTexcoord <= source.nTexcoordCount)
                    {
                        drawVertices[j].texcoord = source.pTexcoords[triangle[j].iTexcoord - 1];
                    }
                    if (triangle[j].iNormal > 0 && (size_t)triangle[j].iNormal <= source.nNormalCount)
                    {
                        drawVertices[j].normal = source.pNormals[triangle[j].iNormal - 1];
                    }
                }

                if (triangle[0].iNormal <= 0 || triangle[1].iNormal <= 0 || triangle[2].iNormal <= 0)
                {
                    faceNormal = GLVec3_Normalize(
                        GLVec3_Cross(
                            GLVec3_Sub(drawVertices[1].position, drawVertices[0].position),
                            GLVec3_Sub(drawVertices[2].position, drawVertices[0].position)));
                    drawVertices[0].normal = faceNormal;
                    drawVertices[1].normal = faceNormal;
                    drawVertices[2].normal = faceNormal;
                }

                if (!GLMesh_AppendVertex(pMesh, &drawVertices[0]) ||
                    !GLMesh_AppendVertex(pMesh, &drawVertices[1]) ||
                    !GLMesh_AppendVertex(pMesh, &drawVertices[2]))
                {
                    fclose(pFile);
                    GLSource_Free(&source);
                    GLWindow_FreeMesh(pMesh);
                    return FALSE;
                }
            }
        }
    }

    fclose(pFile);
    GLSource_Free(&source);

    if (pMesh->nVertexCount == 0)
    {
        GLWindow_FreeMesh(pMesh);
        return FALSE;
    }

    GLWindow_FinalizeMesh(pMesh);
    *ppMesh = pMesh;
    return TRUE;
}

static void GLWindow_FreeMesh(GLMesh* pMesh)
{
    if (!pMesh)
    {
        return;
    }

    free(pMesh->pVertices);
    free(pMesh);
}

static void GLWindow_FinalizeMesh(GLMesh* pMesh)
{
    if (!pMesh || pMesh->nVertexCount == 0)
    {
        return;
    }

    {
        GLVec3 vMin = pMesh->pVertices[0].position;
        GLVec3 vMax = pMesh->pVertices[0].position;

        for (size_t i = 0; i < pMesh->nVertexCount; ++i)
        {
            GLVec3 v = pMesh->pVertices[i].position;
            pMesh->pVertices[i].normal = GLVec3_Normalize(pMesh->pVertices[i].normal);

            if (v.x < vMin.x) vMin.x = v.x;
            if (v.y < vMin.y) vMin.y = v.y;
            if (v.z < vMin.z) vMin.z = v.z;
            if (v.x > vMax.x) vMax.x = v.x;
            if (v.y > vMax.y) vMax.y = v.y;
            if (v.z > vMax.z) vMax.z = v.z;
        }

        pMesh->center = GLVec3_Scale(GLVec3_Add(vMin, vMax), 0.5f);

        {
            float sx = vMax.x - vMin.x;
            float sy = vMax.y - vMin.y;
            float sz = vMax.z - vMin.z;
            float maxExtent = sx;
            if (sy > maxExtent) maxExtent = sy;
            if (sz > maxExtent) maxExtent = sz;
            pMesh->fScale = (maxExtent > 0.0f) ? (2.4f / maxExtent) : 1.0f;
        }
    }
}

static BOOL GLWindow_SyncActiveDocumentTexture(GLWindow* pGLWindow)
{
    PanitentApp* pApp = PanitentApp_Instance();
    Document* pDocument = pApp ? PanitentApp_GetActiveDocument(pApp) : NULL;
    Canvas* pCanvas = pDocument ? Document_GetCanvas(pDocument) : NULL;

    if (!pGLWindow || !pCanvas || !Canvas_GetBuffer(pCanvas))
    {
        pGLWindow->pTextureCanvas = NULL;
        pGLWindow->cxTexture = 0;
        pGLWindow->cyTexture = 0;
        return FALSE;
    }

    if (pGLWindow->uTextureId == 0)
    {
        glGenTextures(1, (GLuint*)&pGLWindow->uTextureId);
        if (pGLWindow->uTextureId == 0)
        {
            return FALSE;
        }
    }

    glBindTexture(GL_TEXTURE_2D, (GLuint)pGLWindow->uTextureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if (pGLWindow->pTextureCanvas != pCanvas ||
        pGLWindow->cxTexture != Canvas_GetWidth(pCanvas) ||
        pGLWindow->cyTexture != Canvas_GetHeight(pCanvas))
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            Canvas_GetWidth(pCanvas),
            Canvas_GetHeight(pCanvas),
            0,
            GL_BGRA_EXT,
            GL_UNSIGNED_BYTE,
            Canvas_GetBuffer(pCanvas));

        pGLWindow->pTextureCanvas = pCanvas;
        pGLWindow->cxTexture = Canvas_GetWidth(pCanvas);
        pGLWindow->cyTexture = Canvas_GetHeight(pCanvas);
    }
    else {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            Canvas_GetWidth(pCanvas),
            Canvas_GetHeight(pCanvas),
            GL_BGRA_EXT,
            GL_UNSIGNED_BYTE,
            Canvas_GetBuffer(pCanvas));
    }

    return TRUE;
}

static void GLWindow_DeleteTexture(GLWindow* pGLWindow)
{
    if (!pGLWindow || pGLWindow->uTextureId == 0)
    {
        return;
    }

    if (pGLWindow->hdcGL && pGLWindow->hglrc)
    {
        wglMakeCurrent(pGLWindow->hdcGL, pGLWindow->hglrc);
    }

    glDeleteTextures(1, (GLuint*)&pGLWindow->uTextureId);
    pGLWindow->uTextureId = 0;
    pGLWindow->pTextureCanvas = NULL;
    pGLWindow->cxTexture = 0;
    pGLWindow->cyTexture = 0;
}

static void GLWindow_Render(GLWindow* pGLWindow)
{
    GLMesh* pMesh = NULL;
    BOOL bTextured = FALSE;
    ULONGLONG uNow = 0;

    if (!pGLWindow || !pGLWindow->hdcGL || !pGLWindow->hglrc)
    {
        return;
    }

    pMesh = (GLMesh*)pGLWindow->pMesh;
    if (!wglMakeCurrent(pGLWindow->hdcGL, pGLWindow->hglrc))
    {
        return;
    }

    glViewport(0, 0, max(1, pGLWindow->cxClient), max(1, pGLWindow->cyClient));
    glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLWindow_SetPerspective(45.0f, (float)max(1, pGLWindow->cxClient) / (float)max(1, pGLWindow->cyClient), 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, -0.15f, -4.6f / pGLWindow->fZoom);

    uNow = GetTickCount64();
    if (pGLWindow->bMouseRotating ||
        (pGLWindow->uLastInteractionMs != 0 && (uNow - pGLWindow->uLastInteractionMs) < kGLWindowAutoResumeDelayMs))
    {
        pGLWindow->uLastAutoTickMs = uNow;
    }
    else {
        float fDeltaSeconds = (float)(uNow - pGLWindow->uLastAutoTickMs) / 1000.0f;
        pGLWindow->fAutoYaw = fmodf(pGLWindow->fAutoYaw + fDeltaSeconds * 45.0f, 360.0f);
        pGLWindow->uLastAutoTickMs = uNow;
    }

    glRotatef(pGLWindow->fUserPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(pGLWindow->fUserYaw + pGLWindow->fAutoYaw, 0.0f, 1.0f, 0.0f);

    {
        const GLfloat lightAmbient[] = { 0.10f, 0.10f, 0.12f, 1.0f };
        const GLfloat lightDiffuse[] = { 0.95f, 0.94f, 0.90f, 1.0f };
        const GLfloat lightSpecular[] = { 0.35f, 0.35f, 0.35f, 1.0f };
        const GLfloat lightPosition[] = { -2.6f, 2.4f, 4.8f, 1.0f };
        const GLfloat materialSpecular[] = { 0.35f, 0.35f, 0.35f, 1.0f };
        const GLfloat materialShininess[] = { 32.0f };

        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialShininess);
    }

    if (!pMesh || pMesh->nVertexCount == 0)
    {
        GLWindow_RenderFallback();
        SwapBuffers(pGLWindow->hdcGL);
        return;
    }

    bTextured = GLWindow_SyncActiveDocumentTexture(pGLWindow);
    if (bTextured)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, (GLuint)pGLWindow->uTextureId);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.78f, 0.84f, 0.90f);
    }

    glScalef(pMesh->fScale, pMesh->fScale, pMesh->fScale);
    glTranslatef(-pMesh->center.x, -pMesh->center.y, -pMesh->center.z);

    if (bTextured && pGLWindow->bMatcapMode)
    {
        glDisable(GL_LIGHTING);
    }
    else {
        glEnable(GL_LIGHTING);
    }

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < pMesh->nVertexCount; ++i)
    {
        GLVertex vertex = pMesh->pVertices[i];
        glNormal3f(vertex.normal.x, vertex.normal.y, vertex.normal.z);
        if (bTextured)
        {
            if (pGLWindow->bMatcapMode)
            {
                GLVec2 texcoord = GLWindow_ComputeMatcapTexcoord(pGLWindow, pMesh, vertex.position, vertex.normal);
                glTexCoord2f(texcoord.u, texcoord.v);
            }
            else {
                glTexCoord2f(vertex.texcoord.u, 1.0f - vertex.texcoord.v);
            }
        }
        glVertex3f(vertex.position.x, vertex.position.y, vertex.position.z);
    }
    glEnd();

    if (bTextured)
    {
        glDisable(GL_TEXTURE_2D);
    }

    glEnable(GL_LIGHTING);

    SwapBuffers(pGLWindow->hdcGL);
}

static void GLWindow_RenderFallback(void)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLES);
    glColor3f(0.85f, 0.25f, 0.25f);
    glVertex3f(-0.8f, -0.6f, 0.0f);
    glColor3f(0.25f, 0.85f, 0.35f);
    glVertex3f(0.8f, -0.6f, 0.0f);
    glColor3f(0.25f, 0.45f, 0.95f);
    glVertex3f(0.0f, 0.85f, 0.0f);
    glEnd();

    glEnable(GL_LIGHTING);
}

static void GLWindow_SetPerspective(float fovyDegrees, float aspect, float zNear, float zFar)
{
    float f = 1.0f / tanf((fovyDegrees * 0.5f) * 3.1415926535f / 180.0f);
    GLfloat matrix[16] = {
        f / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (zFar + zNear) / (zNear - zFar), -1.0f,
        0.0f, 0.0f, (2.0f * zFar * zNear) / (zNear - zFar), 0.0f
    };
    glLoadMatrixf(matrix);
}

static BOOL GLMesh_EnsureVertexCapacity(GLMesh* pMesh, size_t nRequiredCapacity)
{
    GLVertex* pVertices = NULL;
    size_t nNewCapacity = 0;

    if (!pMesh)
    {
        return FALSE;
    }

    if (pMesh->nVertexCapacity >= nRequiredCapacity)
    {
        return TRUE;
    }

    nNewCapacity = pMesh->nVertexCapacity ? pMesh->nVertexCapacity : 4096;
    while (nNewCapacity < nRequiredCapacity)
    {
        nNewCapacity *= 2;
    }

    pVertices = (GLVertex*)realloc(pMesh->pVertices, nNewCapacity * sizeof(GLVertex));
    if (!pVertices)
    {
        return FALSE;
    }

    pMesh->pVertices = pVertices;
    pMesh->nVertexCapacity = nNewCapacity;
    return TRUE;
}

static BOOL GLMesh_AppendVertex(GLMesh* pMesh, const GLVertex* pVertex)
{
    if (!pMesh || !pVertex || !GLMesh_EnsureVertexCapacity(pMesh, pMesh->nVertexCount + 1))
    {
        return FALSE;
    }

    pMesh->pVertices[pMesh->nVertexCount++] = *pVertex;
    return TRUE;
}

static BOOL GLSource_EnsurePositionCapacity(GLSourceBuffers* pSource, size_t nRequiredCapacity)
{
    GLVec3* pPositions = NULL;
    size_t nNewCapacity = 0;

    if (!pSource)
    {
        return FALSE;
    }

    if (pSource->nPositionCapacity >= nRequiredCapacity)
    {
        return TRUE;
    }

    nNewCapacity = pSource->nPositionCapacity ? pSource->nPositionCapacity : 4096;
    while (nNewCapacity < nRequiredCapacity)
    {
        nNewCapacity *= 2;
    }

    pPositions = (GLVec3*)realloc(pSource->pPositions, nNewCapacity * sizeof(GLVec3));
    if (!pPositions)
    {
        return FALSE;
    }

    pSource->pPositions = pPositions;
    pSource->nPositionCapacity = nNewCapacity;
    return TRUE;
}

static BOOL GLSource_EnsureTexcoordCapacity(GLSourceBuffers* pSource, size_t nRequiredCapacity)
{
    GLVec2* pTexcoords = NULL;
    size_t nNewCapacity = 0;

    if (!pSource)
    {
        return FALSE;
    }

    if (pSource->nTexcoordCapacity >= nRequiredCapacity)
    {
        return TRUE;
    }

    nNewCapacity = pSource->nTexcoordCapacity ? pSource->nTexcoordCapacity : 4096;
    while (nNewCapacity < nRequiredCapacity)
    {
        nNewCapacity *= 2;
    }

    pTexcoords = (GLVec2*)realloc(pSource->pTexcoords, nNewCapacity * sizeof(GLVec2));
    if (!pTexcoords)
    {
        return FALSE;
    }

    pSource->pTexcoords = pTexcoords;
    pSource->nTexcoordCapacity = nNewCapacity;
    return TRUE;
}

static BOOL GLSource_EnsureNormalCapacity(GLSourceBuffers* pSource, size_t nRequiredCapacity)
{
    GLVec3* pNormals = NULL;
    size_t nNewCapacity = 0;

    if (!pSource)
    {
        return FALSE;
    }

    if (pSource->nNormalCapacity >= nRequiredCapacity)
    {
        return TRUE;
    }

    nNewCapacity = pSource->nNormalCapacity ? pSource->nNormalCapacity : 4096;
    while (nNewCapacity < nRequiredCapacity)
    {
        nNewCapacity *= 2;
    }

    pNormals = (GLVec3*)realloc(pSource->pNormals, nNewCapacity * sizeof(GLVec3));
    if (!pNormals)
    {
        return FALSE;
    }

    pSource->pNormals = pNormals;
    pSource->nNormalCapacity = nNewCapacity;
    return TRUE;
}

static BOOL GLSource_AppendPosition(GLSourceBuffers* pSource, GLVec3 position)
{
    if (!GLSource_EnsurePositionCapacity(pSource, pSource->nPositionCount + 1))
    {
        return FALSE;
    }

    pSource->pPositions[pSource->nPositionCount++] = position;
    return TRUE;
}

static BOOL GLSource_AppendTexcoord(GLSourceBuffers* pSource, GLVec2 texcoord)
{
    if (!GLSource_EnsureTexcoordCapacity(pSource, pSource->nTexcoordCount + 1))
    {
        return FALSE;
    }

    pSource->pTexcoords[pSource->nTexcoordCount++] = texcoord;
    return TRUE;
}

static BOOL GLSource_AppendNormal(GLSourceBuffers* pSource, GLVec3 normal)
{
    if (!GLSource_EnsureNormalCapacity(pSource, pSource->nNormalCount + 1))
    {
        return FALSE;
    }

    pSource->pNormals[pSource->nNormalCount++] = normal;
    return TRUE;
}

static void GLSource_Free(GLSourceBuffers* pSource)
{
    if (!pSource)
    {
        return;
    }

    free(pSource->pPositions);
    free(pSource->pTexcoords);
    free(pSource->pNormals);
    memset(pSource, 0, sizeof(*pSource));
}

static BOOL GLWindow_ParseFaceVertexToken(char* pszToken, GLFaceVertex* pFaceVertex)
{
    char* pCursor = pszToken;
    char* pEnd = NULL;

    if (!pszToken || !pFaceVertex)
    {
        return FALSE;
    }

    memset(pFaceVertex, 0, sizeof(*pFaceVertex));

    pFaceVertex->iPosition = (int)strtol(pCursor, &pEnd, 10);
    if (pEnd == pCursor)
    {
        return FALSE;
    }

    pCursor = pEnd;
    if (*pCursor == '/')
    {
        ++pCursor;
        if (*pCursor != '/')
        {
            pFaceVertex->iTexcoord = (int)strtol(pCursor, &pEnd, 10);
            pCursor = pEnd;
        }

        if (*pCursor == '/')
        {
            ++pCursor;
            pFaceVertex->iNormal = (int)strtol(pCursor, &pEnd, 10);
        }
    }

    return pFaceVertex->iPosition > 0;
}

static GLVec3 GLVec3_Sub(GLVec3 a, GLVec3 b)
{
    GLVec3 r = { a.x - b.x, a.y - b.y, a.z - b.z };
    return r;
}

static GLVec3 GLVec3_Add(GLVec3 a, GLVec3 b)
{
    GLVec3 r = { a.x + b.x, a.y + b.y, a.z + b.z };
    return r;
}

static GLVec3 GLVec3_Scale(GLVec3 v, float s)
{
    GLVec3 r = { v.x * s, v.y * s, v.z * s };
    return r;
}

static GLVec3 GLVec3_Cross(GLVec3 a, GLVec3 b)
{
    GLVec3 r = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return r;
}

static float GLVec3_Length(GLVec3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static GLVec3 GLVec3_Normalize(GLVec3 v)
{
    float length = GLVec3_Length(v);
    if (length > 0.000001f)
    {
        return GLVec3_Scale(v, 1.0f / length);
    }

    {
        GLVec3 up = { 0.0f, 1.0f, 0.0f };
        return up;
    }
}

static GLVec3 GLVec3_Reflect(GLVec3 v, GLVec3 normal)
{
    float dotVN = v.x * normal.x + v.y * normal.y + v.z * normal.z;
    GLVec3 reflected = {
        v.x - 2.0f * dotVN * normal.x,
        v.y - 2.0f * dotVN * normal.y,
        v.z - 2.0f * dotVN * normal.z
    };
    return reflected;
}

static GLVec3 GLVec3_RotateY(GLVec3 v, float degrees)
{
    float radians = degrees * 3.1415926535f / 180.0f;
    float s = sinf(radians);
    float c = cosf(radians);
    GLVec3 rotated = {
        c * v.x + s * v.z,
        v.y,
        -s * v.x + c * v.z
    };
    return rotated;
}

static GLVec3 GLVec3_RotateX(GLVec3 v, float degrees)
{
    float radians = degrees * 3.1415926535f / 180.0f;
    float s = sinf(radians);
    float c = cosf(radians);
    GLVec3 rotated = {
        v.x,
        c * v.y - s * v.z,
        s * v.y + c * v.z
    };
    return rotated;
}

static GLVec2 GLWindow_ComputeMatcapTexcoord(const GLWindow* pGLWindow, const GLMesh* pMesh, GLVec3 position, GLVec3 normal)
{
    GLVec3 eyeNormal = GLVec3_Normalize(normal);
    GLVec3 eyePosition = position;
    GLVec3 eyeVector = { 0.0f, 0.0f, -1.0f };
    GLVec3 reflected = { 0.0f, 0.0f, 1.0f };
    GLVec2 texcoord = { 0.5f, 0.5f };
    float yaw = 0.0f;
    float pitch = 0.0f;
    float m = 0.0f;
    float zoom = 1.0f;
    float cameraYOffset = -0.15f;
    float cameraZ = -4.6f;

    if (pGLWindow)
    {
        yaw = pGLWindow->fUserYaw + pGLWindow->fAutoYaw;
        pitch = pGLWindow->fUserPitch;
        zoom = pGLWindow->fZoom;
    }

    if (zoom <= 0.0001f)
    {
        zoom = 1.0f;
    }

    if (pMesh)
    {
        eyePosition = GLVec3_Sub(eyePosition, pMesh->center);
        eyePosition = GLVec3_Scale(eyePosition, pMesh->fScale);
    }

    eyeNormal = GLVec3_RotateY(eyeNormal, yaw);
    eyeNormal = GLVec3_RotateX(eyeNormal, pitch);
    eyeNormal = GLVec3_Normalize(eyeNormal);

    eyePosition = GLVec3_RotateY(eyePosition, yaw);
    eyePosition = GLVec3_RotateX(eyePosition, pitch);
    eyePosition.y += cameraYOffset;
    eyePosition.z += cameraZ / zoom;

    eyeVector = GLVec3_Normalize(eyePosition);
    reflected = GLVec3_Reflect(eyeVector, eyeNormal);
    m = 2.0f * sqrtf(
        reflected.x * reflected.x +
        reflected.y * reflected.y +
        (reflected.z + 1.0f) * (reflected.z + 1.0f));

    if (m > 0.000001f)
    {
        texcoord.u = reflected.x / m + 0.5f;
        texcoord.v = 0.5f - reflected.y / m;
    }

    if (texcoord.u < 0.0f) texcoord.u = 0.0f;
    if (texcoord.u > 1.0f) texcoord.u = 1.0f;
    if (texcoord.v < 0.0f) texcoord.v = 0.0f;
    if (texcoord.v > 1.0f) texcoord.v = 1.0f;

    return texcoord;
}

static BOOL GLWindow_IsSpaceChar(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}
