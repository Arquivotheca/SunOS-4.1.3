/******************************************************************************

               "@(#)nettest.h 1.1 7/30/92 Copyright Sun Microsystems"

*******************************************************************************

  net diag structure used to store the name IP and broadcast network address of
  valid "if" interfaces to be tested.  This structure deals with the problem of
  having to set all "if" interfaces to the same IP address to do the proper
  testing.

******************************************************************************/
#include <net/if.h>

#define   NO_IF         0       /* no IF specified */
#define   SPECIFIC_IF   1       /* test the specified IF   */
#define   ALL_IF        2       /* test all available IF's */
#define   TEST          3       /* test device */
#define   DONT_TEST     4       /* don't test device */

struct avail_inet_if_struct {
       char	dev_name[IFNAMSIZ];	/* if name, e.g. "en0" */
       struct   in_addr my_addr;        /* my IP address */
       struct   in_addr broad_addrs;    /* broadcast adde used for my IF */
       int      test_flag;
};


/*
 * This structure contains the network statistics for packets transmitted
 * over the network for the length of the test.
 */

struct net_stat{
      char    name[IFNAMSIZ];          /* host or interface name */
      char    dstroutename[60];        /* Net/Dest            */
      char    src_netaddr[60];         /* Network Address     */
      int     if_ipackets,             /* packets received on interface */
              if_ierrors,              /* input errors on interface */
              if_opackets,             /* packets sent on interface */
              if_oerrors,              /* output errors on interface */
              if_collisions;           /* collisions on csma interfaces
                                         or ring down count for fddi  */
};

struct pkt_size{
  int  ipktsz, opktsz;        /* average packet size */
};

/******************************************************************************

                     ERROR DEFS

******************************************************************************/
#define PACKETSIZE       16
#define MAXPACKLEN      256 
#define WAIT_TIME        20
#define NETTEST_FAIL	  2

#define   READ_SELECT   0     /* select read function on socket */
#define   WRITE_SELECT  1     /* select write function on socket */

#define   MAXDEVNUM     20   /* Max num of network devs to be tested */


#define NO_SOCKET               3
#define NO_NETWORKS             4
#define NO_SELECTED_NETWORK     5
#define NO_HOST_NAME            6		
#define NO_PROTOCOL_ENTRY       7		
#define NO_XMIT_SOCKET          8
#define SELECT_ERROR	 	9	
#define NO_TRANSMIT_BC          10
#define TRANSMIT_BC_TIMEOUT     11
#define BC_RECVFROM_ERROR       12
#define PACKETSIZE_ERROR        13
#define RECEIVE_BC_TIMEOUT      14 
#define IOCTL_ERROR             15 
#define NAMELIST_ERROR          16  
#define MALLOC_ERROR            17 
#define TR_BOARD_ERROR          18
#define TR_NET_ERROR            19
#define TR_CABLE_ERROR          20
#define FILE_OPEN_ERROR         21

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN		64
#endif

