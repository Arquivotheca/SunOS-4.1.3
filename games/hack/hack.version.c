#ifndef lint
static	char sccsid[] = "@(#)hack.version.c 1.1 92/07/30 SMI";
#endif
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* hack.version.c - version 1.0.2 */

#include	"date.h"

doversion(){
	pline("%s 1.0.2 - last edit %s.", (
#ifdef QUEST
		"Quest"
#else
		"Hack"
#endif QUEST
		), datestring);
	return(0);
}
