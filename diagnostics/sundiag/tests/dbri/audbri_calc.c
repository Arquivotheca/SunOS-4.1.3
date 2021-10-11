#ident   "@(#)audbri_calc.c 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */

/*
 *  Calculations for Speaker Box parameters
 * 'Synergy' platforms and Sbus 'Batphone' card.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/asynch.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <math.h>
/* #include <sun/audioio.h> ...not there yet... */
#include "audioio.h" 
#include "audio_filehdr.h" 

#include "sdrtns.h"
#include "../../../lib/include/libonline.h"

#include "audbri.h"
#include "audbri_msg.h"

extern	double	lbins[],rbins[];

double	calc_snr(ch)
int	ch;	/* Left or Right Channel */
{
double	signal,noise,db;
int	i,fund,h1,h2,size,w;

	size = FFT_SIZE/2+1;
	fund = size/4;	/* The bin of the fundamental freq */
	h1   = 2*fund;	/* First Harmonic */
	h2   = 3*fund;	/* Second Harmonic */
	w    = 1;	/* The number of bins to each side */
			/* that counts as signal */

	signal = 0;	/* Frequency Domain Siganl Value */
	noise = 0;	/* Frequency Domain Noise Value */

	/* i starts from 2 to exclued the DC bins */
	for (i = 2; i < size; i++) {
	    if ((i >= (fund-w)) && (i <= (fund+w))) {
		/* Fundamental Frequency */
		if (i == fund) { 
		    if (ch == LEFT) signal += lbins[i];
		    else	    signal += rbins[i];
		}
	    }
	    else {
		/* Do not count harmonics as noise */
		if ((i >= (h1-w)) && (i <= (h1+w))) break;
		if ((i >= (h2-w)) && (i <= (h2+w))) break;

		/* Noise */
		if (ch == LEFT) noise += lbins[i];
		else	        noise += rbins[i];
	    }
	}

	if (signal <= 0) signal = .001; /* Avoid log10 error */
	db = 20*log10(signal/noise);

	return(db);
}

/*	Calculate Total Harmonic distortion	*/

double	calc_thd(ch)
int	ch;	/* Left or Right Channel */
{
double	signal,hd,db;
int	i,fund,size;

	size = FFT_SIZE/2+1;
	fund = size/4;	/* The bin of the fundamental freq */

	if (ch == LEFT) signal = lbins[fund];
	else	        signal = rbins[fund];
	hd = 0;

	for (i = 1; i < size; i++) {
	    if ((i == (2*fund)) || (i == (3*fund))) {
	        if (ch == LEFT) hd += lbins[i];
		else		hd += rbins[i];
	    }
	}

	if (hd <= 0.0) hd = .001;
	db = 20*log10(hd/signal);

	return(db);
}

/*	Calculate the Channel Separation	*/

double	calc_sep(ch)
int	ch;	/* Left or Right channel */
{
double	signal,xtalk,db;
int	i,fund,size;

	size = FFT_SIZE/2+1;
	fund = size/4;	/* The bin of the fundamental freq */

	if (ch == LEFT) {
	    signal = lbins[fund-1] + lbins[fund] + lbins[fund+1];
	    xtalk  = rbins[fund-1] + rbins[fund] + rbins[fund+1];
	    xtalk  += (rbins[2*fund-1] + rbins[2*fund] + rbins[2*fund+1]);
	    xtalk  += (rbins[3*fund-1] + rbins[3*fund] + rbins[3*fund+1]);
	}
	else {
	    signal = rbins[fund-1] + rbins[fund] + rbins[fund+1];
	    xtalk  = lbins[fund-1] + lbins[fund] + lbins[fund+1];
	    xtalk  += (lbins[2*fund-1] + lbins[2*fund] + lbins[2*fund+1]);
	    xtalk  += (lbins[3*fund-1] + lbins[3*fund] + lbins[3*fund+1]);
	}

	db = 20*log10(signal/xtalk);

	return(db);
}
