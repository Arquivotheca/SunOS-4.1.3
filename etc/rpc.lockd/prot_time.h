/*      @(#)prot_time.h 1.1 92/07/30 SMI                              */

/*
 * This file consists of all timeout definition used by rpc.lockd
 */

#define MAX_LM_TIMEOUT_COUNT	1
#define OLDMSG			30	/* counter to throw away old msg */
/*
#define LM_TIMEOUT_DEFAULT 	15
*/
#define LM_TIMEOUT_DEFAULT	5
#define LM_GRACE_DEFAULT 	3
int 	LM_TIMEOUT;
int 	LM_GRACE;
int	grace_period;
