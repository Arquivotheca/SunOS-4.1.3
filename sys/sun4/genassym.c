#ifndef lint
static char sccsid[] = "@(#)genassym.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
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
#include <sys/stream.h>
#include <sys/ttycom.h>
#include <sys/tty.h>

#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <machine/clock.h>
#include <machine/memerr.h>
#include <machine/intreg.h>
#include <machine/eeprom.h>
#include <machine/iocache.h>
#include <machine/eccreg.h>
#include <machine/buserr.h>

#include <sundev/zscom.h>

#include <mon/sunromvec.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>

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

struct zsops *zs_proto[] = { 0 };


main()
{
	register struct proc *p = (struct proc *)0;
	register struct vmmeter *vm = (struct vmmeter *)0;
	register struct user *up = (struct user *)0;
	register struct regs *rp = (struct regs *)0;
	register struct zscom *zs = (struct zscom *)0;
	register struct as *as = (struct as *)0;
	register struct ctx *ctx = (struct ctx *)0;
	register label_t *l = 0;
	register struct memerr *me = (struct memerr *)0;
	register struct fpu *fp = (struct fpu *)0;
#ifdef LWP
	register lwp_t *lp = (lwp_t *)0;
#endif LWP
#ifdef VAC
	struct flushmeter *fm = (struct flushmeter *)0;
#endif VAC
	struct ctx bctx;

#ifdef LWP
	printf("#define SP_OFFSET\t%d\n", &lp->mch_sp);
	printf("#define PC_OFFSET\t%d\n", &lp->mch_pc);
	printf("#define Y_OFFSET\t%d\n", &lp->mch_y);
	printf("#define G2_OFFSET\t%d\n", &(lp->mch_globals[1]));
	printf("#define	O0_OFFSET\t%d\n", &(lp->mch_oregs[0]));
	printf("#define CKSTKOVERHEAD\t%d\n", STKOVERHEAD);
	printf("#ifndef\tMINSTACKSZ\n");
	printf("#define\tMINSTACKSZ\t%d\n",
          (STKOVERHEAD + (sizeof(stkalign_t) - 1)) / sizeof(stkalign_t));
	printf("#endif\tMINSTACKSZ\n");

	/* generate CHECK macro */
	printf("#define CHECK(location, result) {\t\t\\\n");
	printf("\tif (!__stacksafe()) {\t\t\t\\\n");
	printf("\t\tlocation = result;\t\t\\\n");
	printf("\t\treturn;\t\t\t\t\\\n");
	printf("\t}\t\t\t\t\t\\\n");
	printf("}\t\t\t\t\t\t\n");
#endif LWP

	bctx.c_clean = 1;
	printf("#define\tC_CLEAN_BITMASK 0x%x\n", *((char *)&bctx) );

	printf("#define\tP_LINK 0x%x\n", &p->p_link);
	printf("#define\tP_RLINK 0x%x\n", &p->p_rlink);
	printf("#define\tP_UAREA 0x%x\n", &p->p_uarea);
	printf("#define\tP_STACK 0x%x\n", &p->p_stack);
	printf("#define\tP_PRI 0x%x\n", &p->p_pri);
	printf("#define\tP_STAT 0x%x\n", &p->p_stat);
	printf("#define\tP_WCHAN 0x%x\n", &p->p_wchan);
	printf("#define\tP_AS 0x%x\n", &p->p_as);
	printf("#define\tP_RSSIZE 0x%x\n", &p->p_rssize);
	printf("#define\tA_HAT_CTX 0x%x\n", &as->a_hat.hat_ctx);
	printf("#define\tC_NUM 0x%x\n", &ctx->c_num);
	printf("#define\tC_TIME 0x%x\n", &ctx->c_time);
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
	printf("#define\tS_READ 0x%x\n", (int)S_READ);
	printf("#define\tS_WRITE 0x%x\n", (int)S_WRITE);
	printf("#define\tS_EXEC 0x%x\n", (int)S_EXEC);
	printf("#define\tL_PC 0x%x\n", &l->val[0]);
	printf("#define\tL_SP 0x%x\n", &l->val[1]);
	printf("#define\tU_LOFAULT 0x%x\n", &up->u_lofault);
	printf("#define\tU_PROCP 0x%x\n", &up->u_procp);
	printf("#define\tUSIZE 0x%x\n", sizeof (struct user));
	printf("#define\tPCB_REGS 0x%x\n", &up->u_pcb.pcb_regs);
	printf("#define\tPCB_PC 0x%x\n", &up->u_pcb.pcb_pc);
	printf("#define\tPCB_SP 0x%x\n", &up->u_pcb.pcb_sp);
	printf("#define\tPCB_PSR 0x%x\n", &up->u_pcb.pcb_psr);
	printf("#define\tPCB_WBUF 0x%x\n", up->u_pcb.pcb_wbuf);
	printf("#define\tPCB_SPBUF 0x%x\n", up->u_pcb.pcb_spbuf);
	printf("#define\tPCB_WBCNT 0x%x\n", &up->u_pcb.pcb_wbcnt);
	printf("#define\tPCB_UWM 0x%x\n", &up->u_pcb.pcb_uwm);
	printf("#define\tPCB_FPFLAGS 0x%x\n", &up->u_pcb.pcb_fpflags);
	printf("#define\tPCB_FPCTXP 0x%x\n", &up->u_pcb.pcb_fpctxp);
	printf("#define\tPCB_FLAGS 0x%x\n", &up->u_pcb.pcb_flags);
	printf("#define\tPCB_TEM_BIT %d\n", bit(FSR_TEM));
	printf("#define\tPCB_AEXC_BIT %d\n", bit(FSR_AEXC));
	printf("#define\tFPCTX_FSR 0x%x\n", &fp->fpu_fsr);
	printf("#define\tFPCTX_REGS 0x%x\n", &fp->fpu_regs[0]);
	printf("#define\tFPCTX_Q 0x%x\n", &fp->fpu_q[0]);
	printf("#define\tFPCTX_QCNT 0x%x\n",&fp->fpu_qcnt);
	printf("#define\tAST_SCHED_BIT %d\n", bit(AST_SCHED));
	printf("#define\tZSSIZE 0x%x\n", sizeof (struct zscom));
	printf("#define\tZS_ADDR 0x%x\n", &zs->zs_addr);
	printf("#define\tPSR_PIL_BIT %d\n", bit(PSR_PIL));
	printf("#define\tPG_S_BIT %d\n", bit(PG_S));
	printf("#define\tREGSIZE %d\n", sizeof (struct regs));
	printf("#define\tROMP_PRINTF 0x%x\n", &romp->v_printf);
	printf("#define\tROMP_ROMVEC_VERSION 0x%x\n", &romp->v_romvec_version);
	printf("#define\tROMP_MEMORYBITMAP 0x%x\n", &romp->v_memorybitmap);
	printf("#define\tROMP_MEMAVAIL 0x%x\n", &romp->v_memoryavail);
	printf("#define\tCOUNTER_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_COUNTER_ADDR));
	printf("#define\tEEPROM_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_EEPROM_ADDR));
	printf("#define\tCLOCK_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_CLOCK_ADDR));
	printf("#define\tMEMERR_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_MEMERR_ADDR));
	printf("#define\tINTREG_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_INTREG_ADDR));
	printf("#define\tECCREG0_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_ECCREG0_ADDR));
	printf("#define\tECCREG1_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_ECCREG1_ADDR));
	printf("#define\tIOCTAG_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCTAG_ADDR));
	printf("#define\tIOCDATA_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCDATA_ADDR));
	printf("#define\tIOCFLUSH_PTE 0x%x\n",
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCFLUSH_ADDR));
	printf("#define\tME_VADDR 0x%x\n", &me->me_vaddr);
	printf("#define\tGENERIC_PROTERR 0x%x\n", BE_PROTERR);
	printf("#define\tGENERIC_INVALID 0x%x\n", BE_INVALID);
	printf("#define\tPTE_SIZE %d\n", sizeof (struct pte));
#ifdef VAC
	printf("#define\tFM_CTX 0x%x\n", &fm->f_ctx);
	printf("#define\tFM_USR	0x%x\n", &fm->f_usr);
	printf("#define\tFM_REGION 0x%x\n", &fm->f_region);
	printf("#define\tFM_SEGMENT 0x%x\n", &fm->f_segment);
	printf("#define\tFM_PAGE 0x%x\n", &fm->f_page);
	printf("#define\tFM_PARTIAL 0x%x\n", &fm->f_partial);
#endif VAC
	printf("#define MAINMEM_SIZE 0x%x\n",MAINMEM_MAP_SIZE);
	printf("#define PGT_SHIFT 0x%x\n",bit(PGT_MASK));
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
