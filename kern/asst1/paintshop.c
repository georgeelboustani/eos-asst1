#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "paintshop.h"
#include "paintshop_driver.h"



/*
 * **********************************************************************
 * OUR Structs and Global Variables
 */

struct order_form {
    struct paintcan* can;
    struct semaphore *status; // Used to track the status of the order
};

// Number of customers in the store
int num_customers;
int *available_paint;

// Lock on the paint tint array
struct lock *paint_array_lock;
// Lock on the paint tints
struct lock **paint_tint_locks;

// Lock on the order queue
struct lock *order_queue_lock;
// Condition variable for empty queue
struct cv *cv_order_queue_empty;
// Condition variable for full queue
struct cv *cv_order_queue_full;
// Condition variable for available tints
struct cv *cv_tints_unavailable;

int tintsAvailable(struct paintcan*);

/*
 * **********************************************************************
 * Circular queue stuff to handle the order queue
 * **********************************************************************
 */

int order_queue_full(void);
int order_queue_empty(void);
void order_queue_push(struct order_form*);
struct order_form* order_queue_pop(void);

struct order_form **order_queue;
int order_queue_start;
int order_queue_count;

// Return true if queue is full anf false otherwise
int order_queue_full(void) {
	return order_queue_count == NCUSTOMERS;
}
// Return true if queue is empty and false otherwise
int order_queue_empty(void) {
	return order_queue_count == 0;
}
// Push the order provided onto the queue
void order_queue_push(struct order_form* order) {
	int end = (order_queue_start + order_queue_count) % NCUSTOMERS;
	order_queue[end] = order;
	if (order_queue_count == NCUSTOMERS) {
		order_queue_start = (order_queue_start + 1) % NCUSTOMERS;
	} else {
		order_queue_count++;
	}
}
// Remove the first item on the queue and return it
struct order_form* order_queue_pop(void) {
    struct order_form* order = order_queue[order_queue_start];
    order_queue_start = (order_queue_start + 1) % NCUSTOMERS;
    order_queue_count--;
    return order;
}

/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_paint()
 *
 * Takes one argument specifying the can to be filled. The function
 * makes the can available to staff threads and then blocks until the staff
 * have filled the can with the appropriately tinted paint.
 *
 * The can itself contains an array of requested tints.
 */ 

void order_paint(struct paintcan *can)
{
	// Create an order form. This includes the can along with a status semaphore
	struct order_form* order = kmalloc(sizeof(struct order_form *));
	order->can = can;
	order->status = sem_create("order_form_sem", 0);
	if (order->status == NULL) {
		panic("paintshop: sem create failed");
	}

	// Add the order to the queue
	lock_acquire(order_queue_lock);
	while (order_queue_full()) {
		cv_wait(cv_order_queue_full, order_queue_lock);
	}
	order_queue_push(order);
	cv_signal(cv_order_queue_empty, order_queue_lock);
	lock_release(order_queue_lock);

	// Customer waits for the order to be filled
	P(order->status);

	// Clean up after customers order
	sem_destroy(order->status);
	kfree(order);
}



/*
 * go_home()
 *
 * This function is called by customers when they go home. It could be
 * used to keep track of the number of remaining customers to allow
 * paint shop staff threads to exit when no customers remain.
 */

void go_home(void)
{
	num_customers--;

	// If store is empty, tell all the waiting staff to stop waiting
	if (num_customers == 0) {
		cv_broadcast(cv_order_queue_empty, order_queue_lock);
	}
}


/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY PAINT SHOP STAFF THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it records the details, and returns a
 * pointer to something representing the order.
 *
 * The return pointer type is void * to allow freedom of representation
 * of orders.
 *
 * The function can return NULL to signal the staff thread it can now
 * exit as their are no customers nor orders left. 
 */

void * take_order(void)
{
	// Get access to the order queue
	lock_acquire(order_queue_lock);
	while (num_customers > 0 && order_queue_empty()) {
		cv_wait(cv_order_queue_empty, order_queue_lock);
	}

	if (num_customers > 0) {
		// Staff takes an order to work on
		struct order_form* order = order_queue_pop();

		cv_signal(cv_order_queue_full, order_queue_lock);
		lock_release(order_queue_lock);

		return order;
	} else {
		// There is no more customers, so staff should leave
		lock_release(order_queue_lock);
		return NULL;
	}
}


/*
 * fill_order()
 *
 * This function takes an order generated by take order and fills the
 * order using the mix() function to tint the paint.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE TINTS
 * IT NEEDS TO USE TO FILE THE ORDER.
 */

void fill_order(void *v)
{
	int i;
	struct order_form* form = (struct order_form*)v;

	// Wait until we have acces to all the tints we need
	lock_acquire(paint_array_lock);
	while (!tintsAvailable(form->can)) {
		cv_wait(cv_tints_unavailable, paint_array_lock);
	}

	// Gain exclusive access to the tints we need
	struct paintcan* can = form->can;
	for (i = 0; i < PAINT_COMPLEXITY; i++) {
		int col = can->requested_colours[i];

		// Has a colour been specified
		if (col > 0) {
			struct lock* colourLock = paint_tint_locks[col - 1];
			// Have we already acquired the colours lock
			if (!lock_do_i_hold(colourLock)) {
				available_paint[col - 1] = 0;
				lock_acquire(colourLock);
			}
		}
	}
	lock_release(paint_array_lock);
	
	// Fulfill the orders now that we have access to our tints
	mix(can);

	// Release the locks to the tints we were using
	for (i = 0; i < PAINT_COMPLEXITY; i++) {
		int col = can->requested_colours[i];

		// Has a colour been specified
		if (col > 0) {
			struct lock* colourLock = paint_tint_locks[col - 1];
			// Dont release the same lock twice, incase tints are doubled up
			if (lock_do_i_hold(colourLock)) {
				lock_release(colourLock);
				available_paint[col - 1] = 1;
			}
		}
	}
	cv_broadcast(cv_tints_unavailable, paint_array_lock);
}

int tintsAvailable(struct paintcan* can) {
	int i, allAvailable = 0;
	// Check if the colours we need are all available
	for (i = 0; i < PAINT_COMPLEXITY; i++) {
		int col = can->requested_colours[i];
		if (col == 0) {
			// "No colour" is always available
			allAvailable++;
		} else {
			allAvailable += available_paint[col - 1];
		}
	}
	return allAvailable == PAINT_COMPLEXITY;
}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to the waiting customer.
 */

void serve_order(void *v)
{
	struct order_form* form = (struct order_form*)v;
	// Serve the order back to the customer
	V(form->status);
}



/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * paintshop_open()
 *
 * Perform any initialisation you need prior to opening the paint shop to
 * staff and customers
 */

void paintshop_open(void)
{
	int i;

	num_customers = NCUSTOMERS;
	available_paint = kmalloc(sizeof(int) * NCOLOURS);
	for (i = 0; i < NCOLOURS; i++) {
		// Initialise all colours as available
		available_paint[i] = 1;
	}

	order_queue = kmalloc(sizeof(struct paintcan*) * NCUSTOMERS);

	/* create a lock to be used on the paint tint array for each tint*/
	paint_array_lock = lock_create("paint_array_lock");
	if (paint_array_lock == NULL) {
		panic("paintshop: paint_array_lock create failed");
	}
	
	paint_tint_locks = kmalloc(sizeof(struct lock*) * NCOLOURS);
	for (i = 0; i < NCOLOURS; i++) {
        paint_tint_locks[i] = lock_create("paint_tint_locks_" + (i + 1));
		if (paint_tint_locks[i] == NULL) {
			panic("paintshop: paint_tint_locks_%d create failed", i + 1);
		}
	}

	/* create a lock to be used on the counter */
	order_queue_lock = lock_create("order_queue_lock");
	if (order_queue_lock == NULL) {
		panic("paintshop: order_queue_lock create failed");
	}

	/* create a lock to be used on the counter */
	cv_order_queue_empty = cv_create("cv_order_queue_empty");
	if (cv_order_queue_empty == NULL) {
		panic("paintshop: cv_order_queue_empty create failed");
	}

	/* create a lock to be used on the counter */
	cv_order_queue_full = cv_create("cv_order_queue_full");
	if (cv_order_queue_full == NULL) {
		panic("paintshop: cv_order_queue_full create failed");
	}

	/* create a cv to be used on the tints for availability */
	cv_tints_unavailable = cv_create("cv_tints_unavailable");
	if (cv_tints_unavailable == NULL) {
		panic("paintshop: cv_tints_unavailable create failed");
	}

	order_queue_start = 0;
	order_queue_count = 0;
}

/*
 * paintshop_close()
 *
 * Perform any cleanup after the paint shop has closed and everybody
 * has gone home.
 */

void paintshop_close(void)
{
	int i;
	for (i = 0; i < NCOLOURS; i++) {
        lock_destroy(paint_tint_locks[i]);
	}
	kfree(paint_tint_locks);

	lock_destroy(paint_array_lock);
	lock_destroy(order_queue_lock);
	cv_destroy(cv_order_queue_empty);
	cv_destroy(cv_order_queue_full);
	cv_destroy(cv_tints_unavailable);

	kfree(order_queue);
	kfree(available_paint);
}

