#include "../precomp.h"

#include "vector.h"

#include "assert.h"

// Initialize the vector
void Vector_Init(Vector* pVector, size_t nElementSize)
{
    pVector->pData = NULL;
    pVector->nSize = 0;
    pVector->nCapacity = 0;
    pVector->nElementSize = nElementSize;
}

// Reserve memory for a specified number of elements
void Vector_Reserve(Vector* pVector, size_t nNewCapacity)
{
    if (nNewCapacity > pVector->nCapacity)
    {
        pVector->pData = realloc(pVector->pData, nNewCapacity * pVector->nElementSize);
        if (!pVector->pData)
        {
            Panitent_RaiseException(L"MemoryBarrier allocation failed");
        }
        pVector->nCapacity = nNewCapacity;
    }
}

// Resize the vector to contain nNewSize elements
void Vector_Resize(Vector* pVector, size_t nNewSize, void* pDefaultValue)
{
    if (nNewSize > pVector->nCapacity)
    {
        // Reserve more space if necessary
        Vector_Reserve(pVector, nNewSize);
    }

    if (nNewSize > pVector->nSize)
    {
        // Initialize new elements with the default value
        char* pStart = (char*)pVector->pData + pVector->nSize * pVector->nElementSize;        
        for (size_t i = pVector->nSize; i < nNewSize; ++i)
        {
            memcpy(pStart + i * pVector->nElementSize, pDefaultValue, pVector->nElementSize);
        }
    }

    pVector->nSize = nNewSize;
}

// Push an element to the back of the vector
void Vector_PushBack(Vector* pVector, void* pValue)
{
    // Check if resize is needed
    if (pVector->nSize >= pVector->nCapacity)
    {
        // Double the capacity
        Vector_Reserve(pVector, (!pVector->nCapacity) ? 1 : pVector->nCapacity * 2);
    }

    // Copy the value into the new element
    char* pTarget = (char*)pVector->pData + pVector->nSize * pVector->nElementSize;
    memcpy(pTarget, pValue, pVector->nElementSize);
    pVector->nSize++;
}

// Access an element at a specific index
void* Vector_At(Vector* pVector, size_t nIndex)
{
    ASSERT(nIndex < pVector->nSize);
    return (char*)pVector->pData + nIndex * pVector->nElementSize;
}

// Remove an element at a specific index
void Vector_Remove(Vector* pVector, size_t nIndex)
{
    if (nIndex >= pVector->nSize)
    {
        Panitent_RaiseException(L"Index out of bounds");
    }

    // Calculate the location of the element to remove
    char* pRemoveAt = (char*)pVector->pData + nIndex * pVector->nElementSize;

    // If it's not the last element, shift the elements after it
    if (nIndex < pVector->nSize - 1)
    {
        char* pNext = pRemoveAt + pVector->nElementSize;
        memmove(pRemoveAt, pNext, (pVector->nSize - nIndex - 1) * pVector->nElementSize);
    }

    // Decrease the size
    pVector->nSize--;
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
        free(pVector->pData);
    }

    pVector->pData = NULL;
    pVector->nSize = 0;
    pVector->nCapacity = 0;
}

/* Clear the vector. (Remove all elements) */
void Vector_Clear(Vector* pVector)
{
    pVector->nSize = 0;
}
