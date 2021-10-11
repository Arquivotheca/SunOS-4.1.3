#ifndef lint
static  char sccsid[] = "@(#)adbsub.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Subroutines to be called by adbgen2.c, the C program generated
 * by adbgen1.c.
 */

#include <stdio.h>

int last_off;
int warnings = 1;

/*
 * User claims offset is ok.
 * This usually follows call to another script, which we cannot handle.
 */
offsetok()
{
	last_off = -1;
}

/*
 * Get adb.s dot to the right offset.
 */
offset(off)
{
	register int off_diff;

	if (last_off == -1) {
		last_off = off;
		return;
	}
	off_diff = off - last_off;
	if (off_diff) {
		if (off_diff > 0) {
			if (off_diff > 1) {
				printf("%d", off_diff);
			}
			printf("+");
		}
		if (off_diff < 0) {
			if (off_diff < -1) {
				printf("%d", -off_diff);
			}
			printf("-");
		}
	}
	last_off = off;
}

/*
 * Format struct member, checking size.
 */
format(name, size, fmt)
	char *name;
	int size;
	char *fmt;
{
	register int fs;

	fs = do_fmt(fmt);
	if (fs != size && warnings) {
		fprintf(stderr,
			"warning: \"%s\" size is %d, \"%s\" width is %d\n",
			name, size, fmt, fs);
	}
	last_off += fs;
}

/*
 * Emit the format command, return the size.
 */
do_fmt(acp)
	char *acp;
{
	register int rcount, width, sum, i;
	register char *cp;

	cp = acp;
	sum = rcount = 0;
	do {
		while (*cp >= '0' && *cp <= '9') {
			rcount = rcount * 10 + *cp++ - '0';
		}
		if (rcount == 0) {
			rcount = 1;
		}
		switch (*cp) {
		case 'F':
			width = 8;
			break;
		case 'X':
		case 'O':
		case 'Q':
		case 'D':
		case 'U':
		case 'f':
		case 'Y':
		case 'p':
		case 'P':
			width = 4;
			break;
		case 'x':
		case 'o':
		case 'q':
		case 'd':
		case 'u':
			width = 2;
			break;
		case 'b':
		case 'c':
		case 'C':
		case '+':
			width = 1;
			break;
		case 'I':
		case 'a':
		case 'A':
		case 't':
		case 'r':
		case 'n':
			width = 0;
			break;
		case '-':
			width = -1;
			break;
		case 's':
		case 'S':
		case 'i':
			if (warnings) {
				fprintf(stderr,
				"Unknown format size \"%s\", assuming zero\n",
				acp);
			}
			width = 0;
			break;
		default:
			fprintf(stderr, "Unknown format size: %s\n", acp);
			exit(1);
		}
		for (i = 0; i < rcount; i++) {
			putchar(*cp);
		}
		cp++;
		sum += width * rcount;
	} while (*cp);
	return(sum);
}

/*
 * Get the value at offset based on base.
 */
indirect(offset, size, base, member)
	int offset, size;
	char *base, *member;
{
	if (size == 4) {
		if (offset == 0) {
			printf("*%s", base);
		} else {
			printf("*(%s+0t%d)", base, offset);
		}
	} else if (size == 2) {
		if (offset == 2) {
			printf("(*%s&0xffff)", base);
		} else {
			printf("(*(%s+0t%d)&0xffff)", base, offset - 2);
		}
	} else if (size == 1) {
		if (offset == 3) {
			printf("(*%s&0xff)", base);
		} else {
			if ((offset & 0x1) == 0x1) {
				printf("(*(%s+0t%d)&0xff)", base, offset - 3);
			} else {
				printf("((*(%s+0t%d)&0xff00)/0x100)",
				    base, offset - 2);
			}
		}
	} else {
		fprintf(stderr, "Indirect size %d not 1, 2, or 4: %s\n",
		    size, member);
		exit(1);
	}
}
