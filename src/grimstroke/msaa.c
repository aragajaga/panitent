#include "../precomp.h"

#include "msaa.h"

void GrimstrokeMSAA_CreateSampler(GrimstrokeMSAA* msaaCtx)
{
    memset(msaaCtx, 0, sizeof(msaaCtx));

    msaaCtx->wSamples = 4;
    msaaCtx->hSamples = 4;
}

void GrimstrokeMSAA_AllocateBitMask(GrimstrokeMSAA* msaaCtx)
{
    size_t nCells = msaaCtx->wSamples * msaaCtx->hSamples;
    size_t nBytes = nCells / sizeof(unsigned long long) * 8 + 1;
    msaaCtx->bitMask = (char*)malloc(nBytes * sizeof(unsigned long long));
    msaaCtx->bitMask[0] = 1ULL;
}

void GrimstrokeMSAA_FromMSAACords(GrimstrokeMSAA* msaaCtx, TENTPosf pos)
{
}

void GrimstrokeMSAA_ToMSAACords(GrimstrokeMSAA* msaaCtx, int x, int y)
{
}
