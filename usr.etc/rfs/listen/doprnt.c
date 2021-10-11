/*	@(#)doprnt.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:doprnt.c	1.1"
/*	@(#)from sys V 5.0.5: doprnt.c	2.4	*/

/*
 *	_doprnt: common code for printf, fprintf, sprintf
 */

#include "lsparam.h"

#ifdef	DEBUGMODE

#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <values.h>
#include "print.h"	/* parameters & macros for doprnt */
#define PUT(p, n)	if (n == 1) (void) putc(*p, iop); \
			else if (iop->_file == _NFILE) \
			iop->_ptr = (unsigned char *) \
				memcpy((char *)iop->_ptr, p, n) +n; \
			else (void) fwrite(p, 1, n, iop)

/*
 *	C-Library routines for floating conversion
 */
extern char *fcvt(), *ecvt();

extern int strlen(), fwrite();

int
_doprnt(format, args, iop)
register char	*format;
va_list	args;
register FILE	*iop;
{
	/* This variable counts output characters. */
	int	count = 0;

	/* Starting and ending points for value to be printed */
	register char	*bp, *p;

	/* Field width and precision */
	int	width, prec;

	/* Format code */
	register int	fcode;

	/* Number of padding zeroes required on the left and right */
	int	lzero, rzero;

	/* Flags - nonzero if corresponding character is in format */
	int	length;		/* l */
	int	fplus;		/* + */
	int	fminus;		/* - */
	int	fblank;		/* blank */
	int	fsharp;		/* # */

	/* Values are developed in this buffer */
	char	buf[max(MAXDIGS, 1+max(MAXFCVT+MAXEXP, MAXECVT))];

	/* Pointer to sign, "0x", "0X", or empty */
	char	*prefix;

	/* Exponent or empty */
	char	*suffix;

	/* Buffer to create exponent */
	char	expbuf[MAXESIZ + 1];

	/* The value being converted, if integer */
	long	val;

	/* The value being converted, if real */
	double	dval;

	/* Output values from fcvt and ecvt */
	int	decpt, sign;

	/* Pointer to a translate table for digits of whatever radix */
	char	*tab;

	/* Work variables */
	int	k, n, hradix, lowbit;

	expbuf[MAXESIZ] = '\0';

	/*
	 *	The main loop -- this loop goes through one iteration
	 *	for each string of ordinary characters or format specification.
	 */
	for ( ; ; ) {
		for (bp = format;
		    (fcode = *format) != '\0' && fcode != '%'; format++)
			;
		if (n = format - bp) { /* ordinary non-% characters */
			count += n;
			PUT(bp, n);
		}
		if (fcode == '\0') /* end of format; normal return */
			return (ferror(iop) ? EOF : count);
		/*
		 *	% has been found.
		 *	First, parse the format specification.
		 */
		fplus = fminus = fblank = fsharp = lzero = 0;
		for ( ; ; ) { /* Scan the <flags> */
			switch (fcode = *++format) {
			case '+':
				fplus++;
				continue;
			case '-':
				fminus++;
				continue;
			case ' ':
				fblank++;
				continue;
			case '#':
				fsharp++;
				continue;
			}
			break;
		}

		/* Scan the field width */
		if (fcode == '*') {
			width = va_arg(args, int);
			if (width < 0) {
				width = -width;
				fminus++;
			}
			format++;
		} else {
			if (fcode == '0') /* obsolescent spec. */
				lzero++; /* pad with leading zeros */
			for (width = 0; isdigit(fcode = *format); format++)
				width = width * 10 + tonumber(fcode);
		}

		/* Scan the precision */
		if (*format != '.')
			prec = lzero ? width : -1;
		else if (*++format == '*') { /* '*' instead of digits? */
			prec = va_arg(args, int);
			format++;
		} else
			for (prec = 0; isdigit(fcode = *format); format++)
				prec = prec * 10 + tonumber(fcode);

		/* Scan the length modifier */
		length = 0;
		switch (*format) {
		case 'l':
			length++;
			/* No break */
		case 'h':
			format++;
		}

		/*
		 *	The character addressed by format must be
		 *	the format letter -- there is nothing
		 *	left for it to be.
		 *
		 *	The status of the +, -, #, and blank
		 *	flags are reflected in the variables
		 *	"fplus", "fminus", "fsharp", and
		 *	"fblank".  "width" and "prec" contain
		 *	numbers corresponding to the digit
		 *	strings before and after the decimal
		 *	point, respectively. If there was no
		 *	decimal point, "prec" is -1.
		 *
		 *	The following switch sets things up
		 *	for printing.  What ultimately gets
		 *	printed will be padding blanks, a
		 *	prefix, left padding zeroes, a value,
		 *	right padding zeroes, a suffix, and
		 *	more padding blanks.  Padding blanks
		 *	will not appear simultaneously on both
		 *	the left and the right.  Each case in
		 *	this switch will compute the value, and
		 *	leave in several variables the informa-
		 *	tion necessary to construct what is to
		 *	be printed.  
		 *
		 *	The prefix is a sign, a blank, "0x",
		 *	"0X", or null, and is addressed by
		 *	"prefix".
		 *
		 *	The suffix is either null or an
		 *	exponent, and is addressed by "suffix".
		 *
		 *	The value to be printed starts at "bp"
		 *	and continues up to and not including
		 *	"p".
		 *
		 *	"lzero" and "rzero" will contain the
		 *	number of padding zeroes required on
		 *	the left and right, respectively.  If
		 *	either of these variables is negative,
		 *	it will be treated as if it were zero.
		 *
		 *	The number of padding blanks, and
		 *	whether they go on the left or the
		 *	right, will be computed on exit from
		 *	the switch.
		 */
		
		prefix = "";
		suffix = &expbuf[MAXESIZ];
		lzero = rzero = 0;

		switch (fcode = *format++) {

		/*
		 *	fixed point representations
		 *
		 *	"hradix" is half the radix for the
		 *	conversion. Conversion is unsigned
		 *	unless fcode is 'd'. HIBITL is 100...000
		 *	binary, and is equal to	the maximum
		 *	negative number.
		 *	We assume a 2's complement machine
		 */

		case 'd':
		case 'u':
			hradix = 10/2;
			goto fixed;

		case 'o':
			hradix = 8/2;
			goto fixed;

		case 'X':
		case 'x':
			hradix = 16/2;

		fixed:
			/* Establish default precision */
			if (prec < 0)
				prec = 1;

			/* Fetch the argument to be printed */
			if(length)
				val = va_arg(args, long);
			else if(fcode == 'd')
				val = va_arg(args, int);
			else
				val = va_arg(args, unsigned);

			/* If signed conversion, make sign */
			if(fcode == 'd') {
				if(val < 0) {
					prefix = "-";
					/*
					 * Negate, checking in
					 * advance for possible
					 * overflow.
					 */
					if(val != HIBITL)
						val = -val;
				} else if (fplus)
					prefix = "+";
				else if (fblank)
					prefix = " ";
			}

			/* Set translate table for digits */
			tab = (fcode == 'X') ?
			    "0123456789ABCDEF" : "0123456789abcdef";

			/* Develop the digits of the value */
			p = bp = buf + MAXDIGS;
			while (val != 0) {
				lowbit = val & 1;
				val = (val >> 1) & ~HIBITL;
				*--bp = tab[val % hradix * 2 + lowbit];
				val /= hradix;
			}

			/* Calculate padding zero requirement */
			lzero = bp - p + prec;

			/* Handle the # flag */
			if (fsharp && bp != p)
				switch (fcode) {
				case 'o':
					if (lzero < 1)
						lzero = 1;
					break;
				case 'x':
					prefix = "0x";
					break;
				case 'X':
					prefix = "0X";
					break;
				}

			break;

		case 'E':
		case 'e':
			/*
			 * E-format.  The general strategy
			 * here is fairly easy: we take
			 * what ecvt gives us and re-format it.
			 */

			/* Establish default precision */
			if (prec < 0)
				prec = 6;

			/* Fetch the value */
			dval = va_arg(args, double);

			/* Develop the mantissa */
			bp = ecvt(dval, min(prec + 1, MAXECVT), &decpt, &sign);

			/* Determine the prefix */
		e_merge:
			if (sign)
				prefix = "-";
			else if (fplus)
				prefix = "+";
			else if (fblank)
				prefix = " ";

			/* Place the first digit in the buffer*/
			p = &buf[0];
			*p++ = (*bp != '\0') ? *bp++ : '0';

			/* Put in a decimal point if needed */
			if (prec != 0 || fsharp)
				*p++ = '.';

			/* Create the rest of the mantissa */
			for (rzero = prec; rzero > 0 && *bp != '\0'; --rzero)
				*p++ = *bp++;

			bp = &buf[0];

			/* Create the exponent */
			if (dval != 0) {
				n = decpt - 1;
				if (n < 0)
				    n = -n;
				for ( ; n != 0; n /= 10)
					*--suffix = todigit(n % 10);
			}

			/* Prepend leading zeroes to the exponent */
			while (suffix > &expbuf[MAXESIZ - 2])
				*--suffix = '0';

			/* Put in the exponent sign */
			*--suffix = (decpt > 0 || dval == 0) ? '+' : '-';

			/* Put in the e */
			*--suffix = isupper(fcode) ? 'E'  : 'e';

			break;

		case 'f':
			/*
			 * F-format floating point.  This is a
			 * good deal less simple than E-format.
			 * The overall strategy will be to call
			 * fcvt, reformat its result into buf,
			 * and calculate how many trailing
			 * zeroes will be required.  There will
			 * never be any leading zeroes needed.
			 */

			/* Establish default precision */
			if (prec < 0)
				prec = 6;

			/* Fetch the value */
			dval = va_arg(args, double);

			/* Do the conversion */
			bp = fcvt(dval, min(prec, MAXFCVT), &decpt, &sign);

			/* Determine the prefix */
		f_merge:
			if (sign && decpt > -prec && *bp != '0')
				prefix = "-";
			else if (fplus)
				prefix = "+";
			else if (fblank)
				prefix = " ";

			/* Initialize buffer pointer */
			p = &buf[0];

			/* Emit the digits before the decimal point */
			n = decpt;
			k = 0;
			do {
				*p++ = (n <= 0 || *bp == '\0' || k >= MAXFSIG) ?
				    '0' : (k++, *bp++);
			} while (--n > 0);

			/* Decide whether we need a decimal point */
			if (fsharp || prec > 0)
				*p++ = '.';

			/* Digits (if any) after the decimal point */
			n = min(prec, MAXFCVT);
			rzero = prec - n;
			while (--n >= 0)
				*p++ = (++decpt <= 0 || *bp == '\0' ||
				    k >= MAXFSIG) ? '0' : (k++, *bp++);

			bp = &buf[0];

			break;

		case 'G':
		case 'g':
			/*
			 * g-format.  We play around a bit
			 * and then jump into e or f, as needed.
			 */
		
			/* Establish default precision */
			if (prec < 0)
				prec = 6;

			/* Fetch the value */
			dval = va_arg(args, double);

			/* Do the conversion */
			bp = ecvt(dval, min(prec, MAXECVT), &decpt, &sign);
			if (dval == 0)
				decpt = 1;

			k = prec;
			if (!fsharp) {
				n = strlen(bp);
				if (n < k)
					k = n;
				while (k >= 1 && bp[k-1] == '0')
					--k;
			}
				
			if (decpt < -3 || decpt > prec) {
				prec = k - 1;
				goto e_merge;
			}
			prec = k - decpt;
			goto f_merge;

	/*	case '%':	*/
		default:
			buf[0] = fcode;
			goto c_merge;

		case 'c':
			buf[0] = va_arg(args, int);
		c_merge:
			p = (bp = &buf[0]) + 1;
			break;

		case 's':
			p = bp = va_arg(args, char *);
			if (prec < 0)
				p += strlen(bp);
			else { /* a strnlen function would  be useful here! */
				while (*p++ != '\0' && --prec >= 0)
					;
				--p;
			}
			break;

		case '\0': /* unexpected end of format; return error */
			return (EOF);

		}

		/* Calculate number of padding blanks */
		if (lzero < 0)
			lzero = 0;
		if (rzero < 0)
			rzero = 0;
		k = (n = p - bp) + lzero + rzero +
		    (prefix[0] == '\0' ? 0 : prefix[1] == '\0' ? 1 : 2) +
		    (&expbuf[MAXESIZ] - suffix);
		count += (width > k) ? width : k;

		/* Blanks on left if required */
		if (!fminus)
			while (--width >= k)
				(void) putc(' ', iop);

		/* Prefix, if any */
		while (*prefix != '\0')
			(void) putc(*prefix++, iop);

		/* Zeroes on the left */
		while (--lzero >= 0)
			(void) putc('0', iop);
		
		/* The value itself */
		if (n > 0)
			PUT(bp, n);

		/* Zeroes on the right */
		while (--rzero >= 0)
			(void) putc('0', iop);

		/* The suffix */
		while (*suffix != '\0')
			(void) putc(*suffix++, iop);

		/* Blanks on the right if required */
		while (--width >= k)
			(void) putc(' ', iop);
	}
}

#endif

