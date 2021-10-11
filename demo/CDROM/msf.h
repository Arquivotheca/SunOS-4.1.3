/*************************************************************
 *                                                           *
 *                     file: msf.h                           *
 *                                                           *
 *************************************************************/

/*
 * Copyright (C) 1988, 1989 Sun Microsystems, Inc.
 */

/* @(#)msf.h 1.1 92/07/30 Copyr 1989, 1990 Sun Microsystem. */

/* 
 * This file contains the typedefs of msf data structure 
 */

typedef struct msf {
	int	min;
	int	sec;
	int	frame;
} *Msf;

extern	Msf	init_msf();
extern	Msf	diff_msf();
extern struct	msf	add_msf();
extern struct	msf	sub_msf();
extern void	acc_msf();
extern int	cmp_msf();

