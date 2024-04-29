/**
 * pntxml - a fork of ooxi/xml.c adapted for using with WinAPI and
 *     wide-character strings
 *
 * CHANGES:
 * - uint8_t, char, etc. replaced with wide-character PWSTR, PCWSTR and WCHAR
 * - Symbols renamed to distinguish it from original xml.c and accomplish
     Panit.ent coding style
 * - Fixed common typos in comments
 *
 * Copyright (c) 2012 ooxi/xml.c
 *     https://github.com/ooxi/xml.c
 *
 * Copyright (c) 2023 Aragajaga
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software in a
 *     product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source distribution.
 */

#include "precomp.h"

#include "pntxml.h"

#include <stdio.h>
#include <stddef.h>

/**
 * Public domain strtok_r by Charlie Gordon
 *
 * from comp.lang.c  9/14/2007
 *
 * http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 * 
 * (Declaration that it's public domain):
 * http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */
static PWSTR XML_strtok_r(PWSTR pszStr, PCWSTR pszDelim, PCWSTR* ppszNextp)
{
    PWSTR pszRet;

    if (!pszStr)
    {
        pszStr = *ppszNextp;
    }

    pszStr += wcsspn(pszStr, pszDelim);

    if (*pszStr == L'\0')
    {
        return NULL;
    }

    pszRet = pszStr;

    pszStr += wcscspn(pszStr, pszDelim);

    if (*pszStr)
    {
        *pszStr++ = L'\0';
    }

    *ppszNextp = pszStr;

    return pszRet;
}

/**
 * [OPAQUE API]
 * 
 * UTF-8 text 
 */
typedef struct XMLString XMLString;
struct XMLString {
    PCWSTR pszBuffer;
    size_t nLength;
};

/**
 * [OPAQUE API]
 *
 * An XMLAttribute may contain text content. 
 */
typedef struct XMLAttribute XMLAttribute;
struct XMLAttribute {
    XMLString* pxsName;
    XMLString* pxsContent;
};

/**
 * [OPAQUE API]
 *
 * An XMLNode will always contain a tag name, a 0-ternimated list of attributes
 * and a 0-terminated list of children. Moreover it node may contain text content.
 */
typedef struct XMLNode XMLNode;
struct XMLNode {
    XMLString* pxsName;
    XMLString* pxsContent;
    XMLAttribute** ppAttributes;
    XMLNode** ppChildren;
};

/**
 * [OPAQUE API]
 * 
 * An XMLDocument simply contains the root node and the underlying buffer
 */
typedef struct XMLDocument XMLDocument;
struct XMLDocument {
    struct {
        PWSTR pszBuffer;
        size_t length;
    } buffer;

    XMLNode* pRoot;
};

/**
 * [PRIVATE]
 *
 * Parser context
 */
typedef struct XMLParser XMLParser;
struct XMLParser {
    PWSTR pszBuffer;
    size_t nPosition;
    size_t nLength;
};

/**
 * [PRIVATE]
 * 
 * Character offsets
 */
typedef enum XMLParserOffset XMLParserOffset;
enum XMLParserOffset {
    NO_CHARACTER = -1,
    CURRENT_CHARACTER = 0,
    NEXT_CHARACTER = 1,
};

/**
 * [PRIVATE]
 * 
 * @return Number of attributes in 0-terminated array 
 */
static size_t XML_GetAttributeCount(XMLAttribute** ppAttributes)
{
    size_t nElements = 0;

    while (ppAttributes[nElements])
    {
        ++nElements;
    }

    return nElements;
}

/**
 * [PRIVATE]
 * 
 * @return Number of nodes in 0-terminated array
 */
static size_t XML_GetNodeCount(XMLAttribute** ppNodes)
{
    size_t nElements = 0;

    while (ppNodes[nElements])
    {
        ++nElements;
    }

    return nElements;
}

/**
 * [PRIVATE]
 *
 * @warning No UTF conversions will be attempted
 * 
 * @return true if a == b
 */
static BOOL XMLString_Equals(XMLString* a, XMLString* b)
{
    if (a->nLength != b->nLength)
    {
        return FALSE;
    }

    for (size_t i = 0; i < a->nLength; ++i)
    {
        if (a->pszBuffer[i] != b->pszBuffer[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * [PRIVATE]
 * 
 * Clone XMLString to raw pXMLString
 * 
 * @return Raw pXMLString pointer, otherwise NULL if failed
 */
static PWSTR XMLString_Clone(XMLString* pXMLString)
{
    if (!pXMLString)
    {
        return 0;
    }

    PWSTR pszClone = (PWSTR)malloc((pXMLString->nLength + 1) * sizeof(WCHAR));
    
    if (pszClone)
    {
        memset(pszClone, 0, (pXMLString->nLength + 1) * sizeof(WCHAR));

        XMLString_Copy(pXMLString, pszClone, pXMLString->nLength);
        pszClone[pXMLString->nLength] = 0;

        return pszClone;
    }
    

    return NULL;
}

/**
 * [PRIVATE]
 * 
 * Frees the resources allocated by the string
 * 
 * @warning `buffer` must _not_ be freed, since it is a reference to the
 *     document's buffer
 */
static void XMLString_Free(XMLString* pXMLString)
{
    free(pXMLString);
}

/**
 * [PRIVATE]
 * 
 * Frees the resources allocated by the pXMLAttribute
 */
static void XMLAttribute_Free(XMLAttribute* pXMLAttribute)
{
    if (pXMLAttribute->pxsName)
    {
        XMLString_Free(pXMLAttribute->pxsName);
    }

    if (pXMLAttribute->pxsContent)
    {
        XMLString_Free(pXMLAttribute->pxsContent);
    }

    free(pXMLAttribute);
}

/**
 * [PRIVATE]
 *
 * Frees the resources allocated by the node
 */
static void XMLNode_Free(XMLNode* pNode)
{
    XMLString_Free(pNode->pxsName);

    if (pNode->pxsContent)
    {
        XMLString_Free(pNode->pxsContent);
    }

    XMLAttribute** ppAttribute = pNode->ppAttributes;
    while (*ppAttribute)
    {
        XMLAttribute_Free(*ppAttribute);
        ++ppAttribute;
    }
    free(pNode->ppAttributes);

    XMLNode** ppChildNode = pNode->ppChildren;
    while (*ppChildNode)
    {
        XMLNode_Free(*ppChildNode);
        ++ppChildNode;
    }
    free(pNode->ppChildren);

    free(pNode);
}

/**
 * [PRIVATE]
 *
 * Echos the parsers call stack for debugging purposes
 */
#ifdef XML_PARSER_VERBOSE
static void XMLParser_Info(XMLParser* pXMLParser, PWSTR pszMessage)
{
    fwprintf_s(stdout, L"XMLParser_Info %pXMLString\n", pszMessage);
}
#else
#define XMLParser_Info(parser, pszMessage) {}
#endif

/**
 * [PRIVATE]
 * 
 * Echos an error regarding the parser's source to the console
 */
static void XMLParser_Error(XMLParser* pXMLParser, XMLParserOffset offset, PCWSTR pszMessage)
{
    int nRow = 0;
    int nColumn = 0;

    #define min(X,Y) ((X) < (Y) ? (X) : (Y))
    #define MaxOf(X,Y) ((X) > (Y) ? (X) : (Y))
    size_t nCharacter = MaxOf(0, min(pXMLParser->nLength, pXMLParser->nPosition + offset));
    #undef min
    #undef max

    for (size_t nPosition = 0; nPosition < nCharacter; ++nPosition)
    {
        nColumn++;

        if ('\n' == pXMLParser->pszBuffer[nPosition])
        {
            nRow++;
            nColumn = 0;
        }
    }

    if (NO_CHARACTER != offset)
    {
        fwprintf_s(stderr, L"XMLParser_Error at %i:%i (is %c): %s\n",
            nRow + 1, nColumn, pXMLParser->pszBuffer[nCharacter], pszMessage
        );
    }
    else {
        fwprintf_s(stderr, L"XMLParser_Error at %i:%i: %s\n",
            nRow + 1, nColumn, pszMessage
        );
    }
}

/**
 * [PRIVATE]
 * 
 * Returns the n-th not-whitespace byte in parser and 0 if such a byte does not
 * exist
 */
static WCHAR XMLParser_Peek(XMLParser* pXMLParser, size_t n)
{
    size_t nPosition = pXMLParser->nPosition;

    while (nPosition < pXMLParser->nLength)
    {
        if (!isspace(pXMLParser->pszBuffer[nPosition]))
        {
            if (n == 0)
            {
                return pXMLParser->pszBuffer[nPosition];
            }
            else {
                --n;
            }
        }

        nPosition++;
    }

    return 0;
}

/**
 * [PRIVATE]
 *
 * Moves the parser's position n bytes. If the new position would be out of
 * bounds, it will be converted to the bounds itself
 */
static void XMLParser_Consume(XMLParser* pXMLParser, size_t n)
{
    /* Debug information */
    #ifdef XML_PARSER_VERBOSE
    #define min(X,Y) ((X) < (Y) ? (X) : (Y))

    PWSTR pszConsumed = alloca((n + 1) * sizeof(WCHAR));
    memcpy(pszConsumed, &pXMLParser->pszBuffer[pXMLParser->nPosition], min(n, pXMLParser->nLength - pXMLParser->nPosition));
    pszConsumed[n] = 0;
    #undef min

    size_t nMessageBufferLength = 512;
    PWSTR pszMessageBuffer = alloca(512 * sizeof(WCHAR));
    swprintf_s(pszMessageBuffer, nMessageBufferLength, L"Consuming %li bytes \"%s\"", (long)n, pszConsumed);
    pszMessageBuffer[nMessageBufferLength - 1] = 0;

    XMLParser_Info(pXMLParser, pszMessageBuffer);
    #endif

    /* Move the position forward */
    pXMLParser->nPosition += n;

    /* Don't go too far
     * 
     * @warning Valid because pXMLParser->nLength must be greater than 0
     */
    if (pXMLParser->nPosition >= pXMLParser->nLength)
    {
        pXMLParser->nPosition = pXMLParser->nLength - 1;
    }
}

/**
 * [PRIVATE]
 * 
 * Skips to the next non-whitespace character
 */
static void XMLParser_SkipWhitespace(XMLParser* pXMLParser)
{
    XMLParser_Info(pXMLParser, L"whilespace");

    while (isspace(pXMLParser->pszBuffer[pXMLParser->nPosition]))
    {
        if (pXMLParser->nPosition + 1 >= pXMLParser->nLength)
        {
            return;
        }
        else {
            pXMLParser->nPosition++;
        }
    }
}

/**
 * [PRIVATE]
 * 
 * Finds and creates all attributes on the given node.
 * 
 * @author Blake Felt
 * @see http://github.com/Molorius
 */
static XMLAttribute** XMLParser_FindAttributes(XMLParser* pXMLParser, XMLString* pxsTagOpen)
{
    XMLParser_Info(pXMLParser, L"XMLParser_FindAttributes");

    PWSTR pszTemp;
    PWSTR pszRest = NULL;
    PWSTR pszToken;
    PWSTR pszName;
    PWSTR pszContent;
    PCWSTR pszStartName;
    PCWSTR pszStartContent;
    size_t nOldElements;
    size_t nNewElements;
    XMLAttribute* pNewAttribute;
    XMLAttribute** ppAttributes;
    int nPosition;

    ppAttributes = (XMLAttribute**)malloc(sizeof(XMLAttribute*));
    if (!ppAttributes) {
        /* Failed to allocate memory */
        return NULL;
    }

    ppAttributes[0] = NULL;

    pszTemp = (PWSTR)XMLString_Clone(pxsTagOpen);

    /* Skip the first value */
    pszToken = XML_strtok_r(pszTemp, L" ", &pszRest);
    if (!pszToken)
    {
        goto cleanup;
    }
    pxsTagOpen->nLength = wcslen(pszToken);

    for (pszToken = XML_strtok_r(NULL, L" ", &pszRest); pszToken != NULL; pszToken = XML_strtok_r(NULL, L" ", &pszRest))
    {
        pszName = (PWSTR)malloc((wcslen(pszToken) + 1) * sizeof(WCHAR));
        pszContent = (PWSTR)malloc((wcslen(pszToken) + 1) * sizeof(WCHAR));
        /* %s=\"%s\" wasn't working for some reason, ugly hack to make it work */
        if (swscanf_s(pszToken, L"%[^=]=\"%[^\"]", pszName, pszContent) != 2)
        {
            if (swscanf_s(pszToken, L"%[^=]=\'%[^\']", pszName, pszContent) != 2)
            {
                free(pszName);
                free(pszContent);
                continue;
            }
        }
        nPosition = pszToken - pszTemp;
        pszStartName = &pxsTagOpen->pszBuffer[nPosition];
        pszStartContent = &pxsTagOpen->pszBuffer[nPosition + wcslen(pszName) + 2];

        pNewAttribute = (XMLAttribute*)malloc(sizeof(XMLAttribute));

        pNewAttribute->pxsName = (XMLString*)malloc(sizeof(XMLString));
        pNewAttribute->pxsName->pszBuffer = (PWSTR)pszStartName;
        pNewAttribute->pxsName->nLength = wcslen(pszStartName);

        pNewAttribute->pxsContent = (XMLString*)malloc(sizeof(XMLString));
        pNewAttribute->pxsContent->pszBuffer = (PWSTR)pszStartContent;
        pNewAttribute->pxsContent->nLength = wcslen(pszStartContent);

        nOldElements = XML_GetAttributeCount(ppAttributes);
        nNewElements = nOldElements + 1;
        ppAttributes = (XMLAttribute**)realloc(ppAttributes, (nNewElements + 1) * sizeof(XMLAttribute*));

        ppAttributes[nNewElements - 1] = pNewAttribute;
        ppAttributes[nNewElements] = 0;

        free(pszName);
        free(pszContent);
    }

cleanup:
    free(pszTemp);
    return ppAttributes;
}

/**
 * [PRIVATE]
 * 
 * Parses the pxsTagName out of the an XML tag'pXMLString ending
 * 
 * ---( Example )---
 * tag_name>
 * ---
 */
static XMLString* XMLParser_ParseTagEnd(XMLParser* pXMLParser)
{
    XMLParser_Info(pXMLParser, L"XMLParser_ParseTagEnd");
    size_t nStart = pXMLParser->nPosition;
    size_t nLength = 0;

    /* Parse until `>' or a whitespace is reached */
    while (nStart + nLength < pXMLParser->nLength)
    {
        WCHAR chCurrent = XMLParser_Peek(pXMLParser, CURRENT_CHARACTER);

        if ((L'>' == chCurrent) || isspace(chCurrent))
        {
            break;
        }
        else {
            XMLParser_Consume(pXMLParser, 1);
            nLength++;
        }
    }

    /* Consume `>' */
    if (L'>' != XMLParser_Peek(pXMLParser, CURRENT_CHARACTER))
    {
        XMLParser_Error(pXMLParser, CURRENT_CHARACTER, L"XMLParser_ParseTagEnd: Expected tag end");
        return 0;
    }
    XMLParser_Consume(pXMLParser, 1);

    /* Return parsed tag pxsTagName */
    XMLString* pxsTagName = (XMLString*)malloc(sizeof(XMLString));
    pxsTagName->pszBuffer = &pXMLParser->pszBuffer[nStart];
    pxsTagName->nLength = nLength;
    return pxsTagName;
}

/**
 * [PRIVATE]
 *
 * Parses an opening XML tag without attributes
 *
 * ---( Example )---
 * <tag_name>
 * ---
 */
static XMLString* XMLParser_ParseTagOpen(XMLParser* pXMLParser)
{
    XMLParser_Info(pXMLParser, L"XMLParser_ParseTagOpen");

    XMLParser_SkipWhitespace(pXMLParser);

    /* Consume `<' */
    if (L'<' != XMLParser_Peek(pXMLParser, CURRENT_CHARACTER))
    {
        XMLParser_Error(pXMLParser, CURRENT_CHARACTER, L"XMLParser_ParseTagOpen: Expected opening tag");
        return 0;
    }
    XMLParser_Consume(pXMLParser, 1);

    /* Consume tag name */
    return XMLParser_ParseTagEnd(pXMLParser);
}

/**
 * [PRIVATE]
 *
 * Parses an closing XML tag without attributes
 *
 * ---( Example )---
 * </tag_name>
 * ---
 */
static XMLString* XMLParser_ParseTagClose(XMLParser* pXMLParser)
{
    XMLParser_Info(pXMLParser, L"XMLParser_ParseTagClose");

    XMLParser_SkipWhitespace(pXMLParser);

    /* Consume `</' */
    if ((L'<' != XMLParser_Peek(pXMLParser, CURRENT_CHARACTER)) ||
        (L'/' != XMLParser_Peek(pXMLParser, NEXT_CHARACTER)))
    {
        if (L'<' != XMLParser_Peek(pXMLParser, CURRENT_CHARACTER))
        {
            XMLParser_Error(pXMLParser, CURRENT_CHARACTER, L"XMLParser_ParseTagClose: Expected closing tag `<'");
        }

        if (L'/' != XMLParser_Peek(pXMLParser, NEXT_CHARACTER))
        {
            XMLParser_Error(pXMLParser, NEXT_CHARACTER, L"XMLParser_ParseTagClose: expected closing tag `/'");
        }
        
        return 0;
    }
    XMLParser_Consume(pXMLParser, 2);

    /* Consume tag pxsTagName */
    return XMLParser_ParseTagEnd(pXMLParser);
}

/**
 * [PRIVATE]
 *
 * Parses a tag's content
 *
 * ---( Example )---
 *     this is
 *   a
 *       tag {} content
 * ---
 *
 * @warning CDATA etc. is _not_ and will never be supported
 */
static XMLString* XMLParser_ParseContent(XMLParser* pXMLParser)
{
    XMLParser_Info(pXMLParser, L"XMLParser_ParseContent");

    /* Whitespace will be ignored */
    XMLParser_SkipWhitespace(pXMLParser);

    size_t nStart = pXMLParser->nPosition;
    size_t nLength = 0;

    /* Consume until `<' is reached */
    while (nStart + nLength < pXMLParser->nLength)
    {
        WCHAR current = XMLParser_Peek(pXMLParser, CURRENT_CHARACTER);

        if (L'<' == current)
        {
            break;
        }
        else {
            XMLParser_Consume(pXMLParser, 1);
            nLength++;
        }
    }

    /* Next character must be an `<' or we have reached end of file */
    if (L'<' != XMLParser_Peek(pXMLParser, CURRENT_CHARACTER))
    {
        XMLParser_Error(pXMLParser, CURRENT_CHARACTER, L"XMLParser_ParseContent: Expected <");
        return 0;
    }

    /* Ignore tailing whitespace */
    while ((nLength > 0) && isspace(pXMLParser->pszBuffer[nStart + nLength - 1]))
    {
        nLength--;
    }

    /* Return text */
    XMLString* pxsContent = (XMLString*)malloc(sizeof(XMLString));
    pxsContent->pszBuffer = &pXMLParser->pszBuffer[nStart];
    pxsContent->nLength = nLength;
    return pxsContent;
}

/**
 * [PRIVATE]
 *
 * Parses an XML fragment node
 *
 * ---( Example without children )---
 * <Node>Text</Node>
 * ---
 *
 * ---( Example with children )---
 * <Parent>
 *     <Child>Text</Child>
 *     <Child>Text</Child>
 *     <Test>Content</Test>
 * </Parent>
 * ---
 */
static XMLNode* XMLParser_ParseNode(XMLParser* pXMLParser)
{
    XMLParser_Info(pXMLParser, L"XMLParser_ParseNode");

    /* Setup variables */
    XMLString* pxsTagOpen = NULL;
    XMLString* pxsTagClose = NULL;
    XMLString* pxsContent = NULL;

    size_t nOriginalLength;
    XMLAttribute** ppAttributes;

    XMLNode** ppChildren = (XMLNode**)malloc(sizeof(XMLNode*));
    ppChildren[0] = 0;

    /* Parse open tag */
    pxsTagOpen = XMLParser_ParseTagOpen(pXMLParser);
    if (!pxsTagOpen)
    {
        XMLParser_Error(pXMLParser, NO_CHARACTER, L"XMLParser_ParseNode: tag open");
        goto exit_failure;
    }

    nOriginalLength = pxsTagOpen->nLength;
    ppAttributes = XMLParser_FindAttributes(pXMLParser, pxsTagOpen);

    /* If tag ends with `/' it's self closing, skip content lookup */
    if (pxsTagOpen->nLength > 0 && L'/' == pxsTagOpen->pszBuffer[nOriginalLength - 1])
    {
        /* Drop `/' */
        goto node_creation;
    }

    /* If the content does not start with '<', a text content is assumed */
    if (L'<' != XMLParser_Peek(pXMLParser, CURRENT_CHARACTER))
    {
        pxsContent = XMLParser_ParseContent(pXMLParser);

        if (!pxsContent)
        {
            XMLParser_Error(pXMLParser, 0, L"XMLParser_ParseNode: content");
            goto exit_failure;
        }
    }
    /* Otherwise ppChildren are to be expected */
    else {
        while (L'/' != XMLParser_Peek(pXMLParser, NEXT_CHARACTER))
        {
            /* Parse child pNode */
            XMLNode* pChildNode = XMLParser_ParseNode(pXMLParser);
            if (!pChildNode)
            {
                XMLParser_Error(pXMLParser, NEXT_CHARACTER, L"XMLParser_ParseNode: child");
                goto exit_failure;
            }

            /* Grow child array */
            size_t nOldElements = XML_GetNodeCount(ppChildren);
            size_t nNewElements = nOldElements + 1;
            ppChildren = (XMLNode**)realloc(ppChildren, (nNewElements + 1) * sizeof(XMLNode*));

            /* Save chld */
            ppChildren[nNewElements - 1] = pChildNode;
            ppChildren[nNewElements] = 0;
        }
    }

    /* Parse close tag */
    pxsTagClose = XMLParser_ParseTagClose(pXMLParser);
    if (!pxsTagClose)
    {
        XMLParser_Error(pXMLParser, NO_CHARACTER, L"XMLParser_ParseNode: tag close");
        goto exit_failure;
    }

    /* Close tag has to match open tag */
    if (!XMLString_Equals(pxsTagOpen, pxsTagClose))
    {
        XMLParser_Error(pXMLParser, NO_CHARACTER, L"XMLParser_ParseNode: tag missmatch");
        goto exit_failure;
    }

    /* Return parsed node */
    XMLString_Free(pxsTagClose);

node_creation:;
    XMLNode* pXMLNode = (XMLNode*)malloc(sizeof(XMLNode));
    pXMLNode->pxsName = pxsTagOpen;
    pXMLNode->pxsContent = pxsContent;
    pXMLNode->ppAttributes = ppAttributes;
    pXMLNode->ppChildren = ppChildren;
    return pXMLNode;

    /* A failure occured, so free all allocated resources */
exit_failure:
    if (pxsTagOpen)
    {
        XMLString_Free(pxsTagOpen);
    }
    if (pxsTagClose)
    {
        XMLString_Free(pxsTagClose);
    }
    if (pxsContent)
    {
        XMLString_Free(pxsContent);
    }

    XMLNode** pChild = ppChildren;
    while (*pChild)
    {
        XMLNode_Free(*pChild);
        ++pChild;
    }
    free(ppChildren);

    return 0;
}

/**
 * [PUBLIC API]
 */
XMLDocument* XML_ParseDocument(PWSTR pszBuffer, size_t nLength)
{
    /* Initialize parser */
    XMLParser parser = {
        .pszBuffer = pszBuffer,
        .nPosition = 0,
        .nLength = nLength
    };

    /* An empty buffer can never contain a valid document */
    if (!nLength)
    {
        XMLParser_Error(&parser, NO_CHARACTER, L"XML_ParseDocument: length equals zero");
        return 0;
    }

    /* Parse the root node */
    XMLNode* pRoot = XMLParser_ParseNode(&parser);
    if (!pRoot)
    {
        XMLParser_Error(&parser, NO_CHARACTER, L"XML_ParseDocument: parsing pXMLDocument failed");
        return 0;
    }

    /* Return parsed pXMLDocument */
    XMLDocument* pXMLDocument = (XMLDocument*)malloc(sizeof(XMLDocument));
    pXMLDocument->buffer.pszBuffer = pszBuffer;
    pXMLDocument->buffer.length = nLength;
    pXMLDocument->pRoot = pRoot;

    return pXMLDocument;
}

/**
 * [PUBLIC API]
 */
XMLDocument* XML_OpenDocument(FILE* pfSource)
{
    /* Prepare buffer */
    size_t const nReadChunk = 1; // TODO 4096

    size_t nDocumentLength = 0;
    size_t nBufferSize = 1; // TODO 4096
    PWSTR pszBuffer = (PWSTR)malloc(nBufferSize * sizeof(WCHAR));

    /* Read whole file into buffer */
    while (!feof(pfSource))
    {
        /* Reallocate buffer */
        if (nBufferSize - nDocumentLength < nReadChunk)
        {
            pszBuffer = (PWSTR)realloc(pszBuffer, nBufferSize + 2 * nReadChunk);
            nBufferSize += 2 * nReadChunk;
        }

        size_t nRead = fread(
            &pszBuffer[nDocumentLength],
            sizeof(WCHAR), nReadChunk,
            pfSource
        );

        nDocumentLength += nRead;
    }
    fclose(pfSource);

    /* Try to parse buffer */
    XMLDocument* pXMLDocument = XML_ParseDocument(pszBuffer, nDocumentLength);

    if (!pXMLDocument)
    {
        free(pszBuffer);
        return 0;
    }

    return pXMLDocument;
}

/**
 * [PUBLIC API]
 */
void XMLDocument_Free(XMLDocument* document, BOOL bFreeBuffer)
{
    XMLNode_Free(document->pRoot);

    if (bFreeBuffer)
    {
        free(document->buffer.pszBuffer);
    }

    free(document);
}

/**
 * [PUBLIC API]
 */
XMLNode* XMLDocument_GetRoot(XMLDocument* pXMLDocument)
{
    return pXMLDocument->pRoot;
}

/**
 * [PUBLIC API]
 */
XMLString* XMLNode_GetName(XMLNode* pXMLNode)
{
    return pXMLNode->pxsName;
}

/**
 * [PUBLIC API]
 */
XMLString* XMLNode_GetContent(XMLNode* pXMLNode)
{
    return pXMLNode->pxsContent;
}

/**
 * [PUBLIC API]
 * 
 * @warning O(n)
 */
size_t XMLNode_GetChildrenCount(XMLNode* pXMLNode)
{
    return XML_GetNodeCount(pXMLNode->ppChildren);
}

/**
 * [PUBLIC API]
 */
XMLNode* XMLNode_GetChild(XMLNode* pXMLNode, size_t nChild)
{
    if (nChild >= XMLNode_GetChildrenCount(pXMLNode))
    {
        return 0;
    }

    return pXMLNode->ppChildren[nChild];
}

/**
 * [PUBLIC API]
 */
size_t XMLNode_GetAttributesCount(XMLNode* pXMLNode)
{
    return XML_GetAttributeCount(pXMLNode->ppAttributes);
}

/**
 * [PUBLIC API]
 */
XMLString* XMLNode_GetAttributeName(XMLNode* pXMLNode, size_t nAttribute)
{
    if (nAttribute >= XMLNode_GetAttributesCount(pXMLNode))
    {
        return 0;
    }

    return pXMLNode->ppAttributes[nAttribute]->pxsName;
}

/**
 * [PUBLIC API]
 */
XMLString* XMLNode_GetAttributeContent(XMLNode* pNode, size_t nAttribute)
{
    if (nAttribute >= XMLNode_GetAttributesCount(pNode))
    {
        return 0;
    }

    return pNode->ppAttributes[nAttribute]->pxsContent;
}

/**
 * [PUBLIC API]
 */
XMLNode* XMLNode_EasyChild(XMLNode* pNodeParent, PCWSTR pszChildName, ...)
{
    /* Find children, one by one */
    XMLNode* pCurrentNode = pNodeParent;

    va_list arguments;
    va_start(arguments, pszChildName);

    /* Descent to chCurrent child */
    while (pszChildName)
    {
        /* Convert pszChildName to XMLString for easy comparison */
        XMLString xsChildName = {
            .pszBuffer = pszChildName,
            .nLength = wcslen(pszChildName)
        };

        /* Iterate through all children */
        XMLNode* pNextNode = NULL;

        for (size_t i = 0; i < XMLNode_GetChildrenCount(pCurrentNode); ++i)
        {
            XMLNode* child = XMLNode_GetChild(pCurrentNode, i);

            if (XMLString_Equals(XMLNode_GetName(child), &xsChildName))
            {
                if (!pNextNode)
                {
                    pNextNode = child;
                }
                /* Two children with the same name */
                else {
                    va_end(arguments);
                    return 0;
                }
            }
        }

        /* No child with that name found */
        if (!pNextNode)
        {
            va_end(arguments);
            return 0;
        }
        pCurrentNode = pNextNode;

        /* Find name of next child */
        pszChildName = va_arg(arguments, PCWSTR);
    }
    va_end(arguments);

    /* Return chrreutn element */
    return pCurrentNode;
}

/**
 * [PUBLIC API]
 */
PWSTR XMLNode_EasyName(XMLNode* pXMLNode)
{
    if (!pXMLNode)
    {
        return 0;
    }

    return XMLString_Clone(XMLNode_GetName(pXMLNode));
}

/**
 * [PUBLIC API]
 */
PWSTR XMLNode_EasyContent(XMLNode* pXMLNode)
{
    if (!pXMLNode)
    {
        return 0;
    }

    return XMLString_Clone(XMLNode_GetContent(pXMLNode));
}

/**
 * [PUBLIC API]
 */
size_t XMLString_Length(XMLString* pXMLString)
{
    if (!pXMLString)
    {
        return 0;
    }

    return pXMLString->nLength;
}

/**
 * [PUBLIC API]
 */
void XMLString_Copy(XMLString* pXMLString, PWSTR pszBuffer, size_t length)
{
    if (!pXMLString)
    {
        return;
    }

    #define min(X,Y) ((X) < (Y) ? (X) : (Y))
    length = min(length, pXMLString->nLength);
    #undef min

    memcpy(pszBuffer, pXMLString->pszBuffer, length * sizeof(WCHAR));
}
