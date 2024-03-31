#ifndef _WIN32_WINDOWMAP_H_INCLUDED
#define _WIN32_WINDOWMAP_H_INCLUDED

typedef struct Window Window;

typedef struct HashMap HashMap;

void InitHashMap(struct HashMap* map, size_t capacity);
void InsertIntoHashMap(struct HashMap* map, void* key, void* value);
void* GetFromHashMap(struct HashMap* map, void* key);
void FreeHashMap(struct HashMap* map);
void WindowingInit();
void RemoveFromHashMapByKey(struct HashMap* map, void* key);
void RemoveFromHashMapByValue(struct HashMap* map, void* value);

extern Window* pWndCreating;

#endif  // _WIN32_WINDOWMAP_H_INCLUDED
