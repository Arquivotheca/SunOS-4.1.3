#ifndef lint
static	char sccsid[] = "@(#)cb.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

#include <stdio.h>
/* character type flags */
#define	_U	01
#define	_L	02
#define	_N	04
#define	_S	010
#define _P	020
#define _C	040
#define	_X	0100
#define _O	0200

extern	char	_chtype_[];

#define isop(c)	((_chtype_+1)[c]&_O)
#define	isalpha(c)	((_chtype_+1)[c]&(_U|_L))
#define	isupper(c)	((_chtype_+1)[c]&_U)
#define	islower(c)	((_chtype_+1)[c]&_L)
#define	isdigit(c)	((_chtype_+1)[c]&_N)
#define	isxdigit(c)	((_chtype_+1)[c]&(_N|_X))
#define	isspace(c)	((_chtype_+1)[c]&_S)
#define ispunct(c)	((_chtype_+1)[c]&_P)
#define isalnum(c)	((_chtype_+1)[c]&(_U|_L|_N))
#define isprint(c)	((_chtype_+1)[c]&(_P|_U|_L|_N))
#define iscntrl(c)	((_chtype_+1)[c]&_C)
#define isascii(c)	((unsigned)(c)<=0177)
#define toupper(c)	((c)-'a'+'A')
#define tolower(c)	((c)-'A'+'a')
#define toascii(c)	((c)&0177)
char _chtype_[] = {
	0,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
	_S,	_P|_O,	_P,	_P,	_P,	_P|_O,	_P|_O,	_P,
	_P,	_P,	_P|_O,	_P|_O,	_P,	_P|_O,	_P,	_P|_O,
	_N,	_N,	_N,	_N,	_N,	_N,	_N,	_N,
	_N,	_N,	_P,	_P,	_P|_O,	_P|_O,	_P|_O,	_P,
	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
	_U,	_U,	_U,	_P,	_P,	_P,	_P|_O,	_P|_L,
	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
	_L,	_L,	_L,	_P,	_P|_O,	_P,	_P,	_C
};

#define	IF	1
#define	ELSE	2
#define	CASE	3
#define TYPE	4
#define DO	5
#define STRUCT	6
#define OTHER	7

#define ALWAYS	01
#define	NEVER	02
#define	SOMETIMES	04

#define YES	1
#define NO	0

#define	KEYWORD	1
#define	DATADEF	2
#define	SINIT	3

#define CLEVEL	20
#define IFLEVEL	10
#define DOLEVEL	10
#define OPLENGTH	10
#define LINE	256
#define LINELENG	120
#define MAXTABS	8
#define TABLENG	8
#define TEMP	1024

#define OUT	outs(clev->tabs); putchar('\n');opflag = lbegin = 1; count = 0
#define OUTK	OUT; keyflag = 0;
#define BUMP	clev->tabs++; clev->pdepth++
#define UNBUMP	clev->tabs -= clev->pdepth; clev->pdepth = 0
#define eatspace()	while((cc=getch()) == ' ' || cc == '\t'); unget(cc)
#define eatallsp()	while((cc=getch()) == ' ' || cc == '\t' || cc == '\n'); unget(cc)

struct indent {		/* one for each level of { } */
	int tabs;
	int pdepth;
	int iflev;
	int ifc[IFLEVEL];
	int spdepth[IFLEVEL];
} ind[CLEVEL];
struct indent *clev = ind;
struct keyw {
	char	*name;
	char	punc;
	char	type;
} key[] = {
	"switch", ' ', OTHER,
	"do", ' ', DO,
	"while", ' ', OTHER,
	"if", ' ', IF,
	"for", ' ', OTHER,
	"else", ' ', ELSE,
	"case", ' ', CASE,
	"default", ' ', CASE,
	"char", '\t', TYPE,
	"int", '\t', TYPE,
	"short", '\t', TYPE,
	"long", '\t', TYPE,
	"unsigned", '\t', TYPE,
	"float", '\t', TYPE,
	"double", '\t', TYPE,
	"struct", ' ', STRUCT,
	"union", ' ', STRUCT,
	"extern", ' ', TYPE,
	"register", ' ', TYPE,
	"static", ' ', TYPE,
	"typedef", ' ', TYPE,
	0, 0, 0
};
struct keyw *lookup();
struct op {
	char	*name;
	char	blanks;
	char	setop;
} op[] = {
	"+=", 	ALWAYS,  YES,
	"-=", 	ALWAYS,  YES,
	"*=", 	ALWAYS,  YES,
	"/=", 	ALWAYS,  YES,
	"%=", 	ALWAYS,  YES,
	">>=", 	ALWAYS,  YES,
	"<<=", 	ALWAYS,  YES,
	"&=", 	ALWAYS,  YES,
	"^=", 	ALWAYS,  YES,
	"|=", 	ALWAYS,  YES,
	">>", 	ALWAYS,  YES,
	"<<", 	ALWAYS,  YES,
	"<=", 	ALWAYS,  YES,
	">=", 	ALWAYS,  YES,
	"==", 	ALWAYS,  YES,
	"!=", 	ALWAYS,  YES,
	"=", 	ALWAYS,  YES,
	"&&", 	ALWAYS, YES,
	"||", 	ALWAYS, YES,
	"++", 	NEVER, NO,
	"--", 	NEVER, NO,
	"->", 	NEVER, NO,
	"<", 	ALWAYS, YES,
	">", 	ALWAYS, YES,
	"+", 	ALWAYS, YES,
	"/", 	ALWAYS, YES,
	"%", 	ALWAYS, YES,
	"^", 	ALWAYS, YES,
	"|", 	ALWAYS, YES,
	"!", 	NEVER, YES,
	"~", 	NEVER, YES,
	"*", 	SOMETIMES, YES,
	"&", 	SOMETIMES, YES,
	"-", 	SOMETIMES, YES,
	"?",	ALWAYS,YES,
	":",	ALWAYS,YES,
	0, 	0,0
};
FILE *input = stdin;
char *getnext();
int	strict = 0;
int	join	= 0;
int	opflag = 1;
int	keyflag = 0;
int	paren	 = 0;
int	split	 = 0;
int	folded	= 0;
int	dolevel	=0;
int	dotabs[DOLEVEL];
int	docurly[DOLEVEL];
int	dopdepth[DOLEVEL];
int	structlev = 0;
int	question	 = 0;
char	string[LINE];
char	*lastlook;
char	*p = string;
char temp[TEMP];
char *tp;
int err = 0;
char *lastplace = temp;
char *tptr = temp;
int maxleng	= LINELENG;
int maxtabs	= MAXTABS;
int count	= 0;
int next = '\0';
int	inswitch	=0;
int	lbegin	 = 1;
main(argc, argv) int	argc;
char	*argv[];
{
	int	c;
	extern char	*optarg;
	extern int	optind;
	while ((c = getopt(argc, argv, "jl:s")) != -1)
		switch (c) {
		case 's':
			strict = 1;
			break;
		case 'j':
			join = 1;
			break;
		case 'l':
			maxleng = atoi(optarg);
			maxtabs = maxleng/TABLENG - 2;
			maxleng -= .1 * maxleng;
			break;
		default:
			fprintf(stderr, "usage: %s [-s] [-j] %s", argv[0],
				"[-l leng] [filename ...]\n");
			exit(1);
		}
	if (optind >= argc)work("standard input");
	else {
		while (optind < argc){
			if ((input = fopen( argv[optind], "r")) == NULL){
				fprintf(stderr, "cb: ");
				perror(argv[optind]);
				exit(1);
			}
			work(argv[optind]);
			fclose(input);
			optind++;
		}
	}
	return(0);
}
work(filename)
char *filename;
{
	register int c;
	register struct keyw *lptr;
	char *pt;
	char cc;
	int ct;

	while ((c = getch()) != EOF){
		switch (c){
		case '{':
			if ((lptr = lookup(lastlook,p)) != 0){
				if (lptr->type == ELSE)gotelse();
				else if(lptr->type == DO)gotdo();
				else if(lptr->type == STRUCT)structlev++;
			}
			if(++clev >= &ind[CLEVEL-1]){
				fprintf(stderr,"too many levels of curly brackets\n");
				clev = &ind[CLEVEL-1];
			}
			clev->pdepth = 0;
			clev->tabs = (clev-1)->tabs;
			clearif(clev);
			if(strict && clev->tabs > 0)
				putspace(' ',NO);
			putch(c,NO);
			getnl();
			if(keyflag == DATADEF){
				OUT;
			}
			else {
				OUTK;
			}
			clev->tabs++;
			pt = getnext(0);		/* to handle initialized structures */
			if(*pt == '{'){		/* hide one level of {} */
				while((c=getch()) != '{') {
					if (c == EOF)
						goto done;
				}
				putch(c,NO);
				if(strict){
					putch(' ',NO);
					eatspace();
				}
				keyflag = SINIT;
			}
			continue;
		case '}':
			pt = getnext(0);		/* to handle initialized structures */
			if(*pt == ','){
				if(strict){
					putspace(' ',NO);
					eatspace();
				}
				putch(c,NO);
				putch(*pt,NO);
				*pt = '\0';
				ct = getnl();
				pt = getnext(0);
				if(*pt == '{'){
					OUT;
					while((cc = getch()) != '{') {
						if (cc == EOF)
							goto done;
					}
					putch(cc,NO);
					if(strict){
						putch(' ',NO);
						eatspace();
					}
					pt = getnext(0);
					continue;
				}
				else if(strict || ct){
					OUT;
				}
				continue;
			}
			else if(keyflag == SINIT && *pt == '}'){
				if(strict)
					putspace(' ',NO);
				putch(c,NO);
				getnl();
				OUT;
				keyflag = DATADEF;
				*pt = '\0';
				pt = getnext(0);
			}
			outs(clev->tabs);
			if(--clev < ind)clev = ind;
			ptabs(clev->tabs);
			putch(c,NO);
			lbegin = 0;
			lptr=lookup(pt,lastplace+1);
			c = *pt;
			if(*pt == ';' || *pt == ','){
				putch(*pt,NO);
				*pt = '\0';
				lastplace=pt;
			}
			ct = getnl();
			if((dolevel && clev->tabs <= dotabs[dolevel]) || (structlev )
			    || (lptr != 0 &&lptr->type == ELSE&& clev->pdepth == 0)){
				if(c == ';'){
					OUTK;
				}
				else if(strict || (lptr != 0 && lptr->type == ELSE && ct == 0)){
					putspace(' ',NO);
					eatspace();
				}
				else if(lptr != 0 && lptr->type == ELSE){
					OUTK;
				}
				if(structlev){
					structlev--;
					keyflag = DATADEF;
				}
			}
			else {
				OUTK;
				if(strict && clev->tabs == 0){
					if((c=getch()) != '\n'){
						putchar('\n');
						putchar('\n');
						unget(c);
					}
					else {
						putchar('\n');
						if((c=getch()) != '\n')unget(c);
						putchar('\n');
					}
				}
			}
			if(lptr != 0 && lptr->type == ELSE && clev->pdepth != 0){
				UNBUMP;
			}
			if(lptr == 0 || lptr->type != ELSE){
				clev->iflev = 0;
				if(dolevel && docurly[dolevel] == NO && clev->tabs == dotabs[dolevel]+1)
					clev->tabs--;
				else if(clev->pdepth != 0){
					UNBUMP;
				}
			}
			continue;
		case '(':
			paren++;
			if ((lptr = lookup(lastlook,p)) != 0){
				if(!(lptr->type == TYPE || lptr->type == STRUCT))keyflag=KEYWORD;
				if (strict){
					putspace(lptr->punc,NO);
					opflag = 1;
				}
				putch(c,NO);
				if (lptr->type == IF)gotif();
			}
			else {
				putch(c,NO);
				lastlook = p;
				opflag = 1;
			}
			continue;
		case ')':
			if(--paren < 0)paren = 0;
			putch(c,NO);
			if((lptr = lookup(lastlook,p)) != 0){
				if(lptr->type == TYPE || lptr->type == STRUCT)
					opflag = 1;
			}
			else if(keyflag == DATADEF)opflag = 1;
			else opflag = 0;
			outs(clev->tabs);
			pt = getnext(1);
			if ((ct = getnl()) == 1 && !strict){
				if(dolevel && clev->tabs <= dotabs[dolevel])
					resetdo();
				if(clev->tabs > 0 && (paren != 0 || keyflag == 0)){
					if(join){
						eatspace();
						putch(' ',YES);
						continue;
					} else {
						OUT;
						split = 1;
						continue;
					}
				}
				else if(clev->tabs > 0 && *pt != '{'){
					BUMP;
				}
				OUTK;
			}
			else if(strict){
				if(clev->tabs == 0){
					if(*pt != ';' && *pt != ',' && *pt != '(' && *pt != '['){
						OUTK;
					}
				}
				else {
					if(keyflag == KEYWORD && paren == 0){
						if(dolevel && clev->tabs <= dotabs[dolevel]){
							resetdo();
							eatspace();
							continue;
						}
						if(*pt != '{'){
							BUMP;
							OUTK;
						}
						else {
							*pt='\0';
							eatspace();
							unget('{');
						}
					}
					else if(ct){
						if(paren){
							if(join){
								eatspace();
							} else {
								split = 1;
								OUT;
							}
						}
						else {
							OUTK;
						}
					}
				}
			}
			else if(dolevel && clev->tabs <= dotabs[dolevel])
				resetdo();
			continue;
		case ' ':
		case '\t':
			if ((lptr = lookup(lastlook,p)) != 0){
				if(!(lptr->type==TYPE||lptr->type==STRUCT))
					keyflag = KEYWORD;
				else if(paren == 0)keyflag = DATADEF;
				if(strict){
					if(lptr->type != ELSE){
						if(lptr->type == TYPE){
							if(paren != 0)putch(' ',YES);
						}
						else
							putch(lptr->punc,NO);
						eatspace();
					}
				}
				else putch(c,YES);
				switch(lptr->type){
				case CASE:
					outs(clev->tabs-1);
					continue;
				case ELSE:
					pt = getnext(1);
					eatspace();
					if((cc = getch()) == '\n' && !strict){
						unget(cc);
					}
					else {
						unget(cc);
						if(checkif(pt))continue;
					}
					gotelse();
					if(strict) unget(c);
					if(getnl() == 1 && !strict){
						OUTK;
						if(*pt != '{'){
							BUMP;
						}
					}
					else if(strict){
						if(*pt != '{'){
							OUTK;
							BUMP;
						}
					}
					continue;
				case IF:
					gotif();
					continue;
				case DO:
					gotdo();
					pt = getnext(1);
					if(*pt != '{'){
						eatallsp();
						OUTK;
						docurly[dolevel] = NO;
						dopdepth[dolevel] = clev->pdepth;
						clev->pdepth = 0;
						clev->tabs++;
					}
					continue;
				case TYPE:
					if(paren)continue;
					if(!strict)continue;
					gottype(lptr);
					continue;
				case STRUCT:
					gotstruct();
					continue;
				}
			}
			else if (lbegin == 0 || p > string) 
				if(strict)
					putch(c,NO);
				else putch(c,YES);
			continue;
		case ';':
			putch(c,NO);
			if(paren != 0){
				if(strict){
					putch(' ',YES);
					eatspace();
				}
				opflag = 1;
				continue;
			}
			outs(clev->tabs);
			pt = getnext(0);
			lptr=lookup(pt,lastplace+1);
			if(lptr == 0 || lptr->type != ELSE){
				clev->iflev = 0;
				if(clev->pdepth != 0){
					UNBUMP;
				}
				if(dolevel && docurly[dolevel] == NO && clev->tabs <= dotabs[dolevel]+1)
					clev->tabs--;
/*
				else if(clev->pdepth != 0){
					UNBUMP;
				}
*/
			}
			getnl();
			OUTK;
			continue;
		case '\n':
			if ((lptr = lookup(lastlook,p)) != 0){
				pt = getnext(1);
				if (lptr->type == ELSE){
					if(strict)
						if(checkif(pt))continue;
					gotelse();
					OUTK;
					if(*pt != '{'){
						BUMP;
					}
				}
				else if(lptr->type == DO){
					OUTK;
					gotdo();
					if(*pt != '{'){
						docurly[dolevel] = NO;
						dopdepth[dolevel] = clev->pdepth;
						clev->pdepth = 0;
						clev->tabs++;
					}
				}
				else {
					OUTK;
					if(lptr->type == STRUCT)gotstruct();
				}
			}
			else if(p == string)putchar('\n');
			else {
				if(clev->tabs > 0 &&(paren != 0 || keyflag == 0)){
					if(join){
						putch(' ',YES);
						eatspace();
						continue;
					} else {
						OUT;
						split = 1;
						continue;
					}
				}
				else if(keyflag == KEYWORD){
					OUTK;
					continue;
				}
				OUT;
			}
			continue;
		case '"':
		case '\'':
			putch(c,NO);
			while ((cc = getch()) != c){
				if (cc == EOF)
					goto done;
				putch(cc,NO);
				if (cc == '\\'){
					cc = getch();
					if (cc == EOF)
						goto done;
					putch(cc,NO);
				}
				else if (cc == '\n'){
					outs(clev->tabs);
					lbegin = 1;
					count = 0;
				}
			}
			putch(cc,NO);
			opflag=0;
			if (getnl() == 1){
				unget('\n');
			}
			continue;
		case '\\':
			putch(c,NO);
			cc = getch();
			if (cc == EOF)
				goto done;
			putch(cc,NO);
			continue;
		case '?':
			question = 1;
			gotop(c);
			continue;
		case ':':
			if (question == 1){
				question = 0;
				gotop(c);
				continue;
			}
			putch(c,NO);
			if(structlev)continue;
			if ((lptr = lookup(lastlook,p)) != 0){
				if (lptr->type == CASE)outs(clev->tabs - 1);
			}
			else {
				lbegin = 0;
				outs(clev->tabs);
			}
			getnl();
			OUTK;
			continue;
		case '/':
			if ((cc = getch()) != '*'){
				unget(cc);
				gotop(c);
				continue;
			}
			putch(c,NO);
			putch(cc,NO);
			cc = comment(YES);
			if(getnl() == 1){
				if(cc == 0){
					OUT;
				}
				else {
					outs(0);
					putchar('\n');
					lbegin = 1;
					count = 0;
				}
				lastlook = 0;
			}
			continue;
		case '[':
			putch(c,NO);
			ct = 0;
			while((c = getch()) != ']' || ct > 0){
				if (c == EOF)
					goto done;
				putch(c,NO);
				if(c == '[')ct++;
				if(c == ']')ct--;
			}
			putch(c,NO);
			continue;
		case '#':
			putch(c,NO);
			while ((cc = getch()) != '\n'){
				if (cc == EOF)
					goto done;
				if (cc == '\\'){
					putch(cc,NO);
					cc = getch();
					if (cc == EOF)
						goto done;
				}
				putch(cc,NO);
			}
			putch(cc,NO);
			lbegin = 0;
			outs(clev->tabs);
			lbegin = 1;
			count = 0;
			continue;
		default:
			if (c == ','){
				opflag = 1;
				putch(c,YES);
				if (strict){
					if ((cc = getch()) != ' ')unget(cc);
					if(cc != '\n')putch(' ',YES);
				}
			}
			else if(isop(c))gotop(c);
			else {
				if(isalnum(c) && lastlook == 0)lastlook = p;
				putch(c,NO);
				if(keyflag != DATADEF)opflag = 0;
			}
		}
	}

done:
	if (ferror(input)) {
		fprintf(stderr, "cb: Read error on ");
		perror(filename);
	}
}

gotif(){
	outs(clev->tabs);
	if(++clev->iflev >= IFLEVEL-1){
		fprintf(stderr,"too many levels of if\n");
		clev->iflev = IFLEVEL-1;
	}
	clev->ifc[clev->iflev] = clev->tabs;
	clev->spdepth[clev->iflev] = clev->pdepth;
}

gotelse(){
	clev->tabs = clev->ifc[clev->iflev];
	clev->pdepth = clev->spdepth[clev->iflev];
	if(--(clev->iflev) < 0)clev->iflev = 0;
}

checkif(pt)
char *pt;
{
	register struct keyw *lptr;
	int cc;
	if((lptr=lookup(pt,lastplace+1))!= 0){
		if(lptr->type == IF){
			if(strict)putch(' ',YES);
			copy(lptr->name);
			*pt='\0';
			lastplace = pt;
			if(strict){
				putch(lptr->punc,NO);
				eatallsp();
			}
			clev->tabs = clev->ifc[clev->iflev];
			clev->pdepth = clev->spdepth[clev->iflev];
			keyflag = KEYWORD;
			return(1);
		}
	}
	return(0);
}
gotdo(){
	if(++dolevel >= DOLEVEL-1){
		fprintf(stderr,"too many levels of do\n");
		dolevel = DOLEVEL-1;
	}
	dotabs[dolevel] = clev->tabs;
	docurly[dolevel] = YES;
}
resetdo(){
	if(docurly[dolevel] == NO)
		clev->pdepth = dopdepth[dolevel];
	if(--dolevel < 0)dolevel = 0;
}
gottype(lptr)
struct keyw *lptr;
{
	char *pt;
	struct keyw *tlptr;
	int c;
	while(1){
		pt = getnext(1);
		if((tlptr=lookup(pt,lastplace+1))!=0){
			putch(' ',YES);
			copy(tlptr->name);
			*pt='\0';
			lastplace = pt;
			if(tlptr->type == STRUCT){
				putch(tlptr->punc,YES);
				gotstruct();
				break;
			}
			lptr=tlptr;
			continue;
		}
		else{
			putch(lptr->punc,NO);
			while((c=getch())== ' ' || c == '\t');
			unget(c);
			break;
		}
	}
}
gotstruct(){
	int c;
	int cc;
	char *pt;
	while((c=getch()) == ' ' || c == '\t')
		if(!strict)putch(c,NO);
	if(c == '{'){
		structlev++;
		unget(c);
		return;
	}
	if(isalpha(c)){
		putch(c,NO);
		while(isalnum(c=getch()))putch(c,NO);
	}
	unget(c);
	pt = getnext(1);
	if(*pt == '{')structlev++;
	if(strict){
		eatallsp();
		putch(' ',NO);
	}
}
gotop(c)
{
	register int cc;
	char optmp[OPLENGTH];
	char *op_ptr;
	struct op *s_op;
	char *a, *b;
	op_ptr = optmp;
	*op_ptr++ = c;
	while ((cc = getch()) != EOF && isop(cc))
		*op_ptr++ = cc;
	if(!strict)unget(cc);
	else if (cc != ' ')unget(cc);
	*op_ptr = '\0';
	s_op = op;
	b = optmp;
	while ((a = s_op->name) != 0){
		op_ptr = b;
		while ((*op_ptr == *a) && (*op_ptr != '\0')){
			a++;
			op_ptr++;
		}
		if (*a == '\0'){
			keep(s_op);
			opflag = s_op->setop;
			if (*op_ptr != '\0'){
				b = op_ptr;
				s_op = op;
				continue;
			}
			else break;
		}
		else s_op++;
	}
}
keep(o)
struct op *o;
{
	char	*s;
	int ok;
	ok = !strict;
	if (strict && ((o->blanks & ALWAYS)
	    || ((opflag == 0 && o->blanks & SOMETIMES) && clev->tabs != 0)))
		putspace(' ',YES);
	for(s=o->name; *s != '\0'; s++){
		putch(*s,ok);
		ok = NO;
	}
	if (strict && ((o->blanks & ALWAYS)
	    || ((opflag == 0 && o->blanks & SOMETIMES) && clev->tabs != 0))) putch(' ',YES);
}
getnl(){
	register int ch;
	char *savp;
	int gotcmt;
	gotcmt = 0;
	savp = p;
	while ((ch = getch()) == '\t' || ch == ' ')putch(ch,NO);
	if (ch == '/'){
		if ((ch = getch()) == '*'){
			putch('/',NO);
			putch('*',NO);
			comment(NO);
			ch = getch();
			gotcmt=1;
		}
		else if (ch == EOF)
			goto done;
		else {
			if(inswitch)*(++lastplace) = ch;
			else {
				inswitch = 1;
				*lastplace = ch;
			}
			unget('/');
			return(0);
		}
	}
	if(ch == '\n'){
		if(gotcmt == 0)p=savp;
		return(1);
	}

done:
	unget(ch);
	return(0);
}
ptabs(n){
	int	i;
	int num;
	if(n > maxtabs){
		if(!folded){
			printf("/* code folded from here */\n");
			folded = 1;
		}
		num = n-maxtabs;
	}
	else {
		num = n;
		if(folded){
			folded = 0;
			printf("/* unfolding */\n");
		}
	}
	for (i = 0; i < num; i++)putchar('\t');
}
outs(n){
	if (p > string){
		if (lbegin){
			ptabs(n);
			lbegin = 0;
			if (split == 1){
				split = 0;
				if (clev->tabs > 0)printf("    ");
			}
		}
		*p = '\0';
		printf("%s", string);
		lastlook = p = string;
	}
	else {
		if (lbegin != 0){
			lbegin = 0;
			split = 0;
		}
	}
}
putch(c,ok)
char c;
{
	register int cc;
	if(p < &string[LINE-1]){
		if(count+TABLENG*clev->tabs >= maxleng && ok && !folded){
			if(c != ' ')*p++ = c;
			OUT;
			split = 1;
			if((cc=getch()) != '\n')unget(cc);
		}
		else {
			*p++ = c;
			count++;
		}
	}
	else {
		outs(clev->tabs);
		*p++ = c;
		count = 0;
	}
}
struct keyw *
lookup(first, last)
char *first, *last;
{
	register struct keyw *ptr;
	register char	*cptr, *ckey, *k;

	if(first == last || first == 0)return(0);
	cptr = first;
	while (*cptr == ' ' || *cptr == '\t')cptr++;
	if(cptr >= last)return(0);
	ptr = key;
	while ((ckey = ptr->name) != 0){
		for (k = cptr; (*ckey == *k && *ckey != '\0'); k++, ckey++);
		if(*ckey=='\0' && (k==last|| (k<last && !isalnum(*k)))){
			opflag = 1;
			lastlook = 0;
			return(ptr);
		}
		ptr++;
	}
	return(0);
}
comment(ok)
{
	register int ch;
	int hitnl;

	hitnl = 0;
	while ((ch  = getch()) != EOF){
		putch(ch, NO);
		if (ch == '*'){
gotstar:
			if ((ch  = getch()) == '/'){
				putch(ch,NO);
				return(hitnl);
			}
			else if (ch == EOF)
				break;
			putch(ch,NO);
			if (ch == '*')goto gotstar;
		}
		if (ch == '\n'){
			if(ok && !hitnl){
				outs(clev->tabs);
			}
			else {
				outs(0);
			}
			lbegin = 1;
			count = 0;
			hitnl = 1;
		}
	}
	return(hitnl);
}
putspace(ch,ok)
char	ch;
{
	if(p == string)putch(ch,ok);
	else if (*(p - 1) != ch) putch(ch,ok);
}
getch(){
	register int c;
	if(inswitch){
		if(next != '\0'){
			c=next;
			next = '\0';
			return(c);
		}
		if(tptr <= lastplace){
			if(*tptr != '\0')return(*tptr++);
			else if(++tptr <= lastplace)return(*tptr++);
		}
		inswitch=0;
		lastplace = tptr = temp;
	}
	return(getc(input));
}
unget(c)
{
	if(inswitch){
		if(tptr != temp)
			*(--tptr) = c;
		else next = c;
	}
	else ungetc(c,input);
}
char *
getnext(must){
	int c;
	char *beg;
	int prect,nlct;
	prect = nlct = 0;
	if(tptr > lastplace){
		tptr = lastplace = temp;
		err = 0;
		inswitch = 0;
	}
	tp = beg = lastplace;
	if(inswitch && tptr <= lastplace)
		if (isalnum(*lastplace)||ispunct(*lastplace)||isop(*lastplace))return(lastplace);
space:
	while(isspace(c=getc(input)))puttmp(c,1);
	if (c == EOF)
		goto done;
	beg = tp;
	puttmp(c,1);
	if(c == '/'){
		if((c=getc(input)) == '*'){
			puttmp(c,1);
cont:
			while((c=getc(input)) != '*'){
				if (c == EOF)
					goto done;
				puttmp(c,0);
				if(must == 0 && c == '\n')
					if(nlct++ > 2)goto done;
			}
			puttmp(c,1);
	star:
			c=getc(input);
			puttmp(c,1);
			if(c == '/'){
				beg = tp;
				c = getc(input);
				if (c == EOF)
					goto done;
				puttmp(c,1);
			}
			else if(c == '*')goto star;
			else goto cont;
		}
		else if (c == EOF)
			goto done;
		else {
			puttmp(c,1);
			goto done;
		}
	}
	if(isspace(c))goto space;
	if(c == '#' && tp > temp+1 && *(tp-2) == '\n'){
		if(prect++ > 2)goto done;
		while((c=getc(input)) != '\n') {
			if (c == EOF)
				goto done;
			puttmp(c,1);
			if(c == '\\') {
				c = getc(input);
				if (c == EOF)
					goto done;
				(void) puttmp(c,1);
			}
		}
		puttmp(c, 1);
		goto space;
	}
	if(isalnum(c)){
		while(isalnum(c = getc(input)))puttmp(c,1);
		ungetc(c,input);
	}
done:
	puttmp('\0',1);
	lastplace = tp-1;
	inswitch = 1;
	return(beg);
}
copy(s)
char *s;
{
	while(*s != '\0')putch(*s++,NO);
}
clearif(cl)
struct indent *cl;
{
	int i;
	for(i=0;i<IFLEVEL-1;i++)cl->ifc[i] = 0;
}
puttmp(c,keep)
char c;
{
	if(tp < &temp[TEMP-120])
		*tp++ = c;
	else {
		if(keep){
			if(tp >= &temp[TEMP-1]){
				fprintf(stderr,"can't look past huge comment - quiting\n");
				exit(1);
			}
			*tp++ = c;
		}
		else if(err == 0){
			err++;
			fprintf(stderr,"truncating long comment\n");
		}
	}
}
