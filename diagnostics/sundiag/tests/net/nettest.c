#ifndef lint
static char     sccsid[] = "@(#)nettest.c 1.1 7/30/92 Copyright 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/******************************************************************************

  program : nettest.c

  Original Author  : Frank Jones

  1st Revision	   : SCHEUFELE, SUSAN	susans@freedonia

		     Complete clean up and rewrite of original
		     code including 4.0 bug fixes.

  2nd Revision     : MASSOUDI, ROBERT A.     bobm@thesius
		     2-10-88,

		     Structural change into more readable blocks, Complete
		     documentation and comments, bug fix, enhancements, and
		     mods to accommodate other network devices as well
		     (i.e. FDDI)

  3rd Revision	   : Anantha N. Srirama		anantha@ravi

		     Total rewrite, I ripped the guts out of this program.
		     The present version bears very little resemblance to
		     the previous version, tons of useless stuff has been
		     thrown away with vengeance.

  enet.c bug fix and enhancements made in this Version :
  ------------------------------------------------------

  The code now supports FDDI and with minor alteration it should be able to
  support any other network interface device that adheres to the IPC and the
  established inet standards.


  BUG: The nettest software could not handle multiple network interfaces
  configured with different IP addresses (due to the "gethostname" and
  "gethostbyname" scheme), so if you were to configure a system with ie0 having
  one IP address and ie1 having a different IP address and so forth, the test
  would pass ie1 even if you unplugged the cable from ie1 (due to host local
  loopback).  The test would fetch the wrong IP address (ie0) as its own and
  compare it against the received packet and would mark that as a valid packet
  (not as the host local loopback), and consequently pass the diagnostics.


  FIX : use a structure that allows storing of the name,the IP address and the
  broadcast address of each interface. Thus when you are testing a network the
  correct IP address is used to check for host local loopback condition.  The
  storing of name allows the test to directly specify which network interface
  it is testing by its "if" name rather than net0 net1..and so on.  This
  enhancement allows the user' to see the actual "if" name "ie0", "fddi0", etc.


  What is this code supposed to do ? well, I'm glad you asked
  -------------------------------------------------------------

  The net test uses a raw socket with the ICMP protocol (Internet Control
  Protocol), specifically with the message types ICMP_ECHO and ICMP_ECHOREPLY.
  The net test broadcasts an ICMP_ECHO message on a valid network interface,
  and expects to hear a reply from some other host besides itself (localhost
  loopback).  Due to the low level nature of ICMP protocol (access through
  sockets, no routing), only ONE net test can run on a system at one time.
  Running more than one net test at a time has unpredictable results. The test
  uses the SICGIFCONF, SIOCGIFFLAGS, and SIOCGIFBRDADDR ioctls to obtain the
  network interface configuration, receive the "interface flags" to see if the
  net is up or down, and obtain the broadcast address.  The test assumes a
  broadcast network, not a point to point network.  The net_diag test ignores
  "me0" network, if it exists for one or more IPC boards.

  The nettest test formats with specific data and then sends out an ICMP_ECHO
  message to INADDR_ANY (any valid address) on a specified network interface,
  and then waits for replies for a given time period. The replies are counted
  and checked to make sure they have the same data.  Since we want to test the
  network hardware, a reply from the localhost is not counted.  It is
  considered to be an error if there are no replies from any other hosts. 

  Enhancements :
  --------------

  The test now spits out the actual network interface device names used in the
  configuration rather than "enet0", "enet1", ..etc, in all of its messages.

  In the debug mode it also prints out the names of all of the network
  devices found in addition to the number of devices found.

  ADDITIONS:
  ----------

    1) The test can now do a single test on a single (specified) network
       interface or conduct a test on all available and valid network
       interfaces.  Supports onboard ie, le, second ie or le, fddi, and
       token ring (can also support multiple network interfaces).

    2) Initially, the internal driver error counts are checked.  After running 
       the broadcastnets test, the spray test, the udp transmit test, and
       the fddi test (if applicable), the internal driver error counts
       are checked again.  If the difference between the initial and final
       error counts is greater than zero, a warning message is issued.
    
    3) If the interface is fddi, then the fddi interface is tested before
       it goes through the broadcasting, the spray test, and the udp transmit
       test.  In a ring topology, receive timeouts are often seen (from 
       broadcasting or perhaps the test machines are tied up with other tests).
       (Reason for running the fddi_test before the broadcasting, spray, and
       udp test:  if it passes and the broadcasting fails due to time out,
       we can at least assume that the nit/sif level is functional.) 
       Fddi_test tests the interface through the nit/sif (fddi) 
       level; therefore, it does not need to go through the host.  


		 communication through the hosts
                   ____________________________ 
		   |			      |
		--------                   --------
		| host |		   | host |
		--------                   --------
		   |			      |
		   |			      |
		--------                   --------
	        | fddi |  <------------->  | fddi |
		--------                   --------
	        
    4) To stress test the standard transmit and receive paths, the 
       nettest_spray and nettest_udp functions have been added.  Upon
       completion, these two functions print statistical information 
       (ie.transmit rates). Using RPC, nettest_spray sends a one-way stream 
       of packets to the first valid host that replies from the net test 
       broadcasts (an ICMP_ECHO message on a valid network interface).  A 
       warning is issued if the percentage of packets dropped returned by 
       nettest_spray exceeds 80%.   In the UDP test, no connection to the 
       remote host/port, is made, UDP "datagrams" are sent directly.

    5) This test records the following statistical information:

    	  The accumulated number of packets transferred
	  The accumulated number of bits transferred
	  The accumulated number of bytes/bits in error
	  The accumulated number of packets in error
	  The accumulated byte/bit error rate

  Usage: nettest [net_if_name{0-n}|allnets] [D] [W] [S] [C] 
                 [v] [s] [u] [p] [r] [q] [d] [t] 

	 Routine specific arguments [defaults]:
	    [net_<IF_NAME><#> | allnets] = net interface board to test
	    [net_ie0] or all available net interface board
            [D=] = time delay for the spray test, default is 0
            [W]  = flag to print warning messages, default is FALSE 
            [S]  = flag to enable/disable the spray test, default is enabled 
            [C=] = number of packets for the spray test, default is 10000

*******************************************************************************

		      INCLUDE FILES

******************************************************************************/
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>



/******************************************************************************

  			GLOBAL CONSTANTS
			INCLUDE FILES

******************************************************************************/
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "sdrtns.h"
#include "../../../lib/include/libonline.h"
#include "nettest.h"


/******************************************************************************

		     Global Variable Definitions

******************************************************************************/
struct avail_inet_if_struct valid_if[MAXDEVNUM];

struct timeval  sel_time = {1, 0};
struct timeval  current_time[4]; 	/* Total of 4 loops per nettest */

char	device_netname[30];
char	*device = device_netname;

int	id_pid;
int	seqno = 0;
int	packetsize = PACKETSIZE;
int	test_flag = NO_IF;
int	spray_delay = 0;  	/* default=0 for delay option in spray test */
int	spray_cnt = 10000;  	/* default=10000 for count option in spray test */
int	print_warning = FALSE;  /* default=FALSE for print warning messages option */
int	spray_test = TRUE;  	/* default=TRUE for spray test option */
int     tr_driver_error = TRUE; 

char *test_usage_msg = "[net_<IF_NAME>|allnets] [D=<delay for spray test>] [W] [S] \n [C=<number of packets to spray>]\n";
char	*pr_type ();
u_short	in_cksum ();


/******************************************************************************
		     External Variable Definitions
******************************************************************************/
extern float    nettest_spray();
extern int 	udp_test();
extern void     fddi_test();
extern char    *inet_ntoa ();


/***************************************************************************
	Process nettest specific command line arguments.
****************************************************************************/
int
process_net_args(argv, arrcount)
char	*argv[];
int	arrcount;
{
	if (strcmp (argv[arrcount], "allnets") == 0) 
        {
		test_flag = ALL_IF;
	}
	else if (strncmp (argv[arrcount], "net_", 4) == 0) 
            {
		test_flag = SPECIFIC_IF;
		(void) strcpy (device, (argv[arrcount] + 4));
		device_name = device;
	     }
             else if (strncmp (argv[arrcount], "D=", 2) == 0) 
                  { 
                      spray_delay = atoi(&argv[arrcount][2]);
             	  }
                  else if (strncmp (argv[arrcount], "W", 1) == 0) 
  		       { 
                           print_warning = TRUE; 
             	       }
                       else if (strncmp (argv[arrcount], "C=", 2) == 0) 
 			    {
                               spray_cnt = atoi(&argv[arrcount][2]);
                            }
                            else if (strncmp (argv[arrcount], "S", 1) == 0) 
 			         { 
                                     spray_test = FALSE; 
             	                 }
	else 
        {
		 return (FALSE);
   	}

	return (TRUE);
}


int
routine_usage()
{
        send_message(0, CONSOLE, "\n\
Routine specific arguments [defaults]:\n\n\
\tnet_<IF_NAME><#> = net interface board to test, e.g. net_ie0\n\
\tallnets          = all available net interface board\n\
\tD                = time delay for the spray test, default is 0\n\
\tW                = flag to allow printing of warning messages,\n\
\t\t           default is FALSE\n\
\tS                = flag to disable the spray test of nettest\n\
\tC                = number of packets to send in spray test,\n\
                    \t   default is 10000\n");
}
/******************************************************************************

  select_nets_to_test

  Initialize test flags to DONT_TEST for all available boards.  Then check to
  see if no device is specified and set default to test the first IF board in
  the list.  Otherwise, if ALL_IF was specified then we set them all to test,
  and if the SPECIFIC_IF was specified and if we get a match then we will set
  the test flag for that specific flag.  If none of the above conditions are
  met this module returns a FAIL code.

******************************************************************************/
int	select_nets_to_test (valid_if_p, device, net_boards)
struct  avail_inet_if_struct *valid_if_p;
char	*device;
int	net_boards;
{
    int not_found;
    int	i;

    func_name = "select_nets_to_test";
    TRACE_IN
    not_found = net_boards;
    for (i = 0; i < net_boards; i++) 
    {
	valid_if[i].test_flag = DONT_TEST;
    }
    for (i = 0; ((i < net_boards) && not_found); i++) 
    {
	switch (test_flag) 
        {
	    case SPECIFIC_IF:
	    	if (strcmp (valid_if_p[i].dev_name, device) == 0) 
                {
		    valid_if_p[i].test_flag = TEST;
		    not_found = 0;
		}
		break;

	    case ALL_IF:
		valid_if_p[i].test_flag = TEST;
		not_found--;
		break;

	    case NO_IF:
	    default:
		valid_if_p[i].test_flag = TEST;
		not_found = 0;
		break;
	}
    }
    TRACE_OUT
    return (not_found);
}


/******************************************************************************

  get_nets

  This function calls the function 'getbroadcastnets' to figure out what are
  the available nets in the system.  If no broadcast nets are found, then the
  program is exited with an error message/status.

******************************************************************************/
int	get_nets (device)
char	*device;
{
    int		net_boards, i, state;
    char	inbuf[UDPMSGSIZE];

    func_name = "get_nets";
    TRACE_IN
    net_boards = getbroadcastnets (valid_if, inbuf);
    if (net_boards < 1) 
    {
	send_message (-NO_NETWORKS, FATAL, "%s no networks",
		      test_name);
    }
    if (select_nets_to_test (valid_if, device, net_boards)) 
    {
	send_message (-NO_SELECTED_NETWORK, ERROR,
		      "%s network to test (%s) not found",
		      test_name, device);
    }
    TRACE_OUT
    return (net_boards);
}


/******************************************************************************

  make_sock

  This function opens a socket and binds it to the network address used.  It
  returns the socket that is opened to the calling routine.  The program is
  exited with an error code if it cannot open or bind the socket.

******************************************************************************/
int	make_sock (test_net, sock_type)
int	test_net;
int	sock_type;
{
    struct	protoent *proto;
    struct	sockaddr_in from;
    int		dontblock = 1;
    int		psock = 0;

    func_name = "make_sock";
    TRACE_IN
    if (!(proto = getprotobyname ("icmp"))) 
    {
	send_message (-NO_PROTOCOL_ENTRY, FATAL,
		      "%s No protocol entry for ICMP %s",
		      test_name, errmsg (errno));
    }
    if (sock_type == SOCK_DGRAM)
    {
       if ((psock = socket (AF_INET, SOCK_DGRAM, 0)) < 0) 
       {
        send_message (-NO_XMIT_SOCKET, FATAL, "%s socket open failed %s",                      test_name, errmsg (errno));
       }
    }
    if (sock_type == SOCK_RAW)
    {
        if ((psock = socket (AF_INET, SOCK_RAW, proto->p_proto)) < 0) 
        {
	    send_message (-NO_XMIT_SOCKET, FATAL, "%s  socket open failed %s",
		      test_name, errmsg (errno));
        }
    }
    bzero ((char *) &from, sizeof (from));
    from.sin_family = AF_INET;
    from.sin_addr = valid_if[test_net].my_addr;
    from.sin_port = 0;
    if ((bind (psock, (char *) &from, sizeof (from))) < 0) 
    {
	send_message (-NO_XMIT_SOCKET, FATAL, "%s socket bind failed %s",
		      test_name, errmsg (errno));
    }
    (void) ioctl (psock, FIONBIO, (char *) &dontblock);
    TRACE_OUT
    return (psock);
}


/******************************************************************************

  in_cksum	(fudged from ping.c)

  Checksum routine for Internet Protocol family headers (C Version)

******************************************************************************/
u_short	in_cksum (addr, len)
u_short	*addr;
int	len;
{
    register int    sum = 0;		       /* accumulator for checksum */
    register int    nleft = len;	       /* remaining bytes in buffer */
    register u_short *w = addr;		       /* word pointer in buffer */

    /*
     * Our algorithm is simple: using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the carry
     * bits from the top 16 bits into the lower 16 bits.
     */

    func_name = "in_chsum";
    TRACE_IN
    while (nleft > 1) 
    {
	sum += *w++;			/* add 16 bits at a time */
	nleft -= 2;
    }
    if (nleft == 1) sum += (int) (*(u_char *) w);

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */

    sum = (sum >> 16) + (sum & 0xffff);	       /* add hi 16 to low 16 */
    sum += (sum >> 16);			       /* add carry */
    TRACE_OUT
    return ((u_short) ~sum);
}


/******************************************************************************

  net_dtablesize

  Return max # of possible file descriptors

******************************************************************************/
int	net_dtablesize ()
{
    static int  called_already;

    if (!called_already) 
    {
	called_already = getdtablesize ();
    }
    return (called_already);
}


/******************************************************************************

  do_select

  This function calls the driver call 'select' with the 'what_select' status.
  It returns the residual 'time_count' to the calling routine.  A return value
  of 0 should be interpreted as a timeout condition by the calling routine.

******************************************************************************/
int	do_select (sock, what_select, time_count)
int	sock;
int	what_select;
int	time_count;
{
    fd_set	sel_mask;
    int		nfds = 0;  /* total number of ready descriptors in all sets */
			   /* returned by calling select */

    func_name = "do_select";
    TRACE_IN
    for ( ; ((time_count > 0) && (nfds <= 0)); time_count--) 
    {

	/*
	 * get size of descriptor table size for this process and then
	 * using it and then call select to get a mask of those descriptors
	 * which are ready.  The total number of ready descriptors is
	 * returned in nfds.  Select examines the I/O descriptors  specified
	 * by  the  bit masks  readfds,  writefds,  and exceptfds to see if
	 * they are ready for reading, writing, or have an exceptional
	 * condition  pending,  respectively.  Descriptor masks are reset
 	 * after each call to select since they are modified if no errors
         * occur.  Resetting descriptor masks after each call to select
         * helps prevent bogus error reports when a timeout occurs.
	 */

    FD_ZERO (&sel_mask);       /* initializing descriptor masks to null */
    FD_SET (sock, &sel_mask);  /* set/reset descriptor masks */

	if (what_select == WRITE_SELECT) 
 	{
	    nfds = select (net_dtablesize (), (fd_set *) NULL, &sel_mask,
			   (fd_set *) NULL, &sel_time);
	}
	else 
 	{
	    nfds = select (net_dtablesize (), &sel_mask, (fd_set *) NULL,
			   (fd_set *) NULL, &sel_time);
	}
	if (nfds < 0) 
 	{ 
	    send_message (-SELECT_ERROR, ERROR, "%s select %s",
			  test_name, errmsg (errno));
	}
    }
    TRACE_OUT
    return (time_count);
}


/******************************************************************************

  ping_send

  This function sends out a ICMP packet out on the network.  It sets up the
  packet with the appropriate information, generates the checksum, and then
  calls the function 'do_select' to check if the interface is ready for
  transmission.  After this the packet is is shot out and the function returns.
  If it encounters any errors during this, the program is exited with an
  appropriate error code.

******************************************************************************/
void	ping_send (sock, to, device)
int	sock;
struct	sockaddr_in *to;
char	*device;
{
    static	u_char   pack[MAXPACKLEN];
    struct	icmp    *icmp_p = (struct icmp *) pack;
    struct	timeval *time_p = (struct timeval *) & pack[8];
    int		len;

    func_name = "ping_send";
    TRACE_IN
    (void) gettimeofday (&current_time[seqno], (struct timezone *) NULL);
    bcopy ((char *) &current_time[seqno], (char *) time_p,
	   sizeof (struct timeval));
    icmp_p->icmp_type = ICMP_ECHO;
    icmp_p->icmp_code = 0;
    icmp_p->icmp_cksum = 0;
    icmp_p->icmp_seq = seqno++;
    icmp_p->icmp_id = id_pid;
    icmp_p->icmp_cksum = in_cksum ((u_short *) icmp_p, packetsize);
    if (do_select (sock, WRITE_SELECT, WAIT_TIME) == 0) 
    {
	send_message (-TRANSMIT_BC_TIMEOUT, ERROR,
		      "%s could not broadcast (timeout) on %s",
		      test_name, device);
    }
    send_message (0, DEBUG,
		  "%s: Broadcasting ICMP Packet (sendto), ICMP hdr info:",
		  func_name);
    send_message (0, DEBUG, "Type=0x%x, Code=0x%x, Cksum=0x%x, Seq=0x%x, ID=0x%x, B-addr=%sData = 0x%x",
			icmp_p->icmp_type, icmp_p->icmp_code,
			icmp_p->icmp_cksum, icmp_p->icmp_seq,
			icmp_p->icmp_id, inet_ntoa (to->sin_addr),
			*time_p);
    /* is packet sent out = packetsize */
    if ((len = sendto (sock, (char *) pack, packetsize, 0,
		       (struct sockaddr *) to, sizeof (*to))) != packetsize) 
    {
	send_message (0, DEBUG,
		      "%s: send len=%d, packetsize=%d, System errno = %d",
		      func_name, len, packetsize, errno);
	send_message (-NO_TRANSMIT_BC, ERROR, 
		      "%s Transmit failed on %s, broadcast packet %s",
		      test_name, device, errmsg (errno));
    }
    TRACE_OUT
}


/******************************************************************************

  check_reply

  This function checks a packet for integrity.  It checks all the information
  in the packet for correctness.  Any error/mismatch results in the termination
  of checking the reply and displaying an appropriate message.

******************************************************************************/
int 	check_reply (buf, cc, from, device)
char	*buf;			/* IP packet received */
int	cc;			/* received packet length */
struct	sockaddr_in *from;	/* INTERNET style socket address received */
char	*device;
{
    struct ip      *ip;	     /* Struct of an INTERNET hdr, naked of options. */
    struct icmp    *icp;
    struct timeval *time_p;
    int		hdr_len, c;
    int         valid_reply = 1;
    u_short	tmp, cksum_save;

    func_name = "check_reply";
    TRACE_IN
    ip = (struct ip *) buf;		       /* convert to IP structure */
    hdr_len = ip->ip_hl << 2;
    c = cc - hdr_len;
    icp = (struct icmp *) (buf + hdr_len);
    time_p = (struct timeval *) (buf + hdr_len + 8);

    /*
     * Start checking the packet now:
     *
     *   packet type must be ICMP_ECHO.  If the type is ICMP_UNREACH, then the
     *		packet is not tagged as an error, instead it is ignored.
     *   packet size
     *   Verify checksum
     *   Must be our packet (that is same process ID)
     *   Must have the right sequence number
     *   Must have the same time stamp for a valid sequence number
     */
    
    if (icp->icmp_type != ICMP_ECHOREPLY) 
    {
        if ((icp->icmp_type != ICMP_UNREACH) &&
	     (icp->icmp_type != ICMP_REDIRECT)) 
        {
	    if (print_warning)
	    {
	    	send_message (0, WARNING,
	        "%s type = %d, should be ICMP_ECHOREPLY, from %s, on %s",
		test_name, icp->icmp_type, inet_ntoa (from->sin_addr), device);
	    }
	}
	TRACE_OUT
	return(!valid_reply);
    }

    if ((cc < hdr_len + ICMP_MINLEN) || (c != packetsize)) 
    {
	if (print_warning)
	{
	    send_message (0, WARNING,
	    "%s bad pkt size %d should be %d, from %s, on %s",
	    test_name, cc, packetsize, inet_ntoa (from->sin_addr), device);
	}
        TRACE_OUT
        return(!valid_reply);
    }

    cksum_save = icp->icmp_cksum;
    icp->icmp_cksum = 0;

    if ((tmp = in_cksum ((u_short *) icp, packetsize)) != cksum_save) 
    {
        if (print_warning) 
	{
	    send_message (0, WARNING, "%s bad cksum:\tExpect 0x%hx\tGot: 0x%hx", test_name, icp->icmp_cksum, tmp);
	}
        TRACE_OUT
        return(!valid_reply);
    }

    icp->icmp_cksum = cksum_save;

    if (icp->icmp_id != id_pid) 
    {
	if (print_warning)
	{
            send_message (0, WARNING,
	    "%s pkt id (%d) not our pkt id (%d), from %s, on %s",
	     test_name,icp->icmp_id,id_pid,inet_ntoa (from->sin_addr), device);
	}
        TRACE_OUT
        return(!valid_reply);
    }

    if (icp->icmp_seq >= seqno) 
    {
	if (print_warning)
	{
	    send_message (0, WARNING,"%s bad seq num:\tExp less than: %d\tGot: %d", test_name, seqno, icp->icmp_seq);
	}
        TRACE_OUT
        return(!valid_reply);
    }

    if (bcmp ((char *) &current_time[icp->icmp_seq], (char *) time_p,
	     sizeof (struct timeval)) != 0) 
    {
	if (print_warning)
	{
	    send_message (0, WARNING, 
           "%s packet data mis-compare (different time stamp), from %s, on %s", test_name, inet_ntoa (from->sin_addr), device);
	}
        TRACE_OUT
        return(!valid_reply);
    }
    TRACE_OUT
    return(valid_reply);
}


/******************************************************************************

  test_reply

  This function tests a host that has replied to the broadcast net.
  It makes sure that the replying host has not previously failed.
  Then it does 3 tests.  First it sprays large packets.  Second
  it sprays small packets.  Third, it tests UDP transmit.
  Once all three tests pass, this function is done and won't test further.
  Return status is true if tests have passed.

******************************************************************************/
int	test_reply (from, test_net)
struct          sockaddr_in from;       /* address of the responding node */
int     	test_net;
{
    int		test_succeeded = 0;
    struct hostent	*replying_host;
    static char old_host_that_failed[256];
    float       packets_dropped = 0;    /* percentage of packets dropped */
                                        /* after spraying host: hp->h_name */
    int         sundiag_and_delay = 0;  /* if sundiag flag (s) and spray */
                                        /* delay option (D=) are specified */ 
  
    /*
     * if sundiag_and_delay, use spray_cnt_large and spray_cnt_small instead 
     * of the default spray_cnt value = 10000.  spray_cnt_large and 
     * spray_cnt_small are the number of packets required to make the  
     * total  stream  size 100000 bytes
     */

    int    spray_cnt_large = 100000/1502;    
    int    spray_cnt_small = 100000/86;     

    func_name = "test_reply";
    TRACE_IN
    if ((replying_host = gethostbyaddr (&from.sin_addr.s_addr,
        	sizeof (struct in_addr),AF_INET)) == NULL)
    {
        send_message(-NO_HOST_NAME, ERROR,"%s: Invalid broadcast reply address.", func_name );
    }

    if (strcmp(replying_host->h_name, old_host_that_failed) == 0)
    {
	TRACE_OUT
	return(0);
    }

    if (spray_test)
    {
        if ((exec_by_sundiag) && (spray_delay >= 1))
        {
	    sundiag_and_delay = TRUE;
        }

        /* Large spray */

        if ((packets_dropped = nettest_spray(from,
           (sundiag_and_delay ? spray_cnt_large : spray_cnt),
           1502, spray_delay)) < 0)
        {
	    send_message(0, DEBUG, "%s: Large spray failed.", func_name);
	    strcpy(old_host_that_failed, replying_host->h_name);
  	    TRACE_OUT
            return(0);
        }

        if ((print_warning) && (packets_dropped > 80))
        {
	    send_message(0, WARNING,
		"SPRAY WARNING: more than 80 percent of packets dropped");
        }

        /* Small spray */

        if (!quick_test)
        {
            if ((packets_dropped = nettest_spray(from,
               (sundiag_and_delay ? spray_cnt_small : spray_cnt),
               86, spray_delay)) < 0)
            {
	        send_message(0, DEBUG, "%s: Small spray failed.", func_name);
	        strcpy(old_host_that_failed, replying_host->h_name);
  	        TRACE_OUT
                return(0);
            }

            if ((print_warning) && (packets_dropped > 80))
            {
	        send_message(0, WARNING,
		    "SPRAY WARNING: more than 80 percent of packets dropped");
            }
        }
    }
    /* 
     *  Test UDP.  If UDP fails for reasons other than transmitting packets, 
     *  the whole program will exit.  udp_test return 0 if it fails.
     */

    if ( (udp_test(test_net, from)) == 0)
    {
        test_succeeded = 0;
    }
    else
    {
        test_succeeded = 1;
    }

    TRACE_OUT
    return(test_succeeded);

} /* test_reply */


/******************************************************************************

  ping_receive

  This function receives a packet from the socket/network when the select
  call returns a ready.  Otherwise a timeout is indicated and it returns with
  the number of packets it received.  Each received packet is checked by the
  function 'check_reply' which stops checking in case of errors/mismatches.
  If the received packet is valid and it's not a reply from our own machine,
  then spray 10,000 packets of 86 bytes and 1502 bytes each to the first host 
  that replies from the net test broadcasts.  A warning is issued if the 
  percentage of packets dropped exceeds 80%.  Then the function udp transmit
  test is performed.  Like nettest_spray, nettest_udp shall only be called
  once for each interface. Should the number of broadcast replies = 0 or no
  "valid" host (broadcast reply) is found to test spray and udp, the program
   exits with the appropriate error message.
 
******************************************************************************/
void 	ping_receive (sock, my_addr, device, test_net)
int	sock;
struct	in_addr *my_addr;
char	*device;
int     test_net;
{
    struct	sockaddr_in from;       /* address of the responding node */
    u_char	pack[MAXPACKLEN];       /* IP packet received */
    int         broadcast_replies = 0;  /* number of replies from broadcast */
    int		valid_reply = 0;        
    int		cc;       		/* length of received packet */
    int		from_len;
    static int	tested = 0;
                                       
    static int  broadcast_retry=0;
    static int  succeeded_broadcast_retry = 0;
    char  command[80];           /* netstat command */
    FILE *fp_init, *fp_error;

    static struct hostent*   hp;    /* reply packet's machine name for spray */

    func_name = "ping_receive";
    TRACE_IN
    while (do_select(sock, READ_SELECT,
           (broadcast_replies ? WAIT_TIME:(15 * WAIT_TIME))) != 0)
    {
	/*
	 *  if the socket is not ready for a read (i.e. the socket is
	 *  empty) and time out has occurred, no data written to socket
	 *  (no new packets) we return the number of broadcast_replies.
	 */

	bzero ((char *) &from, sizeof (from));
	from_len = sizeof (from);
	if ((cc = recvfrom (sock, (char *) pack, sizeof (pack), 0,
		(struct sockaddr *) &from, (int *) &from_len)) < 0) 
        {
	    send_message (0, ERROR, "recvfrm %s", errmsg (errno));
	    continue;
	}

        if (! (valid_reply = check_reply ((char *) pack, cc, &from, device)))
	    continue;

        /* if reply not from our own machine */
        if (from.sin_addr.s_addr != my_addr->s_addr)
        {
            /* must pass two sprays and one udp test */
 
            if (!tested)
            {
                if ((hp = gethostbyaddr (&from.sin_addr.s_addr,
                                sizeof (struct in_addr),AF_INET)) != NULL)
                {
		    send_message (0, DEBUG,
                          "%s:  'from' REPLY (addr: %s) (host: %s)",
                          "ping_receive", inet_ntoa(from.sin_addr), hp->h_name);

                    tested = test_reply(from, test_net);
                }
            } 
            broadcast_replies++;
            send_message (0, DEBUG,
                          "%s: reply from (addr: %x) self (addr: %x)",
                          "ping_receive", from.sin_addr.s_addr, my_addr->s_addr);
        }                    
        else
        {
            send_message (0, DEBUG, "%s: received reply from self : %x",
                          "ping_receive", from.sin_addr.s_addr);
        }                    
    }  /* while */ 

    send_message(0, DEBUG, "%s: rcv timeout, %d replies to broadcast",
		 "ping_receive", broadcast_replies);

    /*  
     *  have gone through all hosts that replied but have not 
     *  sprayed any host
     */

    if (tested == 0)
    {
        if (strncmp (device, "le", 2) == 0)
        {
            if ((!succeeded_broadcast_retry) && (broadcast_retry < 3))
            {
                broadcast_retry++;
                send_message(0, DEBUG, "Retrying broadcast (%d).", broadcast_retry);  
                run_tests(test_net, device);
		if (tested)
			succeeded_broadcast_retry = TRUE;
            }
            else if (!succeeded_broadcast_retry)
            {   /* after 3 retries, and still have error 
		 * print out netstat status to /tmp/sundiag_netstat_info 
	         * upon error and verbose mode */
		if (verbose)
		{
                    if ((fp_init=fopen("/tmp/sundiag_init_netstat","r"))!=NULL)
		    {
		        sprintf (command, "/bin/mv /tmp/sundiag_init_netstat /tmp/sundiag_netstat_info");
		        system (command);
			if (fp_init != NULL)
			    fclose (fp_init);
		    }
                    else
		    {
                        send_message (-FILE_OPEN_ERROR, ERROR, "Cannot open /tmp/sundiag_init_netstat");
		    }

                    if ((fp_error = fopen ("/tmp/sundiag_netstat_info", "a")) != NULL)
                    { 
                        fprintf(fp_error,"\nNetstat after running nettest.\n");
		        fseek (fp_error, 0, 2);   /* get to the end of file */
                        sprintf(command,"netstat>> /tmp/sundiag_netstat_info");
		        system (command);
			if (fp_error != NULL)
			    fclose (fp_error);
                    }   
                    else
		    {
                        send_message (-FILE_OPEN_ERROR, ERROR, "Cannot open /tmp/sundiag_netstat_info");
		    }

		}  /* if verbose */
            }   
        }

	if (!succeeded_broadcast_retry)
	{
            if (broadcast_replies == 0)
            {
                if (strncmp(device, "tr", 2) == 0)
                {
                    send_message (0, VERBOSE,
                      "%s RCV timeout on %s, no replies to broadcast.",
                          test_name, device);
                    check_status(device, 1, print_warning, &tr_driver_error);
                    if (!tr_driver_error)
                        send_message (-RECEIVE_BC_TIMEOUT, ERROR,
                                "%s RCV timeout on %s, no replies to broadcast.  No error flags set by driver.", test_name, device);

                }
                else
                    send_message (-RECEIVE_BC_TIMEOUT, ERROR,
                      "%s RCV timeout on %s, no replies to broadcast",
                          test_name, device);
            }
            else
            {   
                if (strncmp(device, "tr", 2) == 0)
                {
                    send_message(0, VERBOSE, "Tried %d replies from broadcast.  No valid host found to test spray and udp.", broadcast_replies);
                    check_status(device, 1, print_warning, &tr_driver_error);
                    if (!tr_driver_error)
                        send_message(-RECEIVE_BC_TIMEOUT, ERROR,
                                "Tried %d replies from broadcast.  No valid host found to test spray and udp for %s.  No error flags set by driver.", broadcast_replies, device);
                } 

                else
                    send_message(-RECEIVE_BC_TIMEOUT, ERROR, "Tried %d replies from broadcast.  No valid host found to test spray and udp for %s.", broadcast_replies, device);
            }
	}
    }    

    TRACE_OUT

} /* ping_receive */


/******************************************************************************

  start_test

  Open a socket to the 'test_net', send and receive ICMP packets.  For a
  successful run we must receive more than 0 packets back.

******************************************************************************/
void	start_test (test_net, my_addr, device)
int	test_net;
struct	in_addr	*my_addr;
char	*device;
{
    static	struct sockaddr_in to;
    int		sock;

    /*
     * construct an internet address using the specified nth net valid INTERNET
     * broadcast addresses stored in the addrs table previously 
     */

    func_name = "start_test";
    TRACE_IN

    /* if device == fddi, run fddi_test */
/************* (NOTE:  this is commented out for Sundiag2.1/2.2 release)
    if ((strncmp (device, "fddi", 4)) == 0)
    {
	fddi_test();
	send_message (0, INFO, "Passed fddi (ent) test.");
    }
**************/

    bzero ((char *) &to, sizeof (to));
    to.sin_family = AF_INET;
    to.sin_port = 0;
    to.sin_addr = valid_if[test_net].broad_addrs;
    sock = make_sock (test_net, SOCK_RAW);
    id_pid = getpid () & 0xFFFF;
    send_message (0, DEBUG,
		 "%s: 'To' socket info used for ICMP communication", func_name);
    send_message (0, DEBUG,
		  "Family Type=0x%x, port=0x%x, Broadcast INET addr=%s",
		  to.sin_family, to.sin_port, inet_ntoa (to.sin_addr));
    ping_send (sock, &to, device);
    ping_receive (sock, my_addr, device, test_net);
    (void) close (sock);

    TRACE_OUT
}

/******************************************************************************

  run_tests

  This function tests the network 'test_net'.

******************************************************************************/
run_tests (test_net, device)
int	test_net;
char	*device;
{
    struct	in_addr my_addr, *dummy;

    func_name = "run_tests";
    TRACE_IN
    dummy = (struct in_addr *) &valid_if[test_net].my_addr;
    my_addr = *dummy;
    start_test (test_net, &my_addr, device);
    TRACE_OUT
}

/******************************************************************************

  main

  This program sets up and runs nettest on different nets.  It calls the
  function 'test_init' to extract the interface name.  Then it gets all
  the boards in the system which can be tested and runs the the test on each
  interface.

******************************************************************************/
int	main (argc, argv)
int	argc;
char	*argv[];
{
    struct net_stat ifdev_net_stat;   /* net stats for current IF under test */
    int	     i;
    int	     total_boards;
    extern int process_net_args();
    extern int routine_usage();

    versionid = "1.1";			/* SCCS version id */
    test_init(argc, argv, process_net_args, routine_usage, test_usage_msg);
    total_boards = get_nets (device);

    stat_start_up ();
    for (i = 0; i < total_boards; i++) 
    {
	if (valid_if[i].test_flag == DONT_TEST) 
        {
	    continue;
	}
	(void) strcpy (device, valid_if[i].dev_name);
        device_name = device;
	send_message (0, VERBOSE,"Starting nettest on network = %s",
		      device);
	init_net_stat (device);
        check_status(device, 0, print_warning, &tr_driver_error);
	run_tests (i, device);
	check_point_status (device, &ifdev_net_stat);
	print_net_stats (&ifdev_net_stat);
        check_status(device, 1, print_warning, &tr_driver_error);
    }
    test_end();
}

