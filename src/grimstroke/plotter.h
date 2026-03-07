#pragma once

typedef struct _Canvas Canvas;
typedef struct AlphaMask AlphaMask;

typedef struct Plotter {
    void* userData;
    void (*fn)(void* userData, int x, int y, unsigned char opacity);
} Plotter;

typedef struct PlotterData {
    Canvas* canvas;
    AlphaMask* mask;
    uint32_t color;
    int thickness;
} PlotterData;


void PixelPlotterCallback(void* userData, int x, int y, unsigned char opacity);
void MaskPlotterCallback(void* userData, int x, int y, unsigned char opacity);
