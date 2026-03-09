#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "dockmodelio.h"

#define DOCKMODELIO_MAGIC 0x444D4F44u /* DMOD */
#define DOCKMODELIO_VERSION 2u

typedef struct DockModelFileHeader DockModelFileHeader;
struct DockModelFileHeader
{
	uint32_t magic;
	uint32_t version;
};

typedef struct DockModelDiskNode DockModelDiskNode;
struct DockModelDiskNode
{
	uint32_t uNodeId;
	int32_t nRole;
	int32_t nPaneKind;
	int32_t nDockSide;
	uint32_t dwStyle;
	float fGripPos;
	int16_t iGripPos;
	uint8_t bShowCaption;
	uint8_t bCollapsed;
	WCHAR szName[MAX_PATH];
	WCHAR szCaption[MAX_PATH];
	WCHAR szActiveTabName[MAX_PATH];
};

typedef struct LegacyDockModelDiskNodeV1
{
	int32_t nRole;
	int32_t nPaneKind;
	int32_t nDockSide;
	uint32_t dwStyle;
	float fGripPos;
	int16_t iGripPos;
	uint8_t bShowCaption;
	uint8_t bCollapsed;
	WCHAR szName[MAX_PATH];
	WCHAR szCaption[MAX_PATH];
	WCHAR szActiveTabName[MAX_PATH];
} LegacyDockModelDiskNodeV1;

static BOOL DockModelIO_WriteNode(FILE* fp, const DockModelNode* pNode)
{
	uint8_t bPresent = pNode ? 1 : 0;
	if (!fp || fwrite(&bPresent, sizeof(bPresent), 1, fp) != 1)
	{
		return FALSE;
	}

	if (!pNode)
	{
		return TRUE;
	}

	DockModelDiskNode diskNode = { 0 };
	diskNode.uNodeId = pNode->uNodeId;
	diskNode.nRole = (int32_t)pNode->nRole;
	diskNode.nPaneKind = (int32_t)pNode->nPaneKind;
	diskNode.nDockSide = (int32_t)pNode->nDockSide;
	diskNode.dwStyle = pNode->dwStyle;
	diskNode.fGripPos = pNode->fGripPos;
	diskNode.iGripPos = pNode->iGripPos;
	diskNode.bShowCaption = pNode->bShowCaption ? 1 : 0;
	diskNode.bCollapsed = pNode->bCollapsed ? 1 : 0;
	wcscpy_s(diskNode.szName, ARRAYSIZE(diskNode.szName), pNode->szName);
	wcscpy_s(diskNode.szCaption, ARRAYSIZE(diskNode.szCaption), pNode->szCaption);
	wcscpy_s(diskNode.szActiveTabName, ARRAYSIZE(diskNode.szActiveTabName), pNode->szActiveTabName);

	if (fwrite(&diskNode, sizeof(diskNode), 1, fp) != 1)
	{
		return FALSE;
	}

	return DockModelIO_WriteNode(fp, pNode->pChild1) &&
		DockModelIO_WriteNode(fp, pNode->pChild2);
}

static DockModelNode* DockModelIO_ReadNodeV2(FILE* fp)
{
	uint8_t bPresent = 0;
	if (!fp || fread(&bPresent, sizeof(bPresent), 1, fp) != 1)
	{
		return NULL;
	}

	if (!bPresent)
	{
		return (DockModelNode*)(INT_PTR)1;
	}

	DockModelDiskNode diskNode = { 0 };
	if (fread(&diskNode, sizeof(diskNode), 1, fp) != 1)
	{
		return NULL;
	}

	DockModelNode* pNode = (DockModelNode*)calloc(1, sizeof(DockModelNode));
	if (!pNode)
	{
		return NULL;
	}

	pNode->uNodeId = diskNode.uNodeId;
	pNode->nRole = (DockNodeRole)diskNode.nRole;
	pNode->nPaneKind = (DockPaneKind)diskNode.nPaneKind;
	pNode->nDockSide = diskNode.nDockSide;
	pNode->dwStyle = diskNode.dwStyle;
	pNode->fGripPos = diskNode.fGripPos;
	pNode->iGripPos = diskNode.iGripPos;
	pNode->bShowCaption = diskNode.bShowCaption ? TRUE : FALSE;
	pNode->bCollapsed = diskNode.bCollapsed ? TRUE : FALSE;
	wcscpy_s(pNode->szName, ARRAYSIZE(pNode->szName), diskNode.szName);
	wcscpy_s(pNode->szCaption, ARRAYSIZE(pNode->szCaption), diskNode.szCaption);
	wcscpy_s(pNode->szActiveTabName, ARRAYSIZE(pNode->szActiveTabName), diskNode.szActiveTabName);

	DockModelNode* pChild1 = DockModelIO_ReadNodeV2(fp);
	if (!pChild1)
	{
		DockModel_Destroy(pNode);
		return NULL;
	}
	DockModelNode* pChild2 = DockModelIO_ReadNodeV2(fp);
	if (!pChild2)
	{
		DockModel_Destroy(pNode);
		return NULL;
	}

	pNode->pChild1 = (pChild1 == (DockModelNode*)(INT_PTR)1) ? NULL : pChild1;
	pNode->pChild2 = (pChild2 == (DockModelNode*)(INT_PTR)1) ? NULL : pChild2;
	return pNode;
}

static DockModelNode* DockModelIO_ReadNodeV1(FILE* fp)
{
	uint8_t bPresent = 0;
	if (!fp || fread(&bPresent, sizeof(bPresent), 1, fp) != 1)
	{
		return NULL;
	}

	if (!bPresent)
	{
		return (DockModelNode*)(INT_PTR)1;
	}

	LegacyDockModelDiskNodeV1 diskNode = { 0 };
	if (fread(&diskNode, sizeof(diskNode), 1, fp) != 1)
	{
		return NULL;
	}

	DockModelNode* pNode = (DockModelNode*)calloc(1, sizeof(DockModelNode));
	if (!pNode)
	{
		return NULL;
	}

	pNode->uNodeId = 0;
	pNode->nRole = (DockNodeRole)diskNode.nRole;
	pNode->nPaneKind = (DockPaneKind)diskNode.nPaneKind;
	pNode->nDockSide = diskNode.nDockSide;
	pNode->dwStyle = diskNode.dwStyle;
	pNode->fGripPos = diskNode.fGripPos;
	pNode->iGripPos = diskNode.iGripPos;
	pNode->bShowCaption = diskNode.bShowCaption ? TRUE : FALSE;
	pNode->bCollapsed = diskNode.bCollapsed ? TRUE : FALSE;
	wcscpy_s(pNode->szName, ARRAYSIZE(pNode->szName), diskNode.szName);
	wcscpy_s(pNode->szCaption, ARRAYSIZE(pNode->szCaption), diskNode.szCaption);
	wcscpy_s(pNode->szActiveTabName, ARRAYSIZE(pNode->szActiveTabName), diskNode.szActiveTabName);

	DockModelNode* pChild1 = DockModelIO_ReadNodeV1(fp);
	if (!pChild1)
	{
		DockModel_Destroy(pNode);
		return NULL;
	}
	DockModelNode* pChild2 = DockModelIO_ReadNodeV1(fp);
	if (!pChild2)
	{
		DockModel_Destroy(pNode);
		return NULL;
	}

	pNode->pChild1 = (pChild1 == (DockModelNode*)(INT_PTR)1) ? NULL : pChild1;
	pNode->pChild2 = (pChild2 == (DockModelNode*)(INT_PTR)1) ? NULL : pChild2;
	return pNode;
}

BOOL DockModelIO_WriteToStream(FILE* fp, const DockModelNode* pRootNode)
{
	return DockModelIO_WriteNode(fp, pRootNode);
}

DockModelNode* DockModelIO_ReadFromStream(FILE* fp)
{
	DockModelNode* pRootNode = DockModelIO_ReadNodeV2(fp);
	if (pRootNode == (DockModelNode*)(INT_PTR)1)
	{
		return NULL;
	}

	return pRootNode;
}

BOOL DockModelIO_SaveToFile(const DockModelNode* pRootNode, PCWSTR pszFilePath)
{
	FILE* fp = NULL;
	DockModelFileHeader header = { DOCKMODELIO_MAGIC, DOCKMODELIO_VERSION };

	if (!pRootNode || !pszFilePath || !pszFilePath[0])
	{
		return FALSE;
	}

	if (_wfopen_s(&fp, pszFilePath, L"wb") != 0 || !fp)
	{
		return FALSE;
	}

	BOOL bOk = FALSE;
	if (fwrite(&header, sizeof(header), 1, fp) == 1)
	{
		bOk = DockModelIO_WriteToStream(fp, pRootNode);
	}

	fclose(fp);
	return bOk;
}

DockModelNode* DockModelIO_LoadFromFile(PCWSTR pszFilePath)
{
	return DockModelIO_LoadFromFileEx(pszFilePath, NULL);
}

DockModelNode* DockModelIO_LoadFromFileEx(PCWSTR pszFilePath, PersistLoadStatus* pStatus)
{
	FILE* fp = NULL;
	DockModelFileHeader header = { 0 };
	DockModelNode* pRootNode = NULL;

	if (pStatus)
	{
		*pStatus = PERSIST_LOAD_IO_ERROR;
	}

	if (!pszFilePath || !pszFilePath[0])
	{
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_NOT_FOUND;
		}
		return NULL;
	}

	DWORD dwAttrs = GetFileAttributesW(pszFilePath);
	if (dwAttrs == INVALID_FILE_ATTRIBUTES)
	{
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_NOT_FOUND;
		}
		return NULL;
	}

	if (_wfopen_s(&fp, pszFilePath, L"rb") != 0 || !fp)
	{
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_IO_ERROR;
		}
		return NULL;
	}

	if (fread(&header, sizeof(header), 1, fp) != 1 ||
		header.magic != DOCKMODELIO_MAGIC ||
		(header.version != 1u && header.version != DOCKMODELIO_VERSION))
	{
		fclose(fp);
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_INVALID_FORMAT;
		}
		return NULL;
	}

	pRootNode = (header.version == 1u) ? DockModelIO_ReadNodeV1(fp) : DockModelIO_ReadFromStream(fp);
	fclose(fp);
	if (!pRootNode)
	{
		if (pStatus)
		{
			*pStatus = PERSIST_LOAD_INVALID_FORMAT;
		}
		return NULL;
	}

	if (pStatus)
	{
		*pStatus = PERSIST_LOAD_OK;
	}
	return pRootNode;
}
