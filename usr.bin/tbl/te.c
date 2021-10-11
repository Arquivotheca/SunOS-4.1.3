#ifndef lint
static	char sccsid[] = "@(#)te.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/02/12 */
#endif

 /* te.c: error message control, input line count */
# include "t..c"
error(s)
	char *s;
{
fprintf(stderr, "\n%s: line %d: %s\n", ifile, iline, s);
# ifdef unix
fprintf(stderr, "tbl quits\n");
exit(1);
# endif
# ifdef gcos
fprintf(stderr, "run terminated due to error condition detected by tbl preprocessor\n");
exit(0);
# endif
}
char *
errmsg(errnum)
	int errnum;
{
extern int sys_nerr;
extern char *sys_errlist[];
static char errmsgbuf[18];
if (errnum > sys_nerr)
	{
	sprintf(errmsgbuf, "Error %d", errnum);
	return (errmsgbuf);
	}
else
	return (sys_errlist[errnum]);
}
char *
gets1(s, len)
	char *s;
	int len;
{
char *p;
int nbl;
extern int errno;
while(len > 0)
	{
	iline++;
	while ((p = fgets(s,len,tabin))==0)
		{
		if (swapin()==0)
			return(0);
		}

	while (*s) s++;
	s--;
	if (*s == '\n') *s-- =0;
	else
		{
		if (!feof(tabin))
			{
			if (ferror(tabin))
				error(errmsg(errno));
			else
				error("Line too long");
			}
		}
	for(nbl=0; *s == '\\' && s>p; s--)
		nbl++;
	if (linstart && nbl % 2) /* fold escaped nl if in table */
		{
		s++;
		len -= s - p;
		continue;
		}
	break;
	}

return(p);
}
# define BACKMAX 500
char backup[BACKMAX];
char *backp = backup;
un1getc(c)
{
if (c=='\n')
	iline--;
*backp++ = c;
if (backp >= backup+BACKMAX)
	error("too much backup");
}
get1char()
{
int c;
if (backp>backup)
	c = *--backp;
else
	c=getc(tabin);
if (c== EOF) /* EOF */
	{
	if (swapin() ==0)
		error("unexpected EOF");
	c = getc(tabin);
	}
if (c== '\n')
	iline++;
return(c);
}
