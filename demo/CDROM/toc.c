/*************************************************************
 *                                                           *
 *                     file: toc.c                           *
 *                                                           *
 *************************************************************/

/*
 * Copyright (C) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef lint
static char sccsid[] = "@(#)toc.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/*
 * This file contains code for the table of contents data structure 
 */

#include <stdio.h>
#include "msf.h"
#include "toc.h"

/************************ entry points ***********************/

Toc
init_toc(start, end, total_msf)
int	start, end;
Msf	total_msf;
{
	Toc	toc;

	toc = (Toc)malloc(sizeof(struct toc));
	toc->start_track = start;
	toc->end_track = end;
	toc->toc_list = (Track_entry *)malloc(sizeof(Track_entry) * 
					      ((end - start) + 1));
	toc->size = 0;
	toc->total_msf = total_msf;
	return (toc);
}

void
add_track_entry(toc, track_number, control, msf, next_msf)
Toc	toc;
int	track_number;
int	control;
Msf	msf;
Msf	next_msf;
{
	register Track_entry	track_entry;

	track_entry = (Track_entry)malloc(sizeof(struct track_entry));
	track_entry->track_number = track_number;
	track_entry->control = control;
	track_entry->msf = msf;
	track_entry->duration = diff_msf(next_msf, msf);
	toc->toc_list[toc->size++] = track_entry;
}

void
destroy_toc(toc)
Toc	toc;
{
	free(toc->toc_list);
	free(toc);
}

Msf
get_track_duration(toc, track_number)
Toc	toc;
int	track_number;
{
	Track_entry	te;

	if ((track_number > toc->end_track) ||
	    (track_number < toc->start_track)) {
		return (Msf)NULL;
	}
	te = toc->toc_list[track_number - toc->start_track];
	return (te->duration);
}

int
get_track_control(toc, track_number)
Toc	toc;
int	track_number;
{
	Track_entry	te;

	if ((track_number > toc->end_track) ||
	    (track_number < toc->start_track)) {
		return (-1);
	}
	te = toc->toc_list[track_number - toc->start_track];
	return (te->control);
}

