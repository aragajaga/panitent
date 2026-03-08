#include "precomp.h"

#include "win32/window.h"

#include "canvas.h"
#include "history.h"
#include "document.h"

#include "viewport.h"

#include "dockhost.h"
#include "file_open.h"
#include "crefptr.h"
#include "wic.h"
#include "workspacecontainer.h"

#include "panitentapp.h"

extern TreeNode* viewportNode;

static BOOL Document_InitHistory(Document* pDocument);
static void Document_FreeHistory(History* pHistory);
static void Document_AssignFilePath(Document* pDocument, PCWSTR pszPath);
static ViewportWindow* Document_CreateViewportForWorkspace(Document* pDocument, WorkspaceContainer* pWorkspaceContainer);

void Document_Init(Document* pDocument)
{
    pDocument->canvas = NULL;
    pDocument->history = NULL;
    pDocument->location = NULL;
    pDocument->szFilePath = NULL;
    pDocument->bRecoveryDirty = FALSE;
}

Document* Document_Create()
{
    Document* pDocument = (Document*)malloc(sizeof(Document));
    memset(pDocument, 0, sizeof(Document));

    Document_Init(pDocument);

    return pDocument;
}

void Document_Destroy(Document* pDocument)
{
    if (!pDocument)
    {
        return;
    }

    if (pDocument->szFilePath)
    {
        free(pDocument->szFilePath);
        pDocument->szFilePath = NULL;
    }

    if (pDocument->history)
    {
        Document_FreeHistory(pDocument->history);
        free(pDocument->history);
        pDocument->history = NULL;
    }

    free(pDocument);
}

void Document_SetCanvas(Document* pDocument, Canvas* pCanvas)
{
    pDocument->canvas = pCanvas;
}

BOOL Document_IsFilePathSet(Document* doc)
{
    return doc->szFilePath != NULL;
}

BOOL Document_IsDirty(Document* doc)
{
    if (!doc)
    {
        return FALSE;
    }

    return doc->bRecoveryDirty || History_IsDirty(doc);
}

Document* Document_LoadFile(PCWSTR pszPath)
{
    if (!pszPath || !pszPath[0])
    {
        return NULL;
    }

    Document* pDocument = Document_Create();
    if (!pDocument)
    {
        return NULL;
    }

    if (!Document_InitHistory(pDocument))
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    /* Create and set canvas */
    ImageBuffer ib = ImageFileReader(pszPath);
    if (!ib.bits || ib.width == 0 || ib.height == 0)
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    Canvas* canvas = Canvas_CreateFromBuffer(ib.width, ib.height, ib.bits);
    free(ib.bits);
    if (!canvas)
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    Document_SetCanvas(pDocument, canvas);
    Document_AssignFilePath(pDocument, pszPath);
    pDocument->bRecoveryDirty = FALSE;

    return pDocument;
}

Document* Document_CreateWithCanvas(Canvas* pCanvas)
{
    if (!pCanvas)
    {
        return NULL;
    }

    Document* pDocument = Document_Create();
    if (!pDocument)
    {
        return NULL;
    }

    if (!Document_InitHistory(pDocument))
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    Document_SetCanvas(pDocument, pCanvas);
    return pDocument;
}

BOOL Document_OpenFileInWorkspace(PCWSTR pszPath, WorkspaceContainer* pWorkspaceContainer)
{
    Document* pDocument = Document_LoadFile(pszPath);
    if (!pDocument)
    {
        return FALSE;
    }

    if (!Document_AttachToWorkspace(pDocument, pWorkspaceContainer))
    {
        Document_Destroy(pDocument);
        return FALSE;
    }
    return TRUE;
}

void Document_OpenFile(LPWSTR pszPath)
{
    if (!pszPath || !pszPath[0])
    {
        return;
    }

    Document_OpenFileInWorkspace(pszPath, PanitentApp_GetWorkspaceContainer(PanitentApp_Instance()));
}

void Document_Open(Document* prevDoc)
{
    UNREFERENCED_PARAMETER(prevDoc);

    BOOL bResult;
    LPWSTR pszPath;
    WCHAR szPath[256] = { 0 };

    pszPath = szPath;

    /* Prompt the user file selection */
    bResult = init_open_file_dialog(&pszPath);

    /* If user cancelled the file selection or error occured during file
       dialog initialization */
    if (!bResult)
    {
        return;
    }

    Document_OpenFile(pszPath);
}

void Document_Save(Document* doc)
{
    if (!doc)
    {
        return;
    }

    LPWSTR pszPath = NULL;
    BOOL bResult = init_save_file_dialog(&pszPath);

    if (!bResult)
    {
        return;
    }

    Canvas* c = doc->canvas;
    if (!c)
    {
        free(pszPath);
        return;
    }

    ImageBuffer ib = { 0 };
    ib.width = c->width;
    ib.height = c->height;
    ib.bits = c->buffer;
    ib.size = c->buffer_size;

    ImageFileWriter(pszPath, ib);

    Document_AssignFilePath(doc, pszPath);
    doc->bRecoveryDirty = FALSE;
    History_MarkSaved(doc);
    Window_Invalidate((Window*)PanitentApp_GetWorkspaceContainer(PanitentApp_Instance()));

    free(pszPath);
}

void Document_Purge(Document* doc)
{
    Canvas_Delete(doc->canvas);
}

BOOL Document_Close(Document* doc)
{
    int answer = MessageBox(NULL, L"Do you want to save changes?", L"Panit.ent",
        MB_YESNOCANCEL | MB_ICONWARNING);

    switch (answer) {
    case IDYES:
        Document_Save(doc);
        break;
    case IDNO:
        Document_Purge(doc);
        break;
    default:
        return FALSE;
        break;
    }

    return TRUE;
}

Document* Document_New(int width, int height)
{
    Document* pDocument = Document_Create();
    if (!pDocument)
    {
        return NULL;
    }

    if (!Document_InitHistory(pDocument))
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    Canvas* pCanvas = Canvas_Create(width, height);
    if (!pCanvas)
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    Document_SetCanvas(pDocument, pCanvas);

    if (!Document_AttachToWorkspace(
        pDocument,
        PanitentApp_GetWorkspaceContainer(PanitentApp_Instance())))
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    return pDocument;
}

History* Document_GetHistory(Document* document)
{
    return document->history;
}

Canvas* Document_GetCanvas(Document* document)
{
    if (document == NULL)
        return NULL;

    return document->canvas;
}

PCWSTR Document_GetFilePath(Document* document)
{
    return document ? document->szFilePath : NULL;
}

void Document_SetFilePath(Document* document, PCWSTR pszPath)
{
    Document_AssignFilePath(document, pszPath);
}

void Document_SetRecoveryDirty(Document* document, BOOL bDirty)
{
    if (!document)
    {
        return;
    }

    document->bRecoveryDirty = bDirty ? TRUE : FALSE;
}

BOOL Document_AttachToWorkspace(Document* pDocument, WorkspaceContainer* pWorkspaceContainer)
{
    if (!pDocument)
    {
        return FALSE;
    }

    ViewportWindow* pViewportWindow = Document_CreateViewportForWorkspace(pDocument, pWorkspaceContainer);
    if (!pViewportWindow)
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL Document_InitHistory(Document* pDocument)
{
    if (!pDocument)
    {
        return FALSE;
    }

    pDocument->history = (History*)calloc(1, sizeof(History));
    if (!pDocument->history)
    {
        return FALSE;
    }

    HistoryRecord* initialRecord = (HistoryRecord*)calloc(1, sizeof(HistoryRecord));
    if (!initialRecord)
    {
        free(pDocument->history);
        pDocument->history = NULL;
        return FALSE;
    }

    pDocument->history->peak = initialRecord;
    pDocument->history->saved = initialRecord;
    return TRUE;
}

static void Document_FreeHistory(History* pHistory)
{
    if (!pHistory || !pHistory->peak)
    {
        return;
    }

    HistoryRecord* pHead = pHistory->peak;
    while (pHead->previous)
    {
        pHead = pHead->previous;
    }

    while (pHead)
    {
        HistoryRecord* pNext = pHead->next;
        if (pHead->differential)
        {
            Canvas_Delete(pHead->differential);
            free(pHead->differential);
            pHead->differential = NULL;
        }

        free(pHead);
        pHead = pNext;
    }

    pHistory->peak = NULL;
}

static void Document_AssignFilePath(Document* pDocument, PCWSTR pszPath)
{
    if (!pDocument)
    {
        return;
    }

    if (pDocument->szFilePath)
    {
        free(pDocument->szFilePath);
        pDocument->szFilePath = NULL;
    }

    if (!pszPath || !pszPath[0])
    {
        return;
    }

    size_t cchPath = wcslen(pszPath) + 1;
    pDocument->szFilePath = (LPWSTR)malloc(cchPath * sizeof(WCHAR));
    if (!pDocument->szFilePath)
    {
        return;
    }

    wcscpy_s(pDocument->szFilePath, cchPath, pszPath);
}

static ViewportWindow* Document_CreateViewportForWorkspace(Document* pDocument, WorkspaceContainer* pWorkspaceContainer)
{
    if (!pWorkspaceContainer)
    {
        return NULL;
    }

    ViewportWindow* pViewportWindow = ViewportWindow_Create();
    if (!pViewportWindow)
    {
        return NULL;
    }

    HWND hWndViewport = Window_CreateWindow((Window*)pViewportWindow, NULL);
    if (!hWndViewport || !IsWindow(hWndViewport))
    {
        free(pViewportWindow);
        return NULL;
    }

    if (pDocument)
    {
        ViewportWindow_SetDocument(pViewportWindow, pDocument);
    }

    WorkspaceContainer_AddViewport(pWorkspaceContainer, pViewportWindow);
    PanitentApp_SetActiveViewport(PanitentApp_Instance(), pViewportWindow);
    return pViewportWindow;
}
