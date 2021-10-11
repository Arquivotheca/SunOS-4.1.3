#ifndef lint
static  char sccsid[] = "@(#)assemble.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Microassembler instruction builder */

#include <stdio.h>
#include "micro.h"

extern CODE	code[];
extern CODE	*curword;
extern NODE     n[];
extern NODE    *curnode;
extern int      curlineno;
extern short      curaddr;
extern short maxaddr;
extern char    *curfilename;
extern char    *curline;

init_one(i)
    short           i;
{
    n[i].filename = 0;
    n[i].instr = NULL;
    n[i].lineno = 0;
    n[i].line = 0;
    n[i].sccsid = False;
    n[i].filled = False;
    n[i].org_pseudo = False;
    n[i].routine = False;
    n[i].jumpop = False;
}

init_assm()
{
    short           i;

    for (i = 0; i <= 1; i++) {
	init_one(i);
    }

    /* clear the code array */
    memset( code, 0, NNODE * sizeof (CODE) );
}

anext()
{
    register NODE  *rp;
    int             i;

    rp = ++curnode;
    if (curnode - n + 1 < NNODE) {
	init_one(curnode - n + 1);
    }
    if (curnode >= &n[NNODE]) {
	fatal("too many instructions!");
    }
    rp->filename = curfilename;
    rp->lineno = curlineno;
    rp->line = curline;
}

acode()
{
    curword = &code[ curaddr ];
    if ( curword->used ) {
	    error( "code overlaps at %x", curaddr );
    }
    curword->used = True;
    curnode->instr = curword;
}

Boolean
aseq( brnch, condition, which, num, sym, state, latch )
	int	brnch;
	int	condition;
	SYMTYPE which;
	int	num;
	char   *sym;
	int 	state;
	int 	latch;
{
	SYMBOL	*sp;

	curnode->filled = True;
	curword->word1 |= brnch << 14;
	curword->word1 |= condition << 10;
	curword->word2 |= ( state << 10 ) & 0xc00;
	curword->word3 |= ( state << 13 ) & 0x8000;
	curword->word2 |= latch;
	switch (which) {
	case NEITHER:
		break;
	case ALPHA:
		sp = lookup(sym);
		if (sp == 0) {
			curnode->symptr = enter(sym);
		} else
			curnode->symptr = sp;
		break;
	case NUMBER:
		curword->word1 |= (num >> 4) & 0x3ff;
		curword->word2 |= num << 12; 
	}
	return True;
}


Boolean 
ati8847( func, src, regctl, csrc, output, halt, conf )
{
	curnode->filled = True;
	curword->word3 |= func << 1;
	curword->word3 |= ( src >> 7 ) & 1;
	curword->word4 |= src << 9;
	curword->word4 |= regctl << 3;
	curword->word4 |= csrc << 5;
	curword->word4 |= output << 7;
	curword->word4 |= halt << 2;
	curword->word4 |= conf;
	return True;
}


Boolean 
adata ( dctrl)
	int dctrl;
{
	curnode->filled = True;
	curword->word3 |= dctrl << 11;
	return True;
}


Boolean
arecreg ( recreg )
	int recreg;
{
	curnode->filled = True;
	curword->word4 |= recreg << 4;
	return True;
}


Boolean
aram ( ramcs, ptr )
	int ramcs, ptr;
{
	curnode->filled = True;
	curword->word3 |= ramcs << 10;
	curword->word2 |= ptr << 7;
	return True;
}


Boolean
aptr ( ptr, act, which, num, sym )
	int	ptr, act;
	SYMTYPE which;
	int	num;
	char   *sym;
{
	SYMBOL	*sp;

	curnode->filled = True;
	curword->word2 |= ptr << 4;
	curword->word2 |= act << 2;
	switch (which) {
	case NEITHER:
		break;
	case ALPHA:
		sp = lookup(sym);
		if (sp == 0) {
			curnode->symptr = enter(sym);
		} else
			curnode->symptr = sp;
		break;
	case NUMBER:
		curword->word1 |= (num >> 4) & 0x3ff;
		curword->word2 |= num << 12; 
	}
	return True;
}

asccs(cp)
    char           *cp;
{
    --cp;
    while (True) {
	curnode->filled = True;
	curnode->sccsid = True;
	curnode->org_pseudo = True;
	curnode->addr = curaddr;
	curword->word1 = 0;
	curword->word2 = 0;
	curword->word3 = 0;
	curword->word4 = 0;
	if (*(++cp) == '\n') {
	    curword->word1 = '\0' << 8;
	    break;
	} else {
	    curword->word1 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curword->word1 |= '\0';
	    break;
	} else {
	    curword->word1 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curword->word2 = '\0' << 8;
	    break;
	} else {
	    curword->word2 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curword->word2 |= '\0';
	    break;
	} else {
	    curword->word2 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curword->word3 = '\0' << 8;
	    break;
	} else {
	    curword->word3 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curword->word3 |= '\0';
	    break;
	} else {
	    curword->word3 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curword->word4 = '\0' << 8;
	    break;
	} else {
	    curword->word4 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curword->word4 |= '\0';
	    break;
	} else {
	    curword->word4 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
/*	    curword->word5 = '\0' << 8; */
	    break;
	} else {
/*	    curword->word5 = *cp << 8; */
	}
	if (*cp == '\0')
	    break;
	anext();
	curnode->line = NULL;
	curnode->org_pseudo = True;
	curaddr++;
	if ( curaddr > maxaddr ) {
		maxaddr = curaddr;
	}
    }
}

resolve_addrs()
{
    NODE           *nd, *ni;
    struct sym     *sp;

    for (nd = n; nd <= curnode; nd++) {
	if ( sp = nd->symptr ) {
	    if (sp->defined) {
		ni = sp->node;
		while ( ni->org_pseudo == True || ni->routine == True ) ni++;
		nd->instr->word1 |= (ni->addr >> 4) & 0x3ff;
		nd->instr->word2 |= ni->addr << 12;
	    }
	}
    }
}

restrict ( )
{
	/******* Check for sematics here *******/
}
