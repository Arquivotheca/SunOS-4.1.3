/*************************************************************
 *                                                           *
 *                     file: msf.c                           *
 *                                                           *
 *************************************************************/

/*
 * Copyright (C) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef lint
static char sccsid[] = "@(#)msf.c 1.1 92/07/30 Copyr 1989, 1990 Sun Micro";
#endif

/*
 * This file contains code for the MSF CDROM address format data structure 
 */

#include "msf.h"

/************************ entry points ***********************/

Msf
init_msf()
{
	Msf	msf;

	msf = (Msf)malloc(sizeof(struct msf));
	msf->min = 0;
	msf->sec = 0;
	msf->frame = 0;
	return (msf);
}

/* 
 * for now, forget about the frames, just get the min and sec difference
 */
Msf
diff_msf(msf1, msf2)
Msf	msf1;
Msf	msf2;
{
        Msf	msf;

	msf = init_msf();
	if (msf1->sec < msf2->sec) {
		msf1->sec += 60;
		msf1->min--;
	}
	msf->sec = msf1->sec - msf2->sec;
	msf->min = msf1->min - msf2->min;
	msf->frame = 0;
	return (msf);
}

struct msf
add_msf(msf1, msf2)
Msf msf1, msf2;
{
    int seconds;
    struct  msf rmsf;

    seconds = msf1->min * 60 + msf1->sec +
        msf2->min * 60 + msf2->sec;

    rmsf.min = seconds / 60;
    rmsf.sec = seconds % 60;
    rmsf.frame = 0;

    return(rmsf);
}

struct msf
sub_msf(msf1, msf2)
Msf msf1, msf2;
{
    int seconds;
    struct  msf rmsf;
 
    seconds = (msf1->min * 60 + msf1->sec) -
        (msf2->min * 60 + msf2->sec);
 
    rmsf.min = seconds / 60;
    rmsf.sec = seconds % 60;
    rmsf.frame = 0;
 
    return(rmsf);
}

/* msf1 += msf2 */
void
acc_msf(msf1, msf2)
Msf msf1, msf2;
{
    int seconds;
 
    seconds = msf1->min * 60 + msf1->sec +
        msf2->min * 60 + msf2->sec;
 
    msf1->min = seconds / 60;
    msf1->sec = seconds % 60;
 
    /* currently not expecting negative time values */
    if (msf1->min < 0) msf1->min = 0;
    if (msf1->sec < 0) msf1->sec = 0;
}
 
int
cmp_msf(msf1, msf2)
Msf msf1, msf2;
{
    int m1, m2;
 
    m1 = (msf1->min*60 + msf1->sec) * 255 + msf1->frame;
    m2 = (msf2->min*60 + msf2->sec) * 255 + msf2->frame;
 
    return(m1 - m2);
}
