#ifndef lint
static char sccsid[] = "@(#)hostname.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <pwd.h>
#include <nse/util.h>

static char	hostname[NSE_MAXHOSTNAMELEN] = { '\0' };

char *
_nse_hostname()
{
	if (!*hostname) {
		gethostname(hostname, sizeof(hostname));
	}
	return hostname;
}

Nse_err
_nse_get_user(user, homedir)
	char		*user;
	char		*homedir;
{
	struct passwd	*pwd;

	pwd = getpwuid(getuid());
	if (pwd == NULL) {
		return nse_err_format_msg("Can't determine user name for userid %d", getuid());
	}
	if (user != NULL) {
		strcpy(user, pwd->pw_name);
	}
	if (homedir != NULL) {
		strcpy(homedir, pwd->pw_dir);
	}
	return (NSE_OK);
}
