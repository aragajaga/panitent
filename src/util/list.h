#pragma once

typedef struct List List;

List* List_Create(size_t nElementSize);

void List_InsertFront(List* pList, void* pElement);

void List_InsertBack(List* pList, void* pElement);

void* List_Get(List* pList, int idx);

size_t List_GetLength(List* pList);

size_t List_GetCount(List* pList);
void* List_GetAt(List* pList, int idx);
int List_IndexOf(List* pList, void* pElement);
int List_IndexOfPointer(List* pList, void* pPointer);
void List_RemoveAt(List* pList, int idx);
void List_InsertAt(List* pList, int idx, void* pElement);
void List_Add(List* pList, void* pElement); // Alias for InsertBack
void List_Destroy(List* pList);
int List_Remove(List* pList, void* pElement); // Removes first match
int List_RemovePointer(List* pList, void* pPointer);
