#include "precomp.h"

#include "win32/window.h"

#include <assert.h>
#include <strsafe.h>
#include "history.h"
#include "document.h"
#include "viewport.h"
#include "canvas.h"
#include "panitent.h"

Canvas* g_historyTempSavedState;

void History_Undo(Document* document)
{
    History* history = Document_GetHistory(document);
    Canvas* canvas = Document_GetCanvas(document);

    Canvas* mask = history->peak->differential;
    RECT* rc = &history->peak->rc;

    uint32_t* pDest = canvas->buffer;
    uint32_t* pOrig = mask->buffer;
    pDest += rc->top * canvas->width + rc->left;

    for (int i = 0; i < mask->height; i++) {
        for (int j = 0; j < mask->width; j++) {
            *pDest++ ^= *pOrig++;
        }

        pDest += canvas->width - mask->width;
    }

    history->peak = history->peak->previous;
    Window_Invalidate((Window*)Panitent_GetActiveViewport());
}

void History_Redo(Document* document)
{
    History* history = Document_GetHistory(document);
    Canvas* canvas = Document_GetCanvas(document);

    Canvas* mask = history->peak->next->differential;
    RECT* rc = &history->peak->next->rc;

    uint32_t* pDest = canvas->buffer;
    uint32_t* pOrig = mask->buffer;
    pDest += rc->top * canvas->width + rc->left;

    for (int i = 0; i < mask->height; i++) {
        for (int j = 0; j < mask->width; j++) {
            *pDest++ ^= *pOrig++;
        }

        pDest += canvas->width - mask->width;
    }

    history->peak = history->peak->next;
    Window_Invalidate((Window*)Panitent_GetActiveViewport());
}

void History_PushRecord(Document* document, HistoryRecord record)
{
    History* history = Document_GetHistory(document);

    HistoryRecord* sharedRecord = (HistoryRecord*)malloc(sizeof(HistoryRecord));
    memset(sharedRecord, 0, sizeof(HistoryRecord));
    if (sharedRecord)
    {
        memcpy(sharedRecord, &record, sizeof(HistoryRecord));

        sharedRecord->previous = history->peak;
        history->peak->next = sharedRecord;
        history->peak = sharedRecord;
    }
}

void History_DestroyFollowingRedoes(HistoryRecord* record)
{
    if (record != NULL)
    {
        Canvas_Delete(record->differential);

        History_DestroyFollowingRedoes(record->next);
        free(record);
    }
}

void History_StartDifferentiation(Document* document)
{
    Canvas* canvas = Document_GetCanvas(document);
    History* history = Document_GetHistory(document);

    History_DestroyFollowingRedoes(history->peak->next);

    Canvas* savedState = Canvas_Clone(canvas);
    assert(savedState);

    g_historyTempSavedState = savedState;
}

void History_FinalizeDifferentiation(Document* document)
{
    Canvas* current = Document_GetCanvas(document);
    Canvas* saved = g_historyTempSavedState;

    for (size_t i = 0; i < current->buffer_size; i++)
    {
        ((unsigned char*)saved->buffer)[i] ^= ((unsigned char*)current->buffer)[i];
    }
    g_historyTempSavedState = NULL;


    uint32_t* imageBuffer = saved->buffer;

    RECT rc = { current->width, current->height, 0, 0 };

    BOOL topSet = FALSE;
    for (int y = 0; y < current->height; y++)
    {
        for (int x = 0; x < current->width; x++)
        {
            if (imageBuffer[y * current->width + x] != 0)
            {
                if (!topSet)
                {
                    rc.top = y;
                    topSet = TRUE;
                }

                if (x < rc.left)
                {
                    rc.left = x;
                }

                if (x > rc.right)
                {
                    rc.right = x;
                }

                if (y > rc.bottom)
                {
                    rc.bottom = y;
                }
            }
        }
    }

    rc.right++;
    rc.bottom++;

    Canvas* cropped = Canvas_Substitute(saved, &rc);
    Canvas_Delete(saved);

    HistoryRecord record = { 0 };
    record.differential = cropped;
    record.rc = rc;

    History_PushRecord(document, record);
}
