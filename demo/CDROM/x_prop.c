/*
 *  prop.c - xview support for guide generated property sheet
 *  rct - 4/90
 */

#ifndef lint
static char sccsid[] = "@(#)prop.c @(#) 92/08/10 x_prop.c 1.1 (C) SMI - rct";
#endif

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
/*#include <gdd.h>*/
#include "prop_ui.h"
#include "cdrom.h"
#include "msf.h"
#include "toc.h"
#include "cdrom.h"
#include "player.h"

extern	prop_prop_objects	*prop_objs;


void
prop_notify(menu, menu_item)
Menu	menu;
Menu_item	menu_item;
{
	xv_set(prop_objs->prop, XV_SHOW, TRUE, 0);
	prop_showing++;
}


void
prop_init()
{
	char buf[30];

	sprintf(buf,"CD Player - Properties  (%s - rct)",Version);

	xv_set(prop_objs->leftbal_slider, PANEL_VALUE, left_balance,
	     PANEL_NOTIFY_LEVEL, PANEL_ALL, 0);
	xv_set(prop_objs->rightbal_slider, PANEL_VALUE, right_balance,
	     PANEL_NOTIFY_LEVEL, PANEL_ALL, 0);

	xv_set(prop_objs->idle_choice, PANEL_VALUE, idle_mode, 0);
	xv_set(prop_objs->idle_item, PANEL_VALUE, idle_time, 0);
	/* xv_set(prop_objs->idle_choice, PANEL_INACTIVE, idle_mode, 0); */
	xv_set(prop_objs->mode_choice, PANEL_VALUE, play_mode, 0);
	xv_set(prop_objs->prop, FRAME_LABEL, buf, 0);
}


/*
 * Done callback function for `prop'.
 */
void
prop_done(frame)
	Frame		frame;
{
	DPRINTF(2)("prop: prop_done\n");
	xv_set(frame, XV_SHOW, FALSE, 0);
	prop_showing=0;
}

/*
 * Notify callback function for `mem_list'.
 */
int
mem_list_notify(item, string, client_data, op, event)
	Panel_item	item;
	char		*string;
	Xv_opaque	client_data;
	Panel_list_op	op;
	Event		*event;
{
	int	track;
	
	switch(op) {
	case PANEL_LIST_OP_DESELECT:
		DPRINTF(8)("mem_list: OP_DESELECT: %d - %s\n",client_data,string);
		break;

	case PANEL_LIST_OP_SELECT:
		DPRINTF(8)("mem_list: OP_SELECT: %d - %s\n",client_data,string);
		break;

	case PANEL_LIST_OP_VALIDATE:
	/*
	 * this needs work.  - I don't know how to get the row number
	 * here so I don't know how to correctly set up the strings
	 * or put anything useful in client_data
	 */
		DPRINTF(5)("mem_list: OP_VALIDATE: %d - %s\n",client_data,string);

		if (playing) return XV_ERROR;

		if (sscanf(string,"%d",&track) < 1) return XV_ERROR;

		if (track < toc->start_track || track > toc->end_track)
			return XV_ERROR;
		add_memory_track(track, FALSE);
		break;

	case PANEL_LIST_OP_DELETE:
		DPRINTF(4)("mem_list: OP_DELETE: %d - %s\n",client_data,string);

		if (playing) return XV_ERROR;

		delete_memory_track(client_data, item);
		break;
	}
	return XV_OK;
}

/*
 * Notify callback function for `track_list'.
 */
int
track_list_notify(item, string, client_data, op, event)
	Panel_item	item;
	char		*string;
	Xv_opaque	client_data;
	Panel_list_op	op;
	Event		*event;
{
	
	switch(op) {
	case PANEL_LIST_OP_DESELECT:
		DPRINTF(7)("track_list: PANEL_LIST_OP_DESELECT: %s\n",string);
		break;

	case PANEL_LIST_OP_SELECT:
		DPRINTF(5)("track_list: PANEL_LIST_OP_SELECT: %s\n",string);
		add_memory_track(client_data, TRUE);
		break;

	case PANEL_LIST_OP_VALIDATE:
		fprintf(stderr, "prop: mem_list_notify: PANEL_LIST_OP_VALIDATE: %s\n",string);
		break;

	case PANEL_LIST_OP_DELETE:
		fprintf(stderr, "mem_list: OP_DELETE - READ ONLY %d - %s\n ",
		    client_data,string);
		break;
	}
	return XV_OK;
}


/*
 * Notify callback function for `mode_choice'.
 */
void
play_mode_notify(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	int	i;

	DPRINTF(4)("prop: play_mode_notify: value: %u\n", value);

	if (ejected) {
	    play_mode=PLAY_MODE_NORMAL;
	    xv_set(item, PANEL_VALUE, play_mode, 0);
	    return;
	}

	play_mode=value;
	switch(play_mode) {
	case PLAY_MODE_NORMAL:
	    mem_cleanup();
	    break;

	case PLAY_MODE_MEMORY:
	    break;

	case PLAY_MODE_RANDOM:
	    xv_set(prop_objs->mem_list, PANEL_PAINT, PANEL_NONE, 0);
	    shuffle();
	    xv_set(prop_objs->mem_list, XV_SHOW, TRUE, 0);
	    xv_set(item, PANEL_VALUE, PLAY_MODE_MEMORY, 0);
	    break;

	default:
		fprintf(stderr,"play_mode: unknown choice %d\n",value);
		break;
	}
}


/*
 * Notify callback function for `guage_choice'.
 */
void
guage_notify(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	time_remain_flag = value;
	set_remain_guage();
}

/*
 * Notify callback function for `leftbal_slider'.
 */
void
leftbal_notify(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	left_balance = value;
	set_volume();
}

/*
 * Notify callback function for `rightbal_slider'.
 */
void
rightbal_notify(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	right_balance = value;
	set_volume();
}

/*
 * Notify callback function for `idle_choice'.
 */
void
idle_notify(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	DPRINTF(6)("prop: idle_choice %d",value); 
	idle_mode = value;

#ifdef NOTDEF
	if (value) xv_set(prop_objs->idle_item, PANEL_INACTIVE, FALSE, 0);
	else xv_set(prop_objs->idle_item, PANEL_INACTIVE, TRUE, 0);
#endif

	if (!value && idled) {
	    idled = 0;
	    pause_proc(NULL, NULL);
	}
}

/*
 * Notify callback function for `idle_item'.
 */
Panel_setting
idle_value_notify(item, event)
	Panel_item	item;
	Event		*event;
{
	idle_time = xv_get(prop_objs->idle_item, PANEL_VALUE);

	DPRINTF(6)("prop: idle_value: value: %d\n", idle_time);
}


void
display_mem_tracks(ntracks)
int	ntracks;
{
	char	buf[20];

	sprintf(buf,"%02d",ntracks);
	xv_set(prop_objs->track_mess, PANEL_LABEL_STRING, buf, 0);

	sprintf(buf,"%02d:%02d",mem_total_msf.min, mem_total_msf.sec);
	xv_set(prop_objs->time_mess, PANEL_LABEL_STRING, buf, 0);
}


void
mem_play_init()
{
    int		i, *mp;
    char	str[10];
	
    for (i = 0, mp=mem_play; i < mem_ntracks; i++, mp++) {
	if (sscanf(xv_get(prop_objs->mem_list, PANEL_LIST_STRING, i),"%d",mp)
	!= 1 ) 
	   fprintf(stderr,"mem_play_init: Bad track number %s in list %d\n",
	       xv_get(prop_objs->mem_list, PANEL_LIST_STRING, i), i);
    }

    *mp = -1;
    mem_current_track = 0;
    mem_cur_msf.min = 0;
    mem_cur_msf.sec = 0;

}


void
mem_cleanup()
{
	int i, n;
	char	str[10];

        n = xv_get(prop_objs->mem_list, PANEL_LIST_NROWS);

	for (i = 0; i < n; i++)
		xv_set(prop_objs->mem_list, 
		    PANEL_LIST_DELETE, 0,
		    PANEL_PAINT, PANEL_NONE,
		0);

	xv_set(prop_objs->mem_list, XV_SHOW, TRUE, 0);
	mem_ntracks=0;
	mem_current_track = -1;
	mem_total_msf.min = 0;
	mem_total_msf.sec = 0;
	display_mem_tracks(mem_ntracks);

}


void
set_remain_guage()
{
	int	i, n;
	Msf	cur_msf;

	if (playing && current_track != -1) {
	    switch(time_remain_flag) {
	    case TIME_REMAIN_TRACK:
		    cur_msf = get_track_duration(toc, current_track);
		    if (cur_msf == NULL) return;
		    i = (tmp->tm_min * 60 + tmp->tm_sec) * 100 /
			(cur_msf->min * 60 + cur_msf->sec);
		    break;
	    case TIME_REMAIN_DISC:
		    if (play_mode == PLAY_MODE_NORMAL) {
			cur_msf = init_msf();
			cdrom_get_absmsf(fd, cur_msf);
			i = (cur_msf->min * 60 + cur_msf->sec) * 100 /
			    (total_msf->min * 60 + total_msf->sec); 
			free(cur_msf);
		    } else {
			i = (tmp->tm_min * 60 + tmp->tm_sec +
			    mem_cur_msf.min * 60 + mem_cur_msf.sec) * 100 /
			    (mem_total_msf.min * 60 + mem_total_msf.sec);
		    }
		    break;
	    default: 
		    fprintf(stderr,"%s: unknown setting %d for time remaining\n",
			    ProgName, time_remain_flag);
		    break;
	    }
	} else i=0; /* not currently playing */

	DPRINTF(6)("remain guage %d\n", i);

	/* I shouldn't have to do this.
	 * but the flashing is too annoying so I will try to cut
	 * down on the number of paints
	 */

	if (i != last_guage_value) {
	    xv_set(prop_objs->guage1, PANEL_VALUE, i, 0);
	    last_guage_value=i;
	}
}


void
add_memory_track(track,update)
int	track;
int	update;
{
	char	buf[20];

	if (mem_ntracks == MAX_TRACKS) return;

	if (check_data_track(track)) {
	    DPRINTF(2)("add_memory_track: attempt to add data track\n");
	    return;
	}

	sprintf(buf,"%02d",track);
	if (update) {
		xv_set(prop_objs->mem_list,
		    PANEL_LIST_INSERT, mem_ntracks,
		    PANEL_LIST_STRING, mem_ntracks, buf,
		    PANEL_LIST_CLIENT_DATA, mem_ntracks, track,
		0);
	}

	mem_ntracks++;
	acc_msf(&mem_total_msf, get_track_duration(toc, track));
	display_mem_tracks(mem_ntracks);

	if (playing) {
	    mem_play[mem_ntracks-1] = track;
	}
}

void
delete_memory_track(track,item)
int	track;
Panel_item	item;
{

	if (playing) return;

	mem_ntracks--;
	mem_total_msf = sub_msf(mem_total_msf, get_track_duration(toc, track));
	display_mem_tracks(mem_ntracks);
}


int
search_memlist(track)
{
	int	i;

	for(i=0; i < mem_ntracks; i++)
		if (track == mem_play[i]) return(i);

	return(-1);
}


void
shuffle()
{
	int	i, remain, n, r;
	struct	rt {
	    	int	track;
		struct	rt *next, *prev;
	};
	struct	rt	*tracks, *cur_track;

	mem_cleanup();

	srandom(time((time_t *) 0));

	if ((tracks = (struct rt *)malloc(sizeof(struct rt) * num_tracks)) == NULL) {
	    perror("Malloc of shuffle track buffer failed");
	    exit(-1);
	}
	
	for (i = 1, cur_track=tracks; i <= num_tracks; i++, cur_track++) {
		/* XXX - should check for data tracks here */
		cur_track->track = i;
		cur_track->next = cur_track + 1;
		if (i > 1) cur_track->prev = cur_track - 1;
	}

	/* make list circular */
	tracks->prev = cur_track - 1;
	(cur_track-1)->next = tracks;
	cur_track = tracks;

	remain = num_tracks;

	for (i = 0; i < num_tracks - 1; i++) { /* pick each track */
	     /* r = random() % remain; */
	     /* bigger random interval - possibly longer period? */
	     /* but now spinning more times than necessary */
		r = random() % num_tracks;
DPRINTF(9)("random number %d\n", r);		
		
	    for (n = 0; n < r; n++) {
		cur_track = cur_track->next;
	    }

	    if (cur_track == NULL || cur_track->track < 1 ||
		cur_track->track > num_tracks) {
		    fprintf(stderr,"shuffle: random track generation routine failure\n");
		    abort();
	    }

	    add_memory_track(cur_track->track, TRUE);
	    
	    cur_track->prev->next=cur_track->next;
	    cur_track->next->prev=cur_track->prev;

	    remain--;
	    cur_track->track = -1; /* safety */
	    cur_track->prev = NULL;

	    cur_track=cur_track->next;

	}

	/* at the end of the loop there should only be one track
	 * remaining.  (No need to go through random loop to find
	 * one track...)
	 */

	if (cur_track->next != cur_track || cur_track->prev != cur_track ) 
		fprintf(stderr,
    "shuffle: last track doesn't pointer back to itself - something wrong\n");

	add_memory_track(cur_track->track, TRUE);

	free(tracks);
}


void
check_idle()
{
	time_t	t;
	struct	stat	sbuf;

	t = time((time_t *) 0);
	/* test mouse first - more likely to change */
	if (fstat(ms_fd, &sbuf) < 0) {
	    fprintf(stderr,"%s: idle disabled - fstat of mouse failed",
		ProgName);
	    perror("");
	    idle_mode = 0;
            xv_set(prop_objs->idle_choice, PANEL_INACTIVE, TRUE,
                PANEL_VALUE, idle_mode, 0);
            xv_set(prop_objs->idle_item, PANEL_INACTIVE, TRUE, 0);
	    return;
	}
	
	if ((t - sbuf.st_atime) < idle_time) {
	    if (!idled) return;
	    /* no longer idle */
	    idled = 0;
	    pause_proc(NULL, NULL);
	    return;
	}

	/* mouse idle - now check kbd */
	if (fstat(kbd_fd, &sbuf) < 0) {
	    fprintf(stderr,"%s: idle disabled - fstat of keyboard failed",
		ProgName);
	    perror("");
	    idle_mode = 0;
            xv_set(prop_objs->idle_choice, PANEL_INACTIVE, TRUE,
                PANEL_VALUE, idle_mode, 0);
            xv_set(prop_objs->idle_item, PANEL_INACTIVE, TRUE, 0);
	    return;
	}
	
	if ((t - sbuf.st_atime) < idle_time) {
	    if (!idled) return;
	    /* no longer idle */
	    idled = 0;
	    pause_proc(NULL, NULL);
	    return;
	}

	if (!idled) {
	    idled = 1;
	    xv_set(msg_item, PANEL_LABEL_STRING, "Idle", 0);
	    pause_proc(NULL, NULL);
	}
}
