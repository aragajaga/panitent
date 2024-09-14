#include "precomp.h"

#include <tchar.h>

#include "pntini.h"

#define BUFFER_SIZE 2048

/*
 * Strip whitespace characters off end of given string, in place.
 * 
 * @return PTSTR Just returns the szString argument.
 */
static PTSTR PntINI_RStrip(PTSTR szString)
{
    PTSTR p = szString + _tcslen(szString);

    while (p > szString && _istspace((unsigned short)(*--p)))
    {
        *p = _T('\0');
    }

    return szString;
}

/*
 * Return pointer to first non-whitespace character in given stirng.
 * 
 * @return PTSTR Pointer to a first non-whitespace character.
 */
static PTSTR PntINI_SkipWhitespace(PCTSTR szString)
{
    while (*szString && _istspace((unsigned short)(*szString)))
    {
        szString++;
    }

    return (PTSTR)szString;
}

static PTSTR* PntINI_FindCharsOrComment(PCTSTR s, PCTSTR chars)
{

    /* int was_space = 0; */
    while (*s && (!chars || !_tcschr(chars, *s)) && !(/* was_space && */ _tcschr(_T(";#"), *s)))
    {
        /* was_space = _istspace((unsigned short)(*s)); */
        ++s;
    }

    return (PTSTR)s;
}

/*
 * Similar to strncpy, but ensures dest (size bytes) is NUL-terminated, and doesn't pad with NULs.
 */
static PTSTR PntINI_strncpy0(PTSTR pszDest, PCTSTR pszSrc, size_t size)
{
    /* Could use strncpy internally, but it causes gcc warnings (see issue #91) */
    size_t i;
    for (i = 0; i < size - 1 && pszSrc[i]; i++)
    {
        pszDest[i] = pszSrc[i];
    }
    pszDest[i] = _T('\0');
    return pszDest;
}

typedef struct {
    PCTSTR ptr;
    size_t num_left;
} PntINI_ParseStringCtx;

int PntINI_ParseFile(PTSTR pszPath, FnPntINICallbackFunction* pfnCallback, void* user)
{
    FILE* fp = NULL;
    errno_t err = _tfopen_s(&fp, pszPath, _T("rb"));

    if (!fp)
    {
        return;
    }

    int result = PntINI_ParseStream(_fgetts, fp, pfnCallback, user);
    fclose(fp);
    return result;
}

int PntINI_ParseStream(FnPntINIReader* reader, void* stream, FnPntINICallbackFunction* pfnCallback, void* user)
{
    PntINI pntini = { 0 };
    pntini.pfnCallback = pfnCallback;

    PTSTR line = NULL;
    size_t max_line = BUFFER_SIZE;

    size_t offset = 0;
    
    int lineno = 0;
    int error = 0;


    TCHAR section[256] = _T("");
    TCHAR prev_name[256] = _T("");
    PTSTR name = NULL;
    PTSTR value = NULL;
    PTSTR start = line;
    PTSTR end = NULL;

    line = (PTSTR)malloc(BUFFER_SIZE * sizeof(TCHAR));
    if (!line)
    {
        return -2;
    }

    while (reader(line, 256, stream) != NULL)
    {
        offset = _tcslen(line);

        while (offset == max_line - 1 && line[offset - 1] != _T('\n'))
        {
            max_line *= 2;

            PTSTR new_line = realloc(line, max_line);
            if (!new_line)
            {
                free(line);
                return -2;
            }

            line = new_line;

            if (_fgetts(line + offset, (int)(max_line - offset), stream) == NULL)
            {
                break;
            }

            offset += _tcslen(line + offset);
        }

        ++lineno;
        start = line;

        /* Handle BOM */

        start = PntINI_SkipWhitespace(PntINI_RStrip(start));

        if (_tcschr(_T(";#"), *start))
        {
            /* Start-of-line comment */
        }

        else if (*start == _T('['))
        {
            /* A "[section]" line */
            end = PntINI_FindCharsOrComment(start + 1, _T("]"));
            if (*end == _T(']'))
            {
                *end = _T('\0');
                size_t section_len = end - start;
                PntINI_strncpy0(section, start + 1, section_len * sizeof(TCHAR));
                *prev_name = _T('\0');
            }
            else if (!error)
            {
                /* No ']' found on section line */
                error = lineno;
            }
        }
        else if (*start)
        {
            /* Not a comment, must be a name[=:]value pair */
            end = PntINI_FindCharsOrComment(start, _T("=:"));
            if (*end == _T('=') || *end == _T(':'))
            {
                *end = _T('\0');
                name = PntINI_RStrip(start);
                value = end + 1;

                /* Find inline comment */
                end = PntINI_FindCharsOrComment(value, NULL);
                if (*end)
                {
                    *end = _T('\0');
                }

                value = PntINI_SkipWhitespace(value);
                PntINI_RStrip(value);

                /* Valid name[=:]value pair found, call handler */
                PntINI_strncpy0(prev_name, name, sizeof(prev_name));
                if (!(*pntini.pfnCallback)(user, section, name, value, lineno) && !error)
                {
                    error = lineno;
                }
            }
            else if (!error)
            {
                /* No '=' or ':' found on name[=:]value line */
                /* INI_ALLOW_NO_VALUE */
                error = lineno;
            }
        }

        if (error)
        {
            break;
        }
    }

    free(line);
    return error;
}

static PTSTR _tsgetts(PTSTR str, int num, PTSTR* strp)
{
    if (**strp == _T('\0'))
    {
        return NULL;
    }

    int i;

    for (i = 0; i < num - 1; ++i, ++(*strp))
    {
        str[i] = **strp;

        if (**strp == _T('\0'))
        {
            break;
        }

        if (**strp == _T('\n'))
        {
            str[i + 1] = _T('\0');
            ++(*strp);
            break;
        }
    }

    if (i == num - 1)
    {
        str[i] = _T('\0');
    }

    return str;
}

static PTSTR _tsgetts2(PTSTR str, int num, void* stream)
{
    PntINI_ParseStringCtx* ctx = (PntINI_ParseStringCtx*)stream;

    PCTSTR ctx_ptr = ctx->ptr;
    size_t ctx_num_left = ctx->num_left;
    PTSTR strp = str;
    TCHAR c;

    if (ctx_num_left == 0 || num < 2)
    {
        return NULL;
    }

    while (num > 1 && ctx_num_left != 0)
    {
        c = *ctx_ptr++;
        ctx_num_left--;
        *strp++ = c;

        if (c == _T('\n'))
        {
            break;
        }

        --num;
    }

    *strp = _T('\0');
    ctx->ptr = ctx_ptr;
    ctx->num_left = ctx_num_left;
    return str;
}

int PntINI_ParseString(PWSTR* data, FnPntINICallbackFunction* pfnCallback, void* user)
{
    PntINI_ParseStringCtx ctx;

    ctx.ptr = data;
    ctx.num_left = wcslen(data);

    return PntINI_ParseStream(_tsgetts2, &ctx, pfnCallback, user);
}

int PntINIContextBased_Init(PntINIContextBased* pPntINIContextBased)
{
    static const size_t len = BUFFER_SIZE * sizeof(TCHAR);
    PTSTR pszLine = (PTSTR)malloc(len);

    if (!pszLine)
    {
        return -2;
    }
    memset(pszLine, 0, len);
    
    pPntINIContextBased->pszLine = pszLine;
    pPntINIContextBased->nMaxLine = BUFFER_SIZE;
    return 0;
}

PntINIContextBased* PntINIContextBased_Create()
{
    PntINIContextBased* pPntINIContextBased = (PntINIContextBased*)malloc(sizeof(PntINIContextBased));

    if (pPntINIContextBased)
    {
        memset(pPntINIContextBased, 0, sizeof(PntINIContextBased));
        if (PntINIContextBased_Init(pPntINIContextBased))
        {
            /* Free, if initialization error occurred */
            free(pPntINIContextBased);
            return NULL;
        }        
    }

    return pPntINIContextBased;
}

void PntINIContextBased_LoadFile(PntINIContextBased* pPntINIContextBased, PCTSTR pszFilePath)
{
    if (!pPntINIContextBased || pPntINIContextBased->fResourceLoaded)
    {
        return;
    }

    FILE* fp = NULL;
    errno_t err = _tfopen_s(&fp, pszFilePath, _T("rb"));
    if (!fp)
    {
        return;
    }

    pPntINIContextBased->reader = _fgetts;
    pPntINIContextBased->source = fp;

    pPntINIContextBased->fResourceLoaded = TRUE;
    pPntINIContextBased->sourceType = PNTINI_SOURCE_FILE;
}

void PntINIContextBased_LoadString(PntINIContextBased* pPntINIContextBased, PCTSTR pszString)
{
    if (!pPntINIContextBased || pPntINIContextBased->fResourceLoaded)
    {
        return;
    }

    PntINI_ParseStringCtx* pStringCtx = (PntINI_ParseStringCtx*)malloc(sizeof(PntINI_ParseStringCtx));
    if (!pStringCtx)
    {
        return;
    }

    pStringCtx->ptr = pszString;
    pStringCtx->num_left = _tcslen(pszString);

    pPntINIContextBased->reader = _tsgetts2;
    pPntINIContextBased->source = pStringCtx;

    pPntINIContextBased->fResourceLoaded = TRUE;
    pPntINIContextBased->sourceType = PNTINI_SOURCE_STRING;
}

static const TCHAR g_kCommentChars[] = _T(";#");

int PntINIContextBased_ParseNext(PntINIContextBased* pCtx)
{
    FnPntINIReader* pfnReader = pCtx->reader;

    PTSTR pszLine = pCtx->pszLine;
    PTSTR pszStart = NULL;
    PTSTR pszEnd = NULL;

    while (pfnReader(pszLine, pCtx->nMaxLine, pCtx->source))
    {
        size_t nOffset = 0;
        /* Run till EOL */
        while (nOffset == pCtx->nMaxLine - 1 && pszLine[nOffset - 1] != _T('\n'))
        {
            pCtx->nMaxLine *= 2;

            PTSTR pszNewLine = realloc(pszLine, pCtx->nMaxLine);
            if (!pszNewLine)
            {
                free(pszLine);
                pCtx->pszLine = NULL;
                /* Memory allocation error, return */
                return -2;
            }
            pszLine = pszNewLine;
            pCtx->pszLine = pszLine;

            size_t newBytesRead = pCtx->reader(pszLine + nOffset, (int)(pCtx->nMaxLine - nOffset), pCtx->source);
            if (!newBytesRead)
            {
                break;
            }

            // pCtx->nOffset += newBytesRead;
            // pCtx->pszLine[pCtx->nOffset] = _T('\0');
            nOffset += _tcslen(pszLine + nOffset);
        }

        ++pCtx->iLineNo;
        pszStart = pszLine;

        pszStart = PntINI_SkipWhitespace(PntINI_RStrip(pszStart));

        if (_tcschr(g_kCommentChars, *pszStart))
        {
            /* Start-of-line comment, skip */
        }
        else if (*pszStart == _T('['))
        {
            /* A "[section]" line */
            pszEnd = PntINI_FindCharsOrComment(pszStart + 1, _T("]"));
            if (*pszEnd == _T(']'))
            {
                *pszEnd = _T('\0');
                size_t section_len = pszEnd - pszStart;
                PntINI_strncpy0(pCtx->szSection, pszStart + 1, section_len * sizeof(TCHAR));
                *pCtx->szPrevName = _T('\0');
            }
            else if (!pCtx->iError)
            {
                /* No ']' found on section line */
                pCtx->iError = pCtx->iLineNo;
            }
        }
        else if (*pszStart)
        {
            /* Not a comment, must be a name[=:]value pair */
            pszEnd = PntINI_FindCharsOrComment(pszStart, _T("=:"));
            if (*pszEnd == _T('=') || *pszEnd == _T(':'))
            {
                *pszEnd = _T('\0');
                pCtx->szName = PntINI_RStrip(pszStart);
                pCtx->szValue = pszEnd + 1;

                /* Find inline comment */
                pszEnd = PntINI_FindCharsOrComment(pCtx->szValue, NULL);
                if (*pszEnd)
                {
                    *pszEnd = _T('\0');
                }

                pCtx->szValue = PntINI_SkipWhitespace(pCtx->szValue);
                PntINI_RStrip(pCtx->szValue);

                /* Valid name[=:]value pair found, call handler */
                PntINI_strncpy0(pCtx->szPrevName, pCtx->szName, sizeof(pCtx->szPrevName));


                return 0;
                /*
                if (!(*pntini.pfnCallback)(user, pCtx->szSection, pCtx->szName, pCtx->szValue, pCtx->iLineNo) && !pCtx->iError)
                {
                    pCtx->iError = pCtx->iLineNo;
                }
                */
            }
            else if (!pCtx->iError)
            {
                /* No '=' or ':' found on name[=:]value line */
                pCtx->iError = pCtx->iLineNo;
            }
        }
    }

    return EOF;
}

void PntINIContextBased_Destroy(PntINIContextBased* pPntINIContextBased)
{
    /* Not initialized, return */
    if (!pPntINIContextBased->fResourceLoaded || pPntINIContextBased == PNTINI_SOURCE_UNKNOWN)
    {
        return;
    }

    /* Close file if source stream type is a file */
    if (pPntINIContextBased->sourceType == PNTINI_SOURCE_FILE)
    {
        FILE* fp = (FILE*)pPntINIContextBased->source;
        if (fp)
        {
            fclose(pPntINIContextBased->source);
        }
    }
    else if (pPntINIContextBased->sourceType == PNTINI_SOURCE_STRING && pPntINIContextBased->source)
    {
        free(pPntINIContextBased->source);
    }

    /* Make dirty */
    pPntINIContextBased->sourceType = PNTINI_SOURCE_UNKNOWN;
    pPntINIContextBased->fResourceLoaded = FALSE;
}
