#ifndef PANITENT_SETTINGS_H_
#define PANITENT_SETTINGS_H_

#define IDT_SETTINGS_PAGE_MAIN  0
#define IDT_SETTINGS_PAGE_DEBUG 1

LRESULT CALLBACK SettingsWndProc(HWND hwnd,
                                 UINT msg,
                                 WPARAM wParam,
                                 LPARAM lParam);
int InitSettingsWindow(HWND hwnd);
int ShowSettingsWindow(HWND hwnd);
void RegisterSettingsTabPageMain();
void RegisterSettingsTabPageDebug();
void register_event_handler(int, void (*)(WPARAM, LPARAM));

#endif /* PANITENT_SETTINGS_H_ */
