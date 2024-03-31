#include "precomp.h"

#include "queue.h"

/* Function to create a new queue */
Queue* CreateQueue()
{
    Queue* pQueue = (Queue*)calloc(1, sizeof(Queue));

    if (!pQueue)
    {
        /* Memory allocation error */
    }

    pQueue->front = NULL;
    pQueue->rear = NULL;

    return pQueue;
}

/* Function to check if the queue is empty */
BOOL Queue_IsEmpty(Queue* pQueue)
{
    return !pQueue->front;
}

void Queue_Enqueue(Queue* pQueue, void* pData)
{
    /* Create a new node */
    QueueNode* pNode = (QueueNode*)calloc(1, sizeof(QueueNode));

    if (!pNode)
    {
        /* Memory allocation error */
    }
    
    pNode->data = pData;
    pNode->next = NULL;

    /* If the queue is empty, set both front and rear to the new node */
    if (Queue_IsEmpty(pQueue))
    {
        pQueue->front = pNode;
        pQueue->rear = pNode;
    }
    else {
        /* Otherwise, add the new node to the rear and update the rear pointer */
        pQueue->rear->next = pNode;
        pQueue->rear = pNode;
    }
}

/* Function to dequeue an element */
void* Queue_Dequeue(Queue* pQueue)
{
    if (Queue_IsEmpty(pQueue))
    {
        /* Empty queue error */
        return NULL;
    }

    /* Get the data from the front node */
    void* pData = pQueue->front->data;

    /* Move the front pointer to the next node */
    QueueNode* temp = pQueue->front;
    pQueue->front = pQueue->front->next;

    /* If the queue becomes empty, update the rear pointer to NULL */
    if (!pQueue->front)
    {
        pQueue->rear = NULL;
    }

    /* Free the memory of the dequeued node */
    free(temp);

    return pData;
}

/* Function to free the memory used by the queue */
void Queue_Destroy(Queue* pQueue)
{
    while (!Queue_IsEmpty(pQueue))
    {
        Queue_Dequeue(pQueue);
    }
    free(pQueue);
}
