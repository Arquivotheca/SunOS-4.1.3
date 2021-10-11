/*      @(#)hllcputils.c 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * hllcputils.c - contains routines which should be invarient from application  
 *		to application. The routines contained in this file are:
 *
 *		LLCP_SEND_CMD - macro to send a command to the controller
 *		llcp_get_pkt - polls for a data packet
 *		llcp_send_pkt - sends a data packet
 *		strt_init - initiates llcp initialization 
 *		send_info - sends host info to controller
 *		get_info -  sends controller info to host
 *		llcp_error - gets error string and displays to console
 *		llcp_reset - decides to call reset_cmd or power_on_rst 
 *		reset_cmd - attempts to s/w reset the cntrlr board
 *		pre_err_chk - checks llcp prior to issueing command
 *		post_err_chk - checks llcp after issueing command
 *		add_llc_hdr - adds llc snap hdr for ISO compatibility
 *		strip_llc_hdr - strips llcp snap hdr from packet
 *		llcp_get_status - gets a copy of the current register status
 *					
 *		initial - PAS - 3/16/88
 */


#ifdef PROMCODE
#include "../h/types.h" 
#include "../dev/llcp.h"
#include "../h/sunromvec.h"
#include "../h/globram.h"
#include "../h/socket.h"
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include "../dev/if_llc.h"
#else PROMCODE
#include <sys/types.h>
#include <sunif/llcp.h>
#include <sys/mbuf.h>
#include <sys/uio.h>
#include <sys/map.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include "../sunif/if_llc.h" 
#if  defined(STANDALONE)
#include <mon/sunromvec.h>
#endif
#endif PROMCODE


#ifdef KERNEL
#define UNIXDVR 1
#endif



extern char *llcp_msgs[];

/*
 * LLCP_SEND_CMD - the order of the operations is critical for the
 *		handshake.  The command register being written
 *		to a non-zero value initiates controller action.
 *		The was changed to a macro to save space for
 *		cpu host prom.
 */

#define LLCP_SEND_CMD(regp,a,b,c)\
regp->len = a;\
regp->addr = b;\
regp->cmd = c;			/* issue command */



/*
 * llcp_get_pkt - polls for a packet of data for a maximum time as specified
 *	in the controller info structure. Returns 0 for no packet
 *	or an error, otherwise returns the length of the packet
 *	Upon command completion if an error occurs, prints a message
 *	to the console.
 */

int
llcp_get_pkt(llcptr, buf)
llcp_info_t *llcptr;
char *buf;
{
	int length = 0;
	llcp_reg_t treg;
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait(), strip_llc_hdr();


	/*
	 * check for error conditions prior to issueing command
	 */
#ifndef PROMCODE
	if ( pre_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL ) 
		return(0);		/* return packet of length 0 */
#endif

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, 0L, (u_char *)0, LLCP_GET_PKT);	

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_RDY);	/* wait up to timeout */

	/*
	 * check for errors on command completion 
	 */
	if ( post_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL )
		return(0);		/* return packet of length 0 */

	/*
	 * command completed successfully 
	 * so transfer data to buffer
	 */
	llcp_get_status(llcptr, &treg);	/* get register values */
	if ( ((length = (int) treg.len) > 0) && (length <= LLCP_BUF_SIZ) ) {  
		strip_llc_hdr((char *)llcptr->cinfop->buf, (char*)buf, &length);
		return(length);
	}
#endif

	return(0);			/* return packet of length 0 */
} /* llcp_get_pkt */



/*
 * llcp_send_pkt - transmits a packet of data. Waits for a maximum time as 
 *		specified in the controller info structure. Returns LLCP_FAIL 
 *		for error and LLCP_SUCC otherwise. If an error occurs, prints a 
 *		msg to the console. 
 */

int
llcp_send_pkt(llcptr, buf, length)
llcp_info_t *llcptr;
char *buf;
int length;
{
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait(), add_llc_hdr();

	/*
	 * check for error conditions prior to issueing command
	 */
#ifndef PROMCODE
	if ( pre_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL ) 
		return(LLCP_FAIL);	
#endif

	/* add iso llc header and adjust len */
	add_llc_hdr(buf, (char *)llcptr->cinfop->buf, &length); 	

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, length, (u_char *)0, LLCP_SEND_PKT);	

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_RDY);	/* wait up to timeout */

	/*
	 * check for errors on command completion 
	 */
	if ( post_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL )
		return(LLCP_FAIL);
#endif

	return(LLCP_SUCC);		/* command completed successfully */
} /* llcp_send_pkt */



/*
 * strt_init -  Initiates llcp initialization
 *		Returns either LLCP_SUCC or LLCP_FAIL.
 */

strt_init(llcptr)
llcp_info_t *llcptr;
{
	llcp_reg_t treg;
	register llcp_reg_t *regptr = llcptr->regp;
	llcp_cntrlr_info_t *cinfoptr = llcptr->cinfop;
	void cmd_wait();


#ifndef PROMCODE
	if ( pre_err_chk(llcptr, LLCP_INIT1) == LLCP_FAIL ) 
		return(LLCP_FAIL);
#endif

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, 0L, 
		((cinfoptr->memtyp == DMA_MEM)? cinfoptr->buf: (u_char *)0),
								LLCP_STRT_INIT);

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_INIT2);						/* wait up to timeout */

	if ( post_err_chk(llcptr, LLCP_INIT2) == LLCP_FAIL ) 
		return(LLCP_FAIL);

	/*
	 * save address of shared memory buffer and check if host
	 * has dma interface and controller expects shared mem or 
	 * vice versa.
	 */
	llcp_get_status(llcptr, &treg);		/* get register values*/

	if ( cinfoptr->memtyp == SHARED_MEM ) {	/* host has shared mem int*/
		if ( treg.addr == (u_char *)0 ) { 
			printf(llcp_msgs[E_LLCP_DMA]);
			return(LLCP_FAIL);
		}
		else 				/* save address of shared mem */
			cinfoptr->phys_addr = treg.addr;
	} else {										/* host has dma interface */
		if ( treg.addr != (u_char *)0 ) {
			printf(llcp_msgs[E_LLCP_DMA]);
			return(LLCP_FAIL);
		}
	}
#endif

	return(LLCP_SUCC);
} /* strt_init */



/*
 * get_info -  sends controller info to host
 *		Returns either LLCP_SUCC or LLCP_FAIL.
 */

get_info(llcptr)
llcp_info_t *llcptr;
{
	llcp_reg_t treg;
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait();


#ifndef PROMCODE
	if ( pre_err_chk(llcptr, LLCP_INIT2) == LLCP_FAIL ) 
		return(LLCP_FAIL);
#endif

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, 0L, (u_char *)0, LLCP_GET_INFO);	

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_INIT3);		/* wait up to timeout */

	if ( post_err_chk(llcptr, LLCP_INIT3) == LLCP_FAIL ) 
		return(LLCP_FAIL);

	llcp_get_status(llcptr, &treg);		/* get register values */

	/* transfer controller information structure */
	if ( stuff_cinfo(llcptr, (int)treg.len) == LLCP_FAIL )
		return(LLCP_FAIL);

#ifdef DEBUG
	/*
	 * print out entire Cinfo struct
	 */
	printf("---------------- CINFO STRUCTURE -------------------------\n");
	printf("len = %d",llcptr->cinfop->len);
	printf("				llcp_ver = %d\n",llcptr->cinfop->llcp_ver);
	printf("addrtyp = %x",llcptr->cinfop->addrtyp);
	printf("				net_laddr = %s\n",llcptr->cinfop->net_laddr);
	printf("buf = 0x%x",llcptr->cinfop->buf);
	printf("				memtyp = %x\n",llcptr->cinfop->memtyp);
	printf("max_pkt = %d",llcptr->cinfop->max_pkt);
	printf("				timeout = %d\n",llcptr->cinfop->timeout);
	printf("----------------------------------------------------------\n");
#endif DEBUG
#endif

	return(LLCP_SUCC);
} /* get_info */



/*
 * send_info -  sends host info to controller
 *		Returns either LLCP_SUCC or LLCP_FAIL.
 */

send_info(llcptr)
llcp_info_t *llcptr;
{
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait();


#ifndef PROMCODE
	if ( pre_err_chk(llcptr, LLCP_INIT3) == LLCP_FAIL ) 
		return(LLCP_FAIL);
#endif

	/*
	 * set length of structure and send second half of init command
	 */
	bcopy((char *)llcptr->hinfop, (char *)llcptr->cinfop->buf, 
						sizeof(llcp_host_info_t));

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, (long) sizeof(llcp_host_info_t), (u_char *)0, 
							LLCP_SEND_INFO);

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_RDY);		/* wait up to timeout */

	if ( post_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL )
		return(LLCP_FAIL);		
#endif

	return(LLCP_SUCC);
} /* send_info */



/*
 * ena_net -  sends the enable network command
 *		This command is always sent after the init sequence, and
 *		is not optional.
 *		Returns either LLCP_SUCC or LLCP_FAIL.
 */

ena_net(llcptr)
llcp_info_t *llcptr;
{
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait();

#ifndef PROMCODE
	if ( pre_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL ) 
			return(LLCP_FAIL);
#endif

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, 0L, (u_char *)0, LLCP_ENA_NET);	

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_RDY);		/* wait up to timeout */

	if ( post_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL ) 
		return(LLCP_FAIL);
#endif

	return(LLCP_SUCC);
} /* ena_net */




/*
 * goto_hlcp -  sends goto hlcp command to exit llcp.
 *		Returns either LLCP_SUCC or LLCP_FAIL.
 */

#ifndef PROMCODE
goto_hlcp(llcptr)
llcp_info_t *llcptr;
{
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait();

	if ( pre_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL ) 
			return(LLCP_FAIL);

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, 0L, (u_char *)0, LLCP_GOTO_HLCP);	

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_HLCP);		/* wait up to timeout */

	if ( post_err_chk(llcptr, LLCP_HLCP) == LLCP_FAIL ) 
		return(LLCP_FAIL);
#endif

	return(LLCP_SUCC);
} /* goto_hlcp */
#endif




/*
 * gen_cmd -  sends any generic command to the controller
 *		The caller must fully set up the treg contents
 *		before calling this routine, so because of
 *		this extra knowledge needed, it should only be used
 *		with controller specific commands.  Note that this
 *		command does not transfer any data.
 *		Returns either LLCP_SUCC or LLCP_FAIL.
 */

#ifndef PROMCODE
gen_cmd(llcptr,treg)
llcp_info_t *llcptr;
llcp_reg_t *treg;
{
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait();

	if ( pre_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL ) 
			return(LLCP_FAIL);

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, treg->len, treg->addr, treg->cmd);	

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_HLCP);		/* wait up to timeout */

	if ( post_err_chk(llcptr, LLCP_RDY) == LLCP_FAIL ) 
		return(LLCP_FAIL);
#endif

	return(LLCP_SUCC);
} /* gen_cmd */
#endif





/*
 * llcp_error() - performs the llcp handshake to retrieve the error string,
 *		 if one exists, and prints the message out. Checks on error 
 *		 state and command complete. Returns LLCP_SUCC if an error 
 *		 string is found, and LLCP_FAIL otherwise
 */

llcp_error(llcptr)
llcp_info_t *llcptr;
{
	int length;
	llcp_reg_t treg; 
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait();

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, 0L, (u_char *)0, LLCP_ERROR);	

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_ERR);						/* wait up to timeout */

	/* 
	 * Since some of the llcp registers could be broken, do not even
	 * check on command complete. As long as the length is non-zero
	 * we will attempt to print out the contents of the info buffer.
	 */
	llcp_get_status(llcptr, &treg);		/* get register values */
	if ( (length = (int) treg.len) != 0 ) {
		*(llcptr->cinfop->buf + length) = '\0';	/* terminate str */
		/* XXX what if shared memory has not been setup at this point!*/
		printf(llcp_msgs[E_LLCP_EST], llcptr->cinfop->buf);
		return(LLCP_SUCC);
	}

	return(LLCP_FAIL);
#else
	return(LLCP_SUCC);
#endif
} /* llcp_error */



/*
 * llcp_reset - software resets network board. 
 *		Determines if the controller is in an undetermined state from
 *		power on reset. If so, does not write any of the shared memory
 *		registers until a timeout or valid combination of values is
 *		found. If a valid combination of values is found in the llcp
 *		register set, simply issues the reset command.
 *		This isn't very robust to hardware errors during runtime, but
 *		then again, the reset command doesn't do a whole hell of a lot!
 *		If there is an error, a message will be displayed through the 
 *		post_err_chk routine.
 */

llcp_reset(llcptr)
llcp_info_t *llcptr;
{
#ifdef PROMCODE	
	int retval;

	/* check if first time through the code on system power-up */
	if ( first_time != 0 )
		return( reset_cmd(llcptr) );
	else  {
		retval = power_on_rst(llcptr);
		first_time = 1;	/* not the first time thru code anymore */
		return(retval);
	}
#else
	/*
	 *	can only have a power on reset from prom code
	 */
	return( reset_cmd(llcptr) );
#endif

} /* llcp_reset */



/*
 * reset_cmd - software resets the controller board by writing
 *		the reset command to the llcp registers.
 *		Returns LLCP_SUCC on successful completion and 
 *		LLCP_FAIL on failure.
 */

reset_cmd(llcptr)
llcp_info_t *llcptr;
{
	register llcp_reg_t *regptr = llcptr->regp;
	void cmd_wait();

	llcptr->cinfop->timeout = LLCP_T_RESET;	/* set 4s timeout for reset */

	/* issue llcp command - (length, address, command) */
	LLCP_SEND_CMD(regptr, 0L, (u_char *)0, LLCP_RESET);	

#ifdef DEBUG
	printf("rst_cmd: &reg=0x%x  \n", llcptr->regp);
#endif

#ifndef UNIXDVR
	cmd_wait(llcptr, LLCP_INIT1);		/* wait up to timeout */

	if ( post_err_chk(llcptr, LLCP_INIT1) == LLCP_FAIL )
		return(LLCP_FAIL);
#endif

	return(LLCP_SUCC);
} /* reset_cmd */



/*
 * pre_err_chk - checks all the registers and prints out the appropriate 
 *		message if there is an error. This routine is called before
 *		a command to the controller has been issued. If the 
 *		controller has gone into the error state, the error 
 *		routine is invoked. Returns either LLCP_SUCC or LLCP_FAIL
 */

#ifndef PROMCODE
pre_err_chk(llcptr, expected_state)
llcp_info_t *llcptr;
state_t expected_state;
{
	llcp_reg_t treg;

	llcp_get_status(llcptr, &treg);		/* get controller status */

	/* check if in proper state and command complete */
	if ( (treg.state != expected_state) || (treg.cmd != LLCP_CMD_COM) ) {
		printf(llcp_msgs[E_LLCP_RDY]);
		return(LLCP_FAIL);
	}

	return(LLCP_SUCC);
} /* pre_err_chk */
#endif



/*
 * post_err_chk - checks all the registers and prints out the appropriate 
 *		message if there is an error. This routine is called after
 *		a command to the controller has been issued. If the 
 *		controller has gone into the error state, the error routine
 *		is invoked. Returns either LLCP_SUCC or LLCP_FAIL
 */

post_err_chk(llcptr, expected_state)
llcp_info_t *llcptr;
state_t expected_state;
{
	llcp_reg_t treg;

	llcp_get_status(llcptr, &treg);		/* get controller status */

	/* command did not complete, not in proper state, or bad fault code */
	if ( (treg.cmd != LLCP_CMD_COM) || (treg.state != expected_state) ) {
		printf(llcp_msgs[E_LLCP_RDY]);
		return(LLCP_FAIL);
	}
	else if ( treg.fault != LLCP_FC_OK ) {
		if ( treg.fault == LLCP_FC_INV_CMD )
			printf(llcp_msgs[E_LLCP_INV]);
		else if ( (treg.state != LLCP_RDY) && (treg.state != LLCP_INIT2)
						&& (treg.state != LLCP_INIT3) ) 
			/* cannot use common memory */
			printf(llcp_msgs[E_LLCP_RDY]);	
		else {
#ifndef UNIXDVR
			(void) llcp_error(llcptr);  /* print out error string */
#endif
		}
		return(LLCP_FAIL);
	}

	return(LLCP_SUCC);
} /* post_err_chk */



/*
 *	add_llc_hdr - adds an llc header to our frames.
 *
 *	The packet received by this routine originally looks like
 *	the following:
 *
 *    ------------------------------------S------------
 *    | DA  |  SA  |  TYPE  |  DATA                   |
 *    ------------------------------------S------------
 *    
 *	It is converted to adhere to ISO standards, and subsequently
 *	looks like this:
 *
 *    -------------------------------------------S------------
 *    |  DA  | SA | LLC_SNAP_HDR*  |  DATA                   |
 *    -------------------------------------------S------------
 *    
 *    *note that the end of the llc_snap_hdr contains the type
 *    field from the original packet
 *
 */

void
add_llc_hdr(buf,tbufp,len)
char *buf, *tbufp;
int *len;
{
	int addrlen, etherhdrlen;
	struct llc_snap_hdr *snap_hdr;
	short *intp; 

	/* 
	 *	copy over DA and SA
	 */
	addrlen = sizeof(struct ether_addr) * 2;
	etherhdrlen = addrlen + sizeof(short);
	bcopy(buf, (char *)tbufp, addrlen); 	/* DA,SA */

	/* 
	 *	copy over ISO lsap_snap header 
	 *	for ISO compatibility
	 */
	tbufp += addrlen;

	snap_hdr = (struct llc_snap_hdr *)(tbufp);
	snap_hdr->d_lsap = 0xaa;
	snap_hdr->s_lsap = 0xaa;
	snap_hdr->control = CNTL_LLC_UI;  /* un-numbered information packet */
	snap_hdr->org[0] = 0x0;
	snap_hdr->org[1] = 0x0;
	snap_hdr->org[2] = 0x0;
	intp = (short *)(buf + addrlen);
	snap_hdr->type = (u_short)*intp;  /* stuff in type of packet */
	tbufp += LLC_SNAP_HDR_LEN;

	/* 
	 *	copy over rest of data and adjust len
	 */
	bcopy(buf+etherhdrlen, tbufp, *len - etherhdrlen); /* Data */
	*len += LLC_SNAP_HDR_LEN;

} /* add_llc_hdr */



/*
 *	strip_llc_hdr - strips an llc header from frames received.
 *			see above diagram for packet formats.
 */

void
strip_llc_hdr(tbufp,buf,len)
char *tbufp, *buf;
int *len;
{
	int addrlen;

	addrlen = sizeof(struct ether_addr) * 2;

	bcopy((char *)tbufp, buf, addrlen);	/* DA SA */
	tbufp += addrlen;
	/*
	 * note that this second bcopy skews over the llcp_snap_hdr saving
	 * the type field at the end of the llc header
	 */
	*len = *len - LLC_SNAP_HDR_LEN + sizeof(short);
	bcopy((char *)tbufp+(LLC_SNAP_HDR_LEN - sizeof(short)), 
				buf+addrlen, *len - addrlen); /* Data&type */

} /* strip_llc_hdr */



llcp_get_status(llcptr, treg)		/* get controller status */
llcp_info_t *llcptr;
llcp_reg_t *treg;
{

	*treg = *llcptr->regp;

} /* llcp_get_status */

