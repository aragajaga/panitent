#include "precomp.h"
#include "dock_site.h"

DockSite* DockSite_Create(HWND hWndOwner) {
    DockSite* pSite = (DockSite*)calloc(1, sizeof(DockSite));
    if (!pSite) return NULL;

    pSite->hWnd = hWndOwner;
    pSite->allPanes = List_Create(sizeof(DockPane*));
    pSite->allContents = List_Create(sizeof(DockPane*));
    if (!pSite->allPanes || !pSite->allContents) {
        if(pSite->allPanes) List_Destroy(pSite->allPanes);
        if(pSite->allContents) List_Destroy(pSite->allContents);
        free(pSite);
        return NULL;
    }
    for (int i = 0; i < 4; ++i) {
        pSite->autoHideAreas[i].side = (AutoHideSide)i;
        pSite->autoHideAreas[i].hiddenTools = List_Create(sizeof(void*));
    }
    return pSite;
}

void DockPane_Destroy(DockPane* pPane) {
    if (!pPane) return;
    if (pPane->contents) {
        List_Destroy(pPane->contents);
        pPane->contents = NULL;
    }
    if (pPane->tabRects) {
        List_Destroy(pPane->tabRects);
        pPane->tabRects = NULL;
    }
    free(pPane);
}

void DockGroup_DestroyRecursive(DockGroup* pGroup) {
    if (!pGroup) return;

    if (pGroup->child1) {
        if (pGroup->isChild1Group) {
            DockGroup_DestroyRecursive((DockGroup*)pGroup->child1);
        } else {
            DockPane_Destroy((DockPane*)pGroup->child1);
        }
        pGroup->child1 = NULL;
    }

    if (pGroup->child2) {
        if (pGroup->isChild2Group) {
            DockGroup_DestroyRecursive((DockGroup*)pGroup->child2);
        } else {
            DockPane_Destroy((DockPane*)pGroup->child2);
        }
        pGroup->child2 = NULL;
    }

    free(pGroup);
}

void DockSite_Destroy(DockSite* pSite) {
    if (!pSite) return;

    if (pSite->rootGroup) {
        DockGroup_DestroyRecursive(pSite->rootGroup);
        pSite->rootGroup = NULL;
    }

    if (pSite->allPanes) {
        List_Destroy(pSite->allPanes);
        pSite->allPanes = NULL;
    }

    if (pSite->allContents) {
        List_Destroy(pSite->allContents);
        pSite->allContents = NULL;
    }

    for (int i = 0; i < 4; ++i) {
        if (pSite->autoHideAreas[i].hiddenTools) {
            List_Destroy(pSite->autoHideAreas[i].hiddenTools);
            pSite->autoHideAreas[i].hiddenTools = NULL;
        }
    }
    free(pSite);
}

