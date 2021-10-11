/*
 * @(#)chklabel.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stand/saio.h>
#include <sun/dklabel.h>

char msg_nolabel[] = "No label found - attempting boot anyway.\n";

/*
 * Checks a disk label, returns 0 for success, 1 for failure.
 * If the magic number is right but the checksum is wrong, prints
 * "Corrupt label" error message, otherwise is silent.
 */
chklabel(label)
	register struct dk_label *label;
{
	register int count, sum = 0;
	register short *sp;

	if (label->dkl_magic != DKL_MAGIC)
		return (1);
	count = sizeof (struct dk_label) / sizeof (short);
	sp = (short *)label;
	while (count--) 
		sum ^= *sp++;
	if (sum != 0) {
		printf("Corrupt label\n");
		return (1);
	}
	return (0);
}
