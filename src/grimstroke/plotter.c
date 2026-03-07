#include "../precomp.h"

#include "../alphamask.h"
#include "../canvas.h"
#include "plotter.h"

/*
    [PUBLIC]
    Single pixel plotting callback
*/
void PixelPlotterCallback(void* userData, int x, int y, unsigned char opacity)
{
    PlotterData* data = (PlotterData*)userData;
    if (!data || !data->canvas)
    {
        return;
    }

    int thickness = data->thickness > 0 ? data->thickness : 1;
    int left = x - (thickness - 1) / 2;
    int top = y - (thickness - 1) / 2;

    for (int iy = 0; iy < thickness; ++iy)
    {
        for (int ix = 0; ix < thickness; ++ix)
        {
            Canvas_DrawPixel(data->canvas, left + ix, top + iy,
                color_opacity(data->color, (float)opacity / 255.0f));
        }
    }
}

void MaskPlotterCallback(void* userData, int x, int y, unsigned char opacity)
{
    PlotterData* data = (PlotterData*)userData;
    if (!data || !data->mask)
    {
        return;
    }

    int thickness = data->thickness > 0 ? data->thickness : 1;
    int left = x - (thickness - 1) / 2;
    int top = y - (thickness - 1) / 2;

    for (int iy = 0; iy < thickness; ++iy)
    {
        for (int ix = 0; ix < thickness; ++ix)
        {
            AlphaMask_SetMax(data->mask, left + ix, top + iy, opacity);
        }
    }
}

void Plotter_PlotExact(Plotter* pPlotter, int x, int y, unsigned char opacity)
{
    if (!pPlotter || opacity == 0)
    {
        return;
    }

    PlotterData* data = (PlotterData*)pPlotter->userData;
    if (!data)
    {
        pPlotter->fn(pPlotter->userData, x, y, opacity);
        return;
    }

    if (data->mask)
    {
        AlphaMask_SetMax(data->mask, x, y, opacity);
        return;
    }

    if (data->canvas)
    {
        Canvas_DrawPixel(data->canvas, x, y,
            color_opacity(data->color, (float)opacity / 255.0f));
    }
}
