#include "precomp.h"

#include "workspacedockpolicy.h"

BOOL WorkspaceDockPolicy_CanSplitTarget(
    int nSourceDocumentCount,
    int nTargetDocumentCount,
    BOOL bTargetSupportsSplitByHost)
{
    if (!bTargetSupportsSplitByHost)
    {
        return FALSE;
    }

    /*
     * VS-like behavior:
     * when a single document tab is detached and dragged back to its own empty
     * origin group, only center return is valid. Side splits are suppressed.
     */
    if (nSourceDocumentCount <= 1 && nTargetDocumentCount <= 0)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(
    int nDocumentCount,
    BOOL bIsMainWorkspace,
    BOOL bIsDockedInHost)
{
    if (!bIsDockedInHost)
    {
        return FALSE;
    }

    if (bIsMainWorkspace)
    {
        return FALSE;
    }

    return nDocumentCount <= 0 ? TRUE : FALSE;
}
