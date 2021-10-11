#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)des_soft.c 1.1 92/07/30 (C) 1987 Sun Micro";
#endif
/*
 * Warning!  Things are arranged very carefully in this file to
 * allow read-only data to be moved to the text segment.  The
 * various DES tables must appear before any function definitions
 * (this is arranged by including them immediately below) and partab
 * must also appear before and function definitions
 * This arrangement allows all data up through the first text to
 * be moved to text.
 */

#ifndef KERNEL
#define	CRYPT	/* cannot configure out of user-level code */
#endif


#ifndef	KERNEL
/*
 * Table giving odd parity in the low bit for ASCII characters
 */
static char partab[128] = {
	0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x07, 0x07,
	0x08, 0x08, 0x0b, 0x0b, 0x0d, 0x0d, 0x0e, 0x0e,
	0x10, 0x10, 0x13, 0x13, 0x15, 0x15, 0x16, 0x16,
	0x19, 0x19, 0x1a, 0x1a, 0x1c, 0x1c, 0x1f, 0x1f,
	0x20, 0x20, 0x23, 0x23, 0x25, 0x25, 0x26, 0x26,
	0x29, 0x29, 0x2a, 0x2a, 0x2c, 0x2c, 0x2f, 0x2f,
	0x31, 0x31, 0x32, 0x32, 0x34, 0x34, 0x37, 0x37,
	0x38, 0x38, 0x3b, 0x3b, 0x3d, 0x3d, 0x3e, 0x3e,
	0x40, 0x40, 0x43, 0x43, 0x45, 0x45, 0x46, 0x46,
	0x49, 0x49, 0x4a, 0x4a, 0x4c, 0x4c, 0x4f, 0x4f,
	0x51, 0x51, 0x52, 0x52, 0x54, 0x54, 0x57, 0x57,
	0x58, 0x58, 0x5b, 0x5b, 0x5d, 0x5d, 0x5e, 0x5e,
	0x61, 0x61, 0x62, 0x62, 0x64, 0x64, 0x67, 0x67,
	0x68, 0x68, 0x6b, 0x6b, 0x6d, 0x6d, 0x6e, 0x6e,
	0x70, 0x70, 0x73, 0x73, 0x75, 0x75, 0x76, 0x76,
	0x79, 0x79, 0x7a, 0x7a, 0x7c, 0x7c, 0x7f, 0x7f,
};

/*
 * Add odd parity to low bit of 8 byte key
 */
void
des_setparity(p)
	char *p;
{
	int i;

	for (i = 0; i < 8; i++) {
		*p = partab[*p & 0x7f];
		p++;
	}
}
#endif /* def KERNEL */
