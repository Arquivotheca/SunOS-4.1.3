/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)lookup.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

/*	ctrace - C program debugging tool
 *
 *	keyword look-up routine for the scanner
 *
 */
#include "global.h"
#include "y.tab.h"

enum	bool stdio_preprocessed = no;	/* stdio.h already preprocessed */
enum	bool setjmp_preprocessed = no;	/* setjmp.h already preprocessed */

static	struct	keystruct {
	char	*text;
	int	token;
	struct	keystruct *next;
} keyword[] = {
	"BADMAG",	MACRO,		NULL,
	"EOF",		CONSTANT,	NULL,
	"NULL",		CONSTANT,	NULL,
	"auto",		CLASS,		NULL,
	"break",	BREAK_CONT,	NULL,
	"case",		CASE,		NULL,
	"char",		TYPE,		NULL,
	"continue",	BREAK_CONT,	NULL,
	"default",	DEFAULT,	NULL,
	"do",		DO,		NULL,
	"double",	TYPE,		NULL,
	"else",		ELSE,		NULL,
	"enum",		ENUM,		NULL,
	"extern",	CLASS,		NULL,
	"fgets",	STRRES,		NULL,
	"float",	TYPE,		NULL,
	"for",		FOR,		NULL,
	"fortran",	CLASS,		NULL,
	"gets",		STRRES,		NULL,
	"goto",		GOTO,		NULL,
	"if",		IF,		NULL,
	"int",		TYPE,		NULL,
	"long",		TYPE,		NULL,
	"register",	CLASS,		NULL,
	"return",	RETURN,		NULL,
	"short",	TYPE,		NULL,
	"sizeof",	SIZEOF,		NULL,
	"static",	CLASS,		NULL,
	"stderr",	CONSTANT,	NULL,
	"stdin",	CONSTANT,	NULL,
	"stdout",	CONSTANT,	NULL,
	"strcat",	STRCAT,		NULL,
	"strcmp",	STRCMP,		NULL,
	"strcpy",	STRCPY,		NULL,
	"strlen",	STRLEN,		NULL,
	"strncat",	STRNCAT,	NULL,
	"strncmp",	STRNCMP,	NULL,
	"struct",	STRUCT_UNION,	NULL,
	"switch",	SWITCH,		NULL,
	"typedef",	TYPEDEF,	NULL,
	"union",	STRUCT_UNION,	NULL,
	"unsigned",	TYPE,		NULL,
	"void",		TYPE,		NULL,
	"while",	WHILE,		NULL,
	"_iobuf",	IOBUF,		NULL,
	"jmp_buf",	JMP_BUF,	NULL,
};
#define KEYWORDS	(sizeof(keyword) / sizeof(struct keystruct))

#define HASHSIZE	KEYWORDS + 10	/* approximate number of keywords and typedef names */

static	struct	keystruct *hashtab[HASHSIZE]; /* pointer table */

/* put the keywords into the symbol table */
init_symtab()
{
	register int	i;
	
	for (i = 0; i < KEYWORDS; ++i)
		hashlink(&keyword[i]);
}

/* see if this identifier is a keyword or typedef name */
lookup(ident)
register char	*ident;
{
	register struct	keystruct *p;
	
	/* look up the identifier in the symbol table */
	for (p = hashtab[hash(ident)]; p != NULL; p = p->next)
		if (strcmp(ident, p->text) == 0) {
			if (p->token == IOBUF) {	/* Unix 3.0 check */
				stdio_preprocessed = yes;
				return(IDENT);
			}
			if (p->token == JMP_BUF) {
				setjmp_preprocessed = yes;
				return(IDENT);
			}
			return(p->token);
			}
	/* this is an identifier */
	return(IDENT);
}

/* add a type to the symbol table */
add_type(ident)
char	*ident;
{
	add_symbol(ident, TYPE);
}

/* add an identifier to the symbol table */
add_symbol(ident, type)
char	*ident;
int	type;
{
	struct	keystruct *p;
	char	*strsave(), *malloc();
	int	t;
	
	if ((t = lookup(ident)) == IDENT) {
		p = (struct keystruct *) malloc(sizeof(*p));
		if (p == NULL || (p->text = strsave(ident)) == NULL) {
			fatal("out of storage");
		}
		else {
			t = p->token = type;
			hashlink(p);
			if (type == TYPE && strcmp(p->text, "FILE") == 0) /* Unix 4.0 check */
				stdio_preprocessed = yes;
		}
	}
	return(t);
}
char *
strsave(s)
char	*s;
{
	char	*p, *malloc(), *strcpy();
	
	if ((p = malloc((unsigned) (strlen(s) + 1))) != NULL)
		strcpy(p, s);
	return(p);
}
static
hashlink(p)
register struct	keystruct *p;
{
	register int	hashval;
	
	hashval = hash(p->text);
	p->next = hashtab[hashval];
	hashtab[hashval] = p;
}
static
hash(s)	/* form hash value for string */
register char	*s;
{
	register int	hashval;
	
	for (hashval = 0; *s != '\0'; )
		hashval += *s++;
	return(hashval % HASHSIZE);
}
