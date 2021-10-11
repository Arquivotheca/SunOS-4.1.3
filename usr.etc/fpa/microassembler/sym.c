/*	@(#)sym.c 1.1 92/07/30 SMI	*/
/*
 *	Micro-assembler symbol-table management
 *		sym.c	1.0	2 JUN. 85
 */

#include "micro.h"

SYMBOL syms[NSYM], *cursym = syms;
char   strings[NSTRING], *curstring = strings;

RESERVED *rhash[NHASH];
SYMBOL   *shash[NHASH];

RESERVED words[] = {

	/*   Pseudo-ops                    */
	{ "org",	Pseudo, 1,         },
	{ "sccsid",     Pseudo, 2,         },
	{ "routine",    Pseudo, 3,         },

	/*   Microcode address source      */

	{ "map",	Branch1, 000, 0,	 },
	{ "call",	Branch1, 020, 2,	 },
	{ "next",	Branch1, 040, 0,	 },
	{ "rtn",	Branch1, 060, 0,	 },

	/*   Jump contional code	   */
	{ "jmp",	Branch2, 001, 1,	},
	{ "jgt",	Branch2, 002, 1,	},
	{ "jge",	Branch2, 003, 1,	},
	{ "jlt",	Branch2, 004, 1,	},
	{ "jle",	Branch2, 005, 1,	},
	{ "jeq",	Branch2, 006, 1,	},
	{ "jne",	Branch2, 007, 1,	},
	{ "jloop0",	Branch2, 010, 1,	},
	{ "jloop",	Branch2, 011, 1,	},
	{ "jwerr",	Branch2, 012, 1,	},
	{ "jnowerr",	Branch2, 013, 1,	},
	{ "jclr",	Branch2, 014, 1,	},

	/*   State			   */
	{ "idl1",	State, 0,	},
	{ "idl2",	State, 6,	},
	{ "hng",	State, 5,	},

	/*   Latch 			   */
	{ "cstat",	Latch, 2, 1,	},
	{ "unimpl",	Latch, 4, 2,	},
	{ "restore",	Latch, 1, 2,	},
	{ "cdata",	Latch, 1, 3,	},

	/*   Weitek Load control	   */
	{ "lnop",	Loadctrl, 0, 1,	},
	{ "lf",		Loadctrl, 1, 0,	},
	{ "lbs",	Loadctrl, 2, 1, },
	{ "lbsf",	Loadctrl, 3, 0,	},
	{ "lbl",	Loadctrl, 4, 1,	},
	{ "lblf",	Loadctrl, 5, 0,	},
	{ "lbm",	Loadctrl, 6, 1,	},
	{ "lbmf",	Loadctrl, 7, 0,	},
	{ "las",	Loadctrl, 10, 1,},
	{ "lasf",	Loadctrl, 11, 0,},
	{ "lal",	Loadctrl, 12, 1,},
	{ "lalf",	Loadctrl, 13, 0,},
	{ "lam",	Loadctrl, 14, 1,},
	{ "lamf",	Loadctrl, 15, 0,},
	{ "lmap",	Loadctrl, 16, 2,},

	/*   Weitek multiply function 	   */
	{ "fmap",	Function, 0200, 2,},
	{ "smul",	Function,  0, 0,},
	{ "dmul",	Function,  1, 0,},
	{ "smwa",	Function,  2, 0,},
	{ "dmwa",	Function,  3, 0,},
	{ "smwb",	Function,  4, 0,},
	{ "dmwb",	Function,  5, 0,},
	{ "smwab",	Function,  6, 0,},
	{ "dmwab",	Function,  7, 0,},
	{ "smma",	Function,  8, 0,},
	{ "dmma",	Function,  9, 0,},
	{ "smwama",	Function, 10, 0,},
	{ "dmwama",	Function, 11, 0,},
	{ "smwbma",	Function, 12, 0,},
	{ "dmwbma",	Function, 13, 0,},
	{ "smwabma",	Function, 14, 0,},
	{ "dmwabma",	Function, 15, 0,},
	{ "smmb",	Function, 16, 0,},
	{ "dmmb",	Function, 17, 0,},
	{ "smwamb",	Function, 18, 0,},
	{ "dmwamb",	Function, 19, 0,},
	{ "smwbmb",	Function, 20, 0,},
	{ "dmwbmb",	Function, 21, 0,},
	{ "smwabmb",	Function, 22, 0,},
	{ "dmwabmb",	Function, 23, 0,},
	{ "smmab",	Function, 24, 0,},
	{ "dmmab",	Function, 25, 0,},
	{ "smwamab",	Function, 26, 0,},
	{ "dmwamab",	Function, 27, 0,},
	{ "smwbmab",	Function, 28, 0,},
	{ "dmwbmab",	Function, 29, 0,},
	{ "smwabmab",	Function, 30, 0,},
	{ "dmwabmab",	Function, 31, 0,},
	{ "smn", 	Function, 32, 0,},
	{ "dmn", 	Function, 33, 0,},
	{ "smwan",	Function, 34, 0,},
	{ "dmwan",	Function, 35, 0,},
	{ "smwbn",	Function, 36, 0,},
	{ "dmwbn",	Function, 37, 0,},
	{ "smwabn",	Function, 38, 0,},
	{ "dmwabn",	Function, 39, 0,},
	{ "smman",	Function, 40, 0,},
	{ "dmman",	Function, 41, 0,},
	{ "smwaman",	Function, 42, 0,},
	{ "dmwaman",	Function, 43, 0,},
	{ "smwbman",	Function, 44, 0,},
	{ "dmwbman",	Function, 45, 0,},
	{ "smwabman",	Function, 46, 0,},
	{ "dmwabman",	Function, 47, 0,},
	{ "smmbn",	Function, 48, 0,},
	{ "dmmbn",	Function, 49, 0,},
	{ "smwambn",	Function, 50, 0,},
	{ "dmwambn",	Function, 51, 0,},
	{ "smwbmbn",	Function, 52, 0,},
	{ "dmwbmbn",	Function, 53, 0,},
	{ "smwabmbn",	Function, 54, 0,},
	{ "dmwabmbn",	Function, 55, 0,},
	{ "smmabn",	Function, 56, 0,},
	{ "dmmabn",	Function, 57, 0,},
	{ "smwamabn",	Function, 58, 0,},
	{ "dmwamabn",	Function, 59, 0,},
	{ "smwbmabn",	Function, 60, 0,},
	{ "dmwbmabn",	Function, 61, 0,},
	{ "smwabmabn",	Function, 62, 0,},
	{ "dmwabmabn",	Function, 63, 0,},

	/*   Weitek alu function	   */
	{ "ssub",	Function,  0, 1,},
	{ "dsub",	Function,  1, 1,},
	{ "sdiff",	Function,  2, 1,},
	{ "ddiff",	Function,  3, 1,},
	{ "ssubm",	Function,  4, 1,},
	{ "dsubm",	Function,  5, 1,},
	{ "sdiv",	Function,  6, 1,},
	{ "ddiv",	Function,  7, 1,},
	{ "sneg",	Function,  8, 1,},
	{ "dneg",	Function,  9, 1,},
	{ "sdiva",	Function, 14, 1,},
	{ "ddiva",	Function, 15, 1,},
	{ "sadd",	Function, 16, 1,},
	{ "dadd",	Function, 17, 1,},
	{ "ssum",	Function, 18, 1,},
	{ "dsum",	Function, 19, 1,},
	{ "saddm",	Function, 20, 1,},
	{ "daddm",	Function, 21, 1,},
	{ "sdivb",	Function, 22, 1,},
	{ "ddivb",	Function, 23, 1,},
	{ "sident",	Function, 24, 1,},
	{ "dident", 	Function, 25, 1,},
	{ "sabs",	Function, 28, 1,},
	{ "dabs",	Function, 29, 1,},
	{ "sdivab",	Function, 30, 1,},
	{ "ddivab",	Function, 31, 1,},
	{ "scmp",	Function, 32, 1,},
	{ "dcmp", 	Function, 33, 1,},
	{ "scmpm",	Function, 36, 1,},
	{ "dcmpm",	Function, 37, 1,},
	{ "scmp0", 	Function, 40, 1,},
	{ "dcmp0",	Function, 41, 1,},
	{ "sexct", 	Function, 48, 1,},
	{ "dexct",	Function, 49, 1,},
	{ "swdnrm",	Function, 50, 1,},
	{ "dwdnrm",	Function, 51, 1,},
	{ "sinxct",	Function, 52, 1,},
	{ "dinxct",	Function, 53, 1,},
	{ "scvtint",	Function, 56, 1,},
	{ "dcvtint",	Function, 57, 1,},
	{ "sfloat",	Function, 58, 1,},
	{ "dfloat",	Function, 59, 1,},
	{ "dcvts",	Function, 60, 1,},
	{ "scvtd",	Function, 61, 1,},

	/*   Mode			   */
	{ "lmode3.0",	Mode, 8, 0x0,	},
	{ "lmode7.4",	Mode, 8, 0x10,	},
	{ "lmodeb.8",	Mode, 8, 0x20,	},
	{ "lmodef.c",	Mode, 8, 0x30,	},
	{ "lmodeusr",	Mode, 8, 0x40,	},

	/*   CSL- control		   */
	{ "csl",	Csl, 0,	},
	{ "cslc",	Csl, 1,	},
	{ "csla",	Csl, 2,	},
	{ "cslac",	Csl, 3, },
	{ "cslm",	Csl, 4,	},
	{ "cslmc",	Csl, 5,	},
	{ "cslma",	Csl, 6,	},
	{ "cslmac",	Csl, 7,	},

	/*   CSUX- control		   */
	{ "csux",	Csux, 0,	},
	{ "csuxc",	Csux, 1,	},
	{ "csuxa",	Csux, 2,	},
	{ "csuxac",	Csux, 3,	},
	{ "csuxm",	Csux, 4,	},
	{ "csuxmc",	Csux, 5,	},
	{ "csuxma",	Csux, 6,	},
	{ "csuxmac",	Csux, 7,	},

	/*   U+ control			   */
	{ "ulsw",	U, 0,	},
	{ "umsw",	U, 1,	},
	{ "usp",	U, 1,	},

	/*   WOE- control		   */
	{ "woec",	Woe, 1,	},
	{ "woea",	Woe, 2,	},
	{ "woem",	Woe, 4,	},

	/*   Data muxing		   */
	{ "opdmsw",	Datactrl, 044, 1,	},
	{ "opdsp",	Datactrl, 044, 1,	},
	{ "opdlsw",	Datactrl, 042, 1,	},
	{ "reg",	Datactrl, 050, 0,	},
	{ "dtor",	Datactrl, 021, 0,	},

	/*   Recovery register control	   */
	{ "recoe",	Recoe, 2,	},
	{ "recclk",	Reclk, 1,	},

	/*   Ram WE-			   */
	{ "ramwe",	Ramwe, 1, 	},

	/*   Ram CS-			   */
	{ "rcsmsw",	Ramcs, 2, 1,	},
	{ "rcssp",	Ramcs, 2, 1,	},
	{ "rcslsw",	Ramcs, 1, 2,	},

	/*   Pointers			   */
	{ "ptr1",	Pointer, 1, 0,	},
	{ "ptr2",	Pointer, 2, 0,	},
	{ "ptr3",	Pointer, 3, 0,	},
	{ "ptr4",	Pointer, 4, 0,	},
	{ "ptr5",	Pointer, 5, 0,	},
	{ "ldptr",	Pointer, 0, 1,	},
	{ "imm2",	Pointer, 6, 1,	},
	{ "imm3",	Pointer, 7, 1,	},
	{ "lpreg",	Pointer, 6, 2,	},
	{ "mreg",	Pointer, 7, 2,	},

	{ (char *)0,	Tnull,  0,   0, }
};

extern SYMBOL syms[];

init_symtab()
{   int i;

    for (i = 0; i < NSYM; i++) {
	syms[i].defined = False;
    }
}

int
hashval( s ) 
    char *s;
{
    register int val = 0;

    while (*s) {
	val = (val<<1) + *s++;
    }
    val %= NHASH;
    if (val < 0) val += NHASH;
    return val;
}

void
resw_hash()
{
    RESERVED *rp, **pp;

    rp = words;
    while (rp->name) {
	pp = rhash + hashval(rp->name);
	rp->next_hash = *pp;
	*pp = rp;
	rp++;
    }
}

RESERVED *
resw_lookup( s )
    char *s;
{
    register RESERVED *rp;

    rp = rhash[hashval(s)];
    while (rp && strcmp(s,rp->name))
	rp = rp->next_hash;
    return rp;
}

SYMBOL *
lookup( s )
    char *s;
{
    register SYMBOL *sp;

    sp = shash[hashval(s)];
    while ( sp && strcmp( s, sp->name ))
	sp = sp->next_hash;
    return sp;
}

SYMBOL *
enter( s )
    char *s;
{
    register SYMBOL *sp;
    SYMBOL **pp;
    register char *cp;
    int leng;

    leng = strlen(s);
    cp = curstring;
    if ( curstring+leng > &strings[NSTRING-2])
	fatal( "out of string space");
    while( *cp++ = *s++)
	;
    if (cursym >= &syms[NSYM])
	fatal( "out of symbol space");
    sp = cursym++;
    pp = shash + hashval(sp->name = curstring);
    curstring = cp;
    sp->next_hash = *pp;
    *pp = sp;
    sp->node = 0;
    sp->addr = 0;
    return sp;
}

SYMBOL *
firstsym()
{
    return (cursym == syms)? (SYMBOL *)0 : syms;
}
SYMBOL *
nextsym( sp )
    SYMBOL *sp;
{
    sp++;
    return (sp==cursym)? (SYMBOL *)0 : sp;
}
