/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef lint
static  char *sccsid = "@(#)sh.8bit.c 1.1 92/07/30 SMI; from SUN  5/5/88";
#endif

#include "sh.h"
#include <stdio.h>
#include <sys/file.h>
#include <errno.h>

extern int errno;

#define IN_INIT		0x0000
#define IN_SQUOTE	0x0001
#define IN_DQUOTE	0x0002
#define QUOTE_NEXT	0x0004

#define SLASH		'\\'


/*
 *  set bit buffer
 */
set_bitbuf(s, p)
	register char *s;
	register char *p;
{
	int status = 0;
	int quote_next = 0;
	int cnt;

	/*
	 * First, let me set temporary buffer.
	 */
	while (*s) {
		switch (*s) {
		case '\\':
			switch (status) {
			case IN_SQUOTE:
				*p++ = 1;
				break;
			case IN_DQUOTE:
				*p++ = 1;
				break;
			case QUOTE_NEXT:
				*p++ = 1;
				status = 0;
				break;
			default:
				*p++ = 0;
				status = QUOTE_NEXT;
				break;
			}
			break;
		case '\'':
			switch (status) {
			case IN_SQUOTE:
				/*
				 * end of single quote
				 */
				*p++ = 0;
				status = 0;
				break;
			case IN_DQUOTE:
				*p++ = 0;
				break;
			case QUOTE_NEXT:
				*p++ = 1;
				status = 0;
				break;
			default:
				/*
				 * start single quote
				 */
				 status = IN_SQUOTE;
				 *p++ = 0;
			}
			break;
		case '"':
			switch (status) {
			case IN_SQUOTE:
				*p++ = 1;
				break;
			case IN_DQUOTE:
				/*
				 * end of double quote
				 */
				*p++ = 0;
				status = 0;
				break;
			case QUOTE_NEXT:
				*p++ = 1;
				status = 0;
				break;
			default:
				*p++ = 0;
				status = IN_DQUOTE;
				break;
			}
			break;
		default:
			/*
			 * non-special character
			 */
			switch (status) {
			case IN_SQUOTE:
				*p++ = 1;
				break;
			case IN_DQUOTE:
				if (ISglob(*s) && (*s != '`')) 
					*p++ = 1;
				else 
					*p++ = 0;
				break;
			case QUOTE_NEXT:
				*p++ = 1;
				status = 0;
				break;
			default:
				*p++ = 0;
				break;
			}
			break;
		}
		++s;
	}
}

/*
 * set bit array
 */
set_bitarray(from, to)
	char *from;
	register char *to;
{
	int len;
	register char *p;
	char buf[BUFSIZ];

	set_bitbuf(from, buf);
	p = buf;
	for (len = 0; len < strlen(from); len++)
		*to++ = *p++;

}


/*
 * different any
 */
any8(c, s)
	register int c;
	register char *s;
{
	char buf[BUFSIZ];
	register char *pp;

	if (!s)
		return(0);
	set_bitbuf(s, buf);
	pp = buf;
	while (*s)
		if (*s++ == c && *pp++ == 0) {
			return(1);
		}
	return(0);
}

/*
 * trim off back slash
 */
trim_slash_dbg(t)
	register char **t;
{
	register char *p;

#ifdef DBG
	dprintf("TRIM_SLASH\n");
	while (p = *t++) {
		dprintf(p);
		dprintf("\n");
	}
	dprintf("OUT TRIM_SLASH\n");
#endif
}

/*
 * trim off back slash
 */
trim_slash(t)
	register char **t;
{
	register char *s;
	char buffer[BUFSIZ];
	int status = 0;
	char *p;
	char *s1;

	while (s = *t) {
		p = buffer;
		s1 = s;
		while (*s) {
			switch (*s) {
			case '\\':
				switch (status) {
				case IN_SQUOTE:
					*p++ = *s;
					break;
				case IN_DQUOTE:
					*p++ = *s;
					break;
				case QUOTE_NEXT:
					*p++ = *s;
					status = 0;
					break;
				default:
					status = QUOTE_NEXT;
					break;
				}
				break;
			case '\'':
				switch (status) {
				case IN_SQUOTE:
					/*
					 * end of single quote
					 */
					status = 0;
					break;
				case IN_DQUOTE:
					*p++ = *s;
					break;
				case QUOTE_NEXT:
					*p++ = *s;
					status = 0;
					break;
				default:
					/*
					 * start single quote
					 */
					 status = IN_SQUOTE;
					 break;
				}
				break;
			case '"':
				switch (status) {
				case IN_SQUOTE:
					*p++ = *s;
					break;
				case IN_DQUOTE:
					/*
					 * end of double quote
					 */
					status = 0;
					break;
				case QUOTE_NEXT:
					*p++ = *s;
					status = 0;
					break;
				default:
					status = IN_DQUOTE;
					break;
				}
				break;
			default:
				/*
				 * non-special character
				 */
				switch (status) {
				case IN_SQUOTE:
					*p++ = *s;
					break;
				case IN_DQUOTE:
					*p++ = *s;
					break;
				case QUOTE_NEXT:
					*p++ = *s;
					status = 0;
					break;
				default:
					*p++ = *s;
					break;
				}
				break;
			}
			++s;
		}
	/*
	 * fix real string here
	 */
		*p = 0;
#ifdef DBG
		dprintf("IN trim_slash\n");
		dprintf("BEFORE - "); dprintf(s1); dprintf("\n");
		dprintf("AFTER  - "); dprintf(buffer); dprintf("\n");
#endif
		strcpy(*t, buffer);
		t++;
	}
}


/*
 * set_map and init_map_status
 */
static int map_status = 0;	/* This is used only in set_map & init_map_status */
static int pre_map_status = 0;	/* save the previous one */

/*
 * Give me more task !
 */
init_map_status()
{
	map_status = IN_INIT;
	pre_map_status = IN_INIT;
}

/*
 * restore previous status
 */
fix_map_status()
{
	map_status = pre_map_status;
}

/*
 * Lot's of brother/sister's
 */
set_map(map, c)
	char *map;
	char c;
{
	pre_map_status = map_status;
	switch (c) {
	case '\\':
		switch (map_status) {
		case IN_SQUOTE:
			*map = 1;
			break;
		case IN_DQUOTE:
			*map = 1;
			break;
		case QUOTE_NEXT:
			*map = 1;
			map_status = IN_INIT;
			break;
		case IN_INIT:
		default:
			*map = 1;
			map_status = QUOTE_NEXT;
			break;
		}
		break;
	case '\'':
		switch (map_status) {
		case IN_SQUOTE:
			/*
			 * end of single quote
			 */
			*map = 1;
			map_status = IN_INIT;
			break;
		case IN_DQUOTE:
			*map = 0;
			break;
		case QUOTE_NEXT:
			*map = 1;
			map_status = IN_INIT;
			break;
		case IN_INIT:
		default:
			/*
			 * start single quote
			 */
			 map_status = IN_SQUOTE;
			 *map = 1;
		}
		break;
	case '"':
		switch (map_status) {
		case IN_SQUOTE:
			*map = 1;
			break;
		case IN_DQUOTE:
			/*
			 * end of double quote
			 */
			*map = 0;
			map_status = IN_INIT;
			break;
		case QUOTE_NEXT:
			*map = 1;
			map_status = IN_INIT;
			break;
		case IN_INIT:
		default:
			*map = 0;
			map_status = IN_DQUOTE;
			break;
		}
		break;
	default:
		/*
		 * non-special character
		 */
		switch (map_status) {
		case IN_SQUOTE:
			*map = 1;
			break;
		case IN_DQUOTE:
			if (ISglob(c) && (c != '`'))
				*map = 1;
			else
				*map = 0;
			break;
		case QUOTE_NEXT:
			*map = 1;
			map_status = IN_INIT;
			break;
		case IN_INIT:
		default:
			*map = 0;
			break;
		}
		break;
	}
}

/*
 * check_me
 *	return 1 -- if the next character will be quoted
 *	       0 --                       otherwise
 */

check_me()
{
	int ret = 0;
	switch (map_status) {
	case IN_SQUOTE:
		ret = 1;
		break;
	case IN_DQUOTE:
		ret = 0;
		break;
	case QUOTE_NEXT:
		ret = 1;
		break;
	case IN_INIT:
	default:
		ret = 0;
		break;
	}

	return(ret);
}
