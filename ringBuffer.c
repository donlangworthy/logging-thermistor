

#include "ringBuffer.h"


char getChar(ringBuffer* queue) {
	// if buffer is not empty;
	if (isEmpty(queue)) return 0;
	char result=queue->data[queue->tail&BUFFER_MASK];
	queue->tail++;
	return result;
}

void putChar(ringBuffer *queue, char data)
{
	//if not overflow;
	queue->data[queue->head&BUFFER_MASK]=data;
	queue->head++;
	// signal write process???
}
	
int isEmpty(ringBuffer *queue)
{
	return queue->head == queue->tail;
}

int size(ringBuffer *queue)
{
	return queue->head - queue->tail;
}

int isFull(ringBuffer *queue)
{
	return  size(queue) >= BUFFER_SIZE;
}

int isOverFlow(ringBuffer *queue)
{
	return size(queue) > BUFFER_SIZE;
} 

