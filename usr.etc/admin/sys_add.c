#ifndef lint
static	char sccsid[] = "@(#)sys_add.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * sys_add HOSTNAME=hostname DOMAIN=domainname IPADDR=ipaddr \
 *         TIMESERVER=timeserver TIMEZONE=tzpath
 *
 * Administrative method to perform initial system installation.
 */

#include <stdio.h>
#include <sys/param.h>
#include "admin_amcl.h"
#include "sys_param_names.h"
#include "admin_messages.h"

extern int admin_get_param();
extern int admin_add_argp();
extern void admin_write_err();


int
main(argc, argv)
	int argc;
	char *argv[];
{
	Admin_arg *argp;		/* Arg list for method invocation */
	int status;			/* Output status for methods */
	char *outbuf, *errbuf;		/* Buffers for methods */
	char hostname[MAXHOSTNAMELEN];	/* Hostname to assign */
        char domain[MAXDOMAINLEN];	/* Domain to join */
        char ipaddr[16];          	/* IP address to assign */
        char timeserver[MAXHOSTNAMELEN];/* Server to be synced with */
        char timezone[MAXPATHLEN];      /* Timezone name to place us in */

#ifdef DEBUG
	printf("in sys_add, parsing args\n");
#endif DEBUG

	/* Parse params from command line */
	if (admin_get_param(argc, argv, HOSTNAME_PARAM, hostname,
	    sizeof(hostname)) == -1) {
		admin_write_err(SYS_ERR_NO_HOSTNAME, SYS_ADD_METHOD);
		exit(FAIL_CLEAN);
	}

	if (admin_get_param(argc, argv, DOMAIN_PARAM, domain, 
	    sizeof(domain)) == -1) {
		admin_write_err(SYS_ERR_NO_DOMAIN, SYS_ADD_METHOD);
		exit(FAIL_CLEAN);
	}

	if (admin_get_param(argc, argv, IPADDR_PARAM, ipaddr, 
	    sizeof(ipaddr)) == -1) {
		admin_write_err(SYS_ERR_NO_IPADDR, SYS_ADD_METHOD);
		exit(FAIL_CLEAN);
	}

	if (admin_get_param(argc, argv, TIMESERVER_PARAM, timeserver, 
	    sizeof(timeserver)) == -1) {
		admin_write_err(SYS_ERR_NO_TIMESERV, SYS_ADD_METHOD);
		exit(FAIL_CLEAN);
	}
#ifdef DEBUG
	printf("sys_add: timeserver = %s\n",timeserver);
#endif DEBUG

	if (admin_get_param(argc, argv, TIMEZONE_PARAM, timezone, 
	    sizeof(timezone)) == -1) {
		admin_write_err(SYS_ERR_NO_TIMEZONE, SYS_ADD_METHOD);
		exit(FAIL_CLEAN);
	}

	/*
	 * Now that we have all the config info, call the individual methods
	 * to set each parameter in the appropriate filesystem objects
	 */
	argp = NULL;
	(void) admin_add_argp(&argp, HOSTNAME_PARAM, ADMIN_STRING, 
	    strlen(hostname), hostname);
#ifdef DEBUG
	printf("Setting hostname to %s...", hostname);
#endif
	if ((status = 
	    admin_perf_method(SYSTEM_CLASS_PARAM, SYS_HOSTNAME_METHOD, 
	    ADMIN_LOCAL, NULL, argp, &outbuf, &errbuf, ADMIN_END_OPTIONS)) !=
	    SUCCESS) {
		admin_write_err(errbuf);
		admin_write_err(SYS_ERR_SET_HOSTNAME, SYS_ADD_METHOD);
		exit(status);
	}
	(void) free(argp);
#ifdef DEBUG
	printf(outbuf);
	printf("Done\n");
#endif

	argp = NULL;
	(void) admin_add_argp(&argp, DOMAIN_PARAM, ADMIN_STRING, 
	    strlen(domain), domain);
#ifdef DEBUG
	printf("Setting domain to %s...", domain);
#endif
	if ((status = admin_perf_method(SYSTEM_CLASS_PARAM, SYS_DOMAIN_METHOD, 
	    ADMIN_LOCAL, NULL, argp, &outbuf, &errbuf, ADMIN_END_OPTIONS)) !=
	    SUCCESS) {
		admin_write_err(errbuf);
		admin_write_err(SYS_ERR_SET_DOMAIN, SYS_ADD_METHOD);
		exit(status);
	}
	(void) free(argp);
#ifdef DEBUG
	printf(outbuf);
	printf("Done\n");
#endif

	argp = NULL;
	(void) admin_add_argp(&argp, IPADDR_PARAM, ADMIN_STRING, 
	    strlen(ipaddr), ipaddr);
#ifdef DEBUG
	printf("Setting IP address to %s...", ipaddr);
#endif
	if ((status = admin_perf_method(SYSTEM_CLASS_PARAM, SYS_IPADDR_METHOD, 
	    ADMIN_LOCAL, NULL, argp, &outbuf, &errbuf, ADMIN_END_OPTIONS)) !=
	    SUCCESS) {
		admin_write_err(errbuf);
		admin_write_err(SYS_ERR_SET_IPADDR, SYS_ADD_METHOD);
		exit(status);
	}
	(void) free(argp);
#ifdef DEBUG
	printf(outbuf);
	printf("Done\n");
#endif

	argp = NULL;
	(void) admin_add_argp(&argp, TIMEZONE_PARAM, ADMIN_STRING, 
	    strlen(timezone), timezone);
#ifdef DEBUG
	printf("Setting timezone to %s...", timezone);
#endif
	if ((status = 
	    admin_perf_method(SYSTEM_CLASS_PARAM, SYS_TIMEZONE_METHOD, 
	    ADMIN_LOCAL, NULL, argp, &outbuf, &errbuf, ADMIN_END_OPTIONS)) !=
	    SUCCESS) {
		admin_write_err(errbuf);
		admin_write_err(SYS_ERR_SET_TZ, SYS_ADD_METHOD);
		exit(status);
	}
	(void) free(argp);
#ifdef DEBUG
	printf(outbuf);
	printf("Done\n");
#endif

	argp = NULL;
	(void) admin_add_argp(&argp, TIMESERVER_PARAM, ADMIN_STRING, 
	    strlen(timeserver), timeserver);
#ifdef DEBUG
	printf("Syncing time with server %s...", timeserver);
#endif
	if ((status = admin_perf_method(SYSTEM_CLASS_PARAM, SYS_TIME_METHOD, 
	    ADMIN_LOCAL, NULL, argp, &outbuf, &errbuf, ADMIN_END_OPTIONS)) !=
	    SUCCESS) {
		admin_write_err(errbuf);
		admin_write_err(SYS_ERR_SET_TIME, SYS_ADD_METHOD);
		exit(status);
	}
	(void) free(argp);

#ifdef DEBUG
	printf(outbuf);
	printf("Done\n");
#endif
	exit(SUCCESS);
}


