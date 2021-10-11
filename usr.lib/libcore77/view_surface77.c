/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)view_surface77.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../libcore/coretypes.h"

int deselect_view_surface();
int initialize_view_surface();
int select_view_surface();
int terminate_view_surface();

#define TRUE 1
#define FALSE 0

#define DEVNAMESIZE 20

int deselectvwsurf_(surfname)
struct vwsurf *surfname;
	{
	return(deselect_view_surface(surfname));
	}

int initializevwsurf_(surfname, flag)
    struct vwsurf *surfname;
    int *flag;
{
    return(initializevwsurf(surfname, flag));
}

int initializevwsurf(surfname, flag)
struct vwsurf *surfname;
int *flag;
	{
	int r;
	char *sptr;
	short blankpad[3];

	sptr = surfname->screenname;
	blankpad[0] = FALSE;
	while (sptr < &surfname->screenname[DEVNAMESIZE])
		{
		if (*sptr == '\0')
			break;
		else if (*sptr == ' ')
			{
			blankpad[0] = TRUE;;
			*sptr = '\0';
			break;
			}
		sptr++;
		}
	sptr = surfname->windowname;
	blankpad[1] = FALSE;
	while (sptr < &surfname->windowname[DEVNAMESIZE])
		{
		if (*sptr == '\0')
			break;
		else if (*sptr == ' ')
			{
			blankpad[1] = TRUE;;
			*sptr = '\0';
			break;
			}
		sptr++;
		}
	sptr = surfname->cmapname;
	blankpad[2] = FALSE;
	while (sptr < &surfname->cmapname[DEVNAMESIZE])
		{
		if (*sptr == '\0')
			break;
		else if (*sptr == ' ')
			{
			blankpad[2] = TRUE;;
			*sptr = '\0';
			break;
			}
		sptr++;
		}
	r = initialize_view_surface(surfname, *flag);
	if (blankpad[0])
		{
		sptr = surfname->screenname;
		while (*sptr++)
			;
		--sptr;
		while (sptr < &surfname->screenname[DEVNAMESIZE])
			*sptr++ = ' ';
		}
	if (blankpad[1])
		{
		sptr = surfname->windowname;
		while (*sptr++)
			;
		--sptr;
		while (sptr < &surfname->windowname[DEVNAMESIZE])
			*sptr++ = ' ';
		}
	if (blankpad[2])
		{
		sptr = surfname->cmapname;
		while (*sptr++)
			;
		--sptr;
		while (sptr < &surfname->cmapname[DEVNAMESIZE])
			*sptr++ = ' ';
		}
	return(r);
	}

int selectvwsurf_(surfname)
struct vwsurf *surfname;
	{
	return(select_view_surface(surfname));
	}

int terminatevwsurf_(surfname)
struct vwsurf *surfname;
	{
	return(terminate_view_surface(surfname));
	}
