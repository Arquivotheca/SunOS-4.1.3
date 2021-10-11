/*
 * lexical analyzer for colldef 
 */

/* static  char *sccsid = "@(#)lex.c 1.1 92/07/30 SMI"; */

#include "colldef.h"
#include "y.tab.h"
#include "extern.h"

#define NL    	'\n'
#define QUOTE 	'\"'
#define SLASH 	'\\'
#define COMMENT '#'

#define ISSEP(_P)	(_P == ',' || _P == ';' || _P == '(' || _P == ')' ||\
			 _P == '{' || _P == '}' || _P == '<' || _P == '>')
#define NOT_ISSEP(_P)	(_P != ',' && _P != ';' && _P != '(' && _P != ')' &&\
			 _P != '{' && _P != '}' && _P != '<' && _P != '>')

static char ibuf[IDSIZE];
yylex()	
{
	int c;
	char sbuf[IDSIZE];

	while ((c = getc(ifp)) == ' ' || c == '\t')
		;
	if (c == EOF)
		return(0);
	else if (c == QUOTE) {  /* character string */
		char *p = sbuf;
		while (((c = getc(ifp)) != QUOTE) && c != EOF) {
			*p++ = c;
			if (p > &sbuf[IDSIZE-2])
				warning("lex overflow");
		}
		*p = '\0';
		xstrcpy(ibuf, sbuf);
		yylval.id = ibuf;
		return(QSTRING);
	}
	else if (ISSEP(c))
		return (c);

	else if (c == COMMENT) {/* comment */
		while ((c = getc(ifp)) != NL);
		ungetc(c, ifp);
		return(yylex());
	}

	else if (c == NL) {
		lineno++;
		return(c);
	}

	else {
		char *p = sbuf;
		char d;
		int  rval, lval;

		if (c == SLASH) {
			if ((d = getc(ifp)) == NL) {
				lineno++;
				return(yylex());
				}
			else
				ungetc(d, ifp);
			}

		do {
			*p++ = c;
			if (p > &sbuf[IDSIZE-2])
				warning("lex overflow");
		}  while ((c = getc(ifp)) != EOF  && NOT_ISSEP(c)  &&
			   c != NL && c != ' ' && c != '\t' && c!=COMMENT);
		ungetc(c, ifp);
		*p = '\0';

		/*
		 * check for key words and if it is a keyword,
		 *  return it !
		 */

		 if ((rval = lookkey(sbuf)) != UNDEF) {
			return(rval);
		 }

		 /*
		  * STRING or SYM_NAME
		  */

		 xstrcpy(ibuf, sbuf);
		 yylval.id = ibuf;
		 return(SYM_NAME);
	}
}

/*
 * check for keyword 
 */
lookkey(s)
	char *s;
{
	if (strcmp(s, "charmap") == 0)
		return(CHARMAP);
	else if (strcmp(s, "substitute") == 0)
		return(SUBSTITUTE);
	else if (strcmp(s, "with") == 0)
		return(WITH);
	else if (strcmp(s, "order") == 0)
		return(ORDER);
	else if (strcmp(s, "...") == 0)
		return(ELLIPSIS);
	return(0);
}

/*
 * similar to strcpy(), but xstrcpy allows specifying a char as its
 * hex or oct format.  E.g. xstrcpy(s1, "\x41")  then s1 is "A".
 */

xstrcpy(s1, s2)
	char *s1, *s2;
{
	char *p;
	char t[4];

	p = s1;
	while (*s2) {
	   if (*s2 != SLASH) 
		*p++ = *s2++;
	   else { /* assume hexadecimal or octal format */
	        if (*++s2 == 'x') { /* hexadecimal */
		   	t[0] = *++s2;
		   	t[1] = *++s2;
		   	t[2] = '\0';
			*p++ = axtoi(t);
			s2++;
			}
		else { /* octals */
		   	t[0] = *s2;
		   	t[1] = *++s2;
		   	t[2] = *++s2;
		   	t[3] = '\0';
			*p++ = aotoi(t);
			s2++;
		   	}
		}
	   }
	*p = '\0';
}
