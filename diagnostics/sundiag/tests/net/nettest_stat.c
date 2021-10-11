#ifndef lint
static	char sccsid[] = "@(#)nettest_stat.c 1.1 7/30/92 Copyright Sun Microsystems";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/******************************************************************************

  nettest_stat.c

  Revison	   : Anantha N. Srirama		anantha@ravi

		     Total rewrite, I ripped the guts out of this program.
		     The present version bears very little resemblence to
		     the previous version, tons of useless stuff has been
		     thrown away with vengence.

******************************************************************************/
#include <sys/param.h>
#include <sys/vmmac.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/route.h>
#include <sys/mbuf.h>
#include <fcntl.h>
#include <machine/pte.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <kvm.h>
#include <ctype.h>
#include <errno.h>
#include <nlist.h>
#include <stdio.h>
#include <netdb.h>
#include <sunif/if_lereg.h>
#include <sunif/if_levar.h>
#include <sys/time.h>
#include <sunif/if_ievar.h>
#include "if_trvar.h"
#ifdef	 FDDI			/* defined in FDDI software was installed */
#include <sunfw/fddi/fddi_types.h>
#include <sunfw/fddi/commands.h>
#else
#include "fddi_types.h"
#include "commands.h"
#endif	 FDDI
#include <sys/ioctl.h>
#include <sdrtns.h>
#include "nettest.h"

extern char *routename (),  *inet_ntoa ();
extern char *netname ();
char*	index ();

kvm_t   *kd;
struct  nlist nl[] = {
	{""},
	{""}
};
struct  nlist nl_driver[] = {
	{""},
	{""}
};
struct	net_stat   start_net_stat;


#define   FMT1 "\n%-5.5s  %-12.12s  %-12.12s %-7.7s %-5.5s %-7.7s %-5.5s %-13.13s\n%-5.5s  %-12.12s  %-12.12s %-7d %-5d %-7d %-5d %-6d \n\n"

/******************************************************************************

  stat_start_up

******************************************************************************/
void	stat_start_up ()
{
    char *vmunix, *getenv();
    
    func_name = "stat_start_up";
    TRACE_IN
#ifdef sun386
    nl[0].n_name = "ifnet";
#else
    nl[0].n_name = "_ifnet";
#endif sun386
    vmunix = getenv("KERNELNAME");
    if ((kd = kvm_open (vmunix, NULL, NULL, O_RDONLY, "nettest_stat"))==NULL) 
    {
	send_message (-NAMELIST_ERROR, ERROR,
		 "Unable to read kernel VM %s", errmsg (errno));
    }
    else 
    {
	if (kvm_nlist (kd, nl) < 0) 
        {
	    send_message (-NAMELIST_ERROR, ERROR, "Bad namelist");
	}
	else 
      	{

	    /*
	     * Keep file descriptors open to avoid overhead of open/close on
	     * each call to get routines.
	     */

	    sethostent (1);
	    setnetent (1);
	}
    }
    TRACE_OUT
}


/******************************************************************************

  init_net_stat

  First time call to initialize the network statistics structure.  The status
  printing routines in this file utilize this structure, therefore, this must
  be the first call in every program for any network interface device.

******************************************************************************/
void	init_net_stat (devicenm)
char*	devicenm;
{
    int stat;

    func_name = "init_net_stat";
    TRACE_IN 
    bzero ((char *) &start_net_stat, sizeof (struct net_stat));
    get_dev_netstat (devicenm, &start_net_stat);
    TRACE_OUT
}


/******************************************************************************

                      kread

******************************************************************************/
int	kread (addr, buf, nbytes)
u_long	addr;
char*	buf;
u_int	nbytes;
{
    int		status;
    char*	f_name = "kread";

    if ((status = kvm_read (kd, addr, buf, nbytes)) < 0) {
	send_message (-NAMELIST_ERROR, ERROR, "%s: %s", f_name, errmsg (errno));
    }
    return (status);
}


/******************************************************************************

  netname

  Return the name of the network whose address is given.  The address is
  assumed to be that of a net or subnet, not a host.

******************************************************************************/
char*	netname (iaddr, mask)
u_long	iaddr;
u_long	mask;
{
    static char		line[50];
    struct in_addr	in;
    char*		cp = 0;

    func_name = "netname";
    TRACE_IN
    in.s_addr = ntohl (iaddr);
    if (in.s_addr) {
	struct	netent *np = 0;
	struct	hostent *hp;
	u_long	net;

	if (mask == 0) {
	    register u_long	i = in.s_addr;
	    int			subnetshift;

	    if (IN_CLASSA (i)) {
		mask = IN_CLASSA_NET;
		subnetshift = 8;
	    } else if (IN_CLASSB(i)) {
		mask = IN_CLASSB_NET;
		subnetshift = 8;
	    } else {
		mask = IN_CLASSC_NET;
		subnetshift = 4;
	    }

	    /*
	     * If there are more bits than the standard mask would suggest,
	     * subnets must be in use.  Guess at the subnet mask, assuming
	     * reasonable width subnet fields.
	     */

	    while (in.s_addr &~ mask) {
		mask = (long)mask >> subnetshift;
	    }
	}
	net = in.s_addr & mask;

	/*
	 * Right-justify the network number.
	 *
	 * This is a throw-back to the old conventions used in the kernel.  We
	 * now store it left-justified in the kernel, but still right-justified
	 * in the YP maps for backward compatibility.
	 */

	while ((mask & 1) == 0) {
	    mask >>= 1, net >>= 1;
	}
	np = getnetbyaddr (net, AF_INET);
	if (np) {
	    cp = np->n_name;
	}
	else {

	    /*
	     * gethostbyaddr takes network order; above wanted host order.
	     */

	    in.s_addr = iaddr;
	    hp = gethostbyaddr (&in,sizeof (struct in_addr),AF_INET);
	    if (hp) cp = hp->h_name;
	}
    }
    if (cp) {
	strcpy (line, cp);
    }
    else {
	in.s_addr = iaddr;
	strcpy (line, inet_ntoa (in));
    }
    TRACE_OUT
    return (line);
}


/******************************************************************************

  routename

  Returns a pointer to the specified IF routename.

******************************************************************************/
char*	routename (in)
struct	in_addr in;	/* in network order */
{
    char*		cp;
    static char		line[50];
    struct hostent*	hp;
    static char		domain[MAXHOSTNAMELEN + 1];
    static int		first = 1;

    func_name = "routename";
    TRACE_IN
    bzero (line, 50);
    if (first) {

	/*
	 * Record domain name for future reference.  Check first for the 4.3bsd
	 * convention of keeping it as * part of the hostname.  Failing that,
	 * try extracting it using the domainname system call.
	 */

	first = 0;
	if ((gethostname (domain, MAXHOSTNAMELEN) == 0) &&
	    (cp = index (domain, '.'))) {
	    (void) strcpy (domain, cp + 1);
	}
	else {
	    if (getdomainname (domain, MAXHOSTNAMELEN) < 0) {
		domain[0] = 0;
	    }
	}
    }
    cp = 0;
    hp = gethostbyaddr (&in, sizeof (struct in_addr), AF_INET);
    if (hp) {

	/*
	 * If the hostname contains a domain part, and it's the same as the
	 * local domain, elide it.
	 */

	if ((cp = index (hp->h_name, '.')) && (!strcmp (cp + 1, domain))) {
	    *cp = 0;
	}
	cp = hp->h_name;
    }
    if (cp) {
	strcpy (line, cp);
    }
    else {
	strcpy (line, inet_ntoa (in));
    }
    TRACE_OUT
    return (line);
}


/******************************************************************************

               get_dev_netstat

******************************************************************************/
int	get_dev_netstat (ifname, net_stat_ptr)
char*	ifname;
struct net_stat	*net_stat_ptr;
{
    struct ifnet	ifnet;
    union {
	struct ifaddr ifa;
	struct in_ifaddr in;
    } ifaddr;

    off_t	ifaddraddr;
    off_t	ifnetaddr;      /* value of this symbol (or sdb offset) */
    char	name[16];
   
    func_name = "get_dev_netstat";
    TRACE_IN
    if (kd > 0) {
        ifnetaddr = nl[0].n_value;
	kread (ifnetaddr, &ifnetaddr, sizeof ifnetaddr);
	ifaddraddr = 0;
	while (ifnetaddr || ifaddraddr) {
	    struct sockaddr_in *sin;
	    register char *cp;
	    char *index ();
	    int  n;
	    struct in_addr in, inet_makeaddr ();

	    /*
	     * No address list for the current interface: find the first
	     * address.
	     */

	    if (kread (ifnetaddr, &ifnet, sizeof ifnet) < 0) {
		break;
	    }
	    if (kread ((off_t)ifnet.if_name, name, 16) < 0) {
		break;
	    }
	    name[15] = '\0';
	    ifnetaddr = (off_t) ifnet.if_next;

	    /*
	     * Extend device name with unit number.
	     */

	    cp = index (name, '\0');
	    *cp++ = ifnet.if_unit + '0';
	    *cp = '\0';
	    if (ifname) {

		/*
		 * If a particular interface has been singled out, skip
		 * over all others.
		 */

		if (strcmp (ifname, name) != 0) {
		    continue;
		}
	    }
	    else {
		if ((ifnet.if_flags&IFF_UP) == 0) {

		    /*
		     * The interface is down: don't report on it unless it's
		     * been singled out or we're reporting everything.
		     */

		    *cp++ = '*';
		    *cp = '\0';
		}
		else {
		    if (ifaddr.ifa.ifa_addr.sa_family!= AF_INET) {
			continue;
		    }
		}
	    }
	    ifaddraddr = (off_t)ifnet.if_addrlist;

	    /*
	     * save interface name 
	     */

	    strcpy (net_stat_ptr->name, name);
	    kread (ifaddraddr, &ifaddr, sizeof ifaddr);
	    ifaddraddr = (off_t)ifaddr.ifa.ifa_next;
	    switch (ifaddr.ifa.ifa_addr.sa_family) {
		case AF_UNSPEC:
		    strcpy (net_stat_ptr->dstroutename, "none");
		    strcpy (net_stat_ptr->src_netaddr, "none");
		    break;

		case AF_INET:
		    if (ifnet.if_flags & IFF_POINTOPOINT) {
			sin = (struct sockaddr_in *) &ifaddr.in.ia_dstaddr;
			strcpy (net_stat_ptr->dstroutename, 
			       routename (sin->sin_addr));
		    }
		    else {
			strcpy (net_stat_ptr->dstroutename, 
			       netname (htonl (ifaddr.in.ia_subnet),
				       ifaddr.in.ia_subnetmask));
		    }
		    sin = (struct sockaddr_in *)&ifaddr.in.ia_addr;
		    strcpy (net_stat_ptr->src_netaddr,
			    routename (sin->sin_addr)); 
		    break;

		default:
		    cp = "????";
		    strcpy (net_stat_ptr->dstroutename, cp);
		    strcpy (net_stat_ptr->src_netaddr, cp);
		    break;
	    }
	    net_stat_ptr->if_ipackets = ifnet.if_ipackets;
	    net_stat_ptr->if_ierrors = ifnet.if_ierrors;
	    net_stat_ptr->if_opackets = ifnet.if_opackets;
	    net_stat_ptr->if_oerrors = ifnet.if_oerrors;
	    net_stat_ptr->if_collisions = ifnet.if_collisions;
	}
    }
    TRACE_OUT
    return ((int) kd);
}


/******************************************************************************

  check_point_status

  This module checkpoints the IF status for the specified IF.

  This module assumes that the 'init_net_stat' has been called at
  least once prior to the envocation of this module. 
  The results depend on  that first call to 'init_net_stat'.

******************************************************************************/
void	check_point_status (devnmp, rslt_net_statptr)
char*	devnmp;
struct net_stat *rslt_net_statptr;
{
    struct net_stat cur_net_stat;
 
    bzero ((char *) &cur_net_stat, sizeof (struct net_stat));
    bzero ((char *) rslt_net_statptr, sizeof (struct net_stat));
    if (get_dev_netstat (devnmp, &cur_net_stat) >= 0) {
	strcpy (rslt_net_statptr->name, cur_net_stat.name);
	strcpy (rslt_net_statptr->dstroutename, cur_net_stat.dstroutename);
	strcpy (rslt_net_statptr->src_netaddr, cur_net_stat.src_netaddr);
	rslt_net_statptr->if_ipackets = cur_net_stat.if_ipackets - 
	    start_net_stat.if_ipackets; 
	rslt_net_statptr->if_ierrors = cur_net_stat.if_ierrors -  
	    start_net_stat.if_ierrors; 
	rslt_net_statptr->if_opackets = cur_net_stat.if_opackets -  
	    start_net_stat.if_opackets; 
	rslt_net_statptr->if_oerrors = cur_net_stat.if_oerrors -  
	    start_net_stat.if_oerrors; 
	rslt_net_statptr->if_collisions =  cur_net_stat.if_collisions -  
	    start_net_stat.if_collisions; 
    }
}


/******************************************************************************

                 print_net_stats

******************************************************************************/
void	print_net_stats (net_statp)
struct net_stat *net_statp;
{

    func_name = "print_net_status";
    TRACE_IN
    if (net_statp->name[0] != '\0') {
	send_message (0, VERBOSE, FMT1, "Name", "Net/Dest", "Address", "Ipkts",
		      "Ierrs", "Opkts", "Oerrs", "Collis/RingDn",
		      net_statp->name, net_statp->dstroutename,
		      net_statp->src_netaddr, net_statp->if_ipackets,
		      net_statp->if_ierrors, net_statp->if_opackets,
		      net_statp->if_oerrors, net_statp->if_collisions);
    }
    TRACE_OUT
}

/******************************************************************************

                 check_status

******************************************************************************/
static	struct le_softc	le0, le1;
static	struct ie_softc	ie0, ie1;
static	struct tr_softc	tr0, tr1;
#ifndef sun386
static	struct fddi_status_struct	fddi0, fddi1;
#endif sun386

static	void	check_ie(ifname, how, warning)
char	*ifname;			/* device name */
int	how;
int     warning;      /* print warning messages only if "W" option specified */
{
  int	unit, offset;
  u_int	xmiturun, dogreset, trtry, tnocar, noCTS, runotready, notbds;
  u_int	ierr_crc, ierr_aln, ierr_rsc, ierr_ovr;
  char	buffer[512];

  func_name = "check_ie";
  TRACE_IN
  unit = atoi(ifname+2);
#ifdef sun386
  nl_driver[0].n_name = "ie_softc";
#else
  nl_driver[0].n_name = "_ie_softc";
#endif sun386
  if (kvm_nlist(kd, nl_driver) != 0)
    send_message (-NAMELIST_ERROR, ERROR, "\"_ie_softc\" not in the name list");
  offset = nl_driver[0].n_value + (unit * sizeof(struct ie_softc));

  if (how == 0)				/* first read */
    kvm_read(kd, offset, &ie0, sizeof(struct ie_softc));
  else					/* check for delta */
  {
    kvm_read(kd, offset, &ie1, sizeof(struct ie_softc));
    xmiturun   = ie1.es_xmiturun - ie0.es_xmiturun;
    dogreset   = ie1.es_dogreset - ie0.es_dogreset;
    trtry      = ie1.es_trtry    - ie0.es_trtry;
    tnocar     = ie1.es_tnocar   - ie0.es_tnocar;
    noCTS      = ie1.es_noCTS    - ie0.es_noCTS;
    runotready = ie1.es_runotready - ie0.es_runotready;
    notbds     = ie1.es_notbds   - ie0.es_notbds;
    ierr_crc   = ie1.es_ierr.crc - ie0.es_ierr.crc;
    ierr_aln   = ie1.es_ierr.aln - ie0.es_ierr.aln;
    ierr_rsc   = ie1.es_ierr.rsc - ie0.es_ierr.rsc;
    ierr_ovr   = ie1.es_ierr.ovr - ie0.es_ierr.ovr;
    if (warning)
    {
        if (xmiturun || dogreset || trtry || tnocar || noCTS || runotready || 
                     notbds || ierr_crc || ierr_aln || ierr_rsc || ierr_ovr)
        {
          strcpy(buffer, "driver errors detected:");
          if (xmiturun) sprintf(buffer, "%s es_xmiturun=%d", buffer, xmiturun);
          if (dogreset) sprintf(buffer, "%s es_dogreset=%d", buffer, dogreset);
          if (trtry)      sprintf(buffer, "%s es_trtry=%d", buffer, trtry);
          if (tnocar)     sprintf(buffer, "%s es_tnocar=%d", buffer, tnocar);
          if (noCTS)      sprintf(buffer, "%s es_noCTS=%d", buffer, noCTS);
          if (runotready) sprintf(buffer, "%s es_runotready=%d",
						buffer, runotready);
          if (notbds)   sprintf(buffer, "%s es_notbds=%d", buffer, notbds);
          if (ierr_crc) sprintf(buffer, "%s es_ierr.crc=%d", buffer, ierr_crc);
          if (ierr_aln) sprintf(buffer, "%s es_ierr.aln=%d", buffer, ierr_aln);
          if (ierr_rsc) sprintf(buffer, "%s es_ierr.rsc=%d", buffer, ierr_rsc);
          if (ierr_ovr) sprintf(buffer, "%s es_ierr.ovr=%d", buffer, ierr_ovr);
          strcat(buffer, ".");
          send_message(0, WARNING, buffer);
        }
    }
    send_message(0, VERBOSE, "Driver status: es_xmiturun= %d, es_dogreset= %d, es_trtry= %d, es_tnocar= %d, es_noCTS= %d, es_runotready= %d, es_notbds= %d, es_ierr.crc= %d, es_ierr.aln= %d, es_ierr.rsc= %d, es_ierr.ovr= %d.", xmiturun, dogreset, trtry, tnocar, noCTS, runotready, notbds, ierr_crc, ierr_aln, ierr_rsc, ierr_ovr);
  }
  TRACE_OUT
}

static	void	check_le(ifname, how, warning)
char	*ifname;			/* device name */
int	how;
int     warning;   /* print warning messages only if "W" option specified */
{
  int	unit, offset;
  u_int	fram, crc, oflo, uflo, tlcol, trtry, tnocar;
  char	buffer[512];
  char  command[80];
  FILE  *fp;

  func_name = "check_le";
  TRACE_IN
  unit = atoi(ifname+2);
  nl_driver[0].n_name = "_le_softc";
  if (kvm_nlist(kd, nl_driver) != 0)
    send_message (-NAMELIST_ERROR, ERROR, "\"_le_softc\" not in the name list");
  offset = nl_driver[0].n_value + (unit * sizeof(struct le_softc));

  if (how == 0)				/* first read */
  {
    if ((fp = fopen ("/tmp/sundiag_init_netstat", "w")) != NULL)
    {
        fprintf (fp, "\nNetstat before running nettest:\n");
        fseek (fp, 0, 2);       /* get to end of file */
        sprintf(command, "date >> /tmp/sundiag_init_netstat");
        system (command);
        sprintf(command, "netstat >> /tmp/sundiag_init_netstat");
        system (command);
        fclose(fp);
    }
    else
    {
        send_message (-FILE_OPEN_ERROR, ERROR, "Cannot open /tmp/sundiag_init_netstat");
    }
    kvm_read(kd, offset, &le0, sizeof(struct le_softc));
  }

  else					/* check for delta */
  {
    /*upon successful completion of nettest, remove /tmp/sundiag_init_netstat*/
    if ((fp = fopen ("/tmp/sundiag_init_netstat", "r")) != NULL)
    {   
	sprintf (command, "/bin/rm /tmp/sundiag_init_netstat");
	system (command);
	fclose(fp);
    }
    else
    {   
        send_message (FILE_OPEN_ERROR, WARNING, "Cannot remove /tmp/sundiag_init_netstat");
    }

    kvm_read(kd, offset, &le1, sizeof(struct le_softc));
    fram   = le1.es_fram   - le0.es_fram;
    crc    = le1.es_crc    - le0.es_crc;
    oflo   = le1.es_oflo   - le0.es_oflo;
    uflo   = le1.es_uflo   - le0.es_uflo;
    tlcol  = le1.es_tlcol  - le0.es_tlcol;
    trtry  = le1.es_trtry  - le0.es_trtry;
    tnocar = le1.es_tnocar - le0.es_tnocar;

    if (warning) 
    {
        if (fram || crc || oflo || uflo || tlcol || trtry || tnocar)
        {
          strcpy(buffer, "Driver errors detected:");
          if (fram)   sprintf(buffer, "%s es_fram=%d",   buffer, fram);
          if (crc)    sprintf(buffer, "%s es_crc=%d",    buffer, crc);
          if (oflo)   sprintf(buffer, "%s es_oflo=%d",   buffer, oflo);
          if (uflo)   sprintf(buffer, "%s es_uflo=%d",   buffer, uflo);
          if (tlcol)  sprintf(buffer, "%s es_tlcol=%d",  buffer, tlcol);
          if (trtry)  sprintf(buffer, "%s es_trtry=%d",  buffer, trtry);
          if (tnocar) sprintf(buffer, "%s es_tnocar=%d", buffer, tnocar);
          strcat(buffer, ".");
          send_message(0, WARNING, buffer);
        }
    }

    send_message(0, VERBOSE, "Driver status: es_fram=%d, es_crc=%d, es_oflo=%d, es_uflo=%d, es_tlcol=%d, es_trtry=%d, es_tnocar=%d.", fram, crc, oflo, uflo, tlcol, trtry, tnocar);
  }
  TRACE_OUT
}


static	void	check_tr(ifname, how, warning, tr_driver_error)
char	*ifname;			/* device name */
int	how;
int     warning;   /* print warning messages only if "W" option specified */
int     *tr_driver_error;
{
  int	unit, offset;
  int   i, error_type, s, driver_error;
  struct ifreq  ifr;
  u_int linerr, bsterr, fcierr, lostfrm, recvcong, frmcpy, token, dmabus;
  u_int dmapar, rngdwn ;
  char	buffer[512];

  func_name = "check_tr";
  TRACE_IN
  unit = atoi(ifname+2);
  nl_driver[0].n_name = "_Tr_softc";
  if (kvm_nlist(kd, nl_driver) != 0)
    send_message (-NAMELIST_ERROR, ERROR, "\"_Tr_softc\" not in the name list");
  offset = nl_driver[0].n_value + (unit * sizeof(struct tr_softc));

  if ((s=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    send_message(-NO_SOCKET, FATAL, "Can't open a socket in check_tr().");
  strncpy (ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
  ifr.ifr_ifru.ifru_fddi_gstruct.ifru_fddi_gioctl = TR_CHKSTS;

  if (how == 0)				/* first read */
  {
      kvm_read(kd, offset, &tr0, sizeof(struct tr_softc));  /* get error cnts*/
	
      /* this first ioctl call to is to initialize the sundiag status */
      /* flags in the token ring driver */
      if ((driver_error = ioctl(s, SIOCFDGIOCTL, (caddr_t)&ifr)) == EINVAL)
          send_message(-IOCTL_ERROR, ERROR, "ioctl() failed in check_tr().");
  }
  else
  {
      /* get sundiag status flags in token ring driver
         Peter Tan has provided these hooks into the driver
         via ioctl call in order catch dma aborts and to make
         nettest for token more deterministic.
      */

      if ((driver_error = ioctl(s, SIOCFDGIOCTL, (caddr_t)&ifr)) != EINVAL)
      {
        if (driver_error == 0)
        {
                send_message (0, DEBUG, "No Driver Errors.");
                *tr_driver_error = FALSE;
        }
        else
        {   
            error_type = 1;
            for (i=0; i<17; i++)
            {
                if ((driver_error & error_type) >> i)
                {
                    switch (error_type)
                    {
                        case CAB_WIRE_FLT:
                            send_message(TR_CABLE_ERROR, ERROR, "Token Ring Cable error.  Please check cables and try testing again.");
                            break;
                        case NET_SIG_LOSS:
                        case NET_LINE_ERR:
                        case NET_BST_ERR:
                        case NET_FCI_ERR:
                        case NET_LOST_FRM:
                        case NET_TKN_ERR:
                        case NET_RMV_RECV:
                            send_message(TR_NET_ERROR, ERROR, "Network Error code %d, please check the network and try testing again.", error_type);
                            break;
                        case BRD_BEACON_AUTO_RMV:
                        case BRD_DIO_PAR:
                        case BRD_RNG_URUN:
                        case BRD_RNG_ORUN:
                        case BRD_UNREC_INTINTR:
                        case BRD_OTHER_REASONS:
                            send_message(TR_BOARD_ERROR, ERROR, "Token Ring Board Error code %d.", error_type);
                            break;
                        case BRD_DMA_RD_ABORT:
                            send_message(TR_BOARD_ERROR, ERROR, "Token Ring Board Error code %d.  DMA read abort.", error_type);
                            break;

                        default:
                            send_message(TR_BOARD_ERROR, ERROR, "Token Ring Error code %d.", error_type);

                    }  /* switch (driver_error) */
                }
                error_type = error_type*2;
            }
            send_message(-TR_BOARD_ERROR, ERROR, "Exiting w/ Token Ring Error");
        }
      }  
      else
          send_message(-IOCTL_ERROR, ERROR, "ioctl() failed in check_tr().");

      /* check for delda */
      kvm_read(kd, offset, &tr1, sizeof(struct tr_softc));
      linerr   = tr1.tr_linerr   - tr0.tr_linerr;
      bsterr   = tr1.tr_bsterr   - tr0.tr_bsterr;
      fcierr   = tr1.tr_fcierr   - tr0.tr_fcierr;
      lostfrm  = tr1.tr_lostfrm  - tr0.tr_lostfrm;
      recvcong = tr1.tr_recvcong - tr0.tr_recvcong;
      frmcpy   = tr1.tr_frmcpy   - tr0.tr_frmcpy;
      token    = tr1.tr_token    - tr0.tr_token;
      dmabus   = tr1.tr_dmabus   - tr0.tr_dmabus;
      dmapar   = tr1.tr_dmapar   - tr0.tr_dmapar;
      rngdwn   = tr1.tr_rngdwn   - tr0.tr_rngdwn;

      if (warning) 
      {
        if (linerr || bsterr || fcierr || lostfrm || recvcong || frmcpy 
                     || token || dmabus || dmapar || rngdwn)
        {
          strcpy(buffer, "Driver errors detected:");
          if (linerr)   sprintf(buffer, "%s tr_linerr=%d", buffer, linerr);
          if (bsterr)   sprintf(buffer, "%s tr_bsterr=%d", buffer, bsterr);
          if (fcierr)   sprintf(buffer, "%s tr_fcierr=%d", buffer, fcierr);
          if (lostfrm)  sprintf(buffer, "%s tr_lostfrm=%d", buffer, lostfrm);
          if (recvcong) sprintf(buffer, "%s tr_recvcong=%d", buffer, recvcong);
          if (frmcpy)   sprintf(buffer, "%s tr_frmcpy=%d", buffer, frmcpy);
          if (token)    sprintf(buffer, "%s tr_token=%d", buffer, token);
          if (dmabus)   sprintf(buffer, "%s tr_dmabus=%d", buffer, dmabus);
          if (dmapar)   sprintf(buffer, "%s tr_dmapar=%d", buffer, dmapar);
          if (rngdwn)   sprintf(buffer, "%s tr_rngdwn=%d", buffer, rngdwn);
          strcat(buffer, ".");
          send_message(0, WARNING, buffer);
        }
      }
      send_message(0, VERBOSE, "Driver status: tr_linerr=%d, tr_bsterr=%d, tr_fcierr=%d, tr_lostfrm=%d, tr_recvcong=%d, tr_frmcpy=%d, tr_token=%d, tr_dmabus=%d, tr_dmapar=%d, tr_rngdwn=%d.", linerr, bsterr, fcierr, lostfrm, recvcong, frmcpy, token, dmabus, dmapar, rngdwn);
  }
  TRACE_OUT
}

#ifndef sun386
static	void	check_fddi(ifname, how, warning)
char	*ifname;			/* device name */
int	how;
int	warning;    /* print warning messages only if "W" option's specified */
{
  struct ifreq	ifr;
  int	s;
  ulong_t ring_toggles, receive_full, receive_error, transmit_abort;
  char	buffer[512];

  func_name = "check_fddi";
  TRACE_IN
  if ((s=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    send_message(-NO_SOCKET, FATAL, "Can't open a socket in ckeck_fddi().");
  strncpy (ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
  ifr.ifr_fddi_stat.stat_size = sizeof(struct fddi_status_struct);
  if (how == 0)
  {
    ifr.ifr_fddi_stat.fddi_stats = (caddr_t)&fddi0;
    if (ioctl(s, SIOCGETFDSTAT, (caddr_t)&ifr) < 0)
      send_message(-IOCTL_ERROR, ERROR, "ioctl() failed in ckeck_fddi().");
  }
  else
  {
    ifr.ifr_fddi_stat.fddi_stats = (caddr_t)&fddi1;
    if (ioctl(s, SIOCGETFDSTAT, (caddr_t)&ifr) < 0)
      send_message(-IOCTL_ERROR, ERROR, "ioctl() failed in ckeck_fddi().");

    ring_toggles   = fddi1.ring_toggles - fddi0.ring_toggles;
    receive_full   = fddi1.receive_full - fddi0.receive_full;
    receive_error  = fddi1.receive_error - fddi0.receive_error;
    transmit_abort = fddi1.transmit_abort - fddi0.transmit_abort;

    if (warning) 
    {
        if (ring_toggles || receive_full || receive_error || transmit_abort)
        {
          strcpy(buffer, "Driver errors detected:");
          if (ring_toggles)   sprintf(buffer, "%s ring_toggles=%d",
						buffer, ring_toggles);
          if (receive_full)   sprintf(buffer, "%s receive_full=%d",
						buffer, receive_full);
          if (receive_error)  sprintf(buffer, "%s receive_error=%d",
						buffer, receive_error);
          if (transmit_abort) sprintf(buffer, "%s transmit_abort=%d",
						buffer, transmit_abort);
          strcat(buffer, ".");
          send_message(0, WARNING, buffer);
        }
    }

    send_message(0, VERBOSE, "Driver status: ring_toggles=%d, receive_full=%d, receive_error=%d, transmit_abort=%d.", ring_toggles, receive_full, receive_error, transmit_abort);
    close(s);
  }
  TRACE_OUT
}
#endif sun386

void	check_status(ifname, how, warning, tr_driver_error)
char	*ifname;			/* device name */
int	how;
int     warning;	/* print warning onyl if "W" option specified */
int     *tr_driver_error;
{

    func_name = "check_status";
    TRACE_IN
    if (strncmp(ifname, "ie", 2) == 0) check_ie(ifname, how, warning);
	/* ie devices */
    else if (strncmp(ifname, "le", 2) == 0) check_le(ifname, how, warning);
	/* le devices */
    else if (strncmp(ifname, "tr", 2) == 0) check_tr(ifname, how, warning, tr_driver_error);
	/* tr devices */
#ifndef sun386
    else if (strncmp(ifname, "fddi", 4) == 0) check_fddi(ifname, how, warning);
	/* fddi devices */
#endif sun386
    else if (strncmp(ifname, "ne", 2) == 0)
        send_message (0, DEBUG, "No status check for ne devices.");
        /* OMNI devices */
    else
	send_message(-NO_NETWORKS, ERROR,
		"Unknown network interface %s", ifname);
    TRACE_OUT
}
