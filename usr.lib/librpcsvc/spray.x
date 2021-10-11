/* @(#)spray.x 1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * Spray a server with packets
 * Useful for testing flakiness of network interfaces
 */

const SPRAYMAX = 8845;	/* max amount can spray */

/*
 * GMT since 0:00, 1 January 1970
 */
struct spraytimeval {
	unsigned int sec;
	unsigned int usec;
};

/*
 * spray statistics
 */
struct spraycumul {
	unsigned int counter;
	spraytimeval clock;
};

/*
 * spray data
 */
typedef opaque sprayarr<SPRAYMAX>;

program SPRAYPROG {
	version SPRAYVERS {
		/*
		 * Just throw away the data and increment the counter
		 * This call never returns, so the client should always 
		 * time it out.
		 */
		void
		SPRAYPROC_SPRAY(sprayarr) = 1;

		/*
		 * Get the value of the counter and elapsed time  since
		 * last CLEAR.
		 */
		spraycumul	
		SPRAYPROC_GET(void) = 2;

		/*
		 * Clear the counter and reset the elapsed time
		 */
		void
		SPRAYPROC_CLEAR(void) = 3;
	} = 1;
} = 100012;
