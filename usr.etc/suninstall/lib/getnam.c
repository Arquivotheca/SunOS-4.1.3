/*      @(#)getnam.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <curses.h>
#include "install.h"

getnam(str,x,y)
char str[];
int x, y;
{
	int xlate();
	extern int FORWARD, BACKWARD;
	register c, id, done;
	register char *cp = &str[strlen(str)];

	noecho();
	move(y,x);
	for(id=x, done=0; !done ;) {
		refresh();
		c = getch();
		switch(xlate(c)) {
		case '\n':
		case '\r':
			*cp = 0;
			done = 1;
			break;
		case CERASE:
			if(cp >= &str[0]) {
				mvaddch(y,--id,' ');
				if (cp == &str[0]) {
					move(y,++id);
				} else {
					move(y,id);
					*(cp--) = '\0';
				}
			}
			break;
		case CONTROL_U:
			for (;cp >= &str[0];cp--,--id) {
                                if (cp == &str[0]) {
                                        move(y,id);
					clrtoeol();
					refresh();
					break;
                                }
                        }
			break;
		case CONTROL_B:
		case CONTROL_P:
			*cp = 0;
			BACKWARD = 1;
			done = 1;
			break;
        	case CONTROL_F:
		case CONTROL_N:
			*cp = 0;
			FORWARD = 1;
			done = 1;
                        break;
		default:
			if(c > ' ' && c < '\177') {
				*cp++ = c;
				*cp = '\0';
				mvaddch(y,id++,c);
			}
			break;
		}
	}
	echo();
#ifdef lint
	FORWARD = BACKWARD;
	BACKWARD = FORWARD;
#endif lint
}
