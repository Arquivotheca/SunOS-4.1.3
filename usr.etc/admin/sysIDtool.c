#ifndef lint
static	char sccsid[] = "@(#)sysIDtool.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * sysIDtool - semi-automatic installation tool for diskful desktop systems.
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <rpcsvc/ypclnt.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>

#include "admin_amcl.h"
#include "sys_param_names.h"
#include "admin_messages.h"

#define TIMEZONE_MAP_NAME	"timezone.byname"
#define UNCONFIGURED	"/etc/.UNCONFIGURED"
#define YPBIND	"/usr/etc/ypbind"
#define IFCONFIG "/usr/etc/ifconfig -au netmask + broadcast + >/dev/null 2>&1"
#define TIMEHOST "timehost"
#define RETURN_KEY '\n'
#define	ESC_KEY	'\033'
#define	INPUT_CHAR	0
#define	INPUT_LINE	1

extern int gethostname();
extern char *inet_ntoa();
extern int admin_add_argp();
extern void admin_write_err();
extern char *admin_make_errbuf();

extern int sys_nerr;
extern char *sys_errlist[];
extern int errno;

static void fail_dialog();
static void kill_ypbind();
static void save_modes();
static void restore_modes();
static void canon_input();
static void get_input();
static void admin_perror();

static struct termios ttyb;
static unsigned long flags;
static char	min_chars;	/* VMIN control character */
static char	time;		/* VTIME control character */

int
main()
{
	Admin_arg *argp;		/* Argument list for admin methods */
	char hostname[MAXHOSTNAMELEN];	/* Hostname to configure */
	char domain[MAXDOMAINLEN];	/* Domain to configure */
	struct ifreq ifr;		/* ioctl interface request struct */
	char ether_name[10];		/* name of ethernet interface */
	int s;				/* socket for ioctl */
	int i;				/* counter while waiting for ypbind */
	int status;			/* status from various calls */
	char *tzdata;			/* timezone entry from map */
	int timezonelen;
	char timezone[MAXPATHLEN];	/* timezone name */
	char *ipaddr;			/* ip address to configure */
	struct sockaddr_in *sinp;
	char *outbuf;			/* Pointer to stdout buf from amcl */
	char *errbuf;			/* Pointer to stderr buf from amcl */
	char *msgbuf;			/* Error message buffer */
	char tmpbuf[4096];		/* Temp msg buffer */
	int  ypbind_pid;

	/*
	 * Retrieve name of first ethernet interface, make sure it's up,
	 * which means ifconfig with rarp worked, then get address.
	 */
	if (get_ether0name(ether_name, sizeof(ether_name)) == -1) {
		admin_make_errbuf(&msgbuf, SETUP_FAIL_REASONS,
		     SYS_ERR_NO_ETHER0);
		fail_dialog(msgbuf);
	}
	s = socket(AF_INET, SOCK_DGRAM, 0);
	sprintf(ifr.ifr_name, "%s0", ether_name);
	if (ioctl(s, SIOCGIFFLAGS, &ifr) == -1) {
		admin_perror(tmpbuf, "ioctl");
		admin_make_errbuf(&msgbuf, SETUP_FAIL_REASONS, tmpbuf);
		fail_dialog(msgbuf);
	} else if (!(ifr.ifr_flags & IFF_UP)) {
		fail_dialog(IP_FAIL_REASONS);
	}
#ifdef DEBUG
	printf("Parameters for system as determined by sysIDtool\n");
#endif
	sprintf(ifr.ifr_name, "%s0", ether_name);
	if (ioctl(s, SIOCGIFADDR, &ifr) == -1) {
		admin_perror(tmpbuf, "ioctl");
		admin_make_errbuf(&msgbuf, SETUP_FAIL_REASONS, tmpbuf);
		fail_dialog(msgbuf);
	}
	(void) close(s);
	sinp = (struct sockaddr_in *)&ifr.ifr_addr;
	ipaddr = inet_ntoa(sinp->sin_addr);
#ifdef DEBUG
	printf("\tIP addr is %s\n", ipaddr);
#endif

	/*
	 * Get hostname, NIS domain.  These are set in the kernel by
	 * hostconfig which is called from rc.boot.
	 */
	if (gethostname(hostname, sizeof(hostname)) == -1) {
		admin_perror(tmpbuf, "gethostname");
		admin_make_errbuf(&msgbuf, SETUP_FAIL_REASONS, tmpbuf);
		fail_dialog(msgbuf);
	}
#ifdef DEBUG
	printf("\tHostname is %s\n", hostname);
#endif
	if (strlen(hostname) == 0)
		fail_dialog(HOSTNAME_FAIL_REASONS);
	
	if (getdomainname(domain, sizeof(domain)) == -1) {
		admin_perror(tmpbuf, "getdomainname");
		admin_make_errbuf(&msgbuf, SETUP_FAIL_REASONS, tmpbuf);
		fail_dialog(msgbuf);
	}
#ifdef DEBUG
	printf("\tdomain is %s\n", domain);
#endif
	if (strlen(domain) == 0)
		fail_dialog(DOMAINNAME_FAIL_REASONS);

	/* 
	 * Start ypbind for the purposes of sysIDtool.
	 * We'll later kill this and let it start through normal
	 * channels (i.e. rc.local)
	 */
#ifdef DEBUG
	printf("starting ypbind\n");
#endif DEBUG
	switch(ypbind_pid = fork()) {

	case -1:
		admin_make_errbuf(&msgbuf, "sysIDtool: %s", SYS_ERR_YPBIND);
		fail_dialog(msgbuf);
		
	case 0:
		/*
		 * This is the child - starts ypbind
		 */
		if(system(YPBIND) != 0) {
			admin_write_err(SYS_ERR_YPBIND);
			exit(1);
		}
		exit(0);
	default:
#ifdef DEBUG
		printf("parent: ypbind_pid = %d\n",ypbind_pid);
#endif DEBUG
		break;
	}

	/*
	 * Wait a bit for the ypbind to start and then
	 * do an NIS call to force the binding
	 */
	for (i = 0; i < 20; i++) {
		sleep(1);
		if ((status = yp_master(domain, TIMEZONE_MAP_NAME, 
		    &tzdata)) == 0)
			break;
		else if (status == YPERR_YPBIND) {
#ifdef DEBUG
			printf("waiting for ypbind\n");
#endif DEBUG
			continue;
		} else if (status == YPERR_MAP) {
			kill_ypbind(ypbind_pid);
			fail_dialog(TIMEZONE_MAP_REASON);
		} else if (status != 0) {
			admin_make_errbuf(&msgbuf, "yp_master: %s.\n", 
			    yperr_string(status));
			kill_ypbind(ypbind_pid);
			fail_dialog(msgbuf);
		}
	}

	if ( i >= 20 && status != 0 ) {
		/*
		 * the ypbind never worked, for some reason...
		 */
		admin_make_errbuf(&msgbuf, TIMEZONE_FAIL_REASONS,
		    yperr_string(status));
		kill_ypbind(ypbind_pid);
		fail_dialog(msgbuf);
	}


	/*
	 * Retrieve timezone name for our system from NIS map created for
	 * just this purpose.  Look first for entry for our system, if none
	 * found then look for a domain-wide entry.
	 */
	if ((status = yp_match(domain, TIMEZONE_MAP_NAME, hostname,
	    strlen(hostname), &tzdata, &timezonelen)) == YPERR_KEY) {
		if ((status = yp_match(domain, TIMEZONE_MAP_NAME, domain,
		    strlen(domain), &tzdata, &timezonelen)) == YPERR_KEY) {
			kill_ypbind(ypbind_pid);
			fail_dialog(TIMEZONE_KEY_REASON);
		}
	}
	if (status != 0) {
		admin_make_errbuf(&msgbuf, TIMEZONE_FAIL_REASONS, 
		    yperr_string(status));
		kill_ypbind(ypbind_pid);
		fail_dialog(msgbuf);
	}

	sscanf(tzdata, "%s %*s", timezone);

#ifdef DEBUG
	printf("\ttimezone is %s\n", timezone);
#endif
	/*
	 * Do an ifconfig of netmask so if 'timehost' is on another (sub)net
	 * we can reach it with 'rdate'.
	 */
	if (system(IFCONFIG) != 0) {
		admin_make_errbuf(&msgbuf, SETUP_FAIL_REASONS,
		    SYS_ERR_IFCONFIG);
		fail_dialog(msgbuf);
	}


	/*
	 * Now build the argument list for the method invocation
	 */
	argp = NULL;
        (void) admin_add_argp(&argp, HOSTNAME_PARAM, ADMIN_STRING, 
            strlen(hostname), hostname);
        (void) admin_add_argp(&argp, DOMAIN_PARAM, ADMIN_STRING, 
            strlen(domain), domain);
        (void) admin_add_argp(&argp, IPADDR_PARAM, ADMIN_STRING, 
            strlen(ipaddr), ipaddr);
        (void) admin_add_argp(&argp, TIMEZONE_PARAM, ADMIN_STRING, 
            strlen(timezone), timezone);
        (void) admin_add_argp(&argp, TIMESERVER_PARAM, ADMIN_STRING, 
            strlen(TIMEHOST), TIMEHOST);

#ifdef DEBUG
	printf("Attempting system add method\n");
#endif
	if ((status = 
	    admin_perf_method(SYSTEM_CLASS_PARAM, SYS_ADD_METHOD,
	    ADMIN_LOCAL, NULL, argp, &outbuf, &errbuf, ADMIN_END_OPTIONS)) != 
	    SUCCESS) { 
		admin_make_errbuf(&msgbuf, SETUP_FAIL_REASONS, errbuf);
		kill_ypbind(ypbind_pid);
		fail_dialog(msgbuf);
	} else if ((unlink(UNCONFIGURED) != 0) && (errno != ENOENT)) {
		perror("unlink");
		admin_write_err(SYS_ERR_PLS_RM, UNCONFIGURED);
	}
#ifdef DEBUG
	printf(outbuf);
	printf("status = %d\n",status);
#endif
	kill_ypbind(ypbind_pid);
	exit(status);
}

static void kill_ypbind(child_pid)
	int child_pid;
{
	char pid[20];
	char line[MAXPATHLEN];
	char program[MAXPATHLEN];
	FILE *fp, *popen();

	/* 
	 * Find the process ID of ypbind. Note, we have to do it
	 * this way because ypbind forks a child to do the real
	 * work and we have no way to get the child's pid
	 */
        if((fp = popen("ps ax","r")) == NULL ) {
        }

        while(fgets(line,MAXPATHLEN,fp) != NULL) {

                /* The STAT could be different, so scan this line twice */
                sscanf(line,"%s %*s %*s %*s %s %*s %*s %*s",pid,program);

                if(!strcmp(program,YPBIND)) {
#ifdef DEBUG
	printf("found ypbind - killing %s!\n",pid);
#endif DEBUG
			kill(atoi(pid),SIGKILL);
			break;
                }
                else {

                        sscanf(line,"%s %*s %*s %*s %*s %s %*s %*s %*s",pid,program);
                	if(!strcmp(program,YPBIND)) {
#ifdef DEBUG
	printf("found ypbind - killing %s!\n",pid);
#endif DEBUG
				kill(atoi(pid),SIGKILL);
				break;
			}
                }

        }

	/* 
	 * Also kill the child that exec-ed ypbind - if it's still around 
	 */
	kill(child_pid,SIGKILL);
}

/*
 * fail_dialog(msgbuf) - Walk user through failure reasons & possible courses
 * action.  Never returns, we either exit or halt the system, user's choice.
 */
static void fail_dialog(msgbuf)
	char *msgbuf;
{
	char ib[80];

	save_modes(0);
	canon_input(0);
disp_menu:
	printf(FAIL_MENU);
	fflush(stdout);
	for (;;) {
		printf(ENTER_CHOICE);
		fflush(stdout);
		get_input(INPUT_LINE, ib);
		switch (ib[0]) {
			case '1':
				printf(msgbuf);
				fflush(stdout);
				get_input(INPUT_CHAR);
				goto disp_menu;
			case '2':
				printf(MANUAL_INTRO);
				fflush(stdout);
				for (;;) {
					get_input(INPUT_CHAR, ib);
					if (ib[0] == RETURN_KEY) {
						restore_modes(0);
						exit(1);
					} else if (ib[0] == ESC_KEY)
						goto disp_menu;
					else
						printf(BAD_CHOICE_2);
				}
				break;
			case '3':
				restore_modes(0);
				printf(HALTING_MESSAGE);
				fflush(stdout);
				sleep(1);
				reboot(RB_HALT);
			default:
				printf(BAD_CHOICE);
				fflush(stdout);
		}
	}
}

/*
 * get_input(key, buf) - Get input from user, either a single key or a complete
 * line depending on the value of 'key'.  Input is returned in 'buf'.  In line
 * input mode, we ignore whitespace so that the caller is guaranteed that the
 * significant input is at the beginning of the buffer.
 */
static void get_input(key, buf)
	int key;
	char *buf;
{
	char *cp;

	cp = buf;
	for (;;) {
		(void) read(0, cp, 1);
		if (key == INPUT_CHAR)
			return;
		else if (*cp == RETURN_KEY)
			return;
		else if (isspace(*cp))
			continue;
		else
			++cp;
	}
}


/*
 * This routine turns off canonical mode so that characters
 * can be received as typed
 */
static void canon_input(file_des)
	int file_des;
{

	ttyb.c_lflag &= ~ICANON;
	ttyb.c_cc[VMIN] = 1;
	ttyb.c_cc[VTIME] = 0;
	(void) ioctl(file_des, TCSETSF, &ttyb);
}

/* 
 * Save the terminal modes in the static storage
 */
static void save_modes(file_des)
	int file_des;
{

	/* Save current terminal modes */
        (void) ioctl(file_des, TCGETS, &ttyb);
        flags = ttyb.c_lflag;
	min_chars = ttyb.c_cc[VMIN];
	time = ttyb.c_cc[VTIME];
}

/*
 * Restore terminal modes from values saved in static storage
 */
static void restore_modes(file_des)
	int file_des;
{
	ttyb.c_lflag = flags;
	ttyb.c_cc[VMIN] = min_chars;
	ttyb.c_cc[VTIME] = time;
       	(void) ioctl(file_des, TCSETSW, &ttyb);
}


static void admin_perror(buf, name)
	char *buf;
	char *name;
{

	if (errno > sys_nerr)
		sprintf(buf, "%s: Error code %d\n", name, errno);
	else
		sprintf(buf, "%s: %s\n", name, sys_errlist[errno]);
}
