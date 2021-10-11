static char sccsid[] = "@(#)entest.c	1.1 7/30/92 17:35:48, Copyright Sun Microsystems";

/*****************************************************************************

  Date:		Mon Sep 25 10:58:39 1989
  File:		entest.c
  Author:	Anantha Srirama
  Description:  This file contains the functions and the main routine which
  		constitute the ENTest.  Refer to the header of each function
		for detailed description on their operation (I am not kidding
		this time, I have actually completed the headers not just
		inserting a header template!!)

  Copyright (c) 1989 Sun Microsystems, Inc.

*****************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <sgtty.h>

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stropts.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/nit.h>
#include <net/nit_if.h>
#include <net/nit_pf.h>
#include <net/packetfilt.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>


/* 
 * FDDI specific includes
 */
#include "fddi_types.h"	/* change to <sunfw/fddi/fddi_types.h> when ready */
#include "commands.h"	/* change to <sunfw/fddi/commands.h> when ready   */


/* 
 * Test specific includes
 */
#include "entest.h"


/* 
 * Sundiag specific includes
 */
#include	<signal.h>
/*#include	"entest_msgs.h"*/
#include	"sdrtns.h"
#include	"../../../lib/include/libonline.h"
 

/* 
 * Broadcast, Our, Upstream and Downstream neighbor adresses
 */
mac_addr_t broadcast = {			  /* Broadcast  */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
mac_addr_t una;					  /* Upstream   */
mac_addr_t dna;					  /* Downstream */
mac_addr_t our_addr;				  /* Our        */


/*
 * File descriptor for the open device /dev/nit
 */
int	fd;


/*
 * SIF Configuration Request frame structure
 */
smthdr_t	sif;


/*
 * This is a global variable which has a non-zero value if the FW has been
 * reconfigured.
 */
u_long	fw_reconfigured = !RECONFIGURED;
u_char	recv_data[BUFSIZ], recv_cntl[BUFSIZ];


/*****************************************************************************

  probe_fddi ()

  This function verifies that the FDDI is indeed available in the system.
  This function is called if the test is being executed from the command line
  instead of the SunDiag.  This is so because SunDiag will not call the test
  if the FDDI is not available.  We verify the existance of the FDDI as
  follows

	- Open a socket
	- perform a 'ioctl' with fddi0 as the interface name and request
	  status

  If the interface is not present 'ioctl' returns a -ve value.

  This function will not return in case there is no FDDI.

*****************************************************************************/
void fddi_probe ()
{
    int	sock;
    struct fddi_status_struct fsb;
    struct ifreq ifr;
    
    func_name = "fddi_probe";
    TRACE_IN
    if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
	send_message (-NO_SOCKET, FATAL, "%s could not open socket: %s",
		      test_name, errmsg (errno));
    }

    /* 
     * Initialize the 'ifr' structure with proper values and call 'ioctl'
     * function to find out if we have a FDDI interface in the system
     */

    strncpy (ifr.ifr_name, DEVICE_NAME, sizeof (ifr.ifr_name));
    ifr.ifr_fddi_stat.stat_size = sizeof (fsb);
    ifr.ifr_fddi_stat.fddi_stats = (caddr_t) &fsb;
    if (ioctl (sock, SIOCGETFDSTAT, (caddr_t) &ifr) < 0) 
    {
	send_message (-NO_FDDI_IF, FATAL, "%s no fddi interface: %s",
		      test_name, errmsg (errno));
    }
    close (sock);
    TRACE_OUT
}


/*****************************************************************************

  fddi_fw_setup (fw_agent)

  This function sets up the FDDI firmware so that it passes up all the SMT
  frames to the host.  In the default mode of the firmware, these frames are
  handled by the firmware.  We need to access all the SMT frames because we use
  SMT frames to perform the tests.  Before exiting the test the FDDI firmware
  MUST be reprogrammed to its default state.

  fw_agent = DISABLE ====> program the FDDI firmware to pass up all the SMT
  			   frames except NIF, SIF: config and operation

  fw_agent = ENABLE =====> default operation of FDDI, all SMT frames handled
  			   by firmware.  Host sees no SMT packets.

  The reprogramming of the FDDI firmware switches are done by the use of
  'ioctl' calls.

  Note: Since this function is called from within clean_up it exits directly

*****************************************************************************/
void fddi_fw_setup (fw_agent)
int fw_agent;
{
    int sock;
    struct fddi_smt_config fsc;
    struct ifreq ifr;
    
    func_name = "fddi_fw_setup";
    TRACE_IN
    strcpy (ifr.ifr_name, device_name);
    ifr.ifr_fddi_gstruct.ifru_fddi_gioctl = FDDI_SMT_SWITCHES;
    ifr.ifr_fddi_gstruct.ifru_fddi_gaddr = (caddr_t) &fsc;
    if (fw_agent == ENABLE) 
    {
	send_message (0, VERBOSE, "%s Enabling FW", test_name);
    }
    else 
    {
	send_message (0, VERBOSE, "%s Disabling FW", test_name);
    }
    fsc.resp_frame_sw = fw_agent;
    fsc.nif_frame_sw = ENABLE;			  /* enabled all the time */
    fsc.disable_fw_agent = ENABLE;		  /* enabled all the time */
    if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
	send_message (-NO_SOCKET, FATAL, "%s could not open socket: %s",
		      test_name, errmsg (errno));
    }
    if (ioctl (sock, SIOCFDGIOCTL, (caddr_t) &ifr) < 0) 
    {
	send_message (-RECONFIG_ERROR, FATAL, "%s reconfig failed: %s",
		      test_name, errmsg (errno));
	send_message (-RECONFIG_ERROR, FATAL, "%s REMEMBER TO RECONFIG THE FIRMWARE",
		      test_name);
    }
    close (sock);
    TRACE_OUT
}


/*****************************************************************************

  nit_open ()

  This function opens the device '/dev/nit' and then binds it to the FDDI
  after pushing a packet filter on to it as follows:

  	- opens the device '/dev/nit'
	- puts the device in message-discard mode, refer to streamio(4) for
	  more info on this mode.
	- pushes a filter packet onto the stream effectively filtering the
	  unnecessary packets.
	- binds this device to FDDI, this binding is done after pushing the
	  filter so as to prevent us being flooded with unwanted packets.
	- flush the readside of the stream

  This function returns the file descriptor pointing to the '/dev/nit'.

*****************************************************************************/
int	nit_open ()
{
    int			nit_fddi;
    u_short*		fwp;
    struct packetfilt	pf;
    struct ifreq	ifr;
    machdr_t		machdr;
    struct strioctl	si;
    
    func_name = "nit_open";
    TRACE_IN
    if ((nit_fddi = open ("/dev/nit", (O_RDWR | O_NDELAY))) < 0) 
    {
	send_message (-NIT_OPEN_FAIL, FATAL, "%s unable to open /dev/nit: %s",
		      test_name, errmsg (errno));
    }
    send_message (0, VERBOSE, "%s /dev/nit open succeded", test_name);
    
    /* 
     * Set up the stream to be in 'message-discard' mode.  For more
     * description of this mode refer to streamio(4) and read (2v) of the man
     * pages
     */

    if (ioctl (nit_fddi, I_SRDOPT, (char*) RMSGD) < 0) 
    {
	send_message (-NIT_RMSGD_FAIL, FATAL,
		      "%s unable to set message-discard mode: %s",
		      test_name, errmsg (errno));
    }
    send_message (0, VERBOSE, "%s settting of message-discard succeceded",
		  test_name);

    /* 
     * Push the packet filtering mechanism
     */

    if (ioctl (nit_fddi, I_PUSH, "pf") < 0) 
    {
	send_message (-FILT_PUSH_FAIL, FATAL, "%s filter push failed: %s",
		      test_name, errmsg (errno));
    }
    send_message (0, VERBOSE, "%s Packet Filter PUSH succeceded", test_name);


    /* 
     * Set up the filtering criteria to discard any packet that is
     * not of SMT type.  The streams module maintains an internal stack
     * upon which it performs actions (the stack elements are all 2 octets)
     * The filter works as follows:
     *
     *		- Push the 13th and 14th byte of the incoming packet
     *		- Push the FDDI_MASK which is a predefined constant
     *		- request a logical AND operation by pushing ENF_AND
     *		  This operation takes the top two elements of the stack
     *		  and performs a logical AND leaving the result on top of
     *		  the stack.
     *		- Then we push the Frame class type on the stack
     *		- Request an equality test.  This compares the top two elements
     *		  of the stack leaving a non-zero value on top of the stack if
     *		  the two values are equal.
     *
     * The stream will deliver the packet only if the value on top of the
     * stack is non-zero, meaning it meets the criteria we set.
     */

    fwp = &pf.Pf_Filter[0];
    *fwp++ = ENF_PUSHWORD +
	((u_int) &machdr.type - (u_int) &machdr.da) / sizeof (u_short);
    *fwp++ = ENF_PUSHLIT;
    *fwp++ = FDDI_MASK;
    *fwp++ = ENF_AND;
    *fwp++ = ENF_PUSHLIT;
    *fwp++ = MAC_FC;
    *fwp++ = ENF_EQ;
    pf.Pf_FilterLen = fwp - &pf.Pf_Filter[0];
    pf.Pf_Priority = 5;			/* meaningless, shoul be > 5 */

    /* 
     * Now set the filter using ioctl.
     */

    si.ic_cmd = NIOCSETF;
    si.ic_timout = -1;
    si.ic_len = sizeof (struct packetfilt);
    si.ic_dp = (char*) &pf;
    if (ioctl (nit_fddi, I_STR, &si) < 0) 
    {
	send_message (-SET_FILT_FAIL, FATAL,
		      "%s setting packet filter failed: %s",
		      test_name, errmsg (errno));
    }
    send_message (0, VERBOSE, "%s Packet Filter in place", test_name);

    /* 
     * Now we go ahead and bind the device 'fddi0' to the NIT device.  
     * This deferral is done so that we are not swamped with packets 
     * which are of no interest to us.
     */

    strcpy (ifr.ifr_name, DEVICE_NAME);
    si.ic_cmd = NIOCBIND;
    si.ic_timout = 0;
    si.ic_len = sizeof (struct ifreq);
    si.ic_dp = (char*) &ifr;
    if (ioctl (nit_fddi, I_STR, &si) < 0) 
    {
	send_message (-NIT_BIND_FAIL, FATAL,
		      "%s could not bind fddi to /dev/nit: %s",
		      test_name, errmsg (errno));
    }
    send_message (0, VERBOSE, "%s /dev/nit bound to %s",
		  test_name, DEVICE_NAME);

    /* 
     * Flush the readside of the stream
     */

    if (ioctl (nit_fddi, I_FLUSH, (char*) FLUSHR) < 0) 
    {
	send_message (-NIT_FLUSH_FAIL, FATAL, "%s read flush failed: %s",
		      test_name, errmsg (errno));
    }
    send_message (0, VERBOSE, "%s Flush of readside of stream succeceded",
		  test_name, DEVICE_NAME);
    TRACE_OUT
    return (nit_fddi);
}


/*****************************************************************************

  init_sif_packet ()

  This function intializes a SIF packet.  We have one global buffer, of octets,
  which is cast here appropriately and initialized.  The packet by the way is
  SIF Configuration Request type of SMT packet.  The transaction_id of the
  packet is set to broadcast (0x12345678).  This transaction_id is not a part
  of the FDDI standard.  This transaction_id is copied by the responding node
  into the response packets transaction_id.

*****************************************************************************/
void	init_sif_packet ()
{
    struct ifreq	ifr;
    struct strioctl	si;
    
    func_name = "init_sif_packet";
    TRACE_IN

    /* 
     * Figure out our own address.  After the 'ioctl' call the field
     * ifr.ifr_addr.sa_data will hold our address.
     */

    strcpy (ifr.ifr_name, DEVICE_NAME);
    si.ic_cmd = SIOCGIFADDR;
    si.ic_timout = -1;
    si.ic_len = sizeof (struct ifreq);
    si.ic_dp = (char*) &ifr;
    if (ioctl (fd, I_STR, (char*) &si) < 0) 
    {
	send_message (-IF_ADDR_FAIL, FATAL,
		      "%s could not get interface address: %s",
		      test_name, errmsg (errno));
    }
    memcpy (our_addr.octet, ifr.ifr_addr.sa_data, sizeof (mac_addr_t));
    send_message (0, DEBUG, "%s Our MAC addr: %d:%d:%d:%d:%d:%d",
		  test_name,
		  our_addr.octet[0], our_addr.octet[1], our_addr.octet[2],
		  our_addr.octet[3], our_addr.octet[4], our_addr.octet[5]);
    
    /* 
     * Initialize the different fields of the the SMT header
     */

    sif.frame_class = SMT_SIF_CLASS;
    sif.frame_type = SMT_SIF_REQUEST;
    sif.smt_version = SMT_VERSION;
    sif.transaction_id = 0x12345678;		  /* arbitrary */
    sif.station_id[0] = sif.station_id[1] = 0;
    memcpy (&sif.station_id[2], our_addr.octet, sizeof (mac_addr_t));
    sif.pad =  sif.info_field_len = 0;
    TRACE_OUT
}


/*****************************************************************************

  send_sif_packet (addr)

  This function sends out a SIF packet out on the interface.  It initializes
  the destination address before doing so.

*****************************************************************************/
void	send_sif_packet (addr)
mac_addr_t*	addr;
{
    struct sockaddr	sock_addr;
    struct strbuf	data, ctl;
    machdr_t*		machdr;
    
    func_name = "send_sif_packet";
    TRACE_IN

    /* 
     * Initialize the stream buffer structures for send/recv calls
     */

    ctl.len = sizeof (struct sockaddr);
    data.len = sizeof (smthdr_t);
    ctl.buf = (char*) &sock_addr;
    data.buf = (char*) &sif;
    
    /* 
     * Initialize the fields of the structure sock_addr.
     */

    sock_addr.sa_family = AF_UNSPEC;
    machdr = (machdr_t*) sock_addr.sa_data;
    memcpy (machdr->da.octet, (char*) addr, sizeof (mac_addr_t));
    memcpy (machdr->sa.octet, our_addr.octet, sizeof (mac_addr_t));
    machdr->type = MAC_FC;
    if (putmsg (fd, &ctl, &data, 0) < 0) 
    {
	send_message (-SEND_FAIL, FATAL, "%s could not send: %s",
		   test_name, errmsg (errno));
    }
    send_message (0, VERBOSE, "%s Sent the SIF packet successfully",
		  test_name);
    TRACE_OUT
}



/*****************************************************************************

  cmp_fddi_addr (addr1, addr2)

  This function compares the address' addr1 and addr2.  The address 'addr1' is
  in the normal form and the address 'addr2' is in the FDDI format with the
  most significant bit in bit 0 and the least significant bit in bit 7.  This
  function returns a non-zero value on miscompare.

*****************************************************************************/
int	cmp_fddi_addr (addr1, addr2)
u_char*	addr1;
u_char*	addr2;
{
    int		status = 0;
    u_char	j;
    u_char	i = 0;
    u_char	tmp;

    func_name = "cmp_fddi_addr";
    TRACE_IN
    while ((!status) && (i < 6)) 
    {
	tmp = 0;
	for (j = 1 ; j != 0; j <<= 1) 
        {		  			/* format the FDDI addr */
	    if (j & *(addr2 + i)) 
	    {
		tmp += 128 / j;
	    }
	}
	if (tmp != *(addr1 + i)) 
	{
	    status = 1;
	}
	i++;
    }
    TRACE_OUT
    return (status);
}
	

/*****************************************************************************

  receive_sif_packet ()

  This function reads the stream.  The global buffers 'recv_cntl' and
  'recv_data' will hold the packet on return.  The function 'getmsg' is
  used to read the stream head.  We wait a maximum of 30 seconds for a
  successful read from the stream.  If after the 30 seconds there is no
  message at the stream head we exit the program.

*****************************************************************************/
void	receive_sif_packet ()
{
    int			mode = 0;
    int			done = 0;
    int			i = 30;			  /* 30 seconds */
    struct strbuf	l_cntl;
    struct strbuf	l_data;
    
    func_name = "receive_sif_packet";
    TRACE_IN
    l_cntl.len = l_data.len = 0;
    l_cntl.maxlen = l_data.maxlen = BUFSIZ;
    l_cntl.buf = (char*) recv_cntl;
    l_data.buf = (char*) recv_data;
    while ((!done) && (i > 0)) 
    {
	switch (getmsg (fd, &l_cntl, &l_data, &mode)) 
	{
	    case EAGAIN:
		send_message (0, DEBUG, "%s stream empty: %s",
			      test_name, errmsg (errno));
		i--;
		sleep (1);			  /* wait for a second */
		break;
	    case 0:
		done = 1;
		send_message (0, VERBOSE,
			      "%s Received the SIF packet successfully",
			      test_name);
		break;
	    default:
		send_message (-RECV_FAIL, FATAL, "%s couldn't receive: %s",
			      test_name, errmsg (errno));
		break;
	}
    }
    if (!i) 
    {
	send_message (-RECV_TMOUT, FATAL, "%s getmsg timed out", test_name);
    }
    TRACE_OUT
}

/*****************************************************************************

  get_neighbor_address ()

  This function finds out the Upstream and Downstream neighbor addresses of
  this node.  Then it examines each packets MAC field to determine if it is our
  immediate Upstream or Downstream neighbor.  When this function returns the
  global variables 'una' and 'dna' contain valid addresses of the Upstream and
  Downstream neighbor respectively.

  	- Send out a SIF Configuration Request to everybody by broadcasting
	- Read the stream untill we find out the Upstream/Downstream neighbors

  NOTE: The SIF Response packet information field varies from revision to
	revision.  This may affect where exactly you will find the Up/Down
	stream neighbor address.  The testing for the neighbor address should
	be modifed as the version specifies.

*****************************************************************************/
void	get_neighbor_address ()
{
    int			done = 0;
    u_short		index = 0;
    u_short		param_type = 0;
    sif_resp_t*		sif_resp;
    
    func_name = "get_neighbor_address";
    TRACE_IN
    send_sif_packet (&broadcast);
    sif_resp = (sif_resp_t*) recv_data;
    while (done != 2)  			/* until UNA & DNA are found */
    {
	param_type = 0;
	receive_sif_packet ();

	/* 
	 * Check that we have the right kind of frame.  The packet 
	 * filter just checks to see that we have a SMT frame.  We 
	 * check in the following order:
	 * 
	 * 	- frame class should be SIF
	 * 	- frame type should be an response
	 * 	- transaction id should match with our own copy
	 * 	
	 * If this condition is satisfied then we check the MAC 
	 * neighbor field of the packets info field.  If the upstream 
	 * or the downstream neighbors address match our address then 
	 * we have a neighbor!  Save the source address appropriately
	 */

	if ((sif_resp->smt.frame_class == SMT_SIF_CLASS) &&
	    (sif_resp->smt.frame_type == SMT_SIF_RESPONSE) &&
	    (sif_resp->smt.transaction_id == 0x12345678)) 
        {

	    /* 
	     * Figure out the index for the MAC Neighbors field in the 
	     * information field
	     */

	    while ((param_type = * (u_short*) &sif_resp->sif_info_field[index])
		   != MAC_NEIGHBOR) 
            {
		index += * (u_short*) &sif_resp->sif_info_field[index + 2] + 4;
	    }
	    if (!cmp_fddi_addr (our_addr.octet,
				&sif_resp->sif_info_field[index + 8])) 
	    {
		memcpy (dna.octet, &sif_resp->machdr.sa, sizeof (mac_addr_t));
		send_message (0, VERBOSE, "%s DNA: %d:%d:%d:%d:%d:%d",
			      test_name,
			      dna.octet[0], dna.octet[1], dna.octet[2],
			      dna.octet[3], dna.octet[4], dna.octet[5]);
		done++;
	    }
	    if (!cmp_fddi_addr (our_addr.octet,
				&sif_resp->sif_info_field[index + 14])) 
   	    {
		memcpy (una.octet, &sif_resp->machdr.sa, sizeof (mac_addr_t));
		send_message (0, VERBOSE, "%s UNA: %d:%d:%d:%d:%d:%d",
			      test_name,
			      una.octet[0], una.octet[1], una.octet[2],
			      una.octet[3], una.octet[4], una.octet[5]);
		done++;
	    }
	}
    }
    TRACE_OUT
}


/*****************************************************************************

  run_fddi_tests (num_passes)

  This function runs the actual test until 'num_passes' number of passes are
  registered.  If we hit the error threshold as specified in ERROR_LIMIT then
  the function is exited also.  This function returns the number of errors
  encountered during the test.

*****************************************************************************/
int	run_fddi_tests (num_passes)
{
    int		l_passes = 0;
    int		l_errors = 0;
    int		i = 0;
    int		done = 0;
    sif_resp_t*	sif_resp;
    mac_addr_t	dest;
    
    func_name = "run_fddi_tests";
    TRACE_IN
    
    /* 
     * arbitrary, but different from the broadcast SIF packet which is
     * 0x12345678
     */

    sif.transaction_id = 0x87654321;
    
    /* 
     * Talk a while with the Upstream Neighbor untill we are bored
     */
    
    sif_resp = (sif_resp_t*) recv_data;
    while ((l_passes < (num_passes * 2)) && (!done)) 
    {
	send_message (0, VERBOSE, "%s Loop #: %d", test_name, l_passes);
	if (i++ % 2) 
        {
	    send_message (0, VERBOSE, "Talking to Upstream Neighbor");
	    memcpy (dest.octet, una.octet, sizeof (mac_addr_t));
	}
	else 
        {
	    send_message (0, VERBOSE, "Talking to Downstream Neighbor");
	    memcpy (dest.octet, dna.octet, sizeof (mac_addr_t));
	}
	send_sif_packet (&dest);
	receive_sif_packet ();
	if ((sif_resp->smt.frame_class == SMT_SIF_CLASS) &&
	    (sif_resp->smt.frame_type == SMT_SIF_RESPONSE) &&
	    (sif_resp->smt.transaction_id == 0x87654321)) 
        {
	    if (memcmp(&sif_resp->machdr.sa, &dest, sizeof (mac_addr_t)) == 0) 
            {
		l_passes++;
	    }
	    else 
	    {
		done++;
		send_message (-SA_MISMATCH, ERROR, "Bad SA in SIF response");
		if (++l_errors >= ERROR_LIMIT) 
		{
		    send_message (-TOOMANY_ERR, ERROR, "Too many errors");
		    TRACE_OUT
		    return (l_errors);
		}
		else 
		{
		    if (!run_on_error) 
		    {
			send_message (-EXIT_ON_ERROR, ERROR,
				      "Exiting on first error");
			TRACE_OUT
			return (1);
		    }
		}
	    }
	}
    }
    TRACE_OUT
    return (l_errors);
}


/*****************************************************************************

  clean_up ()

  This function is called before exiting program in any way.  This function
  closes all the open file descriptors, programs the FDDI firmware to its
  default state.

*****************************************************************************/
clean_up()
{
    func_name = "clean_up";
    close (fd);					  /* close /dev/nit */
    if (fw_reconfigured == RECONFIGURED) 
    {
	fddi_fw_setup (ENABLE);
    }
}


/*****************************************************************************

  fddi_test 

*****************************************************************************/
void
fddi_test()
{
    func_name = "fddi_test";
    
    /* 
     * Initialize all the necessary stuff like the test name, parse 
     * the command line....
     */

    TRACE_IN
    versionid = "1.1";
    device_name = DEVICE_NAME;

    /* 
     * If we are running from within SunDiag, then no need to do 
     * verify that the interface is present.  Because SunDiag will not 
     * allow the user to call the test if the interface is not present.
     */

    if (!exec_by_sundiag) 
    {
/*
    test_init (argc, argv, NULL, NULL, NULL);
*/
	fddi_probe ();
    }

    /* 
     * Set up the FDDI board to respond and pass up all SMT frames.  
     * This is required to run this test.  We send out SIF frames, 
     * which belong to the broad class of SMT frames, during the 
     * course of test to establish the upstream and downstream 
     * neighbor addresses and communicate with them.  This involves
     * setting a bit in the FDDI firmware.
     */

    fddi_fw_setup (DISABLE);
    fw_reconfigured = RECONFIGURED;

    /* 
     * Open a streams interface to the FDDI
     */

    fd = nit_open ();

    /* 
     * Initialize the different fields of a SIF Configuration Request packet.
     */

    init_sif_packet ();
    
    /* 
     * Build neighbor addresses by sending out a SIF configuration request
     * packet.
     */

    get_neighbor_address ();
    
    /* 
     * The function run_fddi_tests actually executes the test.  If the 
     * variable 'single_pass' dictates how long the test will take.
     */

    run_fddi_tests ((single_pass || quick_test) ? 1 : TOTAL_PASS);
    
    /* 
     * Reset the FDDI firmware to its default state.
     */

    fddi_fw_setup (ENABLE);
    TRACE_OUT
}
