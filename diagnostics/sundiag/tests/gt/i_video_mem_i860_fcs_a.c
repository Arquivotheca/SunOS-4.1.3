
#ifndef lint
static  char sccsid[] = "@(#)i_video_mem_i860_fcs_a.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>


/**********************************************************************/
void
i_video_mem_i860_fcs_a(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{

	register unsigned * volatile reg;
	register unsigned * volatile addr;
	register unsigned int wr_data, rd_data;
        register int x;
        register int y;
        register int i;
	char txdat[11];
	char xdat[2];
	char ydat[2];
	char bdat[5];
	char actdat[4];
	char expdat[4];
	char string[14];
	char comma[3];
	char nl[2];

	REGS

	SAVE_REGS

	/* initialize text strings */

	txdat[0] = 'F';txdat[1] = 'c';txdat[2] = 's';txdat[3] = 'A';
	txdat[4] = '-';txdat[5] = 'E';txdat[6] = 'r';txdat[7] = 'r';
	txdat[8] = ':';txdat[9] = ' ';txdat[10] = '\0';
	comma[0] = ',';comma[1] = ' ';comma[2] = '\0';
	nl[0] = '\n';nl[1] = '\0';

	xdat[0] = 'x';xdat[1] = '\0';
	ydat[0] = 'y';ydat[1] = '\0';
	bdat[0] = 'b';bdat[1] = 'a';bdat[2] = 'n';bdat[3] = 'k';
	bdat[4] = '\0';

	actdat[0] = 'A';actdat[1] = 'c';actdat[2] = 't';actdat[3] = '\0';
	expdat[0] = 'E';expdat[1] = 'x';expdat[2] = 'p';expdat[3] = '\0';

	/* initialize pp registers */

        reg = (unsigned *)(F_PBM_CSR);
        *reg = 1;               /* set FE state set to one */
        reg = (unsigned *)(FBD_VCXR);
        *reg = 0x7ff;           /* no x view clipping */
        reg = (unsigned *)(FBD_VCYR);
        *reg = 0x7ff;           /* no y view clipping */
        reg = (unsigned *)(FBD_WID_CUR);
        *reg = 0;               /* wid zero */
        reg = (unsigned *)(FBD_WID_CTRL);
        *reg = 0;               /* no wid clip or replace */
        reg = (unsigned *)(FBD_CONST_Z_SRC);
        *reg = 0;               /* constant Z = zero */
        reg = (unsigned *)(FBD_Z_CTRL);
        *reg = 4;               /* enable write Z */
        reg = (unsigned *)(FBD_IWMR);
        *reg = 0xffffffff;              /* RGBA write mask enable */
        reg = (unsigned *)(FBD_WWMR);
        *reg = 0x00001fff;              /* window write mask enable */
        reg = (unsigned *)(FBD_FC_SET);
        *reg = 0xfc;            /* disable FC */
        reg = (unsigned *)(FBD_ROP_OP);
        *reg = 0x0000c;         /* set DST = SRC */
        reg = (unsigned *)(F_PBM_ASR);
        *reg = 6;        /* set FB Addr Select (ASR) reg = 4,5 or 6 */
        reg = (unsigned *)(BUF_SR);
        *reg = 1;    /* set FB buffer select reg = 1 or 6 */


	/* write all memory with (0x50000) data pattern */

	wr_data = 0x50c00; /* for visual affect, turn cursor plane on */
	for (y = 0; y < 1024; y++) {
	   addr = (unsigned int *)(VIDEO_MEM_START + (y << 13));
	   for ( x = 0; x < 1280; x++) {	
	      *addr++ = wr_data;
	   }
	}

	/* verify all memory for (0x50000) data pattern */

	wr_data = 0x50000;
	for (y = 0; y < 1024; y++) {
	   addr = (unsigned int *)(VIDEO_MEM_START + (y << 13));
	   for ( x = 0; x < 1280; x++) {	
	      rd_data = *addr;
	      rd_data &= 0xf0000;
	      if (rd_data != wr_data)
		 goto error;
	      *addr++ = 0xa0000;	/* write back complemented pattern */
	   }
	}

	/* verify all memory with (complemented) pattern */

	wr_data = 0xA0000;
	for (y = 0; y < 1024; y++) {
	   addr = (unsigned int *)(VIDEO_MEM_START + (y << 13));
	   for ( x = 0; x < 1280; x++) {	
	      rd_data = *addr;
	      rd_data &= 0xf0000;
	      if (rd_data != wr_data)
		 goto error;
	      *addr++ = 0;	/* write back to zero */
	   }
	}

	umcbp->errorcode = 0;

	RESTORE_REGS

	return;

    error:
	(void) (*fputs) (txdat);
	(void) (*fputs) (xdat);
	hexasc(x, string);
	(void) (*fputs) (string);
	(void) (*fputs) (comma);
	(void) (*fputs) (ydat);
	hexasc(y, string);
	(void) (*fputs) (string);
	(void) (*fputs) (comma);
	(void) (*fputs) (bdat);
	i = getbank(x);
	hexasc(i, string);
	(void) (*fputs) (string);
	(void) (*fputs) (comma);
	(void) (*fputs) (expdat);
	hexasc((wr_data), string);
	(void) (*fputs) (string);
	(void) (*fputs) (comma);
	(void) (*fputs) (actdat);
	hexasc((rd_data), string);
	(void) (*fputs) (string);
	(void) (*fputs) (nl);

	umcbp->errorcode = -1;

	RESTORE_REGS

	return;
}

/**********************************************************************/
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

/**********************************************************************/
getbank(x)
/**********************************************************************/
unsigned int x;
{

    while (x > 4) {
	x -= 5;
    }
    return(x);
}

