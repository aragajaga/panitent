#ifndef PANITENT_TOOL_H
#define PANITENT_TOOL_H

#include "precomp.h"
#include "panitent.h"

typedef struct _tagTOOL {
    void (* OnLButtonUp)(MOUSEEVENT mEvt);
    void (* OnLButtonDown)(MOUSEEVENT mEvt);
    void (* OnMouseMove)(MOUSEEVENT mEvt);
} TOOL;

#endif /* PANITENT_TOOL_H */
