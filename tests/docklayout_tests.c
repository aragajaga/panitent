#include <assert.h>
#include <stdio.h>

#include "../src/documentsessionmodel.h"
#include "../src/floatingdocumentsessionmodel.h"
#include "../src/dockfloatingmodel.h"
#include "../src/dockviewcatalog.h"
#include "../src/dockmodelbuild.h"
#include "../src/dockmodel.h"
#include "../src/dockmodelio.h"
#include "../src/dockmodelvalidate.h"
#include "../src/floatingdockpolicy.h"
#include "../src/dockgroup.h"
#include "../src/dockshell.h"
#include "../src/docklayout.h"
#include "../src/dockpolicy.h"
#include "../src/docktypes.h"
#include "../src/workspacedockpolicy.h"

static int test_zone_tab_rect_vertical_starts_from_top(void)
{
	RECT rcClient = { 0, 0, 800, 600 };
	RECT rcTab0 = { 0 };
	RECT rcTab1 = { 0 };

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_LEFT, 0, 3, &rcTab0));
	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_LEFT, 1, 3, &rcTab1));

	assert(rcTab0.left == 0);
	assert(rcTab0.right == DOCKLAYOUT_ZONE_TAB_THICKNESS);
	assert(rcTab0.top == DOCKLAYOUT_ZONE_TAB_INSET);
	assert(rcTab0.bottom == rcTab0.top + DOCKLAYOUT_ZONE_TAB_LENGTH);

	assert(rcTab1.top == DOCKLAYOUT_ZONE_TAB_INSET + DOCKLAYOUT_ZONE_TAB_LENGTH + DOCKLAYOUT_ZONE_TAB_GAP);
	assert(rcTab1.bottom == rcTab1.top + DOCKLAYOUT_ZONE_TAB_LENGTH);

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_RIGHT, 0, 3, &rcTab0));
	assert(rcTab0.left == 800 - DOCKLAYOUT_ZONE_TAB_THICKNESS);
	assert(rcTab0.right == 800);

	return 0;
}

static int test_zone_tab_rect_horizontal_starts_from_left(void)
{
	RECT rcClient = { 0, 0, 800, 600 };
	RECT rcTab0 = { 0 };
	RECT rcTab2 = { 0 };

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_TOP, 0, 3, &rcTab0));
	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_TOP, 2, 3, &rcTab2));

	assert(rcTab0.left == DOCKLAYOUT_ZONE_TAB_INSET);
	assert(rcTab0.right == rcTab0.left + DOCKLAYOUT_ZONE_TAB_LENGTH);
	assert(rcTab0.top == 0);
	assert(rcTab0.bottom == DOCKLAYOUT_ZONE_TAB_THICKNESS);

	assert(rcTab2.left == DOCKLAYOUT_ZONE_TAB_INSET + (DOCKLAYOUT_ZONE_TAB_LENGTH + DOCKLAYOUT_ZONE_TAB_GAP) * 2);
	assert(rcTab2.right == rcTab2.left + DOCKLAYOUT_ZONE_TAB_LENGTH);

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_BOTTOM, 0, 3, &rcTab0));
	assert(rcTab0.top == 600 - DOCKLAYOUT_ZONE_TAB_THICKNESS);
	assert(rcTab0.bottom == 600);

	return 0;
}

static int test_zone_tab_rect_clips_when_outside_client(void)
{
	RECT rcClient = { 0, 0, 150, 100 };
	RECT rcTab = { 0 };

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_TOP, 2, 3, &rcTab));
	assert(rcTab.left == 126);
	assert(rcTab.right == 126);

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_LEFT, 2, 3, &rcTab));
	assert(rcTab.top == 76);
	assert(rcTab.bottom == 76);

	return 0;
}

static int test_zone_tab_rect_by_offset_and_length(void)
{
	RECT rcClient = { 0, 0, 800, 600 };
	RECT rcTab = { 0 };

	assert(DockLayout_GetZoneTabRectByOffset(&rcClient, DKS_TOP, 0, 40, &rcTab));
	assert(rcTab.left == DOCKLAYOUT_ZONE_TAB_INSET);
	assert(rcTab.right == DOCKLAYOUT_ZONE_TAB_INSET + 40);

	assert(DockLayout_GetZoneTabRectByOffset(&rcClient, DKS_TOP, 42, 120, &rcTab));
	assert(rcTab.left == DOCKLAYOUT_ZONE_TAB_INSET + 42);
	assert(rcTab.right == DOCKLAYOUT_ZONE_TAB_INSET + 162);

	assert(DockLayout_GetZoneTabRectByOffset(&rcClient, DKS_LEFT, 30, 77, &rcTab));
	assert(rcTab.top == DOCKLAYOUT_ZONE_TAB_INSET + 30);
	assert(rcTab.bottom == DOCKLAYOUT_ZONE_TAB_INSET + 107);

	rcClient.right = 120;
	rcClient.bottom = 100;
	assert(DockLayout_GetZoneTabRectByOffset(&rcClient, DKS_TOP, 80, 120, &rcTab));
	assert(rcTab.left == 96);
	assert(rcTab.right == 96);

	return 0;
}

static int test_stack_style_and_grips(void)
{
	assert((DockLayout_GetZoneStackStyle(DKS_LEFT) & DGD_VERTICAL) != 0);
	assert((DockLayout_GetZoneStackStyle(DKS_TOP) & DGD_HORIZONTAL) == DGD_HORIZONTAL);

	assert(DockLayout_GetZoneStackGrip(DKS_LEFT, 10) == 220);
	assert(DockLayout_GetZoneStackGrip(DKS_TOP, 0) == 260);
	assert(DockLayout_GetZoneStackGrip(DKS_TOP, 100) == 160);
	assert(DockLayout_GetZoneStackGrip(DKS_TOP, 999) == 520);

	assert(DockLayout_GetZoneSplitGrip(DKS_RIGHT, 20) == 160);
	assert(DockLayout_GetZoneSplitGrip(DKS_RIGHT, 999) == 500);
	assert(DockLayout_GetZoneSplitGrip(DKS_BOTTOM, 20) == 56);
	assert(DockLayout_GetZoneSplitGrip(DKS_BOTTOM, 999) == 360);

	return 0;
}

static int test_split_grip_adjustment(void)
{
	assert(DockLayout_ClampSplitGrip(1000, 10, 96) == 96);
	assert(DockLayout_ClampSplitGrip(1000, 980, 96) == 904);
	assert(DockLayout_ClampSplitGrip(120, 80, 96) == 80);
	assert(DockLayout_ClampSplitGrip(0, 80, 96) == 0);

	assert(DockLayout_AdjustSplitGripFromDelta(DGA_START | DGP_ABSOLUTE | DGD_HORIZONTAL, 220, 40, 1000, 96) == 260);
	assert(DockLayout_AdjustSplitGripFromDelta(DGA_END | DGP_ABSOLUTE | DGD_HORIZONTAL, 220, 40, 1000, 96) == 180);
	assert(DockLayout_AdjustSplitGripFromDelta(DGA_START | DGP_ABSOLUTE | DGD_VERTICAL, 100, -90, 300, 96) == 96);

	return 0;
}

static int test_split_grip_ratio_scaling(void)
{
	float ratio = DockLayout_GetGripRatio(600, 240, 96);
	assert(ratio > 0.39f && ratio < 0.41f);
	assert(DockLayout_ScaleGripFromRatio(900, ratio, 96) == 360);
	assert(DockLayout_ScaleGripFromRatio(100, 0.95f, 96) == 95);
	assert(DockLayout_ScaleGripFromRatio(0, 0.50f, 96) == 0);

	return 0;
}

static int test_invalid_arguments(void)
{
	RECT rc = { 0 };
	assert(!DockLayout_GetZoneTabRect(NULL, DKS_TOP, 0, 1, &rc));
	assert(!DockLayout_GetZoneTabRect(&rc, DKS_TOP, 0, 1, NULL));
	assert(!DockLayout_GetZoneTabRect(&rc, DKS_NONE, 0, 1, &rc));
	assert(!DockLayout_GetDockPreviewRect(NULL, DKS_LEFT, &rc));
	assert(!DockLayout_GetDockPreviewRect(&rc, DKS_LEFT, NULL));
	assert(!DockLayout_GetDockPreviewRect(&rc, DKS_NONE, &rc));
	return 0;
}

static int test_dock_preview_rect_behavior(void)
{
	RECT rcHost = { 10, 20, 810, 620 };
	RECT rc = { 0 };

	assert(DockLayout_GetDockPreviewRect(&rcHost, DKS_LEFT, &rc));
	assert(rc.left == 10);
	assert(rc.top == 20);
	assert(rc.bottom == 620);
	assert(rc.right > rc.left);

	assert(DockLayout_GetDockPreviewRect(&rcHost, DKS_RIGHT, &rc));
	assert(rc.right == 810);
	assert(rc.left < rc.right);

	assert(DockLayout_GetDockPreviewRect(&rcHost, DKS_TOP, &rc));
	assert(rc.top == 20);
	assert(rc.bottom > rc.top);

	assert(DockLayout_GetDockPreviewRect(&rcHost, DKS_BOTTOM, &rc));
	assert(rc.bottom == 620);
	assert(rc.top < rc.bottom);

	return 0;
}

static int test_zone_tab_click_policy(void)
{
	DockPolicyZoneTabClickResult result = { 0 };

	DockPolicy_ResolveZoneTabClick(TRUE, TRUE, FALSE, &result);
	assert(result.bCollapsed == TRUE);
	assert(result.bActivateClickedTab == FALSE);

	DockPolicy_ResolveZoneTabClick(TRUE, TRUE, TRUE, &result);
	assert(result.bCollapsed == FALSE);
	assert(result.bActivateClickedTab == FALSE);

	DockPolicy_ResolveZoneTabClick(TRUE, FALSE, TRUE, &result);
	assert(result.bCollapsed == FALSE);
	assert(result.bActivateClickedTab == TRUE);

	DockPolicy_ResolveZoneTabClick(FALSE, FALSE, FALSE, &result);
	assert(result.bCollapsed == TRUE);
	assert(result.bActivateClickedTab == FALSE);

	return 0;
}

static int test_core_panel_lock_policy(void)
{
	assert(!DockPolicy_CanUndockPanelName(L"WorkspaceContainer"));
	assert(!DockPolicy_CanClosePanelName(L"WorkspaceContainer"));
	assert(!DockPolicy_CanPinPanelName(L"WorkspaceContainer"));
	assert(!DockPolicy_CanUndockPanelName(L"Root"));
	assert(!DockPolicy_CanUndockPanelName(L"DockZone.Left"));
	assert(!DockPolicy_CanClosePanelName(L"DockShell.Root"));
	assert(!DockPolicy_CanPinPanelName(L"DockShell.Root"));

	assert(DockPolicy_CanUndockPanelName(L"Palette"));
	assert(DockPolicy_CanClosePanelName(L"Layers"));
	assert(DockPolicy_CanPinPanelName(L"Layers"));
	assert(DockPolicy_CanUndockPanelName(NULL));
	assert(DockPolicy_CanPinPanelName(NULL));

	return 0;
}

static int test_explicit_dock_roles_override_name_fallback(void)
{
	assert(!DockPolicy_CanUndockPanel(DOCK_ROLE_WORKSPACE, L"Palette"));
	assert(!DockPolicy_CanClosePanel(DOCK_ROLE_ZONE, L"Layers"));
	assert(!DockPolicy_CanPinPanel(DOCK_ROLE_PANEL_SPLIT, L"Toolbox"));
	assert(DockPolicy_CanClosePanel(DOCK_ROLE_PANEL, L"WorkspaceContainer"));
	assert(DockNodeRole_IsStructural(DOCK_ROLE_ZONE_STACK_SPLIT, NULL));
	assert(DockNodeRole_UsesProportionalGrip(DOCK_ROLE_PANEL_SPLIT, NULL));

	return 0;
}

static int test_dock_shell_zone_builder_assigns_role_and_side(void)
{
	TreeNode* pZoneNode = DockShell_CreateZoneNode(DKS_RIGHT);
	DockData* pZoneData = pZoneNode ? (DockData*)pZoneNode->data : NULL;

	assert(pZoneNode);
	assert(pZoneData);
	assert(pZoneData->nRole == DOCK_ROLE_ZONE);
	assert(pZoneData->nDockSide == DKS_RIGHT);
	assert(wcscmp(pZoneData->lpszName, L"DockZone.Right") == 0);

	return 0;
}

static int test_dock_shell_appends_zone_stack_split(void)
{
	TreeNode* pZoneNode = DockShell_CreateZoneNode(DKS_LEFT);
	TreeNode* pPanelA = DockShell_CreatePanelNode(L"Toolbox");
	TreeNode* pPanelB = DockShell_CreatePanelNode(L"Palette");
	DockData* pSplitData;

	assert(DockShell_AppendPanelToZone(pZoneNode, pPanelA));
	assert(pZoneNode->node1 == pPanelA);

	assert(DockShell_AppendPanelToZone(pZoneNode, pPanelB));
	assert(pZoneNode->node1 != pPanelA);
	assert(pZoneNode->node1 != pPanelB);

	pSplitData = (DockData*)pZoneNode->node1->data;
	assert(pSplitData);
	assert(pSplitData->nRole == DOCK_ROLE_ZONE_STACK_SPLIT);
	assert(pZoneNode->node1->node1 == pPanelA);
	assert(pZoneNode->node1->node2 == pPanelB);

	return 0;
}

static int test_dock_shell_build_main_layout_attaches_workspace_and_zones(void)
{
	TreeNode* pRoot = DockShell_CreateRootNode();
	TreeNode* pWorkspace = DockShell_CreateWorkspaceNode();
	TreeNode* pZoneLeft = DockShell_CreateZoneNode(DKS_LEFT);
	TreeNode* pZoneRight = DockShell_CreateZoneNode(DKS_RIGHT);
	TreeNode* pZoneTop = DockShell_CreateZoneNode(DKS_TOP);
	TreeNode* pZoneBottom = DockShell_CreateZoneNode(DKS_BOTTOM);
	DockShellMetrics metrics = { 111, 222, 33, 44 };
	DockData* pShellRootData;

	assert(DockShell_BuildMainLayout(
		pRoot,
		pWorkspace,
		pZoneLeft,
		pZoneRight,
		pZoneTop,
		pZoneBottom,
		&metrics));
	assert(pRoot->node1);
	assert(!pRoot->node2);

	pShellRootData = (DockData*)pRoot->node1->data;
	assert(pShellRootData);
	assert(pShellRootData->nRole == DOCK_ROLE_SHELL_SPLIT);
	assert(wcscmp(pShellRootData->lpszName, L"DockShell.Root") == 0);
	assert(pShellRootData->iGripPos == 44);

	return 0;
}

static int test_dock_group_semantics_for_tool_and_document_panes(void)
{
	TreeNode* pRoot = DockShell_CreateRootNode();
	TreeNode* pZone = DockShell_CreateZoneNode(DKS_LEFT);
	TreeNode* pToolPanelA = DockShell_CreatePanelNode(L"Toolbox");
	TreeNode* pToolPanelB = DockShell_CreatePanelNode(L"Palette");
	TreeNode* pWorkspace = DockShell_CreateWorkspaceNode();

	assert(DockShell_AppendPanelToZone(pZone, pToolPanelA));
	assert(DockShell_AppendPanelToZone(pZone, pToolPanelB));

	pRoot->node1 = pZone;
	pRoot->node2 = pWorkspace;

	assert(DockGroup_GetNodeKind(pZone) == DOCK_GROUP_TOOL_PANE);
	assert(DockGroup_GetNodeKind(pWorkspace) == DOCK_GROUP_DOCUMENT_PANE);
	assert(DockGroup_GetNodePaneKind(pToolPanelA) == DOCK_PANE_TOOL);
	assert(DockGroup_GetNodePaneKind(pWorkspace) == DOCK_PANE_DOCUMENT);
	assert(DockGroup_NodeContainsPaneKind(pRoot, DOCK_PANE_DOCUMENT));
	assert(DockGroup_FindOwningGroup(pRoot, pToolPanelA) == pZone);
	assert(DockGroup_FindOwningGroup(pRoot, pWorkspace) == pWorkspace);

	assert(DockGroup_CanTabIntoGroup(DOCK_GROUP_TOOL_PANE, DOCK_PANE_TOOL));
	assert(!DockGroup_CanTabIntoGroup(DOCK_GROUP_TOOL_PANE, DOCK_PANE_DOCUMENT));
	assert(DockGroup_CanSplitGroup(DOCK_GROUP_DOCUMENT_PANE, DOCK_PANE_DOCUMENT));
	assert(!DockGroup_CanSplitGroup(DOCK_GROUP_DOCUMENT_PANE, DOCK_PANE_TOOL));

	return 0;
}

static int test_floating_dock_policy_semantics(void)
{
	assert(FloatingDockPolicy_GetPaneKind(FLOAT_DOCK_POLICY_PANEL) == DOCK_PANE_TOOL);
	assert(FloatingDockPolicy_GetPaneKind(FLOAT_DOCK_POLICY_DOCUMENT) == DOCK_PANE_DOCUMENT);
	assert(FloatingDockPolicy_UsesDocumentFlow(FLOAT_DOCK_POLICY_DOCUMENT));
	assert(!FloatingDockPolicy_UsesDocumentFlow(FLOAT_DOCK_POLICY_PANEL));

	assert(FloatingDockPolicy_CanShowDockCommand(
		FLOAT_DOCK_POLICY_DOCUMENT,
		FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE,
		FALSE));
	assert(FloatingDockPolicy_CanShowDockCommand(
		FLOAT_DOCK_POLICY_DOCUMENT,
		FLOAT_DOCK_CHILD_DOCUMENT_HOST,
		FALSE));
	assert(!FloatingDockPolicy_CanShowDockCommand(
		FLOAT_DOCK_POLICY_DOCUMENT,
		FLOAT_DOCK_CHILD_TOOL_PANEL,
		TRUE));

	assert(FloatingDockPolicy_CanUseHostDockTarget(
		FLOAT_DOCK_POLICY_PANEL,
		FLOAT_DOCK_CHILD_TOOL_PANEL,
		TRUE));
	assert(FloatingDockPolicy_CanUseHostDockTarget(
		FLOAT_DOCK_POLICY_PANEL,
		FLOAT_DOCK_CHILD_TOOL_HOST,
		TRUE));
	assert(!FloatingDockPolicy_CanUseHostDockTarget(
		FLOAT_DOCK_POLICY_PANEL,
		FLOAT_DOCK_CHILD_DOCUMENT_HOST,
		TRUE));

	assert(FloatingDockPolicy_CanMoveDocumentsToWorkspace(FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE));
	assert(FloatingDockPolicy_CanMoveDocumentsToWorkspace(FLOAT_DOCK_CHILD_DOCUMENT_HOST));
	assert(!FloatingDockPolicy_CanMoveDocumentsToWorkspace(FLOAT_DOCK_CHILD_TOOL_PANEL));
	assert(FloatingDockPolicy_RequiresWorkspaceMergeForSideDock(FLOAT_DOCK_CHILD_DOCUMENT_HOST));
	assert(!FloatingDockPolicy_RequiresWorkspaceMergeForSideDock(FLOAT_DOCK_CHILD_DOCUMENT_WORKSPACE));

	return 0;
}

static int test_dock_view_catalog_maps_known_persistent_views(void)
{
	assert(PanitentDockViewCatalog_Find(DOCK_ROLE_WORKSPACE, L"WorkspaceContainer") == PNT_DOCK_VIEW_WORKSPACE);
	assert(PanitentDockViewCatalog_Find(DOCK_ROLE_PANEL, L"Toolbox") == PNT_DOCK_VIEW_TOOLBOX);
	assert(PanitentDockViewCatalog_Find(DOCK_ROLE_PANEL, L"GLWindow") == PNT_DOCK_VIEW_GLWINDOW);
	assert(PanitentDockViewCatalog_Find(DOCK_ROLE_PANEL, L"Palette") == PNT_DOCK_VIEW_PALETTE);
	assert(PanitentDockViewCatalog_Find(DOCK_ROLE_PANEL, L"Layers") == PNT_DOCK_VIEW_LAYERS);
	assert(PanitentDockViewCatalog_Find(DOCK_ROLE_PANEL, L"Option Bar") == PNT_DOCK_VIEW_OPTIONBAR);
	assert(PanitentDockViewCatalog_FindForWindow(L"__ToolboxWindow", L"Toolbox") == PNT_DOCK_VIEW_TOOLBOX);
	assert(PanitentDockViewCatalog_FindForWindow(L"__LayersWindow", L"LayersWindow") == PNT_DOCK_VIEW_LAYERS);
	assert(PanitentDockViewCatalog_FindForWindow(L"__OptionBarWindow", L"Option Bar") == PNT_DOCK_VIEW_OPTIONBAR);
	assert(PanitentDockViewCatalog_IsKnown(DOCK_ROLE_PANEL, L"Toolbox"));
	assert(!PanitentDockViewCatalog_IsKnown(DOCK_ROLE_PANEL, L"Unknown Panel"));
	assert(!PanitentDockViewCatalog_IsKnown(DOCK_ROLE_ZONE, L"DockZone.Left"));

	return 0;
}

static int test_dock_floating_layout_file_round_trip(void)
{
	WCHAR szTempPath[MAX_PATH] = L"";
	WCHAR szTempFile[MAX_PATH] = L"";
	DockFloatingLayoutFileModel model = { 0 };
	DockFloatingLayoutFileModel loaded = { 0 };
	DockModelNode layoutRoot = { 0 };
	DockModelNode layoutPanel = { 0 };

	assert(GetTempPathW(ARRAYSIZE(szTempPath), szTempPath) > 0);
	assert(GetTempFileNameW(szTempPath, L"dfl", 0, szTempFile) != 0);

	model.nEntries = 2;
	SetRect(&model.entries[0].rcWindow, 10, 20, 210, 320);
	model.entries[0].iDockSizeHint = 240;
	model.entries[0].nChildKind = FLOAT_DOCK_CHILD_TOOL_PANEL;
	model.entries[0].nViewId = PNT_DOCK_VIEW_TOOLBOX;
	SetRect(&model.entries[1].rcWindow, 400, 80, 620, 360);
	model.entries[1].iDockSizeHint = 260;
	model.entries[1].nChildKind = FLOAT_DOCK_CHILD_TOOL_HOST;
	layoutRoot.nRole = DOCK_ROLE_ROOT;
	wcscpy_s(layoutRoot.szName, ARRAYSIZE(layoutRoot.szName), L"Root");
	layoutRoot.pChild1 = &layoutPanel;
	layoutPanel.nRole = DOCK_ROLE_PANEL;
	layoutPanel.nPaneKind = DOCK_PANE_TOOL;
	wcscpy_s(layoutPanel.szName, ARRAYSIZE(layoutPanel.szName), L"Palette");
	wcscpy_s(layoutPanel.szCaption, ARRAYSIZE(layoutPanel.szCaption), L"Palette");
	model.entries[1].pLayoutModel = &layoutRoot;

	assert(DockFloatingLayout_SaveToFile(&model, szTempFile));
	assert(DockFloatingLayout_LoadFromFile(szTempFile, &loaded));
	assert(loaded.nEntries == 2);
	assert(loaded.entries[0].nChildKind == FLOAT_DOCK_CHILD_TOOL_PANEL);
	assert(loaded.entries[0].nViewId == model.entries[0].nViewId);
	assert(EqualRect(&loaded.entries[0].rcWindow, &model.entries[0].rcWindow));
	assert(loaded.entries[0].iDockSizeHint == model.entries[0].iDockSizeHint);
	assert(loaded.entries[1].nChildKind == FLOAT_DOCK_CHILD_TOOL_HOST);
	assert(EqualRect(&loaded.entries[1].rcWindow, &model.entries[1].rcWindow));
	assert(loaded.entries[1].iDockSizeHint == model.entries[1].iDockSizeHint);
	assert(loaded.entries[1].pLayoutModel != NULL);
	assert(loaded.entries[1].pLayoutModel->nRole == DOCK_ROLE_ROOT);
	assert(loaded.entries[1].pLayoutModel->pChild1 != NULL);
	assert(loaded.entries[1].pLayoutModel->pChild1->nRole == DOCK_ROLE_PANEL);
	assert(wcscmp(loaded.entries[1].pLayoutModel->pChild1->szName, L"Palette") == 0);

	DockFloatingLayout_Destroy(&loaded);
	DeleteFileW(szTempFile);
	return 0;
}

static int test_document_session_model_file_round_trip(void)
{
	WCHAR szTempPath[MAX_PATH] = L"";
	WCHAR szTempFile[MAX_PATH] = L"";
	DocumentSessionModel model = { 0 };
	DocumentSessionModel loaded = { 0 };

	assert(GetTempPathW(ARRAYSIZE(szTempPath), szTempPath) > 0);
	assert(GetTempFileNameW(szTempPath, L"dds", 0, szTempFile) != 0);

	model.nEntryCount = 2;
	model.nActiveEntry = 1;
	wcscpy_s(model.entries[0].szFilePath, ARRAYSIZE(model.entries[0].szFilePath), L"C:\\test\\a.png");
	wcscpy_s(model.entries[1].szFilePath, ARRAYSIZE(model.entries[1].szFilePath), L"C:\\test\\b.png");

	assert(DocumentSessionModel_SaveToFile(&model, szTempFile));
	assert(DocumentSessionModel_LoadFromFile(szTempFile, &loaded));
	assert(loaded.nEntryCount == 2);
	assert(loaded.nActiveEntry == 1);
	assert(wcscmp(loaded.entries[0].szFilePath, model.entries[0].szFilePath) == 0);
	assert(wcscmp(loaded.entries[1].szFilePath, model.entries[1].szFilePath) == 0);

	DeleteFileW(szTempFile);
	return 0;
}

static int test_floating_document_session_model_file_round_trip(void)
{
	WCHAR szTempPath[MAX_PATH] = L"";
	WCHAR szTempFile[MAX_PATH] = L"";
	FloatingDocumentSessionModel* pModel = (FloatingDocumentSessionModel*)calloc(1, sizeof(FloatingDocumentSessionModel));
	FloatingDocumentSessionModel* pLoaded = (FloatingDocumentSessionModel*)calloc(1, sizeof(FloatingDocumentSessionModel));
	DockModelNode layoutRoot = { 0 };
	DockModelNode layoutWorkspace = { 0 };
	assert(pModel);
	assert(pLoaded);

	assert(GetTempPathW(ARRAYSIZE(szTempPath), szTempPath) > 0);
	assert(GetTempFileNameW(szTempPath, L"fds", 0, szTempFile) != 0);

	pModel->nEntryCount = 1;
	SetRect(&pModel->entries[0].rcWindow, 100, 120, 640, 480);
	layoutRoot.nRole = DOCK_ROLE_ROOT;
	wcscpy_s(layoutRoot.szName, ARRAYSIZE(layoutRoot.szName), L"Root");
	layoutRoot.pChild1 = &layoutWorkspace;
	layoutWorkspace.nRole = DOCK_ROLE_WORKSPACE;
	layoutWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
	wcscpy_s(layoutWorkspace.szName, ARRAYSIZE(layoutWorkspace.szName), L"WorkspaceContainer");
	pModel->entries[0].pLayoutModel = &layoutRoot;
	pModel->entries[0].nWorkspaceCount = 1;
	pModel->entries[0].workspaces[0].nActiveEntry = 1;
	pModel->entries[0].workspaces[0].nFileCount = 2;
	wcscpy_s(pModel->entries[0].workspaces[0].szFilePaths[0], ARRAYSIZE(pModel->entries[0].workspaces[0].szFilePaths[0]), L"C:\\test\\doc1.png");
	wcscpy_s(pModel->entries[0].workspaces[0].szFilePaths[1], ARRAYSIZE(pModel->entries[0].workspaces[0].szFilePaths[1]), L"C:\\test\\doc2.png");

	assert(FloatingDocumentSessionModel_SaveToFile(pModel, szTempFile));
	assert(FloatingDocumentSessionModel_LoadFromFile(szTempFile, pLoaded));
	assert(pLoaded->nEntryCount == 1);
	assert(EqualRect(&pLoaded->entries[0].rcWindow, &pModel->entries[0].rcWindow));
	assert(pLoaded->entries[0].pLayoutModel != NULL);
	assert(pLoaded->entries[0].pLayoutModel->nRole == DOCK_ROLE_ROOT);
	assert(pLoaded->entries[0].pLayoutModel->pChild1 != NULL);
	assert(pLoaded->entries[0].pLayoutModel->pChild1->nRole == DOCK_ROLE_WORKSPACE);
	assert(pLoaded->entries[0].nWorkspaceCount == 1);
	assert(pLoaded->entries[0].workspaces[0].nActiveEntry == 1);
	assert(pLoaded->entries[0].workspaces[0].nFileCount == 2);
	assert(wcscmp(pLoaded->entries[0].workspaces[0].szFilePaths[0], pModel->entries[0].workspaces[0].szFilePaths[0]) == 0);
	assert(wcscmp(pLoaded->entries[0].workspaces[0].szFilePaths[1], pModel->entries[0].workspaces[0].szFilePaths[1]) == 0);

	FloatingDocumentSessionModel_Destroy(pLoaded);
	free(pLoaded);
	free(pModel);
	DeleteFileW(szTempFile);
	return 0;
}

static int test_dock_model_capture_strips_runtime_handles_but_keeps_semantics(void)
{
	TreeNode* pRoot = DockShell_CreateRootNode();
	TreeNode* pZone = DockShell_CreateZoneNode(DKS_LEFT);
	TreeNode* pPanelA = DockShell_CreatePanelNode(L"Toolbox");
	TreeNode* pPanelB = DockShell_CreatePanelNode(L"Palette");
	TreeNode* pWorkspace = DockShell_CreateWorkspaceNode();
	DockData* pZoneData = (DockData*)pZone->data;
	DockData* pPanelAData = (DockData*)pPanelA->data;
	DockData* pPanelBData = (DockData*)pPanelB->data;
	DockModelNode* pModel;

	assert(DockShell_AppendPanelToZone(pZone, pPanelA));
	assert(DockShell_AppendPanelToZone(pZone, pPanelB));
	pRoot->node1 = pZone;
	pRoot->node2 = pWorkspace;

	pPanelAData->hWnd = (HWND)(INT_PTR)0x1001;
	pPanelBData->hWnd = (HWND)(INT_PTR)0x1002;
	pZoneData->hWndActiveTab = pPanelBData->hWnd;
	pZoneData->bCollapsed = TRUE;

	pModel = DockModel_CaptureTree(pRoot);
	assert(pModel);
	assert(pModel->nRole == DOCK_ROLE_ROOT);
	assert(pModel->nPaneKind == DOCK_PANE_NONE);
	assert(pModel->pChild1);
	assert(pModel->pChild2);
	assert(pModel->pChild1->nRole == DOCK_ROLE_ZONE);
	assert(pModel->pChild1->nDockSide == DKS_LEFT);
	assert(pModel->pChild1->bCollapsed == TRUE);
	assert(wcscmp(pModel->pChild1->szActiveTabName, L"Palette") == 0);
	assert(pModel->pChild1->pChild1);
	assert(pModel->pChild1->pChild1->nRole == DOCK_ROLE_ZONE_STACK_SPLIT);
	assert(pModel->pChild1->pChild1->pChild1);
	assert(pModel->pChild1->pChild1->pChild2);
	assert(pModel->pChild2->nRole == DOCK_ROLE_WORKSPACE);
	assert(pModel->pChild2->nPaneKind == DOCK_PANE_DOCUMENT);

	DockModel_Destroy(pModel);
	return 0;
}

static void assert_dock_model_equal(const DockModelNode* pActual, const DockModelNode* pExpected)
{
	assert((pActual == NULL) == (pExpected == NULL));
	if (!pActual || !pExpected)
	{
		return;
	}

	assert(pActual->nRole == pExpected->nRole);
	assert(pActual->nPaneKind == pExpected->nPaneKind);
	assert(pActual->nDockSide == pExpected->nDockSide);
	assert(pActual->dwStyle == pExpected->dwStyle);
	assert(pActual->fGripPos == pExpected->fGripPos);
	assert(pActual->iGripPos == pExpected->iGripPos);
	assert(pActual->bShowCaption == pExpected->bShowCaption);
	assert(pActual->bCollapsed == pExpected->bCollapsed);
	assert(wcscmp(pActual->szName, pExpected->szName) == 0);
	assert(wcscmp(pActual->szCaption, pExpected->szCaption) == 0);
	assert(wcscmp(pActual->szActiveTabName, pExpected->szActiveTabName) == 0);
	assert_dock_model_equal(pActual->pChild1, pExpected->pChild1);
	assert_dock_model_equal(pActual->pChild2, pExpected->pChild2);
}

static int test_dock_model_file_round_trip(void)
{
	WCHAR szTempPath[MAX_PATH] = L"";
	WCHAR szTempFile[MAX_PATH] = L"";
	DockModelNode modelRoot = { 0 };
	DockModelNode modelZone = { 0 };
	DockModelNode modelSplit = { 0 };
	DockModelNode modelPanelA = { 0 };
	DockModelNode modelPanelB = { 0 };
	DockModelNode modelWorkspace = { 0 };
	DockModelNode* pLoadedModel = NULL;

	assert(GetTempPathW(ARRAYSIZE(szTempPath), szTempPath) > 0);
	assert(GetTempFileNameW(szTempPath, L"dmd", 0, szTempFile) != 0);

	modelRoot.nRole = DOCK_ROLE_ROOT;
	wcscpy_s(modelRoot.szName, ARRAYSIZE(modelRoot.szName), L"Root");
	modelRoot.pChild1 = &modelZone;
	modelRoot.pChild2 = &modelWorkspace;

	modelZone.nRole = DOCK_ROLE_ZONE;
	modelZone.nDockSide = DKS_LEFT;
	modelZone.bCollapsed = TRUE;
	wcscpy_s(modelZone.szName, ARRAYSIZE(modelZone.szName), L"DockZone.Left");
	wcscpy_s(modelZone.szActiveTabName, ARRAYSIZE(modelZone.szActiveTabName), L"Palette");
	modelZone.pChild1 = &modelSplit;

	modelSplit.nRole = DOCK_ROLE_ZONE_STACK_SPLIT;
	modelSplit.dwStyle = DGA_END | DGP_ABSOLUTE | DGD_VERTICAL;
	modelSplit.fGripPos = 0.5f;
	modelSplit.iGripPos = 220;
	wcscpy_s(modelSplit.szName, ARRAYSIZE(modelSplit.szName), L"DockShell.ZoneStack");
	modelSplit.pChild1 = &modelPanelA;
	modelSplit.pChild2 = &modelPanelB;

	modelPanelA.nRole = DOCK_ROLE_PANEL;
	modelPanelA.nPaneKind = DOCK_PANE_TOOL;
	modelPanelA.bShowCaption = TRUE;
	wcscpy_s(modelPanelA.szName, ARRAYSIZE(modelPanelA.szName), L"Toolbox");
	wcscpy_s(modelPanelA.szCaption, ARRAYSIZE(modelPanelA.szCaption), L"Toolbox");

	modelPanelB.nRole = DOCK_ROLE_PANEL;
	modelPanelB.nPaneKind = DOCK_PANE_TOOL;
	modelPanelB.bShowCaption = TRUE;
	wcscpy_s(modelPanelB.szName, ARRAYSIZE(modelPanelB.szName), L"Palette");
	wcscpy_s(modelPanelB.szCaption, ARRAYSIZE(modelPanelB.szCaption), L"Palette");

	modelWorkspace.nRole = DOCK_ROLE_WORKSPACE;
	modelWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
	wcscpy_s(modelWorkspace.szName, ARRAYSIZE(modelWorkspace.szName), L"WorkspaceContainer");

	assert(DockModelIO_SaveToFile(&modelRoot, szTempFile));
	pLoadedModel = DockModelIO_LoadFromFile(szTempFile);
	assert(pLoadedModel);
	assert_dock_model_equal(pLoadedModel, &modelRoot);

	DockModel_Destroy(pLoadedModel);
	DeleteFileW(szTempFile);
	return 0;
}

static int test_dock_model_build_tree_round_trip(void)
{
	DockModelNode modelRoot = { 0 };
	DockModelNode modelZone = { 0 };
	DockModelNode modelSplit = { 0 };
	DockModelNode modelPanelA = { 0 };
	DockModelNode modelPanelB = { 0 };
	DockModelNode modelWorkspace = { 0 };
	TreeNode* pBuiltTree = NULL;
	DockModelNode* pCapturedModel = NULL;

	modelRoot.nRole = DOCK_ROLE_ROOT;
	wcscpy_s(modelRoot.szName, ARRAYSIZE(modelRoot.szName), L"Root");
	modelRoot.pChild1 = &modelZone;
	modelRoot.pChild2 = &modelWorkspace;

	modelZone.nRole = DOCK_ROLE_ZONE;
	modelZone.nDockSide = DKS_LEFT;
	modelZone.bCollapsed = TRUE;
	wcscpy_s(modelZone.szName, ARRAYSIZE(modelZone.szName), L"DockZone.Left");
	wcscpy_s(modelZone.szActiveTabName, ARRAYSIZE(modelZone.szActiveTabName), L"Palette");
	modelZone.pChild1 = &modelSplit;

	modelSplit.nRole = DOCK_ROLE_ZONE_STACK_SPLIT;
	modelSplit.dwStyle = DGA_END | DGP_ABSOLUTE | DGD_VERTICAL;
	modelSplit.fGripPos = 0.5f;
	modelSplit.iGripPos = 220;
	wcscpy_s(modelSplit.szName, ARRAYSIZE(modelSplit.szName), L"DockShell.ZoneStack");
	modelSplit.pChild1 = &modelPanelA;
	modelSplit.pChild2 = &modelPanelB;

	modelPanelA.nRole = DOCK_ROLE_PANEL;
	modelPanelA.nPaneKind = DOCK_PANE_TOOL;
	modelPanelA.bShowCaption = TRUE;
	wcscpy_s(modelPanelA.szName, ARRAYSIZE(modelPanelA.szName), L"Toolbox");
	wcscpy_s(modelPanelA.szCaption, ARRAYSIZE(modelPanelA.szCaption), L"Toolbox");

	modelPanelB.nRole = DOCK_ROLE_PANEL;
	modelPanelB.nPaneKind = DOCK_PANE_TOOL;
	modelPanelB.bShowCaption = TRUE;
	wcscpy_s(modelPanelB.szName, ARRAYSIZE(modelPanelB.szName), L"Palette");
	wcscpy_s(modelPanelB.szCaption, ARRAYSIZE(modelPanelB.szCaption), L"Palette");

	modelWorkspace.nRole = DOCK_ROLE_WORKSPACE;
	modelWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
	wcscpy_s(modelWorkspace.szName, ARRAYSIZE(modelWorkspace.szName), L"WorkspaceContainer");

	pBuiltTree = DockModelBuildTree(&modelRoot);
	assert(pBuiltTree);

	pCapturedModel = DockModel_CaptureTree(pBuiltTree);
	assert(pCapturedModel);
	assert_dock_model_equal(pCapturedModel, &modelRoot);

	DockModel_Destroy(pCapturedModel);
	DockModelBuildDestroyTree(pBuiltTree);
	return 0;
}

static int test_dock_model_full_pipeline_round_trip(void)
{
	WCHAR szTempPath[MAX_PATH] = L"";
	WCHAR szTempFile[MAX_PATH] = L"";
	DockModelNode modelRoot = { 0 };
	DockModelNode modelZone = { 0 };
	DockModelNode modelPanel = { 0 };
	DockModelNode modelWorkspace = { 0 };
	DockModelNode* pLoadedModel = NULL;
	TreeNode* pBuiltTree = NULL;
	DockModelNode* pCapturedModel = NULL;

	assert(GetTempPathW(ARRAYSIZE(szTempPath), szTempPath) > 0);
	assert(GetTempFileNameW(szTempPath, L"dmp", 0, szTempFile) != 0);

	modelRoot.nRole = DOCK_ROLE_ROOT;
	wcscpy_s(modelRoot.szName, ARRAYSIZE(modelRoot.szName), L"Root");
	modelRoot.pChild1 = &modelZone;
	modelRoot.pChild2 = &modelWorkspace;

	modelZone.nRole = DOCK_ROLE_ZONE;
	modelZone.nDockSide = DKS_RIGHT;
	wcscpy_s(modelZone.szName, ARRAYSIZE(modelZone.szName), L"DockZone.Right");
	wcscpy_s(modelZone.szActiveTabName, ARRAYSIZE(modelZone.szActiveTabName), L"GLWindow");
	modelZone.pChild1 = &modelPanel;

	modelPanel.nRole = DOCK_ROLE_PANEL;
	modelPanel.nPaneKind = DOCK_PANE_TOOL;
	modelPanel.bShowCaption = TRUE;
	wcscpy_s(modelPanel.szName, ARRAYSIZE(modelPanel.szName), L"GLWindow");
	wcscpy_s(modelPanel.szCaption, ARRAYSIZE(modelPanel.szCaption), L"GLWindow");

	modelWorkspace.nRole = DOCK_ROLE_WORKSPACE;
	modelWorkspace.nPaneKind = DOCK_PANE_DOCUMENT;
	wcscpy_s(modelWorkspace.szName, ARRAYSIZE(modelWorkspace.szName), L"WorkspaceContainer");

	assert(DockModelIO_SaveToFile(&modelRoot, szTempFile));
	pLoadedModel = DockModelIO_LoadFromFile(szTempFile);
	assert(pLoadedModel);

	pBuiltTree = DockModelBuildTree(pLoadedModel);
	assert(pBuiltTree);
	pCapturedModel = DockModel_CaptureTree(pBuiltTree);
	assert(pCapturedModel);
	assert_dock_model_equal(pCapturedModel, &modelRoot);

	DockModel_Destroy(pLoadedModel);
	DockModel_Destroy(pCapturedModel);
	DockModelBuildDestroyTree(pBuiltTree);
	DeleteFileW(szTempFile);
	return 0;
}

static int test_dock_model_validate_repairs_metadata_and_active_tab(void)
{
	DockModelNode modelRoot = { 0 };
	DockModelNode modelZone = { 0 };
	DockModelNode modelPanel = { 0 };
	DockModelNode modelWorkspace = { 0 };
	DockModelNode* pRootNode = &modelRoot;
	DockModelValidateStats stats = { 0 };

	modelRoot.nRole = DOCK_ROLE_PANEL;
	wcscpy_s(modelRoot.szName, ARRAYSIZE(modelRoot.szName), L"WrongRoot");
	modelRoot.pChild1 = &modelZone;
	modelRoot.pChild2 = &modelWorkspace;

	modelZone.nRole = DOCK_ROLE_ZONE;
	modelZone.nDockSide = DKS_LEFT;
	wcscpy_s(modelZone.szName, ARRAYSIZE(modelZone.szName), L"BrokenZone");
	wcscpy_s(modelZone.szActiveTabName, ARRAYSIZE(modelZone.szActiveTabName), L"MissingTab");
	modelZone.pChild1 = &modelPanel;

	modelPanel.nRole = DOCK_ROLE_PANEL;
	wcscpy_s(modelPanel.szName, ARRAYSIZE(modelPanel.szName), L"Toolbox");

	modelWorkspace.nRole = DOCK_ROLE_WORKSPACE;
	modelWorkspace.nPaneKind = DOCK_PANE_TOOL;
	modelWorkspace.bShowCaption = TRUE;
	wcscpy_s(modelWorkspace.szName, ARRAYSIZE(modelWorkspace.szName), L"WrongWorkspace");

	assert(DockModelValidateAndRepairMainLayout(&pRootNode, &stats));
	assert(modelRoot.nRole == DOCK_ROLE_ROOT);
	assert(wcscmp(modelRoot.szName, L"Root") == 0);
	assert(wcscmp(modelZone.szName, L"DockZone.Left") == 0);
	assert(wcscmp(modelZone.szActiveTabName, L"Toolbox") == 0);
	assert(modelPanel.nPaneKind == DOCK_PANE_TOOL);
	assert(wcscmp(modelPanel.szCaption, L"Toolbox") == 0);
	assert(modelWorkspace.nPaneKind == DOCK_PANE_DOCUMENT);
	assert(modelWorkspace.bShowCaption == FALSE);
	assert(wcscmp(modelWorkspace.szName, L"WorkspaceContainer") == 0);
	assert(stats.nRepairs > 0);

	return 0;
}

static int test_dock_model_validate_rejects_unknown_or_duplicate_views(void)
{
	DockModelNode rootUnknown = { 0 };
	DockModelNode zoneUnknown = { 0 };
	DockModelNode panelUnknown = { 0 };
	DockModelNode workspaceUnknown = { 0 };
	DockModelNode* pUnknownRoot = &rootUnknown;

	rootUnknown.nRole = DOCK_ROLE_ROOT;
	rootUnknown.pChild1 = &zoneUnknown;
	rootUnknown.pChild2 = &workspaceUnknown;
	zoneUnknown.nRole = DOCK_ROLE_ZONE;
	zoneUnknown.nDockSide = DKS_LEFT;
	zoneUnknown.pChild1 = &panelUnknown;
	panelUnknown.nRole = DOCK_ROLE_PANEL;
	wcscpy_s(panelUnknown.szName, ARRAYSIZE(panelUnknown.szName), L"Unknown Panel");
	workspaceUnknown.nRole = DOCK_ROLE_WORKSPACE;
	wcscpy_s(workspaceUnknown.szName, ARRAYSIZE(workspaceUnknown.szName), L"WorkspaceContainer");

	assert(!DockModelValidateAndRepairMainLayout(&pUnknownRoot, NULL));

	DockModelNode rootDup = { 0 };
	DockModelNode zoneDup = { 0 };
	DockModelNode splitDup = { 0 };
	DockModelNode panelDupA = { 0 };
	DockModelNode panelDupB = { 0 };
	DockModelNode workspaceDup = { 0 };
	DockModelNode* pDupRoot = &rootDup;

	rootDup.nRole = DOCK_ROLE_ROOT;
	rootDup.pChild1 = &zoneDup;
	rootDup.pChild2 = &workspaceDup;
	zoneDup.nRole = DOCK_ROLE_ZONE;
	zoneDup.nDockSide = DKS_LEFT;
	zoneDup.pChild1 = &splitDup;
	splitDup.nRole = DOCK_ROLE_ZONE_STACK_SPLIT;
	splitDup.pChild1 = &panelDupA;
	splitDup.pChild2 = &panelDupB;
	panelDupA.nRole = DOCK_ROLE_PANEL;
	panelDupB.nRole = DOCK_ROLE_PANEL;
	wcscpy_s(panelDupA.szName, ARRAYSIZE(panelDupA.szName), L"Toolbox");
	wcscpy_s(panelDupB.szName, ARRAYSIZE(panelDupB.szName), L"Toolbox");
	workspaceDup.nRole = DOCK_ROLE_WORKSPACE;
	wcscpy_s(workspaceDup.szName, ARRAYSIZE(workspaceDup.szName), L"WorkspaceContainer");

	assert(!DockModelValidateAndRepairMainLayout(&pDupRoot, NULL));

	return 0;
}

static int test_workspace_document_dock_split_policy(void)
{
	/* Single detached tab returning into its empty origin group: center-only. */
	assert(!WorkspaceDockPolicy_CanSplitTarget(1, 0, TRUE));

	/* Host/anchor cannot support split regardless of document counts. */
	assert(!WorkspaceDockPolicy_CanSplitTarget(2, 2, FALSE));

	/* Side split is still valid for non-empty target groups. */
	assert(WorkspaceDockPolicy_CanSplitTarget(1, 1, TRUE));

	/* Multi-document source may still split around an empty target group. */
	assert(WorkspaceDockPolicy_CanSplitTarget(2, 0, TRUE));

	return 0;
}

static int test_workspace_empty_group_cleanup_policy(void)
{
	/* Main workspace must survive even when empty. */
	assert(!WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(0, TRUE, TRUE));

	/* Detached/floating groups are not removed by host cleanup policy. */
	assert(!WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(0, FALSE, FALSE));

	/* Empty non-main docked groups must be removed. */
	assert(WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(0, FALSE, TRUE));
	assert(WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(-1, FALSE, TRUE));

	/* Non-empty groups remain. */
	assert(!WorkspaceDockPolicy_ShouldAutoRemoveEmptyGroup(1, FALSE, TRUE));

	return 0;
}

int main(void)
{
	int failed = 0;
	failed |= test_zone_tab_rect_vertical_starts_from_top();
	failed |= test_zone_tab_rect_horizontal_starts_from_left();
	failed |= test_zone_tab_rect_clips_when_outside_client();
	failed |= test_zone_tab_rect_by_offset_and_length();
	failed |= test_stack_style_and_grips();
	failed |= test_split_grip_adjustment();
	failed |= test_split_grip_ratio_scaling();
	failed |= test_invalid_arguments();
	failed |= test_dock_preview_rect_behavior();
	failed |= test_zone_tab_click_policy();
	failed |= test_core_panel_lock_policy();
	failed |= test_explicit_dock_roles_override_name_fallback();
	failed |= test_dock_shell_zone_builder_assigns_role_and_side();
	failed |= test_dock_shell_appends_zone_stack_split();
	failed |= test_dock_shell_build_main_layout_attaches_workspace_and_zones();
	failed |= test_dock_group_semantics_for_tool_and_document_panes();
	failed |= test_floating_dock_policy_semantics();
	failed |= test_dock_view_catalog_maps_known_persistent_views();
	failed |= test_dock_floating_layout_file_round_trip();
	failed |= test_document_session_model_file_round_trip();
	failed |= test_floating_document_session_model_file_round_trip();
	failed |= test_dock_model_capture_strips_runtime_handles_but_keeps_semantics();
	failed |= test_dock_model_file_round_trip();
	failed |= test_dock_model_build_tree_round_trip();
	failed |= test_dock_model_full_pipeline_round_trip();
	failed |= test_dock_model_validate_repairs_metadata_and_active_tab();
	failed |= test_dock_model_validate_rejects_unknown_or_duplicate_views();
	failed |= test_workspace_document_dock_split_policy();
	failed |= test_workspace_empty_group_cleanup_policy();

	if (failed)
	{
		printf("docklayout tests FAILED\n");
		return 1;
	}

	printf("docklayout tests PASSED\n");
	return 0;
}
