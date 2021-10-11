/*
 * Operating System interface to the "Etherfind" program.
 * Code in this module should be NOT specific to a particlar protocol.
 *
 * @(#)nit_streams.c 1.1 92/07/30
 *
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <net/if.h>

#include <sys/stropts.h>

#include <net/nit_if.h>
#include <net/nit_buf.h>

#include "etherfind.h"

#define	NIT_DEV		"/dev/nit"

/*
 * We use this value for the chunk size when grabbing all
 * of every packet.  Otherwise, we base chunk size on snaplen.
 */
#define	CHUNKSIZE	8192
#define MINCHUNK	1500		/* grab at least this much */

/*
 * SNAPLEN gives the maximum amount of any packet that we
 * need see, unless the -v, -l or -r flag has been specified.
 */
#define	SNAPLEN		(44)

int	if_fd = -1;
u_int	chunksize;
int printlength;		/* bytes to print with x flag */
int snaplen = SNAPLEN;

extern char *strncpy();
extern char *malloc();

main_loop(device)
    char	*device;
{
    u_long if_flags = 0;
    int cc;
    char *buf;

	/*
	 * Calculate snapshot length to use based on which headers we're
	 * to dig information from.  A value of zero means to return
	 * everything.  If we set snaplen nonzero, we must arrange to
	 * get the nit_if-level nit_iflen header as well, to get the
	 * original packet's length.
	 */
	snaplen = SNAPLEN;
	if (rpcflag[0] || symflag[0] || (xflag[0] && printlength==0)) {
		/*
		 * Need Ethernet header, IP header, TCP/UDP header, and
		 * RPC header.  But: for now, just punt and grab everything.
		 */
		snaplen = 0;
	}
	else if (printlength > snaplen)
		snaplen = printlength;

	/*
	 * Calculate a chunk size appropriate for the snapshot length
	 * at hand.  The values used here are heuristics.  The chunk
	 * size is used to determine how much buffer space we allocate
	 * and therfore must be large enough to hold the largest single
	 * message the buffering module might hand us.  To be safe, we
	 * force it to be at least the maximum Ethernet packet length.
	 */
	if (snaplen == 0)
		chunksize = CHUNKSIZE;
	else
		chunksize = 80 * snaplen;
	if (chunksize < MINCHUNK)
		chunksize = MINCHUNK;

	/*
	 * Use malloc to obtain input buffer, so that the buffer
	 * is aligned correctly for structures cast over its start.
	 * The buffer size depends on the snapshot length in effect.
	 */
	if ((buf = malloc(chunksize)) == NULL) {
		fprintf(stderr, "etherfind: couldn't allocate buffer space\n");
		exit(1);
	}


	if (dflag)
		if_flags |= NI_DROPS;
	if (!pflag)
		if_flags |= NI_PROMISC;
	if (tflag)
		if_flags |= NI_TIMESTAMP;
	if (snaplen > 0)
		if_flags |= NI_LEN;
	initdevice(device, if_flags, snaplen);

	while ((cc = read(if_fd, buf, (int)chunksize)) >= 0) {
		register char	*bp = buf,
				*bufstop = buf + cc;

#ifdef	DEBUG
		(void) fflush (stdout);
		fprintf(stderr, "==> %d\n", cc);
		(void) fflush(stderr);
#endif	DEBUG

		/*
		 * Loop through each message in the chunk.
		 */
		while (bp < bufstop) {
			register char		*cp = bp;
			struct nit_bufhdr	*hdrp;
			struct timeval		*tvp = NULL;
			u_long			drops = 0;
			u_long			pktlen;

			/*
			 * Extract information from the successive objects
			 * embedded in the current message.  Which ones we
			 * have depends on how we set up the stream (and
			 * therefore on what command line flags were set).
			 *
			 * If snaplen is positive then the packet was truncated
			 * before the buffering module saw it, so we must
			 * obtain its length from the nit_if-level nit_iflen
			 * header.  Otherwise the value in *hdrp suffices.
			 */
			hdrp = (struct nit_bufhdr *)cp;
			cp += sizeof *hdrp;
			if (tflag) {
				struct nit_iftime	*ntp;

				ntp = (struct nit_iftime *)cp;
				cp += sizeof *ntp;

				tvp = &ntp->nh_timestamp;
			}
			if (dflag) {
				struct nit_ifdrops	*ndp;

				ndp = (struct nit_ifdrops *)cp;
				cp += sizeof *ndp;

				drops = ndp->nh_drops;
			}
			if (snaplen > 0) {
				struct nit_iflen	*nlp;

				nlp = (struct nit_iflen *)cp;
				cp += sizeof *nlp;

				pktlen = nlp->nh_pktlen;
			}
			else
				pktlen = hdrp->nhb_msglen;

			bp += hdrp->nhb_totlen;

			sp_ts_len = pktlen;

			/*
			 * Process the packet.
			 * (now done in separate module)
			 */
			filter(0, cp, sp_ts_len, tvp, drops);
		}
	}
	perror("etherfind read error");
	exit(-1);
}

initdevice(device, if_flags, snaplen)
	char *device;
	u_long	if_flags,
		snaplen;
{
	struct strioctl	si;
	struct ifreq	ifr;
	struct timeval	timeout;
	
	if (if_fd >= 0)
		return;		/* already open */

	if ((if_fd = open(NIT_DEV, O_RDONLY)) < 0) {
		perror("nit open");
		exit (1);
	}

	/*
	 * Arrange to get discrete messages from the stream.
	 */
	if (ioctl(if_fd, I_SRDOPT, (char *)RMSGD) < 0) {
		perror("ioctl (I_SRDOPT)");
		exit (1);
	}

	si.ic_timout = INFTIM;

	/*
	 * Push and configure the buffering module.
	 */
	if (ioctl(if_fd, I_PUSH, "nbuf") < 0) {
		perror("ioctl (I_PUSH \"nbuf\")");
		exit (1);
	}

	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	si.ic_cmd = NIOCSTIME;
	si.ic_len = sizeof timeout;
	si.ic_dp = (char *)&timeout;
	if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCSTIME)");
		exit (1);
	}

	si.ic_cmd = NIOCSCHUNK;
	si.ic_len = sizeof chunksize;
	si.ic_dp = (char *)&chunksize;
	if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCSCHUNK)");
		exit (1);
	}

	/*
	 * Configure the nit device, binding it to the proper
	 * underlying interface, setting the snapshot length,
	 * and setting nit_if-level flags.
	 */
	(void) strncpy(ifr.ifr_name, device, sizeof ifr.ifr_name);
	ifr.ifr_name[sizeof ifr.ifr_name - 1] = '\0';
	si.ic_cmd = NIOCBIND;
	si.ic_len = sizeof ifr;
	si.ic_dp = (char *)&ifr;
	if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
		perror("ioctl (I_STR: NIOCBIND)");
		exit(1);
	}

	if (snaplen > 0) {
		si.ic_cmd = NIOCSSNAP;
		si.ic_len = sizeof snaplen;
		si.ic_dp = (char *)&snaplen;
		if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
			perror("ioctl (I_STR: NIOCSSNAP)");
			exit (1);
		}
	}

	if (if_flags != 0) {
		si.ic_cmd = NIOCSFLAGS;
		si.ic_len = sizeof if_flags;
		si.ic_dp = (char *)&if_flags;
		if (ioctl(if_fd, I_STR, (char *)&si) < 0) {
			perror("ioctl (I_STR: NIOCSFLAGS)");
			exit (1);
		}
	}

	/*
	 * Flush the read queue, to get rid of anything that
	 * accumulated before the device reached its final configuration.
	 */
	if (ioctl(if_fd, I_FLUSH, (char *)FLUSHR) < 0) {
		perror("ioctl (I_FLUSH)");
		exit(1);
	}
}

