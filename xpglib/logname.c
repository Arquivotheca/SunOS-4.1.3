#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)logname.c 1.1 92/07/30 SMI";
#endif

#define NAME "LOGNAME"

extern char *getenv();


char *logname()
{
	return (getenv(NAME));
}
