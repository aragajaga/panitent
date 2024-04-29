#pragma once

typedef struct PointerTool PointerTool;
typedef struct PencilTool PencilTool;
typedef struct CircleTool CircleTool;
typedef struct LineTool LineTool;
typedef struct RectangleTool RectangleTool;
typedef struct TextTool TextTool;
typedef struct FillTool FillTool;
typedef struct PickerTool PickerTool;
typedef struct BrushTool BrushTool;
typedef struct EraserTool EraserTool;

typedef struct ToolboxWindow ToolboxWindow;
struct ToolboxWindow {
	Window base;

	Tool** tools;
	unsigned int tool_count;

	PointerTool* m_pPointerTool;
	PencilTool* m_pPencilTool;
	CircleTool* m_pCircleTool;
	LineTool* m_pLineTool;
	RectangleTool* m_pRectangleTool;
	TextTool* m_pTextTool;
	FillTool* m_pFillTool;
	PickerTool* m_pPickerTool;
	BrushTool* m_pBrushTool;
	EraserTool* m_pEraserTool;
};

typedef struct _tagTOOLBOXICONTHEME {
  LPWSTR lpszName;
  LPWSTR lpszResource;
} TOOLBOXICONTHEME, *PTOOLBOXICONTHEME;

ToolboxWindow* ToolboxWindow_Create(PanitentApplication* pPanitentApplication);
