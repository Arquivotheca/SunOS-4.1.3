#ifndef lint
static	char sccsid[] = "@(#)poll.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sunwindow/notify.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <fcntl.h>
#include <sys/filio.h>
#include "sony_laser.h"
#include "lasertool.h"

#define NULLC '\0'

int poll_state;
int poll_fd;
char poll_ch;
int ch_go, disk_go;
extern Panel_item adrezz, chpter, dsk_type, error_return;
extern FILE *device;

Notify_value
poll(client, which)
Notify_client client;
int which;
{
	extern void print_disk_info();
	unsigned char buf;
	char string[30];
	int count;

	count = 1;
	switch (poll_state) {
		case POLL_PRINT: 
			print_disk_info();
			return(NOTIFY_DONE);
		case POLL_SEARCH:	/* Wait for poll_ch */
			if (ioctl(poll_fd, FIONREAD, &count) == -1) {
				perror("FIONREAD failed.");
				return(NOTIFY_DONE);
			} else {
				while (count--) {
					if (read(poll_fd, &buf, 1) != 1) {
						perror("(poll)read failed.");
					}
					if (buf == poll_ch) {
						sony_audio_mute_on(device);
						poll_state = POLL_PRINT;
						return(NOTIFY_DONE);
					} else if(buf == MARK_RETURN) {
						sprintf(string,"Mark Return ignored\n");
						panel_set(error_return, PANEL_LABEL_STRING, string, 0);
					}
				}
			}
			break;
		default:
			return(NOTIFY_DONE);
	}
}

void
print_disk_info()
{
	static char string[20];
	unsigned char zip[6], chapter[3];

	sony_addr_inq(device, zip);
	if(zip[0] == NAK)
		sprintf(string,"NAK");
	else {
		zip[5] = NULLC;
		sprintf(string,"%d", atoi(zip));
	}
	panel_set(adrezz, PANEL_LABEL_STRING, string, 0);
	
	if( (ch_go == 0) || (ch_go == 30) ) {
		sony_chapter_inq(device, chapter);
		if(chapter[0] == NAK)
			sprintf(string,"No Chapters");
		else {
			chapter[2] = NULLC;
			sprintf(string,"%d", atoi(chapter));
		}
		panel_set(chpter, PANEL_LABEL_STRING, string, 0);
		ch_go = 1;
	} else
		ch_go++;

	if( (disk_go == 0) || (disk_go == 60) ) {
		if(sony_clv_disk(device))
			sprintf(string,"CLV");
		else
			sprintf(string,"CAV");
		panel_set(dsk_type, PANEL_LABEL_STRING, string,
			0);
		disk_go = 1;
	} else
		disk_go++;
}
