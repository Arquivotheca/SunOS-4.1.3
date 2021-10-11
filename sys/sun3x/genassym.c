#ifndef lint
static	char sccsid[] = "@(#)genassym.c 1.1 92/07/30";
#endif
/* syncd to sun3/genassym.c 1.37 */
/* syncd to sun3/genassym.c 1.46 */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/vmmeter.h>
#include <sys/vmparam.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/vmmac.h>

#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <machine/clock.h>
#include <machine/memerr.h>
#include <machine/interreg.h>
#include <machine/eeprom.h>
#include <machine/eccreg.h>
#include <machine/pcb.h>


#ifdef LWP
#include <lwp/common.h>
#include <machlwp/lwpmachdep.h>
#include <lwp/queue.h>
#include <machlwp/machdep.h>
#include <lwp/lwperror.h>
#include <lwp/cntxt.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/alloc.h>
#include <lwp/condvar.h>
#include <lwp/monitor.h>
#endif LWP

#include <sundev/zscom.h>
#include "fpa.h"
#include <sundev/fpareg.h>

#include <vm/hat.h>
#include <vm/as.h>

struct zsops *zs_proto[] = { 0 };

main()
{
	register struct proc *p = (struct proc *)0;
	register struct vmmeter *vm = (struct vmmeter *)0;
	register struct regs *rp = (struct regs *)0;
	register struct user *up = (struct user *)0;
	register struct zscom *zs = (struct zscom *)0;
	register struct as *as = (struct as *)0;
#ifdef LWP
	register lwp_t *l = (lwp_t *)0;
#endif LWP
	struct fpa_device *fpap = (struct fpa_device *)0;
#ifdef LWP
	printf("#define REGOFFSET\t%d\n", l->lwp_context);
	printf("#ifndef\tMINSTACKSZ\n");
	printf("#define\tMINSTACKSZ\t%d\n",
		(STKOVERHEAD + (sizeof(stkalign_t) - 1)) / sizeof(stkalign_t));
	printf("#endif\tMINSTACKSZ\n");

	/* generate CHECK macro */
	printf("#define CHECK(location, result) {\t\t\t\\\n");
	printf("extern char *__CurStack;\t\t\t\t\\\n");
	printf("\tasm (\"movl ___CurStack, d0\");\t\t\t\\\n");
	printf("\tasm (\"addl #%d, d0\");\t\t\t\t\\\n", STKOVERHEAD);
	printf("\tasm (\"cmpl d0, sp\");\t\t\t\t\\\n");
	printf("\tasm (\"jhi 1f\");\t\t\t\t\t\\\n");
	printf("\tlocation = result;\t\t\t\t\\\n");
	printf("\tasm (\"unlk a6\");\t\t\t\t\\\n");
	printf("\tasm (\"rts\");\t\t\t\t\t\\\n");
	printf("\tasm (\"1:\");\t\t\t\t\t\\\n");
	printf("}\n");
#endif LWP
	printf("#define\tP_LINK 0x%x\n", &p->p_link);
	printf("#define\tP_UAREA 0x%x\n", &p->p_uarea);
	printf("#define\tP_RLINK 0x%x\n", &p->p_rlink);
	printf("#define\tP_PRI 0x%x\n", &p->p_pri);
	printf("#define\tP_STAT 0x%x\n", &p->p_stat);
	printf("#define\tP_WCHAN 0x%x\n", &p->p_wchan);
	printf("#define\tP_AS 0x%x\n", &p->p_as);
	printf("#define\tP_RSSIZE 0x%x\n", &p->p_rssize);
	printf("#define\tP_FLAG 0x%x\n", &p->p_flag);
	printf("#define\tP_PID 0x%x\n", &p->p_pid);
	printf("#define\tPROCSIZE 0x%x\n", sizeof (struct proc));
	printf("#define\tSLOAD 0x%x\n", SLOAD);
	printf("#define\tSSLEEP 0x%x\n", SSLEEP);
	printf("#define\tSRUN 0x%x\n", SRUN);
	printf("#define\tV_SWTCH 0x%x\n", &vm->v_swtch);
	printf("#define\tV_TRAP 0x%x\n", &vm->v_trap);
	printf("#define\tV_SYSCALL 0x%x\n", &vm->v_syscall);
	printf("#define\tV_INTR 0x%x\n", &vm->v_intr);
	printf("#define\tV_PDMA 0x%x\n", &vm->v_pdma);
	printf("#define\tMSGBUFSIZE 0x%x\n", sizeof (struct msgbuf));
	printf("#define\tMBPOOLBYTES 0x%x\n", MBPOOLBYTES);
	printf("#define\tMBPOOLMMUPAGES 0x%x\n", mmu_btop(MBPOOLBYTES));
	printf("#define\tR_SP 0x%x\n", &rp->r_sp);
	printf("#define\tR_RVAL1 0x%x\n", &rp->r_dreg[0]);
	printf("#define\tR_RVAL2 0x%x\n", &rp->r_dreg[1]);
	printf("#define\tR_PC 0x%x\n", &rp->r_pc);
	printf("#define\tR_SR 0x%x\n", ((int)&rp->r_sr) + sizeof (short));
	printf("#define\tR_VOR 0x%x\n", ((int)&rp->r_pc) + sizeof (int));
	printf("#define\tPCB_REGS 0x%x\n", &up->u_pcb.pcb_regs);
	printf("#define\tPCB_SR 0x%x\n", &up->u_pcb.pcb_sr);
	printf("#define\tPCB_FLAGS 0x%x\n", &up->u_pcb.pcb_flags);
	printf("#define\tU_AR0	0x%x\n", &up->u_ar0);
	printf("#define\tR_KSTK 0x%x\n", sizeof (struct regs) + sizeof (short));
	printf("#define\tU_BERR_STACK 0x%x\n", &up->u_pcb.u_berr_stack);
	printf("#define\tU_BERR_PC 0x%x\n", &up->u_pcb.u_berr_pc);
	printf("#define\tPCB_L1PT 0x%x\n", &up->u_pcb.pcb_l1pt);
	if ((int)&up->u_pcb.pcb_l1pt & 0x10) exit(1);
	printf("#define\tR_KSTK 0x%x\n", sizeof (struct regs) + sizeof (short));
	printf("#define\tU_BERR_STACK 0x%x\n", &up->u_pcb.u_berr_stack);
	printf("#define\tU_BERR_PC 0x%x\n", &up->u_pcb.u_berr_pc);
#if NFPA > 0
	printf("#define\tU_FPA_FLAGS 0x%x\n", &up->u_pcb.u_fpa_flags);
	printf("#define\tU_FPA_STATUS 0x%x\n", &up->u_pcb.u_fpa_status);
	printf("#define\tFPA_PIPE_STATUS 0x%x\n", &fpap->fp_pipe_status);
	printf("#define\tFPA_STATE 0x%x\n", &fpap->fp_state);
	printf("#define\tFPA_STABLE 0x%x\n", FPA_STABLE);
#endif NFPA > 0
	printf("#define\tNOAST_FLAG 0x%x\n", ~AST_CLR);
	printf("#define\tFRAMESIZE 0x%x\n", sizeof (struct regs));
        printf("#define\tAST_SCHED_BIT %d\n", bit(AST_SCHED));
	printf("#define\tAST_STEP_BIT %d\n", bit(AST_STEP));
	printf("#define\tTRACE_USER_BIT %d\n", bit(TRACE_USER));
	printf("#define\tTRACE_AST_BIT %d\n", bit(TRACE_AST));
	printf("#define\tSR_SMODE_BIT %d\n", bit(SR_SMODE));
	printf("#define\tSR_TRACE_BIT %d\n", bit(SR_TRACE));
	printf("#define\tIR_SOFT_INT1_BIT %d\n", bit(IR_SOFT_INT1));
	printf("#define\tIR_SOFT_INT2_BIT %d\n", bit(IR_SOFT_INT2));
	printf("#define\tIR_SOFT_INT3_BIT %d\n", bit(IR_SOFT_INT3));
	printf("#define\tU_LOFAULT 0x%x\n", &up->u_lofault);
	printf("#define\tU_FP_ISTATE 0x%x\n", &up->u_pcb.u_fp_istate);
	printf("#define\tU_FPS_REGS 0x%x\n", up->u_pcb.u_fp_status.fps_regs);
	printf("#define\tU_FPS_CTRL 0x%x\n",
		&up->u_pcb.u_fp_status.fps_control);
	printf("#define\tUSIZE 0x%x\n", sizeof (struct user));
	printf("#define\tZSSIZE 0x%x\n", sizeof (struct zscom));
	printf("#define\tZS_ADDR 0x%x\n", &zs->zs_addr);
	printf("#define\tSCBSIZE 0x%x\n", sizeof (struct scb));
	printf("#define\tROMP_PRINTF 0x%x\n", &romp->v_printf);
	printf("#define\tROMP_ROMVEC_VERSION 0x%x\n", &romp->v_romvec_version);
	printf("#define\tROMP_MEMORYBITMAP 0x%x\n", &romp->v_memorybitmap);
	printf("#define\tPTE_SIZE %d\n", sizeof (struct pte));
	printf("#define\tA_HAT_VLD 0x%x\n", &as->a_hat);
	printf("#define\tA_HAT_PFN 0x%x\n", &as->a_hat);
	printf("#define\tA_HAT_SV_UX 0x%x\n", &as->a_hat.hat_sv_uunix);
	printf("#define\tHAT_PFNMASK 0x%x\n", HAT_L1PFNMASK);
	printf("#define\tU_MAPINDEX 0x%x\n", mmu_btop(UADDR - SYSBASE) *
	    sizeof (struct pte));
	printf("#define\tKL1PT_SIZE 0x%x\n", sizeof (struct l1pt));
	printf("#define\tKL2PT_SIZE 0x%x\n", sizeof (struct l2pt));
#ifdef BCOPY
	printf("#define\tBC_MINBCOPY 0x%x\n", 2 * BC_LINESIZE);
#endif BCOPY
	printf("#define\tMAINMEM_MAP 0x%x\n", MAINMEM_MAP_SIZE);
	exit(0);
}

bit(mask)
	register long mask;
{
	register int i;

	for (i = 0; i < sizeof (int) * NBBY; i++) {
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
	exit(1);
}
