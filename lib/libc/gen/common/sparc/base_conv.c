#ifdef sccsid
static char     sccsid[] = "@(#)base_conv.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

/*
 * Machine-independent versions of base conversion primitives.
 * Routines to multiply buffers by 2**16 or 10**4. Base 10**4 buffers have
 * b[i] < 10000, carry in and out < 65536. Base 2**16 buffers have b[i] <
 * 65536, carry in and out < 10000. If n is positive, b[0]..b[n-1] are
 * processed; if n is negative, b[0]..b[n+1] are processed. 
 */

void 
_fourdigits(t, d)
	unsigned        t;
	char            d[4];

/* Converts t < 10000 into four ascii digits at *pc.	 */

{
	register short  i;

	i = 3;
	do {
		d[i] = '0' + t % 10;
		t = t / 10;
	}
	while (--i != -1);
}

unsigned 
_quorem10000(u, pr)
	unsigned        u;
	unsigned       *pr;
{
	*pr = u % 10000;
	return (u / 10000);
}

void 
_mul_10000(b, n, c)
	unsigned       *b;
	int             n;
	unsigned       *c;
{
	/* Multiply base-2**16 buffer by 10000. */

	register unsigned carry, t;
	register short int i;
	register unsigned *pb;

	carry = *c;
	pb = b;
	if ((i = n) > 0) {
		i--;
		do {
			*pb = (t = (*pb * 10000) + carry) & 0xffff;
			pb++;
			carry = t >> 16;
		}
		while (--i != -1);
	} else {
		i = -i - 1;
		do {
			*pb = (t = (*pb * 10000) + carry) & 0xffff;
			pb--;
			carry = t >> 16;
		}
		while (--i != -1);
	}
	*c = carry;
}

void 
_mul_65536(b, n, c)
	unsigned       *b;
	int             n;
	unsigned       *c;
{
	/* Multiply base-10**4 buffer by 65536. */

	register unsigned carry, t;
	register short int i;
	register unsigned *pb;

	carry = *c;
	pb = b;
	if ((i = n) > 0) {
		i--;
		do {
			*pb = (t = (*pb << 16) | carry) % 10000;
			pb++;
			carry = t / 10000;
		}
		while (--i != -1);
	} else {
		i = -i - 1;
		do {
			*pb = (t = (*pb << 16) | carry) % 10000;
			pb--;
			carry = t / 10000;
		}
		while (--i != -1);
	}
	*c = carry;
}
