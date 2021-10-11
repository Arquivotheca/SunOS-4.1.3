/*	@(#)fault.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#ifndef _sun_fault_h
#define	_sun_fault_h

/*
 * Where to go on fault in kernel mode
 * Zero means fault was unexpected
 */
label_t	*nofault;	/* longjmp vector */

/*
 * Additional stuff for handling expected async faults;
 * used to communicate between poke*() and asyncerror()/memerr().
 *  0 means fault unexpected;
 * -1 means fault expected;
 *  1 means fault occured.
 */
int	pokefault;

#if defined(sun4c) || defined(sun4m) || defined(sun4d)
void flush_writebuffers_to(/* addr_t v */);
void flush_all_writebuffers();
void flush_poke_writebuffers();

#else /* sun4c || sun4m || sun4d */
/* Not a machine with write buffers */
#define flush_writebuffers_to(v)
#define flush_all_writebuffers()
#define flush_poke_writebuffers();

#endif /* sun4c || sun4m || sun4d */


#endif /*!_sun_fault_h*/
