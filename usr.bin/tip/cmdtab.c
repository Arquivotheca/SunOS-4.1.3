/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)cmdtab.c 1.1 92/07/30 SMI"; /* from UCB 5.3 5/5/86 */
#endif

#include "tip.h"

extern	int shell(), getfl(), sendfile(), chdirectory();
extern	int finish(), help(), pipefile(), pipeout(), consh(), variable();
extern	int cu_take(), cu_put(), dollar(), genbrk(), suspend();

esctable_t etable[] = {
	{ '!',	NORM,	"shell",			 shell },
	{ '<',	NORM,	"receive file from remote host", getfl },
	{ '>',	NORM,	"send file to remote host",	 sendfile },
	{ 't',	NORM,	"take file from remote UNIX",	 cu_take },
	{ 'p',	NORM,	"put file to remote UNIX",	 cu_put },
	{ '|',	NORM,	"pipe remote file",		 pipefile },
	{ 'C',  NORM,	"connect program to remote host",consh },
	{ 'c',	NORM,	"change directory",		 chdirectory },
	{ '.',	NORM,	"exit from tip",		 finish },
	{CTRL(d),NORM,	"exit from tip",		 finish },
	{ '$',	NORM,	"pipe local command to remote host", pipeout },
	{CTRL(y),NORM,	"suspend tip (local only)",	 suspend },
	{CTRL(z),NORM,	"suspend tip (local+remote)",	 suspend },
	{ 's',	NORM,	"set variable",			 variable },
	{ '?',	NORM,	"get this summary",		 help },
	{ '#',	NORM,	"send break",			 genbrk },
	{ 0, 0, 0 }
};
