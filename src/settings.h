#ifndef SETTINGS_H
#define SETTINGS_H

int access_settings_file();

LRESULT CALLBACK wndproc_settings_window(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int show_settings_window(HWND hwnd);

#endif /* SETTINGS_H */
