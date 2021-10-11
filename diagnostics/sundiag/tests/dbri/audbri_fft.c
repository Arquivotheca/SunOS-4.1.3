#ident   "@(#)audbri_fft.c 1.1 92/07/30 SMI"
#include <stdio.h>
#include <math.h>
#include "audioio.h"
#include "audbri.h"

/*
 *  derived from DBRI Eng group's:
 *	#ident	"@(#)fftmag.c	1.2	92/01/23 SMI"
 */

/*
 * fftmag.c
 *
 * Fast Fourier Transform magnitude routine
 *
 */

#define MINIMUM	-1000.0
#define	FULL_SCALE	32767

static int	dft_size = 0;
static double	*ccoef, *scoef, *hamming;

void dft_done()
{
	dft_size = 0;
	free(ccoef);
	free(scoef);
	free(hamming);
}

void dft_init(order)
unsigned int	order;
{
	register int	i;
	double		alpha;

	if (dft_size > 0)
		dft_done();
	dft_size = order;
	ccoef = (double *) calloc(sizeof (double), dft_size);
 	scoef = (double *) calloc(sizeof (double), dft_size);
 	hamming = (double *) calloc(sizeof (double), dft_size);
	for (i = 0; i < dft_size; i++) {
		alpha = i * M_PI * 2.0;
		hamming[i] = 0.54 - 0.46 * cos(alpha / (dft_size - 1));
		alpha = -alpha / dft_size;
		ccoef[i] = cos(alpha);
		scoef[i] = sin(alpha);
	}
}

void dft(time_r, spec_r, spec_i)
double *time_r;			/* real data arrays */
double *spec_r, *spec_i;
{
	unsigned register int	i,	/* input data array index */
				k,	/* output spectrum index */
				p;	/* coef. index */

	for (k = 0; k < dft_size; k++) {
		spec_r[k] = 0.0;
		spec_i[k] = 0.0;
		p = 0;
		for (i = 0; i < dft_size; i++) {
			spec_r[k] += ccoef[p] * time_r[i];
			spec_i[k] += scoef[p] * time_r[i];
			for (p += k; p >= dft_size; p -= dft_size);
		}
	}
}

int	bin_sel = LEFT;
double	lbins[FFT_SIZE/2+1],lrms,loffset;
double	rbins[FFT_SIZE/2+1],rrms,roffset;
extern	FILE	*deb_fp;

unsigned int fftmag(in_buf, size, out_mag)
short	*in_buf;
int	size;
double	*out_mag;
{
	double	*temp, *spec_r, *spec_i;
	int	i, n;
	extern	int	gen_file;

	if (size <= 0) {
		if (dft_size > 0)
			dft_done();
		return 0;
	} else if (size != dft_size) {
		dft_init(size);
	}

	/* allocate temporary storage */
	temp = (double *) calloc(sizeof (double), dft_size);
	spec_r = (double *) calloc(sizeof (double), dft_size);
	spec_i = (double *) calloc(sizeof (double), dft_size);

	for (i = 0; i < size; i++)	/* apply Hamming window */
		temp[i] = hamming[i] * in_buf[i];

	/* Calculate rms and DC offset values */

	if (bin_sel == LEFT) {
	    lrms = 0;
	    loffset = 0;
	    for (i = 0; i < size; i++) {
	        lrms += in_buf[i]*in_buf[i];
		loffset += in_buf[i];
	    }
	    lrms = sqrt(lrms);		/* Take the sqaure root */
	    if (lrms >= 0) {
	        lrms = 20*log10(lrms);	/* Convert to dB */
	    }
	    loffset = (loffset/size);
	    loffset = (100*loffset)/FULL_SCALE;
	}
	else {
	    rrms = 0;
	    for (i = 0; i < size; i++) {
	        rrms += in_buf[i]*in_buf[i];
		roffset += in_buf[i];
	    }
	    rrms = sqrt(rrms);		/* Take the sqaure root */
	    if (rrms >= 0) {
	        rrms = 20*log10(rrms);	/* Convert to dB */
	    }
	    roffset = (roffset/size);
	    roffset = (100*roffset)/FULL_SCALE;
	}
	   
	if (gen_file == 2) { /* Generate the time domain vaules */
	    fprintf("in_buf :\n");
	    for (i = 0; i < size; i++) {
		fprintf(deb_fp,">%3d %6d %20.3f\n",i,in_buf[i],temp[i]);
	    }
	}

	dft(temp, spec_r, spec_i);

	n = (dft_size >> 1) + 1;	/* log of magnitude */
	for (i = 0; i < n; i++) {
		out_mag[i] = spec_r[i] * spec_r[i] + spec_i[i] * spec_i[i];
		if (bin_sel == LEFT) {
		    lbins[i] = out_mag[i];
		}
		else {
		    rbins[i] = out_mag[i];
		}

		if (out_mag[i] <= 0.0)
			out_mag[i] = MINIMUM;
		else 
			out_mag[i] = 10.0 * log10(out_mag[i]);
	}

	if (gen_file) { /* Generate the frequency domain values */
	    for (i = 0; i < n; i++) {
		fprintf(deb_fp,"#%3d  %18.3f   %18.3f\n",i,lbins[i],rbins[i]);
	    }
	}

	free(temp);			/* free temporary storage */
	free(spec_r);
	free(spec_i);

	return n;
}
