#pragma once

typedef struct SplitterContainer SplitterContainer;

struct SplitterContainer {
	Window base;
	HWND hWnd1;
	HWND hWnd2;
	BOOL bSplitterCaptured;
	int iGripPos;
	BOOL bVertical;
};

SplitterContainer* SplitterContainer_Create(struct Application* app);
void SplitterContainer_PinWindow1(SplitterContainer* pSplitterContainer, HWND hWnd);
void SplitterContainer_PinWindow2(SplitterContainer* pSplitterContainer, HWND hWnd);
