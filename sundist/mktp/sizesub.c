#ifndef lint
static  char sccsid[] = "@(#)sizesub.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 * size related subroutines
 *
 *
 */

#include "mktp.h"


int
Size_of_Entry(eptr)
toc_entry *eptr;
{
        if(eptr->Where.dtype == TAPE)
                return(eptr->Where.Address_u.tape.size);
        else if(eptr->Where.dtype == DISK)
                return(eptr->Where.Address_u.disk.size);
        else
                return(0);
}

void
Set_Size_of_Entry(eptr,size)
toc_entry *eptr;
int size;
{
        if(eptr->Where.dtype == TAPE)
                eptr->Where.Address_u.tape.size = size;
        else if(eptr->Where.dtype == DISK)
                eptr->Where.Address_u.disk.size = size;
}

int
Size_of_Volumes()
{
        register r, max = 0;
        register toc_entry *eptr;

        for(eptr = entries; eptr < ep; eptr++)
                if((r = Size_of_Volume(eptr)) > max)
                        max = r;

        return(r);
}

int
Size_of_Volume(eptr)
toc_entry *eptr;
{
        if(eptr->Where.dtype == TAPE)
                return(eptr->Where.Address_u.tape.volsize);
        else if(eptr->Where.dtype == DISK)
                return(eptr->Where.Address_u.disk.volsize);
        else
                return(1);
}


int
Check_Size(max)
unsigned max;
{
	register toc_entry *eptr;
	register tocsize, size, msize;

	/*
	 * Find size of largest entry
	 */

	for(eptr = entries, tocsize = msize = 0; eptr < ep; eptr++)
	{
		if(msize < (size = Size_of_Entry(eptr)))
			msize = size;
		if(IS_TOC(eptr) && !tocsize)
			tocsize = Size_of_Entry(eptr);
	}
	/*
	 * make sure that toc and largest entry will fit
	 * onto a single volume.
	 *
	 */

	if(msize+tocsize > max)
		return(-1);
	else
		return(0);
}

void
destroy_sizeinfo()
{
	register toc_entry *eptr;
	for(eptr = entries; eptr < ep; eptr++)
	{
		if(!IS_TOC(eptr))
			Set_Size_of_Entry(eptr,-1);
	}
}
