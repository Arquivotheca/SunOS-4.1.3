/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)spellprog.c 1.1 92/07/30 SMI"; /* from S5R3 1.2 */
#endif

#include <stdio.h>
#include <ctype.h>
#define Tolower(c) (isupper(c)?tolower(c):c)
#define DLEV 2
#define MAXWORDLEN 100

char	*strcat();
int	strip();
int	subst();
char	*skipv();
int	an();
int	s();
int	es();
int	ily();
int	CCe();
int	VCe();
int	bility();
int	tion();
int	ize();
int	y_to_e();
int	i_to_y();
int	nop();

struct suftab {
	char *suf;
	int (*p1)();
	int n1;
	char *d1;
	char *a1;
	int (*p2)();
	int n2;
	char *d2;
	char *a2;
} suftab[] = {
	{"ssen",ily,4,"-y+iness","+ness" },
	{"ssel",ily,4,"-y+i+less","+less" },
	{"se",s,1,"","+s",		es,2,"-y+ies","+es" },
	{"s'",s,2,"","+'s"},
	{"s",s,1,"","+s"},
	{"ecn",subst,1,"-t+ce",""},
	{"ycn",subst,1,"-t+cy",""},
	{"ytilb",nop,0,"",""},
	{"ytilib",bility,5,"-le+ility",""},
	{"elbaif",i_to_y,4,"-y+iable",""},
	{"elba",CCe,4,"-e+able","+able"},
	{"yti",CCe,3,"-e+ity","+ity"},
	{"ylb",y_to_e,1,"-e+y",""},
	{"yl",ily,2,"-y+ily","+ly"},
	{"laci",strip,2,"","+al"},
	{"latnem",strip,2,"","+al"},
	{"lanoi",strip,2,"","+al"},
	{"tnem",strip,4,"","+ment"},
	{"gni",CCe,3,"-e+ing","+ing"},
	{"reta",nop,0,"",""},
	{"retc",nop,0,"",""},
	{"re",strip,1,"","+r",		i_to_y,2,"-y+ier","+er"},
	{"de",strip,1,"","+d",		i_to_y,2,"-y+ied","+ed"},
	{"citsi",strip,2,"","+ic"},
	{"citi",ize,1,"-ic+e",""},
	{"cihparg",i_to_y,1,"-y+ic",""},
	{"tse",strip,2,"","+st",	i_to_y,3,"-y+iest","+est"},
	{"cirtem",i_to_y,1,"-y+ic",""},
	{"yrtem",subst,0,"-er+ry",""},
	{"cigol",i_to_y,1,"-y+ic",""},
	{"tsigol",i_to_y,2,"-y+ist",""},
	{"tsi",CCe,3,"-e+ist","+ist"},
	{"msi",CCe,3,"-e+ism","+ist"},
	{"noitacifi",i_to_y,6,"-y+ication",""},
	{"noitazi",ize,4,"-e+ation",""},
	{"rota",tion,2,"-e+or",""},
	{"rotc",tion,2,"","+or"},
	{"noit",tion,3,"-e+ion","+ion"},
	{"naino",an,3,"","+ian"},
	{"na",an,1,"","+n"},
	{"evi",subst,0,"-ion+ive",""},
	{"ezi",CCe,3,"-e+ize","+ize"},
	{"pihs",strip,4,"","+ship"},
	{"dooh",ily,4,"-y+ihood","+hood"},
	{"luf",ily,3,"-y+iful","+ful"},
	{"ekil",strip,4,"","+like"},
	0
};

char *preftab[] = {
	"anti",
	"auto",
	"bio",
	"counter",
	"dis",
	"electro",
	"en",
	"fore",
	"geo",
	"hyper",
	"intra",
	"inter",
	"iso",
	"kilo",
	"magneto",
	"meta",
	"micro",
	"mid",
	"milli",
	"mis",
	"mono",
	"multi",
	"non",
	"out",
	"over",
	"photo",
	"poly",
	"pre",
	"pseudo",
	"psycho",
	"re",
	"semi",
	"stereo",
	"sub",
	"super",
	"tele",
	"thermo",
	"ultra",
	"under",	/*must precede un*/
	"un",
	0
};

int vflag;
int xflag;
char word[MAXWORDLEN];
char original[MAXWORDLEN];
char *deriv[40];
char affix[40];
char errmsg[] = "spell: cannot initialize hash table\n";
FILE *file, *found;
/*	deriv is stack of pointers to notes like +micro +ed
 *	affix is concatenated string of notes
 *	the buffer size 141 stems from the sizes of original and affix.
*/

/*
 *	in an attempt to defray future maintenance misunderstandings, here is an attempt to
 *	describe the input/output expectations of the spell program.
 *
 *	spellprog is intended to be called from the shell file spell.
 *	because of this, there is little error checking (this is historical, not
 *	necessarily advisable).
 *
 *	spellprog hashed-list pass options
 *
 *	the hashed-list is a list of the form made by spellin.
 *	there are 2 types of hashed lists:
 *		1. a stop list: this specifies words that by the rules embodied in
 *		   spellprog would be recognized as correct, BUT are really errors.
 *		2. a dictionary of correctly spelled words.
 *	the pass number determines how the words found in the specified hashed-list
 *	are treated. If the pass number is 1, the hashed-list is treated as the stop-list,
 *	otherwise, it is treated as the regular dictionary list. in this case, the
 *	value of "pass" is a filename. Found words are written to this file.
 *	In the normal case, the filename = /dev/null. However, if the v option is
 *	specified, the derivations are written to this file.
 *	the spellprog looks up words in the hashed-list; if a word is found, it is
 *	printed to the stdout. if the hashed-list was the stop-list, the words found
 *	are presumed to be misspellings. in this case,
 *	a control character is printed ( a "-" is appended to the word.
 *	a hyphen will never occur naturally in the input list because deroff
 *	is used in the shell file before calling spellprog.)
 *	if the regualar spelling list was used (hlista or hlistb), the words are correct,
 *	and may be ditched. (unless the -v option was used - see the manual page).

 *	spellprog should be called twice : first with the stop-list, to flag all
 *	a priori incorrectly spelled words; second with the dictionary.
 *
 *	spellprog hstop 1 |\
 *	spellprog hlista /dev/null 
 *
 *	for a complete scenario, see the shell file: spell.
 *
 */
main(argc,argv)
char **argv;
{
	register char *ep, *cp;
	register char *dp;
	int fold;
	int j;
	int pass;
	int overflo;
	if(!prime(argc,argv)) {
		fwrite(errmsg, sizeof(*errmsg), sizeof(errmsg), stderr);
		exit(1);
	}
/*
 *	if pass is not 1, it is assumed to be a filename.
 *	found words are written to this file.
 */
	pass = argv[2][0] ;
	if ( pass != '1' )
		found = fopen(argv[2],"w");
	for(argc-=3,argv+=3; argc>0 && argv[0][0]=='-'; argc--,argv++)
		switch(argv[0][1]) {
		case 'b':
			ise();
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		}
	for(;;) {
		affix[0] = 0;
		file = stdout;
		overflo = 0;
		for(ep=word;(*ep=j=getchar())!='\n';ep++)
			if(j == EOF)
				exit(0);
			else if((ep-word) == MAXWORDLEN-1) {
				overflo++;
				break;
			}

			if (overflo) {
				*ep = 0;
				fprintf(stderr, "%s: word too long (ignored) %s ...\n",
					"spellprog", word);
				while ((*ep=getchar()) != '\n')
					if (*ep == EOF)
						exit(0);
				continue;	/* go to next word */
			}
/*
 *	here is the hyphen processing. these words were found in the stop
 *	list. however, if they exist as is, (no derivations tried) in the
 *	dictionary, let them through as correct.
 *
 */
		if(ep[-1]=='-') {
			*--ep = 0;
			if(!tryword(word,ep,0))
				fprintf(file, "%s\n", word);
			continue;
		}
		for(cp=word,dp=original; cp<ep; )
			*dp++ = *cp++;
		*dp = 0;
		fold = 0;
		for(cp=word;cp<ep;cp++)
			if(islower(*cp))
				goto lcase;
		if(trypref(ep,".",0))
			goto foundit;
		++fold;
		for(cp=original+1,dp=word+1;dp<ep;dp++,cp++)
			*dp = Tolower(*cp);
lcase:
		if(trypref(ep,".",0)||trysuff(ep,0))
			goto foundit;
		if(isupper(word[0])) {
			for(cp=original,dp=word; *dp = *cp++; dp++)
				if (fold) *dp = Tolower(*dp);
			word[0] = Tolower(word[0]);
			goto lcase;
		}
		fprintf(file, "%s\n", original);
		continue;

foundit:
		if(pass=='1')
			fprintf(file, "%s-\n", original);
		else if(affix[0]!=0 && affix[0]!='.') {
			file = found;
			fprintf(file, "%s\t%s\n", affix, original);
		}
	}
}

/*	strip exactly one suffix and do
 *	indicated routine(s), which may recursively
 *	strip suffixes
*/
trysuff(ep,lev)
char *ep;
{
	register struct suftab *t;
	register char *cp, *sp;
	lev += DLEV;
	deriv[lev] = deriv[lev-1] = 0;
	for(t= &suftab[0];sp=t->suf;t++) {
		cp = ep;
		while(*sp)
			if(*--cp!=*sp++)
				goto next;
		for(sp=cp; --sp>=word&&!vowel(*sp); ) ;
		if(sp<word)
			return(0);
		if((*t->p1)(ep-t->n1,t->d1,t->a1,lev+1))
			return(1);
		if(t->p2!=0) {
			deriv[lev] = deriv[lev+1] = 0;
			return((*t->p2)(ep-t->n2,t->d2,t->a2,lev));
		}
		return(0);
next:		;
	}
	return(0);
}

nop()
{
	return(0);
}

strip(ep,d,a,lev)
char *ep,*d,*a;
{
	return(trypref(ep,a,lev)||trysuff(ep,lev));
}

s(ep,d,a,lev)
char *ep,*d,*a;
{
	if(lev>DLEV+1)
		return(0);
	if(*ep=='s'&&ep[-1]=='s')
		return(0);
	return(strip(ep,d,a,lev));
}

an(ep,d,a,lev)
char *ep,*d,*a;
{
	if(!isupper(*word))	/*must be proper name*/
		return(0);
	return(trypref(ep,a,lev));
}

ize(ep,d,a,lev)
char *ep,*d,*a;
{
	ep[-1] = 'e';
	return(strip(ep,"",d,lev));
}

y_to_e(ep,d,a,lev)
char *ep,*d,*a;
{
	*ep++ = 'e';
	return(strip(ep,"",d,lev));
}

ily(ep,d,a,lev)
char *ep,*d,*a;
{
	if(ep[-1]=='i')
		return(i_to_y(ep,d,a,lev));
	else
		return(strip(ep,d,a,lev));
}

bility(ep,d,a,lev)
char *ep,*d,*a;
{
	*ep++ = 'l';
	return(y_to_e(ep,d,a,lev));
}

i_to_y(ep,d,a,lev)
char *ep,*d,*a;
{
	if(ep[-1]=='i') {
		ep[-1] = 'y';
		a = d;
	}
	return(strip(ep,"",a,lev));
}

es(ep,d,a,lev)
char *ep,*d,*a;
{
	if(lev>DLEV)
		return(0);
	switch(ep[-1]) {
	default:
		return(0);
	case 'i':
		return(i_to_y(ep,d,a,lev));
	case 's':
	case 'h':
	case 'z':
	case 'x':
		return(strip(ep,d,a,lev));
	}
}

subst(ep,d,a,lev)
char *ep, *d, *a;
{
	char *u,*t;
	if(skipv(skipv(ep-1))<word)
		return(0);
	for(t=d;*t!='+';t++)
		continue;
	for(u=ep;*--t!='-'; )
		*--u = *t;
	return(strip(ep,"",d,lev));
}


tion(ep,d,a,lev)
char *ep,*d,*a;
{
	switch(ep[-2]) {
	case 'c':
	case 'r':
		return(trypref(ep,a,lev));
	case 'a':
		return(y_to_e(ep,d,a,lev));
	}
	return(0);
}

/*	possible consonant-consonant-e ending*/
CCe(ep,d,a,lev)
char *ep,*d,*a;
{
	switch(ep[-1]) {
	case 'l':
		if(vowel(ep[-2]))
			break;
		switch(ep[-2]) {
		case 'l':
		case 'r':
		case 'w':
			break;
		default:
			return(y_to_e(ep,d,a,lev));
		}
		break;
	case 's':
		if(ep[-2]=='s')
			break;
	case 'c':
	case 'g':
		if(*ep=='a')
			return(0);
	case 'v':
	case 'z':
		if(vowel(ep[-2]))
			break;
	case 'u':
		if(y_to_e(ep,d,a,lev))
			return(1);
		if(!(ep[-2]=='n'&&ep[-1]=='g'))
			return(0);
	}
	return(VCe(ep,d,a,lev));
}

/*	possible consonant-vowel-consonant-e ending*/
VCe(ep,d,a,lev)
char *ep,*d,*a;
{
	char c;
	c = ep[-1];
	if(c=='e')
		return(0);
	if(!vowel(c) && vowel(ep[-2])) {
		c = *ep;
		*ep++ = 'e';
		if(trypref(ep,d,lev)||trysuff(ep,lev))
			return(1);
		ep--;
		*ep = c;
	}
	return(strip(ep,d,a,lev));
}

char *lookuppref(wp,ep)
char **wp;
char *ep;
{
	register char **sp;
	register char *bp,*cp;
	for(sp=preftab;*sp;sp++) {
		bp = *wp;
		for(cp= *sp;*cp;cp++,bp++)
			if(Tolower(*bp)!=*cp)
				goto next;
		for(cp=bp;cp<ep;cp++) 
			if(vowel(*cp)) {
				*wp = bp;
				return(*sp);
			}
next:	;
	}
	return(0);
}

/*	while word is not in dictionary try stripping
 *	prefixes. Fail if no more prefixes.
*/
trypref(ep,a,lev)
char *ep,*a;
{
	register char *cp;
	char *bp;
	register char *pp;
	int val = 0;
	char space[20];
	deriv[lev] = a;
	if(tryword(word,ep,lev))
		return(1);
	bp = word;
	pp = space;
	deriv[lev+1] = pp;
	while(cp=lookuppref(&bp,ep)) {
		*pp++ = '+';
		while(*pp = *cp++)
			pp++;
		if(tryword(bp,ep,lev+1)) {
			val = 1;
			break;
		}
	}
	deriv[lev+1] = deriv[lev+2] = 0;
	return(val);
}

tryword(bp,ep,lev)
char *bp,*ep;
{
	register i, j;
	char duple[3];
	if(ep-bp<=1)
		return(0);
	if(vowel(*ep)) {
		if(monosyl(bp,ep))
			return(0);
	}
	i = dict(bp,ep);
	if(i==0&&vowel(*ep)&&ep[-1]==ep[-2]&&monosyl(bp,ep-1)) {
		ep--;
		deriv[++lev] = duple;
		duple[0] = '+';
		duple[1] = *ep;
		duple[2] = 0;
		i = dict(bp,ep);
	}
	if(vflag==0||i==0)
		return(i);
	/*	when derivations are wanted, collect them
	 *	for printing
	*/
	j = lev;
	do {
		if(deriv[j])
			strcat(affix,deriv[j]);
	} while(--j>0);
	return(i);
}


monosyl(bp,ep)
char *bp, *ep;
{
	if(ep<bp+2)
		return(0);
	if(vowel(*--ep)||!vowel(*--ep)
		||ep[1]=='x'||ep[1]=='w')
		return(0);
	while(--ep>=bp)
		if(vowel(*ep))
			return(0);
	return(1);
}

char *
skipv(s)
char *s;
{
	if(s>=word&&vowel(*s))
		s--;
	while(s>=word&&!vowel(*s))
		s--;
	return(s);
}

vowel(c)
{
	switch(Tolower(c)) {
	case 'a':
	case 'e':
	case 'i':
	case 'o':
	case 'u':
	case 'y':
		return(1);
	}
	return(0);
}

/* crummy way to Britishise */
ise()
{
	register struct suftab *p;
	for(p = suftab;p->suf;p++) {
		ztos(p->suf);
		ztos(p->d1);
		ztos(p->a1);
	}
}
ztos(s)
char *s;
{
	for(;*s;s++)
		if(*s=='z')
			*s = 's';
}

dict(bp,ep)
char *bp, *ep;
{
	register temp, result;
	if(xflag)
		fprintf(stderr, "=%.*s\n", ep-bp, bp);
	temp = *ep;
	*ep = 0;
	result = hashlook(bp);
	*ep = temp;
	return(result);
}
