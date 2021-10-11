/* @(#)tftp.c 1.1 92/07/30 SMI */

/*
 * Bare-bones tftp implementation for sun2's with ND in proms
 */
#include "../lib/common/saio.h" 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/ip_var.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include "../lib/common/sainet.h" 
#include <arpa/tftp.h>

#define MAXNAMELEN 8	/* 8 bytes of internet address in hex */

#define UDP_OFFSET \
	(sizeof(struct udphdr)+sizeof(struct ip)+sizeof(struct ether_header))
#define MAXSENDSIZE	(4 + MAXNAMELEN + 1 + sizeof(mode))
#define MAXRECVSIZE	(4 + SEGSIZE)
#define millitime()	(*romp->v_nmiclock)
#define flipit()	putchar(flip[flipc++ % 4]); putchar('\b')

extern int random;

char mode[] = "octet";		/* transfer mode is always octet */
char flip[] = "-\\|/";		/* emulates what the proms do */
int flipc = 0;			/* current location in flip */
int sport = -1;			/* source port */
char sendpacket[UDP_OFFSET + MAXSENDSIZE];
char recvpacket[ETHERMTU];

/*
 * Use tftp to read in a file
 */
tftpread(rhost, rether, name, mem, timeout, retries)
	struct sockaddr_in rhost;
	struct ether_addr rether;
	char *name;
	char *mem;	
	int timeout;	/* timeout per request */
	int retries;	/* number of retries before giving up */
{
	int sendsize;
	int recvsize;
	struct tftphdr *sendth;
	struct tftphdr *recvth;
	int lastblock;
	int amount;
	int nrecv;
	int stoprecv;
	int eof;
	int gotblock;
	int tries;
	struct sockaddr_in thost;
	struct ether_addr tether;

	recvth = (struct tftphdr *) (recvpacket + UDP_OFFSET);
	sendth = (struct tftphdr *) (sendpacket + UDP_OFFSET);
	eof = 0;
	tries = 0;
	if (sport >= 0) {
		sendth->th_opcode = htons(ACK);
		sendth->th_block = 1;
		sendsize = 4;
		amount = SEGSIZE;
		lastblock = 1;
	} else {
		sport = random++ & 0xffff;
		amount = 0;
		lastblock = 0;
		rhost.sin_port = IPPORT_TFTP;
		sendsize = makereq(sendth, name);
	}
	
	while ((! eof) && (tries++ < retries)) {

		(void) udp_output(&rhost, &rether, (u_short) sport, sendpacket,
			sendsize);
		flipit();
		gotblock = 0;
		stoprecv = millitime() + timeout;

		while ((! gotblock) && (millitime() < stoprecv)) {
			recvsize = MAXRECVSIZE;
			nrecv = udp_input(&thost, &tether, (u_short) sport, 
				recvpacket, recvsize);
			if (thost.sin_addr.s_addr == rhost.sin_addr.s_addr && 
				nrecv >= 4)
			{
				switch (ntohs(recvth->th_opcode)) {
				case DATA:
					if (ntohs(recvth->th_block) == 
						lastblock + 1) 
					{
						gotblock = 1;
					}
					break;
				case ERROR:
					return(-1);
				}
			}
		}

		if (gotblock) {
			flipit();
			recvsize = nrecv - 4;
			bcopy(recvth->th_data, mem + amount, recvsize);
			amount += recvsize;
			if (recvsize == SEGSIZE) {
				sendth->th_opcode = htons(ACK);
				sendth->th_block = recvth->th_block;
				lastblock = ntohs(recvth->th_block);
				sendsize = 4;
				tries = 0;
			} else {
				eof = 1;
			}
			rhost.sin_port = thost.sin_port;
		} 
	}
	if (eof) {
		printf("Downloaded %d bytes from tftp server.\n", amount);
	}
	return(eof ? amount : 0);
}


/*
 * Find a tftp server by broadcasting
 * Read in the first block, if one is received
 */
tftpfind(rhost, rether, name, mem, timeout)
	struct sockaddr_in *rhost;
	struct ether_addr *rether;
	char *name;
	char *mem;
	int timeout;
{
	struct tftphdr *sendth;
	struct tftphdr *recvth;
	struct sockaddr_in thost;
	struct ether_addr tether;
	extern struct sainet ether_sain;
	extern struct ether_addr etherbroadcastaddr;
	int nrecv;
	int sendsize;
	int stoprecv;
	int recvsize;

	recvth = (struct tftphdr *) (recvpacket + UDP_OFFSET);
	sendth = (struct tftphdr *) (sendpacket + UDP_OFFSET);
	sendsize = makereq(sendth, name);
	sport = random++ & 0xffff;

	thost.sin_port = IPPORT_TFTP;
	thost.sin_addr.s_addr = ~0;
	tether = etherbroadcastaddr;
	(void) udp_output(&thost, &tether, (u_short) sport, sendpacket, 
		sendsize);
	flipit();
	stoprecv = millitime() + timeout;
	while (millitime() < stoprecv) {
		nrecv = udp_input(rhost, rether, (u_short) sport, 
			recvpacket, MAXRECVSIZE);
		if (nrecv >= 4  && ntohs(recvth->th_opcode) == DATA 
			&& ntohs(recvth->th_block) == 1) 
		{
			flipit();
			recvsize = nrecv - 4;
			bcopy(recvth->th_data, mem, recvsize);
			return(recvsize != SEGSIZE);	
		}
	}
	return(-1);	
}

char *
strput(dst, src)
	char *dst;
	char *src;
{
	while (*dst++ = *src++)
		;
	return(dst);
}

makereq(sendth, name)
	struct tftphdr *sendth;
	char *name;
{
	char *cp;

	sendth->th_opcode = htons(RRQ);
	cp = strput(sendth->th_stuff, name);
	cp = strput(cp, mode);
	return(sizeof(sendth->th_opcode) + (cp - sendth->th_stuff));
}
