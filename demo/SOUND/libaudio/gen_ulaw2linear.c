#ifndef lint
static	char sccsid[] = "@(#)gen_ulaw2linear.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

short		audio_ulaw2linear();
unsigned char	audio_linear2ulaw();

/*
 * Create PCM <-> u-law conversion lookup tables
 */
main()
{
	int		i;
	short		j;
	unsigned char	k;

	printf("/* Copyright (c) 1989 by Sun Microsystems, Inc. */\n\n");

	/* u-law to PCM */
	printf("/* Inverted 8-bit u-law to 16-bit signed PCM */\n");
	printf("short\t_ulaw2linear[256] = {");

	for (i = 0; i < 0x100; i++) {
		j = audio_ulaw2linear(i);
		printf("%s%6d,", (i % 8) ? " ": "\n\t", j);
	}

	printf("\n};\n\n");

	/* PCM to u-law */
	printf("/* 13-bit unsigned PCM to inverted 8-bit u-law */\n");
	printf("unsigned char\t_ulinear2ulaw[8192] = {");

	/* Loop through 13-bit space */
	for (j = -0x1000; j < 0x1000; j++) {
		/* add three to center this sample in the 16-bit space */
		k = audio_linear2ulaw((j << 3) + 3);
		printf("%s0x%02x,", ((j + 4) % 12) ? " ": "\n\t", k);
	}

	printf("\n};\n\n");

	printf("/* 13-bit signed PCM to inverted 8-bit u-law */\n");
	printf("unsigned char\t*_linear2ulaw = &_ulinear2ulaw[4096];\n");

	exit(0);
}


#define	SIGN_BIT	(0x80)		/* Sign bit for a u-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of u-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

#define	BIAS		(33)		/* Bias for linear code. */

/*
 * NeXT(tm) ulaw conversions are different.
 * It appears as though they use 32767./8191.5 as the scaling factor.
 */
#define	SCALE_UP	(32767./8158.)
#define	SCALE_DOWN	(8158./32767.)


static int	seg_endpts[NSEGS] = {
	(31 + BIAS),
	(95 + BIAS),
	(223 + BIAS),
	(479 + BIAS),
	(991 + BIAS),
	(2015 + BIAS),
	(4063 + BIAS),
	(8159 + BIAS),
};



/*
 * audio_linear2ulaw() - Convert a value (in the range of +/-32767) to u-law
 *
 * audio_linear2ulaw() accepts a short integer and encodes it as u-law data.
 *
 * U-law encoding uses a non-linear function to compress the input signal
 * and then quantizes it so that successively larger input signal intervals
 * are compressed into constant length quantization intervals. It serves
 * to increase the dynamic range at the expense of the signal-to-noise
 * ratio for large amplitude signals. The u-law characteristic is given
 * by:
 *
 *		       ln (1 + u |x|)
 *	F (x) = sgn(x) --------------
 *	 u	       ln (1 + u)
 *
 * where x is the input signal (-1 <= x <= 1), sgn(x) is the polarity of x,
 * and u is a parameter used to define the amount of compression (u = 255).
 * The expansion characteristic for the u-law compander is given by:
 *
 *	   -1			      |y|
 *	F    (y) = sgn(y)(1/u)[(1 + u)   -1)
 *	 u
 *
 * When u is 255, the companding characteristic is closely approximated
 * by a set of eight straight line segments with the slope of each
 * successive segment being exactly one half of the slope of the previous
 * segment.
 *
 * The algorithm used here implements a u255 PCM encoder using a 13 bit
 * uniform sample as input. The polarity is saved, and the magnitude of
 * the sample is encoded into 3 bits representing the segment number and
 * 4 bits representing the quantization interval within the segment.
 * The resulting code word has the form PSSSQQQQ. In order to simplify
 * the encoding process, the original linear magnitude is biased by adding
 * 33 which shifts the encoding range from (0 - 8158) to (33 - 8191). The
 * result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. (In our case we find the segment number by testing
 * against the value of the segment endpoint, which is adjusted for the bias).
 * The quantization interval is directly available as the four bits wxyz.
 * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char
audio_linear2ulaw(pcm_val)
	short		pcm_val;	/* 2's complement (16-bit precision) */
{
	int		sign;
	int		seg;
	unsigned char	uval;

	/* Get the sign and the magnitude of the value. */
	if (pcm_val < 0) {
		pcm_val = -pcm_val;
		sign = SIGN_BIT;
	} else {
		sign = 0;
	}

	/*
	 * Scale the magnitude of the word from (0 - 32767) to (0 - 8158).
	 * Add the bias to simplify table lookup by scaling to a power of two.
	 */
	pcm_val = BIAS + (int) ((double)pcm_val * SCALE_DOWN);

	/*
	 * Convert the scaled magnitude to segment number.
	 *
	 * Combine the sign, segment, and quantization bits to build
	 * the code word.  Complement the code word to make it fit for
	 * SPARCstation audio and the ISDN world.
	 */
	for (seg = 0; seg < NSEGS; seg++) {
		if (pcm_val < seg_endpts[seg]) {
			uval = (seg << SEG_SHIFT) |
			    ((pcm_val >> (seg + 1)) & QUANT_MASK);
			if (uval != 0)
				uval |= sign;
			return (~uval);
		}
	}

	/* If magnitude is out of range, return maximum value. */
	return (~(sign & (SEG_MASK | QUANT_MASK)));
}


/*
 * audio_ulaw2linear() - Convert a u-law value to PCM (in the range of +/-32767)
 *
 * audio_ulaw2linear() decodes the u-law code word by the inverse process.
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 * The result is then scaled from 12.5 bit precision to 16-bit precision.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with SPARCstation and ISDN
 * conventions.
 */
short
audio_ulaw2linear(u_val)
	int	u_val;
{
	int	t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits.
	 * Shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 1) + BIAS;
	t <<= ((u_val & SEG_MASK) >> SEG_SHIFT);
	t -= BIAS;

	/* Scale to 2s-complement 16-bit range */
	if (u_val & SIGN_BIT)
		return (t * -SCALE_UP);
	else
		return (t * SCALE_UP);
}
