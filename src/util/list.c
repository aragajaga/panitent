#include "../precomp.h"

#include "list.h"

typedef struct List List;
struct List {
    void* pData;
    size_t nCapacity;
    size_t nSize;
    size_t nElementSize;
};

void List_Init(List* pList);

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
