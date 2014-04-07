/* The size of the buffer. The buffer must be exactly this size,
 * ie producer_produce() on more than this size will block,
 * but producer_produce() won't block on this size or less. */
#define BUFFER_SIZE 10

/* This is the data that you will be passing around in your data structure */
struct pc_data {
	int item1;
	int item2;
};

// Add a circular_buffer
struct circular_buffer {
	struct pc_data* data;
	int size;
	int start;
	int count;
};

/* Circular buffer access functions */
void initCircularBuffer(struct circular_buffer *cb, int size);
void freeCircularBuffer(struct circular_buffer *cb);
int isFull(struct circular_buffer *cb);
int isEmpty(struct circular_buffer *cb);
void writeBuffer(struct circular_buffer *cb, struct pc_data data);
struct pc_data readBuffer(struct circular_buffer *cb);

extern int run_producerconsumer(int, char**);

/* Prototypes for the functions you need to write in producerconsumer.c */
struct pc_data consumer_consume(void);
void producer_produce(struct pc_data);
void producerconsumer_startup(void);
void producerconsumer_shutdown(void);
