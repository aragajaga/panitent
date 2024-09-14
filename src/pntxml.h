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

#ifndef PANITENT_PNTXML_H
#define PANITENT_PNTXML_H

/* Opaque structure holding the parsed xml document */
typedef struct XMLDocument XMLDocument;
typedef struct XMLNode XMLNode;
typedef struct XMLAttribute XMLAttribute;

/* Internal character dequence representation */
typedef struct XMLString XMLString;

/**
 * Tries to parse the XML fragment in pszBuffer
 *
 * @param pszBuffer Chunk to parse
 * @param length Size of the pszBuffer
 * 
 * @warning `pszBuffer` will be referenced by the document, you may not free it
 *     until you free the XMLDocument
 * @warning You have to call XMLDocument_Free after you finished using the
 *     document
 * 
 * @return The parsed xml fragment if parsing was successful, NULL otherwise
 */
XMLDocument* XML_ParseDocument(PWSTR pszBuffer, size_t length);

/**
 * Tries to read an XML document from disk
 * 
 * @param source File that will be read into an xml document. Will be closed
 * 
 * @warning YOu have to call xml_document_free with free_buffer = true after you
 *     finished using the document
 * 
 * @return The parsed xml fragment if parsing was successful, 0 otherwise
 */
XMLDocument* XML_OpenDocument(FILE* source);

/**
 * Frees all resources associated with the document. All XMLNode and XMLString
 * references obtained through the document will be invalidated
 * 
 * @param document XMLDocument to free
 * @param free_buffer if true the internal pszBuffer supplied via pntxml_parse_buffer
 *     will be freed with the `free` system call
 */
void XMLDocument_Free(XMLDocument* pXMLDocument, BOOL bFreeBuffer);

/**
 * @return XMLNode representing the document root
 */
XMLNode* XMLDocument_GetRoot(XMLDocument* pXMLDocument);

/**
 * @return The XMLNode's tag name
 */
XMLString* XMLNode_GetName(XMLNode* pXMLNode);

/**
 * @return The XMLNode's string content (if available, otherwise NULL)
 */
XMLString* XMLNode_GetContent(XMLNode* pXMLNode);

/**
 * @return Number of child nodes
 */
size_t XMLNode_GetChildrenCount(XMLNode* pXMLNode);

/**
 * @return The n-th child or 0 if out of range
 */
XMLNode* XMLNode_GetChild(XMLNode* pXMLNode, size_t nChild);

/**
 * @return Number of attribute nodes
 */
size_t XMLNode_GetAttributesCount(XMLNode* pXMLNode);

/**
 * @return the n-th attribute name or 0 if out of range
 */
XMLString* XMLNode_GetAttributeName(XMLNode* pXMLNode, size_t nAttribute);

/**
 * @return the n-th attribute content or 0 if out of range
 */
XMLString* XMLNode_GetAttributeContent(XMLNode* pXMLNode, size_t nAttribute);

/**
 * @return The node described by the path or NULL if child cannot be found
 * @warning Each element on the way must be unique
 * @warning Last argument must be 0
 */
XMLNode* XMLNode_GetChild(XMLNode* pXMLNode, PCWSTR pszChild, ...);

/**
 * @return 0-terminated copy of node name
 * @warning User must free the result
 */
PWSTR XMLNode_EasyName(struct XMLNode* node);

/**
 * @return Length of the string
 */
size_t XMLString_Length(XMLString* pxsString);

/**
 * Copies the string into the supplied pszBuffer
 * 
 * @warning String will not be 0-terminated
 * @warning Will write at most length bytes, even if the string is longer
 */
void XMLString_Copy(XMLString* pxsString, PWSTR pszBuffer, size_t nLength);

#endif  /* PANITENT_PNTXML_H */
