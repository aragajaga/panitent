#include "../precomp.h"

#include "hashmap.h"

/**
 * [PUBLIC]
 *
 * Function to create a new AVL tree node
 *
 * @return Node created
 */
AVLNode* AVLNode_Create(void* pKey, void* pValue, FnAVLNodeKeyCompare* pfnCompare)
{
    AVLNode* pNode = (AVLNode*)malloc(sizeof(AVLNode));
    if (!pNode)
    {
        return NULL;
    }

    pNode->pKey = pKey;
    pNode->pValue = pValue;
    pNode->pLeft = NULL;
    pNode->pRight = NULL;
    pNode->nHeight = 1;
    pNode->pfnCompare = pfnCompare;
    
    return pNode;
}

/**
 * [PUBLIC]
 *
 * Function to get height of a node
 *
 * @return Node height
 */
int AVLNode_Height(AVLNode* pAVLNode)
{
    if (!pAVLNode)
    {
        return 0;
    }

    return pAVLNode->nHeight;
}

/**
 * [PRIVATE]
 *
 * Function to perform right rotation on an AVL tree
 */
AVLNode* AVLNode_RightRotate(AVLNode* pNodeY)
{
    AVLNode* pNodeX = pNodeY->pLeft;
    AVLNode* pNodeTemp = pNodeX->pRight;

    /* Perform rotation */
    pNodeX->pRight = pNodeY;
    pNodeY->pLeft = pNodeTemp;

    /* Update heights */
    pNodeY->nHeight = max(AVLNode_Height(pNodeY->pLeft), AVLNode_Height(pNodeY->pRight)) + 1;
    pNodeX->nHeight = max(AVLNode_Height(pNodeX->pLeft), AVLNode_Height(pNodeX->pRight)) + 1;

    /* Return new root */
    return pNodeX;
}

/**
 * [PRIVATE]
 *
 * Function to perform left rotation on an AVL tree
 */
AVLNode* AVLNode_LeftRotate(AVLNode* pNodeX)
{
    AVLNode* pNodeY = pNodeX->pRight;
    AVLNode* pNodeTemp = pNodeY->pLeft;

    /* Perform rotation */
    pNodeY->pLeft = pNodeX;
    pNodeX->pRight = pNodeTemp;

    /* Update heights */
    pNodeX->nHeight = max(AVLNode_Height(pNodeX->pLeft), AVLNode_Height(pNodeX->pRight)) + 1;
    pNodeY->nHeight = max(AVLNode_Height(pNodeY->pLeft), AVLNode_Height(pNodeY->pRight)) + 1;

    /* Return new root */
    return pNodeY;
}

/**
 * [PRIVATE]
 *
 * Function to get balance factor of a node
 */
int AVLNode_GetBalance(AVLNode* pAVLNode)
{
    if (!pAVLNode)
    {
        return 0;
    }

    return AVLNode_Height(pAVLNode->pLeft) - AVLNode_Height(pAVLNode->pRight);
}

/**
 * [PRIVATE]
 *
 * Function to insert a node into AVL tree
 */
AVLNode* AVLNode_Insert(AVLNode* pAVLNode, void* pKey, void* pValue, FnAVLNodeKeyCompare* pfnCompare)
{
    /* Perform standard BST insertion */
    if (!pAVLNode)
    {
        return AVLNode_Create(pKey, pValue, pfnCompare);
    }

    if (pKey < pAVLNode->pKey)
    {
        pAVLNode->pLeft = AVLNode_Insert(pAVLNode->pLeft, pKey, pValue, pfnCompare);
    }
    else if (pKey > pAVLNode->pKey)
    {
        pAVLNode->pRight = AVLNode_Insert(pAVLNode->pRight, pKey, pValue, pfnCompare);
    }
    else {
        pAVLNode->pValue = pValue;
        return pAVLNode;
    }

    /* Update height of this ancestor node */
    pAVLNode->nHeight = 1 + max(AVLNode_Height(pAVLNode->pLeft), AVLNode_Height(pAVLNode->pRight));

    /* Get the balance factor of this ancestor node to check wheter this node became unbalanced */
    int nBalance = AVLNode_GetBalance(pAVLNode);

    /* If the node becomes unbalanced, there are 4 cases: */

    /* Left Left Case */
    if (nBalance > 1 && CMP_LOWER == pfnCompare(pKey, pAVLNode->pLeft->pKey))
    {
        return AVLNode_RightRotate(pAVLNode);
    }

    /* Right Right Case */
    if (nBalance < -1 && CMP_GREATER == pfnCompare(pKey, pAVLNode->pRight->pKey))
    {
        return AVLNode_LeftRotate(pAVLNode);
    }

    /* Left Right Case */
    if (nBalance > 1 && CMP_GREATER == pfnCompare(pKey, pAVLNode->pLeft->pKey))
    {
        pAVLNode->pLeft = AVLNode_LeftRotate(pAVLNode->pLeft);
        return AVLNode_RightRotate(pAVLNode);
    }

    /* Right Left Case */
    if (nBalance < -1 && CMP_LOWER == pfnCompare(pKey, pAVLNode->pRight->pKey))
    {
        pAVLNode->pRight = AVLNode_RightRotate(pAVLNode->pRight);
        return AVLNode_LeftRotate(pAVLNode);
    }

    /* Return the unchanged node pointer */
    return pAVLNode;
}

/**
 * Function to search for a key in the AVL tree
 * 
 * @return Found AVL node or NULL if failed
 */
AVLNode* AVLNode_Search(AVLNode* pNodeRoot, void* pKey, FnAVLNodeKeyCompare* pfnCompare)
{
    /*
     * If pNodeRoot is NULL, return itself as NULL
     * If pNodeRoot is search target, return itself as found result
     */
    if (!pNodeRoot || CMP_EQUAL == pfnCompare(pNodeRoot->pKey, pKey))
    {
        return pNodeRoot;
    }

    if (CMP_LOWER == pfnCompare(pNodeRoot->pKey, pKey))
    {
        return AVLNode_Search(pNodeRoot->pRight, pKey, pfnCompare);
    }

    return AVLNode_Search(pNodeRoot->pLeft, pKey, pfnCompare);
}

/**
 * Function to create a new hash map
 */
HashMap* HashMap_Create(int nCapacity, FnAVLNodeKeyCompare* pfnCompare)
{
    HashMap* pHashMap = (HashMap*)malloc(sizeof(HashMap));
    if (!pHashMap)
    {
        return NULL;
    }

    pHashMap->nCapacity = nCapacity;
    pHashMap->nSize = 0;
    pHashMap->pfnCompare = pfnCompare;

    AVLNode** ppBuckets = (AVLNode**)malloc(nCapacity * sizeof(AVLNode*));
    if (!ppBuckets)
    {
        free(pHashMap);
        return NULL;
    }

    memset(ppBuckets, 0, nCapacity * sizeof(AVLNode*));
    pHashMap->ppBuckets = ppBuckets;
    
    return pHashMap;
}

/**
 * [PRIVATE]
 * 
 * Function to hash the key to a bucket index
 */
int BucketHash(int key, int capacity)
{
    return key % capacity;
}

/**
 * [PUBLIC]
 * 
 * Function to insert a key-value pair into the hash map
 */
void HashMap_Put(HashMap* pHashMap, void* pKey, void* pValue)
{
    int nIndex = BucketHash(pKey, pHashMap->nCapacity);
    pHashMap->ppBuckets[nIndex] = AVLNode_Insert(pHashMap->ppBuckets[nIndex], pKey, pValue, pHashMap->pfnCompare);
    pHashMap->nSize++;
}

/**
 * [PUBLIC]
 * 
 * Function to get the value associated with a key from the hash map
 * 
 * @param pHashMap HashMap instance pointer
 * @param pKey Key to get value of
 * 
 * @return The value associated with pKey
 */
void* HashMap_Get(HashMap* pHashMap, void* pKey)
{
    int nIndex = BucketHash(pKey, pHashMap->nCapacity);
    AVLNode* pAVLNode = AVLNode_Search(pHashMap->ppBuckets[nIndex], pKey, pHashMap->pfnCompare);

    if (!pAVLNode)
    {
        return NULL;
    }

    return pAVLNode->pValue;
}

/**
 * [PRIVATE]
 * 
 * Function to destroy the AVL tree
 * 
 * @param pNodeRoot An AVLNode pointer that's downstream tree will be deleted
 */
void AVLNode_DestroyTree(AVLNode* pNodeRoot)
{
    if (pNodeRoot)
    {
        AVLNode_DestroyTree(pNodeRoot->pLeft);
        AVLNode_DestroyTree(pNodeRoot->pRight);
        free(pNodeRoot);
    }
}

/**
 * [PRIVATE]
 *
 * Function to find the minimum value node in the AVL tree
 */
AVLNode* AVLNode_FindMin(AVLNode* pNode)
{
    AVLNode* current = pNode;

    while (current->pLeft != NULL)
    {
        current = current->pLeft;
    }

    return current;
}

/**
 * [PRIVATE]
 *
 * Function to delete a node from the AVL tree
 */
AVLNode* AVLNode_Delete(AVLNode* pRoot, void* pKey, FnAVLNodeKeyCompare* pfnCompare)
{
    if (!pRoot)
    {
        return NULL;
    }

    if (CMP_LOWER == pfnCompare(pKey, pRoot->pKey))
    {
        pRoot->pLeft = AVLNode_Delete(pRoot->pLeft, pKey, pfnCompare);
    }
    else if (CMP_GREATER == pfnCompare(pKey, pRoot->pKey))
    {
        pRoot->pRight = AVLNode_Delete(pRoot->pRight, pKey, pfnCompare);
    }
    else
    {
        // Node to be deleted found
        if ((pRoot->pLeft == NULL) || (pRoot->pRight == NULL))
        {
            // One or no children case
            AVLNode* temp = pRoot->pLeft ? pRoot->pLeft : pRoot->pRight;

            if (temp == NULL)
            {
                // No child case
                temp = pRoot;
                pRoot = NULL;
            }
            else
            {
                // One child case
                *pRoot = *temp;
            }

            free(temp);
        }
        else
        {
            // Node with two children: Get the inorder successor (smallest in the right subtree)
            AVLNode* temp = AVLNode_FindMin(pRoot->pRight);

            // Copy the inorder successor's data to this node
            pRoot->pKey = temp->pKey;
            pRoot->pValue = temp->pValue;

            // Delete the inorder successor
            pRoot->pRight = AVLNode_Delete(pRoot->pRight, temp->pKey, pfnCompare);
        }
    }

    // If the tree had only one node then return
    if (!pRoot)
    {
        return pRoot;
    }

    // Update height of the current node
    pRoot->nHeight = 1 + max(AVLNode_Height(pRoot->pLeft), AVLNode_Height(pRoot->pRight));

    // Get the balance factor of this node
    int balance = AVLNode_GetBalance(pRoot);

    // Left Left Case
    if (balance > 1 && AVLNode_GetBalance(pRoot->pLeft) >= 0)
    {
        return AVLNode_RightRotate(pRoot);
    }

    // Left Right Case
    if (balance > 1 && AVLNode_GetBalance(pRoot->pLeft) < 0)
    {
        pRoot->pLeft = AVLNode_LeftRotate(pRoot->pLeft);
        return AVLNode_RightRotate(pRoot);
    }

    // Right Right Case
    if (balance < -1 && AVLNode_GetBalance(pRoot->pRight) <= 0)
    {
        return AVLNode_LeftRotate(pRoot);
    }

    // Right Left Case
    if (balance < -1 && AVLNode_GetBalance(pRoot->pRight) > 0)
    {
        pRoot->pRight = AVLNode_RightRotate(pRoot->pRight);
        return AVLNode_LeftRotate(pRoot);
    }

    return pRoot;
}

/**
 * [PUBLIC]
 *
 * Function to remove a key-value pair from the hash map
 */
void HashMap_Remove(HashMap* pHashMap, void* pKey)
{
    int nIndex = BucketHash(pKey, pHashMap->nCapacity);
    pHashMap->ppBuckets[nIndex] = AVLNode_Delete(pHashMap->ppBuckets[nIndex], pKey, pHashMap->pfnCompare);
    pHashMap->nSize--;
}

/**
 * Function to destroy the hash map and free memory
 */
void HashMap_Destroy(HashMap* pHashMap)
{
    /* Destroy each bucket tree */
    for (int i = 0; i < pHashMap->nCapacity; ++i)
    {
        AVLNode_DestroyTree(pHashMap->ppBuckets[i]);
    }

    /*
     * Free resources
     * Delete buckets memory and hashmap object memory itself
     */
    free(pHashMap->ppBuckets);
    free(pHashMap);
}
