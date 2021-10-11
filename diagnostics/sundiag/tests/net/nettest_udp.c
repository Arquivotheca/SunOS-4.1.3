#ifndef lint
static char sccsid[] = "@(#)nettest_udp.c 1.1 7/30/92 Copyright Sun Microsystems"; 
#endif

#define BSD43
/* #define BSD42 */
/* #define BSD41a */
#if defined(sgi)
#define SYSV
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>		/* struct timeval */
#include <varargs.h>
#include "sdrtns.h"
#include "nettest.h" 

#define BUFLEN (1502-42)       /* max allowed - overhead */
#define NBUF (1502-42) 

#if defined(SYSV)
#include <sys/times.h>
#include <sys/param.h>
struct rusage {
    struct timeval ru_utime, ru_stime;
};
#define RUSAGE_SELF 0
#else
#include <sys/resource.h>
#endif

struct sockaddr_in sinhim;

int fd_sock;			/* fd of network socket */
int numCalls = 0;		/* # of NRead/NWrite calls. */
char *buf;			/* ptr to dynamic buffer */
struct hostent *addr;
extern int errno;

void prep_timer();
double read_timer();
int Nwrite();
extern int make_sock();
double cput, realt;		/* user, real time (seconds) */

int
udp_test(test_net, from)
int   test_net; 
struct  sockaddr_in from;       /* address of the responding node  */
{
	unsigned long addr_tmp;
        int i, packets_transmitted = 0, transmit_succeeded = 1; 
        int buflen = BUFLEN;		/* length of buffer */
 	int nbuf = NBUF;		/* number of buffers to send in sinkmode */
	short port = 5001;		/* UDP port number */
	char stats[128];
	long nbytes = 0;		/* bytes on net */

	func_name = "udp_test";
	TRACE_IN
	/* xmitr */
	bzero((char *)&sinhim, sizeof(sinhim));
	if ((addr=gethostbyaddr(&from.sin_addr.s_addr,
             	sizeof (struct in_addr),AF_INET)) == NULL)
	{
	    send_message(-NO_HOST_NAME, ERROR,"%s: Bad host addr errno=%d", func_name, errno);
	}

	sinhim.sin_family = addr->h_addrtype;
	bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
#ifdef cray
	sinhim.sin_addr = addr_tmp;
#else
	sinhim.sin_addr.s_addr = addr_tmp;
#endif cray

	sinhim.sin_port = htons(port);

	if( (buf = (char *)malloc(buflen)) == (char *)NULL)
	    send_message(-MALLOC_ERROR, ERROR,"%s: malloc errno=%d", func_name, errno);

	send_message(0, VERBOSE,"%s: buflen=%d, nbuf=%d, port=%d %s  -> %s", func_name, buflen, nbuf, port, "udp", addr->h_name);  

	fd_sock = make_sock(test_net, SOCK_DGRAM);

	prep_timer();
	errno = 0;
	/*in sinkmode */ 
	pattern( buf, buflen );
	if (( Nwrite( 4 )) < 0 )     /* rcvr start */
		return (!transmit_succeeded);

	while (nbuf-- && (packets_transmitted = (Nwrite(buflen)) == buflen)) {
		nbytes += buflen;
		if((gethostid() >> 24) == 0x71) delay(1000);
	}

        if (packets_transmitted < 0)
                return (!transmit_succeeded);

	if (( Nwrite( 4 )) < 0 )     /* rcvr end */
		return (!transmit_succeeded);

	(void)read_timer(stats,sizeof(stats));

	/* testing for udp and transmitting */
        for (i = 0 ; i < 4; i++)
        {
	    if (( Nwrite( 4 )) < 0 )     /* rcvr end */
		    return (!transmit_succeeded);
        }

	if( cput <= 0.0 )  cput = 0.001;
	if( realt <= 0.0 )  realt = 0.001;

        if (nbytes)
        {
            send_message(0, VERBOSE,
            "udp_test: %d bytes in %.2f real seconds = %.2f KB/sec +++",
                nbytes, realt, ((double)nbytes)/realt/1024 );
            send_message(0, VERBOSE,
            "udp_test: %d bytes in %.2f CPU seconds = %.2f KB/cpu sec",
                nbytes, cput, ((double)nbytes)/cput/1024 );
        }
	else
        {
            send_message(0, VERBOSE, "No bytes transmitted");
        }
        send_message(0, VERBOSE,
            "udp_test: %d I/O calls, msec/call = %.2f, calls/sec = %.2f",
                numCalls, 1024.0 * realt/((double)numCalls),
                ((double)numCalls)/realt);
	/*  report more stats 
            send_message(0, VERBOSE,"udp_test: %s", stats); 
            printf("udp_test: %s", stats); 
 	*/ 

	TRACE_OUT
	free(buf);
	return(1);
}

pattern( cp, cnt )
register char *cp;
register int cnt;
{
	register char c;
	c = 0;
	while( cnt-- > 0 )  {
		while( !isprint((c&0x7F)) )  c++;
		*cp++ = (c++&0x7F);
	}
}

static struct	timeval time0;	/* Time at which timing started */
static struct	rusage ru0;	/* Resource utilization at the start */

static void prusage();
static void tvadd();
static void tvsub();
static void psecs();

#if defined(SYSV)
static
getrusage(ignored, ru)
    int ignored;
    register struct rusage *ru;
{
    struct tms buf;

    times(&buf);

    ru->ru_stime.tv_sec  = buf.tms_stime / HZ;
    ru->ru_stime.tv_usec = (buf.tms_stime % HZ) * (1000000/HZ);
    ru->ru_utime.tv_sec  = buf.tms_utime / HZ;
    ru->ru_utime.tv_usec = (buf.tms_utime % HZ) * (1000000/HZ);
}

#ifndef sgi	/* it's a real system call */
/*ARGSUSED*/
static 
gettimeofday(tp, zp)
    struct timeval *tp;
    struct timezone *zp;
{
    tp->tv_sec = time(0);
    tp->tv_usec = 0;
}
#endif
#endif SYSV

/*
 *			P R E P _ T I M E R
 */
void
prep_timer()
{
	gettimeofday(&time0, (struct timezone *)0);
	getrusage(RUSAGE_SELF, &ru0);
}

/*
 *			R E A D _ T I M E R
 * 
 */
double
read_timer(str,len)
char *str;
int   len;
{
	struct timeval timedol;
	struct rusage ru1;
	struct timeval td;
	struct timeval tend, tstart;
	char line[132];

	getrusage(RUSAGE_SELF, &ru1);
	gettimeofday(&timedol, (struct timezone *)0);
	prusage(&ru0, &ru1, &timedol, &time0, line);
	(void)strncpy( str, line, len );

	/* Get real time */
	tvsub( &td, &timedol, &time0 );
	realt = td.tv_sec + ((double)td.tv_usec) / 1000000;

	/* Get CPU time (user+sys) */
	tvadd( &tend, &ru1.ru_utime, &ru1.ru_stime );
	tvadd( &tstart, &ru0.ru_utime, &ru0.ru_stime );
	tvsub( &td, &tend, &tstart );
	cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
	if( cput < 0.00001 )  cput = 0.00001;
	return( cput );
}

static void
prusage(r0, r1, e, b, outp)
	register struct rusage *r0, *r1;
	struct timeval *e, *b;
	char *outp;
{
	struct timeval tdiff;
	register time_t t;
	register char *cp;
	register int i;
	int ms;

	t = (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
	    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
	    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
	    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
	ms =  (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

#define END(x)	{while(*x) x++;}
#if defined(SYSV)
	cp = "%Uuser %Ssys %Ereal %P";
#else
	cp = "%Uuser %Ssys %Ereal %P %Xi+%Dd %Mmaxrss %F+%Rpf %Ccsw";
#endif
	for (; *cp; cp++)  {
		if (*cp != '%')
			*outp++ = *cp;
		else if (cp[1]) switch(*++cp) {

		case 'U':
			tvsub(&tdiff, &r1->ru_utime, &r0->ru_utime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
			END(outp);
			break;

		case 'S':
			tvsub(&tdiff, &r1->ru_stime, &r0->ru_stime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
			END(outp);
			break;


		case 'E':
			psecs(ms / 100, outp);
			END(outp);
			break;

		case 'P':
			sprintf(outp,"%d%%", (int) (t*100 / ((ms ? ms : 1))));
			END(outp);
			break;

#if !defined(SYSV)
		case 'W':
			i = r1->ru_nswap - r0->ru_nswap;
			sprintf(outp,"%d", i);
			END(outp);
			break;

		case 'X':
			sprintf(outp,"%d", t == 0 ? 0 : (r1->ru_ixrss-r0->ru_ixrss)/t);
			END(outp);
			break;

		case 'D':
			sprintf(outp,"%d", t == 0 ? 0 :
			    (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'K':
			sprintf(outp,"%d", t == 0 ? 0 :
			    ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
			    (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'M':
			sprintf(outp,"%d", r1->ru_maxrss/2);
			END(outp);
			break;

		case 'F':
			sprintf(outp,"%d", r1->ru_majflt-r0->ru_majflt);
			END(outp);
			break;

		case 'R':
			sprintf(outp,"%d", r1->ru_minflt-r0->ru_minflt);
			END(outp);
			break;

		case 'I':
			sprintf(outp,"%d", r1->ru_inblock-r0->ru_inblock);
			END(outp);
			break;

		case 'O':
			sprintf(outp,"%d", r1->ru_oublock-r0->ru_oublock);
			END(outp);
			break;
		case 'C':
			sprintf(outp,"%d+%d", r1->ru_nvcsw-r0->ru_nvcsw,
				r1->ru_nivcsw-r0->ru_nivcsw );
			END(outp);
			break;
#endif !SYSV
		}
	}
	*outp = '\0';
}

static void
tvadd(tsum, t0, t1)
	struct timeval *tsum, *t0, *t1;
{

	tsum->tv_sec = t0->tv_sec + t1->tv_sec;
	tsum->tv_usec = t0->tv_usec + t1->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

static void
tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

static void
psecs(l,cp)
long l;
register char *cp;
{
	register int i;

	i = l / 3600;
	if (i) {
		sprintf(cp,"%d:", i);
		END(cp);
		i = l % 3600;
		sprintf(cp,"%d%d", (i/60) / 10, (i/60) % 10);
		END(cp);
	} else {
		i = l;
		sprintf(cp,"%d", i / 60);
		END(cp);
	}
	i %= 60;
	*cp++ = ':';
	sprintf(cp,"%d%d", i / 10, i % 10);
}

/*
 *			N W R I T E
 */
int
Nwrite( count )
int count;
{
    register int cnt;
    int retries = 0;
    static int udp_retries = 0;

    while (TRUE)
    {
        cnt = sendto( fd_sock, buf, count, 0, &sinhim, sizeof(sinhim) );
        numCalls++;
        if( cnt<0 )
        {
            switch (errno)
            {
                case  ENOBUFS:
                        delay(50000);
                        errno = 0;
                        break;
                case EMSGSIZE:
                        if (retries > 10)
                        {
                            send_message(0, WARNING,
                                           "udp_test: IO errno=%d", errno);
                            return (cnt);
                        }
                        retries++;
                        delay(100000);
                        errno = 0;
                        break;
                default:
                        if (errno) 
 			{
                            if (udp_retries > 3)
                                send_message(-PACKETSIZE_ERROR, ERROR,
                                            "udp_test: IO errno=%d", errno);
                            udp_retries++;
			    return(cnt);
			}

             }
         }    
         else 
             return(cnt);

	 delay(50000);
    }
}

delay(us)
int us;
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = us;
	(void)select( 1, (char *)0, (char *)0, (char *)0, &tv );
	return(1);
}

