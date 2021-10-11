#ifndef lint
static  char sccsid[] = "@(#)fv.comm.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <fcntl.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
static Event Bottom_event;		/* Fake L5 event */
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

#define MAXCLIENT	64
static int Client[MAXCLIENT];		/* Who we can send signals to */
static int Nclient;			/* Current number */
static int Current_client;		/* Current 'browsing' client */
static char *Back_path;			/* Folder prior to browse */
static BOOLEAN Was_closed;		/* Frame closed? */

void
fv_init_comm()				/* Tell the world our process id */
{
	int fd;
	char buf[6];

	if ((fd = open("/tmp/.fv_dev", O_CREAT|O_WRONLY|O_TRUNC)) != -1)
	{
		(void)sprintf(buf, "%d", getpid()); 
		(void)write(fd, buf, strlen(buf));
		/* Allow everybody to read/write file */
		(void)fchmod(fd, 00666);
		(void)close(fd);
	}
}


/*ARGSUSED*/
Notify_value
fv_catch_signal(me, sig, when)	/* Requests to select and load files */
	Notify_client me;	/* unused */
	int sig;
	Notify_signal_mode when;/* unused */
{
	char buf[1024];
	int fd;
	int n;
	register char *b_p;

	if (sig == SIGUSR1)
	{
		/* Someone wants to be signaled if we drop over them */

		if ((fd = open("/tmp/.fv_in", O_RDONLY)) != -1)
		{
			n = read(fd, buf, 20);
			buf[n] = 0;
			Client[Nclient++] = atoi(buf);
			(void)close(fd);
		}	
	}
	else
	if (sig == SIGUSR2)
	{
		if ((fd = open("/tmp/.fv_in", O_RDONLY)) != -1)
		{
			if (Current_client)
				fv_putmsg(TRUE, Fv_message[MELOAD1ST], 0, 0);
			else
			{
				n = read(fd, buf, 1023);
				buf[n] = 0;
				Current_client = atoi(buf);
				b_p = buf;
				while (*b_p >= '0' && *b_p <= '9')
					b_p++;
				if (*b_p && strcmp(Fv_path, b_p))
				{
					Back_path = malloc((unsigned)strlen(Fv_path)+1);
					(void)strcpy(Back_path, Fv_path);
					fv_openfile(b_p, (char *)0, TRUE);
				}
				else
					Back_path = NULL;

				/* Bring frame forward, show load button,
				 * ring bell to attract user's attention.
				 */
				if (Was_closed = (BOOLEAN)window_get(Fv_frame, 
					FRAME_CLOSED))
					window_set(Fv_frame, FRAME_CLOSED, FALSE, 0);
				else
					window_set(Fv_frame, WIN_SHOW, TRUE, 0);
				fv_show_load_button(TRUE);
				window_bell(Fv_frame);
			}
			(void)close(fd);
		}
	}

	return(NOTIFY_DONE);
}


fv_load_client(win_id)
	int win_id;			/* Window id */
{
	char buf[20];			/* Device name */
	int pid;			/* Process id */
	int fd;				/* File descriptor */
	register int i;				
	char *fv_shelve_primary_selection();

	(void)fv_shelve_primary_selection(FALSE);

#ifdef SV1
	(void)sprintf(buf, "/dev/win%d", win_id);
	if ((fd = open(buf, O_RDONLY)) != -1)
	{
		pid = win_getowner(fd);
		(void)close(fd);
		for (i = 0; i<Nclient; i++)
			if (pid == Client[i])
				return(kill(pid, SIGUSR1));
	}
	return(-1);
#else
	return(0);
#endif
}


void
fv_load_button()		/* Write selection back to calling application */
{
	if (fv_load_client(Current_client) == -1)
		fv_putmsg(TRUE, Fv_message[MEFINDCLIENT], Current_client, 0);
	else
	{
		if (Back_path)
		{
			/* Restore folder context */

			fv_openfile(Back_path, (char *)0, TRUE);
			free(Back_path);
		}
		fv_show_load_button(FALSE);
	}
	Current_client = 0;

	if (Was_closed)
		window_set(Fv_frame, FRAME_CLOSED, TRUE, 0);
	else
	{
#ifdef SV1
		/* Build and send a fake shift-L5 event to frame */
		event_set_id(&Bottom_event, MS_LEFT);
		event_set_down(&Bottom_event);
		event_set_shiftmask(&Bottom_event, SHIFTMASK);
		win_post_event(Fv_frame, &Bottom_event, NOTIFY_SAFE);
#else
		wmgr_bottom(Fv_frame);
#endif
	}
}
