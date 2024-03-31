#pragma once

typedef struct TreeNode TreeNode;
struct TreeNode {
	struct TreeNode* node1;
	struct TreeNode* node2;
	void* data;
};

TreeNode* BinaryTree_AllocEmptyNode();

typedef struct Stack Stack;

typedef struct TreeTraversalRLOT TreeTraversalRLOT;
struct TreeTraversalRLOT {
	Stack* stack1;
	Stack* stack2;
};

void TreeTraversalRLOT_Init(TreeTraversalRLOT* pTreeTraversalRLOT, TreeNode* pNode);
TreeNode* TreeTraversalRLOT_GetNext(TreeTraversalRLOT* pTreeTraversalRLOT);
void TreeTraversalRLOT_Destroy(TreeTraversalRLOT* pTreeTraversalRLOT);

typedef struct InOrderTreeTraversal InOrderTreeTraversal;
struct InOrderTreeTraversal {
	Stack* stack1;
	Stack* stack2;
};

void InOrderTreeTraversal_Init(InOrderTreeTraversal* pInOrderTreeTraversal, TreeNode* pNode);
TreeNode* InOrderTreeTraversal_GetNext(InOrderTreeTraversal* pInOrderTreeTraversal);
void InOrderTreeTraversal_Destroy(InOrderTreeTraversal* pInOrderTreeTraversal);

typedef struct PreOrderTreeTraversal PreOrderTreeTraversal;
struct PreOrderTreeTraversal {
	Stack* stack1;
	Stack* stack2;
};

void PreOrderTreeTraversal_Init(PreOrderTreeTraversal* pPreOrderTreeTraversal, TreeNode* pNode);
TreeNode* PreOrderTreeTraversal_GetNext(PreOrderTreeTraversal* pPreOrderTreeTraversal);
void PreOrderTreeTraversal_Destroy(PreOrderTreeTraversal* pPreOrderTreeTraversal);

typedef struct PostOrderTreeTraversal PostOrderTreeTraversal;
struct PostOrderTreeTraversal {
	Stack* stack;
};

void PostOrderTreeTraversal_Init(PostOrderTreeTraversal* pPostOrderTreeTraversal, TreeNode* pNode);
TreeNode* PostOrderTreeTraversal_GetNext(PostOrderTreeTraversal* pPostOrderTreeTraversal);
void PostOrderTreeTraversal_Destroy(PostOrderTreeTraversal* pPostOrderTreeTraversal);
