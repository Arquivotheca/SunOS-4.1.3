/*	@(#)tty.h 1.1 92/07/30 Copyright Sun Micro.	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#define	MIN_WIDTH	80
#define MIN_HEIGHT	24

#define	CONTROL_TTY	0
#define	STATUS_TTY	1
#define	OPTION_TTY	2
#define	HELP_TTY	3
#define OTHER_TTY	4

#define	SYS_STATUS_COL  47	
#define	SYS_PASS_COL	5
#define	TOTAL_ERROR_COL	27
#define	ELAPSE_COL	49	
#define	STATUS_1	18
#define STATUS_2	43
#define	TEST_COL	18

#define	TTY_WINNO	40

extern	WINDOW	*tty_control[];	/* control windows, up to 40 pages */
extern	WINDOW	*tty_status[];	/* status windows, up to 40 pages */
extern	WINDOW	*tty_option;	/* option window(share just one ) */
extern	WINDOW	*tty_other;	/* other window(share just one) */
extern	WINDOW	*control_button;/* the window to display command button */
extern	WINDOW	*status_button;	/* the window to display test status */
extern	WINDOW	*console_tty;	/* console/error message window(to draw box) */
extern	WINDOW	*message_tty;	/* console/error message subwindow */
extern	int	window_type;	/* type of window currently displayed */
extern	int	control_index;	/* index of current control window displayed */
extern	int	status_index;	/* index of current status window displayed */
extern	int	option_id;	/* test i.d. of the current option popup */
extern	int	control_max;	/* the max. index of control windows(#-1) */
extern	int	status_max;	/* the max. index of status windows(#-1) */
extern	int	status_page;	/* current status page */
extern	int	status_row;	/* current status row */
extern	char	*com_err;	/* command error message(in ttyprocs.c) */
extern	char	*format_err;	/* command format error message(in ttyprocs.c)*/
extern	char	*arg[];		/* up to 20 command line arguments */
extern	int	arg_no;		/* number of arguments available */
extern	char	com_line[]; /* the image of the command line(NULL-terminated) */

struct	com_tbl		/* tty command table, ended with a NULL shartname */
{
  char	*fullname;		/* full command name(a word) */
  char	shortname;		/* short hand command name(a character) */
};
