#ident "@(#)sprintf.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

#ifndef KADB

/*
 * Simple sprintf.  No fixed-width field formatting.
 * Implements  %x %d %i %u %o %o %c %s
 */
#include <varargs.h>
#include <sys/types.h>

#ifdef putchar
# undef putchar
#endif
#define putchar(c) *sprintf_ptr++ = (c)

char *sprintf_ptr;
char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

putn(n, base, signed)
#ifdef GALAXY
/*
 * do not let putn assume high positive numbers as negative
 */
    u_long n;
#else
    int n;
#endif GALAXY
    int base;
    char signed;
{
    char prbuf[12];
    register char *cp;

#ifndef GALAXY
    if (signed == 's' && n < 0) {
	n = -n;
	putchar('-');
    }
#endif GALAXY

    cp = prbuf;
    do {
	    *cp++ = digits[n%base];
	    n /= base;
    } while(n);
    do {
	    putchar(*--cp);
    } while(cp > prbuf);
}

sprintf(va_alist /* char *str, *fmt, arg0, arg1, ... */)
    va_dcl
{
    va_list ap;
    char *fmt;

    char c, *p;
    int i = 0;

    va_start (ap);
    sprintf_ptr = va_arg(ap, char *);
    fmt = va_arg(ap, char *);
 
    for(;;) {
	switch (c = *fmt++) {
	    case '\0': putchar(c); return;
	    case '%':
		switch (c = *fmt++) {
		    case '\0': putchar(c); return;
		    case 'x': putn(va_arg(ap, int), 16, 'u'); break;
		    case 'd': putn(va_arg(ap, int), 10, 's'); break;
		    case 'i': putn(va_arg(ap, int), 10, 's'); break;
		    case 'u': putn(va_arg(ap, int), 10, 'u'); break;
		    case 'o': putn(va_arg(ap, int),  8, 'u'); break;
		    case 'c': putchar(va_arg(ap, int));  break;
		    case 's':
			for (p = va_arg(ap, char *); c = *p++; )
			   putchar (c);
			break;
		    default: putchar(c); break;
		}
		break;
	    default: putchar(c); break;
	}
    }
}

#endif KADB
