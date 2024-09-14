#ifndef BINVIEW_H
#define BINVIEW_H

#include "../win32/dialog.h"

typedef struct BinViewDialog BinViewDialog;
struct BinViewDialog {
    Dialog base;
};

void BinViewDialog_Init(BinViewDialog* pBinViewDialog);

#endif  /* BINVIEW_H */
