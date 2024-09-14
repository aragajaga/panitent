#include "common.h"

#include "listbox.h"

ListBoxCtl* ListBoxCtl_Create();
void ListBoxCtl_Init(ListBoxCtl* pListBoxCtl);
void ListBoxCtl_PreCreate(LPCREATESTRUCT lpcs);

ListBoxCtl* ListBoxCtl_Create()
{
    ListBoxCtl* pListBoxCtl = (ListBoxCtl*)malloc(sizeof(ListBoxCtl));
    
    if (pListBoxCtl)
    {
        ListBoxCtl_Init(pListBoxCtl);
    }

    return pListBoxCtl;
}

void ListBoxCtl_Init(ListBoxCtl* pListBoxCtl)
{
    memset(pListBoxCtl, 0, sizeof(ListBoxCtl));

    Window_Init((Window*)pListBoxCtl);

    pListBoxCtl->base.PreRegister = NULL;
    pListBoxCtl->base.PreCreate = ListBoxCtl_PreCreate;
}

void ListBoxCtl_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = WS_EX_CLIENTEDGE;
    lpcs->lpszClass = WC_LISTBOX;
    lpcs->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_HASSTRINGS;
}
