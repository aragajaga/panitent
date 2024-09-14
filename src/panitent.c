#include "precomp.h"

#include "panitentapp.h"

#include "win32/window.h"

/* Entry point for application */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    int nArgs;
    PWSTR* pszArgList;
    PWSTR pszArgFile;
    if (pCmdLine && *pCmdLine != L'\0')
    {
        pszArgList = CommandLineToArgvW(pCmdLine, &nArgs);

        if (nArgs > 0)
        {
            pszArgFile = pszArgList[0];

            // MessageBox(NULL, pszArgFile, NULL, MB_OK | MB_ICONERROR);
        }
    }

    PanitentApp* pPanitentApp = PanitentApp_Instance();
    return PanitentApp_Run(pPanitentApp);
}
