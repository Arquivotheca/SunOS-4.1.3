/*	@(#)assemble.c 1.1 92/07/30 SMI	*/
/* Microassembler instruction builder */

#include <stdio.h>
#include "micro.h"

extern NODE     n[NNODE + 1];
extern NODE    *curnode;
extern int      curlineno;
extern short      curaddr;
extern char    *curfilename;
extern char    *curline;

init_one(i)
    short           i;
{
    n[i].filename = 0;
    n[i].word1 = 0;
    n[i].word2 = 0;
    n[i].word3 = 0;
    n[i].word4 = 0;
    n[i].word5 = 0;
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

Boolean
aseq (brnch, which, num, sym, state, latch, restore)
	int	brnch;
	SYMTYPE which;
	int	num;
	char   *sym;
	int 	state;
	int 	latch;
{
	SYMBOL	*sp;

	curnode->filled = True;
	curnode->word1 |= brnch << 10;
	curnode->word2 |= state << 9;
	curnode->word2 |= latch << 6;
	curnode->word4 |= restore;
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
		curnode->word1 |= (num >> 4) & 0x3ff;
		curnode->word2 |= num << 12; 
	}
	return True;
}


Boolean 
aweitek (lplus, func, csl, csux, u, woe)
	int	lplus, func, csl, csux, u, woe;
{
	curnode->filled = True;
	curnode->word2 |= lplus << 1;
	curnode->word2 |= func >> 7;
	curnode->word3 |= func << 9;
	curnode->word3 |= csl << 6;
	curnode->word3 |= csux;
	curnode->word4 |= u << 15;
	curnode->word4 |= woe << 12;
	return True;
}


Boolean 
adata ( dctrl)
	int dctrl;
{
	curnode->filled = True;
	curnode->word4 |= dctrl << 6;
	return True;
}


Boolean
arecreg ( recreg )
	int recreg;
{
	curnode->filled = True;
	curnode->word4 |= recreg << 4;
	return True;
}


Boolean
aram ( ramctrl, ramcs, ptr )
	int ramctrl, ramcs, ptr;
{
	curnode->filled = True;
	curnode->word4 |= ramctrl << 3;
	curnode->word4 |= ramcs << 1;
	curnode->word5 |= ptr << 13;
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
	curnode->word5 |= ptr << 10;
	curnode->word5 |= act << 8;
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
		curnode->word1 |= (num >> 4) & 0x3ff;
		curnode->word2 |= num << 12; 
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
	curnode->word1 = 0;
	curnode->word2 = 0;
	curnode->word3 = 0;
	curnode->word4 = 0;
	curnode->word5 = 0;
	if (*(++cp) == '\n') {
	    curnode->word1 = '\0' << 8;
	    break;
	} else {
	    curnode->word1 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word1 |= '\0';
	    break;
	} else {
	    curnode->word1 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word2 = '\0' << 8;
	    break;
	} else {
	    curnode->word2 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word2 |= '\0';
	    break;
	} else {
	    curnode->word2 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word3 = '\0' << 8;
	    break;
	} else {
	    curnode->word3 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word3 |= '\0';
	    break;
	} else {
	    curnode->word3 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word4 = '\0' << 8;
	    break;
	} else {
	    curnode->word4 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word4 |= '\0';
	    break;
	} else {
	    curnode->word4 |= *cp & 0xff;
	}
	if (*cp == '\0')
	    break;
	if (*(++cp) == '\n') {
	    curnode->word5 = '\0' << 8;
	    break;
	} else {
	    curnode->word5 = *cp << 8;
	}
	if (*cp == '\0')
	    break;
	anext();
	curnode->line = NULL;
	curnode->org_pseudo = True;
	curaddr++;
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
		while ( ni->org_pseudo == True ) ni++;
		nd->word1 |= (ni->addr >> 4) & 0x3ff;
		nd->word2 |= ni->addr << 12;
	    }
	}
    }
}

restrict ( )
{
	NODE	*nd, *nc, *ne;
	short	word;
	Boolean first = False;
	Boolean second= False;

    nc = 0;
    nd = n;
    while ( nd->org_pseudo == True ) {
	if (nd->routine == True) first = True;
	nd++;
    }
    ne = nd+1;
    while ( ne<= curnode && ne->org_pseudo == True ) {
	if (ne->routine == True) second = True; 
	ne++;
    }
    if ( ne > curnode ) ne = 0;
    for (; nd && nd <= curnode; ) {
/* 1 2 3 4 have been checked */
	if (((nd->word4 >> 11) & 0x1) == 1) {   /* Wdata enable */
/* 6 */
	    if (nc && ((nc->word4 >> 12) & 0x7 ) != 0 ) {
		error("Line %d: Wdata asserted follows WOE assertion", nd->lineno);
	    }
/* 5 */
	    if (ne && ((ne->word4 >> 12) & 0x7 ) != 0 ) {
		error("Line %d: WOE  = 0 follows Wdata assertion", nd->lineno);
	    }
	}
/* 7 */
        if (((nd->word4 >> 6) & 0x1) == 1 && ((nd->word4 >> 5) & 0x1) == 1) {
	    error("Line %d: ram data enable  and recoe asserted in the same cycle",nd->lineno);
        }
/* 8 */
	if (((nd->word4 >> 3) & 0x1) == 0 && ((nd->word4 >> 1) & 0x3) != 0) {
	    if (((nd->word4 >> 5) & 0x3) != 0) {
		error("Line %d: dtor or recoe asserted in the same cycle of ramcs asserted and ramwe deasserted", nd->lineno);
	    }
	}
/* 9 has been checked */
/* 10 */
        if (((nd->word2 >> 11) & 0x1) == 0 && ((nd->word2 >> 5) & 0x1) == 0) {
	    error("Line %d: idle1 should have L+ from map",nd->lineno);
        }
/* 11 not valid */
/* 12 13 16 has been checked */
/* 15 18 not valid */
/* 17 */
	if (((nd->word2 >> 11) & 0x1) == 0 ) {
	    if ((((nd->word5>>8) & 0x3) == 1 && ((nd->word5>>8) & 0x1f) != 0x19)
			|| ((nd->word2 >> 6) & 3) != 0) { 
	        error("Line %d: load pointer or modereg or WSTATUS or read latch in idle1", nd->lineno);
	    }
        }
/*19 */
	if (((nd->word2 >> 11) & 0x1) == 0 && nc && ((nc->word2 >> 9) & 0x1) == 1) {
	    error ("Line %d: idle1 asserted after hung asserted in previous line", nd->lineno);
	}
/* 20 */
	if (((nd->word2 >> 10) & 0x1) == 1 && ((nd->word1 >> 10) & 0xf) != 0xc) {
	    error("Line %d: idl2 asserted without clear pipe", nd->lineno);
	}
/* extra */
	if ((nd->word4 & 0x0001) == 1 && ((nd->word4 >> 12) & 0x7) != 0) {
	    error("Line %d: restore set with woe set", nd->lineno);
	}
/* 14 */
        if ( first ) {
	    if (((nd->word2 >> 11) & 0x1) == 0) {
	        error("Line %d: first instructioin of routine is in idle1",nd->lineno);
	        first = False;
	    }
	    if (((nd->word2 >> 9) & 0x1) == 1) {
	        error("Line %d: first instructioin of routine is in hung",nd->lineno);
	        first = False;
	    }
        }
        nc = nd;
        nd = ne;
        first = second;
	if (ne == 0) continue;
        ne = nd + 1;
	second = False;
        while ( ne<= curnode && ne->org_pseudo == True ) {
	    if ( ne->routine == True ) second = True;
	    ne++;
        }
        if ( ne > curnode ) ne = 0;
    }
}
