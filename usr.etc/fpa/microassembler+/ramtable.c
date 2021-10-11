#ifndef lint
static  char sccsid[] = "@(#)ramtable.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * This module contains the functions that parse the routine pseudo op
 * for the FPA-3X Microcode Assembler.
 */

#include <string.h>
#include <ctype.h>
#include "micro.h"

/* defines used to construct the call instruction with aseq() */
#define RTN_BRANCH	0x2
#define RTN_COND	0x0
#define RTN_STATE	0x0
#define RTN_LATCH	0x1

#define MAXMAP	4095		/* highest location of a map ram entry */

/* macros used to scan a line of code */
#define scansp(c)  while(isspace(c = *curpos)) curpos++
#define scanb(c) scansp(c); if (c == ',' ){c=nextc(); scansp(c);}
#define nextc()     *++curpos
#define peekc()     *curpos

/* data used to assemble the call instruction */
int func, src, regctl, csrc, outenable, conf, halt,
	dctrl, ramcs, ramptr, ptr, act, num;
SYMTYPE  which;
char	*sym;

/* forward declarations */
Boolean rtn_tipart(), rtn_ramoppart(), rtn_ramctlpart(), rtn_ptrpart();

/* external functions */
extern char *scansym();

/* external data */
char *curpos;
extern short curaddr;
extern NODE *curnode;

doramtable()
{
	register char 	   c;
	register dontcare = 0;
	register mask = 0;
	RESERVED *rp;
	SYMBOL	 *sp;
	char	 *cp;
	char	 name[20];
	char	 addr[13];
	int      i;
	Boolean  first;
	int	 load;
	int	 saveaddr;

	scanb(c);
	cp = scansym( );         		   /* routine name */
	cp = strcpy (name, cp);
	name[19] = '\0';

	scanb( c );
	for ( i = 0; i <= 11; i++ ) {
		c = peekc();
		switch (c) {
		case '0':
			addr[i] = '0';
			dontcare = (dontcare << 1) + 1;
			mask = mask << 1;
			break;
		case '1':
			addr[i] = '1';
			dontcare = (dontcare << 1) + 1;
			mask = (mask << 1) + 1;
			break;
		case 'x':
			addr[i] = 'x';
			dontcare = dontcare << 1;
			mask = mask << 1;
			break;
		default:
			goto botch;
		}
		nextc();
	}
	addr[12] = '\0';
	scanb(c);

	/* check for instruction to be encoded into the call */
	if ( rtn_tipart() == True ) {
		if ( rtn_ramoppart() == True ) {
			if ( rtn_ramctlpart() == True ) {
				rtn_ptrpart();
			}
		}
	}

	saveaddr = curaddr;
	for ( i = 0; i < MAXMAP; i++ ) {
		if ( ( i & dontcare ) == mask ) {
			curnode->addr = curaddr = i;
			curnode->routine = True;

			/* assemble the instruction */
			acode();
			aseq( RTN_BRANCH, RTN_COND, ALPHA, 0, name, RTN_STATE, RTN_LATCH );
			ati8847( func, src, regctl, csrc, outenable, halt, conf );
			adata( dctrl );
			aram( ramcs, ramptr );
			aptr( ptr, act, which, num, sym );
			anext();
		}
	}
	curnode--;	/* slight kludge to undo side effect of anext */
	curaddr = saveaddr;

	/* enter name into symbol table */
	sp = lookup( name );
	if ( sp == 0 ) {
		sp = enter( name );
	}
	label( sp );

	return;
wbotch :
        error ("%s found in routine pseudo_op", rp );
	return;
botch:
	error ("%c is not expected in routine pseudo_op", c );
}

Boolean
rtn_tipart()
{
	register char c;
	register RESERVED *rp;
	char	*cp;

#define CHEK    if(c==';') {c=nextc(); scanb(c); return True;} \
		rp = resw_lookup( cp = scansym( ) ); \
		if ( rp == 0 ) goto wbotch;

	func  = 0x020;		/* default to spass */
	src = 0xff;		/* default to RA input register */
	regctl = 0;
	csrc = 0;
	outenable = 0x2;
	conf = 1;
	halt = 0;

	scanb(c);

	if ( c == '\0' || c == '|' ) {
		return True;
	}

	if ( c == ';' ) {    /* no TIinst field */
		c = nextc();
		scanb(c);
		return True;
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
		outenable = rp->value1;
		scanb( c );
		CHEK;
	}
	if ( rp->type == Output && rp->value2 == 2 ) {
		outenable += rp->value1;
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
			return True;
		}
	}

wbotch :
	error ("%s found in TIinst field", cp );
	return False;
botch:
	error ("TIinst syntax error, character %c is not expected", c);
	return False;

#undef CHEK

}

Boolean
rtn_ramoppart()
{
	register char c;
	register RESERVED *rp;
	char    *cp;

#define CHEK    if(c==';') {c=nextc(); scanb(c); return True;} \
		rp = resw_lookup( cp = scansym( ) ); \
		if ( rp == 0 ) goto wbotch;

	dctrl = 017;	/* default to idle state */
	scanb(c);

	if ( c == '\0' || c == '|' ) {
		return True;
	}

	if ( c == ';' ) {    /* no datamuxing field */
		c = nextc();
		scanb(c);
		return True;
	}
	rp = resw_lookup(cp=scansym( ));
	if ( rp == 0 ) goto wbotch;
	if (rp->type == Regram ) {
		dctrl = rp->value1;
		scanb(c);
		if (c == ';') {
			c = nextc();
			scanb(c);
			return True;
		}
	}
	error ("ramop syntax error, character %c is not expected", c);
	return False;
wbotch :
	error ("%s found in ramop field", cp );
	return False;

#undef CHEK

}

Boolean
rtn_ramctlpart()
{
	register char c;
	register RESERVED *rp;
	char    *cp;

#define CHEK    if(c==';') {c=nextc(); scanb(c); return True;} \
        	rp = resw_lookup(cp=scansym( )); \
        	if ( rp == 0 ) goto wbotch;

	ramcs = 0;
	ramptr = 0;
	scanb(c);

	if ( c == '\0' || c == '|' ) {
		return True;
	}

        if ( c == ';' ) {    /* no ramctrl field */
                c = nextc();
                scanb(c);
                return True;
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
		ramptr = rp->value1;
		scanb(c);
		if (c == ';') {
			c = nextc();
			scanb(c);
			return True;
		}
        }
        error ("ram control field syntax error, character %c is not expected", c);
        return False;
wbotch :
        error ("%s found in ram control field", cp );
        return False;

#undef CHEK
}

Boolean
rtn_ptrpart()
{
	register char 	  c;
	register RESERVED*rp;
	register char 	 *cp;

	ptr = 0;
	act = 0;
	which = NEITHER;
	scanb(c);

	if ( c == '\0' || c == '|' ) {
		return True;
	}

	if ( c== '|' || c == '\0' ) {         /* no ptr_action field */
		if ( c == '|' ) {
			c = nextc();
			scanb(c);
		}
		return True;
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
			if ( c == '|' || c == '\0' ) return True;
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
		if ( c == '|' || c == '\0' ) return True;
	}

wbotch :
        error ("%s found in ptr control field", cp );
        return False;

}
