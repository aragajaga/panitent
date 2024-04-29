#pragma once

typedef int (FnAVLNodeKeyCompare)(void* pKey1, void* pKey2);

enum {
    CMP_LOWER = -1,
    CMP_EQUAL = 0,
    CMP_GREATER = 1
};

typedef struct AVLNode AVLNode;
struct AVLNode {
    void* pKey;
    void* pValue;
    AVLNode* pLeft;
    AVLNode* pRight;
    int nHeight;
    FnAVLNodeKeyCompare* pfnCompare;
};

typedef struct HashMap HashMap;
struct HashMap {
    AVLNode** ppBuckets;
    int nSize;
    int nCapacity;
    FnAVLNodeKeyCompare* pfnCompare;
};

/**
 * [PUBLIC]
 *
 * Function to create a new AVL tree node
 * 
 * @return Node created
 */
AVLNode* AVLNode_Create(void* pKey, void* pValue, FnAVLNodeKeyCompare* pfnCompare);

/**
 * [PUBLIC]
 * 
 * Function to get height of a node
 * 
 * @return Node height
 */
int AVLNode_Height(AVLNode* pAVLNode);

/**
 * [PRIVATE]
 * 
 * Function to perform right rotation on an AVL tree
 */
AVLNode* AVLNode_RightRotate(AVLNode* pNodeY);

/**
 * [PRIVATE]
 * 
 * Function to perform left rotation on an AVL tree
 */
AVLNode* AVLNode_LeftRotate(AVLNode* pNodeX);

/**
 * [PRIVATE]
 * 
 * Function to get balance factor of a node
 */
int AVLNode_GetBalance(AVLNode* pAVLNode);

/**
 * [PRIVATE]
 * 
 * Function to insert a node into AVL tree
 */
AVLNode* AVLNode_Insert(AVLNode* pAVLNode, void* pKey, void* pValue, FnAVLNodeKeyCompare* pfnCompare);

/**
 * Function to search for a key in the AVL tree
 */
AVLNode* AVLNode_Search(AVLNode* pNodeRoot, void* pKey, FnAVLNodeKeyCompare* pfnCompare);

/**
 * Function to create a new hash map
 */
HashMap* HashMap_Create(int nCapacity, FnAVLNodeKeyCompare* pfnCompare);

/**
 * Function to hash the key to an index
 */
int BucketHash(int key, int capacity);

/**
 * Function to insert a key-value pair into the hash map
 */
void HashMap_Put(HashMap* pHashMap, void* pKey, void* pValue);

/**
 * Function to get the value associated with a key from the hash map
 */
void* HashMap_Get(HashMap* pHashMap, void* pKey);

/**
 * Function to destroy the AVL tree
 */
void AVLNode_DestroyTree(AVLNode* pNodeRoot);

/**
 * Function to destroy the hash map and free memory
 */
void HashMap_Destroy(HashMap* pHashMap);
