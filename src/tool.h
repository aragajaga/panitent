#ifndef TOOL_H
#define TOOL_H

#include "stdafx.h"

typedef struct _tagTOOL {
    void (* OnLButtonUp)(MOUSEEVENT mEvt);
    void (* OnLButtonDown)(MOUSEEVENT mEvt);
    void (* OnMouseMove)(MOUSEEVENT mEvt);
} TOOL;

#endif /* TOOL_H */
