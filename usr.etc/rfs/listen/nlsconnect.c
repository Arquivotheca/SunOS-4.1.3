/*	@(#)nlsconnect.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:nlsconnect.c	1.6"

/*
 *		 Network listener process "start server" library routines
 *		 UNIX PX STARLAN NETWORK only.
 *		 Clients use this lib function to connect to server process
 *		 on a remote machine:
 *
 *		int nlsconnect(remote, net_device, svc_code);
 *			char *remote;	
 *			char *net_device; 
 *			int svc_code;
 *		where:
 *			remote is null terminated nodename of remote host.
 *			net_device is TLI pseudo device name (see below).
 *			svc_code is listener's service code of server.
 *
 *		returns:-1 error occurred. see below.
 *			other = fd in DATAXFER state connected to
 *				remote server process (see TLI).
 *
 *		net_device should be "/dev/starlan".  This is a 
 *		parameter so we can support more than one TLI network.
 *		If net_device is NULL, "/dev/starlan" is used.
 *
 *		If an error occurrs, t_errno will contain an error code.
 *		if t_errno is zero (set by nlsconnect) it means the
 *		remote name is too long (longer than a nodename.)
 *
 *		Setting the external integer "_nlslog" to any non-zero
 *		value before calling nlsconnect,  will cause nlp_connect
 *		to print debug information on stderr.
 *
 *		client/server process pairs should include their own
 *		initial handshake to insure connectivity.
 *
 *		If succesful, the address of the t_call structure used
 *		to bind the loceal endpoint is stored in the static
 *		location _nlscall, which is overwritten by each call
 *		to this routine.  The caller can either t_free it, record
 *		it for later use or ignore it.
 *
 * SEE ALSO:
 *	TLI documentation, listen(1), uname(2), lname2addr(3), nlsname(3).
 *
 */


#include	<stdio.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<errno.h>

#include	<sys/utsname.h>		/* for sizeof nodename */

#include	<tiuser.h>
#include	"listen.h"

#define	DEF_NETNAME	"/dev/starlan"

extern	int _nlslog;		/* non-zero allows use of stderr	*/

extern struct t_call *_nlscall;	/* used during t_connect to server	*/

static void
logmessage(s)
char *s;
{
	if (_nlslog)
		fprintf(stderr,s);
}


int
nlsconnect(remote, net_device, svc_code)
char *remote;
char *net_device;
int  svc_code;
{
	int	netfd = -1;
	int	len, err;
	char	*namep;
	char 	buf[128], logbuf[128];
	struct	t_call *sndcall;
	extern  char *nlsname();
	extern  int t_errno;
	extern	struct netbuf *lname2addr();

	t_errno = 0;		/* indicates a 'name' problem	*/
	buf[0] = 0;

	if (strlen(remote) > sizeof(utsname.nodename))  {
		sprintf(buf,"remote name (%s) is too long",remote);
		goto error;
	}

	if (!(namep = nlsname(remote)))
		goto error;

	if (!net_device)
		net_device = DEF_NETNAME;

	if ((netfd = t_open(net_device, O_RDWR, NULL)) < 0) {
		sprintf(buf, "t_open %s failed", net_device);
		goto error;
	}

	if (!(sndcall = (struct t_call *)t_alloc(netfd, T_CALL, T_ALL))) {
		sprintf(buf, "t_alloc failed");
		goto error;
	}

	_nlscall = sndcall;

	if (t_bind (netfd, NULL, NULL) < 0) { /* bind default TLI name */
		sprintf(buf, "t_bind to default name failed");
		goto error;
	}

	(void)memcpy(sndcall->addr.buf, namep, strlen(namep));
	sndcall->addr.len = strlen(namep);

	/*
	 * run the name through lname2addr to normalize it for the
	 * transport provider
	 */

#ifdef S4
	if (!lname2addr(netfd, &(sndcall->addr)))  {
		t_errno = TSYSERR;	/* set for error: below	*/
		sprintf(buf, "lname2addr failed, errno %d", errno);
		goto error;
	}
#endif

	if (t_connect (netfd, sndcall, NULL) < 0) {
		sprintf(buf, "t_connect failed");
		goto error;
	}

	/*
	 * send protocol message requesting the service
	 */

	len = sprintf(buf, nls_prot_msg, svc_code)+1;/* inc trailing null */

	if (t_snd(netfd, buf, len, 0) < len) {
		sprintf(buf, "t_snd failed to send protocol message");
		goto error;
	}

	return(netfd);

	/*
	 * all errors come here: try closing, etc, before returning.
	 */

error:
	sprintf(logbuf, "%s: %s", remote, 
	    ((buf[0] ? buf:"remote name is too long")));
	if (_nlslog)  {
		if  (t_errno)
			t_error(logbuf);
		else
			fprintf(stderr, 
			    "%s: given remote name is too long\n", remote);
	}

	if (netfd >= 0)  {
		err = t_errno;		/* try cleaning up	*/
		if (sndcall)  {
			t_unbind(netfd);
			t_free(sndcall, T_CALL);
			_nlscall = (struct t_call *)0;
		}
		if (t_close(netfd))
			close(netfd);	/* if all else fails, at least close */
		t_errno = err;
	}

	return(-1);
}
