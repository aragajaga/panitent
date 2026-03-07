#pragma once

#include "precomp.h"

#include "win32/window.h"

typedef struct _Canvas Canvas;
typedef struct GLWindow GLWindow;

struct GLWindow {
	Window base;
	HDC hdcGL;
	HGLRC hglrc;
	void* pMesh;
	UINT_PTR nRenderTimerId;
	ULONGLONG uLastAutoTickMs;
	ULONGLONG uLastInteractionMs;
	int cxClient;
	int cyClient;
	unsigned int uTextureId;
	const Canvas* pTextureCanvas;
	int cxTexture;
	int cyTexture;
	BOOL bMouseRotating;
	POINT ptLastMouse;
	float fUserYaw;
	float fUserPitch;
	float fAutoYaw;
	float fZoom;
};

GLWindow* GLWindow_Create();
