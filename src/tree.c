#include "precomp.h"

#include "tree.h"
#include "stack.h"

TreeNode* BinaryTree_AllocEmptyNode()
{
	return (TreeNode*)calloc(1, sizeof(TreeNode));
}

void TreeTraversalRLOT_Init(TreeTraversalRLOT* pTreeTraversalRLOT, TreeNode* pNode)
{
	Stack* stack1 = CreateStack();
	Stack* stack2 = CreateStack();
	pTreeTraversalRLOT->stack1 = stack1;
	pTreeTraversalRLOT->stack2 = stack2;

	Stack_Push(pTreeTraversalRLOT->stack1, pNode);

	while (stack1->top)
	{
		/* Pop a node for the first stack */
		TreeNode* temp = Stack_Pop(stack1);

		/* Push the node to the second stack */
		Stack_Push(stack2, temp);

		/* Push left and right children of the popped node to the first stack */
		if (temp->node1)
		{
			Stack_Push(stack1, temp->node1);
		}
		if (temp->node2)
		{
			Stack_Push(stack1, temp->node2);
		}
	}
}

TreeNode* TreeTraversalRLOT_GetNext(TreeTraversalRLOT* pTreeTraversalRLOT)
{
	Stack* stack2 = pTreeTraversalRLOT->stack2;
	TreeNode* temp = Stack_Pop(stack2);

	return temp;
}

void TreeTraversalRLOT_Destroy(TreeTraversalRLOT* pTreeTraversalRLOT)
{
	free(pTreeTraversalRLOT->stack1);
	free(pTreeTraversalRLOT->stack2);
}

void InOrderTreeTraversal_Init(InOrderTreeTraversal* pInOrderTreeTraversal, TreeNode* pNode)
{
	Stack* stack1 = CreateStack();
	Stack* stack2 = CreateStack();
	pInOrderTreeTraversal->stack1 = stack1;
	pInOrderTreeTraversal->stack2 = stack2;

	/* Push all left nodes onto stack1 */
	while (pNode)
	{
		Stack_Push(stack1, pNode);
		pNode = pNode->node1;
	}
}

TreeNode* InOrderTreeTraversal_GetNext(InOrderTreeTraversal* pInOrderTreeTraversal)
{
	if (!pInOrderTreeTraversal->stack1->top && !pInOrderTreeTraversal->stack2->top)
	{
		/* Traversal completed */
		return NULL;
	}

	/* Process nodes until a leaf is encountered */
	while (((TreeNode*)pInOrderTreeTraversal->stack1->top->node)->node2)
	{
		TreeNode* pCurrentNode = (TreeNode*)Stack_Pop(pInOrderTreeTraversal->stack1);
		Stack_Push(pInOrderTreeTraversal->stack2, pCurrentNode);
		InOrderTreeTraversal_Init(pInOrderTreeTraversal, pCurrentNode->node2);
	}

	/* Pop the leaf node from stack1 and return */
	TreeNode* pNodeResult = Stack_Pop(pInOrderTreeTraversal->stack1);
	return pNodeResult;
}

void InOrderTreeTraversal_Destroy(InOrderTreeTraversal* pInOrderTreeTraversal)
{
	free(pInOrderTreeTraversal->stack1);
	free(pInOrderTreeTraversal->stack2);
}


void PreOrderTreeTraversal_Init(PreOrderTreeTraversal* pPreOrderTreeTraversal, TreeNode* pNode)
{
	Stack* stack1 = CreateStack();
	Stack* stack2 = CreateStack();
	pPreOrderTreeTraversal->stack1 = stack1;
	pPreOrderTreeTraversal->stack2 = stack2;

	Stack_Push(pPreOrderTreeTraversal->stack1, pNode);
	while (pPreOrderTreeTraversal->stack1->top)
	{
		TreeNode* temp = Stack_Pop(pPreOrderTreeTraversal->stack1);
		Stack_Push(pPreOrderTreeTraversal->stack2, temp);

		/* Push left and right children of the popped node to the first stack */
		if (temp->node1)
		{
			Stack_Push(pPreOrderTreeTraversal->stack1, temp->node1);
		}
		if (temp->node2)
		{
			Stack_Push(pPreOrderTreeTraversal->stack1, temp->node2);
		}
	}
}

TreeNode* PreOrderTreeTraversal_GetNext(PreOrderTreeTraversal* pPreOrderTreeTraversal)
{
	if (!pPreOrderTreeTraversal->stack2->top)
	{
		/* Traversal completed */
		return NULL;
	}

	TreeNode* pResultNode = Stack_Pop(pPreOrderTreeTraversal);
	return pResultNode;
}

void PreOrderTreeTraversal_Destroy(PreOrderTreeTraversal* pPreOrderTreeTraversal)
{
	free(pPreOrderTreeTraversal->stack1);
	free(pPreOrderTreeTraversal->stack2);
}

void PostOrderTreeTraversal_Init(PostOrderTreeTraversal* pPostOrderTreeTraversal, TreeNode* pNode)
{
	Stack* stack = CreateStack();
	pPostOrderTreeTraversal->stack = stack;

	Stack_Push(pPostOrderTreeTraversal->stack, pNode);
}

TreeNode* PostOrderTreeTraversal_GetNext(PostOrderTreeTraversal* pPostOrderTreeTraversal)
{
	if (!pPostOrderTreeTraversal->stack->top)
	{
		return NULL;
	}

	TreeNode* pCurrentNode = Stack_Pop(pPostOrderTreeTraversal->stack);
	TreeNode* pNodeResult = pCurrentNode;

	if (pCurrentNode->node2)
	{
		Stack_Push(pPostOrderTreeTraversal->stack, pCurrentNode->node2);
	}
	if (pCurrentNode->node1)
	{
		Stack_Push(pPostOrderTreeTraversal->stack, pCurrentNode->node1);
	}

	return pNodeResult;
}

void PostOrderTreeTraversal_Destroy(PostOrderTreeTraversal* pPostOrderTreeTraversal)
{
	free(pPostOrderTreeTraversal->stack);
}
