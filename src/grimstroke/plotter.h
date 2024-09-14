#pragma once

typedef struct _Canvas Canvas;

typedef struct Plotter {
    void* userData;
    void (*fn)(void* userData, int x, int y, unsigned char opacity);
} Plotter;

typedef struct PlotterData {
    Canvas* canvas;
    uint32_t color;
} PlotterData;


void PixelPlotterCallback(void* userData, int x, int y, unsigned char opacity);
