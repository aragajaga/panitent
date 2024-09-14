#include "common.h"

#include "propertygrid.h"
#include "propertygridimpl.h"

PropertyGridCtl* PropertyGridCtl_Create();
void PropertyGridCtl_Init(PropertyGridCtl* pPropertyGridCtl);
void PropertyGridCtl_PreCreate(LPCREATESTRUCT lpcs);

PropertyGridCtl* PropertyGridCtl_Create()
{
    PropertyGridCtl* pPropertyGridCtl = (PropertyGridCtl*)malloc(sizeof(PropertyGridCtl));
    memset(pPropertyGridCtl, 0, sizeof(PropertyGridCtl));
    
    if (pPropertyGridCtl)
    {
        memset(pPropertyGridCtl, 0, sizeof(PropertyGridCtl));
        PropertyGridCtl_Init(pPropertyGridCtl);
    }

    return pPropertyGridCtl;
}

void PropertyGridCtl_Init(PropertyGridCtl* pPropertyGridCtl)
{
    InitPropertyGrid(GetModuleHandle(NULL));

    Window_Init((Window*)pPropertyGridCtl);

    pPropertyGridCtl->base.PreRegister = NULL;
    pPropertyGridCtl->base.PreCreate = PropertyGridCtl_PreCreate;
}

void PropertyGridCtl_PreCreate(LPCREATESTRUCT lpcs)
{
    lpcs->dwExStyle = WS_EX_CLIENTEDGE;
    lpcs->lpszClass = WC_LISTBOX;
    lpcs->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_HASSTRINGS;
}
