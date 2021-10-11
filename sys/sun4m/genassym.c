/*	@(#)genassym.c  1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <stdio.h>
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
#include <sys/dk.h>

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
#include <machine/buserr.h>

#include <sun/autoconf.h>
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

#include <machine/auxio.h>

#ifdef notdef
#include <sun/audioio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/audio_79C30.h>
#endif	notdef


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
	register struct counterregs *cntr = (struct counterregs *)0;
	register struct fpu *fp = (struct fpu *)0;
#ifdef LWP
	register lwp_t *lp = (lwp_t *)0;
#endif LWP
#ifdef VAC
	struct flushmeter *fm = (struct flushmeter *)0;
#endif VAC
	register struct autovec *av = (struct autovec *)0;
#ifdef notdef
	register struct aud_cmd *cmdp = (struct aud_cmd *)0;
	register amd_dev_t *devp = (amd_dev_t *)0;
	register struct aud_79C30_chip *chip = (struct aud_79C30_chip *)0;
#endif	notdef
	register struct sunromvec *romp = (struct sunromvec *)0;

#ifdef LWP
	printf("#define SP_OFFSET\t%d\n", &lp->mch_sp);
	printf("#define PC_OFFSET\t%d\n", &lp->mch_pc);
	printf("#define Y_OFFSET\t%d\n", &lp->mch_y);
	printf("#define G2_OFFSET\t%d\n", &(lp->mch_globals[1]));
	printf("#define	O0_OFFSET\t%d\n", &(lp->mch_oregs[0]));
	printf("#define CKSTKOVERHEAD\t%d\n", STKOVERHEAD);
	printf("#ifndef\tMINSTACKSZ\n");
	printf("#define\tMINSTACKSZ\t%d\n",
		(STKOVERHEAD + (sizeof (stkalign_t) - 1))/sizeof (stkalign_t));
	printf("#endif\tMINSTACKSZ\n");

	/* generate CHECK macro */
	printf("#define CHECK(location, result) {\t\t\\\n");
	printf("\tif (!__stacksafe()) {\t\t\t\\\n");
	printf("\t\tlocation = result;\t\t\\\n");
	printf("\t\treturn;\t\t\t\t\\\n");
	printf("\t}\t\t\t\t\t\\\n");
	printf("}\t\t\t\t\t\t\n");
#endif LWP

	printf("#define\tP_LINK 0x%x\n", &p->p_link);
	printf("#define\tP_RLINK 0x%x\n", &p->p_rlink);
	printf("#define\tP_UAREA 0x%x\n", &p->p_uarea);
	printf("#define\tP_STACK 0x%x\n", &p->p_stack);
	printf("#define\tP_PRI 0x%x\n", &p->p_pri);
	printf("#define\tP_STAT 0x%x\n", &p->p_stat);
	printf("#define\tP_WCHAN 0x%x\n", &p->p_wchan);
	printf("#define\tP_AS 0x%x\n", &p->p_as);
	printf("#define\tP_RSSIZE 0x%x\n", &p->p_rssize);
	printf("#define\tP_PPTR 0x%x\n", &p->p_pptr);
#ifdef	MULTIPROCESSOR
	printf("#define\tP_CPUID 0x%x\n", &p->p_cpuid);
	printf("#define\tP_PAM 0x%x\n", &p->p_pam);
#endif	MULTIPROCESSOR
	printf("#define\tCP_USER\t%d\n", CP_USER);
	printf("#define\tCP_NICE\t%d\n", CP_NICE);
	printf("#define\tCP_SYS\t%d\n", CP_SYS);
	printf("#define\tCP_IDLE\t%d\n", CP_IDLE);
	printf("#define\tA_HAT_CTX 0x%x\n", &as->a_hat.hat_ctx);
	printf("#define\tA_HAT_CPU 0x%x\n", &as->a_hat.hat_oncpu);
	printf("#define\tC_NUM 0x%x\n", &ctx->c_num);
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
	printf("#define\tS_OTHER 0x%x\n", (int)S_OTHER);
	printf("#define\tL_PC 0x%x\n", &l->val[0]);
	printf("#define\tL_SP 0x%x\n", &l->val[1]);
	printf("#define\tU_LOFAULT 0x%x\n", &up->u_lofault);
	printf("#define\tU_PROCP 0x%x\n", &up->u_procp);
	printf("#define\tUSIZE 0x%x\n", sizeof (struct user));
	printf("#define\tPCB_REGS 0x%x\n", &up->u_pcb.pcb_regs);
	printf("#define\tPCB_PC 0x%x\n", &up->u_pcb.pcb_pc);
	printf("#define\tPCB_SP 0x%x\n", &up->u_pcb.pcb_sp);
	printf("#define\tPCB_PSR 0x%x\n", &up->u_pcb.pcb_psr);
	printf("#define\tPCB_PSRP 0x%x\n", &up->u_pcb.pcb_psrp);
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
	printf("#define\tFPCTX_QCNT 0x%x\n", &fp->fpu_qcnt);
	printf("#define\tAST_SCHED_BIT %d\n", bit(AST_SCHED));
	printf("#define\tZSSIZE 0x%x\n", sizeof (struct zscom));
	printf("#define\tZS_ADDR 0x%x\n", &zs->zs_addr);
	printf("#define\tPSR_PIL_BIT %d\n", bit(PSR_PIL));
	printf("#define\tREGSIZE %d\n", sizeof (struct regs));

	/*
 	 * note that these PTEs don't have their cacheable bits set.
 	 */
	printf("#define\tKL1PT_SIZE 0x%x\n", sizeof (union ptpe));
	printf("#define\tKL2PT_SIZE 0x%x\n", sizeof (union ptpe));
	printf("#define\tHAT_PFNMASK 0x%x\n", HAT_L1PFNMASK);
#ifdef VAC
	printf("#define\tFM_CTX 0x%x\n", &fm->f_ctx);
	printf("#define\tFM_USR	0x%x\n", &fm->f_usr);
	printf("#define\tFM_REGION 0x%x\n", &fm->f_region);
	printf("#define\tFM_SEGMENT 0x%x\n", &fm->f_segment);
	printf("#define\tFM_PAGE 0x%x\n", &fm->f_page);
	printf("#define\tFM_PARTIAL 0x%x\n", &fm->f_partial);
#endif VAC
	printf("#define MAINMEM_SIZE 0x%x\n", MAINMEM_MAP_SIZE);
	printf("#define	CNTR_LIMIT10 0x%x\n", &cntr->limit10);
	printf("#define\tAV_VECTOR 0x%x\n", &av->av_vector);
	printf("#define\tAV_INTCNT 0x%x\n", &av->av_intcnt);
	printf("#define\tAUTOVECSIZE 0x%x\n", sizeof (struct autovec));
#ifdef notdef
	printf("#define\tAUD_CMD_DATA 0x%x\n", &cmdp->data);
	printf("#define\tAUD_CMD_ENDDATA 0x%x\n", &cmdp->enddata);
	printf("#define\tAUD_CMD_NEXT 0x%x\n", &cmdp->next);
	printf("#define\tAUD_CMD_SKIP 0x%x\n", &cmdp->skip);
	printf("#define\tAUD_CMD_DONE 0x%x\n", &cmdp->done);
	printf("#define\tAUD_DEV_CHIP 0x%x\n", &devp->chip);
	printf("#define\tAUD_PLAY_CMDPTR 0x%x\n", &devp->play.cmdptr);
	printf("#define\tAUD_PLAY_SAMPLES 0x%x\n", &devp->play.samples);
	printf("#define\tAUD_PLAY_ACTIVE 0x%x\n", &devp->play.active);
	printf("#define\tAUD_PLAY_ERROR 0x%x\n", &devp->play.error);
	printf("#define\tAUD_REC_CMDPTR 0x%x\n", &devp->rec.cmdptr);
	printf("#define\tAUD_REC_SAMPLES 0x%x\n", &devp->rec.samples);
	printf("#define\tAUD_REC_ACTIVE 0x%x\n", &devp->rec.active);
	printf("#define\tAUD_REC_ERROR 0x%x\n", &devp->rec.error);
	printf("#define\tAUD_CHIP_IR 0x%x\n", &chip->ir);
	printf("#define\tAUD_CHIP_CR 0x%x\n", &chip->cr);
	printf("#define\tAUD_CHIP_DR 0x%x\n", &chip->dr);
	printf("#define\tAUD_CHIP_BBRB 0x%x\n", &chip->bbrb);
	printf("#define\tAUD_CHIP_INIT_REG 0x%x\n",
	    AUDIO_UNPACK_REG(AUDIO_INIT_INIT));
	printf("#define\tAUD_CHIP_DISABLE 0x%x\n",
	    AUDIO_INIT_BITS_ACTIVE | AUDIO_INIT_BITS_INT_DISABLED);
#endif	notdef
	printf("#define\tOP_MAGIC 0x%x\n", &romp->op_magic);
	printf("#define\tOP_ROMVEC_VERSION 0x%x\n", &romp->op_romvec_version);
	printf("#define\tOP_PLUGIN_VERSION 0x%x\n", &romp->op_plugin_version);
	printf("#define\tOP_MON_ID 0x%x\n", &romp->op_mon_id);
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
