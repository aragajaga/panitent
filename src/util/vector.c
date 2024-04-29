#include "../precomp.h"

#include "vector.h"

#include "../panitent.h"

// Initialize the vector
void Vector_Init(Vector* pVector, size_t nElementSize)
{
    pVector->pData = NULL;
    pVector->nSize = 0;
    pVector->nCapacity = 0;
    pVector->nElementSize = nElementSize;
}

// Push an element to the back of the vector
void Vector_PushBack(Vector* pVector, void* pValue, size_t nValueSize)
{
    // Check if resize is needed
    if (pVector->nSize >= pVector->nCapacity)
    {
        // Double the capacity
        pVector->nCapacity = (!pVector->nCapacity) ? 1 : pVector->nCapacity * 2;
        pVector->pData = realloc(pVector->pData, pVector->nCapacity * sizeof(void*));
        if (!pVector->pData)
        {
            Panitent_RaiseException(L"Memory allocation failed");
        }
    }

    // Allocate memory for the new element
    pVector->pData[pVector->nSize] = malloc(nValueSize);
    if (!pVector->pData[pVector->nSize])
    {
        Panitent_RaiseException(L"Memory allocation failed");
    }

    // Copy the value into the new element
    memcpy(pVector->pData[pVector->nSize], pValue, nValueSize);
    pVector->nSize++;
}

// Access an element at a specific index
void* Vector_At(Vector* pVector, size_t nIndex)
{
    if (nIndex >= pVector->nSize)
    {
        Panitent_RaiseException(L"Index out of bounds");
    }
    return pVector->pData[nIndex];
}

// Get the number of elements stored in the vector
size_t Vector_GetSize(Vector* pVector)
{
    return pVector->nSize;
}

// Free the memory allocated by the vector
void Vector_Free(Vector* pVector)
{
    if (pVector->pData)
    {
        for (size_t i = 0; i < pVector->nSize; ++i)
        {
            free(pVector->pData[i]);
        }
        free(pVector->pData);
    }

    pVector->pData = NULL;
    pVector->nSize = 0;
    pVector->nCapacity = 0;
}
