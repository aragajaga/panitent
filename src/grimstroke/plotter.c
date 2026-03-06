#include "../precomp.h"

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
