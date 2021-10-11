#ifndef lint
static  char sccsid[] = "@(#)calc_sizes.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * calculate file sizes
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "mktp.h"

static char vflag;
static char *Size = " SIZE=size ";	/* makefile param to size archive */

main(argc,argv)
int argc;
char **argv;
{
	char *p;

	if(argc < 3)
	{
		(void) fprintf(stderr,"Usage: %s [-v] infile outfile\n",*argv);
		exit(1);
	}


	p = argv[1];

	if(p[0] == '-' && p[1] == 'v')
	{
		setbuf(stdout,(char *) 0);
		vflag = 1;
		argv++;
	}

	if(!get_tables(argv[1]))
		exit(-1);
	else if(calcsizes() < 0)
		exit(-1);
	else
	{
		destroy_vinfo();
		dinfo.dstate = SIZED|PARSED;
		exit(dump_tables(argv[2])?0:1);
	}
	/* NOTREACHED */
}

static int
calcsizes()
{
	extern void Set_Size_of_Entry();
	register size, nbytes;
	register FILE *ip;
	register toc_entry *eptr;
	FILE *popen(); 
	char cmdstring[BUFSIZ], path[BUFSIZ];
	struct stat buf;
	char *tarfile, *archstring, *getenv();
	int retval;

	for(nbytes = 0, eptr = entries; eptr < ep; eptr++)
	{
		if(IS_TOC(eptr))	/* skip TOCs */
		{
			if(vflag) (void) fprintf(stdout,
				"\t...Calculating size for XDR TOC:");
			size = Size_of_Entry(eptr);
		} else if (eptr->Info.kind == INSTALLABLE) {
			/* just stat the tarfile */
			if ((tarfile = getenv("TARFILES")) == NULL) {
				fprintf(stderr, " No Environment Variable - TARFILES");
				return(-1);
			}
			if ((archstring = getenv("ARCH")) == NULL) {
				fprintf(stderr, " No Environment Variable - ARCH");
				return(-1);
			}
;
			if (eptr->Type == TARZ) {
			       sprintf(path, "%s/%s.Z/%s", tarfile, archstring, 
								eptr->Name);
			}
			else { /* tar */
			       sprintf(path, "%s/%s/%s", tarfile, archstring, 
								eptr->Name);
			}
			if ((retval = stat(path,&buf)) == -1) {
				fprintf(stdout, "could not stat %s\n", path);	
				return(-1);
			}
			else if (vflag)
				(void) fprintf(stdout,
				"\t...Calculating size for entry '%s':",
				eptr->Name);
			size = buf.st_size;
			Set_Size_of_Entry(eptr,size);
		} else {
			/* this just runs the command and counts the bytes */
			(void) strcpy(cmdstring, eptr->Command);

			size = 0;
			if((ip = popen(cmdstring,"r")) == NULL)
  			{
  				(void) fprintf(stderr,
  					"calc_sizes: cannot invoke '%s'\n",
  					eptr->Command);
  				return(-1);
  			}
  			else if(vflag)
  				(void) fprintf(stdout,
  				"\t...Calculating size for entry '%s':",
  				eptr->Name);
  
  			while(getc(ip) != EOF)
  				size++; 

			if(ferror(ip))
 			{
 				(void) fprintf(stderr,
 					"Error in reading input\n");
 				exit(-1);
 			}
 			(void) pclose(ip); 

			Set_Size_of_Entry(eptr,size);
		}

		nbytes += size;

		if(vflag)
			(void) fprintf(stdout," %d bytes\n",
				Size_of_Entry(eptr));

	}

	if(vflag)
		(void) fprintf(stdout,"\nTotal of %d bytes\n",nbytes);
	return(0);
}
