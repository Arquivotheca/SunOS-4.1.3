#ifndef lint
static  char sccsid[] = "@(#)fit_file.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * fit_file - fit table of contents onto a set of distribution media
 *
 *	We calculate the size of a 'volume' in the distribution here.
 *
 *	We then decide how to fit the files listed in the entries[]
 *	table into 'n' volumes. FIX ME - Right now, no 'fitting' is done:
 *	Things are taken sequentially from the beginning of the table
 *	(first fit). It would be desirable to perhaps put up a choice
 *	screen for an operator.
 *
 *	Note Well: For each volume in the distribution, the first
 *	two table of contents entries (i.e., the table of contents
 *	in XDR and Ascii form) are replicated as the first 'files'
 *	on that volume.
 *
 *	This is true unless the global value of 'nodup' is set.
 */

#include "mktp.h"

static	char vflag = 0;
static	char nodup = 0;
static	unsigned bytes_per_volume;
static	int volno;
static	int filnum;

main(argc,argv)
int argc;
char **argv;
{
	extern void remove_toc_copies();
	char *p;

	if(argc < 3)
	{
usage:
		(void) fprintf(stderr,"Usage: %s [-vn] infile outfile\n",*argv);
		exit(1);
	}


	p = argv[1];

	if(*p == '-')
	{
		while(*++p)
		{
			if(*p == 'v')
			{
				setbuf(stdout,(char *) 0);
				vflag = 1;
			}
			else if(*p == 'n')
				nodup = 1;
			else
				goto usage;
		}
		argv++;
	}

	if(!get_tables(argv[1]))
		exit(-1);
	else if(dinfo.dstate&(SIZED|PARSED) != (SIZED|PARSED))
	{
		(void) fprintf(stderr,
"I can't very well FIT files if I don't know their bloody size, now can I?\n");
		exit(-1);
	}

	if(!nodup)
		remove_toc_copies();

	if(fit_file() < 0)
		exit(-1);
	else
	{
		if(!nodup)
		{
			dinfo.dstate &= ~EDITTED;
			dinfo.dstate |= FITTED;
		}
		exit(dump_tables(argv[2])?0:1);
	}
	/* NOTREACHED */
}

static int
fit_file()
{
	register toc_entry *eptr = entries;
	register unsigned thisvolsize;
	int (*setaddress)(), (*setup_a_func())();

	/*
	 * 1) Get value of number of bytes per volume
	 *	Check to make sure that each volume can
	 *	hold at least the TOC entries plus at least the
	 *	largest other entry...
	 *	...Note! not necessary for diskettes, 'cause 
	 *	they have multi-volume archive capability...
	 *
 	 *	Initialize a few values.
	 */

	bytes_per_volume = Size_of_Volumes();

	if((eptr->Where.dtype == TAPE) && (Check_Size(bytes_per_volume) < 0))
	{
		(void) fprintf(stderr,
			"Can't fit TOC and entries onto a single volume\n");
		return(-1);
	}



	/*
	 * Volumes are counted from 1 on up....
	 */
	volno = 1;
	/*
	 * File numbers are counted from 0 on up
	 */
	filnum = 0;

	/*
	 * 2) Set up a pointer to the function that will actually stuff
	 * the appropriate values into each TOC entry. This function
	 * will work off of the external static values of 'thisvolume'
	 * and 'filnum', and return either the size contained in a
	 * 'new' volume, or else the increment of the size of the
	 * just added entry.
	 *
	 */

	setaddress = setup_a_func();

	/*
 	 * 3) Now walk through table of contents, assigning
	 * address information as we go. Remember: each time we go to a new
	 * volume, we have to duplicate the first two entries
	 * as the first entries for that volume... for tapes - yes,
	 * but not necessary for diskettes.
	 */

	if(eptr->Where.dtype == TAPE)
	{
	    for(eptr = entries, thisvolsize = 0; eptr < ep; )
	    {
		if(thisvolsize + Size_of_Entry(eptr) > bytes_per_volume)
		{
			if(vflag)
				(void) fprintf(stdout,
					"\t%d files (%d bytes)  on volume %d\n",
						filnum,thisvolsize,volno);
			thisvolsize = (*setaddress)(eptr,1);
			eptr += (nodup)?1:2;
		}
		else
		{
			thisvolsize += (*setaddress) (eptr,0);
			eptr++;
		}
	    }

	    if(vflag)
	    {
		(void) fprintf(stdout,"\t%d files (%d bytes)  on volume %d\n",
			filnum,thisvolsize,volno);
		(void) fprintf(stdout,"\t%d Total Volumes\n",volno);
	    }
	}
	else	/* DISKETTE */
	{

		volno = (*setaddress)(entries,bytes_per_volume);
		if(vflag)
			(void) fprintf(stdout,"\t%d Total Volumes\n",volno);
	}
	return(0);
}

static int
(*setup_a_func())()
{
	/*
	 * FIX ME - need to put in (maybe) a routine that
	 * stuffs values for disk distributions..its been done
	 */
	register toc_entry *eptr = entries;
	int	tapeset();
	int	diskset();

	if (eptr->Where.dtype == TAPE)
		return(tapeset);
	else
		return(diskset);
}

static int
tapeset(eptr,mknewvol)
register toc_entry *eptr;
int mknewvol;
{
	auto char local[16];
	register unsigned sz;
	register tapeaddress *ta;
	extern char *sprintf();

	/*
	 * If we have to make a new volume, now is the place to
	 * do it.
	 *
	 */

	sz = 0;
	if(mknewvol)
	{
		volno++;
		filnum = 0;

		if(!nodup)
		{
			register toc_entry *ep1, *ep2;
			/*
			 * shove rest of list up one place to make room
			 * for duplicates of TOC.
			 */


			for(ep1 = ep, ep2 = ep+1; ep1 >= eptr; ep1--, ep2--)
				*ep2 = *ep1;
			ep = ep + 1;

			dup_entry(eptr,&entries[0]);
			sz = tapeset(eptr,0);
			eptr++;
		}
	}

	eptr->Where.dtype = TAPE;
	ta = &eptr->Where.Address_u.tape;

	/*
	 * Stuff address info...
	 */

	ta->volno = volno;
	ta->label = newstring(sprintf(local,"VOLUME%u",volno));
	ta->file_number = filnum++;
	sz += Size_of_Entry(eptr);
	return(sz);
}

/*
 * fit onto floppy diskettes - called once to fit all onto diskettes
 *	(new-new) way of putting stuff onto diskettes is to bar each catagory
 * into a file (stashed into a scratch directory) and then dd each of those
 * files onto a set of diskettes packed together, which saves 9 or 10 diskettes.
 * To allow a shell script with xdrtoc and dd to find the file, the fileno
 * field is used as an offset (in bytes).
 *	Note also that the TOC and copyright are put onto a separate partition,
 * so they don't take any room on the volume.  Also, the boot,munixfs and
 * miniroot sets are made outside of mktp anyway.
 */
static int
diskset(eptr,bytes_per_volume)
register toc_entry *eptr;
register unsigned bytes_per_volume;
{
	auto char local[16];
	register unsigned sz;
	register diskaddress *da;
	extern char *sprintf();
	int	volno;
	unsigned thisvolsize;
	unsigned offset;

	offset = 0;
	for( volno=1, thisvolsize = bytes_per_volume; eptr < ep; eptr++) {

		/*
		 * If this is a table of contents, skip it.  They don't take
		 * any space (at least on the partition bar uses).
		 */
		if( IS_TOC(eptr) ) {
			continue;
		}

		/*
		 * if entry is a data image, skip it (should'nt even be in toc!)
		 */
		if (eptr->Type == IMAGE) {
/*XXX maybe we should print a message ??? */
			continue;
		}

		if( thisvolsize < 0 ) {
			fprintf(stderr, "horrible error in diskette sizing\n");
				exit(-1);
		}
		if( thisvolsize == 0 ) {
			/* need to start a new volume */
			volno++;
			offset = 0;
			thisvolsize = bytes_per_volume;
		}

		/*
		 * Stuff address info...
		 */
		da = &eptr->Where.Address_u.disk;
		da->volno = volno;
		da->label = newstring(sprintf(local,"VOLUME%u",volno));
		da->offset = offset;

		/*
		 * If entry overflows this diskette, we have to determine
		 * how many diskettes it adds to the running total
		 * else, we just add the size to this diskette.
		 */
		sz = Size_of_Entry(eptr);
		/* round up to 512   (hopefully compiler is smart) */
/*XXX maybe we should round up to 18b ??? */
		sz = ((sz + 511)/512)*512;

		do {
			/* if this file takes all or more than all of rest of
			 * diskette...
			 */
			if( sz >= thisvolsize ) {
				volno++;
				sz -= thisvolsize;
				/* start all over again, leave space for */
				/* bar volume and file headers */
				thisvolsize = bytes_per_volume;
				offset = 0;
			} else {
				/* this file doesn`t fill this diskette */
				thisvolsize -= sz;
				offset += sz;
				sz = 0;
			}
		} while( sz );
	}
	return(volno);
}
