#include "../precomp.h"

#include "stack.h"

Stack* CreateStack()
{
	Stack* stack = (Stack*)malloc(sizeof(Stack));
	memset(stack, 0, sizeof(Stack));
	if (!stack)
	{
		return NULL;
	}
	stack->top = NULL;
	return stack;
}

void Stack_Push(Stack* stack, void* node)
{
	StackNode* newNode = (StackNode*)malloc(sizeof(StackNode));
	memset(newNode, 0, sizeof(StackNode));
	if (newNode)
	{
		newNode->node = node;
		newNode->next = stack->top;
		stack->top = newNode;
	}
}

void* Stack_Pop(Stack* stack)
{
	if (!stack->top)
	{
		return NULL;
	}

	void* poppedNode = stack->top->node;
	StackNode* temp = stack->top;
	stack->top = stack->top->next;
	free(temp);
	return poppedNode;
}
