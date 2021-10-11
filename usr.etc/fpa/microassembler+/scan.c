#ifndef lint
static  char sccsid[] = "@(#)scan.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *	Microassembler scanner for FPA
 *		scan.c	1.1	30 May 1985   
 */

#include "micro.h"
#include <stdio.h>
#include <ctype.h>


#define scansp(c)  while(isspace(c = *curpos)) curpos++
#define scanb(c) scansp(c); if (c == ',' ){c=nextc(); scansp(c);}
#define nextc()     *++curpos
#define peekc()     *curpos
#define MAXSYMBOL 256
#define MAXLINE 1024

int curlineno;
char *curfilename;
char *curpos;
char *curline;
char inputline[MAXLINE];

/* The following 2 lines added by PWC to count lines of ucode. */
extern NODE n[];
extern NODE *curnode;
extern short curaddr;
extern short maxaddr;
extern CODE code[];
extern CODE *curword;

extern Boolean aseq();
extern Boolean ati8847();
extern Boolean adata();
extern Boolean arecreg();
extern Boolean aram();
extern Boolean aptr();
extern char   *strcpy();

Boolean
newline(){
    /* glom a new input line.
     * clobber comments. do
     * continuation-line collection, as well.
     */
    register char *  maxpos = inputline + sizeof inputline - 1;
    register char *cp;
    char *end;
    register c;
    int len;

    cp = inputline;
restart:
    curlineno++;
    do {
	c = getchar();
	if (c == '&') {
		while ( getchar() != '\n' );
		goto restart;
	}
	*cp++ = c;
    } while ( c != '\n' && c != EOF && cp < maxpos );
    if ( c == EOF ) return False;
    end = cp;
    *cp = '\0';
    if (cp == maxpos){
	error("Sorry, line(s) too long");
	goto process;
    }
process:
    curpos = inputline;
    len = end-inputline;
    curline = (char*)malloc(len+2);
    if (curline == 0) {
	error("unable to get sufficient storage\n");
	return False;
    }
    strcpy( curline, inputline );
    return True;
}

char *
scansym( )
{
    static char stuff[MAXSYMBOL];
    register char *cp;
    register char c;
    Boolean ok = True;

    cp = stuff;
    curpos -= 1; /*back up to get a running start*/
    while( ok ){
	while (isalnum(*cp++ = c = nextc()));
	switch(c){
	case '.':
	case '_':
	case '\\':
	    continue;
	default:
	    ok = False;
	}
    }
    *--cp = '\0';
    return( stuff );
}


int
getnum( )
{
    register char c = *curpos;
    register int val, base;
    int sign;

    sign = 1;
    if (c == '-') {
	sign = -1;
	c = nextc();
    } else if (c == '+') {
	c = nextc();
    }
    val = c - '0';
    base = (val==0)? 010 : 10;
    c = nextc();
    if ( val == 0 && (c == 'x' || c == 'X') ){
	base = 0x10;
	c = nextc();
    }
    while(isxdigit(c)){
	val *= base;
	if( isdigit(c) )
	    val += c -'0';
	else if (islower(c))
	    val += c -'a' + 10;
	else
	    val += c -'A' + 10;
	c = nextc();
    }
    if (sign == -1) {
	val = -val;
    }
    return( val );
}

Boolean
seqpart (name)
	char    *name;
{
	register char      c;
	register RESERVED *rp;
	register char     *cp;
	int      brnch = 001;		/* default to next */
	int	 condition = 0;		/* default to never */
	int      state = 0;		/* default */
	int      latch = 1;
	int      offset;
	SYMTYPE  which = NEITHER;
	int	 num   = 0;
	static   char buff[MAXSYMBOL];
	char	*labl = buff;

#define CHEK    if(c==';') {c=nextc(); scanb(c); return ASEQ;} \
		rp = resw_lookup( cp = scansym( ) ); \
		if ( rp == 0 ) goto wbotch;
#define ASEQ 	aseq( brnch, condition, which, num, labl, state, latch )

	if ( name == 0 && peekc() == ';') {   /* no seq field */
		c = nextc();
		scanb(c);
		return ASEQ;
	}

start:
	rp = resw_lookup( cp = name );
	if ( rp == 0 ) goto wbotch;
	if ( rp->type == Branch1) {
		brnch = rp->value1;
		scanb(c);
		if ( rp->value2 == 2 ) {
		    curnode->jumpop = True;
		    if ( isdigit(c) || c == '.' ) goto address;
		    if ( isalpha(c) ) {
			rp = resw_lookup(cp = scansym());
			if ( rp == 0 ) {
			    which = ALPHA;
			    strcpy (labl, cp);
			    scanb(c);
			    CHEK;
			}
		    } else goto botch;
		} else {
		    CHEK
		}
	}
	if ( rp->type == Branch2 ) {
		condition = rp->value1;
	address:
		curnode->jumpop = True;
		scanb(c);
		if (isdigit(c)) {
            		which = NUMBER;
            		num = getnum( );
            		scanb(c);
        	} else if (isalpha(c)) {
            		which = ALPHA;
			strcpy (labl, scansym( ));
            		scanb(c);
        	} else if (c == '.') {
            		which = NUMBER;
            		offset = 0;
            		c = nextc( );
            		scanb(c);
            		if (c == '+') {
                		c = nextc();
                		scanb(c);
                		offset = getnum();
                		scanb(c);
            		} else if (c == '-') {
				c = nextc();
                		scanb(c);
                		offset = -getnum();
                		scanb(c);
			} 
            		num = curaddr - 1 + offset;
        	} else goto botch;
		CHEK
	}
	if ( rp->type == State ) {
		state = rp->value1;
		scanb(c);
		CHEK
	}
	if ( rp->type == Latch && rp->value2 == 1 ) {
		latch = rp->value1;
		scanb(c);
		CHEK
		if ( rp->type == Latch && rp->value2 == 2 ) {
			latch += rp->value1;
			scanb(c);
			CHEK
		}
	}
	if ( rp->type == Latch && rp->value2 == 3 ) {
		latch += rp->value1;
		scanb(c);
		if ( c == ';' ) {
			c = nextc();
			scanb(c);
			return ASEQ;
		}
	}
wbotch :
	error ("%s found in seq_state field", cp );
	return False;
botch:
	error ("seq_state field syntax error, character %c is not expected", c);
	return False;

#undef ASEQ
#undef CHEK
}


Boolean
tipart()
{
	register char c;
	register RESERVED *rp;
	char	*cp;
	int	func  = 0x020,		/* default to spass */
		src = 0xff,		/* default to RA input register */
		regctl = 0,
		csrc = 0,
		output = 0x2,
		conf = 1,
		halt = 0;

#define CHEK    if(c==';') {c=nextc(); scanb(c); return ATI8847;} \
		rp = resw_lookup( cp = scansym( ) ); \
		if ( rp == 0 ) goto wbotch;
#define ATI8847 ati8847( func, src, regctl, csrc, output, halt, conf )

	scanb(c);
	if ( c == ';' ) {    /* no TIinst field */
		c = nextc();
		scanb(c);
		return ATI8847;
	}
	rp = resw_lookup(cp=scansym( ));
	if ( rp == 0 ) goto wbotch;
	if ( rp->type == Function ) {
		func = rp->value1;
		scanb( c );
		CHEK;
		if ( rp->type == Source && rp->value2 == 1 ) {
			src = ( src & ~0x03 ) | rp->value1;
			scanb( c );
			CHEK;
		}
		if ( rp->type == Source && rp->value2 == 2 ) {
			src = ( src & ~0x0c ) | rp->value1;
			scanb( c );
			CHEK;
		}
		if ( rp->type == Source && rp->value2 == 3 ) {
			src = ( src & ~0x30 ) | rp->value1;
			scanb( c );
			CHEK;
		}
		if ( rp->type == Source && rp->value2 == 4 ) {
			src = ( src & ~0xc0 ) | rp->value1;
			scanb( c );
			CHEK;
		}
	}
	if ( rp->type == Regctl && rp->value2 == 1 ) {
		regctl = rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Regctl && rp->value2 == 2 ) {
		regctl += rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Csrc && rp->value2 == 1 ) {
		csrc = rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Csrc && rp->value2 == 2 ) {
		csrc += rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Output && rp->value2 == 1 ) {
		output = rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Output && rp->value2 == 2 ) {
		output += rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Config ) {
		conf = rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Halt ) {
		halt = rp->value1;
		scanb( c );
		if (c == ';') {
			c = nextc();
			scanb(c);
			return ATI8847;
		}
	}

wbotch :
	error ("%s found in TIinst field", cp );
	return False;
botch:
	error ("TIinst syntax error, character %c is not expected", c);
	return False;

#undef CHEK
#undef ATI8847

}

Boolean
ramoppart()
{
	register char c;
	register RESERVED *rp;
	char    *cp;
	int      dctrl = 017;	/* default to idle state */

#define ADATA   adata(dctrl)
#define CHEK    if(c==';') {c=nextc(); scanb(c); return ADATA;} \
		rp = resw_lookup( cp = scansym( ) ); \
		if ( rp == 0 ) goto wbotch;

	scanb(c);
	if ( c == ';' ) {    /* no datamuxing field */
		c = nextc();
		scanb(c);
		return ADATA;
	}
	rp = resw_lookup(cp=scansym( ));
	if ( rp == 0 ) goto wbotch;
	if (rp->type == Regram ) {
		dctrl = rp->value1;
		scanb(c);
		if (c == ';') {
			c = nextc();
			scanb(c);
			return ADATA;
		}
	}
	error ("ramop syntax error, character %c is not expected", c);
	return False;
wbotch :
	error ("%s found in ramop field", cp );
	return False;

#undef ADATA
#undef CHEK

}

Boolean
ramctlpart()
{
	register char c;
	register RESERVED *rp;
	char    *cp;
	int      ramcs = 0,
		 ptr = 0;

#define CHEK    if(c==';') {c=nextc(); scanb(c); return ARAM;} \
        	rp = resw_lookup(cp=scansym( )); \
        	if ( rp == 0 ) goto wbotch;
#define ARAM	aram ( ramcs, ptr )
		
	scanb(c);
        if ( c == ';' ) {    /* no ramctrl field */
                c = nextc();
                scanb(c);
                return ARAM;
        }
        rp = resw_lookup(cp=scansym( ));
        if ( rp == 0 ) goto wbotch;
        if (rp->type == Ramcs ) {
                ramcs = rp->value1; 
                scanb(c);
                CHEK
        }
        if (rp->type == Pointer ) {
		if ( rp->value2 == 2 ) {
			error("pointer %s is not allowed here", cp);
			return False;
		}
		ptr = rp->value1;
		scanb(c);
		if (c == ';') {
			c = nextc();
			scanb(c);
			return ARAM;
		}
        }
        error ("ram control field syntax error, character %c is not expected", c);
        return False;
wbotch :
        error ("%s found in ram control field", cp );
        return False;

#undef CHEK
#undef ARAM
}


Boolean
ptrpart ( )
{
	register char 	  c;
	register RESERVED*rp;
	register char 	 *cp;
	int	 ptr = 0;
	int	 act = 0;
	SYMTYPE  which = NEITHER;
	int	 num;
	char	*sym;

#define APTR 	aptr( ptr, act, which, num, sym) 

	scanb(c);
	if ( c== '|' || c == '\0' ) {         /* no ptr_action field */
		if ( c == '|' ) {
			c = nextc();
			scanb(c);
		}
		return APTR;
	}
	rp = resw_lookup(cp=scansym( ));
	if ( rp == 0 ) goto wbotch;
	if ( rp->type == Pointer ) {
		if ( rp->value2 == 1 ) {
			error("pointer %s is not allowed here", cp);
			return False;
		}
		ptr = rp->value1;
		scanb(c);
		switch (c) {
		case '=' :             /* hold */
			act = 0;
			break;
		case '!':
			if ( ptr == 1 || ptr == 2 || ptr == 3 || ptr == 4) {
				error("action is not allowed for the pointer");
				return False;
			}
			act = 1;       /* load */
			c = nextc();
			scanb(c);
			if ( c == '|' || c == '\0' ) return APTR;
			if ( curnode->jumpop == True ) {
				error("overlap address field with load value");
				return False;
			}
			if (isdigit(c)) {
				which = NUMBER;
				num = getnum( );
				scanb(c);
				if ( c == '+') {
					c = nextc();
					scanb(c);
					if (! isdigit(c)) goto wbotch;
					num += getnum();
					curpos--;
				} else {
					curpos--;
				}
			} else  if(isalpha(c)) {
				which = ALPHA;
				sym = scansym( );
			} else goto wbotch;
			break;
		case '+':
			if ( ptr == 6 || ptr == 7 ) {
				error("action is not allowed for the pointer");
				return False;
			}
			act = 3;
			break;
		case '-':
			if ( ptr == 7 ) {
				error("action is not allowed for the pointer");
				return False;
			}
			act = 2;
			break;
		default :
        		error ("ptr control field syntax error, character %c is not expected", c);
        		return False;
		}
		c = nextc();
		scanb(c);
		if ( c == '|' || c == '\0' ) return APTR;
	}

wbotch :
        error ("%s found in ptr control field", cp );
        return False;

#undef APTR

}

label(sp)
    SYMBOL *sp;
{
    if (sp->defined){
	error("label %s already defined", sp->name );
	return;
    }
    sp->defined = True;
    sp->node = curnode+1;
    DEBUG("Label: defining %s\n", sp->name);
}

Boolean
cpp(){
    /* the scanner just saw a '#' -- treat as cpp leaving */
    register char c ;
    char *sp;
    int leng;
    int cln;

    nextc();
    scanb(c);
    cln = curlineno;
    curlineno = getnum( ) - 1;
    scanb(c);
    if (c != '"') {
	curlineno = cln;
	error("#-line not of form output by C preprocessor");
	return False;
    }
    /* just saw the " */
    c = nextc(); /* skip it */
    sp =  curpos;
    while( (c=nextc()) != '"' );
    *curpos = '\0';
    leng = strlen( sp );
    curfilename = (char *)malloc( leng+1 );
    if (curfilename == 0) {
	error("unable to get sufficient storage\n");
	return False;
    }
    strcpy(  curfilename, sp );
    DEBUG("# %d \"%s\"\n", curlineno+1, curfilename);
    return True;
}

    
scanprog()
{
    /* 
      for each line in the program text:
	if it is blank, continue.
	if it begins with a '#', its a cpp dropping
	look at the first symbol on the line.
	    if it is followed by a :, its a label,
		so define the label and scan out the next symbol
	assemble the seq_state part
	assemble the Weitek instruction part
	assemble the datamuxing part
	assemble the recover reg ctrl part
	assemble the ram control part
	assemble the pointer actoin part

    */
    char c;
    char *name;
    SYMBOL *sp;
    Boolean stopsw = False;
    Boolean bkptsw = False;
    int debugsw = 0;
    register RESERVED *rp;
    Boolean checkeol();

    while (newline()){
	if (*curpos == '#'){
	    cpp();
	    continue;
	}
restart:
	scanb(c);
	if( c == '\0') {
	    continue; /* blank line */
	}
	if (c == ';') {
	    name = 0;
	    anext();
	    goto doseq;
	}
	if (!isalnum(c)) {
	    if (c != '|') {
		error("character %c unexpected", c);
	    }
	    anext();
	    curnode->org_pseudo = True;
	    continue;
	}
	name = scansym( );
	scanb(c);
	if(c== ':') {
	    /* what we have here is a label */
	    sp = lookup( name );
	    if (sp == 0)
		sp = enter( name );
	    label( sp );
	    nextc();
	    scanb(c);
	    (curnode+1)->addr = curaddr;
	    if (c == '\0') {
		anext();
		curnode->org_pseudo = True;
	    }
	    goto restart;
	}
	anext();
	rp = resw_lookup( name );
	if (rp != 0 && rp->type == Pseudo) {
	    /* it's a pseudo-op */
	    switch (rp->value1) {
	    case 1:	/* org */
	        scanb(c);
	        if (!isdigit(c)) {
		    error("expected digit; found %c", c);
		    continue;
		}
		curnode->org_pseudo = True;
		curaddr = getnum();
		break;
	    case 2:     /* sccsid */
                scanb(c);
		acode();
                asccs(curpos);
                curnode->addr = curaddr++;
		if ( curaddr > maxaddr ) {
			maxaddr = curaddr;
		}
                break;
	    case 3:     /* routine */
		doramtable( );
		break;
            }
            continue;
        } 
doseq:
	acode();
	curnode->addr = curaddr++;
	if ( curaddr > maxaddr ) {
		maxaddr = curaddr;
	}
	if (seqpart(name) == False) continue; 
	if (tipart( ) == False) continue; 
	if (ramoppart( ) == False) continue;
	if (ramctlpart( ) == False) continue;
	ptrpart( );
    }
}
