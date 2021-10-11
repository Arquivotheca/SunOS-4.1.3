#ifndef lint
static        char sccsid[] = "@(#)if_gn.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * if_gn.c
 *
 * Sun Generic Network Controller Interface
 *
 *	This generic network standalone driver adheres to the
 *	llcp specification, and includes the additional llcp
 *	files:
 *			hllcputils.c
 *			hportllcp.c
 */


#ifdef PROMCODE 
#include <strings.h>
#include <signal.h> 
#include "../h/types.h"
#include "../h/sunromvec.h"
#include "../h/cpu.map.h"
#include "../h/eeprom.h"
#include "../dev/llcp.h"
#include "../dev/saio.h"
#include "../dev/param.h"
#include "../h/socket.h"
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include "../h/globram.h" 
#else
#include <strings.h>
#include <sys/signal.h> 
#include <sys/types.h>
#include <mon/sunromvec.h>
#include <sunif/llcp.h>
#include <stand/saio.h>
#include <stand/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <mon/cpu.map.h>
#include <mon/eeprom.h>
#endif PROMCODE


#ifndef PROMCODE
struct ether_addr net_mac_addr;			/* our net address */
struct ether_addr *gn_mac_addr = &net_mac_addr;
#endif

struct gn_softc {
	char		es_scrat[PROTOSCRATCH];	/* work space for nd */
	llcp_info_t es_llcp;			/* LLCP information structure */
	llcp_cntrlr_info_t cinfo;		/* controller info on host */
	llcp_host_info_t hinfo;			/* host info on controller */
	char *dataaddr;				/* addr of I/O or dma space */
};

		/* device information */
	/*
	 * Note that these addresses are hard-coded here
	 * to facilitate bringup in the case the the
	 * eeprom was incorrectly set-up or was not
	 * initialized at all.  These addresses are
	 * unique to the first generation fddi controller
	 * and esthetically do not belong here.
	 * (ie. It hurt, but it makes everyone's life
	 *      easier.)
	 * Note: these variables are externs in the file gn_inf.c
	 */
u_long gnaddrs[] = { 0x16c0020, 0x18c0020, 0x1ac0020, 0x1cc0020 };

#ifndef PROMCODE
u_char gnmemtype[] = { ID_VME, ID_VME, ID_VME, ID_VME };
#define NGNADDR		(sizeof(gnaddrs) / sizeof(gnaddrs[0]))
int gnctlrnum;

struct devinfo gninfo = {
	sizeof (llcp_reg_t),		/* size of io space */
	0,				/* size of dma space */
	sizeof(struct gn_softc),	/* size of local bytes */
	NGNADDR,			/* # of standard addresses */
	gnaddrs,			/* vector of standard addrs */
	MAP_VME32A32D,			/* map space 32 bit VME bus I/O */
	0,				/* transfer size handled by ND */
};
#endif PROMCODE
					/* external interfaces */
int gnopen(), gnclose();
extern int xxprobe();
extern char *resalloc();
#ifdef PROMCODE
int nullsys();
extern int tftpboot();
#else PROMCODE
extern int xxboot(), etherstrategy(), gnstats();
#endif


struct boottab gndriver = {
#ifdef PROMCODE
	"gn", xxprobe, tftpboot, gnopen, gnclose, nullsys, 
#else PROMCODE
	"gn", xxprobe, xxboot, gnopen, gnclose, etherstrategy, 
#endif
	"gn: Sun Generic Network", &gninfo,
};
		/* ethernet routines */
int	gnxmit(), gnpoll(), gnreset(), my_mac_addr();
struct saif gnif = {
	gnxmit,
	gnpoll,
	gnreset,
	my_mac_addr
#ifndef PROMCODE
	,
	gnstats
#endif PROMCODE
};


/*
 * Description: Open Generic Network nd connection
 * Synopsis:	status = gnopen(sip)
 *		status	:(int)     = pass value from etheropen
 *				-1 = error
 *		sip	:(char *) point to saio structure
 *
 * Routines:	gninit, etheropen, gnclose
 */
int
gnopen(sip)
struct saioreq *sip;
{
	int result;

	sip->si_sif = &gnif;		/* set interface pointers */

	if ( gninit(sip) || (result = etheropen(sip)) < 0 ) {
		printf("gn: open failed\n");
		gnclose(sip);		/* must close interface */
		return (-1);
	}
	return (result);
} /* gnopen */




/*
 * Description: This routine sets up dma or I/O space and goes through
 *		llcp init sequence.
 *
 * Synopsis:	status = gninit(sip) 
 *		status	:(int)  0 = complete
 *				1 = error
 *		sip	:(char *) point to saio structure
 * Routines:	gnreset
 */
int
gninit(sip)
struct saioreq *sip;
{
	register struct gn_softc *es;
	llcp_cntrlr_info_t *cinfoptr;
	register int retry = 5;			 /* up to 5 retries */
#ifdef PROMCODE
	int gnctlrnum = gp->g_ctlr_number;
#else PROMCODE
	/* for broken code that sets si_ctlr from controller
	 * number to an address behind the scenes
	 */
	for (gnctlrnum=0; gnctlrnum < NGNADDR; gnctlrnum++)
		if (gnaddrs[gnctlrnum] == sip->si_ctlr)
			break;
#endif PROMCODE

	es = (struct gn_softc *)sip->si_devdata; /* pt to local space */
#ifdef DEBUG
	printf("Entering gninit sip = 0x%x   es = 0x%x\n",sip,es);
#endif

	/*
	 * stuff es_softc structure
	 */
	es->es_llcp.cinfop = cinfoptr = &es->cinfo;  /* this setup needed for */
	es->es_llcp.hinfop = &es->hinfo;	     /* generic llcp routines */
	/* base address of llcp registers */
	es->es_llcp.regp = (llcp_reg_t *)sip->si_devaddr; 

	if ( gnmemtype[gnctlrnum] == ID_VME )
		cinfoptr->memtyp = SHARED_MEM;		
	else
		cinfoptr->memtyp = DMA_MEM;		

	if ( cinfoptr->memtyp == DMA_MEM ) 	/* if dma, alloc space */
	    cinfoptr->buf = (u_char *)resalloc(RES_DMAMEM, LLCP_BUF_SIZ); 
	else											/* clr addr of llcp buf */
	    cinfoptr->buf = (u_char *)0; 

	cinfoptr->timeout = LLCP_T_RESET;	/* initial cmd timeout val*/

	if( init_host(sip) == LLCP_FAIL )	/* initialize hinfo struct*/
		return(0);

	/* 
	 * retries for simply initiating communication
	 * with the network controller
	 */
	while( --retry ) {								

		if ( gnreset(es) == 0 ) 	/* llcp reset and init */
			return(0);		/* return success */

		if ( (cinfoptr->memtyp == SHARED_MEM) )  {
			/*
			 * Need to release memory here berfore we try 
			 * again but since we are the only ones running,
			 * there is no routine available to release alloc'd
			 * space. If there are a ton of resets, we might
			 * run out of space.   
			 */
			cinfoptr->buf = (u_char *)0;	/* rezero buf */
			es->es_llcp.hinfop->buf = (u_char *)0;
		}
	}

	return(1);				/* return failure */
} /* gninit */




/*
 * Description:	Basic Generic Network llcp initialization
 *		Goes through the llcp initialization sequence. 
 *
 * Synopsis:	status = gnreset(es)
 *		status	:(int)  0 = complete
 *				1 = error
 *		es	:(char *) pointer to ethernet structure
 */
int
gnreset(es)
	register struct gn_softc *es;
{
	char *physaddr;
	llcp_cntrlr_info_t *cinfoptr = es->es_llcp.cinfop;
	llcp_host_info_t *hinfoptr = es->es_llcp.hinfop;
#ifdef PROMCODE
    int gnctlrnum = gp->g_ctlr_number;
#endif PROMCODE
	char *devalloc();

#ifdef DEBUG
	printf("Entering gnreset es = 0x%x\n",es);
#endif

	if ( llcp_reset(&es->es_llcp) == LLCP_FAIL ) 
		return(1);							/* return error */

	if ( strt_init(&es->es_llcp) == LLCP_FAIL )
		return(1);							/* return error */


	/*
	 * If we have a shared memory interface, then use the physical address
	 * returned by the controller to map a virtual address to. 
	 */
	if ( (cinfoptr->memtyp == SHARED_MEM) ) {
		/*
		 * to get the actual physical address of the buffer area, 
		 * need to or in the base address of the llcp registers
		 * without any offset into the page.
		 */
		physaddr = (char *)(gnaddrs[gnctlrnum] + cinfoptr->phys_addr);
		cinfoptr->buf = (u_char *)devalloc(MAP_VME32A32D, 
							physaddr, LLCP_BUF_SIZ);
		hinfoptr->buf = cinfoptr->buf;
	}


#ifdef DEBUG
	printf("after strt_init: phys_addr=0x%x   mapped addr = 0x%x\n",
					cinfoptr->phys_addr,cinfoptr->buf);
#endif
	if ( get_info(&es->es_llcp) == LLCP_FAIL ) 
		return(1);		/* return error */

	/* check if user specified address to be provided by controller */
	if (hinfoptr->partition & 0x20) {
		/* save the MAC address from the controller */
		bcopy((char *)cinfoptr->net_laddr,(char *)gn_mac_addr, 
						sizeof(struct ether_addr));
		bcopy((char *)cinfoptr->net_laddr, (char *)hinfoptr->net_laddr, 
						sizeof(struct ether_addr));	
	}
	else {		/* else use host MAC address */
		myetheraddr((struct ether_addr *)gn_mac_addr);	
		bcopy((char *)gn_mac_addr, (char *)cinfoptr->net_laddr, 
						sizeof(struct ether_addr));	
		bcopy((char *)gn_mac_addr, 
					(char *)es->es_llcp.hinfop->net_laddr, 
					sizeof(struct ether_addr));	
	}
		 
	if ( send_info(&es->es_llcp) == LLCP_FAIL ) 
		return(1);		/* return error */

	if ( ena_net(&es->es_llcp) == LLCP_FAIL ) 
		return(1);		/* return error */

#ifdef DEBUG
	printf("GNRESET SUCCEEDED !! \n");
#endif
	return(0);			/* reset succeeded */
} /* gnreset */




/*
 * Description:	
 *	Transmits a packet of data. Waits for a maximum time as 
 *	specified in the controller info structure. Returns 1 for 
 *	error and 0 otherwise. If an error occurs, prints a msg 
 *	to the console. 
 *
 * Synopsis:	status = gnxmit(es, buf, count)
 *		es	:(char *) pointer to ethernet structure
 *		buf	:(char *) pointer to buffer
 *		count	:(int)	character count
 * Routines:	iesimple, bzero, bcopy
 */
gnxmit(es, buf, count)
register struct gn_softc *es;
char *buf;
int count;
{

	if ( llcp_send_pkt(&es->es_llcp, buf, count) == LLCP_FAIL ) {
		printf("gn: xmit failed\n");
		return(1);		/* return error */
	}

	return(0);			/* command completed successfully */
} /* gnxmit */




/*
 * Description: 
 * 	polls for a packet of data for a maximum time as specified
 *	in the controller info structure. Returns 0 for no packet
 *	or an error, otherwise returns the length of the packet
 *	Upon command completion if an error occurs, prints a message
 *	to the console.
 *
 * Synopsis:	status = lepoll(es, buf)
 * 		status	:(int) 0 = no packet yet
 *			      >0 = length of received packet
 *		es	:(char *) pointer to ethernet structure
 *		buf	:(char *) pointer of receiving buffer
 * Routines:	
 */
int
gnpoll(es, buf)
register struct gn_softc *es;
char *buf;
{

	return( llcp_get_pkt(&es->es_llcp, buf) );
} /*  gnpoll */




/*
 * Description: Close down Lance ethernet device

 * Synopsis:	status = gnclose(sip)
 *		status	:(void)
 *		sip	:(char *) pointer to saioreq structure
 */
gnclose(sip)
struct saioreq *sip;
{
	if ( llcp_reset(&((struct gn_softc *)(sip->si_devdata))->es_llcp) ==
								LLCP_FAIL ){
		printf("gn: close failed\n");
		return(1);		/* return error */
	}

	return(0);			/* return successful */
} /* gnclose */



/*
 * init_host - XXX - sets up hinfo structures. This routine will 
 *		probably be a focal point for future revisions to llcp.
 */

init_host(sip)
struct saioreq *sip;
{
#ifdef PROMCODE
    int gnctlrnum = gp->g_ctlr_number;
#endif PROMCODE
	register struct gn_softc *es = (struct gn_softc *)sip->si_devdata;
	llcp_host_info_t *hinfoptr = es->es_llcp.hinfop;

	hinfoptr->len = sizeof(llcp_host_info_t);	/* len of structure */
	hinfoptr->llcp_ver = LLCP_VERSION;		/* version of llcp */

	hinfoptr->addrtyp = LONG_ADDR;			/* type of net addr */
	/* MAC address initialized during getinfo handshake */

	hinfoptr->memtyp = es->es_llcp.cinfop->memtyp;	/* type of memory */
	hinfoptr->buf = es->es_llcp.cinfop->buf; 
/*	hinfoptr->prom_rev = *romp->op_mon_id;		/* host prom revision */
	hinfoptr->prom_rev = 1;				/* host prom revision */
	hinfoptr->ctlr = gnctlrnum;			/* cntrlr num */
	hinfoptr->unit = sip->si_unit;			/* Unit num in cntrlr */
	hinfoptr->partition = (long) sip->si_boff; 	/* Partition num */

	return(LLCP_SUCC);
} /* init_host */



/*
 * my_mac_addr - stuffs the current MAC address into the network address
 *		pointer passed.
 */

my_mac_addr(ea)
struct ether_addr *ea;
{
	bcopy((char *)gn_mac_addr, (char *)ea, sizeof(struct ether_addr));	
} /* my_mac_addr */


#ifndef PROMCODE
gnstats()
{
	/* XXX implement sometime */
}
#endif PROMCODE
