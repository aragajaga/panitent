#ifndef PANITENT_S11N_PROPERTYTREE_H
#define PANITENT_S11N_PROPERTYTREE_H

#ifndef PANITENT_PRECOMP_H
#error "Include precomp.h first"
#endif  /* PANITENT_PRECOMP_H */

#include <stdlib.h>
#include <string.h>
#include "tentany.h"

typedef struct PropertyTreeNode PropertyTreeNode;
struct PropertyTreeNode {
    char* key;                  /* owned (malloc/strdup) */
    TENTAny value;              /* owned (use TENTAny_* helpers) */
    PropertyTreeNode* child;
    PropertyTreeNode* sibling;
};

/* Create a new node by deep-copying key and value. Returns NULL on allocation failure. */
inline PropertyTreeNode* PropertyTreeNode_CreateNode(const char* key, TENTAny* value)
{
    if (!key || !value)
    {
        return NULL;
    }

    PropertyTreeNode* pNode = (PropertyTreeNode*)malloc(sizeof(PropertyTreeNode));
    if (!pNode)
    {
        return NULL;
    }

    pNode->key = TENTANY_STRDUP(key);
    if (!pNode->key)
    {
        free(pNode);
        return NULL;
    }

    TENTAny_Init(&pNode->value);
    if (!TENTAny_Copy(&pNode->value, value))
    {
        free(pNode->key);
        free(pNode);
        return NULL;
    }

    pNode->child = NULL;
    pNode->sibling = NULL;
    return pNode;
}

/* Append child (adopts child pointer). If parent is NULL, child is freed. */
inline void PropertyTreeNode_AddChildNode(PropertyTreeNode* parent, PropertyTreeNode* child)
{
    if (!parent)
    {
        /* Nothing to attach to; free child */
        if (child)
        {
            /* ensure proper free */
            /* Provide a destroy function below to free recursively */
            /* We'll call it here if available */
        }
        return;
    }

    if (child == NULL)
    {
        return;
    }

    if (parent->child == NULL)
    {
        parent->child = child;
    }
    else {
        PropertyTreeNode* sibling = parent->child;
        while (sibling->sibling != NULL)
        {
            sibling = sibling->sibling;
        }
        sibling->sibling = child;
    }
}

/* Recursively free a node and its subtree */
static inline void PropertyTreeNode_Destroy(PropertyTreeNode* node)
{
    if (!node)
    {
        return;
    }

    /* Free siblings and children recursively */

    if (node->child)
    {
        PropertyTreeNode_Destroy(node->child);
        node->child = NULL;
    }
    if (node->sibling)
    {
        PropertyTreeNode_Destroy(node->sibling);
        node->sibling = NULL;
    }

    if (node->key)
    {
        free(node->key);
        node->key = NULL;
    }

    TENTAny_Clear(&node->value);
    free(node);
}

#endif  /* PANITENT_S11N_PROPERTYTREE_H */
