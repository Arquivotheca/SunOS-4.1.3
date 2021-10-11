
#ifndef lint
static  char sccsid[] = "@(#)i_clut.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hgpfe.h>
#include <i860.h>
#include <hdwr_regs.h>

/*
#define DEBUG		1
*/

#define REG(i)	i
#define CLUT_DATA_MASK	0xffffff
#define CLUT_LENGTH	8192
#define DELAY_ZERO	0x1000
#define DELAY_ONE	0x100000

/**********************************************************************/
void
i_clut(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */


{

    void hexasc();

    void shadow2mem();
    void mem2shadow();
    void fill_shadow();
    int memtest();
    int verify_shadow();
    int shadow2bank();
    int bank2shadow();

    register unsigned int *start;
    register unsigned int *addr;
    register unsigned int wone;
    RESULT result;
    unsigned int res;
    unsigned int save[CLUT_LENGTH];
    int bank=-1;

    char string[14];
    char nl[2];
    char comma[3];
    nl[0] = '\n';
    nl[1] = '\0';
    comma[0] = ',';
    comma[1] = ' ';
    comma[2] = '\0';
    char txtdat[11];
    char addrtxt[5];
    char expect[4];
    char actual[4];
    char banktxt[5];

    txtdat[0] = 'C';txtdat[1] = 'l';txtdat[2] = 'u';txtdat[3] = 't';
    txtdat[4] = '-';txtdat[5] = 'E';txtdat[6] = 'r';txtdat[7] = 'r';
    txtdat[8] = ':';txtdat[9] = ' ';txtdat[10] = '\0';
    addrtxt[0] = 'A';addrtxt[1] = 'd';addrtxt[2] = 'd';addrtxt[3] = 'r';
    addrtxt[4] = '\0';
    expect[0] = 'E'; expect[1] = 'x'; expect[2] = 'p'; expect[3] = '\0';
    actual[0] = 'A'; actual[1] = 'c'; actual[2] = 't'; actual[3] = '\0';
    banktxt[0] = 'B'; banktxt[1] = 'a'; banktxt[2] = 'n';
    banktxt[3] = 'k'; banktxt[4] = '\0';

#ifdef DEBUG
    hexasc(1, string);
    (void) (*fputs) (string);
    (void) (*fputs) (nl);
#endif DEBUG

    /* test shadow RAM */
    start = (unsigned int *) FBD_CLUT_0;
    res = memtest(start, CLUT_LENGTH, &result);
    if (res) {
	goto error_return;
    }

#ifdef DEBUG
    hexasc(2, string);
    (void) (*fputs) (string);
    (void) (*fputs) (nl);
#endif DEBUG

    /* loop to test 5 banks */
    for (bank = 0 ; bank < 5 ; bank++) {
	/* save wlut bank */
#ifdef DEBUG
	res = bank2shadow(bank, fputs);
#else !DEBUG
	res = bank2shadow(bank);
#endif DEBUG
	if (res) {
	    goto error_return;
	}
	(void)shadow2mem(save);

	for (wone = 1 ; wone != (1<<24) ; wone <<= 1) {
	    (void)fill_shadow(wone);
#ifdef DEBUG
	    res = shadow2bank(bank, fputs);
#else !DEBUG
	    res = shadow2bank(bank);
#endif DEBUG
	    if (res) {
		goto error_return;
	    }
#ifdef DEBUG
	    res = bank2shadow(bank, fputs);
#else !DEBUG
	    res = bank2shadow(bank);
#endif DEBUG
	    if (res) {
		goto error_return;
	    }
#ifdef DEBUG
	    res = verify_shadow(wone, &result, fputs);
#else !DEBUG
	    res = verify_shadow(wone, &result);
#endif DEBUG
	    if (res) {
		/* restore wlut bank */
		(void)mem2shadow(save);
#ifdef DEBUG
		(void)shadow2bank(bank, fputs);
#else !DEBUG
		(void)shadow2bank(bank);
#endif DEBUG
		goto error_return;
	    }
	}
	/* restore wlut bank */
	(void)mem2shadow(save);
#ifdef DEBUG
	res = shadow2bank(bank, fputs);
#else !DEBUG
	res = shadow2bank(bank);
#endif DEBUG
	if (res) {
	    goto error_return;
	}
    }

    /* clear error flag */
    umcbp->errorcode = 0;
    return;

error_return:

    (void) (*fputs) (txtdat);
    (void) (*fputs) (addrtxt);
    hexasc((unsigned int)result.addr, string);
    (void) (*fputs) (string);
    (void) (*fputs) (comma);
    (void) (*fputs) (expect);
    hexasc(result.expect, string);
    (void) (*fputs) (string);
    (void) (*fputs) (comma);
    (void) (*fputs) (actual);
    hexasc(result.actual, string);
    (void) (*fputs) (string);
    if (bank >= 0) {
	(void) (*fputs) (comma);
	(void) (*fputs) (banktxt);
	hexasc(bank, string);
	(void) (*fputs) (string);
    }
    (void) (*fputs) (nl);

    /* clear error flag */
    if (res) {
	umcbp->errorcode = res;
    } else {
	umcbp->errorcode = -1;
    }

    return;

}

/**********************************************************************/
void
shadow2mem(s)
/**********************************************************************/
unsigned int s[];

{
    register int i;
    register unsigned int *addr;

    for (i = 0, addr = (unsigned int *) FBD_CLUT_0 ;
				i < CLUT_LENGTH ; i++, addr++) {
	s[i] = (*addr) & CLUT_DATA_MASK;
    }
}

/**********************************************************************/
void
mem2shadow(s)
/**********************************************************************/
unsigned int s[];

{
    register int i;
    register unsigned * volatile addr;

    for (i = 0, addr = (unsigned int *) FBD_CLUT_0 ;
				i < CLUT_LENGTH ; i++, addr++) {
	*addr = s[i] & CLUT_DATA_MASK;
    }
}

/**********************************************************************/
void
fill_shadow(d)
/**********************************************************************/
unsigned int d;

{
    register int i;
    register unsigned * volatile addr;

    for (i = 0, addr = (unsigned int *) FBD_CLUT_0 ;
				i < CLUT_LENGTH ; i++, addr++) {
	*addr = d & CLUT_DATA_MASK;
    }
}
#ifdef DEBUG
/**********************************************************************/
int
verify_shadow(d, res, putstr)
/**********************************************************************/
unsigned int d;
RESULT *res;
void (*putstr)();
#else !DEBUG
/**********************************************************************/
int
verify_shadow(d, res)
/**********************************************************************/
RESULT *res;
unsigned int d;
#endif DEBUG
{
#ifdef DEBUG
    void hexasc();
#endif DEBUG

    register int i;
    register unsigned * volatile addr;

#ifdef DEBUG
	char	string[14];
	char    nl[2];
	char	comma[3];

	nl[0] = '\n';
	nl[1] = '\0';
	comma[0] = ',';
	comma[1] = ' ';
	comma[2] = '\0';
#endif DEBUG


    d &= CLUT_DATA_MASK;

    for (i = 0, addr = (unsigned int *) FBD_CLUT_0 ;
				i < CLUT_LENGTH ; i++, addr++) {

	if ((*addr & CLUT_DATA_MASK) != d) {

#ifdef DEBUG
	    hexasc((unsigned int)addr-(unsigned int)FBD_CLUT_0, string);
	    (void) (*putstr) (string);
	    (void) (*putstr) (comma);
	    hexasc((*addr) & CLUT_DATA_MASK, string);
	    (void) (*putstr) (string);
	    (void) (*putstr) (comma);
	    hexasc(d, string);
	    (void) (*putstr) (string);
	    (void) (*putstr) (nl);
#endif DEBUG

	    res->addr = addr;
	    res->expect = d;
	    res->actual = *addr & CLUT_DATA_MASK;
	    return -2;
	}
    }

    return 0;
}

#ifdef DEBUG
/**********************************************************************/
int
shadow2bank(b, putstr)
/**********************************************************************/
int b;
void (*putstr)();
#else !DEBUG
/**********************************************************************/
int
shadow2bank(b)
/**********************************************************************/
int b;
#endif DEBUG

{
    register unsigned int cmd;
    register unsigned int csr0;
    register unsigned int csr1;
    register unsigned int mask;
    register unsigned int *csr = (unsigned int *)F_PBM_CSR;
    register unsigned int *wlut = (unsigned int *)FBD_CLUT_CSR0;
    register int j;
    register int k;

#ifdef DEBUG
	char	string[14];
	char    nl[2];
	char	comma[3];

	nl[0] = '\n';
	nl[1] = '\0';
	comma[0] = ',';
	comma[1] = ' ';
	comma[2] = '\0';
#endif DEBUG


    cmd = (b << 4) + 1;
    mask = cmd & 0x3;

    /* copy lower shadow to CLUT bank */
    *wlut = cmd;

     j = DELAY_ZERO;
     k = DELAY_ONE;
     do {
        csr0 = *wlut++;        /* get CSR0 data */
        csr1 = *wlut--;        /* get CSR1 data */
	if (*csr & 0x2) {
	    j--;
	} else {
	    k--;
	}
     } while ((csr0 & mask) || (csr1 & mask) && j && k);

     if (j <= 0 || k <= 0) { /* time out */
	return -3;
    }

    /* set upper bit, then copy upper shadow to CLUT bank */
    cmd |= 0x8;
    mask = cmd & 0x3;
    *wlut = cmd;

     j = DELAY_ZERO;
     k = DELAY_ONE;
     do {
        csr0 = *wlut++;        /* get CSR0 data */
        csr1 = *wlut--;        /* get CSR1 data */
	if (*csr & 0x2) {
	    j--;
	} else {
	    k--;
	}
     } while ((csr0 & mask) || (csr1 & mask) && j && k);

     if (j <= 0 || k <= 0) { /* time out */
	return -3;
    }

#ifdef DEBUG
    hexasc(j, string);
    (void) (*putstr) (string);
    (void) (*putstr) (comma);
    hexasc(k, string);
    (void) (*putstr) (string);
    (void) (*putstr) (nl);
#endif DEBUG


    return 0;

}

#ifdef DEBUG
/**********************************************************************/
int
bank2shadow(b, putstr)
/**********************************************************************/
int b;
void (*putstr)();
#else !DEBUG
/**********************************************************************/
int
bank2shadow(b)
/**********************************************************************/
int b;
#endif DEBUG

{
    register unsigned int cmd;
    register unsigned int csr0;
    register unsigned int csr1;
    register unsigned int mask;
    register unsigned int *csr = (unsigned int *)F_PBM_CSR;
    register unsigned int *wlut = (unsigned int *)FBD_CLUT_CSR0;
    register int j;
    register int k;

#ifdef DEBUG
	char	string[14];
	char    nl[2];
	char	comma[3];

	nl[0] = '\n';
	nl[1] = '\0';
	comma[0] = ',';
	comma[1] = ' ';
	comma[2] = '\0';
#endif DEBUG

    /* copy CLUT bank to lower shadow */
    cmd = (b << 4) + 5;
    mask = cmd & 0x3;
    *wlut = cmd;

     j = DELAY_ZERO;
     k = DELAY_ONE;
     do {
        csr0 = *wlut++;        /* get CSR0 data */
        csr1 = *wlut--;        /* get CSR1 data */
	if (*csr & 0x2) {
	    j--;
	} else {
	    k--;
	}
     } while ((csr0 & mask) || (csr1 & mask) && j && k);

     if (j <= 0 || k <= 0) { /* time out */
	return -3;
    }

    /* set upper bit, then copy CLUT bank to upper shadow */
    cmd += 0x8;
    mask = cmd & 0x3;
    *wlut = cmd;

     j = DELAY_ZERO;
     k = DELAY_ONE;
     do {
        csr0 = *wlut++;        /* get CSR0 data */
        csr1 = *wlut--;        /* get CSR1 data */
	if (*csr & 0x2) {
	    j--;
	} else {
	    k--;
	}
     } while ((csr0 & mask) || (csr1 & mask) && j && k);

     if (j <= 0 || k <= 0) { /* time out */
	return -3;
    }

#ifdef DEBUG
    hexasc(j, string);
    (void) (*putstr) (string);
    (void) (*putstr) (comma);
    hexasc(k, string);
    (void) (*putstr) (string);
    (void) (*putstr) (nl);
#endif DEBUG

    return 0;

}

/**********************************************************************/
int
memtest(start, length, res)
/**********************************************************************/
register unsigned int *start;
register unsigned int length;
register RESULT *res;

{

	register unsigned * volatile addr;
	register unsigned int wone;
        register int i;

	for (wone = 1 ; wone != (1<<24) ; wone <<= 1) {
	    for (addr = (unsigned int *)start, i=length; i ; addr++, i--){

		*addr = wone;
		if ((*addr & CLUT_DATA_MASK) != wone) {
		    goto error;
		}
	    }
	}

	return 0;

    error:
	res->addr = addr;
	res->expect = wone;
	res->actual = *addr & CLUT_DATA_MASK;
	return -1;
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
