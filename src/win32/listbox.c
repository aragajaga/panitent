#include "common.h"

#include "listbox.h"

ListBoxCtl* ListBoxCtl_Create(Application* pApplication);
void ListBoxCtl_Init(ListBoxCtl* pListBoxCtl, Application* pApplication);
void ListBoxCtl_PreCreate(LPCREATESTRUCT lpcs);

ListBoxCtl* ListBoxCtl_Create(Application* pApplication)
{
    ListBoxCtl* pListBoxCtl = (ListBoxCtl*)malloc(sizeof(ListBoxCtl));
    memset(pListBoxCtl, 0, sizeof(ListBoxCtl));
    
    if (pListBoxCtl)
    {
        memset(pListBoxCtl, 0, sizeof(ListBoxCtl));
        ListBoxCtl_Init(pListBoxCtl, pApplication);
    }

    return pListBoxCtl;
}

void ListBoxCtl_Init(ListBoxCtl* pListBoxCtl, Application* pApplication)
{
    Window_Init((Window*)pListBoxCtl, pApplication);

    pListBoxCtl->base.PreRegister = NULL;
    pListBoxCtl->base.PreCreate = ListBoxCtl_PreCreate;
}

void ListBoxCtl_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = WS_EX_CLIENTEDGE;
    lpcs->lpszClass = WC_LISTBOX;
    lpcs->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_HASSTRINGS;
}
