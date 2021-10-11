/*
 * @(#)vmem.h - Rev 1.1 - 7/30/92
 *
 * vmem.h:  header file for vmem.c.
 *
 */

#define         SHOW_ERRS       30      /* max # of contig errors to display */
#define         PAT_SHIFT       24      /* passcount to shifted to high byte
                                         * and or'ed with test pattern */
#define         MAXERRS         10      /* max # of noncontiguous errors before                                         * aborting test */
#define         MISCOMPARE      1
 
#define         PAGEFILE        "/tmp/vmem.page"

#define         FIXED_MARGIN    0x280000
#define         IPC_MARGIN      0x400000
#define         COLOR_MARGIN    0x200000
#define         GP2_MARGIN      0x500000
#define         CG5_MARGIN      0x400000
#define         IBIS_MARGIN     0x500000
#define         TAAC_MARGIN     0x900000
#define		ZEBRA_MARGIN	0x100000
#define		CG12_MARGIN	0x300000

struct verr {
        unsigned long   *base;          /* address of initial error     */
        unsigned long   conterrs;       /* number of contiguous errors  */
        unsigned long   observe[SHOW_ERRS]; /* keep up to 30 observed errs */
};

struct nlist nl[] = {
#define NL_SANON        0
#ifdef  SVR4
      { "anoninfo" },
#else
      { "_anoninfo" },
#endif
      { "" }
};

