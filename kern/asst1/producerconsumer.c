/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include "producerconsumer_driver.h"

/* This is called by a consumer to request more data. */
struct pc_data
consumer_consume(void)
{
	struct pc_data thedata;

	/* FIXME: this data should come from your buffer, obviously... */
	thedata.item1 = 1;
	thedata.item2 = 2;

	return thedata;
}

/* This is called by a producer to store data. */
void
producer_produce(struct pc_data item)
{
	(void) item; /* Remove this when you add your code */
}

/* Perform any initialisation (e.g. of global data) you need here */
void
producerconsumer_startup(void)
{
}

/* Perform any clean-up you need here */
void
producerconsumer_shutdown(void)
{
}
