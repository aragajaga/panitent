#include "../precomp.h"

#include "list.h"

typedef struct List List;
struct List {
    void* pData;
    size_t nCapacity;
    size_t nSize;
    size_t nElementSize;
};

void List_Init(List* pList, size_t nElementSize);

List* List_Create(size_t nElementSize)
{
    List* pList = (List*)malloc(sizeof(List));
    if (pList)
    {
        List_Init(pList, nElementSize);
    }

    return pList;
}

#define LIST_CAPACITY 16

void List_Init(List* pList, size_t nElementSize)
{
    memset(pList, 0, sizeof(List));
    pList->pData = malloc(nElementSize * LIST_CAPACITY);
    memset(pList->pData, 0, nElementSize * LIST_CAPACITY);
    pList->nCapacity = LIST_CAPACITY;
    pList->nSize = 0;
    pList->nElementSize = nElementSize;

    return;
}

void List_InsertFront(List* pList, void* pElement)
{
    if (pList->nSize >= pList->nCapacity)
    {
        printf("Reallocation triggered\n");
        size_t newCapacity = pList->nCapacity * 2;
        void* newpData = realloc(pList->pData, newCapacity * pList->nElementSize);
        if (!newpData)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return;
        }
        pList->pData = newpData;
        pList->nCapacity = newCapacity;
    }

    if (pList->nSize)
    {
        memmove((unsigned char*)pList->pData + pList->nElementSize, pList->pData, pList->nSize * pList->nElementSize);
    }
    memcpy(pList->pData, pElement, pList->nElementSize);
    pList->nSize++;
}

void List_InsertBack(List* pList, void* pElement)
{
    if (pList->nSize >= pList->nCapacity)
    {
        printf("Reallocation triggered\n");
        size_t newCapacity = pList->nCapacity * 2;
        void* newpData = realloc(pList->pData, newCapacity * pList->nElementSize);
        if (!newpData) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return;
        }
        pList->pData = newpData;
        pList->nCapacity = newCapacity;
    }

    memcpy((unsigned char*)pList->pData + pList->nSize * pList->nElementSize, pElement, pList->nElementSize);
    pList->nSize++;
}

void* List_Get(List* pList, int idx)
{
    return (unsigned char*)pList->pData + idx * pList->nElementSize;
}

size_t List_GetLength(List* pList)
{
    return pList->nSize;
}

size_t List_GetCount(List* pList) {
    return pList->nSize;
}

void* List_GetAt(List* pList, int idx) {
    if (idx < 0 || (size_t)idx >= pList->nSize) return NULL;
    return (unsigned char*)pList->pData + idx * pList->nElementSize;
}

int List_IndexOf(List* pList, void* pElement) {
    for (size_t i = 0; i < pList->nSize; ++i) {
        void* current = (unsigned char*)pList->pData + i * pList->nElementSize;
        if (memcmp(current, pElement, pList->nElementSize) == 0) {
            return (int)i;
        }
    }
    return -1;
}

void List_RemoveAt(List* pList, int idx) {
    if (idx < 0 || (size_t)idx >= pList->nSize) return;

    if ((size_t)idx < pList->nSize - 1) {
        void* dst = (unsigned char*)pList->pData + idx * pList->nElementSize;
        void* src = (unsigned char*)pList->pData + (idx + 1) * pList->nElementSize;
        size_t bytesToMove = (pList->nSize - idx - 1) * pList->nElementSize;
        memmove(dst, src, bytesToMove);
    }

    pList->nSize--;
}

void List_InsertAt(List* pList, int idx, void* pElement) {
    if (idx < 0 || (size_t)idx > pList->nSize) return;

    if (pList->nSize >= pList->nCapacity) {
        size_t newCapacity = pList->nCapacity * 2;
        void* newpData = realloc(pList->pData, newCapacity * pList->nElementSize);
        if (!newpData) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return;
        }
        pList->pData = newpData;
        pList->nCapacity = newCapacity;
    }

    void* insertPos = (unsigned char*)pList->pData + idx * pList->nElementSize;
    void* moveSrc = insertPos;
    void* moveDst = (unsigned char*)insertPos + pList->nElementSize;
    size_t bytesToMove = (pList->nSize - idx) * pList->nElementSize;
    memmove(moveDst, moveSrc, bytesToMove);
    memcpy(insertPos, pElement, pList->nElementSize);
    pList->nSize++;
}

void List_Add(List* pList, void* pElement) {
    List_InsertBack(pList, pElement);
}

void List_Destroy(List* pList) {
    if (pList) {
        free(pList->pData);
        free(pList);
    }
}

int List_Remove(List* pList, void* pElement) {
    int index = List_IndexOf(pList, pElement);
    if (index >= 0) {
        List_RemoveAt(pList, index);
        return 1;
    }
    return 0;
}

int List_IndexOfPointer(List* pList, void* pPointer) {
    // This function is for lists that store pointers.
    // It compares the pointer values directly.
    for (size_t i = 0; i < pList->nSize; ++i) {
        void** pCurrentPointer = (void**)((unsigned char*)pList->pData + i * pList->nElementSize);
        if (*pCurrentPointer == pPointer) {
            return (int)i;
        }
    }
    return -1;
}

int List_RemovePointer(List* pList, void* pPointer) {
	int index = List_IndexOfPointer(pList, pPointer);
	if (index >= 0) {
		List_RemoveAt(pList, index);
		return 1;
	}
	return 0;
}
