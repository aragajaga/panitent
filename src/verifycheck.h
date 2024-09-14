#ifndef PANITENT_VERIFYCHECK_H
#define PANITENT_VERIFYCHECK_H

#include "win32/dialog.h"

typedef struct VerifyCheckDlg VerifyCheckDlg;
struct VerifyCheckDlg {
    Dialog base;
};

void VerifyCheckDlg_Init(VerifyCheckDlg* pVerifyCheckDlg);

#endif  /* PANITENT_VERIFYCHECK_H */
