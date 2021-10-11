/* @(#) low.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
#ifndef __LOW_H__
#define	__LOW_H__

#define	ENTRY(name)			\
	.globl ___/**/name;		\
	___/**/name:

#endif __LOW_H__
