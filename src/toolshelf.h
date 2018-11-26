#ifndef TOOLSHELF_H
#define TOOLSHELF_H

#define TOOLSHELF_WC L"ToolShelfClass"
void ToolShelf_OnPaint(HWND hwnd);
void ToolShelf_OnMouseMove(HWND hwnd, LPARAM lParam);
LRESULT CALLBACK _toolshelf_msgproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RegisterToolShelf();

#endif /* TOOLSHELF_H */
