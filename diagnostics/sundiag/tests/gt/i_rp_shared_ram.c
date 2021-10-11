#ifndef lint
static  char sccsid[] = "@(#)i_rp_shared_ram.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>

#define REG(i)	i
#define SU_SHARED_RAM_START	0xFF084000
#define SU_SHARED_RAM_END	0xFF087FFF
#define SU_SHARED_RAM_SIZE (SU_SHARED_RAM_END-SU_SHARED_RAM_START+1)


/**********************************************************************/
void
i_rp_shared_ram(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{

	void hexasc();

	register unsigned * volatile addr;
	register unsigned int wone;
	register unsigned int tmp;
	register unsigned int start = (unsigned int) SU_SHARED_RAM_START;
        register int i;
	unsigned int errdat;

	char string[14];
	char nl[2];
	char comma[3];
	nl[0] = '\n';
	nl[1] = '\0';
	comma[0] = ',';
	comma[1] = ' ';
	comma[2] = '\0';
	char txtdat[12];
	char addrtxt[5];
	char expect[4];
	char actual[4];

	txtdat[0] = 'S';txtdat[1] = 'U';txtdat[2] = 'r';txtdat[3] = 'a';
	txtdat[4] = 'm';txtdat[5] = '-';txtdat[6] = 'E';txtdat[7] = 'r';
	txtdat[8] = 'r';txtdat[9] = ':';txtdat[10] = ' ';txtdat[11] = '\0';
	addrtxt[0] = 'A';addrtxt[1] = 'd';addrtxt[2] = 'd';addrtxt[3] = 'r';
	addrtxt[4] = '\0';
	expect[0] = 'E';expect[1] = 'x';expect[2] = 'p';expect[3] = '\0';
	actual[0] = 'A';actual[1] = 'c';actual[2] = 't';actual[3] = '\0';


	for (wone = 1 ; wone != 0 ; wone <<= 1) {
	    for (addr = (unsigned int *)start, i=SU_SHARED_RAM_SIZE/sizeof(int); i ; addr++, i--){

		tmp = *addr;
		*addr = wone;
		if (*addr != wone) {
		    errdat = *addr;
		    *addr = tmp;
		    goto error;
		}
		*addr = tmp;
	    }
	}

	/* clear error flag */
	umcbp->errorcode = 0;
	return;

    error:

	(void) (*fputs) (txtdat);
	(void) (*fputs) (addrtxt);
	hexasc((unsigned int)(addr), string);
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

    str[0] = ' ';
    str[1] = '=';
    str[2] = ' ';
    str[3] = '0';
    str[4] = 'x';
    for (i=0 ; i < 8 ; i++) {
	c = (char)(hex & 0xf);
	str[12-i] = (c < 0xa)? c + '0' : (c-0xa) + 'A';
	hex >>= 4;
    }
    str[i+5] = '\0';
    return;
}
