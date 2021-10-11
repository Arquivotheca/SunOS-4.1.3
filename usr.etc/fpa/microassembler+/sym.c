#ifndef lint
static  char sccsid[] = "@(#)sym.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

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
	{ "org",	Pseudo, 1,	},
	{ "sccsid",     Pseudo, 2,	},
	{ "routine",    Pseudo, 3,	},

	/*   Microcode address source      */
	{ "pipe",	Branch1, 0x0, 0,	},
	{ "next",	Branch1, 0x1, 0,	},
	{ "call",	Branch1, 0x2, 2,	},
	{ "rtn",	Branch1, 0x3, 0,	},

	/*   Jump contional code	   */
	{ "jmp",	Branch2, 0x01, 1,	},
	{ "jgt",	Branch2, 0x02, 1,	},
	{ "jge",	Branch2, 0x03, 1,	},
	{ "jlt",	Branch2, 0x04, 1,	},
	{ "jle",	Branch2, 0x05, 1,	},
	{ "jeq",	Branch2, 0x06, 1,	},
	{ "jne",	Branch2, 0x07, 1,	},
	{ "jloop0",	Branch2, 0x08, 1,	},
	{ "jloop",	Branch2, 0x09, 1,	},
	{ "jtierr",	Branch2, 0x0a, 1,	},
	{ "jnotierr",	Branch2, 0x0b, 1,	},
	{ "jclr",	Branch2, 0x0c, 1,	},
	{ "jtifast",	Branch2, 0x0d, 1,	},

	/*   State			   */
	{ "idl1",	State, 0x2,	},
	{ "idl2",	State, 0x4,	},
	{ "hng",	State, 0x1,	},

	/*   Register RAM operations	   */
	{ "regtoti",	Regram, 0x0,	},
	{ "titoreg",	Regram, 0x1,	},
	{ "regtotmp",	Regram, 0x2,	},
	{ "tmptoreg",	Regram, 0x3,	},
	{ "tmptoti",	Regram, 0x4,	},
	{ "titotmp",	Regram, 0x5,	},
	{ "adtoti",	Regram, 0x6,	},
	{ "adtoreg",	Regram, 0x7,	},
	{ "optoti",	Regram, 0x8,	},
	{ "optoreg",	Regram, 0x9,	},
	{ "readreg",	Regram, 0xa,	},
	{ "restore",	Regram, 0xb,	},
	{ "regtitmp",	Regram, 0xc,	},

	/*   Latch 			   */
	{ "cstat",	Latch, 0, 1,	},
	{ "unimpl",	Latch, 2, 2,	},
	{ "cdata",	Latch, 0, 3,	},

	/*   Pointers			   */
	{ "lptr",	Pointer, 0, 1,	},
	{ "ptr1",	Pointer, 1, 0,	},
	{ "ptr2",	Pointer, 2, 0,	},
	{ "ptr3",	Pointer, 3, 0,	},
	{ "ptr4",	Pointer, 4, 0,	},
	{ "ptr5",	Pointer, 5, 0,	},
	{ "imm2",	Pointer, 6, 1,	},
	{ "imm3",	Pointer, 7, 1,	},
	{ "lpreg",	Pointer, 6, 2,	},
	{ "mreg",	Pointer, 7, 2,	},

	/*   Ram CS-			   */
	{ "rcsmsw",	Ramcs, 0,	},
	{ "rcslsw",	Ramcs, 1,	},
	{ "rcssp",	Ramcs, 0,	},

	/*   Instruction input		   */
	{ "sadd",	Function, 0x000	},
	{ "ssub",	Function, 0x001	},
	{ "scmp",	Function, 0x002	},
	{ "srsub",	Function, 0x003	},
	{ "ssum",	Function, 0x004	},
	{ "sdiff",	Function, 0x005	},
	{ "scmpym",	Function, 0x006	},
	{ "srsubym",	Function, 0x007	},
	{ "saddbm",	Function, 0x008	},
	{ "ssubbm",	Function, 0x009	},
	{ "scmpbm",	Function, 0x00a	},
	{ "srsubbm",	Function, 0x00b	},
	{ "saddbmym",	Function, 0x00c	},
	{ "ssubbmym",	Function, 0x00d	},
	{ "scmpbmym",	Function, 0x00e	},
	{ "srsubbmym",	Function, 0x00f	},
	{ "saddam",	Function, 0x010	},
	{ "ssubam",	Function, 0x011	},
	{ "scmpam",	Function, 0x012	},
	{ "srsubam",	Function, 0x013	},
	{ "saddamym",	Function, 0x014	},
	{ "ssubamym",	Function, 0x015	},
	{ "scmpamym",	Function, 0x016	},
	{ "srsubamym",	Function, 0x017	},
	{ "saddm",	Function, 0x018	},
	{ "ssubm",	Function, 0x019	},
	{ "scmpm",	Function, 0x01a	},
	{ "srsubm",	Function, 0x01b	},
	{ "saddmym",	Function, 0x01c	},
	{ "ssubmym",	Function, 0x01d	},
	{ "scmpmym",	Function, 0x01e	},
	{ "srsubmym",	Function, 0x01f	},
	{ "snop",	Function, 0x020	},
	{ "sneg",	Function, 0x021	},
	{ "sfloat",	Function, 0x022	},
	{ "scvtint",	Function, 0x023	},
	{ "scvtd",	Function, 0x026	},
	{ "swdnrm",	Function, 0x028	},
	{ "sexct",	Function, 0x02c	},
	{ "sinxct",	Function, 0x02d	},
	{ "srnd",	Function, 0x02e	},
	{ "sabs",	Function, 0x030	},
	{ "snegym",	Function, 0x031	},
	{ "sfloatym",	Function, 0x032	},
	{ "scvtintym",	Function, 0x033	},
	{ "scvtdym",	Function, 0x036	},
	{ "swdnrmym",	Function, 0x038	},
	{ "sexctym",	Function, 0x03c	},
	{ "sinxctym",	Function, 0x03d	},
	{ "srndym",	Function, 0x03e	},
	{ "smul",	Function, 0x040	},
	{ "smwb",	Function, 0x041	},
	{ "smwa",	Function, 0x042	},
	{ "smwab",	Function, 0x043	},
	{ "smn",	Function, 0x044	},
	{ "smwbn",	Function, 0x045	},
	{ "smwan",	Function, 0x046	},
	{ "smwabn",	Function, 0x047	},
	{ "smmb",	Function, 0x048	},
	{ "smwbmb",	Function, 0x049	},
	{ "smwamb",	Function, 0x04a	},
	{ "smwabmb",	Function, 0x04b	},
	{ "smmbn",	Function, 0x04c	},
	{ "smwbmbn",	Function, 0x04d	},
	{ "smwambn",	Function, 0x04e	},
	{ "smwabmbn",	Function, 0x04f	},
	{ "smma",	Function, 0x050	},
	{ "smwbma",	Function, 0x051	},
	{ "smwama",	Function, 0x052	},
	{ "smwabma",	Function, 0x053	},
	{ "smman",	Function, 0x054	},
	{ "smwbman",	Function, 0x055	},
	{ "smwaman",	Function, 0x056	},
	{ "smwabman",	Function, 0x057	},
	{ "smmab",	Function, 0x058	},
	{ "smwbmab",	Function, 0x059	},
	{ "smwamab",	Function, 0x05a	},
	{ "smwabmab",	Function, 0x05b	},
	{ "smmabn",	Function, 0x05c	},
	{ "smwbmabn",	Function, 0x05d	},
	{ "smwamabn",	Function, 0x05e	},
	{ "smwabmabn",	Function, 0x05f	},
	{ "sdiv",	Function, 0x060	},
	{ "sdivb",	Function, 0x061	},
	{ "sdiva",	Function, 0x062	},
	{ "sdivab",	Function, 0x063	},
	{ "sdn",	Function, 0x064	},
	{ "sdbn",	Function, 0x065	},
	{ "sdan",	Function, 0x066	},
	{ "sdabn",	Function, 0x067	},
	{ "sqrt",	Function, 0x068	},
	{ "sqrta",	Function, 0x06a	},
	{ "sqrtn",	Function, 0x06c	},
	{ "sqrtan",	Function, 0x06e	},
	{ "sdivma",	Function, 0x070	},
	{ "sdivmawb",	Function, 0x071	},
	{ "sdivmawa",	Function, 0x072	},
	{ "sdivmawab",	Function, 0x073	},
	{ "sdivman",	Function, 0x074	},
	{ "sdivmawbn",	Function, 0x075	},
	{ "sdivmawan",	Function, 0x076	},
	{ "sdivmawabn",	Function, 0x077	},
	{ "sqrtma",	Function, 0x078	},
	{ "sqrtwama",	Function, 0x07a	},
	{ "sqrtman",	Function, 0x07c	},
	{ "sqrtwaman",	Function, 0x07e	},
	{ "dadd",	Function, 0x080	},
	{ "dsub",	Function, 0x081	},
	{ "dcmp",	Function, 0x082	},
	{ "drsub",	Function, 0x083	},
	{ "dsum",	Function, 0x084	},
	{ "ddiff",	Function, 0x085	},
	{ "dcmpym",	Function, 0x086	},
	{ "drsubym",	Function, 0x087	},
	{ "daddbm",	Function, 0x088	},
	{ "dsubbm",	Function, 0x089	},
	{ "dcmpbm",	Function, 0x08a	},
	{ "drsubbm",	Function, 0x08b	},
	{ "daddbmym",	Function, 0x08c	},
	{ "dsubbmym",	Function, 0x08d	},
	{ "dcmpbmym",	Function, 0x08e	},
	{ "drsubbmym",	Function, 0x08f	},
	{ "daddam",	Function, 0x090	},
	{ "dsubam",	Function, 0x091	},
	{ "dcpmam",	Function, 0x092	},
	{ "drsubam",	Function, 0x093	},
	{ "daddamym",	Function, 0x094	},
	{ "dsubamym",	Function, 0x095	},
	{ "dcmpamym",	Function, 0x096	},
	{ "drsubamym",	Function, 0x097	},
	{ "daddm",	Function, 0x098	},
	{ "dsubm",	Function, 0x099	},
	{ "dcmpm",	Function, 0x09a	},
	{ "drsubm",	Function, 0x09b	},
	{ "daddmym",	Function, 0x09c	},
	{ "dsubmym",	Function, 0x09d	},
	{ "dcmpmym",	Function, 0x09e	},
	{ "drsubmym",	Function, 0x09f	},
	{ "dnop",	Function, 0x0a0 },
	{ "dneg",	Function, 0x0a1	},
	{ "dfloat",	Function, 0x0a2	},
	{ "dcvtint",	Function, 0x0a3	},
	{ "dcvts",	Function, 0x0a6	},
	{ "dwdnrm",	Function, 0x0a8	},
	{ "dinxct",	Function, 0x0ad	},
	{ "dinxctym",	Function, 0x0bd },
	{ "dabs",	Function, 0x0b0	},
	{ "dmul",	Function, 0x0c0	},
	{ "dmwb",	Function, 0x0c1	},
	{ "dmwa",	Function, 0x0c2	},
	{ "dmwab",	Function, 0x0c3	},
	{ "dmn",	Function, 0x0c4	},
	{ "dmwbn",	Function, 0x0c5	},
	{ "dmwan",	Function, 0x0c6	},
	{ "dmwabn",	Function, 0x0c7	},
	{ "dmmb",	Function, 0x0c8	},
	{ "dmwbmb",	Function, 0x0c9	},
	{ "dmwamb",	Function, 0x0ca	},
	{ "dmwabmb",	Function, 0x0cb	},
	{ "dmmbn",	Function, 0x0cc	},
	{ "dmwbmbn",	Function, 0x0cd	},
	{ "dmwambn",	Function, 0x0ce	},
	{ "dmwabmbn",	Function, 0x0cf	},
	{ "dmma",	Function, 0x0d0	},
	{ "dmwbma",	Function, 0x0d1	},
	{ "dmwama",	Function, 0x0d2	},
	{ "dmwabma",	Function, 0x0d3	},
	{ "dmman",	Function, 0x0d4	},
	{ "dmwbman",	Function, 0x0d5	},
	{ "dmwaman",	Function, 0x0d6	},
	{ "dmwabman",	Function, 0x0d7	},
	{ "dmmab",	Function, 0x0d8	},
	{ "dmwbmab",	Function, 0x0d9	},
	{ "dmwamab",	Function, 0x0da	},
	{ "dmwabmab",	Function, 0x0db	},
	{ "dmmabn",	Function, 0x0dc	},
	{ "dmwbmabn",	Function, 0x0dd	},
	{ "dmwamabn",	Function, 0x0de	},
	{ "dmwabmabn",	Function, 0x0df	},
	{ "dexct",      Function, 0x0ac },
	{ "dexctym",    Function, 0x0bc },
	{ "ddiv",	Function, 0x0e0	},
	{ "ddivb",	Function, 0x0e1	},
	{ "ddiva",	Function, 0x0e2	},
	{ "ddivab",	Function, 0x0e3	},
	{ "dadn",	Function, 0x0e4	},
	{ "dadbn",	Function, 0x0e5	},
	{ "dadan",	Function, 0x0e6	},
	{ "dadabn",	Function, 0x0e7	},
	{ "dsqrt",	Function, 0x0e8	},
	{ "dsqrta",	Function, 0x0ea	},
	{ "dsqrtn",	Function, 0x0ec	},
	{ "dsqrtan",	Function, 0x0ee	},
	{ "ddivma",	Function, 0x0f0	},
	{ "ddivmawb",	Function, 0x0f1	},
	{ "ddivmawa",	Function, 0x0f2	},
	{ "ddivmawab",	Function, 0x0f3	},
	{ "ddivman",	Function, 0x0f4	},
	{ "ddivmawbn",	Function, 0x0f5	},
	{ "ddivmawan",	Function, 0x0f6	},
	{ "ddivmawabn",	Function, 0x0f7	},
	{ "dsqrtma",	Function, 0x0f8	},
	{ "dsqrtwama",	Function, 0x0fa	},
	{ "dsqrtman",	Function, 0x0fc	},
	{ "dsqrtwaman",	Function, 0x0fe	},
	{ "i2add",	Function, 0x100	},
	{ "i2sub",	Function, 0x101	},
	{ "i2cmp",	Function, 0x102	},
	{ "i2rsub",	Function, 0x103	},
	{ "i2land",	Function, 0x108	},
	{ "i2landnb",	Function, 0x109	},
	{ "i2landna",	Function, 0x10a	},
	{ "i2lor",	Function, 0x10c	},
	{ "i2lxor",	Function, 0x10d	},
	{ "i2pass",	Function, 0x120	},
	{ "i2negy2",	Function, 0x121	},
	{ "i2negy1",	Function, 0x122	},
	{ "i2sftll",	Function, 0x128	},
	{ "i2sftrl",	Function, 0x129	},
	{ "i2sftra",	Function, 0x12d	},
	{ "imul",	Function, 0x140	},
	{ "idiv",	Function, 0x160	},
	{ "isqrt",	Function, 0x168	},
	{ "iuadd",	Function, 0x180	},
	{ "iusub",	Function, 0x181	},
	{ "iucmp",	Function, 0x182	},
	{ "iursub",	Function, 0x183	},
	{ "iuland",	Function, 0x188	},
	{ "iulandnb",	Function, 0x189	},
	{ "iulandna",	Function, 0x18a	},
	{ "iulor",	Function, 0x18c	},
	{ "iulxor",	Function, 0x18d	},
	{ "iupassy",	Function, 0x1a0	},
	{ "iunegy2",	Function, 0x1a1	},
	{ "iunegy1",	Function, 0x1a2	},
	{ "iusftll",	Function, 0x1a8	},
	{ "iusftrl",	Function, 0x1a9	},
	{ "iusftra",	Function, 0x1ad	},
	{ "iumul",	Function, 0x1c0	},
	{ "iudiv",	Function, 0x1e0	},
	{ "iusqrt",	Function, 0x1e8	},

	/*   Input source		   */
	{ "aluacreg",	Source, 0x01, 1	},
	{ "aluamul",	Source, 0x02, 1	},
	{ "aluara",	Source, 0x03, 1	},
	{ "alubcreg",	Source, 0x04, 2	},
	{ "alubalu",	Source, 0x08, 2	},
	{ "alubrb",	Source, 0x0c, 2	},
	{ "mulacreg",	Source, 0x10, 3	},
	{ "mulaalu",	Source, 0x20, 3	},
	{ "mulara",	Source, 0x30, 3	},
	{ "mulbcreg",	Source, 0x40, 4	},
	{ "mulbmul",	Source, 0x80, 4	},
	{ "mulbrb",	Source, 0xc0, 4	},

	/*   Output control		   */
	{ "tioe",	Output, 0, 1	},
	{ "tilsw",	Output, 0, 2	},
	{ "timsw",	Output, 1, 2	},
	{ "tisp",	Output, 1, 2	},

	/*   C register source		   */
	{ "srccalu",	Csrc, 0, 1	},
	{ "srccmul",	Csrc, 2, 1	},
	{ "clkcc",	Csrc, 1, 2	},

	/*   Register control		   */
	{ "enra",	Regctl, 2, 1	},
	{ "enrb",	Regctl, 1, 2	},

	/*   Halt input			   */
	{ "halt",	Halt, 1,	},

	/* Configuration		   */
	{ "loadrv",	Config, 0	},
	{ "loadsp",	Config, 1	},
	{ "loaddp",	Config, 2	},
	{ "loadms",	Config, 3	},

	/*   End of table		   */
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
