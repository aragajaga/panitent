#include "precomp.h"

#include "win32/window.h"
#include "win32/dialog.h"
#include "win32/util.h"

#include <windowsx.h>
#include <commctrl.h>
#include <strsafe.h>
#include <assert.h>

#include "util/stack.h"
#include "util/tree.h"
#include "dockhost.h"
#include "resource.h"
#include "floatingwindowcontainer.h"
#include "dockinspectordialog.h"
#include "toolwndframe.h"

#include "panitentapp.h"

static const WCHAR szClassName[] = L"__DockHostWindow";

POINT captionPos;

BOOL fSuggestTop;

int iCaptionHeight = 24;
int iBorderWidth = 4;

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y);
BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y);
void Dock_DestroyInclusive(TreeNode*, TreeNode*);
void DockNode_Paint(TreeNode*, HDC, HBRUSH);

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam);
void DockHostWindow_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags);

BOOL DockData_GetClientRect(DockData* pDockData, RECT* rc)
{
	if (!pDockData)
	{
		return FALSE;
	}

	RECT rcClient = pDockData->rc;

	if (pDockData->bShowCaption)
	{
		rcClient.top += iCaptionHeight + 1;
	}

	/* TODO: memcpy? */
	*rc = rcClient;

	return TRUE;
}

void DockData_Init(DockData* pDockData);

void DockData_Init(DockData* pDockData)
{
	pDockData->iGripPos = 64;
	pDockData->dwStyle = DGA_START | DGD_HORIZONTAL | DGP_ABSOLUTE;
	pDockData->bShowCaption = FALSE;
}

BOOL DockData_GetCaptionRect(DockData* pDockData, RECT* rc)
{
	if (!pDockData->bShowCaption)
	{
		return FALSE;
	}

	RECT rcCaption = pDockData->rc;
	rcCaption.top += iBorderWidth;
	rcCaption.left += iBorderWidth;
	rcCaption.right -= iBorderWidth;
	rcCaption.bottom = rcCaption.top + iBorderWidth + iCaptionHeight;

	*rc = rcCaption;

	return TRUE;
}

#define DHT_UNKNOWN 0
#define DHT_CAPTION 1
#define DHT_CLOSEBTN 2

#define WINDOWBUTTONSIZE 14

BOOL Dock_CaptionHitTest(DockData* pDockData, int x, int y)
{
	RECT rcCaption = { 0 };
	DockData_GetCaptionRect(pDockData, &rcCaption);

	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	return PtInRect(&rcCaption, pt);
}

BOOL Dock_CloseButtonHitTest(DockData* pDockData, int x, int y)
{
	RECT rcCaption = { 0 };
	DockData_GetCaptionRect(pDockData, &rcCaption);
	rcCaption.left = rcCaption.right - WINDOWBUTTONSIZE;

	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	return PtInRect(&rcCaption, pt);
}

int DockHostWindow_HitTest(DockHostWindow* pDockHostWindow, TreeNode** ppTreeNode, int x, int y)
{
	if (!pDockHostWindow->pRoot_)
	{
		return 0;
	}

	TreeTraversalRLOT traversal = { 0 };
	TreeTraversalRLOT_Init(&traversal, pDockHostWindow->pRoot_);
	
	TreeNode* pCurrentNode = NULL;
	TreeNode* pHittedNode = NULL;
	while (pCurrentNode = TreeTraversalRLOT_GetNext(&traversal))
	{
		DockData* pDockData = (DockData*)pCurrentNode->data;

		POINT pt = { 0 };
		pt.x = x;
		pt.y = y;
		if (PtInRect(&pDockData->rc, pt))
		{
			pHittedNode = (DockData*)pCurrentNode;
			break;
		}
	}
	TreeTraversalRLOT_Destroy(&traversal);

	/*
	int localX = x - pHittedDock->rc.left;
	int localY = y - pHittedDock->rc.top;
	*/

	*ppTreeNode = pHittedNode;

	if (pHittedNode && pHittedNode->data)
	{
		if (Dock_CloseButtonHitTest((DockData*)pHittedNode->data, x, y))
		{
			return DHT_CLOSEBTN;
		}
		else if (Dock_CaptionHitTest((DockData*)pHittedNode->data, x, y))
		{
			return DHT_CAPTION;
		}
	}

	return DHT_UNKNOWN;
}

void DockNode_Paint(TreeNode* pNodeParent, HDC hdc, HBRUSH hCaptionBrush)
{
	/* Break if node is invalid */
	if (!pNodeParent)
	{
		return;
	}

	TreeTraversalRLOT dockNodeTraversal;
	TreeTraversalRLOT_Init(&dockNodeTraversal, pNodeParent);

	TreeNode* pCurrentNode = NULL;
	while (pCurrentNode = TreeTraversalRLOT_GetNext(&dockNodeTraversal))
	{
		DockData* pDockData = (DockData*)pCurrentNode->data;

		if (!pDockData)
		{
			continue;
		}

		RECT* rc = &pDockData->rc;
	
		if (pDockData->bShowCaption)
		{
			/* Draw pinned window background */
			SelectObject(hdc, hCaptionBrush);
			SelectObject(hdc, GetStockObject(DC_PEN));
			SetDCPenColor(hdc, Win32_HexToCOLORREF(L"#6d648e"));
			Rectangle(hdc, rc->left, rc->top, rc->right, rc->bottom);
	
			/* Window title rect */
			RECT rcCaption = { 0 };
			rcCaption.left = rc->left + iBorderWidth;
			rcCaption.top = rc->top + iBorderWidth;
			rcCaption.right = rc->right - iBorderWidth;
			rcCaption.bottom = rc->top + iBorderWidth + iCaptionHeight;
	
			/* ================================================================================ */
	
			/* Draw window caption text */
			
			RECT rcCaptionText = { 0 };
			memcpy(&rcCaptionText, rc, sizeof(RECT));
			Win32_ContractRect(&rcCaptionText, 4, 4);
	
			HFONT guiFont = PanitentApp_GetUIFont(PanitentApp_Instance());
			HFONT hOldFont = SelectObject(hdc, guiFont);
	
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, COLORREF_WHITE);
	
			size_t chCount = wcsnlen_s(pDockData->lpszCaption, MAX_PATH);
			DrawText(hdc, pDockData->lpszCaption, (int)chCount, &rcCaptionText, 0);
	
			SelectObject(hdc, hOldFont);
	
			/* ================================================================================ */
	
			/*
                Draw close button

				Calculate right-align of title bar buttons sex
				rc->left is the offset of current dock area
	
				[ RectWidth             ]*
								 *[ <-  ]   Button width
			*/
	
			CaptionButton button = { 0 };
			button.glyph = GLYPH_CLOSE;
			button.size = (SIZE){ WINDOWBUTTONSIZE, WINDOWBUTTONSIZE };
			DrawCaptionButton(&button, hdc, rc->left + Win32_Rect_GetWidth(rc) - WINDOWBUTTONSIZE - iBorderWidth, rc->top + iBorderWidth, 14, 14);
	
			/* ================================================================================ */
	
			/* Redraw window */
	
			if (pNodeParent->data && ((DockData*)pNodeParent->data)->hWnd)
			{
				HWND hWnd = ((DockData*)pNodeParent->data)->hWnd;
				InvalidateRect(hWnd, NULL, FALSE);
			}
		}
	}
	
	TreeTraversalRLOT_Destroy(&dockNodeTraversal);
}

void DockHostWindow_Rearrange(DockHostWindow* pDockHostWindow)
{
	TreeNode* pRoot = pDockHostWindow->pRoot_;

	if (!pRoot)
	{
		return;
	}
 
	PostOrderTreeTraversal traversal = { 0 };
	PostOrderTreeTraversal_Init(&traversal, pRoot);

	TreeNode* pCurrentNode = NULL;
	while (pCurrentNode = PostOrderTreeTraversal_GetNext(&traversal))
	{
		DockData* pDockData = (DockData*)pCurrentNode->data;

		TreeNode* pChildNode1 = pCurrentNode->node1;
		TreeNode* pChildNode2 = pCurrentNode->node2;

		/* Draw split if both children present */
		if (pChildNode1 && pChildNode2)
		{
			RECT rcClient = { 0 };
			DockData_GetClientRect(pDockData, &rcClient);

			RECT rcNode1 = rcClient;

			DWORD dwStyle = pDockData->dwStyle;

			if (dwStyle & DGP_RELATIVE)
			{
				rcNode1.right = (rcNode1.right - rcNode1.left) / 2 - iBorderWidth / 2;
			}
			else {
				if (dwStyle & DGA_END)
				{
					if (dwStyle & DGD_VERTICAL)
					{
						rcNode1.bottom = rcNode1.bottom - pDockData->iGripPos - iBorderWidth / 2;
					}
					else {
						rcNode1.right = rcNode1.right - pDockData->iGripPos - iBorderWidth / 2;
					}
				}
				else {
					if (dwStyle & DGD_VERTICAL)
					{
						rcNode1.bottom = rcNode1.top + pDockData->iGripPos - iBorderWidth / 2;
					}
					else {
						rcNode1.right = rcNode1.left + pDockData->iGripPos - iBorderWidth / 2;
					}
				}
			}

			if (pChildNode1 && pChildNode1->data)
			{
				DockData* pDockData = (DockData*)pChildNode1->data;
				pDockData->rc = rcNode1;
			}

			/* Second part */
			RECT rcNode2 = rcClient;
			if (dwStyle & DGP_RELATIVE)
			{
				rcNode2.left += (rcNode2.right - rcNode2.left) / 2 + iBorderWidth / 2;
			}
			else {
				if (dwStyle & DGA_END)
				{
					if (dwStyle & DGD_VERTICAL)
					{
						rcNode2.top = rcNode2.bottom - pDockData->iGripPos + iBorderWidth / 2;
					}
					else {
						rcNode2.left = rcNode2.right - pDockData->iGripPos + iBorderWidth / 2;
					}
				}
				else {
					if (dwStyle & DGD_VERTICAL)
					{
						rcNode2.top = rcNode2.top + pDockData->iGripPos + iBorderWidth / 2;
					}
					else {
						rcNode2.left = rcNode2.left + pDockData->iGripPos + iBorderWidth / 2;
					}
				}
			}

			if (pChildNode2 && pChildNode2->data)
			{
				DockData* pDockData = (DockData*)pChildNode2->data;
				pDockData->rc = rcNode2;
			}
		}
		else if (pChildNode1 || pChildNode2) {
			TreeNode* child = pChildNode1 ? pChildNode1 : pChildNode2;

			RECT rcClient = { 0 };
			DockData_GetClientRect(pDockData, &rcClient);

			((DockData*)child->data)->rc = rcClient;
		}

		if (pDockData && pDockData->hWnd)
		{
			RECT rc = { 0 };
			DockData_GetClientRect(pDockData, &rc);

			Win32_ContractRect(&rc, 4, 4);

			SetWindowPos(pDockData->hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
			UpdateWindow(pDockData->hWnd);
		}
	}

	PostOrderTreeTraversal_Destroy(&traversal);

	Window_Invalidate((Window *)pDockHostWindow);

	DockInspectorDialog_Update(pDockHostWindow->m_pDockInspectorDialog, pDockHostWindow->pRoot_);
}

TreeNode* DockNode_FindParent(TreeNode* root, TreeNode* node)
{
	if (!root || !node)
	{
		return NULL;
	}

	TreeNode* pFoundNode = NULL;

	if (root->node1)
	{
		if (root->node1 == node)
		{
			return root;
		}
		else {
			pFoundNode = DockNode_FindParent(root->node1, node);
		}
	}

	if (!pFoundNode && root->node2)
	{
		if (root->node2 == node)
		{
			return root;
		}
		else {
			pFoundNode = DockNode_FindParent(root->node2, node);
		}
	}

	return pFoundNode;
}

void DockHostWindow_DestroyInclusive(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	TreeNode* pRoot = pDockHostWindow->pRoot_;

	DestroyWindow(((DockData*)pTargetNode->data)->hWnd);
	TreeNode* pParentOfTarget = DockNode_FindParent(pRoot, pTargetNode);

	TreeNode* pDetachedNode = NULL;
	if (pParentOfTarget)
	{
		if (pTargetNode == pParentOfTarget->node1)
		{
			free(pParentOfTarget->node1);
			pParentOfTarget->node1 = NULL;

			pDetachedNode = pParentOfTarget->node2;
		}
		else if (pTargetNode == pParentOfTarget->node2) {
			free(pParentOfTarget->node2);
			pParentOfTarget->node2 = NULL;

			pDetachedNode = pParentOfTarget->node1;
		}

		TreeNode* pGrandparentOfTarget = DockNode_FindParent(pRoot, pParentOfTarget);
		if (pGrandparentOfTarget)
		{
			if (pParentOfTarget == pGrandparentOfTarget->node1)
			{
				pGrandparentOfTarget->node1 = pDetachedNode;
				free(pParentOfTarget);
				pParentOfTarget = NULL;
			}
			else if (pParentOfTarget == pGrandparentOfTarget->node2) {
				pGrandparentOfTarget->node2 = pDetachedNode;
				free(pParentOfTarget);
				pParentOfTarget = NULL;
			}
		}

	}
	else {
		free(pTargetNode);
	}

	DockHostWindow_Rearrange(pDockHostWindow);
}

void DockHostWindow_Undock(DockHostWindow* pDockHostWindow, TreeNode* pTargetNode)
{
	TreeNode* pRoot = pDockHostWindow->pRoot_;

	if (!pRoot || !pTargetNode)
	{
		return;
	}

	TreeNode* pParentOfTarget = DockNode_FindParent(pRoot, pTargetNode);
	if (!pParentOfTarget)
	{
		return;
	}

	TreeNode* pNodeSibling = NULL;
	TreeNode* pNodeDetached = NULL;
	if (pTargetNode == pParentOfTarget->node1)
	{
		pNodeDetached = pParentOfTarget->node1;
		pParentOfTarget->node1 = NULL;

		pNodeSibling = pParentOfTarget->node2;
		
	}
	else if (pTargetNode == pParentOfTarget->node2) {
		pNodeDetached = pParentOfTarget->node2;
		pParentOfTarget->node2 = NULL;

		pNodeSibling = pParentOfTarget->node1;
	}

	TreeNode* pGrandparentOfTarget = DockNode_FindParent(pRoot, pParentOfTarget);
	if (pGrandparentOfTarget)
	{
		if (pGrandparentOfTarget->node1 == pParentOfTarget)
		{
			pGrandparentOfTarget->node1 = pNodeSibling;
			free(pParentOfTarget);
		}
		else if (pGrandparentOfTarget->node2 == pParentOfTarget) {
			pGrandparentOfTarget->node2 = pNodeSibling;
			free(pParentOfTarget);
		}
	}

	DockHostWindow_Rearrange(pDockHostWindow);
}

BOOL DockHostWindow_OnCreate(DockHostWindow* pDockHostWindow, LPCREATESTRUCT lpcs)
{
	pDockHostWindow->hCaptionBrush_ = CreateSolidBrush(Win32_HexToCOLORREF(L"#9185be"));

	return TRUE;
}

void DockHostWindow_OnDestroy(DockHostWindow* pDockHostWindow)
{
	DeleteObject(pDockHostWindow->hCaptionBrush_);
}

#define DOCKHOSTBGMARGIN 16

void DockHostWindow_OnPaint(DockHostWindow* pDockHostWindow)
{
	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);

	PAINTSTRUCT ps = { 0 };
	HDC hdc = BeginPaint(hWnd, &ps);

	if (pDockHostWindow->pRoot_) {
		DockNode_Paint(pDockHostWindow->pRoot_, hdc, pDockHostWindow->hCaptionBrush_);
	}
	else {
		HDC hdcLogo = CreateCompatibleDC(hdc);
		HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_DOCKHOSTBG));

		BITMAP bm;
		GetObject(hBitmap, sizeof(BITMAP), &bm);
		int width = bm.bmWidth;
		int height = bm.bmHeight;

		HBITMAP hOldBitmap = SelectObject(hdcLogo, hBitmap);

		BLENDFUNCTION blendFunc;
		blendFunc.BlendOp = AC_SRC_OVER;
		blendFunc.BlendFlags = 0;
		blendFunc.SourceConstantAlpha = 0xFF;
		blendFunc.AlphaFormat = AC_SRC_ALPHA;

		RECT rcClient;
		Window_GetClientRect(pDockHostWindow, &rcClient);

		int clientWidth = rcClient.right - rcClient.left;
		int clientHeight = rcClient.bottom - rcClient.top;

		int x = clientWidth - width - DOCKHOSTBGMARGIN;
		int y = clientHeight - height - DOCKHOSTBGMARGIN;
		AlphaBlend(hdc, x, y, width, height, hdcLogo, 0, 0, width, height, blendFunc);

		SelectObject(hdcLogo, hOldBitmap);
		DeleteDC(hdcLogo);
	}

	EndPaint(hWnd, &ps);
}

void DockHostWindow_OnSize(DockHostWindow* pDockHostWindow, UINT state, int cx, int cy)
{
	UNREFERENCED_PARAMETER(state);

	if (pDockHostWindow->pRoot_ && pDockHostWindow->pRoot_->data)
	{
		RECT rcRoot = { 0 };
		rcRoot.right = cx;
		rcRoot.bottom = cy;
		((DockData*)pDockHostWindow->pRoot_->data)->rc = rcRoot;

		DockHostWindow_Rearrange(pDockHostWindow);
	}
}

HWND g_hWndDragOverlay;

void DockHostWindow_UndockToFloating(DockHostWindow* pDockHostWindow, TreeNode* pNode)
{
	TreeNode* pRoot = pDockHostWindow->pRoot_;
	DockHostWindow_Undock(pDockHostWindow, pNode);

	FloatingWindowContainer* pFloatingWindowContainer = FloatingWindowContainer_Create();
	Window_CreateWindow((Window*)pFloatingWindowContainer, NULL);
	FloatingWindowContainer_PinWindow(pFloatingWindowContainer, ((DockData*)pNode->data)->hWnd);

	DockHostWindow_Rearrange(pDockHostWindow, pNode);

	DestroyWindow(g_hWndDragOverlay);
}

void DockHostWindow_OnMouseMove(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags)
{
	if (pDockHostWindow->fCaptionDrag)
	{
		int distance = (int)roundf(sqrtf((powf(pDockHostWindow->ptDragPos_.x - x, 2.0f) + powf(pDockHostWindow->ptDragPos_.y - y, 2.0f))));
		int activateDistance = 32;

		if (distance >= activateDistance)
		{
			DockHostWindow_UndockToFloating(pDockHostWindow, pDockHostWindow->m_pSubjectNode);
			pDockHostWindow->fCaptionDrag = FALSE;

			ReleaseCapture();
		}
		/*
		else {
			POINT pt = { 0 };
			pt.x = pDockHostWindow->ptDragPos_.x;
			pt.y = pDockHostWindow->ptDragPos_.y;
			HDC hScreenDC = GetDC(NULL);
			ClientToScreen(pDockHostWindow->base.hWnd, &pt);

			Ellipse(hScreenDC, pt.x - activateDistance, pt.y - activateDistance, pt.x + activateDistance, pt.y + activateDistance);
		}
		*/
	}
}

LRESULT CALLBACK DragOverlayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

float smoothstepf(float edge0, float edge1, float x) {
	float t = fminf(fmaxf((x - edge0) / (edge1 - edge0), 0.0f), 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

float clampf(float value, int min, int max)
{
	if (value < min)
	{
		return min;
	}
	else if (value > max)
	{
		return max;
	}
	else {
		return value;
	}
}

void DockHostWindow_StartDrag(DockHostWindow* pDockHostWindow, int x, int y)
{
	WNDCLASSEX wcex = { 0 };
	if (!GetClassInfoEx(GetModuleHandle(NULL), L"__DragOverlayClass", &wcex))
	{
		wcex.cbSize = sizeof(wcex);
		wcex.lpfnWndProc = (WNDPROC)DragOverlayWndProc;
		wcex.hInstance = GetModuleHandle(NULL);
		wcex.lpszClassName = L"__DragOverlayClass";
		RegisterClassEx(&wcex);
	}

	HWND g_hWndDragOverlay = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, L"__DragOverlayClass", L"DragOverlay", WS_VISIBLE | WS_POPUP, x - 64, y - 64, 128, 128, NULL, NULL, GetModuleHandle(NULL), NULL);

	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);

	BLENDFUNCTION blendFunction = { 0 };
	blendFunction.BlendOp = AC_SRC_OVER;
	blendFunction.SourceConstantAlpha = 255;
	blendFunction.AlphaFormat = AC_SRC_ALPHA;

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 128;
	bmi.bmiHeader.biHeight = -128;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	unsigned int* pBits;

	// Create a compatible bitmap with the desired format
	HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);

	// Draw
	{
		int centerX = 32;
		int centerY = 32;
		int radius = 32;

		unsigned char* pCircle = malloc(64 * 64 * sizeof(unsigned char));

		for (int y = 0; y < 64; ++y)
		{
			for (int x = 0; x < 64; ++x)
			{
				int dx = x - centerX;
				int dy = y - centerY;
				float distance = sqrtf((float)(dx * dx + dy * dy));

				float dist = distance / (float)radius;

				// Calculate alpha based on the distance

				float uStripeWidth = 0.1f;
				float stripeEdge = 0.5 * (1.0 / uStripeWidth);
				BYTE alpha1 = (BYTE)(255.0f * smoothstepf(1.0f, 0.95f, dist));
				BYTE alpha2 = (BYTE)(255.0f * (sinf(dist * 24.0f) * 0.5f + 0.5f));
				BYTE alpha = min(alpha1, alpha2);
				if (alpha > 255)
				{
					alpha = 255;
				}
				if (alpha < 0)
				{
					alpha = 0;
				}

				pCircle[y * 64 + x] = alpha;
			}
		}

		unsigned int* pData = pBits;
		pData += 128 * ((128 - 64) / 2) + ((128 - 64) / 2);
		for (int y = 0; y < 64; ++y)
		{
			for (int x = 0; x < 64; ++x)
			{
				BYTE alpha = pCircle[y * 64 + x];
				unsigned int color = (alpha << 24) | (alpha << 16);
				*pData = color;
				pData++;
			}
			
			pData += 64;
		}

		free(pCircle);

		/*
		for (int y = 0; y < 128; ++y)
		{
			for (int x = 0; x < 128; ++x)
			{
				unsigned int color = pBits[y * 128 + x];

				// Extract RGBA components
				BYTE alpha = (color >> 24) & 0xFF;
				BYTE red = (color >> 16) & 0xFF;
				BYTE green = (color >> 8) & 0xFF;
				BYTE blue = (color) & 0xFF;

				// Premultiply RGB values by alpha
				BYTE premultRed = (BYTE)((red * alpha) / 255);
				BYTE premultGreen = (BYTE)((green * alpha) / 255);
				BYTE premultBlue = (BYTE)((blue * alpha) / 255);

				// Store the premultiplied color back into the bitmap
				pBits[y * 128 + x] = (premultRed << 16) | (premultGreen << 8) | premultBlue | (alpha << 24);
			}
		}
		*/
		
		/*
		for (int y = 0; y < 128; ++y)
		{
			pBits[y * 128] = 0xFFFF0000;
			pBits[y * 128 + 127] = 0xFFFF0000;
			pBits[y] = 0xFFFF0000;
			pBits[y + 127 * 128] = 0xFFFF0000;
		}
		*/

		/*
		for (int y = 0; y < 128; ++y)
		{
			for (int x = 0; x < 128; ++x)
			{
				int dx = x - centerX;
				int dy = y - centerY;
				float distance = sqrtf((float)(dx * dx + dy * dy));

				// Calculate alpha based on the distance
				BYTE alpha = (BYTE)(255 * (1.0f - (distance / radius)));
				if (alpha > 255)
				{
					alpha = 255;
				}
				if (alpha < 0 || distance > radius)
				{
					alpha = 0;
				}

				// Set the pixel color (white with varying alpha)
				DWORD color = (alpha << 24) | (alpha << 16) | (alpha << 8) | alpha;
				pBits[y * 128 + x] = color;
			}
		}
		*/
	}

	POINT ptPos = { x, y };
	SIZE sizeWnd = { 128, 128 };
	POINT ptSrc = { 0, 0 };

	ClientToScreen(pDockHostWindow->base.hWnd, &ptPos);
	ptPos.x -= 64;
	ptPos.y -= 64;

	UpdateLayeredWindow(g_hWndDragOverlay, hdcScreen, &ptPos, &sizeWnd, hdcMem, &ptSrc, RGB(0, 0, 0), &blendFunction, ULW_ALPHA);

	SelectObject(hdcMem, hOldObj);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);
}

void DockHostWindow_OnLButtonDown(DockHostWindow* pDockHostWindow, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	UNREFERENCED_PARAMETER(fDoubleClick);
	UNREFERENCED_PARAMETER(keyFlags);

	HWND hWnd = Window_GetHWND((Window*)pDockHostWindow);

	/* Get whole dock window client rect */
	RECT rcClient = { 0 };
	GetClientRect(hWnd, &rcClient);

	TreeNode* pTreeNode = NULL;
	int htType = DockHostWindow_HitTest(pDockHostWindow, &pTreeNode, x, y);

	switch (htType)
	{
	case DHT_CLOSEBTN:
	{
		/* Do nothing. */
	}
	break;

	case DHT_CAPTION:
	{
		pDockHostWindow->fCaptionDrag = TRUE;
		pDockHostWindow->m_pSubjectNode = pTreeNode;

		SetCapture(hWnd);

		/* Save click position */
		pDockHostWindow->ptDragPos_.x = x - (rcClient.left + 2);
		pDockHostWindow->ptDragPos_.y = y - (rcClient.top + 2);
		pDockHostWindow->fDrag_ = TRUE;

		DockHostWindow_StartDrag(pDockHostWindow, pDockHostWindow->ptDragPos_.x, pDockHostWindow->ptDragPos_.y);
	}
	break;
	}
}

void DockHostWindow_InvokeDockInspectorDialog(DockHostWindow* pDockHostWindow)
{
	HWND hWndDialog = Dialog_CreateWindow((Dialog*)pDockHostWindow->m_pDockInspectorDialog, IDD_DOCKINSPECTOR, Window_GetHWND((Window*)pDockHostWindow), FALSE);
	if (hWndDialog && IsWindow(hWndDialog))
	{
		/* Important. Idk why */
		ShowWindow(hWndDialog, SW_SHOW);
	}
}

void DockHostWindow_OnLButtonUp(DockHostWindow* pDockHostWindow, int x, int y, UINT keyFlags) {
	UNREFERENCED_PARAMETER(keyFlags);

	TreeNode* pTreeNode = NULL;
	int htType = DockHostWindow_HitTest(pDockHostWindow, &pTreeNode, x, y);

	switch (htType)
	{
	case DHT_CLOSEBTN:
	{
		DockHostWindow_DestroyInclusive(pDockHostWindow, pTreeNode);
		Window_Invalidate((Window*)pDockHostWindow);
	}
	break;

	case DHT_CAPTION:
	{
		/* Do nothing. */
	}
	break;
	}

	pDockHostWindow->fDrag_ = FALSE;
	DestroyWindow(g_hWndDragOverlay);
	ReleaseCapture();
}

#define IDM_DOCKINSPECTOR 101

void DockHostWindow_OnContextMenu(DockHostWindow* pDockHostWindow, HWND hWndContext, int x, int y)
{
	POINT pt = { 0 };
	pt.x = x;
	pt.y = y;
	ScreenToClient(Window_GetHWND((Window*)pDockHostWindow), &pt);

	TreeNode* pTreeNode = NULL;
	int htType = DockHostWindow_HitTest(pDockHostWindow, &pTreeNode, pt.x, pt.y);

	switch (htType)
	{
	case DHT_CLOSEBTN:
		{
			/* Do nothing. */
		}
		break;

	case DHT_CAPTION:
		{
			/* Do nothing. */
		}
		break;

	case DHT_UNKNOWN:
		{
			HMENU hMenu = CreatePopupMenu();
			if (hMenu)
			{
				InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_DOCKINSPECTOR, L"Inspect Element...");
				TrackPopupMenu(hMenu, 0, x, y, 0, Window_GetHWND((Window*)pDockHostWindow), NULL);
			}
		}
		break;
	}
}

BOOL DockHostWindow_OnCommand(DockHostWindow* pDockHostWindow, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDM_DOCKINSPECTOR:
		DockHostWindow_InvokeDockInspectorDialog(pDockHostWindow);
		break;
	}
}

LRESULT DockHostWindow_UserProc(DockHostWindow* pDockHostWindow, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
		DockHostWindow_OnMouseMove(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_LBUTTONDOWN:
		DockHostWindow_OnLButtonDown(pDockHostWindow, FALSE, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_LBUTTONUP:
		DockHostWindow_OnLButtonUp(pDockHostWindow, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam), (UINT)wParam);
		return 0;
		break;

	case WM_CONTEXTMENU:
		DockHostWindow_OnContextMenu(pDockHostWindow, (HWND)wParam, (int)(short)GET_X_LPARAM(lParam), (int)(short)GET_Y_LPARAM(lParam));
		return 0;
		break;
	}

	return Window_UserProcDefault((Window *)pDockHostWindow, hWnd, message, wParam, lParam);
}

void DockHostWindow_PreRegister(LPWNDCLASSEX lpwcex)
{
	lpwcex->style = CS_VREDRAW | CS_HREDRAW;
	lpwcex->hCursor = LoadCursor(NULL, IDC_ARROW);
	lpwcex->hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	lpwcex->lpszClassName = szClassName;
}

void DockHostWindow_PreCreate(LPCREATESTRUCT lpcs)
{
	lpcs->dwExStyle = 0;
	lpcs->lpszClass = szClassName;
	lpcs->lpszName = L"DockHost";
	lpcs->style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN;
	lpcs->x = 0;
	lpcs->y = 0;
	lpcs->cx = 0;
	lpcs->cy = 0;
}

void DockHostWindow_Init(DockHostWindow* pDockHostWindow, PanitentApp* pPanitentApp)
{
	Window_Init(&pDockHostWindow->base, pPanitentApp);

	pDockHostWindow->base.szClassName = szClassName;

	pDockHostWindow->base.OnCreate = (FnWindowOnCreate)DockHostWindow_OnCreate;
	pDockHostWindow->base.OnDestroy = (FnWindowOnDestroy)DockHostWindow_OnDestroy;
	pDockHostWindow->base.OnPaint = (FnWindowOnPaint)DockHostWindow_OnPaint;
	pDockHostWindow->base.OnSize = (FnWindowOnSize)DockHostWindow_OnSize;
	pDockHostWindow->base.OnCommand = (FnWindowOnCommand)DockHostWindow_OnCommand;

	_WindowInitHelper_SetPreRegisterRoutine((Window *)pDockHostWindow, (FnWindowPreRegister)DockHostWindow_PreRegister);
	_WindowInitHelper_SetPreCreateRoutine((Window *)pDockHostWindow, (FnWindowPreCreate)DockHostWindow_PreCreate);
	_WindowInitHelper_SetUserProcRoutine((Window *)pDockHostWindow, (FnWindowUserProc)DockHostWindow_UserProc);

	pDockHostWindow->pRoot_ = NULL;

	pDockHostWindow->m_pDockInspectorDialog = DockInspectorDialog_Create();
}

DockHostWindow* DockHostWindow_Create(PanitentApp* pPanitentApp)
{
	DockHostWindow* pDockHostWindow = (DockHostWindow*)malloc(sizeof(DockHostWindow));

	if (pDockHostWindow)
	{
		memset(pDockHostWindow, 0, sizeof(DockHostWindow));
		DockHostWindow_Init(pDockHostWindow, pPanitentApp);
	}

	return pDockHostWindow;
}

TreeNode* DockHostWindow_SetRoot(DockHostWindow* pDockHostWindow, TreeNode* pNewRoot)
{
	TreeNode* pOldRoot = pDockHostWindow->pRoot_;
	pDockHostWindow->pRoot_ = pNewRoot;
	return pOldRoot;
}

TreeNode* DockHostWindow_GetRoot(DockHostWindow* pDockHostWindow)
{
	return pDockHostWindow->pRoot_;
}

void DockData_PinWindow(DockHostWindow* pDockHostWindow, DockData* pDockData, Window* window)
{
	HWND hWnd = Window_GetHWND(window);

	GetWindowText(hWnd, pDockData->lpszCaption, MAX_PATH);

	SetParent(hWnd, pDockHostWindow->base.hWnd);

	DWORD dwStyle = Window_GetStyle(window);
	dwStyle &= ~(WS_CAPTION | WS_THICKFRAME);
	dwStyle |= WS_CHILD;
	Window_SetStyle(window, dwStyle);

	pDockData->bShowCaption = TRUE;
	pDockData->hWnd = hWnd;
}

DockData* DockData_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
	DockData* pDockData = (DockData*)malloc(sizeof(DockData));

	if (pDockData)
	{
		memset(pDockData, 0, sizeof(DockData));

		pDockData->dwStyle = dwStyle;
		pDockData->iGripPos = iGripPos;
		pDockData->bShowCaption = bShowCaption;

		return pDockData;
	}

	return NULL;
}

TreeNode* DockNode_Create(int iGripPos, DWORD dwStyle, BOOL bShowCaption)
{
	TreeNode* pTreeNode = BinaryTree_AllocEmptyNode();
	DockData* pDockData = DockData_Create(iGripPos, dwStyle, bShowCaption);

	pTreeNode->data = pDockData;

	return pTreeNode;
}
