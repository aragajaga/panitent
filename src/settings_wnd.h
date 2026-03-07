#ifndef SETTINGS_WND_H_
#define SETTINGS_WND_H_

#include "win32/window.h"

typedef struct SettingsWindow SettingsWindow;

struct SettingsWindow {
    Window base;
    HWND hGroupWindow;
    HWND hCheckRememberWindowPos;
    HWND hLabelPosX;
    HWND hEditPosX;
    HWND hLabelPosY;
    HWND hEditPosY;
    HWND hLabelWidth;
    HWND hEditWidth;
    HWND hLabelHeight;
    HWND hEditHeight;

    HWND hGroupBehavior;
    HWND hCheckLegacyFileDialogs;
    HWND hCheckEnablePenTablet;

    HWND hButtonOk;
    HWND hButtonCancel;
    HWND hButtonApply;
    BOOL fDirty;
};

SettingsWindow* SettingsWindow_Create(void);
HWND SettingsWindow_Show(void);

#endif  /* SETTINGS_WND_H_ */
