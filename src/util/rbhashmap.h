#pragma once

#ifndef PANITENT_UTIL_RBHASHMAP_H_
#define PANITENT_UTIL_RBHASHMAP_H_

typedef enum { RED, BLACK } RBTreeColor;

typedef int (FnRBTreeKeyComparator)(void* key1, void* key2);

typedef struct RBTreeNode RBTreeNode;
struct RBTreeNode {
    void* pKey;
    void* pValue;
    RBTreeColor color;
    RBTreeNode* pLeft;
    RBTreeNode* pRight;
    RBTreeNode* pParent;
};

typedef struct RBTree RBTree;
struct RBTree {
    RBTreeNode* pRoot;
    FnRBTreeKeyComparator* pfnKeyComparator;
    void* (*pfnKeyDeleter)(void*);
    void* (*pfnValueDeleter)(void*);
};

typedef struct RBHashMap RBHashMap;
struct RBHashMap {
    RBTree* tree;
};

typedef struct RBHashMapContext RBHashMapContext;
struct RBHashMapContext {
    FnRBTreeKeyComparator* pfnKeyComparator;
    void* (*pfnKeyDeleter)(void*);
    void* (*pfnValueDeleter)(void*);
};

RBHashMap* RBHashMap_Create(RBHashMapContext* ctx);
void RBHashMap_Insert(RBHashMap* map, void* key, void* value);
void* RBHashMap_Lookup(RBHashMap* map, void* key);
void* RBHashMap_Delete(RBHashMap* map, void* key);
void RBHashMap_Destroy(RBHashMap* map);

#endif  /* PANITENT_UTIL_RBHASHMAP_H_ */
