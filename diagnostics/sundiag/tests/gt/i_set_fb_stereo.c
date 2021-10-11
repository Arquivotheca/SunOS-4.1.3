#ifndef lint
static  char sccsid[] = "@(#)i_set_fb_stereo.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>

#define DELAY(t)	for (i = t ; i ; i--)

/**********************************************************************/
void
i_set_fb_stereo(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{
    unsigned int hoz[8];
    unsigned int ver[7];
    unsigned int *addr;
    int i;

    hoz[0] = 0x705f;
    hoz[1] = 0x00b5;
    hoz[2] = 0xc0bf;
    hoz[3] = 0x40cc;
    hoz[4] = 0x60d1;
    hoz[5] = 0x50d9;
    hoz[6] = 0x10f5;
    hoz[7] = 0x0;

    ver[0] = 0x02a7;
    ver[1] = 0x22aa;
    ver[2] = 0x32ae;
    ver[3] = 0x42ce;
    ver[4] = 0x12cf;
    ver[5] = 0x0;
    ver[6] = 0x0;


    addr = (unsigned int *)VIDEO_MODE0;
    *addr = 0x0;

    addr = (unsigned int *)VIDEO_MODE1;
    *addr = 0x2002;

    /* delay */
    DELAY(0xfffff);

    addr = (unsigned int *)SSA0;
    *addr = 0x0;

    addr = (unsigned int *)SSA1;
    *addr = 0x20000;

    addr = (unsigned int *)LIN_OS;
    *addr = 0x0;

    addr = (unsigned int *)HOZREG;
    for (i = 0 ; i < 8 ; i++) {
	*addr++ = hoz[i];
    }

    addr = (unsigned int *)VERREG;
    for (i = 0 ; i < 7 ; i++) {
	*addr++ = ver[i];
    }

    addr = (unsigned int *)VIDEO_MODE0;
    *addr = 0x0f;

    addr = (unsigned int *)STATE;
    *addr = 0x1;

    addr = (unsigned int *)STEREO;
    *addr = 0x3;

    addr = (unsigned int *)STATE;
    *addr = 0x0;

}

