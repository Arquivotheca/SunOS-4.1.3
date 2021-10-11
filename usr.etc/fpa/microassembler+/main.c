#ifndef lint
static  char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *	Micro-assembler -- main driver
 *		main.c	1.1	31 May 1984
 */

#include "micro.h"
#include <stdio.h>

char * curfilename = "<stdin>";
short curaddr, maxaddr;
int curlineno;
Boolean dflag = False;
Boolean waserror = False;
Boolean Scanning = False;
NODE n[NNODE];
NODE *curnode = n-1;
CODE code[ NNODE ];
CODE *curword;
char *abinfile = NULL;
char *binfile = NULL;

main(argc, argv)
    int argc;
    char **argv;
{
    int c;
    char *ap;

    for (c=1; c<argc; c++) {
	ap = argv[c];
	if (ap[0] == '-') {
	    switch (ap[1]) {
		case 'o':
			if (freopen(argv[++c], "w", stdout) == NULL) 
	    		    fatal("could not open file %s",argv[c]);
			continue;
		case 'a':
			abinfile = argv[++c];
			continue;
		case 'b':
			binfile = argv[++c];
			continue;
		default:
			help();
			fatal("bad option %s", argv[c]);
	    }
	} else {
	    curfilename = argv[c];
	    if (freopen(argv[c], "r", stdin) == NULL) 
	    	fatal("could not open file %s",argv[c]);
	}
    }
    Scanning = True;
    curaddr = 4096;
    init_symtab();
    resw_hash(); /* hash keywords */
    init_assm();
    scanprog();
    Scanning = False;
/*    if (waserror) exit(1);    */
    resolve_addrs();
    restrict();              /* rectriction checking */
    output();
    if (waserror) exit(1);
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

help()
{
	fprintf( stderr, "usage: fpas [-o file] [-a file] [-b file]\n" );
	fprintf( stderr, "\t-o: output listing to file\n" );
	fprintf( stderr, "\t-a: output ascii (pseudo binary) to file\n" );
	fprintf( stderr, "\t-b: output binary to file\n" );
}
