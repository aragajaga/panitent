#pragma once

typedef struct List List;

List* List_Create(size_t nElementSize);

void List_InsertFront(List* pList, void* pElement);

void List_InsertBack(List* pList, void* pElement);

void* List_Get(List* pList, int idx);

size_t List_GetLength(List* pList);
