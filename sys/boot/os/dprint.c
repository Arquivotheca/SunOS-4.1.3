#ifndef lint
static char sccsid[] = "@(#)dprint.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1983-1990 Sun Microsystems Inc.
 */

#include <stand/saio.h>
#include <sys/reboot.h>
#include <mon/sunromvec.h>

#ifdef DUMP_DEBUG
/*
 * Utilities used by both client and server
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

/*VARARGS2*/
dprint(var, level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)
        int var;
        int level;
        char *str;
        int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
        if (var == level || (var > 10 && (var - 10) >= level))
                printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
}
#else
dprint() { }
#endif

dump_buffer (string, from, length)
char    *string;
char    *from;
int     length;
{
        int     i;
        char    *p;

	printf ("dump buffer: ");
        printf ("%s", string);
        for (p = from, i = 0; i < length; i++, p++)     {
                if ((i & 0x3) == 0) printf (" ");
                if ((*p & 0xf0) == 0) printf ("0");
                printf ("%x", (*p & 0xff));
        }
        printf ("\n");

}

