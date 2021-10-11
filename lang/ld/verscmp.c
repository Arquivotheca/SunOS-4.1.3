/* @(#)verscmp.c 1.1 92/07/30 */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#define SKIP_DOT(str)  ((*str == '.')  ? ++str : str)
#define EMPTY(str) ((str == NULL) || (*str == '\0'))
#define NULL 0
#define isdigit(c) (((c) >= '0') && ((c) <= '9') ? 1:0)

int stol();

/* 
 * Test whether the string is of digit[.digit]* format
 */
rest_ok(str)
	char *str;			/* input string */
{
	int dummy;			/* integer place holder */
	int legal = 1;			/* return flag */

	while (!EMPTY(str)) {
		if (!stol(str, '.', &str, &dummy)) {
			legal = 0; 
			break;
		};
		if (EMPTY(str))
			break;
		else SKIP_DOT(str);
	};
	return(legal);
};

/*
 * Compare 2 strings and test whether they are of the form digit[.digit]*. 
 * It will return -1, 0, or 1 depending on whether c1p is less, equal or
 * greater than c2p
 */
verscmp(c1p, c2p)
	char *c1p;			/* input string */
	char *c2p;			/* input string */	
{
	char *l_c1p = c1p;		/* working copy of c1p */
	char *l_c2p = c2p;		/* working copy of c2p */
	int l_c1p_ok = 0;		/* is c1p a legal string */
	int c2p_dig = 0;		/* int that c1p currently represents */
	int c1p_dig = 0;		/* int that c2p currently represents */	
	int result = 0;

	while ((l_c1p_ok = stol(l_c1p,'.', &l_c1p, &c1p_dig)) &&
		stol(l_c2p,'.', &l_c2p, &c2p_dig) && (c2p_dig == c1p_dig)) { 
		if (EMPTY(l_c1p) && EMPTY(l_c2p))
			return(0);
		else if (EMPTY(l_c1p) && !EMPTY(l_c2p) && rest_ok(SKIP_DOT(l_c2p)))  
			return(-1);
		else if (EMPTY(l_c2p) && !EMPTY(l_c1p) && rest_ok(SKIP_DOT(l_c1p)))  
			return(1);
		l_c1p++; l_c2p++;
	};
	if (!l_c1p_ok)
		return(-1);
	else if (c1p_dig < c2p_dig) 
		return(-1);
	else if ((c1p_dig > c2p_dig) && rest_ok(SKIP_DOT(l_c1p)))
		return(1);
	else return(-1);
}

/*
 * "stol" attempts to interpret a collection of characters between delimiters
 * as a decimal digit. It stops interpreting when it reaches a delimiter or
 * when character does not represent a digit. In the first case it returns
 * success and the latter failure. 
 */
int
stol(cp, delimit, ptr, i)
	char *cp;			/* ptr to input string */
	char delimit;			/* delimiter */
	char **ptr;			/* left pointing to next del. or illegal 
					   character */
	int *i;				/* digit that the string represents */
{
	int c = 0;			/* current char */
	int n = 0;			/* working copy of i */
	int neg = 0;			/* is number negative */

	if (ptr != (char **)0)
		*ptr = cp; /* in case no number is formed */

	if (EMPTY(cp))
		return(0);

	if (!isdigit(c = *cp) && (c == '-')) {
		neg++;
		c = *++cp;
	};
	if (EMPTY(cp) || !isdigit(c))
		return(0);

	while (isdigit(c = *cp) && (*cp++ != '\0')) {
		n *= 10;
		n += c - '0';
	};
	if (ptr != (char **)0)
		*ptr = cp;

	if ((*cp == '\0') || (*cp == delimit)) {
		*i = neg ? -n : n;
		return (1);
	};
	return (0);
}
