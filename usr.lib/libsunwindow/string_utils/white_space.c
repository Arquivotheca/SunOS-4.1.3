#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)white_space.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/string_utils.h>
/*
 * these two procedures are in a separate file because they are also defined
 * in libio_stream.a, and having them be in string_utils.c tends to make the
 * loader complain about things being multiply defined 
 */

enum CharClass
white_space(c)
	char            c;
{
	switch (c) {
	    case ' ':
	    case '\n':
	    case '\t':
		return (Sepr);
	    default:
		return (Other);
	}
}

/* ARGSUSED */
struct CharAction
everything(c)
	char            c;
{
	static struct CharAction val = {False, True};

	return (val);
}
