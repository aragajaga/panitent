#include "precomp.h"

#include "floatingdockpolicy.h"

DockPaneKind FloatingDockPolicy_GetPaneKind(FloatingDockPolicy nDockPolicy)
{
	return nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT ? DOCK_PANE_DOCUMENT : DOCK_PANE_TOOL;
}

BOOL FloatingDockPolicy_UsesDocumentFlow(FloatingDockPolicy nDockPolicy)
{
	return nDockPolicy == FLOAT_DOCK_POLICY_DOCUMENT;
}

BOOL FloatingDockPolicy_CanShowDockCommand(FloatingDockPolicy nDockPolicy, FloatingDockChildHostKind nChildKind, BOOL bHasDockHostTarget)
{
	if (nChildKind == FLOAT_DOCK_CHILD_NONE)
	{
		return FALSE;
	}

	if (FloatingDockPolicy_UsesDocumentFlow(nDockPolicy))
	{
		return FloatingDockPolicy_CanMoveDocumentsToWorkspace(nChildKind);
	}

	return FloatingDockPolicy_CanUseHostDockTarget(nDockPolicy, nChildKind, bHasDockHostTarget);
}

BOOL FloatingDockPolicy_CanUseHostDockTarget(FloatingDockPolicy nDockPolicy, FloatingDockChildHostKind nChildKind, BOOL bHasDockHostTarget)
{
	UNREFERENCED_PARAMETER(nDockPolicy);

	if (!bHasDockHostTarget)
	{
		return FALSE;
	}

	switch (nChildKind)
	{
	case FLOAT_DOCK_CHILD_TOOL_PANEL:
	case FLOAT_DOCK_CHILD_TOOL_HOST:
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL FloatingDockPolicy_CanMoveDocumentsToWorkspace(FloatingDockChildHostKind nChildKind)
{
	switch (nChildKind)
	{
	case FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE:
	case FLOAT_DOCK_CHILD_DOCUMENT_HOST:
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL FloatingDockPolicy_RequiresWorkspaceMergeForSideDock(FloatingDockChildHostKind nChildKind)
{
	return nChildKind == FLOAT_DOCK_CHILD_DOCUMENT_HOST;
}
