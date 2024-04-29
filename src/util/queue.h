#pragma once

typedef struct QueueNode QueueNode;
struct QueueNode {
    void* data;
    QueueNode* next;
};

typedef struct Queue Queue;
struct Queue {
    QueueNode* front;
    QueueNode* rear;
};

Queue* CreateQueue();
BOOL Queue_IsEmpty(Queue* pQueue);
void Queue_Enqueue(Queue* pQueue, void* pData);
void* Queue_Dequeue(Queue* pQueue);
void Queue_Destroy(Queue* pQueue);
