#ifndef lint
static	char *sccsid = "@(#)rmt.c 1.1 92/07/30 SMI"; /* from CalTech */
#endif not lint

/*
 *  Multi-process streaming 4.3bsd /etc/rmt server.
 *  Has three locks (for stdin, stdout, and the tape)
 *  that are passed by signals and received by sigpause().
 */

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>

struct mtop mtop;
struct mtget mtget;
extern long atol();
extern int errno;
jmp_buf sjbuf;

#define RECV	SIGIO
#define TAPE	SIGURG
#define SEND	SIGALRM
#define ERROR	SIGTERM
#define OPEN	SIGUSR1
#define CLOSE	SIGUSR2

#define CMDMASK (sigmask(RECV) | sigmask(OPEN) | sigmask(CLOSE) | sigmask(ERROR))
#define MASKALL (CMDMASK | sigmask(TAPE) | sigmask(SEND))

#define MAXCHILD 1
int children, childpid[MAXCHILD];

int	tape = -1;
int	maxrecsize;
char	*record;

#define SSIZE	64
char	device[SSIZE], pos[SSIZE], op[SSIZE], mode[SSIZE], count[SSIZE];

FILE	*debug;
#define DEBUG(f)	if (debug) fprintf(debug, f)
#define DEBUG1(f,a)	if (debug) fprintf(debug, f, a)
#define DEBUG2(f,a1,a2) if (debug) fprintf(debug, f, a1, a2)

char key;

catch(sig)
	int sig;
{
	switch (sig) {
	default:    return;
	case OPEN:  key = 'O';	break;
	case CLOSE: key = 'C';	break;
	case ERROR: key = 'E';	break;
	}
	longjmp(sjbuf, 1);
}

main(argc, argv)
	int argc;
	char *argv[];
{
	int parent = getpid(), next = parent;
	register int n, i, cc, rval, saverr;

	if (argc > 1) {
		if ((debug=fopen(argv[1], "w")) == NULL)
			exit(1);
		setbuf(debug, NULL);
	}
	signal(RECV, catch);
	signal(SEND, catch);
	signal(TAPE, catch);
	signal(OPEN, catch);
	signal(CLOSE, catch);
	signal(ERROR, catch);
	sigblock(MASKALL);
	kill(parent, TAPE);
	kill(parent, SEND);
	while (read(0, &key, 1) == 1) {
		switch (key) {
		case 'L':		/* lseek */
			getstring(count);
			getstring(pos);
			DEBUG2("rmtd: L %s %s\n", count, pos);
			kill(next, RECV);
			sigpause(MASKALL &~ sigmask(TAPE));
			rval = lseek(tape, atol(count), atoi(pos));
			saverr = errno;
			kill(next, TAPE);
			sigpause(MASKALL &~ sigmask(SEND));
			respond(rval, saverr);
			break;

		case 'I':		/* ioctl */
			getstring(op);
			getstring(count);
			DEBUG2("rmtd: I %s %s\n", op, count);
			mtop.mt_op = atoi(op);
			mtop.mt_count = atoi(count);
			kill(next, RECV);
			sigpause(MASKALL &~ sigmask(TAPE));
			rval = ioctl(tape, MTIOCTOP, (char *)&mtop);
			saverr = errno;
			kill(next, TAPE);
			sigpause(MASKALL &~ sigmask(SEND));
			respond(rval < 0 ? rval : mtop.mt_count, saverr);
			break;

		case 'S':		/* status */
			DEBUG("rmtd: S\n");
			kill(next, RECV);
			sigpause(MASKALL &~ sigmask(TAPE));
			rval = ioctl(tape, MTIOCGET, (char *)&mtget);
			saverr = errno;
			kill(next, TAPE);
			sigpause(MASKALL &~ sigmask(SEND));
			if (rval < 0)
				respond(rval, saverr);
			else {
				respond(sizeof(mtget), saverr);
				write(1, (char *)&mtget, sizeof(mtget));
			}
			break;

		case 'W':
			getstring(count);
			n = atoi(count);
			checkbuf(n);
			DEBUG1("rmtd: W %s\n", count);
			for (i = 0; i < n; i += cc) {
				cc = read(0, &record[i], n - i);
				if (cc <= 0) {
					DEBUG("rmtd: premature eof\n");
					exit(2);
				}
			}
			kill(next, RECV);
			sigpause(MASKALL &~ sigmask(TAPE));
			rval = write(tape, record, n);
			saverr = errno;
			kill(next, TAPE);
			sigpause(MASKALL &~ sigmask(SEND));
			respond(rval, saverr);
			break;

		case 'R':
			getstring(count);
			n = atoi(count);
			checkbuf(n);
			DEBUG1("rmtd: R %s\n", count);
			kill(next, RECV);
			sigpause(MASKALL &~ sigmask(TAPE));
			rval = read(tape, record, n);
			saverr = errno;
			kill(next, TAPE);
			sigpause(MASKALL &~ sigmask(SEND));
			respond(rval, saverr);
			write(1, record, rval);
			break;

		default:
			DEBUG1("rmtd: garbage command %c\n", key);
			/*FALLTHROUGH*/

		case 'C':
		case 'O':
			/* rendezvous back into a single process */
			if (setjmp(sjbuf) == 0 || getpid() != parent) {
				sigpause(MASKALL &~ sigmask(TAPE));
				sigpause(MASKALL &~ sigmask(SEND));
				kill(parent, key == 'O' ? OPEN :
					key == 'C' ? CLOSE : ERROR);
				sigpause(0);
			}
			while (children > 0) {
				kill(childpid[--children], SIGKILL);
				while (wait(NULL) != childpid[children])
					;
			}
			next = parent;
			if (key == 'C') {
				getstring(device);
				DEBUG1("rmtd: C %s\n", device);
				rval = close(tape);
				respond(rval, errno);
				kill(parent, TAPE);
				kill(parent, SEND);
				continue;
			}
			if (key != 'O') 		/* garbage command */
				exit(3);
			close(tape);
			getstring(device);
			getstring(mode);
			DEBUG2("rmtd: O %s %s\n", device, mode);
			tape = open(device, atoi(mode));
			respond(tape, errno);
			if (tape >= 0)			/* fork off */
				while (children < MAXCHILD &&
					(childpid[children] = fork()) > 0)
						next = childpid[children++];
			if (next == parent) {
				kill(parent, RECV);
				kill(parent, TAPE);
				kill(parent, SEND);
			}
			sigpause(MASKALL &~ CMDMASK);
			continue;
		}
		kill(next, SEND);
		sigpause(MASKALL &~ CMDMASK);
	}
	kill(next, RECV);
	exit(0);
	/* NOTREACHED */
}

respond(rval, errno)
	register int rval, errno;
{
	extern char *sys_errlist[];
	char resp[SSIZE];

	if (rval < 0) {
		sprintf(resp, "E%d\n%s\n", errno, sys_errlist[errno]);
		DEBUG2("rmtd: E %d (%s)\n", errno, sys_errlist[errno]);
	} else {
		sprintf(resp, "A%d\n", rval);
		DEBUG1("rmtd: A %d\n", rval);
	}
	write(1, resp, strlen(resp));
}

getstring(cp)
	register char *cp;
{
	do {
		if (read(0, cp, 1) != 1)
			exit(0);
	} while (*cp++ != '\n');
	*--cp = '\0';
}

checkbuf(size)
	unsigned size;
{
	extern char *malloc();

	if (size <= maxrecsize)
		return;
	if (record != 0)
		free(record);
	if ((record=malloc(size)) == NULL) {
		DEBUG("rmtd: cannot allocate buffer space\n");
		exit(4);
	}
	maxrecsize = size;
}
