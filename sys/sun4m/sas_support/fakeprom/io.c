#ident "@(#)io.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */
#include "fakeprom.h"
#include <stand/saio.h>
#include <mon/sunromvec.h>

extern struct boottab sd_boottab;

struct boottab *(boottab[]) = {
 	&sd_boottab,
	(struct boottab *)NULL,
};

u_int firstopen = 1;
struct saioreq iob[NFILES];

/*
 * open - open device for reading or writing
 *
 *	    012345678
 * name is: xy(c,u,p)
 *
 * returns:
 *
 *    fdesc >= 0 on success
 *    -1 on error
 */
u_int
open(name)
char *name;
{
	struct boottab **btp;
	struct boottab *bp;
	struct saioreq *sip;
	int fdesc;

	/* one time initialization of iob */
	if (firstopen) {
		firstopen = 0;
		for (fdesc = 0; fdesc < NFILES; ++fdesc)
			iob[fdesc].si_flgs = 0;
	}

	if (debug)
		printf("fpopen: name=%s\n", name);

	for (fdesc = 0; fdesc < NFILES; ++fdesc)
		if (iob[fdesc].si_flgs == 0)
			break;

	if (fdesc < NFILES) { /* found a slot */

		for (btp = &boottab[0]; *btp; ++btp) 
			if (name[0] == (*btp)->b_dev[0] &&
			    name[1] == (*btp)->b_dev[1])
				break;

		if (bp = *btp) { /* found the device */

			sip = &iob[fdesc];
			sip->si_ctlr = name[3] - '0';
			sip->si_unit = name[5] - '0';
			sip->si_boff = name[7] - '0';

			/* open the device */
			if ((bp->b_open)(sip) == 0) {
				sip->si_flgs |= F_ALLOC;
				sip->si_boottab = bp;
				if (debug)
					printf("fpopen: fdesc=%d\n", fdesc);
				return fdesc;
			}
		}
	}
	if (debug)
		printf("fpopen: failed\n");

	return -1;
}

u_int
close(fdesc)
u_int fdesc;
{
	if (debug)
		printf("fpclose: fdesc=%d\n", fdesc);

	if (fdesc < NFILES && iob[fdesc].si_flgs) {
		((iob[fdesc].si_boottab)->b_close)(&iob[fdesc]);
		iob[fdesc].si_flgs = 0;
	}
	return 0;
}

u_int
readblks(fdesc, nblks, sblk, vaddrp)
int fdesc;
int nblks;
int sblk;
caddr_t vaddrp;
{
	struct saioreq *sip;

	if (fdesc < 0 || fdesc >= NFILES || !(iob[fdesc].si_flgs & F_ALLOC))
		return -1;

	if (debug)
		printf("readblks: fdesc=%d nblks=%d sblk=%d vaddr=%x\n",
		       fdesc, nblks, sblk, vaddrp);

	sip = &iob[fdesc];
	sip->si_bn = sblk;
	sip->si_cc = nblks << DEV_BSHIFT;
	sip->si_ma = vaddrp;
	return (sip->si_boottab->b_strategy)(sip, READ); 
}

u_int
writeblks(fdesc, nblks, sblk, vaddrp)
int fdesc;
int nblks;
int sblk;
caddr_t vaddrp;
{
	struct saioreq *sip;

	if (fdesc < 0 || fdesc >= NFILES || !(iob[fdesc].si_flgs & F_ALLOC))
		return -1;

	if (debug)
		printf("writeblks: fdesc=%d nblks=%d sblk=%d vaddr=%x\n",
		       fdesc, nblks, sblk, vaddrp);

	sip = &iob[fdesc];
	sip->si_bn = sblk;
	sip->si_cc = nblks << DEV_BSHIFT;
	sip->si_ma = vaddrp;
	return (sip->si_boottab->b_strategy)(sip, WRITE); 
}

