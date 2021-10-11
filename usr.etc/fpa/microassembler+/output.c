#ifndef lint
static  char sccsid[] = "@(#)output.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *	Microassembler output routines
 */

#include "micro.h"
#include <stdio.h>

extern char *version();
extern short curaddr;
extern short maxaddr;
extern NODE *curnode, n[];
extern NODE *adr[];
extern CODE code[];
extern CODE *curword;
extern SYMBOL *shash[];
extern char *abinfile;
extern char *binfile;

alpha_out(){
    /* print listing */
    register NODE *nd;
    register int i;
    CODE *cp;
    Boolean any_undef = False;
    SYMBOL *sp;

    printf( "\nSun Microsystems FPA-3X Microassembler -- Version %s -- Company Confidential\n", version() );
    for (i = 0; i < NHASH; i++) {
	sp = shash[i];
	while (sp != 0) {
	    if (!sp->defined) {
		if (!any_undef) {
		    any_undef = True;
		    error ("\nError:  undefined symbols:\n");
		}
		printf("     %s\n",sp->name);
	    }
	    sp = sp->next_hash;
	}
    }
/*	Let it print for now
    if (any_undef) {
	printf("\n");
	return;
    }
*/

    printf("Microcode Output\n");
    printf("\n            uC  jmp next ram ptr ptr abac ram 8847 op  TI\n");
    printf(  "   ln# addr src cc  addr src sel act bits op  inst src bits conf\n");
    for( nd = n; nd <=curnode; nd++) {
	if (nd->org_pseudo) {
	    printf("\t\t\t\t\t\t\t\t%s",nd->line);
	} else if (nd->sccsid) {
	    printf("%7x  ",nd->addr);
	    printf("%04x%04x%04x%04x                           ",cp->word1,cp->word2,cp->word3,
		cp->word4 );
	    printf("%s", nd->line != 0 ? nd->line : "\n");
	} else if (nd->filled) {
	    cp = nd->instr;
	    printf(" %4d%5x  ",nd->lineno, nd->addr);
	    printf(" %01x   %01x  %04x  %01x   %01x   %01x   %02x   %01x   %03x  %02x  %02x    %01x\t",
		( cp->word1 >> 14 ) & 0x3,
		( cp->word1 >> 10 ) & 0xf,
		( ( cp->word1 << 4 ) & 0x3ff0 ) | ( ( cp->word2 >> 12 ) & 0xf ),
		( cp->word2 >> 7 ) & 0x7,
		( cp->word2 >> 4 ) & 0x7,
		( cp->word2 >> 2 ) & 0x3,
		( ( cp->word2 >> 6 ) & 0x20 ) | ( ( cp->word3 >> 14 ) & 0x2 ) |
		   ( ( cp->word2 << 2 ) & 0xc ) | ( ( cp->word3 >> 10 ) & 0x1 ),
		( cp->word3 >> 11 ) & 0xf,
		( cp->word3 >> 1 ) & 0x1ff,
		( ( cp->word3 << 7 ) & 0x80 ) | ( ( cp->word4 >> 9 ) & 0x7f ),
		( cp->word4 >> 2 ) & 0x7f,
		cp->word4 & 0x3 );
	    printf("%s", nd->line);
	}
    }
}

/*
 * write pseudo binary output
 */
write_abinary()
{
	int i, j, lastout;
	char name[ 256 ];
	FILE *f[ 16 ];

	lastout = -1;
	for ( i = 0; i < 16; i++ ) {
		sprintf( name, "%s.%d", abinfile, i );
		f[ i ] = fopen( name, "w+" );
		if ( f[ i ] == NULL ) {
			fprintf( stderr, "%s ", name );
			perror( "open failed" );
			goto abort;
		}
	}

	for ( i = 0; i < maxaddr; i++ ) {
		if ( code[ i ].used ) {
			for ( j = lastout + 1; j <= i; j++ ) {
				write_code( &code[ j ], f );
			}
			lastout = i;
		}
	}

abort:
	for ( i = 0; i < 16; i++ ) {
		fprintf( f[ i ], "\n" );
		fclose( f[ i ] );
	}
}

write_code( code, files )
	CODE *code;
	FILE *files[];
{
	int i;
	unsigned short *word;

	word = &code->word1;
	for ( i = 0; i < 16; i++ ) {
		if ( i % 4 == 0 ) {
			fprintf( files[ i >> 2 ], "\n" );
		}
		if ( *word & ( 0x8000 >> ( i % 16 ) ) ) {
			fprintf( files[ i >> 2 ], "1" );
		} else {
			fprintf( files[ i >> 2 ], "0" );
		}
	}
	word = &code->word2;
	for ( i = 0; i < 16; i++ ) {
		if ( i % 4 == 0 ) {
			fprintf( files[ (i >> 2) + 4 ], "\n" );
		}
		if ( *word & ( 0x8000 >> ( i % 16 ) ) ) {
			fprintf( files[ (i >> 2) + 4 ], "1" );
		} else {
			fprintf( files[ (i >> 2) + 4 ], "0" );
		}
	}
	word = &code->word3;
	for ( i = 0; i < 16; i++ ) {
		if ( i % 4 == 0 ) {
			fprintf( files[ (i >> 2) + 8 ], "\n" );
		}
		if ( *word & ( 0x8000 >> ( i % 16 ) ) ) {
			fprintf( files[ (i >> 2) + 8 ], "1" );
		} else {
			fprintf( files[ (i >> 2) + 8 ], "0" );
		}
	}
	word = &code->word4;
	for ( i = 0; i < 16; i++ ) {
		if ( i % 4 == 0 ) {
			fprintf( files[ (i >> 2) + 12 ], "\n" );
		}
		if ( *word & ( 0x8000 >> ( i % 16 ) ) ) {
			fprintf( files[ (i >> 2) + 12 ], "1" );
		} else {
			fprintf( files[ (i >> 2) + 12 ], "0" );
		}
	}
}

/*
 * write binary output
 */
write_binary()
{
	int i, j;
	int lastout;
	FILE *binstream;

	binstream = fopen( binfile, "w+" );
	if ( binstream == NULL ) {
		fprintf( stderr, "%s ", binfile );
		perror( "open failed" );
		return;
	}

	lastout = -1;
	fwrite( &maxaddr, sizeof maxaddr, 1, binstream );
	for ( i = 0; i < maxaddr; i++ ) {
		if ( code[ i ].used ) {
			for ( j = lastout + 1; j <= i; j++ ) {
				fwrite( &code[ j ].word1, sizeof (short), 1, binstream );
				fwrite( &code[ j ].word2, sizeof (short), 1, binstream );
				fwrite( &code[ j ].word3, sizeof (short), 1, binstream );
				fwrite( &code[ j ].word4, sizeof (short), 1, binstream );
			}
			lastout = i;
		}
	}

	fclose( binstream );
}

output()
{
    /* output the listing */
    alpha_out();

    /* if ascii pseudo binary output is requested then do it */
    if ( abinfile != NULL ) {
        write_abinary();
    }

    /* if binary output is requested then do it */
    if ( binfile != NULL ) {
        write_binary();
    }
}
