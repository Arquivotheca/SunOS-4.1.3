#ifndef lint
static	char sccsid[] = "@(#)sys_ipaddr.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/* 
 * sys_ipaddr IPADDR=ipaddr
 *
 * Administrative method to set the host's IP address.
 */

#include <stdio.h>
#include <sys/param.h>
#include "admin_amcl.h"
#include "add_key_entry.h"
#include "sys_param_names.h"
#include "admin_messages.h"

#define HOSTS_FILE	"/etc/hosts"
#define LOCALHOST_ADDR	"127.0.0.1"
#define LOCALHOST_NAME	"localhost"

static int set_ipaddr();

extern int add_key_entry();
extern int admin_get_param();
extern void admin_write_err();

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char addr[16];		/* IP address to assign */
	int status;

        if (admin_get_param(argc, argv, IPADDR_PARAM, addr, sizeof(addr)) == 
	    -1) {
		admin_write_err(SYS_ERR_NO_IPADDR, SYS_IPADDR_METHOD);
		exit(FAIL_CLEAN);
	}

	if ((status = set_ipaddr(addr)) != SUCCESS)
		admin_write_err(SYS_ERR_SET_IPADDR, SYS_IPADDR_METHOD);
	exit(status);
}

/*
 * set_ipaddr(addr) adds or replaces the IP address entry for this host in
 * the hosts file.  Also rewrites localhost entry to take loghost off it.
 */
static int
set_ipaddr(addr)
	char *addr;
{
	char	hostname[MAXHOSTNAMELEN];	/* Current name */
	char 	value[3*MAXHOSTNAMELEN];	/* Entry for file */
	int	status;				/* Return status */

	/* Get our name from kernel so we can fix up hosts file */	
	if (gethostname(hostname, sizeof(hostname)) != 0) {
		perror("gethostname");
		return (FAIL_CLEAN);
	} else {
		/*
		 * Replace localhost entry with one which looks like what
		 * suninstall/sys-config put in.
		 */
		if ((status = add_key_entry(LOCALHOST_ADDR, LOCALHOST_NAME,
		    HOSTS_FILE, KEY_ONLY)) != SUCCESS)
			admin_write_err(SYS_ERR_ADD_ENTRY, "set_ipaddr",
			    HOSTS_FILE);
		else {
			/*
			 * Typical entry looks like:
			 * 192.9.200.1	foo loghost
			 * add_key_entry knows how to deal with this format 
			 * and will replace it
			 */
			sprintf(value, "%s loghost", hostname);
			if ((status = add_key_entry(addr, value, HOSTS_FILE, 
			    KEY_OR_VALUE)) != SUCCESS)
				admin_write_err(SYS_ERR_ADD_ENTRY, 
				    "set_ipaddr", HOSTS_FILE);
		}
		return (status);
	}
}
	
	
