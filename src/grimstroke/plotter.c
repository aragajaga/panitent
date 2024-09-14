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
    Canvas_DrawPixel(data->canvas, x, y, color_opacity(data->color, (float)opacity / 255.0f));
}
