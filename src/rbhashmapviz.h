#pragma once

#ifndef RBHASHMAPVIZ_H_
#define RBHASHMAPVIZ_H_

#include "win32/dialog.h"

typedef struct RBHashMap RBHashMap;

typedef struct RBHashMapVizDialog RBHashMapVizDialog;
struct RBHashMapVizDialog {
	Dialog base;

	RBHashMap* m_pHashMap;
};

RBHashMapVizDialog* RBHashMapVizDialog_Create();
void RBHashMapVizDialog_Init(RBHashMapVizDialog* pRBHashMapVizDialog);

#endif  /* RBHASHMAPVIZ_H_ */
