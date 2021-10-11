#ifndef lint
static	char sccsid[] = "@(#)save.c 1.1 92/07/30 SMI"; /* from UCB 4.1 82/05/11 */
#endif
/* save (III)   J. Gillogly
 * save user core image for restarting
 * usage: save(<command file (argv[0] from main)>,<output file>)
 * bugs
 *   -  impure code (i.e. changes in instructions) is not handled
 *      (but people that do that get what they deserve)
 */

#include <a.out.h>
int filesize;                    /* accessible to caller         */

#ifdef sun
#define	PGSIZE	8192
#endif
#ifdef vax
#define	PGSIZE	1024
#endif
#ifndef PGSIZE
#define	PGSIZE	512
#endif

char *sbrk();

save(cmdfile,outfile)                   /* save core image              */
char *cmdfile,*outfile;
{       register char *c;
	register int i,fd;
	int fdaout;
	struct exec header;
	int counter;
	char buff[PGSIZE],pwbuf[120];
	fdaout=getcmd(cmdfile);         /* open command wherever it is  */
	if (fdaout<0) return(-1);       /* can do nothing without text  */
	if ((fd=open(outfile,0))>0)     /* this restriction is so that  */
	{       printf("Can't use an existing file\n"); /* we don't try */
		close(fd);              /* to write over the commnd file*/
		return(-1);
	}
	if ((fd=creat(outfile,0755))== -1)
	{       printf("Cannot create %s\n",outfile);
		return(-1);
	}
	/* can get the text segment from the command that we were
	 * called with, and change all data from uninitialized to
	 * initialized.  It will start at the top again, so the user
	 * is responsible for checking whether it was restarted
	 * could ignore sbrks and breaks for the first pass
	 */
	read(fdaout,&header,sizeof header);/* get the header       */
	header.a_bss = 0;                  /* no data uninitialized        */
	header.a_syms = 0;                 /* throw away symbol table      */
	switch (header.a_magic)            /* find data segment            */
	{   case 0407:                     /* non sharable code            */
		c = (char *) header.a_text;/* data starts right after text */
		header.a_data=sbrk(0)-c;   /* current size (incl allocs)   */
		break;
	    case 0410:                     /* sharable code                */
		c = (char *)
#ifdef pdp11
		    (header.a_text	   /* starts after text            */
		    & 0160000)             /* on an 8K boundary            */
		    +  020000;             /* i.e. the next one up         */
#endif
#ifdef vax
		    (header.a_text	   /* starts after text            */
		    & 037777776000)        /* on an 1K boundary            */
		    +        02000;        /* i.e. the next one up         */
#endif
#ifdef z8000
		    (header.a_text	   /* starts after text            */
		    & 0174000)             /* on an 2K boundary            */
		    +  004000;             /* i.e. the next one up         */
#endif
#ifdef sun
		    0;
		c = (char *)N_DATADDR(header);
#ifdef notdef
		    (header.a_text	   /* starts after text            */
		    & ~0x1ffff)            /* on a 128K boundary            */
		    +  0x20000;            /* i.e. the next one up         */
#endif
#endif
		header.a_data=sbrk(0)-c;   /* current size (incl allocs)   */
		break;
	    case 0411:                     /* sharable with split i/d      */
		c = 0;                     /* can't reach text             */
		header.a_data=(int)sbrk(0);/* current size (incl allocs)   */
		break;
	    case 0413:
#ifdef sun
		c = (char *)(header.a_text /* starts after text            */
		    & ~0x1ffff)             /* on a 128K boundary            */
		    +  0x20000;            /* i.e. the next one up         */
#else
		c = (char *) header.a_text;/* starts after text            */
#endif
#ifdef vax
		lseek(fdaout, 1024L, 0);   /* skip unused part of 1st block*/
#endif
#ifdef sun
		lseek(fdaout, 0L, 0);      /* skip to beginning of 1st block*/
#endif
	}
	if (header.a_data<0)               /* data area very big           */
		return(-1);                /* fail for now                 */

	filesize=sizeof header+header.a_text+header.a_data;
	write(fd,&header,sizeof header);   /* make the new header          */
	if (header.a_magic==0413)
#ifdef vax
		lseek(fd, 1024L, 0);       /* Start on 1K boundary	   */
#endif
#ifdef sun
		lseek(fd, 0L, 0);          /* Start at beginning of 1st block*/
#endif
	counter=header.a_text;             /* size of text                 */
	while (counter>PGSIZE)             /* copy PGSIZE-byte blocks      */
	{       read(fdaout,buff,PGSIZE);  /* as long as possible          */
		write(fd,buff,PGSIZE);
		counter -= PGSIZE;
	}
	read(fdaout,buff,counter);         /* then pick up the rest        */
	write(fd,buff,counter);
	write(fd,c,header.a_data);         /* write all data in 1 glob     */
#ifdef sun
	if (header.a_magic == 0413) {
		/* rewrite the header, since writing the text overwrites it */
		lseek(fd, 0L, 0);          /* Start at beginning of 1st block*/
		write(fd,&header,sizeof header);   /* make the new header  */
	}
#endif
	close(fd);
}

#define	NULL	0

char	*execat(), *getenv();

getcmd(command)         /* get command name (wherever it is) like shell */
char *command;
{
	char *pathstr;
	register char *cp;
	char fname[128];
	int fd;

	if ((pathstr = getenv("PATH")) == NULL)
		pathstr = ":/bin:/usr/bin";
	cp = index(command, '/')? "": pathstr;

	do {
		cp = execat(cp, command, fname);
		if ((fd=open(fname,0))>0)
			return(fd);
	} while (cp);

	printf("Couldn't open %s\n",command);
	return(-1);
}

static char *
execat(s1, s2, si)
register char *s1, *s2;
char *si;
{
	register char *s;

	s = si;
	while (*s1 && *s1 != ':' && *s1 != '-')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}
