/* The size of the buffer. The buffer must be exactly this size,
 * ie producer_produce() on more than this size will block,
 * but producer_produce() won't block on this size or less. */
#define BUFFER_SIZE 10

/* This is the data that you will be passing around in your data structure */
struct pc_data {
	int item1;
	int item2;
};

extern int run_producerconsumer(int, char**);

/* Prototypes for the functions you need to write in producerconsumer.c */
struct pc_data consumer_consume(void);
void producer_produce(struct pc_data);
void producerconsumer_startup(void);
void producerconsumer_shutdown(void);

