#ident "@(#)fakepte.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

/*
 * fakeprompte.c - generate page tables for fake prom
 */

#include <sys/types.h>
#include <a.out.h>
#include <stdio.h>
#include "mmu.h"
#include "pte.h"
#include "vm_hat.h"

#define FPROM_ADDR 0x700000
#define FPROM_SIZE 0x100000
#define FPTE_ADDR  0x780000
#define FPTE_ROOT  FPTE_ADDR
#define FPTE_LEV1  (FPTE_ADDR + sizeof(struct ctx_table))
#define FPTE_LEV2  (FPTE_LEV1 + sizeof(struct l1pt))

struct exec exec;

struct l2pt level2;

struct l1pt level1;

struct ctx_table root;


main(argc, argv) 
int argc;
char * argv[];
{
	int i;
	union ptpe * pp;
	struct ptp * ptpp;
	int size = 0;

	/* build context (root) table */
	root.ct[0].PageTablePointer = FPTE_LEV1 >> MMU_STD_PTPSHIFT;
	root.ct[0].EntryType = MMU_ET_PTP;
	size += sizeof(struct ctx_table);

	/* build level1 table */
	for (i = 0, pp = &level1.ptpe[0]; i < MMU_NPTE_ONE; ++i, ++pp) {
		switch (i) {
		      case 0x0:
			pp->pte.PhysicalPageNumber = 0;
			pp->pte.Cacheable = 0;
			pp->pte.Referenced = 0;
			pp->pte.Modified = 0;
			pp->pte.AccessPermissions = MMU_STD_SRWX;
			pp->pte.EntryType = MMU_ET_PTE;
			break;

		      case 0xff:
			pp->ptp.PageTablePointer = FPTE_LEV2 >> 
				MMU_STD_PTPSHIFT;
			pp->ptp.EntryType = MMU_ET_PTP;
			break;

		      default:
			pp->ptp.PageTablePointer = 0xfffffff;
			pp->ptp.EntryType = MMU_ET_INVALID;
			break;

		}
	}
	size += sizeof(struct l1pt);

	/* build level2 table */
	for (i = 0, pp = &level2.ptpe[0]; i < MMU_NPTE_TWO; ++i, ++pp) {
		switch (i) {

		      case 0x34:
		      case 0x35:
			pp->pte.PhysicalPageNumber = (FPROM_ADDR >> 
				MMU_STD_PAGESHIFT) + 
					((i - 0x34) << MMU_STD_SECONDSHIFT);
			pp->pte.Cacheable = 0;
			pp->pte.Referenced = 0;
			pp->pte.Modified = 0;
			pp->pte.AccessPermissions = MMU_STD_SRWX;
			pp->pte.EntryType = MMU_ET_PTE;
			break;

		      default:
			pp->ptp.PageTablePointer = 0xfffffff;
			pp->ptp.EntryType = MMU_ET_INVALID;
			break;

		}
	}
	size += sizeof(struct l2pt);

	exec.a_machtype = M_SPARC;
	exec.a_toolversion = TV_SUN4;
	exec.a_magic = OMAGIC;
	exec.a_text = 0;
	exec.a_data = size;

	fwrite((int *)&exec, sizeof(struct exec), 1, stdout);

	/* write context table */
	for (i = 0, ptpp = &root.ct[0]; i < NUM_CONTEXTS; ++i, ++ptpp) {
		putw(*(u_long *)ptpp, stdout);
	}

	/* write level1 table */
	for (i = 0, pp = &level1.ptpe[0]; i < MMU_NPTE_ONE; ++i, ++pp) {
		putw(*(u_long *)pp, stdout);
	}

	/* write level2 table */
	for (i = 0, pp = &level2.ptpe[0]; i < MMU_NPTE_TWO; ++i, ++pp) {
		putw(*(u_long *)pp, stdout);
	}

	exit(0);
}

