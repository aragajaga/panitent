#ifndef PANITENT_S11N_PROPERTYTREE_H
#define PANITENT_S11N_PROPERTYTREE_H

#ifndef PANITENT_PRECOMP_H
#error "Include precomp.h first"
#endif  /* PANITENT_PRECOMP_H */

#include "tentany.h"

typedef struct PropertyTreeNode PropertyTreeNode;
struct PropertyTreeNode {
    char* key;
    TENTAny value;
    PropertyTreeNode* child;
    PropertyTreeNode* sibling;
};

/* Function to create a new tree node */
inline PropertyTreeNode* PropertyTreeNode_CreateNode(const char* key, TENTAny* value)
{
    PropertyTreeNode* pNode = (PropertyTreeNode*)malloc(sizeof(PropertyTreeNode));
    pNode->key = _strdup(key);
    pNode->value = *value;
    pNode->child = NULL;
    pNode->sibling = NULL;
    return pNode;
}

/* Function to add a child node to a parent node */
inline void PropertyTreeNode_AddChildNode(PropertyTreeNode* parent, PropertyTreeNode* child)
{
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

#endif  /* PANITENT_S11N_PROPERTYTREE_H */
