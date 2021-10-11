/*	@(#)talk.h 1.1 92/07/30 SMI; from UCB 1.2 83/04/23	*/

#include <curses.h>
#include <utmp.h>

#define	forever		for(;;)

#define	BUF_SIZE	512

FILE *popen();
int quit(), sleeper();

extern int sockt;
extern int curses_initialized;
extern int invitation_waiting;

extern char *current_state;
extern int current_line;

typedef struct xwin {
	WINDOW *x_win;
	int x_nlines;
	int x_ncols;
	int x_line;
	int x_col;
	char kill;
	char cerase;
	char werase;
} xwin_t;

extern xwin_t my_win;
extern xwin_t his_win;
extern WINDOW *line_win;
