#ifndef FILE_OPEN_H
#define FILE_OPEN_H

#define _UNICODE
#define UNICODEls

#define NTDDI_VERSION NTDDI_VISTA
#define _WINNT_WIN32 0600

#include <windows.h>
#include <initguid.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <propsys.h>

#define INDEX_BMP 1
#define INDEX_PNG 2
#define INDEX_JPG 3

#define CONTROL_GROUP           2000
#define CONTROL_RADIOBUTTONLIST 2
#define CONTROL_RADIOBUTTON1    1
#define CONTROL_RADIOBUTTON2    2

typedef struct _tagDialogEventHandler DialogEventHandler;

typedef struct {
    /* IUnknown methods */
    HRESULT (__stdcall *QueryInterface)(DialogEventHandler* This, REFIID riid,
        void **ppvObject);
    ULONG (__stdcall *AddRef)(DialogEventHandler* This);
    ULONG (__stdcall *Release)(DialogEventHandler* This);
    
    /* IFileDialogEvents methods */
    HRESULT (__stdcall *OnFileOk)(IFileDialog *);
    HRESULT (__stdcall *OnFolderChange)(IFileDialog *);
    HRESULT (__stdcall *OnFolderChanging)(IFileDialog *, IShellItem *);
    HRESULT (__stdcall *OnHelp)(IFileDialog *);
    HRESULT (__stdcall *OnSelectionChange)(IFileDialog *);
    HRESULT (__stdcall *OnShareViolation)(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *);
    HRESULT (__stdcall *OnTypeChange)(IFileDialog *pfd);
    HRESULT (__stdcall *OnOverwrite)(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *);
    
    /* IFileDialogControlEvents methods */
    HRESULT (__stdcall *OnItemSelected)(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem);
    HRESULT (__stdcall *OnButtonClicked)(IFileDialogCustomize *, DWORD);
    HRESULT (__stdcall *OnCheckButtonToggled)(IFileDialogCustomize *, DWORD, BOOL);
    HRESULT (__stdcall *OnControlActivating)(IFileDialogCustomize *, DWORD);
} DialogEventHandlerVtbl;

typedef struct _tagDialogEventHandler {
    CONST_VTBL DialogEventHandlerVtbl* lpVtbl;
    DWORD _cRef;
} DialogEventHandler;

int FileOpen(void);

#endif /* FILE_OPEN_H */
