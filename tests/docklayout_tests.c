#include <assert.h>
#include <stdio.h>

#include "../src/docklayout.h"
#include "../src/dockpolicy.h"

static int test_zone_tab_rect_vertical_starts_from_top(void)
{
	RECT rcClient = { 0, 0, 800, 600 };
	RECT rcTab0 = { 0 };
	RECT rcTab1 = { 0 };

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_LEFT, 0, 3, &rcTab0));
	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_LEFT, 1, 3, &rcTab1));

	assert(rcTab0.left == 0);
	assert(rcTab0.right == DOCKLAYOUT_ZONE_TAB_THICKNESS);
	assert(rcTab0.top == 0);
	assert(rcTab0.bottom == DOCKLAYOUT_ZONE_TAB_LENGTH);

	assert(rcTab1.top == DOCKLAYOUT_ZONE_TAB_LENGTH + DOCKLAYOUT_ZONE_TAB_GAP);
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

	assert(rcTab0.left == 0);
	assert(rcTab0.right == DOCKLAYOUT_ZONE_TAB_LENGTH);
	assert(rcTab0.top == 0);
	assert(rcTab0.bottom == DOCKLAYOUT_ZONE_TAB_THICKNESS);

	assert(rcTab2.left == (DOCKLAYOUT_ZONE_TAB_LENGTH + DOCKLAYOUT_ZONE_TAB_GAP) * 2);
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
	assert(rcTab.left == 150);
	assert(rcTab.right == 150);

	assert(DockLayout_GetZoneTabRect(&rcClient, DKS_LEFT, 2, 3, &rcTab));
	assert(rcTab.top == 100);
	assert(rcTab.bottom == 100);

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

static int test_invalid_arguments(void)
{
	RECT rc = { 0 };
	assert(!DockLayout_GetZoneTabRect(NULL, DKS_TOP, 0, 1, &rc));
	assert(!DockLayout_GetZoneTabRect(&rc, DKS_TOP, 0, 1, NULL));
	assert(!DockLayout_GetZoneTabRect(&rc, DKS_NONE, 0, 1, &rc));
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
	assert(!DockPolicy_CanUndockPanelName(L"Root"));
	assert(!DockPolicy_CanUndockPanelName(L"DockZone.Left"));
	assert(!DockPolicy_CanClosePanelName(L"DockShell.Root"));

	assert(DockPolicy_CanUndockPanelName(L"Palette"));
	assert(DockPolicy_CanClosePanelName(L"Layers"));
	assert(DockPolicy_CanUndockPanelName(NULL));

	return 0;
}

int main(void)
{
	int failed = 0;
	failed |= test_zone_tab_rect_vertical_starts_from_top();
	failed |= test_zone_tab_rect_horizontal_starts_from_left();
	failed |= test_zone_tab_rect_clips_when_outside_client();
	failed |= test_stack_style_and_grips();
	failed |= test_invalid_arguments();
	failed |= test_zone_tab_click_policy();
	failed |= test_core_panel_lock_policy();

	if (failed)
	{
		printf("docklayout tests FAILED\n");
		return 1;
	}

	printf("docklayout tests PASSED\n");
	return 0;
}
