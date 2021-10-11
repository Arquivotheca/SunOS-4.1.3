/*
 * sys_time TIMESERVER=servername
 *
 * Administrative method to roughly synchronize the system's date/time clock 
 * with that of	a remote server.
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)sys_time.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <sys/errno.h>
#include <sys/param.h>
#include "admin_amcl.h"
#include "sys_param_names.h"
#include "admin_messages.h"

#define	DATE_COMMAND "/usr/ucb/rdate"

extern int admin_get_param();
extern void admin_write_err();

static int set_date();

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char server[MAXHOSTNAMELEN];	/* Server to be synced with */
	int status;

	if (admin_get_param(argc, argv, TIMESERVER_PARAM, server, sizeof(server)) == -1) {
		admin_write_err(SYS_ERR_NO_TIMESERV, SYS_TIME_METHOD);
		exit(FAIL_CLEAN);
	}

	if ((status = set_date(server)) != SUCCESS)
		admin_write_err(SYS_ERR_SET_TIME, SYS_TIME_METHOD);
	exit(status);
}

/*
 * set_date(server) calls 'rdate' to sync the clock with a server.
 */
static int
set_date(server)
	char *server;			/* Server to be synced with */
{
	char command_line[1024];	/* Command line to be issued */
	int status;

	sprintf(command_line, "%s %s", DATE_COMMAND, server);
	if ((status = system(command_line)) != 0) {
		admin_write_err(SYS_ERR_SYS_CMD, "set_date", DATE_COMMAND,
		    status);
		return (FAIL_CLEAN);
	} else
		return (SUCCESS);
}
