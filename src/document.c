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

void Document_Init(Document* pDocument)
{
    pDocument->canvas = NULL;
    pDocument->history = NULL;
    pDocument->location = NULL;
    pDocument->szFilePath = NULL;
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

    if (pDocument->history)
    {
        free(pDocument->history);
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

void Document_OpenFile(LPWSTR pszPath)
{
    Document* pDocument = Document_Create();
    if (!pDocument)
    {
        return;
    }

    pDocument->history = (History*)malloc(sizeof(History));
    memset(pDocument->history, 0, sizeof(History));
    if (!pDocument->history)
    {
        Document_Destroy(pDocument);
        return;
    }

    HistoryRecord* initialRecord = (HistoryRecord*)malloc(sizeof(HistoryRecord));
    memset(initialRecord, 0, sizeof(HistoryRecord));
    pDocument->history->peak = initialRecord;

    /* Create and set canvas */
    ImageBuffer ib = ImageFileReader(pszPath);
    Canvas* canvas = Canvas_CreateFromBuffer(ib.width, ib.height, ib.bits);
    Document_SetCanvas(pDocument, canvas);

    ViewportWindow* pViewportWindow = ViewportWindow_Create();
    Window_CreateWindow((Window*)pViewportWindow, NULL);

    ViewportWindow_SetDocument(pViewportWindow, pDocument);

    // Panitent_SetActiveViewport(pViewportWindow);
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
    LPWSTR pszPath = NULL;
    BOOL bResult = init_save_file_dialog(&pszPath);

    if (!bResult)
    {
        return;
    }

    Canvas* c = doc->canvas;
    ImageBuffer ib = { 0 };
    ib.width = c->width;
    ib.height = c->height;
    ib.bits = c->buffer;
    ib.size = c->buffer_size;

    ImageFileWriter(pszPath, ib);

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
    ViewportWindow* pViewportWindow = PanitentApp_GetActiveViewport(PanitentApp_Instance());

    if (!pViewportWindow) {
        pViewportWindow = ViewportWindow_Create();
        Window_CreateWindow((Window*)pViewportWindow, NULL);

        WorkspaceContainer* pWorkspaceContainer = PanitentApp_GetWorkspaceContainer(PanitentApp_Instance());
        WorkspaceContainer_AddViewport(pWorkspaceContainer, pViewportWindow);

        PanitentApp_SetActiveViewport(PanitentApp_Instance(), pViewportWindow);
    }

    Document* pDocument = Document_Create();
    if (!pDocument)
    {
        return NULL;
    }

    pDocument->history = (History*)malloc(sizeof(History));
    memset(pDocument->history, 0, sizeof(History));
    if (!pDocument->history)
    {
        Document_Destroy(pDocument);
        return NULL;
    }

    HistoryRecord* initialRecord = (HistoryRecord*)malloc(sizeof(HistoryRecord));
    memset(initialRecord, 0, sizeof(HistoryRecord));
    pDocument->history->peak = initialRecord;

    Canvas* pCanvas = Canvas_Create(width, height);
    Document_SetCanvas(pDocument, pCanvas);
    pDocument->canvas = pCanvas;

    ViewportWindow_SetDocument(pViewportWindow, pDocument);

    return pDocument;
}

History* Document_GetHistory(Document* document)
{
    return document->history;
}

Canvas* Document_GetCanvas(Document* document)
{
    return document->canvas;
}
