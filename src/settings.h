#ifndef SETTINGS_H
#define SETTINGS_H

#define IDT_SETTINGS_PAGE_MAIN  0
#define IDT_SETTINGS_PAGE_DEBUG 1

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int InitSettingsWindow(HWND hwnd);
int ShowSettingsWindow(HWND hwnd);
void RegisterSettingsTabPageMain();
void RegisterSettingsTabPageDebug();

#endif /* SETTINGS_H */
