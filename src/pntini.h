#pragma once

typedef int (FnPntINICallbackFunction)(void* user, PCTSTR section, PCTSTR name, PCTSTR value, int lineno);

typedef struct PntINI PntINI;
struct PntINI {
    FnPntINICallbackFunction* pfnCallback;
};

int PntINI_ParseString(PWSTR* data, FnPntINICallbackFunction* pfnCallback, void* user);
int PntINI_ParseFile(PTSTR pszPath, FnPntINICallbackFunction* pfnCallback, void* user);

typedef enum PntINIContextBasedSourceType PntINIContextBasedSourceType;
enum PntINIContextBasedSourceType {
    PNTINI_SOURCE_UNKNOWN = 0,
    PNTINI_SOURCE_STRING,
    PNTINI_SOURCE_FILE
};

typedef PTSTR(FnPntINIReader)(PTSTR* str, int num, void* stream);

typedef struct PntINIContextBased PntINIContextBased;
struct PntINIContextBased {
    BOOL fResourceLoaded;
    FnPntINIReader* reader;
    void* source;
    PntINIContextBasedSourceType sourceType;

    int iLineNo;
    int iError;

    TCHAR szSection[256];
    PTSTR szName;
    PTSTR szValue;
    TCHAR szPrevName[256];

    size_t nMaxLine;

    PTSTR pszLine;
};

int PntINIContextBased_Init(PntINIContextBased* pPntINIContextBased);
PntINIContextBased* PntINIContextBased_Create();
void PntINIContextBased_LoadFile(PntINIContextBased* pPntINIContextBased, PCTSTR pszFilePath);
void PntINIContextBased_LoadString(PntINIContextBased* pPntINIContextBased, PCTSTR pszString);
int PntINIContextBased_ParseNext(PntINIContextBased* pCtx);
void PntINIContextBased_Destroy(PntINIContextBased* pPntINIContextBased);