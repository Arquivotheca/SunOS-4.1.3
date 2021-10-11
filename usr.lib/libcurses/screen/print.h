/*	@(#)print.h 1.1 92/07/30 SMI; from S5R3 1.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* externs from iexpand.c, cexpand.c */
extern void tpr();
extern int cpr();
extern char *cexpand(), *iexpand(), *cconvert(), *rmpadding();

/* externs from print.c */
enum printtypes
    {
    pr_none,
    pr_terminfo,		/* print terminfo listing */
    pr_cap,			/* print termcap listing */
    pr_longnames		/* print C variable name listing */
    };

extern void pr_onecolumn();
extern void pr_caprestrict();
extern void pr_width();
extern void pr_init();
extern void pr_heading();
extern void pr_bheading();
extern void pr_boolean();
extern void pr_bfooting();
extern void pr_nheading();
extern void pr_number();
extern void pr_nfooting();
extern void pr_sheading();
extern void pr_string();
extern void pr_sfooting();
