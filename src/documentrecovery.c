#include "precomp.h"

#include <stdint.h>
#include <stdio.h>

#include "canvas.h"
#include "document.h"
#include "documentrecovery.h"

#define DOCRECOVERY_MAGIC 0x56434452u /* RDCV */
#define DOCRECOVERY_VERSION 1u

typedef struct DocumentRecoveryHeader
{
	uint32_t magic;
	uint32_t version;
	int32_t width;
	int32_t height;
	uint32_t bufferSize;
} DocumentRecoveryHeader;

BOOL DocumentRecovery_Save(Document* pDocument, PCWSTR pszPath)
{
	Canvas* pCanvas = Document_GetCanvas(pDocument);
	FILE* fp = NULL;
	if (!pDocument || !pCanvas || !pszPath || !pszPath[0])
	{
		return FALSE;
	}

	DocumentRecoveryHeader header = {
		DOCRECOVERY_MAGIC,
		DOCRECOVERY_VERSION,
		pCanvas->width,
		pCanvas->height,
		(uint32_t)pCanvas->buffer_size
	};

	if (_wfopen_s(&fp, pszPath, L"wb") != 0 || !fp)
	{
		return FALSE;
	}

	BOOL bOk =
		fwrite(&header, sizeof(header), 1, fp) == 1 &&
		fwrite(pCanvas->buffer, 1, pCanvas->buffer_size, fp) == pCanvas->buffer_size;
	fclose(fp);
	return bOk;
}

Document* DocumentRecovery_Load(PCWSTR pszPath)
{
	FILE* fp = NULL;
	DocumentRecoveryHeader header = { 0 };
	if (!pszPath || !pszPath[0])
	{
		return NULL;
	}

	if (_wfopen_s(&fp, pszPath, L"rb") != 0 || !fp)
	{
		return NULL;
	}

	BOOL bOk =
		fread(&header, sizeof(header), 1, fp) == 1 &&
		header.magic == DOCRECOVERY_MAGIC &&
		header.version == DOCRECOVERY_VERSION &&
		header.width > 0 &&
		header.height > 0 &&
		header.bufferSize == (uint32_t)((size_t)header.width * (size_t)header.height * 4u);
	if (!bOk)
	{
		fclose(fp);
		return NULL;
	}

	void* pBuffer = malloc(header.bufferSize);
	if (!pBuffer)
	{
		fclose(fp);
		return NULL;
	}

	bOk = fread(pBuffer, 1, header.bufferSize, fp) == header.bufferSize;
	fclose(fp);
	if (!bOk)
	{
		free(pBuffer);
		return NULL;
	}

	Canvas* pCanvas = Canvas_CreateFromBuffer(header.width, header.height, pBuffer);
	free(pBuffer);
	if (!pCanvas)
	{
		return NULL;
	}

	return Document_CreateWithCanvas(pCanvas);
}

BOOL DocumentRecovery_OpenInWorkspace(PCWSTR pszPath, WorkspaceContainer* pWorkspaceContainer)
{
	Document* pDocument = DocumentRecovery_Load(pszPath);
	if (!pDocument)
	{
		return FALSE;
	}

	if (!Document_AttachToWorkspace(pDocument, pWorkspaceContainer))
	{
		Document_Destroy(pDocument);
		return FALSE;
	}

	return TRUE;
}
