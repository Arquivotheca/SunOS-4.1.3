#ifndef lint
static  char sccsid[] = "@(#)i_restore_fb_mode.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>

#define DELAY(t)	for (i = t ; i ; i--)


/**********************************************************************/
void
i_restore_fb_mode(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{

    unsigned int *value;
    unsigned int *addr;
    unsigned int *sync;
    int i;

    value = (unsigned int *)ctxp->risc_regs[4];

    /* turn off sync */
    addr = (unsigned int *)VIDEO_MODE0;
    *addr &= ~0x1;
    /* delay */
    DELAY(0xfffff);

    /* advance pass value of video_mode 0 */
    value++;

    addr = (unsigned int *)VIDEO_MODE1;
    *addr = *value++;
    addr = (unsigned int *)SSA0;
    *addr = *value++;
    addr = (unsigned int *)SSA1;
    *addr = *value++;
    addr = (unsigned int *)LIN_OS;
    *addr = *value++;

    addr = (unsigned int *)HOZREG;
    for (i = 0 ; i < 8 ; i++) {
	*addr++ = *value++;
    }

    addr = (unsigned int *)VERREG;
    for (i = 0 ; i < 7 ; i++) {
	*addr++ = *value++;
    }

    /* restore video mode 0 reg */
    sync = (unsigned int *)ctxp->risc_regs[4];
    addr = (unsigned int *)VIDEO_MODE0;
    *addr = *sync;

    /* advance pass the state reg */
    value++;

    /* set state to 1 */
    addr = (unsigned int *)STATE;
    *addr = 1;

    addr = (unsigned int *)STEREO;
    *addr = *value--;

    /* restore state reg */
    addr = (unsigned int *)STATE;
    *addr = *value;

}

