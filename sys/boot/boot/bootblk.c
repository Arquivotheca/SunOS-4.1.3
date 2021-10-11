/*
 * @(#)bootblk.c 1.1 92/07/30 Copyr 1985 Sun Micro
 * bootblk.c - read blocks into memory to jump start the real 'boot' program
 */


#include "../lib/stand/saio.h"
#include "../lib/stand/param.h"
#include <a.out.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <mon/sunromvec.h>
#ifdef OPENPROMS
#include "boot/conf.h"
#endif

/*
 * Block zero is reserved for the label.
 * Blocks 1 through 15 are reserved for the boot block.
 * The boot block has a branch followed 64 entries for information
 * about the blocks of the real boot program followed by
 * a simple program to read them in.
 */

/*
 * entries in table to describe location of boot on disk
 * written by installboot program
 */
struct blktab {
	unsigned int	b_blknum:24;	/* starting block number */
	unsigned int	b_size:8;	/* number of DEV_BSIZE byte blocks */
};


struct saioreq iob;
#ifdef OPENPROMS
struct binfo	binfo;
extern struct   boottab blkdriver;
#endif

extern struct blktab bootb[];
extern int bbsize;
extern int bbchecksum;

/*
 * Reads in the "real" boot program and jumps to it.  If this
 * attempt fails, prints "boot failed" and returns to its caller.
 *
 * It will try to determine if it is loading a Unix file by
 * looking at what should be the magic number.  If it makes
 * sense, it will use it; otherwise it jumps to the first
 * address of the blocks that it reads in.
 *
 * When it is created (by installboot) a list of starting
 * blocks and sizes is written into an array of 64 words
 * at the beginning of this program.  The information is
 * assumed to describe the blocks of the boot program which
 * are read in at LOAD and jumped to.
 */
main()
{
	register struct bootparam *bp;
	register struct saioreq *io;
	register int *addr;
	register unsigned sum;
	register struct exec *x;
#ifdef OPENPROMS
	register struct binfo	*bd;
	extern void prom_init();
	extern char *prom_bootpath();
	extern struct bootparam *prom_bootparam();
#endif

	extern struct boottab *(devsw[]);

#ifdef OPENPROMS
	prom_init("Bootblock");
	if (prom_getversion() > 0) {
		io = &iob;
		io->si_boottab = &blkdriver;
		bd = &binfo;
		(struct binfo *)io->si_devdata = bd;
		bd->name = prom_bootpath();
	} else {
		bp = prom_bootparam();
#else	OPENPROMS
		bp = *(romp->v_bootparam);
#endif	OPENPROMS
		if (devsw[0] != (struct boottab *)0) {
			bp->bp_boottab = devsw[0];
		}

		io = &iob;
		io->si_boottab = bp->bp_boottab; /* Record ptr to boot table */

		io->si_ctlr = bp->bp_ctlr;
		io->si_unit = bp->bp_unit;
		io->si_boff = bp->bp_part;
#ifdef OPENPROMS
	}
#endif

	if (devopen(io)) {
		io->si_flgs = 0;
		goto failed;
	}

	/* if zero-length boot file, can't boot */
	if (bootb->b_size <= 0) goto failed;

	io->si_flgs |= 1;

	if (sum = getboot(io) != -1) {
		devclose(io);
		sum = 0;
		for (addr = (int *)LOAD; addr < (int *)(LOAD+bbsize); addr++)
			sum += *addr;
		if (sum != bbchecksum)
			printf("checksum %x != %x, trying to boot anyway\n",
				sum, bbchecksum);
		x = (struct exec *)LOAD;
		if (x->a_magic == OMAGIC ||
		    x->a_magic == ZMAGIC ||
		    x->a_magic == NMAGIC)
			_exitto(LOAD + sizeof (struct exec));
		else
			_exitto(LOAD);
	}
failed:
	printf("boot-block startup failed\n");
}

getboot(io)
	register struct saioreq *io;
{
	register struct blktab *bt = (struct blktab *)bootb;
	register int i, size;

	for (io->si_ma = (char *)LOAD; bt->b_size; bt++) {
		io->si_offset = 0;
		io->si_bn = bt->b_blknum;
		size = io->si_cc = bt->b_size << DEV_BSHIFT;
		if (i = devread(io) != size) {
			return (-1);
		}
		io->si_ma += io->si_cc;
	}
	return (0);
}

#ifdef	sun4c

/*
 * Keep sun4c bootblk small by eliminating prom_printf/prom_putchar.
 * sun4c V0 and V2 PROMS are guaranteed to have both interfaces and
 * there are no current plans to put V3 proms on sun4c's. XXX if they do.
 * The varargs.h version of prom_printf didn't work -- v_printf only
 * takes format string + five args.
 */


void
prom_putchar(c)
	char c;
{
	(void)(*romp->v_putchar)(c);
}

/*VARARGS1*/
prom_printf(format, i1, i2, i3, i4, i5)
	char *format;
	int i1, i2, i3, i4, i5;
{
	return ((*romp->v_printf)(format, i1, i2, i3, i4, i5));
}

#endif	sun4c
