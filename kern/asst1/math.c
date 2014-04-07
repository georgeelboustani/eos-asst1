#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>

enum {
	NADDERS = 10,    /* the number of adder threads */
	NADDS   = 10000, /* the number of overall increments to perform */
	NCOUNTERS = 1,   /* the number of counters available */
};



/*
 * Declare the counter variable that all the adder() threads increment 
 *
 * Declaring it "volatile" instructs the compiler to always (re)read the
 * variable from memory and not optimise by removing memory references
 * and re-using the content of a register.
 */
volatile unsigned long int counter;


/*
 * Declare an array of adder counters to count per-thread
 * increments. These are used for printing statistics.
 */  
unsigned long int adder_counters[NADDERS];


/* We use a semaphore to wait for adder() threads to finish */
struct semaphore *finished;


/*
 * **********************************************************************
 * ADD YOUR OWN VARIABLES HERE AS NEEDED
 * **********************************************************************
 */
/* Use a semaphore to indicate how many counters are available for use */
struct semaphore *counterAccess;


/*
 * adder()
 *
 *  Each adder thread simply keeps incrementing the counter until we
 *  hit the max value.
 *
 * **********************************************************************
 * YOU NEED TO INSERT SYNCHRONISATION PRIMITIVES APPROPRIATELY 
 * TO ENSURE COUNTING IS CORRECTLY PERFORMED.
 * **********************************************************************
 *
 * You should not re-write the existing code.
 *
 * + Only the correct number of increments are performed
 * + Ensure x+1 == x+1 
 * + Ensure that the statistics kept match the number of increments
 *   performed.
 *
 *
 */

static void adder(void * unusedpointer, unsigned long addernumber)
{
	unsigned long int a, b;
	int flag = NCOUNTERS;

	/*
	 * Avoid unused variable warnings.
	 */
	(void) unusedpointer; /* remove this line if variable is used */

	while (flag) {
		/* loop doing increments until we achieve the overall number
		   of increments */
		P(counterAccess);
		a = counter;
		if (a < NADDS) {
			counter = counter + NCOUNTERS;
			b = counter;

			/* count the number of increments we perform  for statistics */
			adder_counters[addernumber]++;    

			/* check we are getting sane results */
			if (a + NCOUNTERS != b) {
				kprintf("In thread %ld, %ld + 1 == %ld?\n", 
						addernumber, a, b) ;
			}
		} else {
			flag = 0;
		}
		V(counterAccess);
	}

	/* signal the main thread we have finished and then exit */
	V(finished);

	thread_exit();
}

int maths (void *, unsigned long);
/*
 * math()
 *
 * This function:
 *
 * + Initialises the counter variables
 * + Creates a semaphore to wait for adder threads to complete
 * + Starts the define number of adder threads
 * + waits, prints statistics, cleans up, and exits
 */
int maths (void * data1, unsigned long data2)
{
	int index, error;
	unsigned long int sum;

	/*
	 * Avoid unused variable warnings.
	 */
	(void) data1;
	(void) data2;

	/* create a semaphore to allow main thread to wait on workers */

	finished = sem_create("finished", 0);

	if (finished == NULL) {
		panic("maths: sem create failed");
	}

	/*
	 * **********************************************************************
	 * INSERT ANY INITIALISATION CODE YOU REQUIRE HERE
	 * **********************************************************************
	 */
	/* create a semaphore to count how many counters are available for use */
	counterAccess = sem_create("counterAccess", NCOUNTERS);

	if (counterAccess == NULL) {
		panic("maths: sem create failed");
	}

	/*
	 * Start NADDERS adder() threads.
	 */

	kprintf("Starting %d adder threads\n", NADDERS);

	for (index = 0; index < NADDERS; index++) {

		error = thread_fork("adder thread", NULL,
				&adder, NULL, index);

		/*
		 * panic() on error.
		 */

		if (error) {
			panic("adder: thread_fork failed: %s\n", strerror(error));
		}
	}


	/* Wait until the adder threads complete */

	for (index = 0; index < NADDERS; index++) {
		P(finished);
	}

	kprintf("Adder threads performed %ld adds\n", counter);

	/* Print out some statistics */
	sum = 0;
	for (index = 0; index < NADDERS; index++) {
		sum += adder_counters[index];
		kprintf("Adder %d performed %ld increments.\n", 
				index, adder_counters[index]);
	}
	kprintf("The adders performed %ld increments overall\n", sum);

	/*
	 * **********************************************************************
	 * INSERT ANY CLEANUP CODE YOU REQUIRE HERE 
	 * **********************************************************************
	 */
	/* clean up the semaphore we allocated earlier */
	sem_destroy(counterAccess);

	/* clean up the semaphore we allocated earlier */
	sem_destroy(finished);
	return 0;
}

