#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)getpass.c 1.1 92/07/30 SMI"; /* from S5R3 1.10 */
#endif
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#include <stdio.h>
#include <signal.h>
#include <termios.h>

extern void setbuf();
extern FILE *fopen();
extern int fclose(), fprintf(), findiop();
extern int kill(), ioctl(), getpid();
static int intrupt;

#define	MAXPASSWD	8	/* max significant characters in password */

char *
getpass(prompt)
char	*prompt;
{
	struct termios ttyb;
	unsigned long flags;
	register char *p;
	register int c;
	FILE	*fi;
	static char pbuf[ MAXPASSWD + 1 ];
	struct sigvec osv, sv;
	void	catch();

	if((fi = fopen("/dev/tty", "r")) == NULL)
#ifdef S5EMUL
		return((char*)NULL);
#else
		fi = stdin;
#endif
	else
		setbuf(fi, (char*)NULL);
	sv.sv_handler = catch;
	sv.sv_mask = 0;
	sv.sv_flags = SV_INTERRUPT;
	(void) sigvec(SIGINT, &sv, &osv);
	intrupt = 0;
	(void) ioctl(fileno(fi), TCGETS, &ttyb);
	flags = ttyb.c_lflag;
	ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	(void) ioctl(fileno(fi), TCSETSF, &ttyb);
	(void) fputs(prompt, stderr);
	p = pbuf;
	while( !intrupt  &&
		(c = getc(fi)) != '\n'  &&  c != '\r'  &&  c != EOF ) {
		if(p < &pbuf[ MAXPASSWD ])
			*p++ = c;
	}
	*p = '\0';
	ttyb.c_lflag = flags;
	(void) ioctl(fileno(fi), TCSETSW, &ttyb);
	(void) putc('\n', stderr);
	(void) sigvec(SIGINT, &osv, (struct sigvec *)NULL);
	if(fi != stdin)
		(void) fclose(fi);
#ifdef S5EMUL	/* XXX - BOTH versions should probably do this! */
	if(intrupt)
		(void) kill(getpid(), SIGINT);
#endif
	return(pbuf);
}

static void
catch()
{
	++intrupt;
}
