#ifndef lint
static  char sccsid[] = "@(#)volsub.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * volume subroutines
 *
 */

#include "mktp.h"

int
IsNewVol(curv,eptr)
toc_entry *eptr;
{
	if(curv != Volume_of_Entry(eptr))
		return(1);
	else
		return(0);
}

int
Get_Vol_Mounted(volume)
register volume;
{
	char buf[32];
	(void) fprintf(stdout,
		"Mount volume %d - hit <RETURN> when ready: ",volume);
	if(gets(buf) == NULL)
		return(-1);
	else
		return(0);
}

int
Volume_of_Entry(eptr)
toc_entry *eptr;
{
	if(eptr->Where.dtype == TAPE)
		return(eptr->Where.Address_u.tape.volno);
	else if(eptr->Where.dtype == DISK)
		return(eptr->Where.Address_u.disk.volno);
	else
	{
		(void) fprintf(stderr,"Volume_of_Entry- unknown media type\n");
		exit(1);
		/*NOTREACHED*/
	}
}


/*
 * Set new volume...
 */

int
Set_New_Vol(eptr,newvol)
toc_entry *eptr;
int newvol;
{
	extern char *realloc(), *sprintf(), *strcpy();
	auto char local[16];

	if(eptr->Where.dtype == TAPE)
	{
		tapeaddress *ta;
		ta = &eptr->Where.Address_u.tape;
		ta->volno = newvol;
		ta->label = newstring(sprintf(local,"VOLUME %d",newvol));
		ta->file_number = Entry_in_Volume(eptr);
		return(0);
	}
	else if(eptr->Where.dtype == DISK)
	{
		diskaddress *da;
		da = &eptr->Where.Address_u.disk;
		da->volno = newvol;
		da->label = newstring(sprintf(local,"VOLUME %d",newvol));
		/* offset? */
		return(0);
	}
	else
		return(-1);
}


/*
 * get the number of volumes in a distribution
 */

int
getnvols()
{
	register toc_entry *eptr;
	register cv = 0, nv = 0;

	for(eptr = entries; eptr < ep; eptr++)
		if(IsNewVol(cv,eptr))
		{
			cv = Volume_of_Entry(eptr);
			nv++;
		}
	return((entries[0].Where.dtype == TAPE) ? nv : cv);
}

/*
 * return the number of entries in a volume
 *  - return 0 if illegal volume
 *
 *	assumes entries table sorted by volume....
 */

int
getnent(vol)
{
	register toc_entry *eptr;
	register nent = 0;

	/*
	 * find correct volume
 	 */
	for(eptr = entries; eptr < ep; eptr++)
		if(Volume_of_Entry(eptr) == vol)
			break;

	while(eptr < ep)
		if(Volume_of_Entry(eptr) == vol)
		{
			nent++;
			eptr++;
		}
		else
			break;

	return(nent);
}

/*
 * return the ordinal number of an entry in its volume (for tapes)
 * for diskettes, just return the ordinal entry number
 */

int
Entry_in_Volume(ent)
toc_entry *ent;
{
	register toc_entry *eptr;
	register int vol, filnum;

	if(ent->Where.dtype == TAPE)
	{
		vol = Volume_of_Entry(ent);

		for(eptr = entries; eptr < ep; eptr++)
			if(vol == Volume_of_Entry(eptr))
				break;

		for(filnum = 0; eptr < ep; eptr++, filnum++)
		{
			if(Volume_of_Entry(eptr) != vol)
				break;
			else if(eptr == ent)
				return(filnum);
		}
	} else /* DISK */
		return(ent - entries);
	return(-1);
}

/*
 * return the sum of all sizes for a particular volume
 */

int
sumvolume(vol)
{
	register toc_entry *eptr;
	register long sum = 0;

	/*
	 * sum all entries in a particular volume
	 *
	 */

	for(eptr = entries; eptr < ep; eptr++)
	{
		if(Volume_of_Entry(eptr) == vol)
			sum += Size_of_Entry(eptr);
	}

	return(sum);
}

/*
 * copy an address structure
 */

copyaddress(from,to)
register Address *from, *to;
{
	to->dtype = from->dtype;
	if(to->dtype == TAPE)
	{
		to->Address_u.tape.volno = from->Address_u.tape.volno;
		to->Address_u.tape.size = from->Address_u.tape.size;
		to->Address_u.tape.volsize = from->Address_u.tape.volsize;
		to->Address_u.tape.file_number =
				from->Address_u.tape.file_number;
		to->Address_u.tape.dname =
			newstring(from->Address_u.tape.dname);
		to->Address_u.tape.label =
			newstring(from->Address_u.tape.label);
	}
	else if(to->dtype == DISK)
	{
		to->Address_u.disk.volno = from->Address_u.disk.volno;
		to->Address_u.disk.size = from->Address_u.disk.size;
		to->Address_u.disk.volsize = from->Address_u.disk.volsize;
		to->Address_u.disk.offset = from->Address_u.disk.offset;
		to->Address_u.disk.dname =
			newstring(from->Address_u.disk.dname);
		to->Address_u.disk.label =
			newstring(from->Address_u.disk.label);
	}
}


/*
 * destroy_vinfo - destroy specific volume information
 *	(i.e., file_number/offset, volno, label)
 *
 */


destroy_vinfo()
{
	register toc_entry *eptr;

	for(eptr = entries; eptr < ep; eptr++)
	{
		Set_New_Vol(eptr,-1);
		eptr->Where.Address_u.disk.offset = -1;
	}
}

/*
 * entrytovol - convert the ordinal number of an entry to diskette volume
 *	ie.	[1] Make volume #1	(munix)
 *		[2] Make volume #2	(munixfs)
 *		[3] Make volume #3	(miniroot)
 *		[4] Make volume #6	(root)
 *		[5] Make volume #13	(usr)
 * 	Note - a table of contents marks the beginning of a new archive.
 */

int
entrytovol(num)
register int num;
{
	register toc_entry *eptr;
	register int	   toc;

	for (eptr=entries,toc = 0; (toc < num) && (eptr < ep); eptr++)
		/* each TOC signifies the beginning
		 * of a new volume entry
		 */
		if (IS_TOC(eptr))
			toc++;

	return(Volume_of_Entry(eptr));
}
