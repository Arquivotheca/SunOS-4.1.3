#ifndef lint
static  char sccsid[] = "@(#)i_save_fb_mode.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>

#define DELAY(t)	for (i = t ; i ; i--)

/**********************************************************************/
void
i_save_fb_mode(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{
    unsigned int *value;
    unsigned int *addr;
    int i;

    value = (unsigned int *)ctxp->risc_regs[4];

    /* save VIDEO_MODE0 reg first */
    addr = (unsigned int *)VIDEO_MODE0;
    *value++ = *addr;

    /* turn off sync */
    addr = (unsigned int *)VIDEO_MODE0;
    *addr &= ~0x1;
    /* delay */
    DELAY(0xfffff);

    addr = (unsigned int *)VIDEO_MODE1;
    *value++ = *addr;
    addr = (unsigned int *)SSA0;
    *value++ = *addr;
    addr = (unsigned int *)SSA1;
    *value++ = *addr;
    addr = (unsigned int *)LIN_OS;
    *value++ = *addr;

    addr = (unsigned int *)HOZREG;
    for (i = 0 ; i < 8 ; i++) {
	*value++ = *addr++;
    }

    addr = (unsigned int *)VERREG;
    for (i = 0 ; i < 7 ; i++) {
	*value++ = *addr++;
    }

    /* turn on sync */
    addr = (unsigned int *)VIDEO_MODE0;
    *addr |= 0x1;

    /* save state reg */
    addr = (unsigned int *)STATE;
    *value++ = *addr;

    /* set state to 1 */
    addr = (unsigned int *)STATE;
    *addr = 1;

    /* save mode */
    addr = (unsigned int *)STEREO;
    *value-- = *addr;

    /* restore state */
    addr = (unsigned int *)STATE;
    *addr = *value++;

}
