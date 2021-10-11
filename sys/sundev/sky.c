#ifndef lint
static	char sccsid[] = "@(#)sky.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 *  Sky FFP
 */
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <sundev/mbvar.h>
#include <sundev/skyreg.h>

/*
 * "page" size for VME sky board
 * user page (0) doesn't allow access to nasty registers
 * supervisor page (1) does
 */
#define	SKYPGSIZE	0x800

/*
 * Driver information for auto-configuration stuff.
 */
int	skyprobe(), skyattach(), skyintr();
struct	mb_device *skyinfo[1];	/* XXX only supports 1 board */
struct	mb_driver skydriver = {
	skyprobe, 0, skyattach, 0, 0, skyintr,
	2 * SKYPGSIZE, "sky", skyinfo, 0, 0, 0,
};

struct	skyreg *skyaddr;
int skyinit = 0, skyisnew = 0;

/*ARGSUSED*/
skyprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register struct skyreg *skybase = (struct skyreg *)reg;

	if (peek((short *)skybase) == -1)
		return (0);
	if (poke((short *)&skybase->sky_status, SKY_IHALT)) 
		return (0);
	skyaddr = (struct skyreg *)(SKYPGSIZE + reg);
	if (cpu == CPU_SUN2_120 ||
	    poke((short *)&skyaddr->sky_status, SKY_IHALT)) {
		/* old VMEbus or Multibus */
		skyisnew = 0;
		skyaddr = (struct skyreg *)reg;
	} else
		skyisnew = 1;
	return (sizeof (struct skyreg));
}

/*
 * Initialize the VME interrupt vector to be identical to
 * the 68000 auto-vector for the appropriate interrupt level
 * unless vectored interrupts have been specified.
 */
skyattach(md)
	struct mb_device *md;
{

	if (skyisnew) {
		if (!md->md_intr) {
			/* use auto-vectoring */
			(void) poke((short *)&skyaddr->sky_vector,
			    AUTOBASE + md->md_intpri);
		} else {
			/* use vectored interrupts */
			(void) poke((short *)&skyaddr->sky_vector,
			    md->md_intr->v_vec);
		}
	}
}

/*ARGSUSED*/
skyopen(dev, flag)
	dev_t dev;
	int flag;
{
	int i;
	register struct skyreg *s = skyaddr;

	if (skyaddr == 0)
		return (ENXIO);
	if (skyinit == 2) {
		/*
		 * Initialize the FFP.
		 * VME users can't do this themselves;
		 * since the status isn't writeable
		 */
		s->sky_status = SKY_RESET;
		s->sky_command = SKY_START0;
		s->sky_command = SKY_START0;
		s->sky_command = SKY_START1;
		s->sky_status = SKY_RUNENB;
		u.u_pcb.u_skyctx.usc_used = 1;
		u.u_pcb.u_skyctx.usc_cmd = SKY_NOP;
		for (i=0; i<8; i++)
			u.u_pcb.u_skyctx.usc_regs[i] = 0;
		skyrestore();
	} else if (flag & (FNDELAY|FNBIO|FNONBIO))
		skyinit = 1;
	else
		return (ENXIO);
	return (0);
}

/*ARGSUSED*/
skyclose(dev, flag)
	dev_t dev;
	int flag;
{

	/*
	 * We have to save context here in case the user aborted
	 * and left the board in an unclean state.
	 */
	if (skyinit == 2)
		skysave();
	if (skyinit == 1)
		skyinit = 2;
	u.u_pcb.u_skyctx.usc_used = 0;
	return (0);
}

/*ARGSUSED*/
skymmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	struct pte pte;

	if (off)
		return (-1);
	off = (off_t)skyaddr;
	if (skyisnew && skyinit == 2)	/* use user page */
		off -= SKYPGSIZE;
	mmu_getkpte((addr_t)off, &pte);
	return (MAKE_PFNUM(&pte));
}

/*ARGSUSED*/
skyintr(n)
	int n;
{
        static u_short  skybooboo = 0;

        if (skyaddr && (skyaddr->sky_status & (SKY_INTENB|SKY_INTRPT))) {
                if (skyaddr->sky_status & SKY_INTENB) {
			printf("skyintr: sky board interrupt enabled, status = 0x%x\n",
                                skyaddr->sky_status);
                        skyaddr->sky_status &= ~(SKY_INTENB|SKY_INTRPT);
                        return (1);
                }
                if (!skybooboo && (skyaddr->sky_status & SKY_INTRPT)) {
			printf("skyintr: sky board unrecognized status, status = 0x%x\n",
                                skybooboo = skyaddr->sky_status);
                        return (0);
                }
	}
	return (0);
}

skysave()
{
	register short i;
	register struct skyreg *s = skyaddr;
	register u_short stat;

	for (i = 0; i < 100; i++) {
		stat = s->sky_status;
		if (stat & SKY_IDLE) {
			u.u_pcb.u_skyctx.usc_cmd = SKY_NOP;
			goto sky_save;
		}
		if (stat & SKY_IORDY)
			goto sky_ioready;
	}
	printf("sky0: hung\n");
	skyinit = 0;
	u.u_pcb.u_skyctx.usc_used = 0;
	return;

	/*
	 * I/O is ready, is it a read or write?
	 */
sky_ioready:
	s->sky_status = SKY_SNGRUN;	/* set single step mode */
	if (stat & SKY_IODIR)
		i = s->sky_d1reg;
	else
		s->sky_d1reg = i;

	/*
	 * Check again since data may have been a long word.
	 */
	stat = s->sky_status;
	if (stat & SKY_IORDY)
		if (stat & SKY_IODIR)
			i = s->sky_d1reg;
		else
			s->sky_d1reg = i;

	/*
	 * Read and save the command register.
	 * Decrement by 1 since command register
	 * is actually FFP program counter and we
	 * want to back it up.
	 */
	u.u_pcb.u_skyctx.usc_cmd = s->sky_command - 1;

	/*
	 * Reinitialize the FFP.
	 */
	s->sky_status = SKY_RESET;
	s->sky_command = SKY_START0;
	s->sky_command = SKY_START0;
	s->sky_command = SKY_START1;
	s->sky_status = SKY_RUNENB;

	/*
	 * Finally, actually do the context save function.
	 * (Unrolled loop for efficiency.)
	 */
sky_save:
	s->sky_command = SKY_NOP;	/* set FFP in a clean mode */
	s->sky_command = SKY_SAVE;
	u.u_pcb.u_skyctx.usc_regs[0] = s->sky_data;
	u.u_pcb.u_skyctx.usc_regs[1] = s->sky_data;
	u.u_pcb.u_skyctx.usc_regs[2] = s->sky_data;
	u.u_pcb.u_skyctx.usc_regs[3] = s->sky_data;
	u.u_pcb.u_skyctx.usc_regs[4] = s->sky_data;
	u.u_pcb.u_skyctx.usc_regs[5] = s->sky_data;
	u.u_pcb.u_skyctx.usc_regs[6] = s->sky_data;
	u.u_pcb.u_skyctx.usc_regs[7] = s->sky_data;
}

skyrestore()
{
	register struct skyreg *s = skyaddr;

	if (skyinit != 2) {
		u.u_pcb.u_skyctx.usc_used = 0;
		return;
	}
	s->sky_command = SKY_NOP;	/* set FFP in a clean mode */

	/*
	 * Do the context restore function.
	 */
	s->sky_command = SKY_RESTOR;
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[0];
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[1];
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[2];
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[3];
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[4];
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[5];
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[6];
	s->sky_data = u.u_pcb.u_skyctx.usc_regs[7];
	s->sky_command = u.u_pcb.u_skyctx.usc_cmd;
}

/*
 * special ioctl to allow user to do a save in case of signal handling
 * where no context switch occured. 
 */
/*ARGSUSED*/
skyioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	int i;
	register struct skyreg *s = skyaddr;
	register u_short stat;

	/*
	 * I/O is ready, is it a read or write?
	 */
	s->sky_status = SKY_SNGRUN;	/* set single step mode */
	stat = s->sky_status;
	if (stat & SKY_IORDY)
		if (stat & SKY_IODIR)
			i = s->sky_d1reg;
		else
			s->sky_d1reg = i;

	/*
	 * Check again since data may have been a long word.
	 */
	stat = s->sky_status;
	if (stat & SKY_IORDY)
		if (stat & SKY_IODIR)
			i = s->sky_d1reg;
		else
			s->sky_d1reg = i;

	/*
	 * Read and save the command register.
	 * Decrement by 1 since command register
	 * is actually FFP program counter and we
	 * want to back it up.
	 */
	*(u_short *)data = s->sky_command - 1;

	/*
	 * Reinitialize the FFP.
	 */
	s->sky_status = SKY_RESET;
	s->sky_command = SKY_START0;
	s->sky_command = SKY_START0;
	s->sky_command = SKY_START1;
	s->sky_status = SKY_RUNENB;

	return(0);
}
