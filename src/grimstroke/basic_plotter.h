#ifndef PANITENT_GRIMSTROKE_BASICPLOTTER_H
#define PANITENT_GRIMSTROKE_BASICPLOTTER_H

typedef struct BasicPlotter BasicPlotter;

struct BasicPlotter_vtbl {
    void (*DrawPixel)(BasicPlotter* plotterContext, float xPos, float yPos);
};

struct BasicPlotter {
    struct BasicPlotter_vtbl* pVtbl;
};

void __impl_BasicPlotter_DrawPixel(BasicPlotter* plotterContext, float xPos, float yPos);

struct BasicPlotter_vtbl __g_BasicPlotter_vtbl = {
    .DrawPixel = __impl_BasicPlotter_DrawPixel
};

void BasicPlotter_Init(BasicPlotter* plotterContext)
{
    plotterContext->pVtbl = &__g_BasicPlotter_vtbl;
}

inline BasicPlotter_DrawPixel(BasicPlotter* plotterContext, float xPos, float yPos)
{
    plotterContext->pVtbl->DrawPixel(plotterContext, xPos, yPos);
}

void __impl_BasicPlotter_DrawPixel(BasicPlotter* plotterContext, float xPos, float yPos)
{
    
}

#endif  // PANITENT_GRIMSTROKE_BASICPLOTTER
