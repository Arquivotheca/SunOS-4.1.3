#ifndef lint
static char sccsid[] = "@(#)trpt.c 1.1 92/07/30 SMI";	/* from 5.2 (Berkeley) 9/18/85 */
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#define PRUREQUESTS
#include <sys/protosw.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#define	TCPTIMERS
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#define	TANAMES
#include <netinet/tcp_debug.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <nlist.h>
#include <kvm.h>

n_time	ntime;
int	sflag;
int	tflag;
int	jflag;
int	aflag;
int	numeric();
struct	nlist nl[] = {
	{ "_tcp_debug" },
	{ "_tcp_debx" },
	0
};
struct	tcp_debug tcp_debug[TCP_NDEBUG];
caddr_t	tcp_pcbs[TCP_NDEBUG];
int	tcp_debx;
char	*ntoa();

main(argc, argv)
	int argc;
	char **argv;
{
	int i, npcbs = 0;
	char *system = NULL, *core = NULL;
	kvm_t *kd;

	argc--, argv++;
again:
	if (argc > 0 && !strcmp(*argv, "-a")) {
		aflag++, argc--, argv++;
		goto again;
	}
	if (argc > 0 && !strcmp(*argv, "-s")) {
		sflag++, argc--, argv++;
		goto again;
	}
	if (argc > 0 && !strcmp(*argv, "-t")) {
		tflag++, argc--, argv++;
		goto again;
	}
	if (argc > 0 && !strcmp(*argv, "-j")) {
		jflag++, argc--, argv++;
		goto again;
	}
	if (argc > 0 && !strcmp(*argv, "-p")) {
		argc--, argv++;
		if (argc < 1) {
			fprintf(stderr, "-p: missing tcpcb address\n");
			exit(1);
		}
		if (npcbs >= TCP_NDEBUG) {
			fprintf(stderr, "-p: too many pcb's specified\n");
			exit(1);
		}
		sscanf(*argv, "%x", &tcp_pcbs[npcbs++]);
		argc--, argv++;
		goto again;
	}
	if (argc > 0) {
		system = *argv;
		argc--, argv++;
	}
	if (argc > 0) {
		core = *argv;
		argc--, argv++;
	}
	if ((kd = kvm_open(system, core, NULL, O_RDONLY, argv[0])) == NULL)
		exit(1);
	if (kvm_nlist(kd, nl) != 0) {
		fprintf(stderr, "trpt: no namelist\n");
		exit(1);
	}
	if (kvm_read(kd, nl[1].n_value, &tcp_debx, sizeof (tcp_debx)) != sizeof (tcp_debx)) {
		fprintf(stderr, "trpt: kernel read error (tcp_debx)\n");
		exit(3);
	}
	printf("tcp_debx=%d\n", tcp_debx);
	if (kvm_read(kd, nl[0].n_value, tcp_debug, sizeof (tcp_debug)) != sizeof (tcp_debug)) {
		fprintf(stderr, "trpt: kernel read error (tcp_debug)\n");
		exit(3);
	}
	kvm_close(kd);
	/*
	 * If no control blocks have been specified, figure
	 * out how many distinct one we have and summarize
	 * them in tcp_pcbs for sorting the trace records
	 * below.
	 */
	if (npcbs == 0) {
		for (i = 0; i < TCP_NDEBUG; i++) {
			register int j;
			register struct tcp_debug *td = &tcp_debug[i];

			if (td->td_tcb == 0)
				continue;
			for (j = 0; j < npcbs; j++)
				if (tcp_pcbs[j] == td->td_tcb)
					break;
			if (j >= npcbs)
				tcp_pcbs[npcbs++] = td->td_tcb;
		}
	}
	qsort(tcp_pcbs, npcbs, sizeof (caddr_t), numeric);
	if (jflag) {
		char *cp = "";

		for (i = 0; i < npcbs; i++) {
			printf("%s%x", cp, tcp_pcbs[i]);
			cp = ", ";
		}
		if (*cp)
			putchar('\n');
		exit(0);
	}
	for (i = 0; i < npcbs; i++) {
		printf("\n%x:\n", tcp_pcbs[i]);
		dotrace(tcp_pcbs[i]);
	}
	exit(0);
	/* NOTREACHED */
}

dotrace(tcpcb)
	register caddr_t tcpcb;
{
	register int i;
	register struct tcp_debug *td;

	for (i = 0; i < tcp_debx % TCP_NDEBUG; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, td->td_tcb, &td->td_cb,
		    &td->td_ti, td->td_req);
	}
	for (i = tcp_debx % TCP_NDEBUG; i < TCP_NDEBUG; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, td->td_tcb, &td->td_cb,
		    &td->td_ti, td->td_req);
	}
}

/*
 * Tcp debug routines
 */
tcp_trace(act, ostate, atp, tp, ti, req)
	short act, ostate;
	struct tcpcb *atp, *tp;
	struct tcpiphdr *ti;
	int req;
{
	tcp_seq seq, ack;
	int len, flags, win, timer;
	char *cp;

	ptime(ntime);
	printf("%s:%s ", tcpstates[ostate], tanames[act]);
	switch (act) {

	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (aflag) {
			printf("(src=%s,%d, ", ntoa(ti->ti_src),
				ntohs(ti->ti_sport));
			printf("dst=%s,%d)", ntoa(ti->ti_dst),
				ntohs(ti->ti_dport));
		}
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		win = ti->ti_win;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs(len);
			win = ntohs(win);
		}
		if (act == TA_OUTPUT)
			len -= sizeof (struct tcphdr);
		if (len)
			printf("[%x..%x)", seq, seq+len);
		else
			printf("%x", seq);
		printf("@%x", ack);
		if (win)
			printf("(win=%x)", win);
		flags = ti->ti_flags;
		if (flags) {
			char *cp = "<";
#define pf(f) { if (ti->ti_flags&TH_/**/f) { printf("%s%s", cp, "f"); cp = ","; } }
			pf(SYN); pf(ACK); pf(FIN); pf(RST); pf(PUSH); pf(URG);
			printf(">");
		}
		break;

	case TA_USER:
		timer = req >> 8;
		req &= 0xff;
		printf("%s", prurequests[req]);
		if (req == PRU_SLOWTIMO || req == PRU_FASTTIMO)
			printf("<%s>", tcptimers[timer]);
		break;
	}
	printf(" -> %s", tcpstates[tp->t_state]);
	/* print out internal state of tp !?! */
	printf("\n");
	if (sflag) {
		printf("\trcv_nxt %x rcv_wnd %x snd_una %x snd_nxt %x snd_max %x\n",
		    tp->rcv_nxt, tp->rcv_wnd, tp->snd_una, tp->snd_nxt,
		    tp->snd_max);
		printf("\tsnd_wl1 %x snd_wl2 %x snd_wnd %x\n", tp->snd_wl1,
		    tp->snd_wl2, tp->snd_wnd);
	}
	/* print out timers? */
	if (tflag) {
		char *cp = "\t";
		register int i;

		for (i = 0; i < TCPT_NTIMERS; i++) {
			if (tp->t_timer[i] == 0)
				continue;
			printf("%s%s=%d", cp, tcptimers[i], tp->t_timer[i]);
			if (i == TCPT_REXMT)
				printf(" (t_rxtshft=%d)", tp->t_rxtshift);
			cp = ", ";
		}
		if (*cp != '\t')
			putchar('\n');
	}
}

ptime(ms)
	int ms;
{

	printf("%03d ", (ms/10) % 1000);
}

numeric(c1, c2)
	caddr_t *c1, *c2;
{
	
	return (*c1 - *c2);
}

/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */
char *
ntoa(in)
	struct in_addr in;
{
	static char b[18];
	register char *p;

	in.s_addr = ntohl(in.s_addr);
	p = (char *)&in;
#define	UC(b)	(((int)b)&0xff)
	sprintf(b, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
	return (b);
}
