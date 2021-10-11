#ifndef lint
static  char sccsid[] = "@(#)tapeblock.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * tapeblock - buffer input to a certain amount and send it to
 *		output file...
 *
 */

#define	FULLBLOCK

#include <fcntl.h>

main(argc,argv)
char **argv;
{
	char *malloc();
	register rin, fd, amt;
	register char *base, *p;
	register unsigned int obsize;

	if(argc < 3)
	{
		(void) write(2,"No file specified\n",18);
		exit(1);
	}
        if (strcmp(*++argv, "-") == 0) {
                fd = 1;
        } else if ((fd = open(*argv, O_WRONLY)) < 0) {
                (void) write(2, "Could not open ", 15);
                (void) write(2, *argv, strlen(*argv));
                (void) write(2, "\n", 1);
                exit(1);
        }
 
	obsize = atoi(*++argv);

	if((base = malloc(obsize+3)) == (char *) 0)
	{	
		(void) write(2,"No Memory\n",10);
		exit(1);
	}

	while((int)base & 0x3)
		base++;
	p = base;
	amt = obsize;

	while((rin = read(0,p,amt)) > 0)
	{
		p += rin;
		if(p == base + obsize)
		{
			if(write(fd,base,(int)obsize) != obsize)
			{
				(void) write(2,"Error in flush\n",15);
				exit(1);
			}
			p = base;
			amt = obsize;
		}
		else
			amt = obsize - (int) (p - base);
	}

	if(rin < 0)
	{
		(void) write(2,"Error in read\n",14);
		exit(1);
	}
	else if(p > base)
	{
#ifdef	FULLBLOCK
		while(p < base+obsize)
			*p++ = 0;

		if(write(fd,base,(int)obsize) != obsize)
#else
		if(write(fd,base,p-base) != p-base)
#endif
		{
			(void) write(2,"Error in final flush\n",15);
			exit(1);
		}
	}
	close(fd);
	exit(0);
	/* NOTREACHED */
}
