#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setpgrp.c 1.1 92/07/30 SMI";
#endif

extern int	setsid();

int
setpgrp()
{

	return (setsid());
}
