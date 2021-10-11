#ifndef lint
static  char sccsid[] = "@(#)i_rendering_pipeline.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>

/* Debug mode */
/*
#define DEBUG		1
*/



#define MAXCOUNT	100000		/* max count waiting */
#define MAX_SAMPLING	0x2D672         /* for PBM's sampling */
#define REG(i)	i

/* The folowing checksums were acquired by experiments */
#define ADDRSUM		0x2E0A238E
#define DATASUM		0x579FF88A
#define DEPTHSUM	0x3FA3D872


/**********************************************************************/
void
i_rendering_pipeline(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{
    void chksum();
#ifdef DEBUG
    void hexasc();
#endif DEBUG

    unsigned int *pbm = (unsigned *)(F_PBM_CSR);
    unsigned int *addr = (unsigned *)(XYADDR);
    unsigned int *data = (unsigned *)(DATAADDR);
    unsigned int *depth = (unsigned *)(DEPTHADDR);
    unsigned int rdata;

    unsigned int addrsum = 0;
    unsigned int datasum = 0;
    unsigned int depthsum = 0;

    int k;
    int count;

#ifdef DEBUG
	char	string[9];
	char    nl[2];
	char	comma[3];

	nl[0] = '\n';
	nl[1] = '\0';
	comma[0] = ',';
	comma[1] = ' ';
	comma[2] = '\0';
#endif DEBUG


    /* loop to read output from the SI */
    for (k =  MAX_SAMPLING; k ; k--) {
	/* only when ready then read the data */
	count = MAXCOUNT;
	while (!(*pbm & 0x100) && count--);
	if (count <= 0) {
	    *pbm &= ~0x80;               /* reset SI loopback enable */
	    goto error_return;
	}
	rdata = *addr;
	(void)chksum(&addrsum, rdata & 0xfffffff);
	rdata = *data;
	(void)chksum(&datasum, rdata);
	rdata = *depth;
	(void)chksum(&depthsum, rdata & 0xffffff);
    }

    *pbm &= ~0x80;               /* reset SI loopback enable */

#ifdef DEBUG

    hexasc(addrsum, string);
    (void) (*fputs) (string);
    (void) (*fputs) (comma);
    hexasc(datasum, string);
    (void) (*fputs) (string);
    (void) (*fputs) (comma);
    hexasc(depthsum, string);
    (void) (*fputs) (string);
    (void) (*fputs) (nl);

#else DEBUG

    if (addrsum != ADDRSUM ||
	datasum != DATASUM ||
	depthsum != DEPTHSUM) {

	goto error_return;

    }

#endif

    umcbp->errorcode = 0;
    return;

error_return:
#ifdef DEBUG
    hexasc(k, string);
    (void) (*fputs) (string);
    (void) (*fputs) (nl);
#endif DEBUG

    umcbp->errorcode = -4;
    return;

}

/**********************************************************************/
void
chksum(sum, data)
/**********************************************************************/
unsigned int *sum;
unsigned int data;

{
    unsigned int oldsum;

    oldsum = *sum;
    *sum += data;
    if (*sum < oldsum) {
	(*sum)++;
    }
}

#ifdef DEBUG
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
#endif DEBUG
