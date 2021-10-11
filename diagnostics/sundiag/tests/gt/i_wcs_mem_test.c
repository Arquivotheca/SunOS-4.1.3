#ifndef lint
static  char sccsid[] = "@(#)i_wcs_mem_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/***********************************************************************

    Non-destructive memory test. The entire WCS is tested with
    the walking one pattern.
    D-cache is flushed and disabled and all interrupt is disabled
    for the duration of the test.

 **********************************************************************/

#include <fe_includes.h>
#include <hgpfe.h>
#include <i860.h>

/*
#define DEBUG	1
*/
#define REG(i)	i

/**********************************************************************/
void
ldm_mem(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{

	unsigned int	interrupt_disable();
	unsigned int	D_cache_disable();
	void		interrupt_enable();
	void		D_cache_enable();

	void		hexasc();

	register unsigned int save_psr;
	register unsigned int save_dirbase;
	register unsigned int *addr;
	register unsigned int wone;
	register unsigned int tmp;
	register unsigned int wcs_start = (unsigned int) WCS_START;
        register unsigned int i;
	unsigned int errdat;

	char string[9];
	char nl[2];
	char comma[3];
	nl[0] = '\n';
	nl[1] = '\0';
	comma[0] = ',';
	comma[1] = ' ';
	comma[2] = '\0';
	char addrtxt[7];
	char expect[9];
	char actual[9];
	char banktxt[7];

	addrtxt[0] = 'A';addrtxt[1] = 'd';addrtxt[2] = 'd';addrtxt[3] = 'r';
	addrtxt[4] = '=';addrtxt[5] = ' ';addrtxt[6] = '\0';
	expect[0] = 'E'; expect[1] = 'x'; expect[2] = 'p'; expect[3] = 'e';
	expect[4] = 'c'; expect[5] = 't'; expect[6] = '='; expect[7] = ' ';
	expect[8] = '\0';
	actual[0] = 'A'; actual[1] = 'c'; actual[2] = 't'; actual[3] = 'u';
	actual[4] = 'a'; actual[5] = 'l'; actual[6] = '='; actual[7] = ' ';
	actual[8] = '\0';
	banktxt[0] = 'B'; banktxt[1] = 'a'; banktxt[2] = 'n';
	banktxt[3] = 'k'; banktxt[4] = '='; banktxt[5] = ' ';
	banktxt[6] = '\0';

#ifdef DEBUG
	string[0] = 'S';
	string[1] = 'T';
	string[2] = 'A';
	string[3] = 'R';
	string[4] = 'T';
	string[5] = '\0';
	(void) (*fputs) (string);
	(void) (*fputs) (nl);
#endif DEBUG

	/* Turn off D-cache and disable interrupts here */
	/* save the current content of the psr  and dirbase */
	save_psr = interrupt_disable();
	save_dirbase = D_cache_disable();

#ifdef DEBUG
		hexasc(save_psr, string);
		(void) (*fputs) (string);
		(void) (*fputs) (nl);
		hexasc(save_dirbase, string);
		(void) (*fputs) (string);
		(void) (*fputs) (nl);
#endif DEBUG

	for (wone = 1 ; wone != 0 ; wone <<= 1) {
	    for (addr = (unsigned int *)wcs_start, i=WCS_SIZE/sizeof(int); i ; addr++, i--){

		tmp = *addr;
		*addr = wone;


		/**************** error insertion ****************/
		/*
		if (i == 0x1ffff) {
		    hexasc((unsigned int)addr, string);
		    (void) (*fputs) (string);
		    (void) (*fputs) (string);
		    (void) (*fputs) (comma);
		    hexasc(*addr, string);
		    (void) (*fputs) (string);
		    (void) (*fputs) (nl);

		    *addr |= 0x40;

		    hexasc((unsigned int)addr, string);
		    (void) (*fputs) (string);
		    (void) (*fputs) (comma);
		    hexasc(*addr, string);
		    (void) (*fputs) (string);
		    (void) (*fputs) (nl);
		}
		*/
		/***********************************************/

		if (*addr != wone) {
		    errdat = *addr;
		    *addr = tmp;
		    goto error;
		}
		*addr = tmp;
	    }
	}

	/* Restore D-cache and interrupts here */
	D_cache_enable(save_dirbase);
	interrupt_enable(save_psr);

	/* clear error flag */
	umcbp->errorcode = 0;
	return;

    error:
	/* Restore D-cache and interrupts here */
	D_cache_enable(save_dirbase);
	interrupt_enable(save_psr);

	(void) (*fputs) (addrtxt);
	hexasc((unsigned int)addr, string);
	(void) (*fputs) (string);
	(void) (*fputs) (comma);
	(void) (*fputs) (expect);
	hexasc(wone, string);
	(void) (*fputs) (string);
	(void) (*fputs) (comma);
	(void) (*fputs) (actual);
	hexasc(errdat, string);
	(void) (*fputs) (string);
	(void) (*fputs) (nl);

	/* set error flag */
	umcbp->errorcode = -1;
	return;
}

/**********************************************************************/
void
hexasc(hex, str)
/**********************************************************************/
unsigned int hex;
char *str;
{
    unsigned char c;
    int i;

    for (i=0 ; i < 8 ; i++) {
	c = (char)(hex & 0xf);
	str[7-i] = (c < 0xa)? c + '0' : (c-0xa) + 'A';
	hex >>= 4;
    }
    str[8] = '\0';
    return;
}
