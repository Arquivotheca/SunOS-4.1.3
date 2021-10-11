/*	@(#)nlsrequest.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:nlsrequest.c	1.4"

/*
 *
 * nlsrequest(3):
 *
 *		Send service request message to remote listener
 *		on previously established virtual circuit to remote
 *		listener process.
 *
 *		If an error occurrs, t_errno will contain an error code.
 *
 *		Setting the external integer "_nlslog" to any non-zero
 *		value before calling nlsrequest,  will cause nlsrequest
 *		to print debug information on stderr.
 *
 *		client/server process pairs should include their own
 *		initial handshake to insure connectivity.
 *
 *		This version of nlsrequest includes the
 *		service request response message.
 */


#include	<stdio.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<string.h>
#include	<tiuser.h>
#include	"listen.h"
#include	"lsdbf.h"

extern	int _nlslog;		/* non-zero allows use of stderr	*/
char *_nlsrmsg = (char *)0;
static char _nlsbuf[256];


int
nlsrequest(fd, svc_code)
int  fd;
char *svc_code;
{
	int	len, err, flags;
	char 	buf[256];
	char	*p;
	int	version, ret;
	extern  int t_errno;

	t_errno = 0;		/* indicates a 'name' problem	*/
	buf[0] = 0;

	/*
	 * Validate service code
	 */

	if (!svc_code || !strlen(svc_code) || (strlen(svc_code) >= SVC_CODE_SZ)) {
		if (_nlslog)
			fprintf(stderr, "nlsrequest: invalid service code format\n");
		return(-1);
	}

	/*
	 * send protocol message requesting the service
	 */

	len = sprintf(buf, nls_v2_msg, svc_code)+1;/* inc trailing null */

	if (t_snd(fd, buf, len, 0) < len) {
		if (_nlslog)
			t_error("t_snd of listener request message failed");
		return(-1);
	}

	p = _nlsbuf;
	len = 0;

	do {
		if (++len > sizeof(_nlsbuf)) {
			if (_nlslog)
				fprintf(stderr, "nlsrequest: _nlsbuf not large enough\n");
			return(-1);
		}
		if (t_rcv(fd, p, sizeof(char), &flags) != sizeof(char)) {
			if (_nlslog)
				t_error("t_rcv of listener response msg failed");
			return(-1);
		}

	} while (*p++ != '\0');


	if ((p = strtok(_nlsbuf, ":")) == (char *)0)
		goto parsefail;
	version = atoi(p);

	if ((p = strtok((char *)0, ":")) == (char *)0)
		goto parsefail;
	ret = atoi(p);
	_nlsrmsg = p + strlen(p) + 1;
	if (ret && _nlslog)
		fprintf(stderr, "%s\n", _nlsrmsg); /* debug only */
	return(ret);

parsefail:
	if (_nlslog)
		fprintf(stderr, "nlsrequest: failed parse of response message\n");
	return(-1);
}
