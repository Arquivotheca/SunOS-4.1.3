/* @(#)bootnd.c 1.1 92/07/30 SMI */

/*
 * Boot block for machine with ND in proms.  Does tftp to load
 * next stage of boot.
 */
#include "../lib/common/saio.h"
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include "../lib/common/sainet.h"
#include <mon/bootparam.h>

#define TIMEOUT 4000	/* retransmission timeout, in milliseconds */
#define RETRIES 15	/* number of times to retransmit, before giving up */
#define MAXTIMEOUT 64000	/* max time to wait for reply to broadcast */

main()
{
	struct bootparam *bp;
	struct boottab b;
	static char fname[] = "01234567.SUN2";
	struct sockaddr_in raddr;
	struct ether_addr rether;
	int timeout;
	int eof;
	extern int random; 
	extern struct saioreq ether_si;
	extern struct sainet ether_sain;
	extern char recvpacket[];

	if (romp->v_bootparam == 0) {
		bp = (struct bootparam *)0x2000;	/* Old Sun-1 address */
	} else {
		bp = *(romp->v_bootparam);		/* S-2: via romvec */
	}

	/*
	 * This is necessary to get the prom ethernet driver to work
	 */
	b = *bp->bp_boottab;
	bp->bp_boottab = &b;
	bp->bp_boottab->b_devinfo = 0;

	random = *romp->v_nmiclock;
	if (etheropen(bp, &ether_si) < 0) {
		printf("can't open ethernet\n");
		return;
	}
	inet_init(&ether_si, &ether_sain, recvpacket);

	printf("Using IP Address "); 
	printaddr(ether_sain.sain_myaddr);

	ultox(fname, ether_sain.sain_myaddr.s_addr);
	raddr.sin_addr = ether_sain.sain_hisaddr;	
	rether = ether_sain.sain_hisether;
	eof = 0;
	printf("Booting from tftp server at ");
	printaddr(raddr.sin_addr);
	while (! eof && tftpread(raddr, rether, fname, (char *)LOADADDR, 
		TIMEOUT, RETRIES) <= 0)
	{
		timeout = TIMEOUT;
		while ((eof = tftpfind(&raddr, &rether, fname, 
			(char *)LOADADDR, timeout)) < 0) 
		{
			if (timeout < MAXTIMEOUT) {
				timeout *= 2;	/* backoff */
			}
			printf("tftp: timeout\n");
		}
		printf("Booting from tftp server at ");
		printaddr(raddr.sin_addr);
	}

	_exitto(LOADADDR);
}


/*
 * Print an internet address
 */
printaddr(addr)
	struct in_addr addr;
{
	printf("%d.%d.%d.%d = %x.\n", addr.s_net, addr.s_host, addr.s_lh,
		addr.s_impno, ntohl(addr.s_addr));
}


/*
 * Unsigned long to hex conversion
 */
ultox(buf, val)
	char *buf;
	unsigned long val;	
{
	int i;
	static char hex[] = "0123456789ABCDEF";

	for (i = 7; i >= 0; i--) {
		buf[i] = hex[val & 0x0f];
		val >>= 4;
	}
}


/*
 * Open the ethernet driver in proms
 */
etheropen(bp, si)
	struct bootparam *bp;
	struct saioreq *si;
{
	si->si_boottab = bp->bp_boottab;
	si->si_ctlr = bp->bp_ctlr;
	si->si_unit = bp->bp_unit;
	si->si_boff = bp->bp_part;
	si->si_cc = 0;
	si->si_offset = 0;
	si->si_flgs = 0;
	si->si_devaddr = si->si_devdata = si->si_dmaaddr = 0;
	return((si->si_boottab->b_open)(si));
}

/*
 * Nobody calls this, but must resolve reference in srt0
 */
exit()
{
	_exit();
}

