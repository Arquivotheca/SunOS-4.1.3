/*	@(#)gmon.h 1.1 92/07/30 SMI	*/

struct phdr {
    char	*lpc;
    char	*hpc;
    int		ncnt;
};

    /*
     *	histogram counters are unsigned shorts (according to the kernel).
     */
#define	HISTCOUNTER	unsigned short

    /*
     *	fraction of text space to allocate for histogram counters
     *	here, 1/2
     */
#define	HISTFRACTION	2

    /*
     *	Fraction of text space to allocate for from hash buckets.
     *	The value of HASHFRACTION is based on the minimum number of bytes
     *	of separation between two subroutine call points in the object code.
     *	Given MIN_SUBR_SEPARATION bytes of separation the value of
     *	HASHFRACTION is calculated as:
     *
     *		HASHFRACTION = MIN_SUBR_SEPARATION / (2 * sizeof(short) - 1);
     *
     *	For the VAX, the shortest two call sequence is:
     *
     *		calls	$0,(r0)
     *		calls	$0,(r0)
     *
     *	which is separated by only three bytes, thus HASHFRACTION is 
     *	calculated as:
     *
     *		HASHFRACTION = 3 / (2 * 2 - 1) = 1
     *
     *	Note that the division above rounds down, thus if MIN_SUBR_FRACTION
     *	is less than three, this algorithm will not work!
     *
     *	On a 680x0, the shortest two-call sequence is:
     *
     *		jsr	a0@
     *		jsr	a0@
     *
     *	which is separated by only two bytes! My interpretation of the
     *	correct value for HASHFRACTION is:
     *	
     *		HASHFRACTION = floor( MIN_SUBR_SEPARATION / sizeof(short) )
     *	or
     *		HASHFRACTION = 2 div 2 
     *	so I get the same results by different means!
     *
     *  On a SPARC, the shortest two call sequence is:
     *
     *		call	func1
     *		<delay slot instruction>
     *		call	func2
     *
     *  which is separated by 8 bytes.  So HASHFRACTION is:
     *
     *		HASHFRACTION = 8/((2*2)-1) = 2
     *
     *
     *
     *
     *
     *
     */
#ifdef sparc
#define	HASHFRACTION	2
#else
#define	HASHFRACTION	1
#endif sparc

    /*
     *	percent of text space to allocate for tostructs
     *	with a minimum.
     */
#define ARCDENSITY	2
#define MINARCS		50

struct tostruct {
    char		*selfpc;
    long		count;
    unsigned short	link;
};

    /*
     *	a raw arc,
     *	    with pointers to the calling site and the called site
     *	    and a count.
     */
struct rawarc {
    unsigned long	raw_frompc;
    unsigned long	raw_selfpc;
    long		raw_count;
};

    /*
     *	general rounding functions.
     */
#define ROUNDDOWN(x,y)	(((x)/(y))*(y))
#define ROUNDUP(x,y)	((((x)+(y)-1)/(y))*(y))

#define GMON_OUT	"gmon.out"
