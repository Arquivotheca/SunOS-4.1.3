/*	@(#)hportllcp.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * hportllcp.c - contains routines that may need to be modified from
 *		system to system. They include:
 *
 *		stuff_cinfo - loads cinfo structure from controller info
 *		cmd_wait - waits for command complete up to timeout
 *		power_on_rst - waits for sane controller on power up
 *		on_timeout - timeout signals this routine
 *
 *		initial - PAS - 3/16/88
 */


#ifdef PROMCODE
#include <strings.h>
#include <sys/types.h>
#include "../dev/llcp.h"
#include "../h/sunromvec.h"
#include "../dev/saio.h"		/* for delay routines */
#include "../h/socket.h"
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#else PROMCODE
#include <strings.h>
#include <sys/types.h>
#include <sunif/llcp.h>
#include <sys/mbuf.h>
#include <sys/uio.h>
#include <sys/map.h>
#if defined(STANDALONE)
#include <mon/sunromvec.h>
#include <stand/saio.h>			/* for delay routines */
#endif
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif PROMCODE


#ifdef KERNEL
#define	UNIXDVR
#endif


#if (defined(PROMCODE) || defined(STANDALONE))
#ifdef OPENPROMS
#define	millitime() prom_gettime()
#else
#define	millitime() (*romp->v_nmiclock)
#endif !OPENPROMS
#endif
#ifdef UNIXAPP
int	Timer_exp;		/* timer expired flag */

#define	TIMER_EXP	1		/* timer expired */
#define	TIMER_NOT_EXP	0		/* timer not expired */
#endif

char *llcp_msgs[] = {
/* E_LLCP_DMA */ "gn: ctlr memory type (dma/shared) incorrect\n",
/* E_LLCP_RDY */ "gn: ctlr not responding\n",
/* E_LLCP_INV */ "gn: bad ctlr cmd\n",
/* E_LLCP_EST */ "gn: %s\n"
};




/*
 * stuff_cinfo - transfer select items to cinfo structure to be
 *		subsequently passed to the host. This routine will
 *		probably be a focal point for future revisions.
 */

stuff_cinfo(llcptr, length)
llcp_info_t *llcptr;
int length;
{
	u_char *tbuf;			/* temp buffer pointer */

	tbuf = llcptr->cinfop->buf;	/* save Cinfo.buf pointer */

	/* get cntlr info */
	bcopy((char *)tbuf, (char *)llcptr->cinfop,
		((length > sizeof (llcp_cntrlr_info_t))?
		sizeof (llcp_cntrlr_info_t) : length));
	llcptr->cinfop->buf = tbuf;	/* restore Cinfo.buf ptr */

	return (LLCP_SUCC);
} /* stuff_cinfo */



/*
 * cmd_wait - simply waits up to the timeout specified by the controller
 *		(through the init stage), checking for command complete
 */

void
cmd_wait(llcptr, state)
llcp_info_t *llcptr;
state_t state;
{
	llcp_reg_t treg;
#ifdef UNIXAPP
	unsigned ualarm();

	Timer_exp = TIMER_NOT_EXP;		/* clear timer expired flag */
	(void) ualarm(llcptr->cinfop->timeout, 0);	/* start timer */

	while (Timer_exp != TIMER_EXP) {	/* wait up to timeout */
		llcp_get_status(llcptr, &treg);
		if ((treg.cmd == LLCP_CMD_COM) && (treg.state == state))
			break;			/* command completed */
		/*
		 * to prevent processor overload only check every 5ms
		 */
		usleep(LLCP_OFFBUS);
	}
#endif
#if (defined(PROMCODE) || defined(STANDALONE))
	register int time;
	register int llcptimeout;

	llcptimeout = llcptr->cinfop->timeout / 1000;	/* convert Us to ms */
	time = millitime() + llcptimeout;

	/* poll for reply */
	while (millitime() <= time) {
		llcp_get_status(llcptr, &treg);
		if ((treg.cmd == LLCP_CMD_COM) && (treg.state == state))
			break;			/* command completed */
		/*
		 * to prevent processor overload only check every 2ms
		 */
		DELAY(LLCP_OFFBUS);
	}
#endif 
} /* cmd_wait */



/*
 * power_on_rst - waits for up to 20 seconds for some time of sane
 *		data on the llcp registers after power up. As
 *		soon as the status is valid, breaks out of the
 *		polled loop. Returns LLCP_FAIL on failure and
 *		LLCP_SUCC otherwise.
 *		Note that this routine will wait the entire time
 *		even if there is a valid bad fault code value, as this
 *		could have been written by the diagnostics.
 */

#ifdef PROMCODE
power_on_rst(llcptr)
llcp_info_t *llcptr;
{
	register int i;
	llcp_reg_t treg;

	/*
	 * wait for up to 20 seconds for LLCP_NOT_RDY state
	 */
	for (i = 0; i < LLCP_RST_TIME; i += LLCP_OFFBUS) {
		llcp_get_status(llcptr, &treg);	/* get register values */
#ifdef DEBUG
		printf("power_on_rst: &reg=%x cmd=0x%x state0x%x fault=0x%x\n",
		    llcptr->regp, treg.cmd, treg.state, treg.fault);
#endif

		if ((treg.cmd == LLCP_CMD_COM) && (treg.fault == LLCP_FC_OK) &&
		    (treg.state == LLCP_INIT1))
			return (LLCP_SUCC);	/* controller ready for init */
		if ((treg.cmd == LLCP_CMD_COM) && (treg.fault == LLCP_FC_OK) &&
		    (treg.state == LLCP_NOT_RDY))
			break;
		/*
		 * to prevent processor overload only check every 5ms
		 */
		DELAY(LLCP_OFFBUS);
	}

	if (i == LLCP_RST_TIME) {
		printf(llcp_msgs[E_LLCP_RDY]);
		return (LLCP_FAIL);
	}


	/*
	 * wait for up to 20 additional seconds for LLCP_INIT1 state
	 * from LLCP_NOT_RDY state.
	 */
	for (i = 0; i < LLCP_RST_TIME; i += LLCP_OFFBUS) {
		llcp_get_status(llcptr, &treg);	/* get register values */
#ifdef DEBUG
		printf("** 2nd **:&reg=0x%x   cmd=%x  state0x%x   fault=0x%x\n",
		    llcptr->regp, treg.cmd, treg.state, treg.fault);
#endif

		if (treg.fault != LLCP_FC_OK) {	/* check for fault code err */
			printf(llcp_msgs[E_LLCP_RDY]);
			return (LLCP_FAIL);
		}

		if ((treg.cmd == LLCP_CMD_COM) && (treg.fault == LLCP_FC_OK) &&
		    (treg.state == LLCP_INIT1))
			return (LLCP_SUCC);	/* controller ready for init */
		/*
		 * to prevent processor overload only check every 5ms
		 */
		DELAY(LLCP_OFFBUS);
	}

	if (i == LLCP_RST_TIME) {
		printf(llcp_msgs[E_LLCP_RDY]);
		return (LLCP_FAIL);
	}

	return (LLCP_SUCC);
} /* power_on_rst */
#endif

#ifdef UNIXAPP
/*
 * on_timeout - simply sets the timer expired flag
 */
on_timeout()
{
	Timer_exp = TIMER_EXP;			/* set timer expired flag */
} /* on_timeout */
#endif 
