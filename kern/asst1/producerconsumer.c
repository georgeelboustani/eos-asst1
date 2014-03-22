/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include "producerconsumer_driver.h"
#include <lib.h>
#include <thread.h>
#include <synch.h>

int buffer_full(void);
int buffer_empty(void);
void buffer_push(struct pc_data);
struct pc_data buffer_pop(void);

struct lock *buffer_lock;
struct cv *cv_empty;
struct cv *cv_full;

struct pc_data packets[BUFFER_SIZE];
int start;
int count;

int buffer_full(void) {
	return count == BUFFER_SIZE;
}

int buffer_empty(void) {
	return count == 0;
}
 
void buffer_push(struct pc_data packet) {
	int end = (start + count) % BUFFER_SIZE;
	packets[end] = packet;
	if (count == BUFFER_SIZE) {
		start = (start + 1) % BUFFER_SIZE;
	} else {
		count++;
	}
}
 
struct pc_data buffer_pop(void) {
    struct pc_data packet = packets[start];
    start = (start + 1) % BUFFER_SIZE;
    count--;
    return packet;
}

/* This is called by a consumer to request more data. */
struct pc_data
consumer_consume(void)
{
	lock_acquire(buffer_lock);
	while (buffer_empty()) {
		cv_wait(cv_empty, buffer_lock);
	}
	struct pc_data thedata = buffer_pop();
	cv_signal(cv_full, buffer_lock);
	lock_release(buffer_lock);
	
	return thedata;
}

/* This is called by a producer to store data. */
void
producer_produce(struct pc_data item)
{
	lock_acquire(buffer_lock);
	while (buffer_full()) {
		cv_wait(cv_full, buffer_lock);
	}
	buffer_push(item);
	cv_signal(cv_empty, buffer_lock);
	lock_release(buffer_lock);
}

/* Perform any initialisation (e.g. of global data) you need here */
void
producerconsumer_startup(void)
{
	/* create a lock to be used on the counter */
	buffer_lock = lock_create("buffer_lock");
	if (buffer_lock == NULL) {
		panic("producerconsumer: buffer_lock create failed");
	}

	/* create a lock to be used on the counter */
	cv_empty = cv_create("cv_empty");
	if (cv_empty == NULL) {
		panic("producerconsumer: cv_empty create failed");
	}

		/* create a lock to be used on the counter */
	cv_full = cv_create("cv_full");
	if (cv_full == NULL) {
		panic("producerconsumer: cv_full create failed");
	}

	start = 0;
	count = 0;
}

/* Perform any clean-up you need here */
void
producerconsumer_shutdown(void)
{
	lock_destroy(buffer_lock);
	cv_destroy(cv_empty);
	cv_destroy(cv_full);
}
