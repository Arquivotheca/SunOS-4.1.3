#ident   "@(#)audbri_utils.c 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */

/*
 *  Utility routines for DBRI/MMCODEC audio tests.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/asynch.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
/* #include <sun/audioio.h> ...not there yet... */
#include "audioio.h"

#include "sdrtns.h"
#include "../../../lib/include/libonline.h"

#include "audbri.h"
#include "audbri_msg.h"
#include "audbri_g711.h"

extern	int	bin_sel;
extern	double	lbins[],rbins[],lrms,rrms;

/*
 *  Calculate calibration and loopback offset (in shorts) into signal.
 */
int
calc_offset(auptr)
au_data_t	*auptr;
{
	int	chancnt, offset;

	func_name = "calc_offset";
	TRACE_IN

	if (auptr->channels == MONO) {
		chancnt = 1;
	}
	else {
		chancnt = 2;
	}

	offset = .75 * (chancnt * auptr->sample_rate);

	TRACE_OUT
	return(offset);
}


/*
 *  Calculate signal's RMS value.
 */
double
rmscalc(sigptr, sample_cnt)
short	*sigptr;
int	sample_cnt;
{
	int	i;
	double	sum;

	func_name = "rmscalc";
	TRACE_IN

	/*
	 *  Calculate RMS
	 */
	for (i = 0, sum = 0; i < sample_cnt; sigptr++, i++) {
		sum = sum + ((double) *sigptr) * ((double) *sigptr);
	}
	sum = sum / sample_cnt;
	sum = sqrt(sum);

	/*
	 *  Convert to db.
	 */
	sum = 20 * log10(sum);

	TRACE_OUT
	return(sum);
}


/*
 *  Returns the maximum fft magnitude for a given frequency range
 */
double
bandrms(magp, fsize, size, start, end, sr)
double	*magp;
int	fsize;
int	size;
int	start;
int	end;
int	sr;
{
	int	i, lower_slot, upper_slot, tmp_slot;
	double	sum, tmp, save;

	func_name = "bandrms";
	TRACE_IN

	/*
	 *  Determine starting and ending fft magnitude slots for 
	 * this frequency range.
	 */
	if (start == 0) {
		lower_slot = 0;
	}
	else {
		lower_slot = rint((double) (((double) start * 
			(double) size) / (double) sr));
	}

	if (end == END_FREQ) {
		upper_slot = fsize;
	}
	else {
		upper_slot = rint((double) (((double) end * 
			(double) size) / (double) sr));
	}

	/*
	 *  Determine the RMS for this band.
	 */
	if (lower_slot > upper_slot) {
	    tmp_slot = upper_slot;
	    upper_slot = lower_slot;
	    lower_slot = tmp_slot;
	}
	    
	sum = 0;
	for (i = lower_slot; i <= upper_slot; i++) {
		tmp = exp10(magp[i]/20);
		sum = sum + (tmp * tmp);
	}
	sum = sum / (upper_slot - lower_slot + 1);
	sum = sqrt(sum);

	/*
	 *  Convert to db.
	 */
	sum = 20 * log10(sum);

	TRACE_OUT
	return(sum);
}




/*
 *  Generate a sine wave of given frequency, sample_rate and duration
 * allocated for the signal (an array of doubles) and pointed to by 
 * 'signal'.  The signal produced will be scaled to the range 
 * specified by the value 'maxval'.  'maxval' is the maximum value
 * the signal have. 
 */
int
gensine(freq, srate, dur, sigptr, maxval)
int		freq;	/* Hz */
int		srate;	/* Hz */
int		dur;	/* seconds */
short		**sigptr;
int		maxval;
{
	int	i, sample_cnt;
	short	*sptr;
	double	radians, samples_per_cycle;

	func_name = "gensine";
	TRACE_IN

	if (freq != 0) {
		samples_per_cycle = (double) srate / (double) freq;
	}
	sample_cnt = srate * dur;

	/*
	 *  get space for the signal
	 */
	if ((*sigptr = (short *) calloc(sample_cnt, sizeof(short))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, 
			(sizeof(short)*sample_cnt), SYSMSG);
	}

	/*
	 *  make the signal
	 */
	for (i = 0, sptr=*sigptr; i < sample_cnt;) {
		if (freq == 0) {
			*sptr++ = 0;
			i++;
		}
		else {
			for (radians = 0; radians < (2*M_PI), i < sample_cnt; 
					i++, radians += 
					(2*M_PI)/samples_per_cycle ) {
				*sptr++ = (short) (sin(radians) * 
					(double) maxval);
			}
		}
	}

	TRACE_OUT
	return(sample_cnt);
}


/*
 *  Generate a stereo (2 channel) signal from 2 mono signals.  If a NULL
 * is passed for either channel then that channel is filled with zeros.
 */
int
genstereo(lsigptr, rsigptr, ssigptr, sample_cnt)
short		*lsigptr, *rsigptr;
short		**ssigptr;
int		sample_cnt;
{
	int	i, st_sample_cnt;
	short	*sptr;

	func_name = "genstereo";
	TRACE_IN

	st_sample_cnt = 2*sample_cnt;

	/*
	 *  get space for the signal
	 */
	if ((*ssigptr = (short *) calloc(st_sample_cnt, 
			sizeof(short))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, 
			(sizeof(short)*st_sample_cnt), SYSMSG);
	}

	/*
	 *  make the signal
	 */
	for (i = 0, sptr=*ssigptr; i < sample_cnt; i++) {
		*sptr++ = (lsigptr ? *lsigptr++ : 0);
		*sptr++ = (rsigptr ? *rsigptr++ : 0);
	}

	TRACE_OUT
	return(st_sample_cnt);
}


/*
 *  Generate a signal to play, either mono or stereo depending upon
 * the contents of the au_data_t structure.
 */
int
gensig(auptr, sigptr, duration)
au_data_t	*auptr;
short		**sigptr;
int		duration;
{
	short	*lsig, *rsig;
	int	sample_cnt;

	func_name = "gensig";
	TRACE_IN

	/*
	 *  Generate left channel signal.
	 */
	sample_cnt = gensine(auptr->lfreq, auptr->sample_rate, 
		duration, &lsig, MAXPCM);

	/*
	 *  MONO signals just use left channel data from the au_data_t.
	 */
	if (auptr->channels == MONO) {
		*sigptr = lsig;
	}

	/*
	 *  Generate right side also if this is STEREO
	 */
	else {
		(void) gensine(auptr->rfreq, auptr->sample_rate, duration, 
			&rsig, MAXPCM);
		sample_cnt = genstereo(lsig, rsig, sigptr, sample_cnt);
	}

	TRACE_OUT
	return(sample_cnt);
}



/*
 *  Split a stereo (2 channel) signal into 2 mono signals.
 */
void
splitstereo(lsigptr, rsigptr, ssigptr, sample_cnt)
short		**lsigptr, **rsigptr;
short		*ssigptr;
int		sample_cnt;
{
	int	i;
	short	*lptr, *rptr;

	func_name = "splitstereo";
	TRACE_IN

	sample_cnt = sample_cnt / 2;

	/*
	 *  get space for mono signals
	 */
	if ((*lsigptr = (short *) calloc(sample_cnt, 
			sizeof(short))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, 
			(sizeof(short)*sample_cnt), SYSMSG);
	}
	if ((*rsigptr = (short *) calloc(sample_cnt, 
			sizeof(short))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, 
			(sizeof(short)*sample_cnt), SYSMSG);
	}

	/*
	 *  split the signal
	 */
	for (i = 0, lptr=*lsigptr, rptr=*rsigptr; i < sample_cnt; i++) {
		*lptr++ = *ssigptr++;
		*rptr++ = *ssigptr++;
	}

	TRACE_OUT
}


/*
 *  Play data.  Doesn't return until the play is completed.
 */
int
play(psig, sample_cnt, precision)
caddr_t	psig;
int	sample_cnt;
int	precision;
{
	int		i, err, wcnt, write_size, size, count;
	audio_info_t	info;
	struct timeval  begin_tp, end_tp;
	long		time;

	func_name = "play";
	TRACE_IN

	if (precision == 16) {
		size = sample_cnt * sizeof(short);	/* we're writing shorts */
	}
	else {
		size = sample_cnt;
	}

	/*
	 *  unpause
	 */
	AUDIO_INITINFO(&info);
	info.play.pause = 0;
	if (ioctl(audiofd, AUDIO_SETINFO, &info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	/*
	 *  play, continue on EINTR
	 */
	(void) gettimeofday(&begin_tp, NULL);
	count = 0;
	while (count < size && dc_flag != 4) {

		if (play_size > size - count) {
			write_size = size - count;
		}
		else {
			write_size = play_size;
		}

		/*
		 *  Restart write if error in EINTR.
		 */
		do {
			wcnt = write(audiofd, psig, write_size);
			if (wcnt == -1 && errno != EINTR) {
				send_message(ERSYS, ERROR, er_write, SYSMSG);
			}
		} while(wcnt == -1);

		count += wcnt;
		psig += wcnt;
	}

	/*
	 *  wait for completion or controls test completion (dc_flag), 
	 * continue on EINTR
	 */
	while (dc_flag != 4 && (err = ioctl(audiofd, 
				AUDIO_DRAIN, NULL)) != 0) {
		if (err == -1 && errno != EINTR) {
			send_message(ERSYS, ERROR, er_ioctl, 
				"AUDIO_DRAIN", SYSMSG);
		}	
		
	}

	(void) gettimeofday(&end_tp, NULL);
	time = (long) intervalcalc(&begin_tp, &end_tp);

	/*
	 *  pause play
	 */
	AUDIO_INITINFO(&info);
	info.play.pause = 1;
	if (ioctl(audiofd, AUDIO_SETINFO, &info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	/*
	 *  flush the audio stream
	 */
	if (ioctl(audiofd, I_FLUSH, FLUSHW) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "I_FLUSH", SYSMSG);
	}

	TRACE_OUT

	return((int) time);
}


/*
 *  Play and record data at the same time.  This routine is used
 * for loopback tests.  This routine expects psig to point to a 16 bit
 * PCM signal and returns a pointer to a 16 bit PCM signal in *rsig.
 *
 *  The variable auptr->encoding determines what type of signal is
 * actually played and recorded.
 */
void
playrecord(psig, rsig, sample_cnt, dur, auptr)
short	*psig;
short	**rsig;
int	sample_cnt;
int	dur;
au_data_t	*auptr;
{
	aio_result_t	aw_result, ar_result;
	struct timeval	timeout;
	int		i, sample_size;
	audio_info_t	info;
	u_char		*play_sig, *record_sig, *tmptr;
	short		*sptr;

	func_name = "playrecord";
	TRACE_IN

	/*
	 *  Convert play signal to encoding indicated by auptr->encoding
	 */
	switch (auptr->encoding) {
	  case AUDIO_ENCODING_ULAW:
		/*
		 *  Make sure precision is 8.
		 */
		if (auptr->precision != 8) {
			send_message(ERAUDIO, ERROR, er_precision);
		}
			
		/*
		 *  get space for the signal
		 */
		sample_size = sizeof(char);
		if ((play_sig = (u_char *) calloc(sample_cnt, 
				sample_size)) == NULL) {
			send_message(ERSYS, ERROR, er_mem, sample_cnt, SYSMSG);
		}

		/*
		 *  Convert play signal to ulaw.
		 */
		tmptr = play_sig;
		for (i = 0; i < sample_cnt; i++) {
			*tmptr++ = audio_s2u(*psig++);
		}
		break;

	  case AUDIO_ENCODING_ALAW:
		/*
		 *  Make sure precision is 8.
		 */
		if (auptr->precision != 8) {
			send_message(ERAUDIO, ERROR, er_precision);
		}
			
		/*
		 *  get space for the signal
		 */
		sample_size = sizeof(char);
		if ((play_sig = (u_char *) calloc(sample_cnt, 
				sample_size)) == NULL) {
			send_message(ERSYS, ERROR, er_mem, sample_cnt, SYSMSG);
		}

		/*
		 *  Convert play signal to alaw.
		 */
		tmptr = play_sig;
		for (i = 0; i < sample_cnt; i++) {
			*tmptr++ = audio_s2a(*psig++);
		}
		break;

	  case AUDIO_ENCODING_LINEAR:
		play_sig = (u_char *) psig;
		sample_size = sizeof(short);
		break;

	  default:
		send_message(ERAUDIO, ERROR, er_encoding);
		break;
	}

	/*
	 *  get space for the recorded signal
	 */
	if ((record_sig = (u_char *) calloc(sample_cnt, sample_size)) == NULL) {
		send_message(ERSYS, ERROR, er_mem, 
			(sample_size*sample_cnt), SYSMSG);
	}

	/*
	 *  set up unpause
	 */
	AUDIO_INITINFO(&info);
	info.play.pause = 0;
	info.record.pause = 0;

	/*
	 *  initialize aio results
	 */
	aw_result.aio_return = ar_result.aio_return = AIO_INPROGRESS;

	/*
	 *  start play
	 */
	if (aiowrite(audiofd, play_sig, sample_size*sample_cnt, 
		0, 0, &aw_result) == -1) {
		send_message(ERSYS, ERROR, er_aiowrite, SYSMSG);
	}

	/*
	 *  unpause record and play
	 */
	if (ioctl(audiofd, AUDIO_SETINFO, &info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	/*
	 *  start record
	 */
	if (aioread(audiofd, record_sig, sample_size*sample_cnt, 
			0, 0, &ar_result) == -1) {
		send_message(ERSYS, ERROR, er_aioread, SYSMSG);
	}

	/*
	 *  wait for completion
	 */
	i = 0;
	do {
		timeout.tv_sec = dur;
		timeout.tv_usec = 0;
		if (aiowait(&timeout) == -1) {
			if (errno == EINVAL) {  /* nothing to wait for */
				break;
			}
			send_message(ERSYS, ERROR, er_aiowait, SYSMSG);
		}
		i++;
	} while ((i<aioto_cnt) && (aw_result.aio_return == AIO_INPROGRESS ||
	ar_result.aio_return == AIO_INPROGRESS));

	/*
	 *  wait for play to "drain"
	 */
	if (ioctl(audiofd, AUDIO_DRAIN, NULL) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_DRAIN", SYSMSG);
	}	

	/*
	 *  pause record and play
	 */
	AUDIO_INITINFO(&info);
	info.play.pause = 1;
	info.record.pause = 1;
	if (ioctl(audiofd, AUDIO_SETINFO, &info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	/*
	 *  see if aiowrite and aioread completed ok
	 */
	if (aw_result.aio_return == AIO_INPROGRESS || 
			aw_result.aio_return == -1) {
		(void) aiocancel(&aw_result);
		send_message(ERSYS, ERROR, er_aiowrite, SYSMSG);
	}
	
	if (ar_result.aio_return == AIO_INPROGRESS ||
			ar_result.aio_return == -1) {
		(void) aiocancel(&ar_result);
		send_message(ERSYS, ERROR, er_aioread, SYSMSG);
	}

	/*
	 *  See if all data was written
	 */
	if (aw_result.aio_return != sample_cnt*sample_size) {
		send_message(ERAUDIO, ERROR, er_aiowritep, 
			aw_result.aio_return, sample_cnt*sample_size);
	}

	/*
	 *  See if all data was read
	 */
	if (ar_result.aio_return != sample_cnt*sample_size) {
		send_message(ERAUDIO, ERROR, er_aioreadp, 
			ar_result.aio_return, sample_cnt*sample_size);
	}

	/*
	 *  flush the audio stream
	 */
	if (ioctl(audiofd, I_FLUSH, FLUSHRW) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "I_FLUSH", SYSMSG);
	}

	/*
	 *  Convert recorded signal to linear encoding (unless it's
	 * already linear).
	 */
	switch (auptr->encoding) {
	  case AUDIO_ENCODING_ULAW:
		/*
		 *  get space for the signal
		 */
		if ((*rsig = (short *) calloc(sample_cnt, 
				sizeof(short))) == NULL) {
			send_message(ERSYS, ERROR, er_mem, 
				sample_cnt*sizeof(short), SYSMSG);
		}

		/*
		 *  Convert recorded signal to 16 bit PCM.
		 */
		tmptr = record_sig;
		sptr = *rsig;
		for (i = 0; i < sample_cnt; i++) {
			*sptr++ = audio_u2s(*tmptr++);
		}
		free(play_sig);
		free(record_sig);
		break;

	  case AUDIO_ENCODING_ALAW:
		/*
		 *  get space for the signal
		 */
		if ((*rsig = (short *) calloc(sample_cnt, 
				sizeof(short))) == NULL) {
			send_message(ERSYS, ERROR, er_mem, 
				sample_cnt*sizeof(short), SYSMSG);
		}

		/*
		 *  Convert recorded signal to 16 bit PCM.
		 */
		sptr = *rsig;
		tmptr = record_sig;
		for (i = 0; i < sample_cnt; i++) {
			*sptr++ = audio_a2s(*tmptr++);
		}
		free(play_sig);
		free(record_sig);
		break;

	  case AUDIO_ENCODING_LINEAR:
		*rsig = (short *) record_sig;
		break;

	  default:
		send_message(ERAUDIO, ERROR, er_encoding);
		break;
	}


	TRACE_OUT
}

/*
 *  Release reference file resources.
 */
unset_ref()
{
	func_name = "unset_ref";
	TRACE_IN

	(void) munmap(refptr, ref_mapsize);

	refname = NULL;
	refptr = NULL;

	TRACE_OUT
}


/*
 *  This routine is called if the calibration test is not being
 * run.  It attempts to open the reference file pointed to by
 * 'refname' if refname is non-NULL, otherwise, the file opened is
 * derived from the loopback "type".
 */
int
set_ref()
{
	int		fd;
	struct stat	statbuf;

	func_name = "set_ref";
	TRACE_IN

	if (refname[0] < 'A') {
		strcpy(refname,ldata[ltype].refname);
	}

	/*
	 *	open the file, get it's size and map it
	 */
	if ((fd = open(refname, O_RDONLY)) == -1 ) {
		send_message(ERSYS, ERROR, er_open, refname, SYSMSG);
	}

	if (fstat(fd, &statbuf) == -1) {
		send_message(ERSYS, ERROR, er_stat, SYSMSG);
	}

	ref_mapsize = statbuf.st_size;
	if ((refptr = (ref_data_t *)mmap((caddr_t) 0, ref_mapsize, 
			PROT_READ, MAP_SHARED, 
			fd, (off_t) 0)) == (ref_data_t *)-1 ) {
		send_message(ERSYS, ERROR, er_mmap, SYSMSG);
	}
	close(fd);

	TRACE_OUT
}


/*
 *  Get number of elements in an band_data_t array.
 */
int
getbdsiz(bdptr)
band_data_t	*bdptr;
{
	int	i=0;

	while (bdptr->type != NULL) {
		i++;
		bdptr++;
	}
	return(i);
}


/*
 *  Get number of elements in an au_data_t array.
 */
int
getausiz(auptr)
au_data_t	*auptr;
{
	int	i=0;

	while (auptr->sample_rate != NULL) {
		i++;
		auptr++;
	}
	return(i);
}


/*
 *  Determine the size of the reference file, open and map it.
 */
void
map_ref(fname, auptr)
char		*fname;
au_data_t	*auptr;
{
	int	fd, fftsize, rdsize;
	double	rms;

	func_name = "map_ref";
	TRACE_IN

	/*
	 *	open the file
	 */
	if ((fd = open(fname, O_CREAT | O_RDWR, 0666)) == -1 ) {
		send_message(ERSYS, ERROR, er_open, fname, SYSMSG);
	}

	/*
	 *  The reference file has space for left/right channel data
	 * even if the au_data_t says MONO.  This makes coding a bit
	 * easier.
	 */
	fftsize = ref_size/2 + 1;
	rdsize = fftsize * sizeof(double) + sizeof(ref_data_t);
	ref_mapsize = getausiz(auptr) * rdsize * 2;

	if (ftruncate(fd, ref_mapsize) == -1 ) {
		send_message(ERSYS, ERROR, er_truncate, SYSMSG);
	}

	/*
	 *  map the fft magnitude file
	 */
	if ((refptr = (ref_data_t *) mmap ((caddr_t) 0, ref_mapsize,
			(PROT_READ | PROT_WRITE), 
			MAP_SHARED, fd, (off_t) 0)) == (ref_data_t *)-1 ) {
		send_message(ERSYS, ERROR, er_mmap, SYSMSG);
	}
	close(fd);
}

/*
 *  Performs RMS and fft calculations and saves RMS, gain and fft
 * magnitude data in a file as reference data for further tests.
 */
void
calc_ref(sigptr, size, pgain, rgain, refp)
short	*sigptr;
int	size;
int	pgain;
int	rgain;
ref_data_t *refp;
{
	int	fftsize;
	double	rms;

	func_name = "calc_ref";
	TRACE_IN

	/*
	 *  perform RMS calculation and write rms and gain
	 */
	rms = rmscalc(sigptr, size);
	if (bin_sel == LEFT) lrms = rms;
	else		     rrms = rms;
	refp->rms = rms;
	refp->play_gain = pgain;
	refp->record_gain = rgain;

	/*
	 *  perform fft and write magnitude data
	 */
	fftsize = size/2+1;
	bin_sel = LEFT;
	if (fftmag(sigptr, size, &refp->mag) != fftsize) {
		send_message(ERAUDIO, ERROR, er_fftmag);
	}

	send_message(0, DEBUG, ref_msg, pgain, rgain, rms);

	TRACE_OUT
}


/*
 *  Generate reference data and get it into the reference file.
 */
void
genref(sigptr, sample_cnt, pgain, auptr, aud_index)
short   	*sigptr;
int		sample_cnt;
int		pgain;
au_data_t	*auptr;
int		aud_index;
{
	int	i, fftsize, rdsize;
	short	*lsig, *rsig;
	ref_data_t *refp;

	func_name = "genref";
	TRACE_IN

	/*
	 *  Calculate size of reference data (per sample) and
	 * get a pointer into the mapped reference file.
	 */
	fftsize = sample_cnt/2 + 1;
	rdsize = fftsize * sizeof(double) + sizeof(ref_data_t);
	refp = (ref_data_t *) ((caddr_t) refptr + (rdsize * 2 * aud_index));

	if (auptr->channels == MONO) {
		calc_ref(sigptr, sample_cnt, pgain, auptr->cal_rgain, refp);
		if (dump_signals) {
			dump_sig(sigptr, sample_cnt, 
				aufname("ref-rec-mono", auptr));
		}
	}

	else {
		splitstereo(&lsig, &rsig, sigptr, 2*sample_cnt);
		calc_ref(lsig, sample_cnt, pgain, auptr->cal_rgain, refp);

		if (dump_signals) {
			dump_sig(lsig, sample_cnt, 
				aufname("ref-rec-left", auptr));
			dump_sig(rsig, sample_cnt, 
				aufname("ref-rec-right", auptr));
		}

		refp = (ref_data_t *) ((caddr_t) refptr + 
			(rdsize * 2 * aud_index) + rdsize);
		calc_ref(rsig, sample_cnt, pgain, auptr->cal_rgain, refp);
		free(lsig);
		free(rsig);
	}

	TRACE_OUT
}


/*
 *  Returns the maximum fft magnitude for a given frequency range
 */
double
maxmag(magp, fsize, size, start, end, sr)
double	*magp;
int	fsize;
int	size;
int	start;
int	end;
int	sr;
{
	int	i, lower_slot, upper_slot;
	double	max;

	func_name = "maxmag";
	TRACE_IN

	/*
	 *  Determine starting and ending fft magnitude slots for 
	 * this frequency range.
	 */
	if (start == 0) {
		lower_slot = 0;
	}
	else {
		lower_slot = rint((double) (((double) start * 
			(double) size) / (double) sr));
	}

	if (end == END_FREQ) {
		upper_slot = fsize;
	}
	else {
		upper_slot = rint((double) (((double) end * 
			(double) size) / (double) sr));
	}

	/*
	 *  Find biggest value and return it.
	 */
	max = 0;
	for (i = lower_slot; i <= upper_slot; i++) {
		if (magp[i] > max) {
			max = magp[i];
		}
	}

	TRACE_OUT
	return(max);
}


/*
 *  Calculate the thresholds for a given signal and reference data.
 * We just malloc some space for a bunch of doubles and put all the
 * threshold values there.
 */
calc_threshold(refp, sample_cnt, auptr, thdata)
ref_data_t	*refp;
int		sample_cnt;
au_data_t	*auptr;
thresh_data_t	*thdata;
{
	int		i, bcnt, fftsize, rdsize;
	double		*lrefmag, *rrefmag, lref_bandrms, rref_bandrms;
	double		tmp_rms, tmp_mag, ref_bandrms;
	ref_data_t	*lrefp, *rrefp;

	func_name = "calc_threshold";
	TRACE_IN

	/*
	 *  Get some space for the rms limits (upper and lower for
	 * each channel).
	 */
	if ((thdata->lrms = (double *) calloc(2, sizeof(double))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, (sizeof(double)* 2), SYSMSG);
	}

	if ((thdata->rrms = (double *) calloc(2, sizeof(double))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, (sizeof(double)* 2), SYSMSG);
	}

	/*
	 *  Get some space for FFT band threshold values (number of
	 * bands * 2 channels).
	 */
	bcnt = getbdsiz(auptr->bdata);	/* number of freq bands */

	if ((thdata->lmag = (double *) calloc(bcnt, sizeof(double))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, 
			(sizeof(double)*bcnt), SYSMSG);
	}

	if ((thdata->rmag = (double *) calloc(bcnt, sizeof(double))) == NULL) {
		send_message(ERSYS, ERROR, er_mem, 
			(sizeof(double)*bcnt), SYSMSG);
	}

	fftsize = sample_cnt/2+1;	/* offsets for ref data */
	rdsize = fftsize * sizeof(double) + sizeof(ref_data_t);

	lrefp = refp;		/* left and right chan ref ptrs */
	rrefp = (ref_data_t *) ((caddr_t) refp + rdsize);

	lrefmag = &lrefp->mag;	/* left and right ref fft mags */
	rrefmag = &rrefp->mag;

	/*
	 *  RMS limit and fft magnitude threshold calculation for 
	 * mono signals.  The rms limits and fft magnitude thresholds 
	 * for mono signals are placed in the left channel "slot".
	 * Right channel slots are filled with zero.
	 *
	 */
	if (auptr->channels == MONO) {
		/*
		 *  set RMS limits
		 */
		thdata->lrms[0] = lrefp->rms + aurms_tol;
		thdata->lrms[1] = lrefp->rms - aurms_tol;
		thdata->rrms[0] = 0;
		thdata->rrms[1] = 0;

		/*
		 *  Set FFT magnitude thresholds for each band.
		 */
		for (i = 0; i < bcnt; i++) {
			ref_bandrms = bandrms(lrefmag, fftsize, sample_cnt, 
				auptr->bdata[i].start_freq, 
				auptr->bdata[i].end_freq, 
				auptr->sample_rate);

			if (auptr->bdata[i].type == BAND_FUND) {
				thdata->lmag[i] = ref_bandrms - aumag_tol;
				thdata->rmag[i] = 0;
			}
			else {
				thdata->lmag[i] = ref_bandrms + aumag_tol;
				thdata->rmag[i] = 0;
			}
		}
	}

	/*
	 *  RMS limit and fft magnitude threshold calculation for 
	 * stereo signal with both channels.
	 */
	else if (auptr->lfreq && auptr->rfreq) {
		/*
		 *  Set RMS limits, use the average of the left and
		 * right channels as the basis.
		 */
		tmp_rms = 20*log10((exp10(lrefp->rms/20) + 
			exp10(rrefp->rms/20))/2);
		send_message(0, DEBUG, tmprms_msg, tmp_rms);
		thdata->lrms[0] = tmp_rms + aurms_tol;
		thdata->lrms[1] = tmp_rms - aurms_tol;
		thdata->rrms[0] = tmp_rms + aurms_tol;
		thdata->rrms[1] = tmp_rms - aurms_tol;

		/*
		 *  Set FFT magnitude thresholds for each band.
		 */
		for (i = 0; i < bcnt; i++) {
			lref_bandrms = bandrms(lrefmag, fftsize, sample_cnt, 
				auptr->bdata[i].start_freq, 
				auptr->bdata[i].end_freq, 
				auptr->sample_rate);

			rref_bandrms = bandrms(rrefmag, fftsize, sample_cnt, 
				auptr->bdata[i].start_freq, 
				auptr->bdata[i].end_freq, 
				auptr->sample_rate);

			tmp_mag = 20*log10((exp10(lref_bandrms/20) + 
				exp10(rref_bandrms/20))/2);
			send_message(0, DEBUG, tmpmag_msg, tmp_mag);
			if (auptr->bdata[i].type == BAND_FUND) {
				thdata->lmag[i] = tmp_mag - aumag_tol;
				thdata->rmag[i] = tmp_mag - aumag_tol;
			}
			else {
				thdata->lmag[i] = tmp_mag + aumag_tol;
				thdata->rmag[i] = tmp_mag + aumag_tol;
			}
		}
	}

	/*
	 *  RMS limit and fft magnitude threshold calculation for 
	 * stereo signal with one channel driven.
	 *
	 *  If only one channel is driven then the threshold for the
	 * non-driven channel will be the difference between the
	 * the left and right channels in the reference signal 
	 * plus some offset.
	 *
	 *  In this case, the number calculated below for the non-
	 * driven channel's RMS and FFT mag is not the actual 
	 * threshold but a value to subtracted from the test signal's
	 * driven channel RMS or FFT mag.  (confused yet?)  The 
	 * actual threshold will be calculated when when get the 
	 * test signal's RMS and SST values in chksig().
	 *
	 *  The idea is that the threshold for the non-driven channel
	 * should be:
	 *	D(test) - [D(ref) - ND(ref) - tolerance]
	 *
	 * where D(test) = driven channel from test, D(ref) =
	 * driven channel from reference and ND(ref) = non-
	 * driven channel from reference.
	 *
	 */
	else {
		/*
		 *  Set RMS limits.
		 */
		if (auptr->lfreq) {
			thdata->lrms[0] = lrefp->rms + aurms_tol;
			thdata->lrms[1] = lrefp->rms - aurms_tol;
		}
		else {
			tmp_rms = rrefp->rms - lrefp->rms;
			send_message(0, DEBUG, tmprms_msg, tmp_rms);
			thdata->lrms[0] = tmp_rms - aurms_tol;
			thdata->lrms[1] = 0;
		}
		if (auptr->rfreq) {
			thdata->rrms[0] = rrefp->rms + aurms_tol;
			thdata->rrms[1] = rrefp->rms - aurms_tol;
		}
		else {
			tmp_rms = lrefp->rms - rrefp->rms;
			send_message(0, DEBUG, tmprms_msg, tmp_rms);
			thdata->rrms[0] = tmp_rms - aurms_tol;
			thdata->rrms[1] = 0;
		}

		/*
		 *  Set FFT magnitude thresholds for each band.
		 */
		for (i = 0; i < bcnt; i++) {
			lref_bandrms = bandrms(lrefmag, fftsize, sample_cnt, 
				auptr->bdata[i].start_freq, 
				auptr->bdata[i].end_freq, 
				auptr->sample_rate);

			rref_bandrms = bandrms(rrefmag, fftsize, sample_cnt, 
				auptr->bdata[i].start_freq, 
				auptr->bdata[i].end_freq, 
				auptr->sample_rate);

			if (auptr->bdata[i].type == BAND_FUND) {
				if (auptr->lfreq) {
					thdata->lmag[i] = 
						lref_bandrms - aumag_tol;
				}
				else {
					tmp_mag = rref_bandrms - lref_bandrms;
					send_message(0, DEBUG, tmpmag_msg, 
						tmp_mag);
					thdata->lmag[i] = tmp_mag - aumag_tol;
				}

				if (auptr->rfreq) {
					thdata->rmag[i] = 
						rref_bandrms - aumag_tol;
				}
				else {
					tmp_mag = lref_bandrms - rref_bandrms;
					send_message(0, DEBUG, tmpmag_msg, 
						tmp_mag);
					thdata->rmag[i] = tmp_mag - aumag_tol;
				}
			}
			else {
				if (auptr->lfreq) {
					thdata->lmag[i] = 
						lref_bandrms + aumag_tol;
				}
				else {
					tmp_mag = rref_bandrms - lref_bandrms;
					send_message(0, DEBUG, tmpmag_msg, 
						tmp_mag);
					thdata->lmag[i] = tmp_mag - aumag_tol;
				}

				if (auptr->rfreq) {
					thdata->rmag[i] = 
						rref_bandrms + aumag_tol;
				}
				else {
					tmp_mag = lref_bandrms - rref_bandrms;
					send_message(0, DEBUG, tmpmag_msg, 
						tmp_mag);
					thdata->rmag[i] = tmp_mag - aumag_tol;
				}
			}
		}
	}
	TRACE_OUT
}


/*
 *  returns whether or not a signal is valid.
 */
int
sig_valid(refp, test_sig, sample_cnt, auptr)
ref_data_t	*refp;
short		*test_sig;
int		sample_cnt;
au_data_t	*auptr;
{
	int	i, fftsize, rdsize;
	short	*lsig, *rsig;
	double	*thptr, *rmsptr, *lmag, *rmag;
	thresh_data_t   thdata;

	func_name = "sig_valid";
	TRACE_IN

	/*
	 *  Calculate RMS and FFT magnitude thresholds.
	 */
	calc_threshold(refp, sample_cnt, auptr, &thdata);

	/*
	 *  Check mono signal.
	 */
	if (auptr->channels == MONO) {
		fftsize = sample_cnt/2 + 1;
		rdsize = fftsize * sizeof(double) + sizeof(ref_data_t);

		if (dump_signals) {
			dump_sig(test_sig, sample_cnt, 
				aufname("test-rec-mono", auptr));
		}

		/*
		 *  get space for fft magnitudes
		 */
		if ((lmag = (double *) calloc(fftsize, 
				sizeof(double))) == NULL) {
			send_message(ERSYS, ERROR, er_mem, 
				(sizeof(double)*fftsize), SYSMSG);
		}
	
		/*
		 *  perform fft on test signal
		 */
		 bin_sel = LEFT;
		if (fftmag(test_sig, sample_cnt, lmag) != fftsize) {
			send_message(ERAUDIO, ERROR, er_fftmag);
		}

		if (chksig(refp, auptr->lfreq, test_sig, NULL, lmag, NULL, 
				sample_cnt, auptr, thdata.lrms, 
				thdata.lmag) == FALSE) {
			TRACE_OUT
			return(FALSE);
		}
	}

	/*
	 *  Check STEREO signal.
	 */
	else {
		splitstereo(&lsig, &rsig, test_sig, 2*sample_cnt);
		fftsize = sample_cnt/2 + 1;
		rdsize = fftsize * sizeof(double) + sizeof(ref_data_t);

		if (dump_signals) {
			dump_sig(lsig, sample_cnt, 
				aufname("test-rec-left", auptr));
			dump_sig(rsig, sample_cnt, 
				aufname("test-rec-right", auptr));
		}

		/*
		 *  get space for fft magnitudes
		 */
		if ((lmag = (double *) calloc(fftsize, 
				sizeof(double))) == NULL) {
			send_message(ERSYS, ERROR, er_mem, 
				(sizeof(double)*fftsize), SYSMSG);
		}
		if ((rmag = (double *) calloc(fftsize, 
				sizeof(double))) == NULL) {
			send_message(ERSYS, ERROR, er_mem, 
				(sizeof(double)*fftsize), SYSMSG);
		}
	
		/*
		 *  perform fft on test signal
		 */
		 bin_sel = LEFT;
		if (fftmag(lsig, sample_cnt, lmag) != fftsize) {
			send_message(ERAUDIO, ERROR, er_fftmag);
		}
		 bin_sel = RIGHT;
		if (fftmag(rsig, sample_cnt, rmag) != fftsize) {
			send_message(ERAUDIO, ERROR, er_fftmag);
		}

		send_message(0, DEBUG, left_msg);
		if (chksig(refp, auptr->lfreq, lsig, rsig, lmag, 
				rmag, sample_cnt, auptr, thdata.lrms, 
				thdata.lmag) == FALSE) {
			send_message(0, DEBUG, er_lnotvalid);
			TRACE_OUT
			return(FALSE);
		}
	
		send_message(0, DEBUG, right_msg);
		refp = (ref_data_t *) ((caddr_t) refp + rdsize);
		if (chksig(refp, auptr->rfreq, rsig, lsig, rmag, 
				lmag, sample_cnt, auptr, thdata.rrms, 
				thdata.rmag) == FALSE) {
			send_message(0, DEBUG, er_rnotvalid);
			TRACE_OUT
			return(FALSE);
		}
	}

	TRACE_OUT
	return(TRUE);
}


/*
 *  Determines whether or not a signal "matches" the reference signal.
 * The FFT magnitudes are divided into 4 bands and "matching" means that 
 * the peak FFT magnitudes (in a particular band) for the test signal are 
 * within 'aumag_tol' db of the FFT magnitudes for the reference signal
 * and that the test signal's RMS value is within 'aurms_tol db.
 */
int
chksig(refp, driven, test_sig, alt_sig, testmag, altmag, sample_cnt, auptr, rmsptr, magptr)
ref_data_t	*refp;
int		driven;
short		*test_sig;
short		*alt_sig;
double		*testmag;
double		*altmag;
int		sample_cnt;
au_data_t	*auptr;
double		*rmsptr;
double		*magptr;
{
	int	i, fftsize, rdsize, count;
	double	alt_rms, rms, tolerance, upper_limit, lower_limit;
	double	ref_fbandrms, alt_threshold;
	double	threshold, *refmag, ref_bandrms, test_bandrms, alt_bandrms;
	double	*lrmsptr, *rrmsptr, *lmagptr, *rmagptr;

	func_name = "chksig";
	TRACE_IN

	fftsize = sample_cnt/2+1;

	/*
	 *  Perform RMS calculation
	 */
	rms = rmscalc(test_sig, sample_cnt);
	if (!driven) {
		alt_rms = rmscalc(alt_sig, sample_cnt);
	}

	/*
	 *  calculate upper and lower limits for rms
	 */
	upper_limit = *rmsptr++;
	lower_limit = *rmsptr++;

	/*
	 *  Verify that RMS is within tolerance
	 */
	if (driven) {
		if (rms < lower_limit || rms > upper_limit) {
			send_message(0, VERBOSE+DEBUG, er_rms, rms, lower_limit,
				upper_limit, refp->rms);
			TRACE_OUT
			return(FALSE);
		}
		else {
			send_message(0, DEBUG, rms_msg, rms, refp->rms, 
				lower_limit, upper_limit);
		}
	}
	else {
		upper_limit = alt_rms - upper_limit;
		if (rms > upper_limit) {
			send_message(0, VERBOSE+DEBUG, er_rms, rms, lower_limit,
				upper_limit, refp->rms);
			TRACE_OUT
			return(FALSE);
		}
		else {
			send_message(0, DEBUG, rms_msg, rms, refp->rms, 
				lower_limit, upper_limit);
		}
	}

	/*
	 *  Determine an alternative non-fundamental band threshold 
	 * from the fundamental frequency band.  The alternative is
	 * the fundamental RMS - bandrms_tol.  It's used if it is
	 * greater than the regular threshold.
	 * 
	 */
	for (i = 0; auptr->bdata[i].type != NULL; i++) {
		if (auptr->bdata[i].type == BAND_FUND) {
			if (driven) {
				refmag = &refp->mag;
			}
			else {
				refmag = altmag;
			}

			ref_fbandrms = bandrms(refmag, fftsize, 
				sample_cnt, 
				auptr->bdata[i].start_freq, 
				auptr->bdata[i].end_freq, 
				auptr->sample_rate);

			alt_threshold = ref_fbandrms - bandrms_tol;
		}
	}

	/*
	 *  check each fft frequency band
	 */
	for (i=0, refmag = &refp->mag; auptr->bdata[i].type != NULL; i++) {

		/*
		 *  Find the RMS of the fft mag in the reference signal for 
		 * this band and calculate upper limit.
		 */
		ref_bandrms = bandrms(refmag, fftsize, sample_cnt, 
			auptr->bdata[i].start_freq, auptr->bdata[i].end_freq, 
			auptr->sample_rate);

		/*
		 *  Find test signal's max fft mag in this band.
		 */
		test_bandrms = bandrms(testmag, fftsize, sample_cnt,
			auptr->bdata[i].start_freq, auptr->bdata[i].end_freq, 
			auptr->sample_rate);

		/*
		 *  If this is a stereo signal and this channel isn't
		 * driven then we need to derive the threshold.  This
		 * code assumes that if one channel isn't driven then
		 * the other is.  
		 *  Mono signal are always driven (of course).
		 */
		if (driven) {
			threshold = *magptr++;
		}
		else {
			alt_bandrms = bandrms(altmag, fftsize, sample_cnt,
				auptr->bdata[i].start_freq, 
				auptr->bdata[i].end_freq, 
				auptr->sample_rate);
			threshold = alt_bandrms - *magptr++;
		}

		/*
		 *  If this is the fundamental's band, then make sure 
		 * the test signal's maximum magnitude is above the 
		 * reference maximum magnitude (minus tolerance).
		 */
		if (driven && (auptr->bdata[i].type == BAND_FUND)) {
			if (test_bandrms < threshold) {
				send_message(0, DEBUG+VERBOSE, er_fft, 
					(auptr->bdata[i].type == BAND_FUND ? 
					"Fundamental" : "Non-Fundamental"), 
					auptr->bdata[i].start_freq, 
					(auptr->bdata[i].end_freq == END_FREQ ? 
					auptr->sample_rate/2 : 
					auptr->bdata[i].end_freq),
					test_bandrms, 
					ref_bandrms, threshold);
				free(testmag);
				TRACE_OUT
				return(FALSE);
			}
			else {
				send_message(0, DEBUG, fft_msg, 
					(auptr->bdata[i].type == BAND_FUND ? 
					"Fundamental" : "Non-Fundamental"), 
					auptr->bdata[i].start_freq, 
					(auptr->bdata[i].end_freq == END_FREQ ? 
					auptr->sample_rate/2 :
					auptr->bdata[i].end_freq),
					test_bandrms, 
					ref_bandrms, threshold);
			}
		}

		/*
		 *  Otherwise, this is not the fundamental band (or there's
		 * no signal driven) so make sure the test signal's maximum 
		 * magnitude is below the reference maximum magnitude 
		 * (plus tolerance).
		 */
		else {
			/*
			 *  Use the greater of (fundamental RMS - bandrms_tol)
			 * or the previously calculated threshold as the 
			 * actual threshold.
			 */
			if (threshold < alt_threshold) {
				threshold = alt_threshold;
				send_message(0, DEBUG, altth_msg);
			}

			if (test_bandrms > threshold) {
				send_message(0, DEBUG+VERBOSE, er_fft, 
					(auptr->bdata[i].type == BAND_FUND ? 
					"Fundamental" : "Non-Fundamental"), 
					auptr->bdata[i].start_freq, 
					(auptr->bdata[i].end_freq == END_FREQ ? 
					auptr->sample_rate/2 :
					auptr->bdata[i].end_freq),
					test_bandrms, 
					ref_bandrms, threshold);
				free(testmag);
				TRACE_OUT
				return(FALSE);
			}
			else {
				send_message(0, DEBUG, fft_msg, 
					(auptr->bdata[i].type == BAND_FUND ? 
					"Fundamental" : "Non-Fundamental"), 
					auptr->bdata[i].start_freq, 
					(auptr->bdata[i].end_freq == END_FREQ ? 
					auptr->sample_rate/2 :
					auptr->bdata[i].end_freq),
					test_bandrms, 
					ref_bandrms, threshold);
			}
		}
	}

	free(testmag);
	TRACE_OUT
	return(TRUE);
}


/*
 *  returns whether or not a signal is clipped.  A signal is
 * considered clipped if any sample is at max.  Signal passed in
 * are expected to be 16 bit PCM but may have been converted from
 * other encodings.  The max value will vary depending upon the
 * signal original encoding.
 */
int
clipchk(sigptr, sample_cnt, encoding)
short	*sigptr;
int	sample_cnt;
int	encoding;
{
	int	i, max;

	func_name = "clipchk";
	TRACE_IN

	switch (encoding) {
	  case AUDIO_ENCODING_ALAW:
		max = MAXALAW;
		break;

	  case AUDIO_ENCODING_ULAW:
		max = MAXULAW;
		break;

	  case AUDIO_ENCODING_LINEAR:
		max = MAXPCM;
		break;

	  default:
		send_message(ERAUDIO, ERROR, er_encoding);
		break;
	}

	for (i = 0; i < sample_cnt; sigptr++, i++) {
		if (abs((int) *sigptr) >= max) {
			TRACE_OUT
			return(TRUE);
		}
	}

	TRACE_OUT
	return(FALSE);
}


/*
 *  returns whether or not a signal is clipped.
 */
int
isclipped(sigptr, sample_cnt, auptr)
short		*sigptr;
int		sample_cnt;
au_data_t	*auptr;
{
	int	i;
	short	*lsig, *rsig;

	func_name = "st_isclipped";
	TRACE_IN

	if (auptr->channels == MONO) {
		if (clipchk(sigptr, sample_cnt, auptr->encoding) == TRUE) {
			send_message(0, DEBUG, clip_msg);
			TRACE_OUT
			return(TRUE);
		}
	}

	else {
		splitstereo(&lsig, &rsig, sigptr, sample_cnt);

		if (clipchk(lsig, sample_cnt/2, auptr->encoding) == TRUE) {
			send_message(0, DEBUG, lclip_msg);
			free(lsig);
			TRACE_OUT
			return(TRUE);
		}
		free(lsig);
	
		if (clipchk(rsig, sample_cnt/2, auptr->encoding) == TRUE) {
			send_message(0, DEBUG, rclip_msg);
			free(rsig);
			TRACE_OUT
			return(TRUE);
		}
		free(rsig);
	}

	TRACE_OUT
	return(FALSE);
}


/*
 *  Calculate interval in usec.
 */
long
intervalcalc(btp, etp)
struct timeval	*btp, *etp;
{
	long	seconds, useconds;

	func_name = "intervalcalc";
	TRACE_IN

	seconds = etp->tv_sec - btp->tv_sec;
	useconds = etp->tv_usec - btp->tv_usec;
	TRACE_OUT
	return (seconds*1000000+useconds);
}

/*
 *  Loopback open.  Open audio device for write/read (play/record), 
 * pause the play and record, flush the record and setinfo.
 */
void
loopback_open(info)
audio_info_t	*info;
{
	audio_info_t	pinfo;

	func_name = "loopback_open";
	TRACE_IN

	/*
	 *  open audio device
	 */
	if ((audiofd = open(device_name, O_RDWR|O_NDELAY)) < 0) {
		send_message(ERSYS, ERROR, er_open, device_name, SYSMSG);
	}

	/*
	 *  pause play and record
	 */
	AUDIO_INITINFO(&pinfo);
	pinfo.record.pause = 1;
	pinfo.play.pause = 1;
	if (ioctl(audiofd, AUDIO_SETINFO, &pinfo) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	/*
	 *  set info and flush
	 */
	if (ioctl(audiofd, AUDIO_SETINFO, info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	if (ioctl(audiofd, I_FLUSH, FLUSHRW) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "I_FLUSH", SYSMSG);
	}
	TRACE_OUT
}

/*
 *  Play open.  Open audio device for write (play), pause, 
 * flush and setinfo.
 */
void
play_open(info)
audio_info_t	*info;
{
	audio_info_t	pinfo;

	func_name = "play_open";
	TRACE_IN

	/*
	 *  open audio device
	 */
	if ((audiofd = open(device_name, O_WRONLY|O_NDELAY)) < 0) {
		send_message(ERSYS, ERROR, er_open, device_name, SYSMSG);
	}

	/*
	 *  pause play
	 */
	AUDIO_INITINFO(&pinfo);
	pinfo.play.pause = 1;
	if (ioctl(audiofd, AUDIO_SETINFO, &pinfo) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	/*
	 *  set info and flush
	 */
	if (ioctl(audiofd, AUDIO_SETINFO, info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "AUDIO_SETINFO", SYSMSG);
	}	

	if (ioctl(audiofd, I_FLUSH, FLUSHW) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "I_FLUSH", SYSMSG);
	}
	TRACE_OUT
}


/*
 *  Do a DBRI GETINFO ioctl and print results.
 */

print_auinfo()
{
	int		fd;
	audio_info_t	info;

	if (ioctl(audioctlfd, AUDIO_GETINFO, &info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl,
			"AUDIO_GETINFO", SYSMSG);
	}	

	send_message(0, DEBUG, "info.play.sample_rate = %d", 
		info.play.sample_rate);
	send_message(0, DEBUG, "info.play.channels = %d", 
		info.play.channels);
	send_message(0, DEBUG, "info.play.precision = %d", 
		info.play.precision);
	send_message(0, DEBUG, "info.play.encoding = %d", 
		info.play.encoding);
	send_message(0, DEBUG, "info.play.gain = %d", info.play.gain);
	send_message(0, DEBUG, "info.play.port = %d", info.play.port);
	send_message(0, DEBUG, "info.play.avail_ports = %d", 
		info.play.avail_ports);
	send_message(0, DEBUG, "info.play.samples = %d", info.play.samples);
	send_message(0, DEBUG, "info.play.eof = %d", info.play.eof);
	send_message(0, DEBUG, "info.play.pause = %d", info.play.pause);
	send_message(0, DEBUG, "info.play.error = %d", info.play.error);
	send_message(0, DEBUG, "info.play.waiting = %d", info.play.waiting);
	send_message(0, DEBUG, "info.play.minordev = %d", info.play.minordev);
	send_message(0, DEBUG, "info.play.open = %d", info.play.open);
	send_message(0, DEBUG, "info.play.active = %d", info.play.active);
	send_message(0, DEBUG, "\ninfo.record.sample_rate = %d", 
		info.record.sample_rate);
	send_message(0, DEBUG, "info.record.channels = %d", 
		info.record.channels);
	send_message(0, DEBUG, "info.record.precision = %d", 
		info.record.precision);
	send_message(0, DEBUG, "info.record.encoding = %d", 
		info.record.encoding);
	send_message(0, DEBUG, "info.record.gain = %d", info.record.gain);
	send_message(0, DEBUG, "info.record.port = %d", info.record.port);
	send_message(0, DEBUG, "info.record.avail_ports = %d", 
		info.record.avail_ports);
	send_message(0, DEBUG, "info.record.samples = %d", 
		info.record.samples);
	send_message(0, DEBUG, "info.record.eof = %d", info.record.eof);
	send_message(0, DEBUG, "info.record.pause = %d", info.record.pause);
	send_message(0, DEBUG, "info.record.error = %d", info.record.error);
	send_message(0, DEBUG, "info.record.waiting = %d", 
		info.record.waiting);
	send_message(0, DEBUG, "info.record.minordev = %d", 
		info.record.minordev);
	send_message(0, DEBUG, "info.record.open = %d", info.record.open);
	send_message(0, DEBUG, "info.record.active = %d", info.record.active);
	send_message(0, DEBUG, "\ninfo.monitor_gain = %d", info.monitor_gain);
	send_message(0, DEBUG, "info.speaker_muted = %d", info.speaker_muted);
}


/*
 *  Dump signal to file, used for debug.
 */
int
dump_sig(sigptr, sample_cnt, fname)
short	*sigptr;
int	sample_cnt;
char	*fname;
{
	int	i, fd;
	short	*writeptr;

	func_name = "dump_sig";
	TRACE_IN

	/*
	 *	open the file
	 */
	if ((fd = open(fname, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1 ) {
		send_message(ERSYS, ERROR, er_open, fname, SYSMSG);
	}

	/*
	 *  map the file
	 */
	if (ftruncate(fd, sizeof(short) * sample_cnt) == -1 ) {
		send_message(ERSYS, ERROR, er_truncate, SYSMSG);
	}

	if ((writeptr = (short *) mmap ((caddr_t) 0, sizeof(short) * sample_cnt,
			(PROT_READ | PROT_WRITE), 
			MAP_SHARED, fd, (off_t) 0)) == (short *)-1 ) {
		send_message(ERSYS, ERROR, er_mmap, SYSMSG);
	}
	close(fd);

	for (i = 0; i < sample_cnt; i++) {
		*writeptr++ = *sigptr++;
	}
	send_message(0, DEBUG, dump_msg, fname);

	(void) munmap(writeptr, sample_cnt);

	TRACE_OUT
}


/*
 *  Create a filename - used for dumping audio files.
 */
static char fname[256];

char *
aufname(bn, auptr)
char	*bn;
au_data_t	*auptr;
{

	sprintf(fname, "%s-%d-%d-%d-%d-%s.sig", bn, auptr->sample_rate,
		auptr->lfreq, auptr->rfreq, auptr->channels,
		encodestr[auptr->encoding]);
	return(fname);
}
