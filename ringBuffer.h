
#define BUFFER_BITS 4 // valid values [2-7]
#define BUFFER_SIZE (1<<BUFFER_BITS)
#define BUFFER_MASK (BUFFER_SIZE-1)

typedef volatile struct
{
	unsigned char head; // write to head
	unsigned char tail; // read from tail
	char data[BUFFER_SIZE];
} ringBuffer;

char getChar(ringBuffer* queue);
void putChar(ringBuffer *queue, char data);
int isEmpty(ringBuffer *queue);
unsigned int size(ringBuffer *queue);
int isFull(ringBuffer *queue);
int isOverFlow(ringBuffer *queue);
