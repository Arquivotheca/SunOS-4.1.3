static char     sccsid[] = "@(#)mputil.c 1.1 7/30/92 Copyright 1985 Sun Micro"; 
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sun/mem.h>
#include <sys/ioctl.h>
#include "mptest_msg.h"
#include "mptest.h"
#include "sdrtns.h"

#ifndef MIOCSPAM
#define MIOCSPAM _IOWR(M, 2, unsigned int) /* set processor affinity mask */
#define MIOCGPAM _IOWR(M, 3, unsigned int) /* set processor affinity mask */
#endif

extern int errno ;
extern char *sys_errlist[] ;

caddr_t v2p();	/* forward declaration */
unsigned getnextbitmsk();

int	pid ;
char 	*valloc();

int memfd = NULL;

/*
 *      get_cpumask() will return the processor mask in the
 *      system. If for a non-MP system, it return 0.
 *      (at least one bit should be set)
 */
unsigned
get_grandmask()
{
        unsigned mask ;

        if ( (memfd = open("/dev/null",0) ) < 0 )
        {
            send_message(-OPEN_DEV_NULL_ERR, ERROR, open_dev_null_err);
        }
        mask = ~0 ;
        if ( ioctl(memfd, MIOCSPAM, &mask) < 0 ) {
            send_message(-MIOCSPAM_ERR, ERROR, miocspam_err, mask);
            return(0);
        }
        ioctl(memfd, MIOCGPAM, &mask);
        return(mask);
}
 
lock2cpu(mask)
unsigned *mask ;
{
    ioctl(memfd, MIOCSPAM, mask);
}

getcpu(mask)
unsigned *mask ;
{
    ioctl(memfd, MIOCGPAM, mask);
}

count_ones(mask)
unsigned mask ;
{
    int cnt = 0;

    while ( mask ) 
    {
        if ( mask % 2 ) cnt++ ;
        mask /= 2 ;
    }
    return(cnt);
}

/*
 *  getnextbitmsk(base,mask), takes two bit mask. 'Mask' has
 *  one bit set, the same bit is also set in 'base'. 
 *  getnextbitmsk() will search the next higher bit that is
 *  set in base and return the mask with the found bit set.
 *  If mask is zero, getnextbitmsk() return the first
 *  lowest bit that is set(first call)
 *	
 *  Example : base = 19(1011 binary)
 *	      getnextbitmsk(base,0)	returns		1
 *	      getnextbitmsk(base,1) 	returns		2
 *	      getnextbitmsk(base,2) 	returns		16
 *	      getnextbitmsk(base,16) 	returns		1
 *	      getnextbitmsk(base,1) 	returns		2
 *	      getnextbitmsk(base,2) 	returns		16
 *			...
 */

unsigned
getnextbitmsk(base, mask)
unsigned base, mask;
{
    unsigned i, x ;

    if ( mask && !(base & mask)) /* ERROR */
    {
        send_message(0, DEBUG, getnextbitmsk_err);
        return(0);
    }
    do
    {
        if ( mask == 0 || mask > base ) mask = 1 ;
        else mask <<= 1 ;
    }
    while ( !(base & mask) ) ;
    return(mask);
}

/*
 *  rotate(base, current_number) returns a number increment by one
 *  and negates that number if it's an odd number.  This function
 *  rotates the number depending on the base value.
 *      Example:  rotate(5, 0) = -1
 *                rotate(5, 1) = 2
 *                rotate(5, 2) = -3
 *                rotate(5, 3) = 4
 *                rotate(5, 4) = 0
 */
rotate(base, current_number)
int base, current_number;
{
    if ( abs(current_number) < base - 1 )
    {
        if ( current_number >= 0 ) 
        {
            current_number++; 
	    return(-current_number);
	} 
        else 
	{ 
 	    current_number = -current_number; 
	    current_number++; 
            return(current_number); 
        }
    }
    else return(0);
}


