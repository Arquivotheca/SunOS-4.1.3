/*	@(#)subr_xxx.c 1.1 92/07/30 SMI; from UCB 4.22 83/05/27	*/

/*LINTLIBRARY*/

#include <machine/pte.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/uio.h>

/*
 * Routine placed in illegal entries in the bdevsw and cdevsw tables.
 */
nodev()
{

	return (ENODEV);
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
nulldev()
{

	return (0);
}

/*
 * Null system calls. Not invalid, just not configured.
 */
errsys()
{
	u.u_error = EINVAL;
}

nullsys()
{
}

imin(a, b)
{

	return (a < b ? a : b);
}

imax(a, b)
{

	return (a > b ? a : b);
}

unsigned
min(a, b)
	u_int a, b;
{

	return (a < b ? a : b);
}

unsigned
max(a, b)
	u_int a, b;
{

	return (a > b ? a : b);
}

#ifndef vax
/*
 * Return bit position of least significant bit set in mask,
 * starting numbering from 1.
 */
ffs(mask)
	register long mask;
{
	register int i;

	if (mask == 0)
		return (0);
	for (i = 1; i <= NBBY * sizeof mask; i++) {
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
	return (0);
}

bcmp(s1, s2, len)
	register char *s1, *s2;
	register int len;
{

	while (len--)
		if (*s1++ != *s2++)
			return (1);
	return (0);
}
#endif

/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */
strlen(s)
	register char *s;
{
	register int n;

	n = 0;
	while (*s++)
		n++;
	return (n);
}

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */
strcmp(s1, s2)
	register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return (0);
	return (*s1 - *--s2);
}

strncmp(s1, s2, n)
	register char *s1, *s2;
	register int n;
{

	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return (0);
	return (n<0 ? 0 : *s1 - *--s2);
}

/*
 * Copy string s2 to s1.  s1 must be large enough.
 * return ptr to null in s1
 */
char *
strcpy(s1, s2)
	register char *s1, *s2;
{

	while (*s1++ = *s2++)
		;
	return (s1 - 1);
}

/*
 * Copy s2 to s1, truncating to copy n bytes
 * return ptr to null in s1 or s1 + n
 */
char *
strncpy(s1, s2, n)
	register char *s1, *s2;
{
	register i;

	for (i = 0; i < n; i++) {
		if ((*s1++ = *s2++) == '\0') {
			return (s1 - 1);
		}
	}
	return (s1);
}

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
 */

#ifndef	NULL
#define	NULL 0
#endif	NULL

char *
strrchr(sp, c)
register char *sp, c;
{
	register char *r;

	r = NULL;
	do {
		if (*sp == c)
			r = sp;
	} while (*sp++);
	return (r);
}

/*
 * Convert hex string to u_int. Assumes no leading "0x".
 */
u_int
atou(s)
	char *s;
{
	u_int val = 0;
	u_int digit;

	while (*s) {
		if (*s >= '0' && *s <= '9')
			digit = *s++ - '0';
		else if (*s >= 'a' && *s <= 'f')
			digit = *s++ - 'a' + 10;
		else if (*s >= 'A' && *s <= 'F')
			digit = *s++ - 'A' + 10;
		else
			break;
		val = (val * 16) + digit;
	}
	return (val);
}
