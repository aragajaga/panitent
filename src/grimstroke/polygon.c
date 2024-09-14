#include "../precomp.h"

#include "polygon.h"
#include "../util/assert.h"

#include "../canvas.h"
#include "plotter.h"
#include "shapecontext.h"
#include "../panitentapp.h"
#include "pixelbuffer.h"

/* Forward declarations */

/*
    [PRIVATE]
*/
void PolygonPath_Init(PolygonPath* pPolygonPath);

/*
    [PUBLIC]
    Create and initialize a PolygonPath object
*/
PolygonPath* PolygonPath_Create()
{
    PolygonPath* pPolygonPath = (PolygonPath*)malloc(sizeof(PolygonPath));

    if (pPolygonPath)
    {
        PolygonPath_Init(pPolygonPath);
    }
    
    return pPolygonPath;
}

/*
    [PRIVATE]
    Initialize a PolygonPath object
*/
void PolygonPath_Init(PolygonPath* pPolygonPath)
{
    ASSERT(pPolygonPath);
    memset(pPolygonPath, 0, sizeof(PolygonPath));
    Vector_Init(&pPolygonPath->points, sizeof(Point));
}

/*
    [PUBLIC]
    Add a point to the polygon path
*/
void PolygonPath_AddPoint(PolygonPath* pPolygonPath, int x, int y)
{
    Point p = {
        .x = x,
        .y = y
    };

    Vector_PushBack(&pPolygonPath->points, &p);
}

/*
    [PUBLIC]
    Close the polygon
*/
void PolygonPath_Enclose(PolygonPath* pPolygonPath)
{
    Vector_PushBack(&pPolygonPath->points, Vector_At(&pPolygonPath->points, 0));
}

typedef struct PixelBufferPlotterData {
    PixelBuffer* pPixelBuffer;
    Color color;
} PixelBufferPlotterData;

void PixelBufferPlotterCallback(void* userData, int x, int y, unsigned int opacity)
{
    PixelBufferPlotterData* pPixelBufferPlotterData = (PixelBufferPlotterData*)userData;
    
    PixelBuffer* pPixelBuffer = pPixelBufferPlotterData->pPixelBuffer;
    Color* color = &pPixelBufferPlotterData->color;
    color->a = opacity;
    PixelBuffer_SetPixel(pPixelBuffer, x, y, *color);
}

void Polygon_DrawOnPixelBuffer(PolygonPath* pPolygonPath, PixelBuffer* pPixelBuffer, Color color)
{
    ASSERT(pPolygonPath && pPixelBuffer);

    ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());

    Plotter plotter = { 0 };

    // Initiate pixel buffer plotter data
    PixelBufferPlotterData* pPixelBufferPlotterData = (PixelBufferPlotterData*)malloc(sizeof(PixelBufferPlotterData));
    ASSERT(pPixelBufferPlotterData);
    memset(pPixelBufferPlotterData, 0, sizeof(PixelBufferPlotterData));
    pPixelBufferPlotterData->pPixelBuffer = pPixelBuffer;
    pPixelBufferPlotterData->color = color;

    plotter.userData = (void*)pPixelBufferPlotterData;
    plotter.fn = PixelBufferPlotterCallback;

    Vector* points = &pPolygonPath->points;

    Vector slopes;
    Vector_Init(&slopes, sizeof(float));

    // Reserve memory for slopes
    Vector_Reserve(&slopes, Vector_GetSize(points));

    // Calculate slopes
    for (size_t i = 0; i < Vector_GetSize(points); ++i)
    {
        Point* ptThis = (Point*)Vector_At(points, i);
        Point* ptNext = (Point*)Vector_At(points, (i + 1) % Vector_GetSize(points));    // Wrap around the first point
        int dx = ptNext->x - ptThis->x;
        int dy = ptNext->y - ptThis->y;

        float slope = (dx == 0) ? 0.0f : (dy == 0) ? 0.0f : dx / (float)dy;
        Vector_PushBack(&slopes, &slope);
    }

    // Scanline fill
    ShapeContext_SetPlotter(pShapeContext, &plotter);

    for (int y = 0; y < PixelBuffer_GetHeight(pPixelBuffer); ++y)
    {
        Vector xi;
        Vector_Init(&xi, sizeof(int));

        for (size_t i = 0; i < Vector_GetSize(points); ++i)
        {
            Point* ptThis = (Point*)Vector_At(points, i);
            Point* ptNext = (Point*)Vector_At(points, (i + 1) % Vector_GetSize(points));    // Wrap around the first point

            if (((ptThis->y <= y) && (ptNext->y > y)) ||
                ((ptThis->y > y) && (ptNext->y <= y)))
            {
                int x = (int)(ptThis->x + (*(float*)Vector_At(&slopes, i)) * (y - ptThis->y));
                Vector_PushBack(&xi, &x);
            }
        }

        // Sort xi
        if (Vector_GetSize(&xi) > 1)
        {
            for (size_t j = 0; j < Vector_GetSize(&xi) - 1; ++j)
            {
                for (size_t i = 0; i < Vector_GetSize(&xi) - 1; ++i)
                {
                    if (*(int*)Vector_At(&xi, i) > *(int*)Vector_At(&xi, i + 1))
                    {
                        int temp = *(int*)Vector_At(&xi, i);
                        *(int*)Vector_At(&xi, i) = *(int*)Vector_At(&xi, i + 1);
                        *(int*)Vector_At(&xi, i + 1) = temp;
                    }
                }
            }
        }        

        // Draw lines between pairs of intersections
        for (size_t i = 0; i < Vector_GetSize(&xi); i += 2)
        {
            //if (i + 1 < Vector_GetSize(&xi))
            //{
                ShapeContext_DrawLine(pShapeContext, *(int*)Vector_At(&xi, i), y, *(int*)Vector_At(&xi, i + 1), y);
            //}
        }

        // Free xi for this scanline
        Vector_Free(&xi);
    }

    ShapeContext_SetPlotter(pShapeContext, NULL);

    Vector_Free(&slopes);
    free(plotter.userData);
}

void Canvas_FillPolygon(Canvas* pCanvas, PolygonPath* pPolygonPath)
{
    ShapeContext* pShapeContext = PanitentApp_GetShapeContext(PanitentApp_Instance());

    Plotter plotter = { 0 };
    PlotterData* pPlotterData = (PlotterData*)malloc(sizeof(PlotterData));
    ASSERT(pPlotterData && pPolygonPath);
    memset(pPlotterData, 0, sizeof(PlotterData));

    pPlotterData->canvas = pCanvas;
    pPlotterData->color = 0xFF00FFFF;
    plotter.userData = pPlotterData;
    plotter.fn = PixelPlotterCallback;

    Vector* points = &pPolygonPath->points;

    Vector slopes;
    Vector_Init(&slopes, sizeof(float));

    // TODO: Vector_Reserve
    float val = 0.0f;
    for (size_t i = 0; i < Vector_GetSize(points) + 1; ++i)
    {
        Vector_PushBack(&slopes, (void*)&val);
    }

    ShapeContext_SetPlotter(pShapeContext, &plotter);
    for (size_t i = 0; i < Vector_GetSize(points); ++i)
    {
        Point* ptThis = (Point*)Vector_At(points, i);
        Point* ptNext = (Point*)Vector_At(points, i + 1);
        int dx = ptNext->x - ptThis->x;
        int dy = ptNext->y - ptThis->y;

        if (!dx)
        {
            *((float*)Vector_At(&slopes, i)) = 0.0f;
        }

        if (!dy)
        {
            *((float*)Vector_At(&slopes, i)) = 1.0f;
        }

        if (dx && dy)
        {
            *((float*)Vector_At(&slopes, i)) = dx / (float)dy;
        }
    }

    for (int y = 0; y < Canvas_GetHeight(pCanvas); y++)
    {
        Vector xi;
        Vector_Init(&xi, sizeof(int));

        for (size_t i = 0; i < Vector_GetSize(points); ++i)
        {
            Point* ptThis = (Point*)Vector_At(points, i);
            Point* ptNext = (Point*)Vector_At(points, i + 1);
            if (((ptThis->y <= y) && (ptNext->y > y)) ||
                ((ptThis->y > y) && (ptNext->y <= y)))
            {
                int val = (int)(ptThis->x + *(float*)Vector_At(&slopes, i) * ptThis->y);
                Vector_PushBack(&xi, &val);
            }
        }

        for (size_t j = 0; j < Vector_GetSize(&xi) - 1; ++j) {
            for (size_t i = 0; i < Vector_GetSize(&xi) - 1; ++i) {
                if (*(int*)Vector_At(&xi, i) > *(int*)Vector_At(&xi, i + 1)) {
                    int temp = *(int*)Vector_At(&xi, i);
                    *(int*)Vector_At(&xi, i) = *(int*)Vector_At(&xi, i + 1);
                    *(int*)Vector_At(&xi, i + 1) = temp;
                }
            }
        }

        for (size_t i = 0; i < Vector_GetSize(&xi); i += 2)
        {
            ShapeContext_DrawLine(pShapeContext, *(int*)Vector_At(&xi, i), y, *(int*)Vector_At(&xi, i + i), y);
        }
    }

    ShapeContext_SetPlotter(pShapeContext, NULL);

    free(plotter.userData);
}
