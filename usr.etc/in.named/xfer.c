/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 * This version of xfer is by David Waitzman (dwaitzman@bbn.com) 9/2/88
 *
 * @(#)xfer.c 1.1 92/07/30 SMI from UCB 5.2 (Berkeley & BBN STC) 9/2/88
 */

#define SUNOS4

#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#define COMPILE_XFER			/* modifies the ns.h include file */
#include "ns.h"


char *savestr();

/* max length of data in RR data field */
#define MAXDATA		256	/* from db.h */
 
int	debug = 0; 
int	read_interrupted = 0;
struct	zoneinfo zones;		/* zone information */
struct	timeval	tt;

static char *ddtfile = "/usr/tmp/xfer.ddt";
static char *dbfname;
FILE *fp = 0, *ddt, *dbfp;
char *domain;			/* domain being xfered */
int domain_len;			/* strlen(domain) */

int have_old_serial = 0;	/* set to 1 if -s option given */
u_long old_serial;		/* old serial, given as argument to -s option */

extern char *malloc();
extern int errno;

main(argc, argv)
	int argc;
	char *argv[];
{
	register struct zoneinfo *zp;
	register struct hostent *hp;
 	char *dbfile = NULL, *tracefile = NULL, *tm = NULL;
	int dbfd, result, c;
	extern char *optarg;
	extern int optind, getopt();
	struct timeval ttut[2];

	(void) umask(022);
#ifdef LOG_DAEMON
#ifdef LOGLEVEL
	openlog("named", LOG_PID|LOG_CONS|LOG_NDELAY, LOGLEVEL);
#else
	openlog("named", LOG_PID|LOG_CONS|LOG_NDELAY, LOG_DAEMON);
#endif /* LOGLEVEL */
#else
	openlog("named", LOG_PID);
#endif
	while ((c = getopt(argc, argv, "d:l:t:z:f:s:")) != EOF)
	    switch (c) {
	      case 'd':
		debug = atoi(optarg);
		break;
	      case 'l':
		ddtfile = savestr(optarg);
		break;
	      case 't':
		tracefile = savestr(optarg);
		break;
	      case 'z':		/* zone == domain */
		domain = savestr(optarg);
		domain_len = strlen(domain);
		break;
	      case 'f':
		dbfile = savestr(optarg);
		dbfname = malloc((unsigned)strlen(optarg) + 7 + 1);
		(void) strcpy(dbfname, optarg);
		break;
	      case 's':
		(void)sscanf(optarg, "%lu", &old_serial);
		have_old_serial++;
		break;
	      case '?':
	      default:
		usage();
		exit(-1);
	    }

	if (!domain || !dbfile || optind >= argc) {
	    usage();
	    exit(-1);
	}	    
        if (tracefile && (fp = fopen(tracefile, "w")) == NULL) {
	    perror(tracefile);
	    exit(-1);
	}
	(void) strcat(dbfname, ".XXXXXX");
	/* dbfname is now something like "/etc/named/named.bu.db.XXXXXX" */
	if ((dbfd = mkstemp(dbfname)) == NULL) {
	    perror(dbfname);
	    exit(-1);
	}
	if (fchmod(dbfd, 0644) == -1) {
	    perror(dbfname);
	    exit(-1);
	}
	if ((dbfp = fdopen(dbfd, "r+")) == NULL) {
	    perror(dbfname);
	    exit(-1);
	}
#ifdef DEBUG
	if (debug && (ddt = fopen(ddtfile, "a")) == NULL) {
	    perror(ddtfile);
	    exit(-1);		/* could be more forgiving here */
	}
#endif
	/*
	 * Ignore many types of signals that named (assumed to be our parent)
	 * considers important- if not, the user controlling named with
	 * signals usually kills us.
	 */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGSYS, SIG_IGN);
	if (debug == 0) { /* only ignore if not debugging to give the user
			     some keyboard control */
	    (void) signal(SIGINT, SIG_IGN);
	    (void) signal(SIGQUIT, SIG_IGN);
	}
	(void) signal(SIGIOT, SIG_IGN);

#ifdef notdef	/* want to die here */
#ifdef ALLOW_UPDATES
        /* Catch SIGTERM so we can dump the database upon shutdown if it
           has changed since it was last dumped/booted */
        (void) signal(SIGTERM, onintr);
#endif ALLOW_UPDATES
#endif /* notdef */
#if defined(SIGUSR1) && defined(SIGUSR2)
	(void) signal(SIGUSR1, SIG_IGN);
	(void) signal(SIGUSR2, SIG_IGN);
#else	SIGUSR1&&SIGUSR2
	(void) signal(SIGEMT, SIG_IGN);
	(void) signal(SIGFPE, SIG_IGN);
#endif SIGUSR1&&SIGUSR2

#ifdef DEBUG
	if (debug) (void)fprintf(ddt, "domain `%s' file `%s'\n", domain, dbfile);
#endif
	buildservicelist();
	buildprotolist();

	/* init zone data */
 
	zp = &zones;
	zp->z_type = Z_SECONDARY;
	zp->z_origin = savestr(domain);
	zp->z_source = savestr(dbfile);
	zp->z_addrcnt = 0;
#ifdef DEBUG
	if (debug) {
	    (void)fprintf(ddt,"zone found (%d): ", zp->z_type);
	    if (zp->z_origin[0] == '\0')
	      (void)fprintf(ddt,"'.'");
	    else
	      (void)fprintf(ddt,"'%s'", zp->z_origin);
	    (void)fprintf(ddt,", source = %s\n", zp->z_source);
	}
#endif
	for (; optind != argc; optind++) {
		zp->z_addrcnt++;
		tm = argv[optind];
		zp->z_addr[zp->z_addrcnt].s_addr = inet_addr(tm);
 
		if (zp->z_addr[zp->z_addrcnt].s_addr == (unsigned)-1) {
			hp = gethostbyname(tm);
			if (hp == NULL)
				continue;
#ifdef DEBUG
			if (debug)
				(void)fprintf(ddt,", %s",tm);
#endif
			if (++zp->z_addrcnt >= NSMAX) {
				zp->z_addrcnt = NSMAX;
#ifdef DEBUG
				if (debug)
				    (void)fprintf(ddt, "\nns.h NSMAX reached\n");
#endif
				break;
			}
		}
	}
#ifdef DEBUG
	if (debug) (void)fprintf(ddt," (addrcnt) = %d\n", zp->z_addrcnt);
#endif
        result = getzone(zp);
	(void) fclose(dbfp);
	switch (result) {
	  case 0:
	    if (rename(dbfname, dbfile) == -1) {
		perror("Rename");
		exit(-1);
	    }
	    exit(0);			/* ok exit */
	  case 1:
#ifdef DEBUG
	    if (!debug)
#endif
	      (void) unlink(dbfname);
	    exit(1);			/* servers not reachable exit */
	  case 2:
#ifdef DEBUG
	    if (!debug)
#endif
	      (void) unlink(dbfname);
	    ttut[0] = tt;	/* tt is set by soa_zinfo(), called already */
	    ttut[1] = tt;
	    (void) utimes(dbfile, ttut);
	    exit(2);		/* zone up to date exit */
	  case -1:
	  default:
#ifdef DEBUG
	    if (!debug)
#endif
	      (void) unlink(dbfname);
	    exit(-1);			/* yuck exit */
	}
}
 
usage()
{
    (void)fprintf(stderr,
"Usage: xfer\n\
\t-z zone to transfer\n\
\t-f db file\n\
\t[-d debug level]\n\
\t[-l debug log file (default %s)]\n\
\t[-t trace file]\n\
\tservers...\n", ddtfile);
    exit(-1);
}

getzone(zp)
	struct zoneinfo *zp;
{
	HEADER *hp;
	u_short len;
	u_long serial;
	int s, n, l, cnt, soacnt, error = 0;
 	u_char *cp, *nmp, *eom, *tmp, buf[2 * PACKETSZ]; /* dwaitzma hack 2 * */
	u_char name[MAXDNAME], name2[MAXDNAME];
	struct sockaddr_in sin;
	struct zoneinfo zp_start, zp_finish;
	struct itimerval ival, zeroival;
#ifdef	SUNOS4
	extern void read_alarm();
#else	/* !SUNOS4 */
	extern int read_alarm();
#endif	/* SUNOS4 */
	struct sigvec sv, osv;
#ifdef DEBUG
	if (debug)
		(void)fprintf(ddt,"getzone() %s\n", zp->z_origin);
#endif
	bzero((char *)&zeroival, sizeof(zeroival));
	ival = zeroival;
	ival.it_value.tv_sec = 30;
	sv.sv_handler = read_alarm;
	sv.sv_onstack = 0;
	sv.sv_mask = ~0;
	(void) sigvec(SIGALRM, &sv, &osv);

	_res.options &= ~RES_DEFNAMES; /* Don't use default name rules */

	for (cnt = 1; cnt <= zp->z_addrcnt; cnt++) {
		error = 0;
		bzero((char *)&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(NAMESERVER_PORT);
		sin.sin_addr = zp->z_addr[cnt];
		if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			syslog(LOG_ERR, "zoneref: socket: %m");
			error++;
			break;
		}	
#ifdef DEBUG
		if (debug >= 2) {
			(void)fprintf(ddt,"connecting to server #%d %s, %d\n",
			   cnt+1, inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
		}
#endif
		if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			(void) close(s);
			error++;
#ifdef DEBUG
			if (debug >= 2)
				(void)fprintf(ddt,"connect failed, errno %d\n", errno);
#endif
			continue;
		}	
		if ((n = res_mkquery(QUERY, zp->z_origin, C_IN,
		    T_SOA, (char *)NULL, 0, NULL, buf, sizeof(buf))) < 0) {
		        syslog(LOG_ERR, "zoneref: res_mkquery failed");
			(void) close(s);
			(void) sigvec(SIGALRM, &osv, (struct sigvec *)0);
		        return -1;
		}
		/*
	 	 * Send length & message for zone transfer
	 	 */
		if (writemsg(s, buf, n) < 0) {
			(void) close(s);
			error++;
#ifdef DEBUG
			if (debug >= 2)
				(void)fprintf(ddt,"writemsg failed\n");
#endif
			continue;	
		}
		/*
		 * Get out your butterfly net and catch the SOA
		 */
		cp = buf;
		l = sizeof(u_short);
		read_interrupted = 0;
		while (l > 0) {
#ifdef DEBUG
		        if (debug > 10) (void)fprintf(ddt,"Before setitimer\n");
#endif
			(void) setitimer(ITIMER_REAL, &ival,
			    (struct itimerval *)NULL);
#ifdef DEBUG
			if (debug > 10) (void)fprintf(ddt,"Before recv(l = %d)\n",n);
#endif
			errno = 0;
		        if ((n = recv(s, (char *)cp, l, 0)) > 0) {
				cp += n;
				l -= n;
			} else {
#ifdef DEBUG
			        if (debug > 10)
			          (void)fprintf(ddt,
"bad recv->%d, errno= %d, read_interrupt=%d\n", n, errno, read_interrupted);
#endif
				if (n == -1 && errno == EINTR 
				    && !read_interrupted)
					continue;
				error++;
				break;
			}
		}

		(void) setitimer(ITIMER_REAL, &zeroival,
		    (struct itimerval *)NULL);
		if (error) {
			(void) close(s);
			continue;
		}
		if ((len = htons(*(u_short *)buf)) == 0) {
			(void) close(s);
			continue;
		}
		l = len;
		cp = buf;
		while (l > 0) {
			(void) setitimer(ITIMER_REAL, &ival,
			    (struct itimerval *)NULL);
			errno = 0;
			if ((n = recv(s, (char *)cp, l, 0)) > 0) {
				cp += n;
				l -= n;
			} else {
				if (errno == EINTR && !read_interrupted)
					continue;
				error++;
#ifdef DEBUG
			        if (debug > 10)
				    (void)fprintf(ddt,
					    "recv failed: n= %d, errno = %d\n",
					    n, errno);
#endif
				break;
			}
		}
		(void) setitimer(ITIMER_REAL, &zeroival,
		    (struct itimerval *)NULL);
		if (error) {
			(void) close(s);
			continue;
		}
#ifdef DEBUG
		if (debug >= 3) {
			(void)fprintf(ddt,"len = %d\n", len);
			fp_query(buf, ddt);
		}
#endif DEBUG
		zp_start = *zp;
		tmp = buf + sizeof(HEADER);
		eom = buf + len;
		tmp += dn_skipname(tmp, eom) + QFIXEDSZ;
		tmp += dn_skipname(tmp, eom);
		soa_zinfo(&zp_start, tmp, eom);
		/*
		 * If we were give the old serial number on the argument line,
		 * check to see if it is the same or newer than is returned
		 * in the SOA record.  If so, don't bother with the transfer.
		 */
		if (have_old_serial && old_serial >= zp_start.z_serial) {
			(void) close(s);
			(void) sigvec(SIGALRM, &osv, (struct sigvec *)0);
			return 2;
		}
		hp = (HEADER *) buf;
		soacnt = 0;
		for (;;) {
			if (soacnt == 0) {
			    if ((n = res_mkquery(QUERY, zp->z_origin, C_IN,
			      T_AXFR, (char *)NULL, 0, NULL,
			      buf, sizeof(buf))) < 0) {
				syslog(LOG_ERR, "zoneref: res_mkquery failed");
				(void) close(s);
				(void) sigvec(SIGALRM, &osv,
				    (struct sigvec *)0);
				return -1;
			    }
			    /*
	 	 	     * Send length & message for zone transfer
	 	 	     */
			    if (writemsg(s, buf, n) < 0) {
				    (void) close(s);
				    error++;
#ifdef DEBUG
				    if (debug >= 2)
					    (void)fprintf(ddt,"writemsg failed\n");
#endif
				    break;	
			    }
			}
			/*
			 * Receive length & response
			 */
			cp = buf;
			l = sizeof(u_short);
			while (l > 0) {
				(void) setitimer(ITIMER_REAL, &ival,
				    (struct itimerval *)NULL);
				errno = 0;
				if ((n = recv(s, (char *)cp, l, 0)) > 0) {
					cp += n;
					l -= n;
				} else {
					if (errno == EINTR && !read_interrupted)
						continue;
					error++;
#ifdef DEBUG
					if (debug >= 2)
					  (void)fprintf(ddt,
					    "recv failed: n= %d, errno = %d\n",
					    n, errno);
#endif
					break;
				}
			}
			(void) setitimer(ITIMER_REAL, &zeroival,
			    (struct itimerval *)NULL);
			if (error)
				break;
			if ((len = htons(*(u_short *)buf)) == 0)
				break;
			l = len;
			cp = buf;
			eom = buf + len;
			while (l > 0) {
				(void) setitimer(ITIMER_REAL, &ival,
				    (struct itimerval *)NULL);
				errno = 0;
				if ((n = recv(s, (char *)cp, l, 0)) > 0) {
					cp += n;
					l -= n;
				} else {
					if (errno == EINTR && !read_interrupted)
						continue;
					error++;
#ifdef DEBUG
					if (debug >= 2)
						(void)fprintf(ddt,"recv failed\n");
#endif
					break;
				}
			}
			(void) setitimer(ITIMER_REAL, &zeroival,
			    (struct itimerval *)NULL);
			if (error)
				break;
#ifdef DEBUG
			if (debug >= 3) {
				(void)fprintf(ddt,"len = %d\n", len);
				fp_query(buf, ddt);
			}
			if (fp) fp_query(buf,fp);
#endif
			cp = buf + sizeof(HEADER);
			if (hp->qdcount)
				cp += dn_skipname(cp, eom) + QFIXEDSZ;
			nmp = cp;
			tmp = cp + dn_skipname(cp, eom);

			n = print_output(buf, sizeof(buf), cp);
			if (cp + n != eom) {
#ifdef DEBUG
			   if (debug)
	                     (void)fprintf(ddt,
				     "zoneref: print_update failed (%d, %d)\n",
					cp - buf, n);
#endif
				error++;
				break;
			}
			GETSHORT(n, tmp);
			if (n == T_SOA) {
				if (soacnt == 0) {
					soacnt++;
					dn_expand(buf, buf + 512, nmp, name,
						sizeof(name));
					tmp += 2 * sizeof(u_short)
						+ sizeof(u_long);
					tmp += dn_skipname(tmp, eom);
					tmp += dn_skipname(tmp, eom);
					GETLONG(serial, tmp);
#ifdef DEBUG
					if (debug > 2)
					    (void)fprintf(ddt,
					        "first SOA for %s, serial %d\n",
					        name, serial);
#endif DEBUG
					continue;
				}
				dn_expand(buf, buf + 512, nmp, name2, 
					sizeof(name2));
				if (strcasecmp((char *)name, (char *)name2) != 0) {
#ifdef DEBUG
					if (debug > 1)
					    (void)fprintf(ddt,
					      "extraneous SOA for %s\n",
					      name2);
#endif DEBUG
					continue;
				}
				tmp -= sizeof(u_short);
				soa_zinfo(&zp_finish, tmp, eom);
#ifdef DEBUG
				if (debug > 1)
				    (void)fprintf(ddt,
				      "SOA, serial %d\n", zp_finish.z_serial);
#endif DEBUG
				if (serial != zp_finish.z_serial) {
					 soacnt = 0;
#ifdef DEBUG
					if (debug)
					    (void)fprintf(ddt,
						"serial changed, restart\n");
#endif DEBUG
				} else
					break;
			}
		}
		(void) close(s);
		if ( error == 0) {
			zp->z_refresh = zp_finish.z_refresh;
			zp->z_retry   = zp_finish.z_retry;
			zp->z_expire  = zp_finish.z_expire;
			zp->z_minimum = zp_finish.z_minimum;
			zp->z_serial  = zp_finish.z_serial;
			zp->z_lastupdate = tt.tv_sec;
			zp->z_sysloged = 0;
			zp->z_auth = 1;
			(void) sigvec(SIGALRM, &osv, (struct sigvec *)0);
			return 0;
		}
#ifdef DEBUG
		if (debug >= 2)
                    (void)fprintf(ddt,"error receiving zone transfer\n");
#endif
	}
	(void) sigvec(SIGALRM, &osv, (struct sigvec *)0);
	/* djw: 7/5/88 next three lines added based upon 
	 * bug report by slevy@uc.msc.edu
	 */
	zp->z_refresh = zp->z_retry;
	if (tt.tv_sec - zp->z_lastupdate > zp->z_expire)
		zp->z_auth = 0;
	/*
	 *     Freedom at last!!
	 *
	 *  The land where all repressed slaves dream of.
	 *
	 *  Can't find a master to talk to.
	 *  syslog it and hope we can find a master during maintenance
	 */
	if (error && (!zp->z_sysloged)) {
	    syslog(LOG_WARNING,
		"zoneref: Masters for secondary zone %s unreachable",
		zp->z_origin);
	    return 1;
	}
	return -1;
}

/*
 * Set flag saying to read was interrupted
 * used for a read timer
 */
#ifdef	SUNOS4
void
#else	/* !SUNOS4 */
int
#endif	/* SUNOS4 */
read_alarm()
{
	extern int read_interrupted;
	read_interrupted = 1;
}
 
writemsg(rfd, msg, msglen)
	int rfd;
	u_char *msg;
	int msglen;
{
	struct iovec iov[2];
	u_short len = htons((u_short)msglen);
 
	iov[0].iov_base = (caddr_t)&len;
	iov[0].iov_len = sizeof(len);
	iov[1].iov_base = (caddr_t)msg;
	iov[1].iov_len = msglen;
	if (writev(rfd, iov, 2) != sizeof(len) + msglen) {
#ifdef DEBUG
	    if (debug)
	      (void)fprintf(ddt,"write failed %d\n", errno);
#endif
	    return (-1);
	}
	return (0);
}


soa_zinfo(zp, cp, eom)
	register struct zoneinfo *zp;
	register u_char *cp;
	u_char *eom;
{
	cp += 3 * sizeof(u_short);
	cp += sizeof(u_long);
	cp += dn_skipname(cp, eom);
	cp += dn_skipname(cp, eom);
	GETLONG(zp->z_serial, cp);
	GETLONG(zp->z_refresh, cp);
	gettime(&tt);
	zp->z_time = tt.tv_sec + zp->z_refresh;
	GETLONG(zp->z_retry, cp);
	GETLONG(zp->z_expire, cp);
	GETLONG(zp->z_minimum, cp);
}

gettime(ttp)
struct timeval *ttp;
{
	if (gettimeofday(ttp, (struct timezone *)0) < 0)
		syslog(LOG_ERR, "gettimeofday failed: %m");
	return;
}

/*
 * Parse the message, determine if it should be printed, and if so, print it
 * in .db file form.
 * Does minimal error checking on the message content.
 */
print_output(msg, msglen, rrp)
	u_char *msg;
	int msglen;
	u_char *rrp;
{
	register u_char *cp;
        register HEADER *hp = (HEADER *) msg;
	u_long addr, ttl;
	int n, i, j, tab, result, class, type, dlen, n1;
	u_char *cp1, data[BUFSIZ];
	char *cdata, *origin, *proto, dname[MAXDNAME];
	static int minimum_ttl = 0, got_soa = 0;
	static char prev_origin[MAXDNAME] = "", prev_dname[MAXDNAME] = "";
	static int have_printed = 0;
	extern char *inet_ntoa(), *p_protocol(), *p_service();

	cp = rrp;
	if ((n = Xdn_expand(msg, msg + msglen, cp, (u_char *)dname,
			   sizeof(dname))) < 0) {
	    hp->rcode = FORMERR;
	    return (-1);
	}
	cp += n;
	GETSHORT(type, cp);
	GETSHORT(class, cp);
	GETLONG(ttl, cp);
	GETSHORT(dlen, cp);

	origin = index(dname, '.');	
	if (origin == NULL)
	    origin = "";
	else
	    origin++;		/* move past the '.' */
#ifdef DEBUG
	if (debug > 2)
	  (void)fprintf(ddt,"print_output: dname %s type %d class %d ttl %d\n",
		  dname, type, class, ttl);
#endif
	/*
	 * Convert the resource record data into the internal
	 * database format.
	 */
	switch (type) {
	  case T_A:
	  case T_WKS:
	  case T_HINFO:
	  case T_UINFO:
	  case T_UID:
	  case T_GID:		
#ifdef	T_MP
	  case T_MP:
#endif	/* T_MP */
	    cp1 = cp;
	    n = dlen;
	    cp += n;
	    break;
	    
	  case T_CNAME:
	  case T_MB:
	  case T_MG:
	  case T_MR:
	  case T_NS:
	  case T_PTR:
#ifdef	T_UNAME
	  case T_UNAME:
#endif	/* T_UNAME */
	    if ((n = Xdn_expand(msg, msg + msglen, cp, data,
			       sizeof(data))) < 0) {
				   hp->rcode = FORMERR;
				   return (-1);
			       }
	    cp += n;
	    cp1 = data;
	    n = strlen((char *)data) + 1;
	    break;
	    
	  case T_MINFO:
	  case T_SOA:
	    if ((n = Xdn_expand(msg, msg + msglen, cp, data,
			       sizeof(data))) < 0) {
				   hp->rcode = FORMERR;
				   return (-1);
			       }
	    cp += n;
	    cp1 = data + (n = strlen((char *)data) + 1);
	    n1 = sizeof(data) - n;
	    if (type == T_SOA)
	      n1 -= 5 * sizeof(u_long);
	    if ((n = Xdn_expand(msg, msg + msglen, cp, cp1, n1)) < 0) {
		hp->rcode = FORMERR;
		return (-1);
	    }
	    cp += n;
	    cp1 += strlen((char *)cp1) + 1;
	    if (type == T_SOA) {
		register u_char *s = (u_char *)((u_long *)cp + 4);
		GETLONG(minimum_ttl, s);
		bcopy((char *)cp, (char *)cp1, n = 5 * sizeof(u_long));
		cp += n;
		cp1 += n;
	    }
	    n = cp1 - data;
	    cp1 = data;
	    break;
	    
	  case T_MX:
	    /* grab preference */
	    bcopy((char *)cp, (char *)data, sizeof(u_short));
	    cp1 = data + sizeof(u_short);
	    cp += sizeof(u_short);
	    
	    /* get name */
	    if ((n = Xdn_expand(msg, msg + msglen, cp, cp1,
			       sizeof(data)-sizeof(u_short))) < 0)
	      return(-1);
	    cp += n;
	    
	    /* compute end of data */
	    cp1 += strlen((char *)cp1) + 1;
	    /* compute size of data */
	    n = cp1 - data;
	    cp1 = data;
	    break;
	    
	  default:
#ifdef DEBUG
	    if (debug >= 3)
	      (void)fprintf(ddt,"unknown type %d\n", type);
#endif
	    return ((cp - rrp) + dlen);
	}
	if (n > MAXDATA) {
#ifdef DEBUG
	    if (debug)
	      (void)fprintf(ddt,
		      "update type %d: %d bytes is too much data\n",
		      type, n);
#endif
	    hp->rcode = NOCHANGE;	/* XXX - FORMERR ??? */
	    return(-1);
	}
	
	cdata = (char *)cp1;
	result = cp - rrp;

	/*
	 * Print the time of day at the top of the file as a comment
	 */
	if (!have_printed) {
		gettime(&tt);
		(void)fprintf(dbfp, ";\n; Last update %s;\n",
		    ctime((long *)&tt.tv_sec));
		have_printed = 1;
	}

	/*
	 * Only print one SOA per db file
	 */
        if (type == T_SOA) {
	    if (got_soa)
	      return result;
	    else
	      got_soa++;
	}

	/*
	 * Filter out RRs that are not in the domain being xfer''ed-
	 * compare the tail end of the RR dname with the domain name that
	 * this program is xfering.  On a mismatch, do not print the RR
	 */
	{
	    register char *tail_dname = dname + strlen(dname) - domain_len;
	    if (tail_dname < dname || strcasecmp(tail_dname, domain))
	      return result;
	}

	/*
	 * If the origin has changed, print the new origin
	 */
	if (strcasecmp(prev_origin, origin)) {
	    (void) strcpy(prev_origin, origin);
	    (void)fprintf(dbfp, "$ORIGIN %s.\n", origin);
	}

 	tab = 0;

	if (strcasecmp(prev_dname, dname)) {
	    /*
	     * set the prev_dname to be the current dname, then cut off all
	     * characters of dname after (and including) the first '.'
	     */
	    char *cutp = index(dname, '.');
	    (void) strcpy(prev_dname, dname);
	    if (cutp) *cutp = NULL;

	    if (dname[0] == 0) {
		if (origin[0] == 0)
		  (void)fprintf(dbfp, ".\t");
		else
		  (void)fprintf(dbfp, ".%s.\t", origin); /* ??? */
	    } else
	      (void)fprintf(dbfp, "%s\t", dname);
	    if (strlen(dname) < 8)
	      tab = 1;
	} else {
	    (void) putc('\t', dbfp);
	    tab = 1;
	}

	if (ttl != 0 &&
	    ttl != minimum_ttl)
	  (void)fprintf(dbfp, "%d\t", (int)ttl);
	else if (tab)
	  (void) putc('\t', dbfp);

	(void)fprintf(dbfp, "%s\t%s\t", p_class(class),
		p_type(type));
	cp = (u_char *)cdata;
	/*
	 * Print type specific data
	 */
	switch (type) {
	  case T_A:
	    switch (class) {
	      case C_IN:
		GETLONG(n, cp);
		n = htonl(n);
		(void)fprintf(dbfp, "%s",
			inet_ntoa(*(struct in_addr *)&n));
		break;
	    }
/*XXXX
	    if (nstime)
	      (void)fprintf(dbfp, "\t; %d", nstime);
XXXX*/
	    (void)fprintf(dbfp, "\n");
	    break;
	  case T_CNAME:
	  case T_MB:
	  case T_MG:
	  case T_MR:
	  case T_PTR:
#ifdef	T_UNAME
	  case T_UNAME:
#endif	/* T_UNAME */
	    if (cp[0] == '\0')
	      (void)fprintf(dbfp, ".\n");
	    else
	      (void)fprintf(dbfp, "%s.\n", cp);
	    break;
	    
	  case T_NS:
	    cp = (u_char *)cdata;
	    if (cp[0] == '\0')
	      (void)fprintf(dbfp, ".\t");
	    else
	      (void)fprintf(dbfp, "%s.", cp);
/*XXXXX
	    if (nstime)
	      (void)fprintf(dbfp, "\t; %d???", nstime);
XXXXXX*/
	    (void)fprintf(dbfp, "\n");
	    break;
	    
	  case T_HINFO:
	    if (n = *cp++) {
		(void)fprintf(dbfp, "\"%.*s\" ", (int)n, cp);
		cp += n;
	    } else
	      (void)fprintf(dbfp, "\"\" ");
#ifdef	T_MP
	    /* FALL THROUGH */
	  case T_MP:
#endif	/* T_MP */
	    if (n = *cp++)
	      (void)fprintf(dbfp, "\"%.*s\"", (int)n, cp);
	    else
	      (void)fprintf(dbfp, "\"\"");
	    (void) putc('\n', dbfp);
	    break;
	    
	  case T_SOA:
	    (void)fprintf(dbfp, "%s.", cp);
	    cp += strlen((char *)cp) + 1;
	    (void)fprintf(dbfp, " %s. (\n", cp);
	    cp += strlen((char *)cp) + 1;
	    GETLONG(n, cp);
	    (void)fprintf(dbfp, "\t\t%lu", n);
	    GETLONG(n, cp);
	    (void)fprintf(dbfp, " %lu", n);
	    GETLONG(n, cp);
	    (void)fprintf(dbfp, " %lu", n);
	    GETLONG(n, cp);
	    (void)fprintf(dbfp, " %lu", n);
	    GETLONG(n, cp);
	    (void)fprintf(dbfp, " %lu )\n", n);
	    break;
	    
	  case T_MX:
	    GETSHORT(n, cp);
	    (void)fprintf(dbfp,"%lu", n);
	    (void)fprintf(dbfp," %s.\n", cp);
	    break;
	    
	    
	  case T_UINFO:
	    (void)fprintf(dbfp, "\"%s\"\n", cp);
	    break;
	    
	  case T_UID:
	  case T_GID:
	    if (n == sizeof(u_long)) {
		GETLONG(n, cp);
		(void)fprintf(dbfp, "%lu\n", n);
	    }
	    break;
	    
	  case T_WKS:
	    GETLONG(addr, cp);	
	    addr = htonl(addr);	
	    (void)fprintf(dbfp,"%s ",
		    inet_ntoa(*(struct in_addr *)&addr));
	    proto = p_protocol(*cp); /* protocol */
	    cp += sizeof(char); 
	    (void)fprintf(dbfp, "%s ", proto);
	    i = 0;
	    while(cp < (u_char *)cdata + n) {
		j = *cp++;
		do {
		    if(j & 0200)
		      (void)fprintf(dbfp," %s",
			      p_service(i, proto));
		    j <<= 1;
		} while(++i & 07);
	    } 
	    (void)fprintf(dbfp,"\n");
	    break;
	    
	  case T_MINFO:
	    (void)fprintf(dbfp, "%s.", cp);
	    cp += strlen((char *)cp) + 1;
	    (void)fprintf(dbfp, " %s.\n", cp);
	    break;
	  default:
	    (void)fprintf(dbfp, "???\n");
	}
    return result;
}

/*
 * Make a copy of a string and return a pointer to it.
 */
char *
savestr(str)
	char *str;
{
	char *cp;

	cp = malloc((unsigned)strlen(str) + 1);
	if (cp == NULL) {
		syslog(LOG_ERR, "savestr: %m");
		exit(-1);
	}
	(void) strcpy(cp, str);
	return (cp);
}



/* dwaitzma dwaitzma dwaitzma dwaitzma dwaitzma dwaitzma dwaitzma */



Xdn_expand(msg, eomorig, comp_dn, exp_dn, length)
	u_char *msg, *eomorig, *comp_dn, *exp_dn;
	int length;
{
	register u_char *cp, *dn;
	register int n, c;
	u_char *eom;
	int len = -1, checked = 0;

	dn = exp_dn;
	cp = comp_dn;
	eom = exp_dn + length - 1;
	/*
	 * fetch next label in domain name
	 */
	while (n = *cp++) {
		/*
		 * Check for indirection
		 */
		switch (n & INDIR_MASK) {
		case 0:
			if (dn != exp_dn) {
				if (dn >= eom)
					return (-1);
				*dn++ = '.';
			}
			if (dn+n >= eom)
				return (-1);
			checked += n + 1;
			while (--n >= 0) {
				if ((c = *cp++) == '.') {
					if (dn+n+1 >= eom)
						return (-1);
					*dn++ = '\\';
				}
				*dn++ = c;
				if (cp >= eomorig)	/* out of range */
					return(-1);
			}
			break;

		case INDIR_MASK:
			if (len < 0)
				len = cp - comp_dn + 1;
			cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			if (cp < msg || cp >= eomorig)	/* out of range */
				return(-1);
			checked += 2;
			/*
			 * Check for loops in the compressed name;
			 * if we've looked at the whole message,
			 * there must be a loop.
			 */
			if (checked >= eomorig - msg)
				return (-1);
			break;

		default:
			return (-1);			/* flag error */
		}
	}
	*dn = '\0';
	if (len < 0)
		len = cp - comp_dn;
	return (len);
}

