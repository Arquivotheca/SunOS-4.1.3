
/*
 * Lego definitions, for use with real hardware.
 *
 * A good 'C' compiler optimizer should create code as if the source
 * had used pointers, and constant expression optimization gets rid of
 * the excess junk from read_lego_dfb() and write_lego_dfb().
 * Need to check that the optimizer is well-behaved for memory-mapped
 * references and doesn't assume they can't change in the absence
 * of direct assignment. The Sun-3 optimizer seems ok there.
 */

/* SCCS version info comment
*	static  char sccsid[] = "@(#)lego.h 1.1 92/07/30 SMI";
*/

/* static char	sccsid_lego_h[] = { "lego.h 3.5 88/10/20" }; */
/* static char	sccsid_lego_hal[] = { "@(#)lego.h 1.3 89/12/19" }; */

/*
*	#include "lego-fbc-reg.h"
*	#include "lego-tec-reg.h"
*	#include "lego-fhc-reg.h"
*	#include "lego-thc-reg.h"
*	#include "lego-dac-reg.h"
*/
typedef int	*fbc_addr_t;
typedef int	*tec_addr_t;
typedef int	*fhc_addr_t;
typedef int	*thc_addr_t;
typedef int	*dac_addr_t;

/*
 * Lego access "routines"
 */

fbc_addr_t	map_lego_fbc();
#define	read_lego_fbc(addr)		(*(addr))
#define	write_lego_fbc(addr, datum)	((*(addr)) = (datum))

fhc_addr_t	map_lego_fhc();
#define	read_lego_fhc(addr)		(*(addr))
#define	write_lego_fhc(addr, datum)	((*(addr)) = (datum))

tec_addr_t	map_lego_tec();
#define	read_lego_tec(addr)		(*(addr))
#define	write_lego_tec(addr, datum)	((*(addr)) = (datum))

thc_addr_t	map_lego_thc();
#define	read_lego_thc(addr)		(*(addr))
#define	write_lego_thc(addr, datum)	((*(addr)) = (datum))

dac_addr_t	map_lego_dac();
#define	read_lego_dac(addr)		(*(addr))
#define	write_lego_dac(addr, datum)	((*(addr)) = (datum))

char	*map_lego_dfb();

#define	read_lego_dfb(addr, cnt)  (((cnt) == 1) ? (*(char *)(addr)) : \
				   ((cnt) == 2) ? (*(short int *)(addr)) : \
				   ((cnt) == 4) ? (*(int *)(addr)) : \
				   printf("*** read_lego_dfb: invalid byte count\n") \
				  )

#define	write_lego_dfb(addr, cnt, datum) \
		(((cnt) == 1) ? (*(char *)(addr) = (datum)) : \
		 ((cnt) == 2) ? (*(short int *)(addr) = (datum)) : \
		 ((cnt) == 4) ? (*(int *)(addr) = (datum)) : \
		 printf("*** write_lego_dfb: invalid byte count\n") \
		)

#define	read_lego_dfb_1(addr)		(*(char *)addr)
#define	write_lego_dfb_1(addr, datum)	((*(char *)addr) = (datum))

#define read_lego_dfb_2(addr)		(*(short int *)addr)
#define	write_lego_dfb_2(addr, datum)	((*(short int *)addr) = (datum))

#define	read_lego_dfb_4(addr)		(*(int *)addr)
#define	write_lego_dfb_4(addr, datum)	((*(int *)addr) = (datum))

#define	read_lego_dfb_int(addr)		(*(int *)addr)
#define	write_lego_dfb_int(addr, datum)	((*(int *)addr) = (datum))

/*
 * null routines.
 */

#define start_lego()
#define end_lego()

#define	flushfb_lego_fbc()

#define	delay_lego()
