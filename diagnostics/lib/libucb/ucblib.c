static char sccsid[] = "@(#)ucblib.c	1.1 7/30/92 Copyright Sun Microsystems Inc.";

#ifdef SVR4
#include <sys/unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>

int
gethostname(name,length)
char *name ;
int length ;
{
	return(sysinfo(SI_HOSTNAME,name,length));
}

gethostid()
{
	char buf[SYS_NMLN+1] ;

	sysinfo(SI_MACHINE,buf,SYS_NMLN);
	if ( strcmp(buf,"sun4c") )
	    return(0x20000000) ;
	else
	    return(0x50000000) ; /* hard coded for sun4c */
}

getpagesize()
{
	return(sysconf(_SC_PAGESIZE)) ;
}

bzero(b,l)
char *b;
int l ;
{
	memset((void *)b,0,(size_t)l);
}

char *
index(s,c)
char *s, c ;
{
	while ( *s && *s != c ) s++ ;
	if ( *s ) return(s) ;
	else return(0);
}

int
bcmp(b1,b2,length)
char *b1, *b2 ;
int length ;
{
	return(memcmp((void *)b1,(void *)b2,(size_t)length));
}

bcopy(b1,b2,length)
char *b1, *b2 ;
int length ;
{
	memcpy((void *)b2,(const void *)b1,(size_t)length);
}

getdtablesize()
{
	struct rlimit rlp ;

	getrlimit(RLIMIT_NOFILE,&rlp);
	return(rlp.rlim_cur);
}
#endif SVR4
