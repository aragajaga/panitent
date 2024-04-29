#pragma once

typedef struct StackNode StackNode;
struct StackNode {
	void* node;
	StackNode* next;
};

typedef struct Stack Stack;
struct Stack {
	StackNode* top;
};

struct Stack* CreateStack();
void Stack_Push(struct Stack* stack, void* node);
void* Stack_Pop(struct Stack* stack);
