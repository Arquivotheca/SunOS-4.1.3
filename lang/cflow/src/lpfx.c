/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)lpfx.c 1.1 92/07/30 SMI"; /* from S5R3 1.7 */
#endif

#include <stdio.h>
#include <ctype.h>
#include "lerror.h"
#include "mip.h"
#include "lmanifest.h"
#include "lpass2.h"
#define FNSIZE LFNM
#define NSIZE LCHNM

typedef struct {
	union rec r;
#ifdef FLEXNAMES
	char *fname;
#else
	char fname[FNSIZE + 1];
#endif
	} funct;

typedef struct LI {
	struct LI *next;
	funct fun;
	} li;

extern void	exit();

/*
 * lpfx - read lint1 output, sort and format for dag
 *
 *	options -i_ -ix (inclusion)
 *
 *	while (read lint1 output into "funct" structures)
 *		if (this is a filename record)
 *			save filename
 *		else
 *			read arg records and throw on floor
 *			if (this is to be included)
 *				copy filename into "funct"
 *				insert into list
 *	format and print
 */


main(argc, argv)
	int argc;
	char **argv;
	{
	extern int optind;
	extern char *optarg;
	funct fu;
	int uscore, fmask, c;
	void rdargs(), insert(), putout();
#ifdef FLEXNAMES
	char *filename, *funcname, *getstr();
#else
	char filename[FNSIZE + 1], *strncpy();
#endif

	fmask = LDS | LDX | LRV;
	uscore = 0;
	while ((c = getopt(argc, argv, "i:")) != EOF)
		if (c == 'i')
			if (*optarg == '_')
				uscore = 1;
			else if (*optarg == 'x')
				fmask &= ~(LDS | LDX);
			else
				goto argerr;
		else
		argerr:
			(void)fprintf(stderr, "lpfx: bad option %c ignored\n", c);
	while (0 < fread((char *)&fu.r, sizeof(fu.r), 1, stdin))
		if (fu.r.l.decflag & LFN)
			{
#ifdef FLEXNAMES
			filename = fu.r.f.fn = getstr();
#else
			(void)strncpy(filename, fu.r.f.fn, FNSIZE);
			filename[FNSIZE] = '\0';
#endif
			}
		else
			{
#ifdef FLEXNAMES
			funcname = fu.r.l.name = getstr();
#endif
			rdargs(&fu);
			if (((fmask & LDS) ? ISFTN(fu.r.l.type.aty) :
			      !(fu.r.l.decflag & fmask)) &&
			    ((uscore) ? 1 : (*fu.r.l.name != '_')))
				{
#ifdef FLEXNAMES
				fu.fname = filename;
#else
				(void)strncpy(fu.fname, filename, FNSIZE);
				fu.fname[FNSIZE] = '\0';
#endif
				insert(&fu);
				}
			}
	putout();
	}

/* getstr - get strings from intermediate file
 *
 * simple while loop reading into static buffer
 * transfer into malloc'ed buffer
 * panic and die if format or malloc error
 *
 */

char *getstr()
	{
	static char buf[BUFSIZ];
	char *malloc(), *strcpy();
	register int c;
	register char *p = buf;

	while ((c = getchar()) != EOF)
		{
		*p++ = c;
		if (c == '\0' || !isascii(c))
			break;
		}
	if (c != '\0')
		{
		fputs("lpfx: PANIC! Intermediate file string format error\n",
		    stderr);
		exit(1);
		/*NOTREACHED*/
		}
	if (!(p = malloc(strlen(buf) + 1)))
		{
		fputs("lpfx: out of heap space\n", stderr);
		exit(1);
		/*NOTREACHED*/
		}
	return (strcpy(p, buf));
	}

/*
 * rdargs - read arg records and throw on floor
 *
 *	if ("funct" has args)
 *		get absolute value of nargs
 *		if (too many args)
 *			panic and die
 *		read args into temp array
 */

void rdargs(pf)
	register funct *pf;
	{
	struct ty atype[50];

	if (pf->r.l.nargs)
		{
		if (pf->r.l.nargs < 0)
			pf->r.l.nargs = -pf->r.l.nargs - 1;
		if (pf->r.l.nargs > 50)
			{
			(void) fprintf(stderr, "lpfx: PANIC! nargs=%d\n",
			    pf->r.l.nargs);
			exit(1);
			}
		if (fread((char *)atype, sizeof(ATYPE), pf->r.l.nargs, stdin) <= 0)
			{
			(void)perror("lpfx.rdargs");
			exit(1);
			}
		}
	}

/*
 * insert - insertion sort into (singly) linked list
 *
 *	stupid linear list insertion
 */

static li *head = NULL;

void insert(pfin)
	register funct *pfin;
	{
	register li *list_item, *newf;
	char *malloc();

	if ((newf = (li *)malloc(sizeof(li))) == NULL)
		{
		(void)fprintf(stderr, "lpfx: out of heap space\n");
		exit(1);
		}
	newf->fun = *pfin;
	if (list_item = head)
		if (newf->fun.r.l.fline < list_item->fun.r.l.fline)
			{
			newf->next = head;
			head = newf;
			}
		else
			{
			while (list_item->next &&
			  list_item->next->fun.r.l.fline < newf->fun.r.l.fline)
				list_item = list_item->next;
			while (list_item->next &&
			  list_item->next->fun.r.l.fline == newf->fun.r.l.fline &&
			  list_item->next->fun.r.l.decflag < newf->fun.r.l.decflag)
					list_item = list_item->next;
			newf->next = list_item->next;
			list_item->next = newf;
			}
	else
		{
		head = newf;
		newf->next = NULL;
		}
	}

/*
 * putout - format and print sorted records
 *
 *	while (there are records left)
 *		copy name and null terminate
 *		if (this is a definition)
 *			if (this is a function**)
 *				save name for reference formatting
 *			print definition format
 *		else if (this is a reference)
 *			print reference format
 *
 *	** as opposed to external/static variable
 */

void putout()
	{
	register li *pli;
#ifdef FLEXNAMES
	char lname[BUFSIZ], name[BUFSIZ];
#else
	char lname[NSIZE+1], name[NSIZE+1];
#endif
	char *prtype(), *strncpy(), *strcpy();
	
	pli = head;
	name[0] = lname[0] = '\0';
	while (pli != NULL)
		{
#ifdef FLEXNAMES
		(void) strcpy(name, pli->fun.r.l.name);
#else
		(void)strncpy(name, pli->fun.r.l.name, NSIZE);
		name[NSIZE] = '\0';
#endif
		if (pli->fun.r.l.decflag & (LDI | LDC | LDS))
			{
			if (ISFTN(pli->fun.r.l.type.aty))
				(void)strcpy(lname, name);
			(void)printf("%s = %s, <%s %d>\n", name, prtype(pli),
			    pli->fun.fname, pli->fun.r.l.fline);
			}
		else if (pli->fun.r.l.decflag & (LUV | LUE | LUM))
			(void)printf("%s : %s\n", lname, name);
		pli = pli->next;
		}
	}

static char *types[] = {
	"void", "???", "char", "short", "int", "long", "float",
	"double", "struct", "union", "enum", "???", "unsigned char",
	"unsigned short", "unsigned int", "unsigned long"};

/*
 * prtype - decode type fields
 *
 *	strictly arcana
 */

char *prtype(pli)
	register li *pli;
	{
	static char bigbuf[64];
	char buf[32], *shift(), *strcpy(), *strcat();
	register char *bp;
	register int typ;

	typ = pli->fun.r.l.type.aty;
	(void)strcpy(bigbuf, types[typ & 017]);
	*(bp = buf) = '\0';
	for (typ >>= 4; typ > 0; typ >>= 2)
		switch (typ & 03)
			{
			case 1:
				bp = shift(buf);
				buf[0] = '*';
				break;
			case 2:
				*bp++ = '(';
				*bp++ = ')';
				*bp = '\0';
				break;
			case 3:
				*bp++ = '[';
				*bp++ = ']';
				*bp = '\0';
				break;
			}
	(void)strcat(bigbuf, buf);
	return(bigbuf);
	}

char *shift(s)
	register char *s;
	{
	register char *p1, *p2;
	char *rp;

	for (p1 = s; *p1; ++p1)
		;
	rp = p2 = p1++;
	while (p2 >= s)
		*p1-- = *p2--;
	return(++rp);
	}
