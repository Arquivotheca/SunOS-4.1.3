/*	@(#)llib-lsa.c 1.1 92/07/30 SMI	*/

    /* LINTLIBRARY */

#include <sys/types.h>
#include <mon/cpu.map.h>

int	poke(a, c) short *a; int c; { a=a; c=c; return 0; }
int	pokec(a, c) char *a; int c; { a=a; c=c; return 0; }
int	peek(a) caddr_t a; { a=a; return 0; }
	setsegmap(a, v) caddr_t a; int v; {a=a; v=v;}
	getidprom(a, s) unsigned char *a; unsigned s; {a=a; s=s;}
	setpgmap(a, v) caddr_t a; struct pgmapent *v; {a=a; v=v;}
/* this declaration gets fixed when <mon/cpu.map.h> does */
int /*struct pgmapent*/	getpgmap(a) caddr_t a;
		{ /*struct pgmapent b; return b;*/ a=a; return 0; }
