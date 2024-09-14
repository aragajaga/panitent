#include "../precomp.h"

#include "rbhashmap.h"

/*
 * [PUBLIC]
 * 
 * Create a new Red-Black Tree node with the specified key, value, and color
 */
RBTreeNode* RBTreeNode_Create(void* pKey, void* pValue, RBTreeColor color)
{
    /* Allocate memory for the new node */
    RBTreeNode* pNode = (RBTreeNode*)malloc(sizeof(RBTreeNode));
    if (!pNode)
    {
        return NULL;
    }
    memset(pNode, 0, sizeof(RBTreeNode));

    pNode->pKey = pKey;
    pNode->pValue = pValue;
    pNode->color = color;
}

/*
 * [PUBLIC]
 * 
 * Destory a Red-Black Tree node and optionally call key/value deleters
 */
void RBTreeNode_Destroy(RBTreeNode* pNode, void (*pfnKeyDeleter)(void* pKey), void (*pfnValueDeleter)(void* pValue))
{
    if (pNode)
    {
        /* Call the key deleter if it exists */
        if (pNode->pKey && pfnKeyDeleter)
        {
            pfnKeyDeleter(pNode->pKey);
        }

        /* Call the value deleter if it exists */
        if (pNode->pValue && pfnValueDeleter)
        {
            pfnValueDeleter(pNode->pValue);
        }

        /* Free the memory allocated for the node */
        free(pNode);
    }
}

/*
 * [PRIVATE]
 * 
 * Perform a left rotation on the node x in the Red-Black Tree
 */
void RBTree_LeftRotate(RBTree* tree, RBTreeNode* x)
{
    RBTreeNode* y = x->pRight;

    /* Turn y's left subtree into x's right subtree */
    x->pRight = y->pLeft;

    if (y->pLeft)
    {
        y->pLeft->pParent = x;
    }

    /* Link x's parent to y */
    y->pParent = x->pParent;

    /* If x was root, make y the new root */
    if (!x->pParent)
    {
        tree->pRoot = y;
    }
    /* If x is a left child */
    else if (x == x->pParent->pLeft)
    {
        x->pParent->pLeft = y;
    }
    /* If x is a right child */
    else {        
        x->pParent->pRight = y;
    }

    /* Put x on y's left */
    y->pLeft = x;
    x->pParent = y;
}

void RBTree_RightRotate(RBTree* tree, RBTreeNode* y)
{
    RBTreeNode* x = y->pLeft;

    y->pLeft = x->pRight;

    if (x->pRight)
    {
        x->pRight->pParent = y;
    }

    x->pParent = y->pParent;

    if (!y->pParent)
    {
        tree->pRoot = x;
    }
    else if (y == y->pParent->pLeft)
    {
        y->pParent->pLeft = x;
    }
    else {
        y->pParent->pRight = x;
    }

    x->pRight = y;
    y->pParent = x;
}

void RBTree_InsertFixUp(RBTree* pTree, RBTreeNode* z)
{
    while (z->pParent && z->pParent->color == RED)
    {
        /*
        if (!z->pParent->pParent)
        {
            return;
        }
        */

        if (z->pParent == z->pParent->pParent->pLeft)
        {
            RBTreeNode* y = z->pParent->pParent->pRight;

            if (y && y->color == RED)
            {
                z->pParent->color = BLACK;
                y->color = BLACK;
                z->pParent->pParent->color = RED;
                z = z->pParent->pParent;
            }
            else {
                if (z == z->pParent->pRight)
                {
                    z = z->pParent;
                    RBTree_LeftRotate(pTree, z);
                }

                z->pParent->color = BLACK;
                z->pParent->pParent->color = RED;
                RBTree_RightRotate(pTree, z->pParent->pParent);
            }
        }
        else {
            RBTreeNode* y = z->pParent->pParent->pLeft;
            if (y && y->color == RED)
            {
                z->pParent->color = BLACK;
                y->color = BLACK;
                z->pParent->pParent->color = RED;
                z = z->pParent->pParent;
            }
            else {
                if (z == z->pParent->pLeft)
                {
                    z = z->pParent;
                    RBTree_RightRotate(pTree, z);
                }
                z->pParent->color = BLACK;
                z->pParent->pParent->color = RED;
                RBTree_LeftRotate(pTree, z->pParent->pParent);
            }
        }
    }
    pTree->pRoot->color = BLACK;
}

void RBTree_DeleteFixUp(RBTree* tree, RBTreeNode* x)
{
    while (x != tree->pRoot && (!x || x->color == BLACK))
    {
        if (x == x->pParent->pLeft)
        {
            RBTreeNode* w = x->pParent->pRight;
            if (w->color == RED)
            {
                w->color = BLACK;
                w->pParent->color = RED;
                RBTree_LeftRotate(tree, x->pParent);
                w = x->pParent->pRight;
            }

            if ((!w->pLeft || w->pLeft->color == BLACK) && (!w->pRight || w->pRight->color == BLACK))
            {
                w->color = RED;
                x = x->pParent;
            }
            else {
                if (!w->pRight || w->pRight->color == BLACK)
                {
                    if (w->pLeft)
                    {
                        w->pLeft->color = BLACK;
                    }
                    w->color = RED;
                    RBTree_RightRotate(tree, w);
                    w = x->pParent->pRight;
                }
                w->color = x->pParent->color;
                x->pParent->color = BLACK;
                if (w->pRight)
                {
                    w->pRight->color = BLACK;
                }

                RBTree_LeftRotate(tree, x->pParent);
                x = tree->pRoot;
            }
        }
        else {
            RBTreeNode* w = x->pParent->pLeft;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->pParent->color = RED;
                RBTree_RightRotate(tree, x->pParent);
                w = x->pParent->pLeft;
            }

            if ((!w->pLeft || w->pLeft->color == BLACK) && (!w->pRight || w->pRight->color == BLACK))
            {
                w->color = RED;
                x = x->pParent;
            }
            else {
                if (!w->pLeft || w->pLeft->color == BLACK)
                {
                    if (w->pRight)
                    {
                        w->pRight->color = BLACK;
                    }

                    w->color = RED;
                    RBTree_LeftRotate(tree, w);
                    w = x->pParent->pLeft;
                }

                w->color = x->pParent->color;
                x->pParent->color = BLACK;
                if (w->pLeft)
                {
                    w->pLeft->color = BLACK;
                }
                RBTree_RightRotate(tree, x->pParent);
                x = tree->pRoot;
            }
        }
    }

    if (x)
    {
        x->color = BLACK;
    }
}

void RBTree_Insert(RBTree* tree, void* key, void* value)
{
    RBTreeNode* z = NULL;
    z = RBTreeNode_Create(key, value, RED);
    if (!z)
    {
        return;
    }

    RBTreeNode* y = NULL;
    RBTreeNode* x = tree->pRoot;

    while (x)
    {
        y = x;
        if (tree->pfnKeyComparator(z->pKey, x->pKey) < 0)
        {
            x = x->pLeft;
        }
        else {
            x = x->pRight;
        }
    }

    z->pParent = y;

    if (!y)
    {
        tree->pRoot = z;
    }
    else if (tree->pfnKeyComparator(z->pKey, y->pKey) < 0)
    {
        y->pLeft = z;
    }
    else {
        y->pRight = z;
    }

    RBTree_InsertFixUp(tree, z);
}

RBTreeNode* RBTree_Search(RBTree* pTree, void* pKey)
{
    RBTreeNode* pNode = pTree->pRoot;
    while (pNode)
    {
        int cmp = pTree->pfnKeyComparator(pKey, pNode->pKey);

        if (!cmp)
        {
            return pNode;
        }

        if (cmp < 0)
        {
            pNode = pNode->pLeft;
        }
        else {
            pNode = pNode->pRight;
        }
    }

    return NULL;
}

void RBTree_Transplant(RBTree* pTree, RBTreeNode* pNodeX, RBTreeNode* pNodeY)
{
    if (pNodeX->pParent)
    {
        pTree->pRoot = pNodeY;
    }
    else if (pNodeX == pNodeX->pParent->pLeft)
    {
        pNodeX->pParent->pLeft = pNodeY;
    }
    else {
        pNodeX->pParent->pRight = pNodeY;
    }

    if (pNodeY)
    {
        pNodeY->pParent = pNodeX->pParent;
    }
}

RBTreeNode* RBTreeNode_Minimum(RBTreeNode* node)
{
    while (node->pLeft)
    {
        node = node->pLeft;
    }

    return node;
}

void RBTree_Delete(RBTree* pTree, void* pKey)
{
    RBTreeNode* z = RBTree_Search(pTree, pKey);

    if (!z)
    {
        return;
    }

    RBTreeNode* y = z;
    RBTreeNode* x;
    RBTreeColor yOriginalColor = y->color;

    if (!z->pLeft)
    {
        x = z->pRight;
        RBTree_Transplant(pTree, z, z->pRight);
    }
    else if (!z->pRight)
    {
        x = z->pLeft;
        RBTree_Transplant(pTree, z, z->pLeft);
    }
    else {
        y = RBTreeNode_Minimum(z->pRight);
        yOriginalColor = y->color;
        x = y->pRight;

        if (y->pParent == z)
        {
            if (x)
            {
                x->pParent = y;
            }
        }
        else {
            RBTree_Transplant(pTree, y, y->pRight);
            y->pRight = z->pRight;
            y->pRight->pParent = y;
        }

        RBTree_Transplant(pTree, z, y);
        y->pLeft = z->pLeft;
        y->pLeft->pLeft = y;
        y->color = z->color;
    }

    RBTreeNode_Destroy(z, pTree->pfnKeyDeleter, pTree->pfnValueDeleter);

    if (yOriginalColor == BLACK)
    {
        RBTree_DeleteFixUp(pTree, x);
    }
}

RBHashMap* RBHashMap_Create(RBHashMapContext* ctx)
{
    RBHashMap* map = (RBHashMap*)malloc(sizeof(RBHashMap));

    if (!map)
    {
        return NULL;
    }

    map->tree = (RBTree*)malloc(sizeof(RBTree));
    if (!map->tree)
    {
        free(map);
        return NULL;
    }

    map->tree->pfnKeyComparator = ctx->pfnKeyComparator;
    map->tree->pfnKeyDeleter = ctx->pfnKeyDeleter;
    map->tree->pfnValueDeleter = ctx->pfnValueDeleter;

    map->tree->pRoot = NULL;

    return map;
}

void RBHashMap_Insert(RBHashMap* map, void* key, void* value)
{
    RBTree_Insert(map->tree, key, value);
}

void* RBHashMap_Lookup(RBHashMap* map, void* key)
{
    RBTreeNode* node = RBTree_Search(map->tree, key);

    return node ? node->pValue : NULL;
}

void* RBHashMap_Delete(RBHashMap* map, void* key)
{
    RBTree_Delete(map->tree, key);
}

void FreeRBTreeNodes(RBTreeNode* node, void (*pfnKeyDeleter)(void* pKey), void (*pfnValueDeleter)(void* pValue))
{
    if (!node)
    {
        return;
    }

    /*
     * TODO: Make iterative
     */

    FreeRBTreeNodes(node->pLeft, pfnKeyDeleter, pfnValueDeleter);
    FreeRBTreeNodes(node->pRight, pfnKeyDeleter, pfnValueDeleter);
    RBTreeNode_Destroy(node, pfnKeyDeleter, pfnValueDeleter);
}

void RBHashMap_Destroy(RBHashMap* map)
{
    if (!map)
    {
        return;
    }

    if (map->tree)
    {
        FreeRBTreeNodes(map->tree->pRoot, map->tree->pfnKeyDeleter, map->tree->pfnValueDeleter);
        free(map->tree);
    }
    free(map);
}
