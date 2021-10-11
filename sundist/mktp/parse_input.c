#ifndef lint
static  char sccsid[] = "@(#)parse_input.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 *	parse_input.c - parse named input file and dump output to named
 *			intermediate file
 */

#include "mktp.h"

char *rname = (char *) 0;
int volsize = -1;


main(argc,argv)
int argc;
char *argv[];
{
	extern Set_Size_of_Entry();
	extern int yyanyerrors;
	extern FILE *yyin, *yyout;
	register toc_entry *eptr;
	tapeaddress *tp;
	extern int dump_tables();

	if(argc < 3)
	{
		(void) fprintf(stderr,"Usage: %s infile outfile\n",argv[0]);
		exit(1);
	}
	else if((yyin = fopen(argv[1],"r")) == NULL)
	{
		(void) fprintf(stderr,"%s: cannot open input file %s\n",argv[0],
			argv[1]);
		exit(1);
	}

	/*
	 * 1) Parse input file.
	 *	This will fill in entries in the table from 1..n
	 */



	yyout = stderr;
	ep = entries+1;

	if(yyparse() || yyanyerrors != 0)
	{
		(void) fprintf(stderr,"Parse of input file '%s' failed\n",
			argv[1]);
		exit(-1);
	}

	(void) fclose(yyin);

	/*
	 * 2) Make dummy entry for the xdr toc itself (entries[0])
	 *
	 */

	eptr = entries;
	eptr->Command = newstring("exit");
	eptr->Name = newstring("XDRTOC");
	eptr->LongName = newstring("Binary XDR readable table of contents");
	eptr->Type = TOC;
	eptr->Info.kind = CONTENTS;

	copyaddress(&(eptr+1)->Where,&eptr->Where);
	/* For right now, I'll fake it and say that it is  4k in size. */
        Set_Size_of_Entry(eptr,(u_long) 4096);

	/*
 	 * 3) Flesh out the dummy volume info (which isn't valid
	 * until the distribution is made.
	 */

	if (eptr->Where.dtype == TAPE) {
		vinfo.vaddr.dtype = TAPE;
		tp = &vinfo.vaddr.Address_u.tape;
	} else {
		vinfo.vaddr.dtype = DISK;
		(diskaddress *) tp = &vinfo.vaddr.Address_u.disk;
	}
	tp->volno = tp->file_number = -1;
	tp->label = newstring("VOLUME0");
	tp->dname = newstring(rname);
	tp->volsize = volsize;
	tp->size = -1;
	/*
	 * 4) Dump all tables, and exit.
	 *
	 */

	dinfo.dstate = PARSED;
	exit(dump_tables(argv[2])?0:1);
	/* NOTREACHED */
}
