/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include "producerconsumer_driver.h"

#define EMPTY 0

struct circular_buffer* buffer;
struct lock* bufferLock;
struct cv* cvEmpty;
struct cv* cvFull;

/* This is called by a consumer to request more data. */
struct pc_data
consumer_consume(void)
{
	lock_acquire(bufferLock);
	while (isEmpty(buffer)) {
		cv_wait(cvEmpty, bufferLock);
	}

	struct pc_data thedata = readBuffer(buffer);

	cv_signal(cvFull, bufferLock);
	lock_release(bufferLock);

	return thedata;
}

/* This is called by a producer to store data. */
void
producer_produce(struct pc_data item)
{
	lock_acquire(bufferLock);
	while (isFull(buffer)) {
		cv_wait(cvFull, bufferLock);
	}

	writeBuffer(buffer, item);

	cv_signal(cvEmpty, bufferLock);
	lock_release(bufferLock);
}

/* Perform any initialisation (e.g. of global data) you need here */
void
producerconsumer_startup(void)
{
	// Initialize all the locking mechanisms.
	cvEmpty = cv_create("cvEmpty");
	if (cvEmpty == NULL) {
		panic("producerconsumer: cvEmpty create failed!");
	}

	cvFull = cv_create("cvFull");
	if (cvFull == NULL) {
		panic("producerconsumer: cvFull create failed!");
	}

	bufferLock = lock_create("bufferLock");
	if (bufferLock == NULL) {
		panic("producerconsumer: bufferLock create failed!");
	}

	buffer = kmalloc(sizeof(buffer));
	if (buffer == NULL) {
		panic("producerconsumer: buffer kmalloc failed!");
	}

	initCircularBuffer(buffer, BUFFER_SIZE);
}

/* Perform any clean-up you need here */
void
producerconsumer_shutdown(void)
{
	freeCircularBuffer(buffer);
	lock_destroy(bufferLock);
	cv_destroy(cvEmpty);
	cv_destroy(cvFull);
}

void initCircularBuffer(struct circular_buffer *cb, int size) {
	cb->size = size;
	cb->count = EMPTY;
	cb->start = EMPTY;
	cb->data = (struct pc_data *)kmalloc(cb->size*sizeof(struct pc_data));
	if (cb->data == NULL) {
		panic("producerconsumer: cb->data kmalloc failed!");
	}
}

void freeCircularBuffer(struct circular_buffer *cb) {
	kfree(cb->data);
	kfree(cb);
}

int isFull(struct circular_buffer *cb) {
	return cb->count == cb->size;
}

int isEmpty(struct circular_buffer *cb) {
	return cb->count == EMPTY;
}

void writeBuffer(struct circular_buffer *cb, struct pc_data data) {
	int end = (cb->start + cb->count) % cb->size;
	cb->data[end] = data;
	if (cb->count == cb->size) {
		cb->start = (cb->start + 1) % cb->size;
	} else {
		++cb->count;
	}
}

struct pc_data readBuffer(struct circular_buffer *cb) {
	struct pc_data data = cb->data[cb->start];

	cb->start = (cb->start + 1) % cb->size;
	--cb->count;

	return data;
}
