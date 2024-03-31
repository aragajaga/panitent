#pragma once

typedef struct LayeredWindow LayeredWindow;

struct LayeredWindow {
	Window base;
	int iHover;
};

LayeredWindow* LayeredWindow_Create(struct Application* app);
