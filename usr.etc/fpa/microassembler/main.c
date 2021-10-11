/*	@(#)main.c 1.1 92/07/30 SMI	*/
/*
 *	Micro-assembler -- main driver
 *		main.c	1.1	31 May 1984
 */

#include "micro.h"
#include <stdio.h>

char * curfilename = "<stdin>";
short curaddr;
int curlineno;
Boolean dflag = False;
Boolean waserror = False;
Boolean Scanning = False;
NODE n[NNODE];
RAMNODE ram[NINST];
NODE *curnode = n-1;

main(argc, argv)
    int argc;
    char **argv;
{
    int c;
    char *ap;
    FILE *bout_stream = NULL;
    FILE *tbl_stream = NULL;

    for (c=1; c<argc; c++) {
	ap = argv[c];
	if (ap[0] == '-') {
	    switch (ap[1]) {
		case 'o':
			if (freopen(argv[++c], "w", stdout) == NULL) 
	    		    fatal("could not open file %s",argv[c]);
			continue;
		case 'b':
			if ((bout_stream = fopen(argv[++c], "w")) == NULL) 
	    		    fatal("could not open file %s",argv[c]);
			continue;
		case 'r':
			if ((tbl_stream = fopen(argv[++c], "w")) == NULL) 
	    		    fatal("could not open file %s",argv[c]);
			continue;
		default:
			fatal("bad option %s", argv[c]);
	    }
	} else {
	    curfilename = argv[c];
	    if (freopen(argv[c], "r", stdin) == NULL) 
	    	fatal("could not open file %s",argv[c]);
	}
    }
    Scanning = True;
    curaddr = 0;
    init_symtab();
    resw_hash(); /* hash keywords */
    init_assm();
    scanprog();
    Scanning = False;
/*    if (waserror) exit(1);    */
    resolve_addrs();
    restrict();              /* rectriction checking */
    output(bout_stream, tbl_stream);
    if (waserror) exit(1);
    
    exit(0);
    /* NOTREACHED */
}

fatal(s, a)
    char *s, *a;
{
    if (Scanning) {
    	printf("File %s, line %d: %s\n", curfilename, curlineno, s);
    }
    printf(s, a);
    printf("\n");
    exit(1);
    /*_cleanup();
    abort();*/
}

error(s, a, b, c, d, e, f)
    char *s;
{
    if (Scanning) {
	printf("File %s, line %d: ", curfilename, curlineno);
    }
    printf(s, a, b, c, d, e, f);
    printf("\n");
    waserror = True;
}

warn(s, a, b, c, d, e, f)
    char *s;
{
    printf("Warning:");
    if ( Scanning) {
	printf(" file %s, line %d: ", curfilename, curlineno);
    }
    printf(s, a, b, c, d, e, f);
    printf("\n");
}
