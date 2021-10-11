/*	@(#)mmu.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _sun4m_mmu_h
#define	_sun4m_mmu_h

#ifdef	IOC
#include <machine/iocache.h>
#endif	IOC

/*
 * Definitions for the Sparc Reference MMU
 */

#ifndef LOCORE
extern u_int nctxs;		/* number of contexts */
extern u_int shm_alignment;	/* VAC address consistency modulus */
#endif

/*
 * Address Space Identifiers (see SPARC Arch Manual, Appendix I)
 */

/*			0x0	reserved */
/*			0x1	unassigned */
#define	ASI_MXCC	0x2	/* Viking MXCC registers */
#define	ASI_FLPR	0x3	/* Reference MMU flush/probe */
#define	ASI_MOD		0x4	/* Module control/status register */
#define	ASI_ITLB	0x5	/* RefMMU diagnostic for Instruction TLB */
#define	ASI_DTLB	0x6	/* RefMMU diag for Data TLB */
#define	ASI_IOTLB	0x7	/* RefMMU diag for I/O TLB */
#define	ASI_UI		0x8	/* user instruction */
#define	ASI_SI		0x9	/* supervisor instruction */
#define	ASI_UD		0xA	/* user data */
#define	ASI_SD		0xB	/* supervisor data */
#define	ASI_ICT		0xC	/* I-Cache Tag */
#define	ASI_ICD		0xD	/* I-Cache Data */
#define	ASI_DCT		0xE	/* D-Cache Tag */
#define	ASI_DCD		0xF	/* D-Cache Data */
#define	ASI_FCP		0x10	/* flush cache page */
#define	ASI_FCS		0x11	/* flush cache segment */
#define	ASI_FCR		0x12	/* flush cache region */
#define	ASI_FCC		0x13	/* flush cache context */
#define	ASI_FCU		0x14	/* flush cache user */
/*			0x15 - 0x16	reserved */
#define	ASI_BC		0x17	/* Block copy */
#define	ASI_FDCP	0x18	/* flush D cache page */
#define	ASI_FDCS	0x19	/* flush D cache segment */
#define	ASI_FDCR	0x1A	/* flush D cache region */
#define	ASI_FDCC	0x1B	/* flush D cache context */
#define	ASI_FDCU	0x1C	/* flush D cache user */
/*			0x1D - 0x1E	reserved */
#define	ASI_BF		0x1F	/* Block Fill */

/*
 * MMU Pass-Through ASIs
 * Does SingleSPARC fold all these into 0x20, and if so
 * what do we do about it?
 */
#define	ASI_MEM		0x20	/* direct access to physical memory */
#define	ASI_VIDEO	0x29	/* memory-based video (TBD) */
#define	ASI_MVMEU16	0x2A	/* vme master port, user, 16-bit data */
#define	ASI_MVMEU32	0x2B	/* vme master port, user, 32-bit data */
#define	ASI_MVMES16	0x2C	/* vme master port, supv, 16-bit data */
#define	ASI_MVMES32	0x2D	/* vme master port, supv, 32-bit data */
#define	ASI_SBUS	0x2E	/* S-bus direct access */
#define	ASI_CTL		0x2F	/* Control Space */

#define	ASI_SBT		0x30	/* store buffer tags */
#define	ASI_SBD		0x31	/* store buffer data */
#define	ASI_SBC		0x32	/* store buffer control */
/* 			0x33 - 0x35	unassigned */
#define	ASI_ICFCLR	0x36	/* I-Cache Flash Clear */
#define	ASI_DCFCLR	0x37	/* D-Cache Flash Clear */
#define	ASI_MDIAG	0x38	/* Viking MMU Breakpoint Diagnostics */
#define	ASI_BIST	0x39	/* Viking BIST diagnostics */
/*			0x3A - 0x3F	reserved */
#define	ASI_MTMP1	0x40	/* Viking Emulation Temp1 */
#define	ASI_MTMP2	0x41	/* Viking Emulation Temp2 */
/*			0x42 - 0x43	reserved */
#define	ASI_MDIN	0x44	/* Viking Emulation Data In */
/*			0x45		reserved */
#define	ASI_MDOUT	0x46	/* Viking Emulation Data Out */
#define	ASI_MPC		0x47	/* Viking Emulation Exit PC */
#define	ASI_MNPC	0x48	/* Viking Emulation Exit NPC */
#define	ASI_MCTRV	0x49	/* Viking Emulation Counter Value */
#define	ASI_MCTRC	0x4A	/* Viking Emulation Counter Mode */
#define	ASI_MCTRS	0x4B	/* Viking Emulation Counter Status */
#define	ASI_MBAR	0x4C	/* Viking Breakpoint Action Reg */
/* 			0x4D - 0x7F 	unassigned */
/*			0x80 - 0xFF	reserved */

#define	ASI_RMMU	ASI_MOD		/* More descriptive ASI name */

#if defined(SAS)
#define	ASI_SAS	0x50
#endif

/*
 * Module Control Register bit mask defines - ASI_MOD
 * XXX - were listed as CPU_whatever. Wrong Answer!
 */
#define	MCR_IMPL	0xF0000000	/* SRMMU implementation number */
#define	MCR_VER		0x0F000000	/* SRMMU version number */
#define	MCR_BM		0x00004000	/* Boot mode */
#define	MCR_NF		0x00000002	/* No fault */
#define	MCR_ME		0x00000001	/* MMU enable */

/*
 * Module virtual addresses defined by the architecture (ASI_MOD)
 */
#define	RMMU_CTL_REG		0x000
#define	RMMU_CTP_REG		0x100
#define	RMMU_CTX_REG		0x200
#define	RMMU_FSR_REG		0x300
#define	RMMU_FAV_REG		0x400
#define	RMMU_AFS_REG		0x500
#define	RMMU_AFA_REG		0x600
#define	RMMU_RST_REG		0x700

#define	RMMU_TRCR	0x1400	/* TLB replacement control register */


/*
 * XXX-Need other Emulation (Debug) definitions added.
 */
#define	MBAR_MIX	0x00001000	/* Turn on SuperScalar */

/*
 * Viking MXCC registers addresses defined by the architecture (ASI_MXCC)
 */
#define	MXCC_STRM_DATA  0x01C00000	/* stream data register */
#define	MXCC_STRM_SRC   0x01C00100	/* stream source */
#define	MXCC_STRM_DEST  0x01C00200	/* stream destination */
#define	MXCC_REF_MISS   0x01C00300	/* Reference/Miss count */
#define	MXCC_BIST	0x01C00804	/* Built-in Selftest */
#define	MXCC_CNTL	0x01C00A04	/* MXCC control register */
#define	MXCC_STATUS	0x01C00B00	/* MXCC status register */
#define	MXCC_RESET	0x01C00C04	/* module reset register */
#define	MXCC_ERROR	0x01C00E00	/* error register */
#define	MXCC_PORT	0x01C00F04	/* MBux port address register */

/*
 * VIKING specific module control definitions (XXX - don't belong here)
 */

#define	CPU_VIK_PF	0x00040000	/* data prefetcher enable */
					/* ignored in MBus mode */
#define	CPU_VIK_TC	0x00010000	/* table-walk cacheable */
#define	CPU_VIK_AC	0x00008000	/* alternate cacheable */
#define	CPU_VIK_SE	0x00004000	/* snoop enable */
#define	CPU_VIK_BT	0x00002000	/* boot mode 0=boot 1=normal */
#define	CPU_VIK_PE	0x00001000	/* parity check enable */
#define	CPU_VIK_MB	0x00000800	/* MBUS mode, (w/o E-$) copy back */
#define	CPU_VIK_SB	0x00000400	/* store buffer enable */
#define	CPU_VIK_IE	0x00000200	/* i-$ enable */
#define	CPU_VIK_DE	0x00000100	/* d-$ enable */
#define	CACHE_VIK_ON    (CPU_VIK_SE|CPU_VIK_SB|CPU_VIK_IE|CPU_VIK_DE)
#define	CACHE_VIK_ON_E  (CACHE_VIK_ON|CPU_VIK_TC|CPU_VIK_PF)

/*
 * Viking MXCC specific control definitions
 */
#define	MXCC_CE		0x00000004	/* E$ enable */
#define	MXCC_PE		0x00000008	/* Parity enable */
#define	MXCC_MC		0x00000010	/* Multiple command enable */
#define	MXCC_PF		0x00000020	/* Prefetch enable */
#define	MXCC_RC		0x00000200	/* Read reference count */
#define	CACHE_MXCC_ON   (MXCC_CE|MXCC_MC|MXCC_PF)

/*
 * Viking MXCC specific error register definitions (bit<63:32>)
 */
#define	MXCC_ERR_ME	0x80000000	/* multiple errors */
#define	MXCC_ERR_CC	0x20000000	/* cache consistency error */
#define	MXCC_ERR_VP	0x10000000	/* parity err on viking write(UD) */
#define	MXCC_ERR_CP	0x08000000	/* parity error on E$ access */
#define	MXCC_ERR_AE	0x04000000	/* Async. error */
#define	MXCC_ERR_EV	0x02000000	/* Error information valid */
#define	MXCC_ERR_CCOP	0x01FF8000	/* MXCC operation code */
#define	MXCC_ERR_ERR	0x00007F80	/* Error code */
#define	MXCC_ERR_S	0x00000040	/* supervisor mode */
#define	MXCC_ERR_PA	0x0000000F	/* physical address <35:32> */

#define	MXCC_ERR_ERR_SHFT	7
#define	MXCC_ERR_CCOP_SHFT	15
/*
 * Ross605 specific module control register definitions (XXX - don't belong)
 */
#define	CPU_PT		0x00040000	/* Parity Test */
#define	CPU_PE		0x00020000	/* Parity Enable */
#define	CPU_DE		0x00010000	/* Dual Directory enable */
#define	CPU_IC		0x00008000	/* Instruction/Data Cache */
#define	CPU_C		0x00002000	/* Cacheable bit for 2nd level cache */
#define	CPU_CP		0x00001800	/* Cache Parameters */
#define	CPU_CB		0x00000400	/* Write back cache */
#define	CPU_CE		0x00000100	/* Cache enable */

#define	CACHE_ON	CPU_DE + CPU_CP + CPU_CB + CPU_CE

/* Some control register shift values */
/* context table register is bits 35-6 of pa of context table in bits 31-2 of
 * context table register
 */
#define	RMMU_CTP_SHIFT	2

/*
 * Reset Register Layout
 */
#define	RSTREG_WD	0x00000004 /* Watchdog Reset */
#define	RSTREG_SI	0x00000002 /* Software internal Reset */

/*
 * Synchronous Fault Status Register Layout
 */
#define	MMU_SFSR_OW		0x00000001	/* multi. errs have occured */
#define	MMU_SFSR_FAV		0x00000002 	/* fault address valid */
#define	MMU_SFSR_FT_SHIFT	2
#define	MMU_SFSR_FT_MASK	0x0000001C	/* fault type mask */
#define	MMU_SFSR_FT_NO		0x00000000	/* .. no error */
#define	MMU_SFSR_FT_INV		0x00000004	/* .. invalid address */
#define	MMU_SFSR_FT_PROT	0x00000008	/* .. protection error */
#define	MMU_SFSR_FT_PRIV	0x0000000C	/* .. privilege violation */
#define	MMU_SFSR_FT_TRAN	0x00000010	/* .. translation error */
#define	MMU_SFSR_FT_BUS		0x00000014	/* .. bus access error */
#define	MMU_SFSR_FT_INT		0x00000018	/* .. internal error */
#define	MMU_SFSR_FT_RESV	0x0000001C	/* .. reserved code */
#define	MMU_SFSR_AT_SHFT	5
#define	MMU_SFSR_AT_SUPV	0x00000020	/* access type: SUPV */
#define	MMU_SFSR_AT_INSTR	0x00000040	/* access type: INSTR */
#define	MMU_SFSR_AT_STORE	0x00000080	/* access type: STORE */
#define	MMU_SFSR_LEVEL_SHIFT	8
#define	MMU_SFSR_LEVEL		0x00000300	/* table walk level */
#define	MMU_SFSR_BE		0x00000400	/* M-Bus Bus Error */
#define	MMU_SFSR_TO		0x00000800	/* M-Bus Timeout */
#define	MMU_SFSR_UC		0x00001000	/* M-Bus Uncorrectable */
#define	MMU_SFSR_UD		0x00002000	/* Viking: Undefined Error */

#define	MMU_SFSR_FATAL		0x00003C00	/* fatal conditions */
#define	MMU_SFSR_P		0x00004000	/* Viking: Parity Error, */
						/* Parity err also set */
						/* MMU_SFSR_UC */
#define	MMU_SFSR_SB		0x00008000	/* Viking: Store Buffer Err */
#define	MMU_SFSR_CS		0x00010000	/* Viking: Control Space */
						/* Access Error */
#define	MMU_SFSR_EM		0x00020000	/* Viking: Error Mode Reset */
						/* Taken */

/*
 * Asynchronous Fault Status Register Layout
 */
#define	MMU_AFSR_AFV		0x00000001	/* async fault occured */
#define	MMU_AFSR_AFA_SHIFT	4
#define	MMU_AFSR_AFA		0x000000F0	/* high 4 bits of fault addr */
#define	MMU_AFSR_BE		0x00000400	/* M-Bus Bus Error */
#define	MMU_AFSR_TO		0x00000800	/* M-Bus Timeout */
#define	MMU_AFSR_UC		0x00001000	/* M-Bus Uncorrectable */

#define	MMU_AFSR_FATAL		0x00001C00	/* fatal conditions */

/*
 *	Do the following macros really belong in this file?
 */

/*
 * GETMID(r)
 * Returns module id (8..11) in register r.
 * uses the GETCPU macro below
 * %%% returns 8, not F, for level-1 modules.
 */
#define	GETMID(m)		\
	GETCPU(m)			;\
	or	m, 8, m

/*
 * GETCPU(r)
 * Returns cpu id (0..3) in register r.
 * uses cpu index bits from %tbr
 */
#define	GETCPU(c)		\
	mov	%tbr, c			;\
	srl	c, PERCPU_SHIFT, c	;\
	and	c, 3, c

/*
 * Memory related Addresses - ASI_MMUPASS
 */

#define	ECC_EN			0x0		/* ECC Memory Enable Register */
#define	ECC_FSR			0x8		/* ECC Memory Fault Status */
#define	ECC_FAR			0x10		/* ECC Memory Fault Address */
#define	ECC_DIAG		0x18		/* ECC Diagnostic Register */
#define	MBUS_ARB_EN		0x20		/* Mbus Arbiter Enable */
#define	DIAG_MESS		0x1000		/* Diag Message Passing */

/*
 * Mbus Arbiter bit fields
 */
#define	EN_ARB_P1		0x2		/* Enable Mbus Arb. Module 1 */
#define	EN_ARB_P2		0x4		/* Enable Mbus Arb. Module 2 */
#define	EN_ARB_P3		0x8		/* Enable Mbus Arb. Module 3 */
#define	EN_ARB_SBUS		0x10		/* Enable Mbus Arb. Sbus */

/* XXX */
/*
 * #define	EN_ARB_SBUS	0x1F0000	* Enable Mbus Arb. Sbus *
 */
#define	SYSCTLREG_WD		0x0010		/* sys control/status WDOG */


#define	IDPROMSIZE	0x20		/* size of id prom in bytes */

/*
 * The usable DVMA space size.
 */
#define	DVMASIZE	((1024*1024))
#define	DVMABASE	(0-(1024*1024))

/*
 * Context for kernel. On a Sun-4 the kernel is in every address space,
 * but KCONTEXT is magic in that there is never any user context there.
 */
#define	KCONTEXT	0

#if defined(KERNEL) && !defined(LOCORE)
/*
 * Low level mmu-specific functions
 */
/*
 * Cache specific routines - ifdef'ed out if there is no chance
 * of running on a machine with a virtual address cache.
 */
#ifdef VAC
void	vac_init();
void	vac_flushall();
void	vac_usrflush();
void	vac_ctxflush();
void	vac_segflush(/* base */);
void	vac_pageflush(/* base */);
void	vac_flush(/* base, len */);
#else VAC
#define	vac_init()
#define	vac_flushall()
#define	vac_usrflush()
#define	vac_ctxflush()
#define	vac_segflush(base)
#define	vac_pageflush(base)
#define	vac_flush(base, len)
#endif VAC

int	valid_va_range(/* basep, lenp, minlen, dir */);

#endif defined(KERNEL) && !defined(LOCORE)

#if defined(KERNEL) && !defined(LOCORE)
struct ControlRegister {
	u_int cr_sysControl:8;
	u_int cr_reserved:22;
	u_int cr_noFault:1;
	u_int cr_enabled:1;
};
#define	X_SYS_CONTROL(x) (((struct ControlRegister *)&(x))->cr_sysControl)
#define	MSYS_CONTROL		X_SYS_CONTROL(CtrlRegister)
#define	X_CRRESERVED(x) (((struct ControlRegister *)&(x))->cr_reserved)
#define	MCRRESERVED		X_CRRESERVED(CtrlRegister)
#define	X_NO_FAULT(x) (((struct ControlRegister *)&(x))->cr_noFault)
#define	MNO_FAULT		X_NO_FAULT(CtrlRegister)
#define	X_ENABLED(x)  (((struct ControlRegister *)&(x))->cr_enabled)
#define	MENABLED		X_ENABLED(CtrlRegister)

	/* The context register */
struct CtxtTablePtr {
	u_int ct_tablePointer:30;
	u_char ct_reserved:2;
};

#define	X_TABLE_POINTER(x) (((struct CtxtTablePtr *)&(x))->ct_tablePointer)
#define	MTABLE_POINTER		X_TABLE_POINTER(CtxtTablePtr)
#define	X_CTRESERVED(x) (((struct CtxtTablePtr *)&(x))->ct_reserved)
#define	MCTRESERVED		X_CTRESERVED(CtxtTablePtr)


	/* The Fault status register */
struct FaultStatus {
	u_int fs_Reserved:14;
	u_char fs_ExternalBusError:8;
	u_char fs_Level:2;
	u_char fs_AccessType:3;
	u_char fs_FaultType:3;
	u_char fs_FaultAddressValid:1;
	u_char fs_Overwrite:1;
};
#define	X_FSRESERVED(x) (((struct FaultStatus *)&(x))->fs_Reserved)
#define	MFSRESERVED		X_FSRESERVED(MMUFaultStatus)
#define	X_EBE(x) (((struct FaultStatus *)&(x))->fs_ExternalBusError)
#define	MEBUSERROR		X_EBE(MMUFaultStatus)
#define	X_LEVEL(x) (((struct FaultStatus *)&(x))->fs_Level)
#define	MLEVEL		X_LEVEL(MMUFaultStatus)
#define	X_ACCESS_TYPE(x) (((struct FaultStatus *)&(x))->fs_AccessType)
#define	MACCESS_TYPE		X_ACCESS_TYPE(MMUFaultStatus)
/* The following doesn't work with -O and register x, see below */
/* #define	X_FAULT_TYPE(x) ( ((struct FaultStatus *)&(x))->fs_FaultType) */
#define	MFAULT_TYPE		X_FAULT_TYPE(MMUFaultStatus)
#define	X_FAULT_AV(x) (((struct FaultStatus *)&(x))->fs_FaultAddressValid)
#define	MFAULT_AV		X_FAULT_AV(MMUFaultStatus)
#define	X_OVERWRITE(x) (((struct FaultStatus *)&(x))->fs_Overwrite)
#define	MOVERWRITE		X_OVERWRITE(MMUFaultStatus)

#endif defined(KERNEL) && !defined(LOCORE)

/* Defines for Fault status register AccessType */
#define	AT_UREAD	0
#define	AT_SREAD	1
#define	AT_UEXECUTE	2
#define	AT_SEXECUTE	3
#define	AT_UWRITE	4
#define	AT_SWRITE	5

#define	FS_ATSHIFT	5	/* shift left by this amount to get access */
				/* type */
#define	FS_ATMASK	0x7	/* After shift, mask by this amount to */
				/* get type */


/* Defines for Fault type field of the faultstatus register */

#define	FT_NONE		0
#define	FT_INVALID_ADDR	1
#define	FT_PROT_ERROR	2
#define	FT_PRIV_ERROR	3
#define	FT_TRANS_ERROR	4
#define	FT_ACC_BUSERR	5
#define	FT_INTERNAL	6

/*
 * Flush Types
 */
#define	FT_PAGE		0x00000000 /* page flush */
#define	FT_SEG		0x00000001 /* segment flush */
#define	FT_RGN		0x00000002 /* region flush */
#define	FT_CTX		0x00000003 /* context flush */
#define	FT_USER		0x00000004 /* user flush [cache] */
#define	FT_ALL		0x00000004 /* flush all [tlb] */

#define	FS_FTSHIFT		2	/* shift right by this amount to get */
					/* fault type */
#define	FS_FTMASK		0x7	/* and then mask by this value to */
					/* get fault type */
/* See above */
#define	X_FAULT_TYPE(x)	((((u_int)x) >> FS_FTSHIFT) & FS_FTMASK)

#define	FS_EBESHIFT		10 	/* shift right this amount to get */
					/* external bus error */
#define	FS_EBEMASK		0xFF	/* and mask by this value */

#define	FS_FAVSHIFT		1	/* shift right by this amount to */
					/* get adress valid */
#define	FS_FAVMASK		0x1	/* and mask by this value to get */
					/* av bit */

#define	FS_OWMASK		0x1	/* mask by this value to get ow bit */
#define	MMUERR_BITS		"\20\10EBE\9L23\8L01\7ST\6USER\5EXEC\1FAV\0OW"


#define	NL3PTEPERPT		64	/* entries in level 3 table */
#define	NL3PTESHIFT		6	/* entries in level 3 table */
#define	L3PTSIZE		(NL3PTEPERPT * MMU_PAGESIZE) /* bytes mapped */
#define	L3PTOFFSET		(L3PTSIZE - 1)
#define	NL2PTEPERPT		64	/* entries in table */
#define	L2PTSIZE		(NL2PTEPERPT * L3PTSIZE)    /* bytes mapped */
#define	L2PTOFFSET		(L2PTSIZE - 1)
#define	NL1PTEPERPT		256	/* entries in table */


#ifndef LOCORE
#ifdef VAC
extern void vac_flushall(/* */);
#endif

/* module specific mmu functions */
extern u_int	mmu_getctx(/* */);
extern u_int	mmu_getsyncflt(/* */);
extern void	mmu_getasyncflt(/* u_int *ptr */);
extern u_int	mmu_getcr(/* */);
extern u_int	mmu_getctp(/* */);
extern void	mmu_getkpte(/* addr_t base, struct pte *ppte */);
extern void	mmu_getpte(/* addr_t base, struct pte *ppte */);
extern u_int	mmu_probe(/* u_int probe_val; */);
extern void	mmu_setcr(/* u_int c */);
extern void	mmu_setctp(/* u_int c */);
extern void	mmu_setctx(/* u_int c_num */);
extern void	mmu_setpte(/* addr_t base, struct pte pte */);
#endif
#endif	/* !_sun4_mmu_h */
