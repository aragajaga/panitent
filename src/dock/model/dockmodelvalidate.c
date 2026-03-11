#include "precomp.h"

#include "dockmodelvalidate.h"

#include "dockviewcatalog.h"

typedef struct DockModelValidateContext
{
	BOOL seenViews[PNT_DOCK_VIEW_OPTIONBAR + 1];
	int nWorkspaceCount;
	DockModelValidateStats stats;
} DockModelValidateContext;

static BOOL DockModelValidate_IsSplitRole(DockNodeRole nRole)
{
	return nRole == DOCK_ROLE_SHELL_SPLIT ||
		nRole == DOCK_ROLE_ZONE_STACK_SPLIT ||
		nRole == DOCK_ROLE_PANEL_SPLIT;
}

static void DockModelValidate_CopyString(WCHAR* pszDest, size_t cchDest, PCWSTR pszSource)
{
	if (!pszDest || cchDest == 0)
	{
		return;
	}

	pszDest[0] = L'\0';
	if (pszSource && pszSource[0] != L'\0')
	{
		wcscpy_s(pszDest, cchDest, pszSource);
	}
}

static BOOL DockModelValidate_SetStringIfDifferent(WCHAR* pszDest, size_t cchDest, PCWSTR pszValue, DockModelValidateContext* pContext)
{
	if (!pszDest || !pszValue)
	{
		return FALSE;
	}

	if (wcscmp(pszDest, pszValue) == 0)
	{
		return FALSE;
	}

	DockModelValidate_CopyString(pszDest, cchDest, pszValue);
	if (pContext)
	{
		pContext->stats.nRepairs++;
	}
	return TRUE;
}

static PCWSTR DockModelValidate_GetZoneName(int nDockSide)
{
	switch (nDockSide)
	{
	case DKS_LEFT:
		return L"DockZone.Left";
	case DKS_RIGHT:
		return L"DockZone.Right";
	case DKS_TOP:
		return L"DockZone.Top";
	case DKS_BOTTOM:
		return L"DockZone.Bottom";
	default:
		return NULL;
	}
}

static BOOL DockModelValidate_SubtreeContainsPaneKind(const DockModelNode* pNode, DockPaneKind nPaneKind)
{
	if (!pNode)
	{
		return FALSE;
	}

	if (pNode->nPaneKind == nPaneKind)
	{
		return TRUE;
	}

	return DockModelValidate_SubtreeContainsPaneKind(pNode->pChild1, nPaneKind) ||
		DockModelValidate_SubtreeContainsPaneKind(pNode->pChild2, nPaneKind);
}

static BOOL DockModelValidate_SubtreeContainsName(const DockModelNode* pNode, PCWSTR pszName)
{
	if (!pNode || !pszName || !pszName[0])
	{
		return FALSE;
	}

	if (wcscmp(pNode->szName, pszName) == 0)
	{
		return TRUE;
	}

	return DockModelValidate_SubtreeContainsName(pNode->pChild1, pszName) ||
		DockModelValidate_SubtreeContainsName(pNode->pChild2, pszName);
}

static PCWSTR DockModelValidate_FindFirstLeafName(const DockModelNode* pNode)
{
	if (!pNode)
	{
		return NULL;
	}

	if (pNode->nRole == DOCK_ROLE_PANEL || pNode->nRole == DOCK_ROLE_WORKSPACE)
	{
		return pNode->szName[0] ? pNode->szName : NULL;
	}

	PCWSTR pszName = DockModelValidate_FindFirstLeafName(pNode->pChild1);
	if (pszName)
	{
		return pszName;
	}

	return DockModelValidate_FindFirstLeafName(pNode->pChild2);
}

static BOOL DockModelValidate_CollapseUnarySplit(DockModelNode** ppNode, DockModelValidateContext* pContext)
{
	DockModelNode* pNode;
	DockModelNode* pChild;
	if (!ppNode || !*ppNode)
	{
		return TRUE;
	}

	pNode = *ppNode;
	if (!DockModelValidate_IsSplitRole(pNode->nRole))
	{
		return TRUE;
	}

	if (!pNode->pChild1 && !pNode->pChild2)
	{
		pContext->stats.nErrors++;
		return FALSE;
	}

	if (pNode->pChild1 && pNode->pChild2)
	{
		return TRUE;
	}

	pChild = pNode->pChild1 ? pNode->pChild1 : pNode->pChild2;
	free(pNode);
	*ppNode = pChild;
	pContext->stats.nRepairs++;
	return TRUE;
}

static BOOL DockModelValidate_ValidateNode(DockModelNode** ppNode, DockModelValidateContext* pContext)
{
	DockModelNode* pNode;
	if (!ppNode || !*ppNode || !pContext)
	{
		return TRUE;
	}

	pNode = *ppNode;
	if (!DockModelValidate_ValidateNode(&pNode->pChild1, pContext) ||
		!DockModelValidate_ValidateNode(&pNode->pChild2, pContext))
	{
		return FALSE;
	}

	if (!DockModelValidate_CollapseUnarySplit(ppNode, pContext))
	{
		return FALSE;
	}
	pNode = *ppNode;

	switch (pNode->nRole)
	{
	case DOCK_ROLE_ROOT:
		pNode->uViewId = PNT_DOCK_VIEW_NONE;
		pNode->nPaneKind = DOCK_PANE_NONE;
		pNode->nDockSide = DKS_NONE;
		pNode->bCollapsed = FALSE;
		pNode->szActiveTabName[0] = L'\0';
		DockModelValidate_SetStringIfDifferent(pNode->szName, ARRAYSIZE(pNode->szName), L"Root", pContext);
		break;

	case DOCK_ROLE_ZONE:
	{
		PCWSTR pszZoneName = DockModelValidate_GetZoneName(pNode->nDockSide);
		if (!pszZoneName)
		{
			pContext->stats.nErrors++;
			return FALSE;
		}

		pNode->uViewId = PNT_DOCK_VIEW_NONE;
		pNode->nPaneKind = DOCK_PANE_NONE;
		pNode->bShowCaption = FALSE;
		DockModelValidate_SetStringIfDifferent(pNode->szName, ARRAYSIZE(pNode->szName), pszZoneName, pContext);
		if (DockModelValidate_SubtreeContainsPaneKind(pNode->pChild1, DOCK_PANE_DOCUMENT) ||
			DockModelValidate_SubtreeContainsPaneKind(pNode->pChild2, DOCK_PANE_DOCUMENT))
		{
			pContext->stats.nErrors++;
			return FALSE;
		}

		if (pNode->szActiveTabName[0] != L'\0' &&
			!DockModelValidate_SubtreeContainsName(pNode->pChild1, pNode->szActiveTabName) &&
			!DockModelValidate_SubtreeContainsName(pNode->pChild2, pNode->szActiveTabName))
		{
			PCWSTR pszFallback = DockModelValidate_FindFirstLeafName(pNode);
			DockModelValidate_SetStringIfDifferent(
				pNode->szActiveTabName,
				ARRAYSIZE(pNode->szActiveTabName),
				pszFallback ? pszFallback : L"",
				pContext);
		}
		break;
	}

	case DOCK_ROLE_WORKSPACE:
		pNode->uViewId = PNT_DOCK_VIEW_WORKSPACE;
		if (pNode->pChild1 || pNode->pChild2)
		{
			pContext->stats.nErrors++;
			return FALSE;
		}
		pNode->nPaneKind = DOCK_PANE_DOCUMENT;
		pNode->nDockSide = DKS_NONE;
		pNode->bShowCaption = FALSE;
		pNode->bCollapsed = FALSE;
		pNode->szActiveTabName[0] = L'\0';
		DockModelValidate_SetStringIfDifferent(pNode->szName, ARRAYSIZE(pNode->szName), L"WorkspaceContainer", pContext);
		pContext->nWorkspaceCount++;
		break;

	case DOCK_ROLE_PANEL:
	{
		PanitentDockViewId nViewId;
		PCWSTR pszCanonicalName;
		if (pNode->pChild1 || pNode->pChild2)
		{
			pContext->stats.nErrors++;
			return FALSE;
		}

		nViewId = pNode->uViewId != 0 ?
			(PanitentDockViewId)pNode->uViewId :
			PanitentDockViewCatalog_Find(pNode->nRole, pNode->szName);
		if (nViewId == PNT_DOCK_VIEW_NONE)
		{
			pContext->stats.nErrors++;
			return FALSE;
		}
		pNode->uViewId = nViewId;
		pszCanonicalName = PanitentDockViewCatalog_GetCanonicalName(nViewId);
		if (pszCanonicalName && pszCanonicalName[0] != L'\0')
		{
			DockModelValidate_SetStringIfDifferent(
				pNode->szName,
				ARRAYSIZE(pNode->szName),
				pszCanonicalName,
				pContext);
		}
		if (pContext->seenViews[nViewId])
		{
			pContext->stats.nErrors++;
			return FALSE;
		}

		pContext->seenViews[nViewId] = TRUE;
		pNode->nPaneKind = DOCK_PANE_TOOL;
		pNode->nDockSide = DKS_NONE;
		pNode->bShowCaption = TRUE;
		pNode->szActiveTabName[0] = L'\0';
		if (pNode->szCaption[0] == L'\0')
		{
			DockModelValidate_SetStringIfDifferent(pNode->szCaption, ARRAYSIZE(pNode->szCaption), pNode->szName, pContext);
		}
		break;
	}

	case DOCK_ROLE_SHELL_SPLIT:
	case DOCK_ROLE_ZONE_STACK_SPLIT:
	case DOCK_ROLE_PANEL_SPLIT:
		pNode->uViewId = PNT_DOCK_VIEW_NONE;
		pNode->nPaneKind = DOCK_PANE_NONE;
		pNode->nDockSide = DKS_NONE;
		pNode->bShowCaption = FALSE;
		pNode->bCollapsed = FALSE;
		pNode->szActiveTabName[0] = L'\0';
		if (!pNode->pChild1 || !pNode->pChild2)
		{
			pContext->stats.nErrors++;
			return FALSE;
		}
		break;

	default:
		pContext->stats.nErrors++;
		return FALSE;
	}

	return TRUE;
}

BOOL DockModelValidateAndRepairMainLayout(DockModelNode** ppRootNode, DockModelValidateStats* pStats)
{
	DockModelValidateContext context = { 0 };
	if (pStats)
	{
		memset(pStats, 0, sizeof(*pStats));
	}

	if (!ppRootNode || !*ppRootNode)
	{
		if (pStats)
		{
			pStats->nErrors = 1;
		}
		return FALSE;
	}

	(*ppRootNode)->nRole = DOCK_ROLE_ROOT;
	if (!DockModelValidate_ValidateNode(ppRootNode, &context))
	{
		if (pStats)
		{
			*pStats = context.stats;
		}
		return FALSE;
	}

	if (context.nWorkspaceCount <= 0)
	{
		context.stats.nErrors++;
		if (pStats)
		{
			*pStats = context.stats;
		}
		return FALSE;
	}

	if (pStats)
	{
		*pStats = context.stats;
	}

	return TRUE;
}
