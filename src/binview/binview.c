#include "../precomp.h"

#include "binview.h"

#include "../util/assert.h"

void BinView_Init(BinView* pBinView)
{
    ASSERT(pBinView);
    memset(pBinView, 0, sizeof(BinView));

    BinViewDialog_Init(&pBinView->binViewDialog);
}
