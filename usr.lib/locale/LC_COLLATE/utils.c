/*      @(#)utils.c 1.1 92/07/30 SMI      */
/*
 *  hold various utilies
 */

#include "colldef.h"
#include "y.tab.h"
#include "extern.h"

panic(s, d, form)
	char *s;
	int d;
	int form;
{
	if (form)
		fprintf(stderr, "panic:(0x%x) %s\n",d, s);
	else
		fprintf(stderr, "panic:(%d) %s\n",d, s);
	exit(1);
}

ipanic(s)
	char *s;
{
	fprintf(stderr, "%s\n", s);
}

ierror(s)
	char *s;
{
	fprintf(stderr, "%s\n", s);
}

get_type(s)
	register char *s;
{
	while (*s) {
		if (ME_HEX(*s) || *s == 'x')
			return(H_NUM);
		s++;
	}
	return(O_NUM);
}
	

/*
 * I assume that the first character here is digit.
 */
getnum(fp, val)
	FILE *fp;
	unsigned int *val;
{
	char nbuf[IDSIZE];
	register char *p = nbuf;
	int hflag = 0;
	int c;

	c = getc(fp);
	while (isdigit(c) || ME_HEX(c) || c == 'x') {
		*p++ = c;
		if (ME_HEX(c) || c == 'x')
			++hflag;
		c = getc(fp);
	}
	*p = '\0';
	ungetc(c, fp);
	if (!hflag)
		*val = atoi(nbuf);
	else
		*val = axtoi(nbuf);
	return(hflag ? H_NUM : O_NUM);
}

extern long strtol();
/*
 * ascii hex. to integer
 */
axtoi(p)
	register char *p;
{
	return strtol(p, (char **)NULL,16);
}

/*
 * ascii octal. to integer
 */
aotoi(p)
	register char *p;
{
	return strtol(p, (char **)NULL,8);
}


/*
 * string save
 */
char *
strsave(s)
	char *s;
{
	char *sp;

	if (s == NULL)
		return(NULL);
	sp = (char *)malloc(strlen(s) + 1);
	if (sp == NULL)
		ipanic("strsave: can't allocate memory");
	strcpy(sp, s);
	return(sp);
}

/*
 * free's
 */
free_type_var(t)
	struct type_var *t;
{
	if (t->type == STRING || t->type == SYM_NAME)
		free (t->type_val.str_val);
	free(t);
}

