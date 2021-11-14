#ifndef PANITENT_FLEXIBLE_H
#define PANITENT_FLEXIBLE_H

enum {
  LAYOUTBOX_VERTICAL = 0,
  LAYOUTBOX_HORIZONTAL
};

enum {
  LAYOUTBOX_LEFT,
  LAYOUTBOX_CENTER,
  LAYOUTBOX_RIGHT
};

enum {
  LAYOUTBOX_TYPE_UNKNOWN = 0,
  LAYOUTBOX_TYPE_CONTROL,
  LAYOUTBOX_TYPE_BOX,
  LAYOUTBOX_TYPE_GROUP
};

typedef struct _LAYOUTBOX LAYOUTBOX, *PLAYOUTBOX;
typedef struct _GROUPBOX GROUPBOX, *PGROUPBOX;

typedef struct _LAYOUTBOXCONTENT {
  unsigned int type;
  union {
    HWND hWnd;
    LAYOUTBOX *pBox;
    GROUPBOX *pGroup;
  } data;
} LAYOUTBOXCONTENT, *PLAYOUTBOXCONTENT;

struct _LAYOUTBOX {
  HWND hContainer;
  LAYOUTBOXCONTENT hContent[16];
  size_t size;
  RECT rc;
  unsigned int type;
  unsigned int direction; 
  unsigned int align;
  unsigned int padding;
  unsigned int spaceBetween;
  unsigned int minWidth;
  BOOL stretch;
};

enum {
  FILL_STYLE_NONE,
  FILL_STYLE_SOLID,
  FILL_STYLE_GRADIENT
};

typedef struct _tagCAPTIONGRADIENT {
  DWORD dwColorStart;
  DWORD dwColorEnd;
  UINT uDirection;
} CAPTIONGRADIENT, *PCAPTIONGRADIENT;

typedef struct _tagCAPTIONFILLSTYLE {
  UINT uType;
  union {
    DWORD dwColor;
    CAPTIONGRADIENT gradient;
  } data;
} CAPTIONFILLSTYLE, *PCAPTIONFILLSTYLE;

typedef struct _tagGROUPBOXCAPTIONSTYLE {
  HFONT hFont;
  UINT uHeight;
  DWORD dwTextColor;
  CAPTIONFILLSTYLE captionBkgFill;
} GROUPBOXCAPTIONSTYLE, *PGROUPBOXCAPTIONSTYLE;

struct _GROUPBOX {
  RECT rc;
  LAYOUTBOXCONTENT hContent;
  PGROUPBOXCAPTIONSTYLE pCaptionStyle;
  LPWSTR lpszCaption;
  BOOL bFitContent;
};

void CreateLayoutBox(PLAYOUTBOX *, HWND);
void LayoutBox_AddControl(PLAYOUTBOX, HWND);
void LayoutBox_AddBox(PLAYOUTBOX, PLAYOUTBOX);
void LayoutBox_AddGroup(PLAYOUTBOX, PGROUPBOX);
void LayoutBox_SetPosition(PLAYOUTBOX, UINT, UINT);
void LayoutBox_SetSize(PLAYOUTBOX, UINT, UINT);
void LayoutBox_Update(PLAYOUTBOX);
void LayoutBox_Destroy(PLAYOUTBOX);
void DrawLayoutBox(HDC, PLAYOUTBOX);

void CreateGroupBox(PGROUPBOX *);
void GroupBox_SetLayoutBox(PGROUPBOX, PLAYOUTBOX);
void GroupBox_SetCaption(PGROUPBOX, LPWSTR);
void GroupBox_SetSize(PGROUPBOX, UINT, UINT);
void GroupBox_Update(PGROUPBOX);
void GroupBox_Destroy(PGROUPBOX);

#endif  /* PANITENT_FLEXIBLE_H */
