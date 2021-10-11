/*
 * @(#)readfile.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stand/saio.h>
#include <a.out.h>

/*
 * Read in a Unix executable file and return its entry point.
 * Handle the various a.out formats correctly.
 * "Io" is the standalone file descriptor to read from.
 * Print informative little messages if "print" is on.
 * Returns -1 for errors.
 *
 * The parameter <LOAD> must be set in the command line to cc, e.g.
 * 	cc -DLOAD=0x4000 -o readfile.c
 * The makefile takes care of this.
 */
int (*
readfile(io, print))()
	register int io;
	int print;
{
	struct exec x;
	register int i;
	register char *addr;
	register int shared = 0;
	register int loadaddr;
	register int segsiz;
	register int datasize;
	extern char start[];

	i = read(io, (char *)&x, sizeof x);
	if (x.a_magic == ZMAGIC || x.a_magic == NMAGIC)
		shared = 1;
	if (i != sizeof x || (!shared && x.a_magic != OMAGIC)) {
		printf("Not executable\n");
		return ((int (*)()) -1);
	}
	if (print)
		printf("Size: %d", x.a_text);
	datasize = x.a_data;
	if (!shared) {
		x.a_text = x.a_text + x.a_data;
		x.a_data = 0;
	}
	if (lseek(io, N_TXTOFF(x), 0) == -1)
		goto shread;
	addr = (char *)(x.a_text + LOAD);
	if (addr  > (char *)start) {
		printf("\nProgram text overlaps boot code! 0x%x > 0x%x\n",
			addr, start);
		goto shread;
	}
#ifdef	NFS_BOOT
#define	FUDGE	(0x2000-0x20)
	if (read(io, (char *)LOAD, FUDGE) != FUDGE)
		goto shread;
	if (read(io, (char *)(LOAD+FUDGE), (int)x.a_text-FUDGE) !=
		(x.a_text-FUDGE))
		goto shread;
#else
	if (read(io, (char *)LOAD, (int)x.a_text) != x.a_text)
		goto shread;
#endif NFS_BOOT
	if (x.a_machtype == M_OLDSUN2)
		segsiz = OLD_SEGSIZ;
	else
		segsiz = SEGSIZ;
	if (shared)
		while ((int)addr & (segsiz-1))
			*addr++ = 0;
	if (addr + x.a_data > (char *)start) {
		printf("\nProgram data overlaps boot code!\n");
		goto shread;
	}
	if (print)
		printf("+%d", datasize);

	if ((int)x.a_data != 0)		/* Avoid zero length read */
		if (read(io, addr, (int)x.a_data) < x.a_data)
			goto shread;

	close(io);		/* Done with it. */
	if (print)
		printf("+%d", x.a_bss);
	addr += x.a_data;
	for (i = 0; i < x.a_bss && addr < (char *)start; i++)
		*addr++ = 0;
	if (print)
		printf(" bytes\n");
	if (x.a_machtype != M_OLDSUN2 && x.a_magic == ZMAGIC)
		loadaddr = LOAD + sizeof (struct exec);
	else
		loadaddr = LOAD;
	return (int (*)()) (loadaddr);

shread:
	printf("Truncated file\n");
	return ((int (*)()) -1);
}
