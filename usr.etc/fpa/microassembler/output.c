/*	@(#)output.c 1.1 92/07/30 SMI	*/
/*
 *	Microassembler output routines
 */

#include "micro.h"
#include <stdio.h>

extern RAMNODE ram[];
extern short curaddr;
extern NODE *curnode, n[];
extern NODE *adr[];
extern SYMBOL *shash[];
FILE *binfile;
FILE *tblfile;

alpha_out(){
    /* print listing */
    register NODE *nd;
    register int i;
    Boolean any_undef = False;
    SYMBOL *sp;

    printf("\nSun Microsystems FPA Microassembler -- Version 1.0 (10 June 1985) -- Company Confidential\n");
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
    if (any_undef) {
	printf("\n");
	return;
    }

    printf("Microcode Output\n");
    printf("\n                 next                 data rec ram  ptr\n");
    printf(  "    ln# addr seq addr stat L+ F+ sgnl mux  reg ctrl ctrl  source line\n");
    for( nd = n; nd <=curnode; nd++) {
	if (nd->org_pseudo) {
	    printf("                                                     %s",nd->line);
	} else if (nd->sccsid) {
	    printf("%7x  ",nd->addr);
	    printf("%04x%04x%04x%04x%02x                           ",nd->word1,nd->word2,nd->word3,
		nd->word4, nd->word5>>8 );
	    printf("%s", nd->line != 0 ? nd->line : "\n");
	} else if (nd->filled) {
	    printf("  %4d%5x  ",nd->lineno, nd->addr);
	    printf("%02x  %04x  %02x%1s %02x %02x %04x  %02x   %01x   %02x   %02x   ",
		nd->word1 >> 10,
		((nd->word1 << 4) + (nd->word2 >> 12)) & 0x3fff,
		(nd->word2 >> 6) & 0x3f,
		(nd->word4 & 0x0001) ? "*" : " ",
		(nd->word2 >> 1) & 0x1f,
		((nd->word2 << 7) + (nd->word3 >> 9) ) & 0xff,
		((nd->word3 << 4) + (nd->word4 >> 12)) & 0x1fff,
		(nd->word4 >> 6) & 0x3f,
		(nd->word4 >> 4) & 0x3,
		((nd->word4 >> 1 << 3) + ((nd->word5 >> 13) & 0x7)) & 0x7f,
		((nd->word5 >> 8) & 0x1f ));
	    printf("%s", nd->line);
	}
    }
    printf("\n\n");
    printf("Ram Table Output\n\n");
    printf("name             addr in ram     entry in ucode   L+        F+\n");
    for( i=0; i<NINST; i++ ) {
	if ( ram[i].name[0] != 0 ) {
		printf("%-17s%12s        %4x         %x         %2x\n", 
			ram[i].name, ram[i].addr, ram[i].entry, ram[i].load, ram[i].func);
	}
    }
}

FILE *
open_a_file( s )
    char *s;
{
    register int i;
    i = creat( s, 0644 );
    if (i<0){
	perror( s );
	fatal("could not create output file");
    }
    return fdopen( i, "w");
}

write_binary()
{
        register NODE *ni;
        short word;
	int   addr = 0;

	fwrite(&curaddr, sizeof(curaddr), 1, binfile);
        for( ni = n; ni <=curnode; ni++ ) {
		if (ni->org_pseudo) continue;
		while (ni->addr != addr ) {
			word = 0;
			fwrite(&word,sizeof(word),1,binfile);
			word = 0x40;
			fwrite(&word,sizeof(word),1,binfile);
			word = 0x8;
			fwrite(&word,sizeof(word),1,binfile);
			word = 0;  
			fwrite(&word,sizeof(word),1,binfile);
			word = 0;
			fwrite(&word,sizeof(word),1,binfile);
			word = 0;
			fwrite(&word,sizeof(word),1,binfile);
			addr++;
		}
		addr++;
		word = 0;
		fwrite(&word,sizeof(word),1,binfile);
		word = ni->word1 >> 8;
		fwrite(&word,sizeof(word),1,binfile);
		word = (ni->word1 << 8) + (ni->word2 >> 8);
		fwrite(&word,sizeof(word),1,binfile);
		word = (ni->word2 << 8) + (ni->word3 >> 8);
		fwrite(&word,sizeof(word),1,binfile);
		word = (ni->word3 << 8) + (ni->word4 >> 8);
		fwrite(&word,sizeof(word),1,binfile);
		word = (ni->word4 << 8) + (ni->word5 >> 8);
		fwrite(&word,sizeof(word),1,binfile);
	}
}

write_ramtbl()
{
	short i, word;

	for( i = 0; i < NINST; i++){
		word = ram[i].entry >> 6;
		fwrite(&word,sizeof(word),1,tblfile);
		word = (ram[i].entry << 10) + (ram[i].load << 6) + ram[i].func;
		fwrite(&word,sizeof(word),1,tblfile);
	}
}

output(bout_stream, tbl_stream)
FILE * bout_stream;
FILE * tbl_stream;
{
    /* there are three parts to the output writing -- the listing, the binary, and the ramtbl */
    alpha_out();
    if (bout_stream != 0) {
	binfile = bout_stream;
        write_binary();
        fclose( binfile );
    }
    if (tbl_stream != 0 ) {
	tblfile = tbl_stream;
	write_ramtbl();
        fclose( tblfile );
    }
}
