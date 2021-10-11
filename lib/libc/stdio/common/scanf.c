#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)scanf.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <errno.h>

#define ON	1
#define OFF	0

#define ARGMAX  64
static unsigned char newap[ARGMAX * sizeof(double)];
static unsigned char newform[256];
extern char *strcpy();

extern int _doscan();
static int format_arg();

/*VARARGS1*/
int
scanf(fmt, va_alist)
char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	if (format_arg(strcpy(newform, fmt), ap, newap) == ON) {
		return(_doscan(stdin, newform, newap));
	}
	return(_doscan(stdin, fmt, ap));
}

/*VARARGS2*/
int
fscanf(iop, fmt, va_alist)
FILE *iop;
char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
#ifdef POSIX
	if ( !(iop->_flag & (_IOREAD|_IORW)) ) {
		iop->_flag |= _IOERR;
		errno = EBADF;
		return (EOF);
	}
#endif POSIX
	if (format_arg(strcpy(newform, fmt), ap, newap) == ON)
		return(_doscan(iop, newform, newap));
	return(_doscan(iop, fmt, ap));
}

/*VARARGS2*/
int
sscanf(str, fmt, va_alist)
register char *str;
char *fmt;
va_dcl
{
	va_list ap;
	FILE strbuf;

	va_start(ap);
	strbuf._flag = _IOREAD|_IOSTRG;
	strbuf._ptr = strbuf._base = (unsigned char*)str;
	strbuf._cnt = strlen(str);
	strbuf._bufsiz = strbuf._cnt;
	if (format_arg(strcpy(newform, fmt), ap, newap) == ON)
		return(_doscan(&strbuf, newform, newap));
	return(_doscan(&strbuf, fmt, ap));
}

/*
 * This function reorganises the format string and argument list.
 */


#ifndef NL_ARGMAX
#define NL_ARGMAX	9
#endif

struct al {
	int a_num;		/* arg # specified at this position */
	unsigned char *a_start;	/* ptr to 'n' part of '%n$' in format str */
	unsigned char *a_end;	/* ptr to '$'+1 part of '%n$' in format str */
	int *a_val;		/* pointers to arguments */
};

static int
format_arg(format, list, newlist)
unsigned char *format, *list, *newlist;
{
	unsigned char *aptr, *bptr, *cptr;
	register i, fcode, nl_fmt, num, length, j;
	unsigned char *fmtsav;
	struct al args[ARGMAX + 1];

#ifdef VTEST
	{
		int fd;
		fd = creat("/tmp/SCANF", 0666);
	}
#endif
	for (i = 0; i <= ARGMAX; args[i++].a_num = 0);
	nl_fmt = 0;
	i = j = 1;
	while (*format) {
		while ((fcode = *format++) != '\0' && fcode != '%') ;
		if (!fcode || i > ARGMAX)
			break;
	charswitch:
		switch (fcode = *format++) {
		case 'l':
		case 'h':
			goto charswitch;
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9':
			num = fcode - '0';
			fmtsav = format;
			while (isdigit(fcode = *format)) {
				num = num * 10 + fcode - '0';
				format++;
			}
			if (*format == '$') {
				nl_fmt++;
				args[i].a_start = fmtsav - 1;
				args[i].a_end = ++format;
				if (num > NL_ARGMAX)
					num = num;
				args[i].a_num = num;
			}
			goto charswitch;
	/* now have arg type only to parse */
		case 'd': case 'u': case 'o':
		case 'x': case 'e': case 'f':
		case 'g': case 'c': case '[':
		case 's':
			if (nl_fmt == 0)
				return(OFF);
			if (!args[i].a_num) {
				args[i].a_start = args[i].a_end = format - 1;
				args[i].a_num = j++;
			}
			i++;
			break;
		case '*':
		case '%':
			break;
		default:
			format--;
			break;
		}
	}
	length = i;
	if (nl_fmt == 0)
		return (OFF);
	for (i = 1; i < length && args[i].a_num == 0; i++);

	/*
	 * Reformat the format string
	 */
	cptr = aptr = args[i].a_start;
	do {
		bptr = args[i++].a_end;
		for (; i < length && args[i].a_num == 0; i++);
		if (i == length) 
			while (*cptr++);
		else
			cptr = args[i].a_start;
		for (; bptr != cptr; *aptr++ = *bptr++);
	} while (i < length);

	/*
	 * Create arglist
	 * assuming that pointer to all variable type have
	 * same size.
	 */
	for (i = 1; i < length; i++)
		args[i].a_val = ((int **)(list += sizeof(int *)))[-1];

	for (i = 1; i < length; i++) {
		int **ptr;
		ptr = (int **)newlist;
		*ptr = args[args[i].a_num].a_val;
		newlist += sizeof(int *);
	}
	return(ON);
}
