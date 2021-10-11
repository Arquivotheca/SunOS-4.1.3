#ident   "@(#)audbri_tests.c 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */

/*
 *  Audio tests for the DBRI/MMCODEC used on 'Campus2' and 
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

extern	int	param_debug,gen_file;
extern	double	lrms,rrms,loffset,roffset;
FILE	*deb_fp;
char	deb_file[] = "fft.debug";

/*
 * Loopback Calibration
 *
 *  Determine the output gain required to deliver a valid input signal
 * when in loopback configuration.  Verify that the audio system delivers
 * adequate power.  Determine reference values to be used in subsequent
 * tests for a given loopback configuration.  A reference gain and RMS 
 * are determined and an FFT is performed on the recorded signal and 
 * the magnitude values are saved in a file.
 */
int 
audbri_cal()
{
	audio_info_t	ai;
	short		*play_sig, *record_sig, *sine_sig;
	int		i, sample_cnt, clipped, pgain, stop;
	int		isclip,retry;
	int		omask;
	double		rms;
	au_data_t	*aup;

	func_name = "audbri_cal";
	TRACE_IN


	omask = enter_critical();
	/*
	 *  Mmap the reference file.
	 */
	map_ref((refname ? refname : ldata[ltype].refname), 
		ldata[ltype].audata);

	/*
	 *  configure audio according to loopback type selection
	 */
	AUDIO_INITINFO(&ai);

	ai.play.port = ldata[ltype].play_port;
	ai.record.port = ldata[ltype].record_port;

	/*
	 *  initialize common audio parameters
	 */

	ai.monitor_gain = AUDIO_MIN_GAIN;
 
	loopback_open(&ai);

	/*
	 *  do calibration for all sample rates
	 */
	for (i = 0; ldata[ltype].audata[i].sample_rate != NULL; i++) {
		aup = &ldata[ltype].audata[i];

		/*
		 *  set sample rate and gain
		 */
		AUDIO_INITINFO(&ai);
		ai.play.channels = aup->channels;
		ai.record.channels = aup->channels;
		ai.play.precision = aup->precision;
		ai.record.precision = aup->precision;
		ai.play.encoding = aup->encoding;
		ai.record.encoding = aup->encoding;
		pgain = aup->cal_pgain_max;
		ai.play.gain = aup->cal_pgain_max;
		ai.record.gain = aup->cal_rgain;	
		ai.play.sample_rate = aup->sample_rate;
		ai.record.sample_rate = aup->sample_rate;
		if (ioctl(audiofd, AUDIO_SETINFO, &ai) < 0) {
			free(sine_sig);
			close(audiofd);
			send_message(ERSYS, ERROR, er_ioctl, 
				"AUDIO_SETINFO", SYSMSG);
		}
		send_message(0, VERBOSE+DEBUG, au_msg, aup->sample_rate,
			aup->lfreq, aup->rfreq, aup->channels, pgain,
			aup->cal_rgain, aup->precision, 
			encodestr[aup->encoding]);
		
		/*
		 *  Create a signal to play based upon the au_data_t
		 * info.
		 */
		if (!(sample_cnt = gensig(aup, &play_sig, duration))) { 
			close(audiofd);
			TRACE_OUT
			return(1);
		}

		/*
		 *  play and record
		 */
		playrecord(play_sig, &record_sig, sample_cnt, duration, aup);
	
		/*
		 *  if the recorded signal isn't clipped then fail
		 * (signal should be clipped at this point because
		 * the gain is at maximum for play and record)
		 */
		if (isclipped(record_sig, sample_cnt, aup) == FALSE) { 
			send_message(ERAUDIO, ERROR, er_notclip);
		}
	
		free(record_sig);

		/*
		 *  determine the optimal play gain setting such that
		 * the recorded signal is just below being clipped
		 */
		 clipped = TRUE;
		 stop = FALSE;
		 do {
			/*
			 *  reduce play gain
			 */
			AUDIO_INITINFO(&ai);
			pgain -= gain_step;
			ai.play.gain = pgain;
			send_message(0, DEBUG, pgain_msg, pgain);
			if (ioctl(audiofd, AUDIO_SETINFO, &ai) < 0) {
				send_message(ERSYS, ERROR, er_ioctl, 
						"AUDIO_SETINFO", SYSMSG);
			}
	
			/*
			 *  play and record
			 */
			playrecord(play_sig, &record_sig, sample_cnt, 
				duration, aup);

			if (dump_signals) {
				dump_sig(play_sig, sample_cnt, 
					aufname("cal-play", aup));
				dump_sig(record_sig, sample_cnt, 
					aufname("cal-rec", aup));
			}

			/*
			 *  if the recorded signal is clipped then we're done
			 */
			if (isclipped(record_sig, sample_cnt, aup) == FALSE) {
				/*
				 *  Go one "gain_step" beyond the
				 * unclipped gain so that we'll
				 * have a bit of tolerance.
				 */
				if (!clipped) {
					stop = TRUE;
					if (ai.play.port == AUDIO_SPEAKER)
					    pgain -= (gain_step); 
					else
					    pgain -= (4*gain_step);
				}
				else {
					clipped = FALSE;
				}
			}
			else {
				free(record_sig);
			}
		/*
		 *  exit if we've gone below the minimum play gain
		 */
		} while ((pgain > aup->cal_pgain_min) && !stop);
	
		/*
		 *  if the signal's still clipped, fail
		 */
		if (clipped) {
			free(record_sig);
			send_message(ERAUDIO, ERROR, er_auclip);
		}
	
		/*
		 * generate and save calibration data
		 *
		 *  Note that we offset the recorded signal to skip 
		 * garbage at the beginning and use a sample size of 
		 * ref_size.
		 */
		genref(&record_sig[calc_offset(aup)], ref_size, pgain, aup, i);
	
		free(play_sig);
		free(record_sig);
	}

	close(audiofd);
	unset_ref();

	exit_critical(omask);
	TRACE_OUT
	return(0);
}


/*
 * Loopback Test
 *
 *  Verify that the specified looback channel can reproduce a 
 * signal of given frequency and amplitude.
 */
int 
audbri_loop()
{
	ref_data_t	*refp;
	audio_info_t	ai;
	short		*play_sig, *record_sig;
	int		i, sample_cnt, fftsize, rdsize;
	au_data_t	*aup;
	double	lfreq,rfreq,sample,snr,thd,sep,channels;
	char	mode[8];
	extern	double	calc_snr(),calc_thd(),calc_sep();

	func_name = "audbri_loop";
	TRACE_IN

	if (gen_file) {
	    deb_fp = fopen(deb_file,"w");
	    if (deb_fp == NULL) {
		printf("Cannot open %s\n",deb_file);
		exit(1);
	    }
	}

	/*
	 *  configure audio 
	 */
	AUDIO_INITINFO(&ai);
	ai.play.port = ldata[ltype].play_port;
	ai.record.port = ldata[ltype].record_port;

	ai.monitor_gain = AUDIO_MIN_GAIN;
 
	/*
	 *  open audio device and set parameters
	 */
	loopback_open(&ai);

	/*
	 *  do loopback test for all sample rates
	 */
	for (i = 0; ldata[ltype].audata[i].sample_rate != NULL; i++) {
		aup = &ldata[ltype].audata[i];

		/*
		 *  Get pointer to reference data.  There's actually 
		 * reference data for left and right channels (for MONO
		 * only the left is used) but the reference play and 
		 * records gains are the same for both channels.
		 */
		fftsize = ref_size/2 + 1;
		rdsize = fftsize * sizeof(double) + sizeof(ref_data_t);
		refp = (ref_data_t *) ((caddr_t) refptr + (rdsize * 2 * i));

		AUDIO_INITINFO(&ai);
		ai.play.gain = refp->play_gain;
		ai.record.gain = refp->record_gain;
		ai.play.channels = aup->channels;
		ai.record.channels = aup->channels;
		ai.play.precision = aup->precision;
		ai.record.precision = aup->precision;
		ai.play.encoding = aup->encoding;
		ai.record.encoding = aup->encoding;
		ai.play.sample_rate = aup->sample_rate;
		ai.record.sample_rate = aup->sample_rate;
		if (ioctl(audiofd, AUDIO_SETINFO, &ai) < 0) {
			send_message(ERSYS, ERROR, er_ioctl, 
				"AUDIO_SETINFO", SYSMSG);
		}

		sample = aup->sample_rate;
		lfreq = aup->lfreq;
		rfreq = aup->rfreq;
		channels = aup->channels;

		send_message(0, VERBOSE+DEBUG, au_msg, aup->sample_rate,
			aup->lfreq, aup->rfreq, aup->channels, refp->play_gain,
			refp->record_gain, aup->precision, 
			encodestr[aup->encoding]);

		/*
		 *  generate sine wave signal for both channels
		 */
		if (!(sample_cnt = gensig(aup, &play_sig, duration))) {
			close(audiofd);
			TRACE_OUT
			return(1);
		}

		/*
		 *  play and record
		 */
		playrecord(play_sig, &record_sig, sample_cnt, duration, aup);

		if (dump_signals) {
			dump_sig(play_sig, sample_cnt, 
				aufname("loop-play", aup));
			dump_sig(record_sig, sample_cnt, 
				aufname("loop-rec", aup));
		}

		/*
		 *  verify frequency response and RMS of recorded signal
		 *
		 *  Note that we offset the recorded signal to skip 
		 * garbage at the beginning and use a sample size of 
		 * ref_size.
		 */

		if (sig_valid(refp, &record_sig[calc_offset(aup)], 
				ref_size, aup) == FALSE) {
			send_message(ERAUDIO, ERROR, er_notvalid);
		}

		sig_valid(refp, &record_sig[calc_offset(aup)],ref_size, aup);
		free(record_sig);
		free(play_sig);

	    if (param_debug) {
		snr = calc_snr(LEFT);
		thd = calc_thd(LEFT);
		if (channels == MONO) {
		    printf("Sample Rate =%6.0f Hz, Freq =%5.0f Hz\n",sample,lfreq);
		    printf("        snr =%7.2f dB, thd =%8.2f dB, rms =%7.2f dB, off =%6.2f %%\n",
				snr,thd,lrms,loffset);
		    if (gen_file) {
		        fprintf(deb_fp,"Sample Rate =%6.0f Hz, Freq =%5.0f Hz\n",sample,lfreq);
		        fprintf(deb_fp,"        snr =%7.2f dB, thd =%8.2f dB, rms =%7.2f dB, off =%6.2f %%\n",
				snr,thd,lrms,loffset);
		    }
		}
		else {
		    printf("Sample Rate =%6.0f Hz, left =%5.0f Hz,  right =%5.0f Hz"
			,sample,lfreq,rfreq);
		    if (gen_file) {
		        fprintf(deb_fp,"Sample Rate =%6.0f Hz, left =%5.0f Hz,  right =%5.0f Hz"
				,sample,lfreq,rfreq);
		    }

		    if ((lfreq == 0) || (rfreq == 0)) {
		        if (lfreq == 0) {
		            sep = calc_sep(RIGHT);
		        }
		        if (rfreq == 0) {
		            sep = calc_sep(LEFT);
		        }
			printf(", sep =%7.2f dB\n",sep);
			if (gen_file) {
			    fprintf(deb_fp,", sep =%7.2f dB\n",sep);
			}
		    }
		    else {
		        printf("\n");
			if (gen_file) {
		            fprintf(deb_fp,"\n");
			}
		    }
		    printf("left  : snr =%7.2f dB, thd =%8.2f dB, rms =%7.2f dB, off =%6.2f %%\n",
				snr,thd,lrms,loffset);
		    if (gen_file) {
		        fprintf(deb_fp,"left  : snr =%7.2f dB, thd =%8.2f dB, rms =%7.2f dB, off =%6.2f %%\n",
				snr,thd,lrms,loffset);
		    }
		    snr = calc_snr(RIGHT);
		    thd = calc_thd(RIGHT);
		    printf("right : snr =%7.2f dB, thd =%8.2f dB, rms =%7.2f dB, off =%6.2f %%\n",
				snr,thd,rrms,roffset);
		    if (gen_file) {
		        fprintf(deb_fp,"right : snr =%7.2f dB, thd =%8.2f dB, rms =%7.2f dB, off =%6.2f %%\n",
				snr,thd,rrms,roffset);
		    }
		}
	    }
	}

	close(audiofd);

	if (gen_file) {
	    fclose(deb_fp);
	}

	TRACE_OUT
	return(0);
}


/*
 * Crystal Test
 *
 *  Verify that the timing of the audio channel is correct at 
 * each of the sample rates.
 */

int 
audbri_crystal()
{
	audio_info_t	ai;
	short		*play_sig;
	int		i, size, duration_usec, tolerance;
	int		upper_limit, lower_limit;
	int		time;

	func_name = "audbri_crystal";
	TRACE_IN

	/*
	 *  calculate upper and lower limits for timing
	 */
	duration_usec = (duration * 1000000);
	tolerance = nint(aucrystal_tol * (double) duration_usec);
	upper_limit = duration_usec + tolerance;
	lower_limit = duration_usec - tolerance;

	/*
	 * initialize  audio parameters
	 */
	AUDIO_INITINFO(&ai);
	ai.play.port = AUDIO_SPEAKER;
	ai.monitor_gain = AUDIO_MIN_GAIN;

	/*
	 *  open audio device and set parameters
	 */
	play_open(&ai);
 

	/*
	 *  test timing for each frequency
	 */
	for (i = 0; crystal_audata[i].sample_rate != NULL; i++) {
		send_message(0, DEBUG, sr_msg, crystal_audata[i].sample_rate);

		/*
		 *  generate a signal to play
		 */
		if (!(size = gensig(&crystal_audata[i], &play_sig, duration))) {
			close(audiofd);
			TRACE_OUT
			return(1);
		}

		/*
		 * set play audio parameters
		 */
		AUDIO_INITINFO(&ai);
		ai.play.sample_rate = crystal_audata[i].sample_rate;
		ai.play.channels = crystal_audata[i].channels;
		ai.play.precision = crystal_audata[i].precision;
		ai.play.encoding = crystal_audata[i].encoding;
		ai.play.gain = crystal_audata[i].cal_pgain_max;
		if (ioctl(audiofd, AUDIO_SETINFO, &ai) < 0) {
			send_message(ERSYS, ERROR, er_ioctl, 
				"AUDIO_SETINFO", SYSMSG);
		}	

		/*
		 *  play a signal and time it
		 */
		time = play(play_sig, size, crystal_audata[i].precision);
		free(play_sig);		/* all done with the signal */

		/*
		 * verify crystal tolerance
		 */
		if (time < lower_limit || time > upper_limit) {
			close(audiofd);
			TRACE_OUT
			send_message(ERAUDIO, ERROR, er_aucrystal, 
				crystal_audata[i].sample_rate, time);
		}
		else {
			send_message(0, VERBOSE+DEBUG, crystal_msg, 
				crystal_audata[i].sample_rate, time);
		}
	}
	close(audiofd);
	TRACE_OUT
	return(0);
}


/*
 *  SIGPOLL handler used by audbri_controls()
 */
sigpollhandler(sig, code, scp, addr)
int			sig, code;
struct sigcontext       *scp;
char			*addr;
{
	audio_info_t	ai;

	func_name = "sigpollhandler";
	TRACE_IN

	/*
	 *  get audio state
	 */
	if (ioctl(audioctlfd, AUDIO_GETINFO, &ai) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, 
				"AUDIO_GETINFO", SYSMSG);
	}

	switch (dc_flag) {

	  case 0:
		/*
		 *  If volume went down, tell the user to press volume up
		 */
		 if (ai.play.gain < audio_info.play.gain) {
			audio_info = ai;
			dc_flag = 1;
			send_message(0, DEBUG+VERBOSE, vup_msg);
		}
		break;

	  case 1:
		/*
		 *  If volume went up, tell the user to press mute
		 */
		 if (ai.play.gain > audio_info.play.gain) {
			audio_info = ai;
			dc_flag = 2;
			send_message(0, DEBUG+VERBOSE, mute_msg);
		}
		break;

	  case 2:
		/*
		 *  If mute is on, tell user to press it "unmute"
		 */
		if (ai.speaker_muted) {
			audio_info = ai;
			dc_flag = 3;
			send_message(0, DEBUG+VERBOSE, unmute_msg);
		}
		break;

	  case 3:
		/*
		 *  If mute is off, control test passed
		 */
		if (!ai.speaker_muted) {
			audio_info = ai;
			dc_flag = 4;
			send_message(0, DEBUG+VERBOSE, ctlpass_msg);
		}
		break;
	}
	TRACE_OUT
}


/*
 * Controls Test
 *
 *  Determine 
 */
int 
audbri_controls()
{
	audio_info_t	ai;
	short		*play_sig;
	struct itimerval	time;
	int		fd, precision;
	struct stat	statbuf;
	Audio_filehdr	*ah;

	func_name = "audbri_controls";
	TRACE_IN

	/*
	 *	open the music file, get it's size and map it
	 */
	if ((fd = open(music_file, O_RDONLY)) == -1 ) {
		send_message(ERSYS, ERROR, er_open, music_file, SYSMSG);
	}

	if (fstat(fd, &statbuf) == -1) {
		send_message(ERSYS, ERROR, er_stat, SYSMSG);
	}

	if ((ah = (Audio_filehdr *)mmap((caddr_t) 0, statbuf.st_size, 
			PROT_READ, MAP_SHARED, 
			fd, (off_t) 0)) == (Audio_filehdr *) -1) {
		send_message(ERSYS, ERROR, er_mmap, SYSMSG);
	}
	close(fd);

	/*
	 *  Make sure it's a valid audio file.
	 */
	if (ah->magic != AUDIO_FILE_MAGIC) {
		send_message(ERAUDIO, ERROR, er_afile);
	}

	/*
	 * set play and monitor gain, open audio device
	 */
	AUDIO_INITINFO(&ai);
	ai.play.channels = ah->channels;

	switch(ah->encoding) {
	  case AUDIO_FILE_ENCODING_MULAW_8:
		ai.play.precision = 8;
		ai.play.encoding = AUDIO_ENCODING_ULAW;
		break;

	  case AUDIO_FILE_ENCODING_LINEAR_16:
		ai.play.precision = 16;
		ai.play.encoding = AUDIO_ENCODING_LINEAR;
		break;

	  default:
		send_message(ERAUDIO, ERROR, er_afile);
		break;
	}
	precision = ai.play.precision;
	ai.play.gain = music_gain;
	ai.play.port = AUDIO_SPEAKER;
	ai.play.sample_rate = ah->sample_rate;
	ai.monitor_gain = AUDIO_MIN_GAIN;
	play_open(&ai);

	/*
	 *  get current audio state
	 */
	if (ioctl(audioctlfd, AUDIO_GETINFO, &audio_info) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, 
				"AUDIO_GETINFO", SYSMSG);
	}

	/*
	 *  initialize controls test "state" flag
	 */
	dc_flag = 0;

	/*
	 *  set up to catch volume change and mute
	 */
	signal(SIGPOLL, sigpollhandler);
	if (ioctl(audioctlfd, I_SETSIG, S_MSG) < 0) {
		send_message(ERSYS, ERROR, er_ioctl, "I_SETSIG", SYSMSG);
	}
 
	/*
	 *  tell user to press volume down
	 */
	send_message(0, DEBUG+VERBOSE, vdown_msg);

	/*
	 *  play the file's data
	 */
	play_sig = (short *) ((caddr_t) ah + ah->hdr_size);
	(void) play(play_sig, ah->data_size, precision);

	/*
	 *  unmap
	 */
	(void) munmap(ah, statbuf.st_size);

	close(audiofd);
	close(audioctlfd);

	if (dc_flag == 4) {
		TRACE_OUT
		return(0);
	}
	else {
		TRACE_OUT
		return(1);
	}
}

/*
 * Music Test
 *
 */
int 
audbri_music()
{
	audio_info_t	ai;
	short		*play_sig;
	struct itimerval	time;
	int		fd, precision;
	struct stat	statbuf;
	Audio_filehdr	*ah;

	func_name = "audbri_music";
	TRACE_IN

	/*
	 *	open the music file, get it's size and map it
	 */
	if ((fd = open(music_file, O_RDONLY)) == -1 ) {
		send_message(ERSYS, ERROR, er_open, music_file, SYSMSG);
	}

	if (fstat(fd, &statbuf) == -1) {
		send_message(ERSYS, ERROR, er_stat, SYSMSG);
	}

	if ((ah = (Audio_filehdr *)mmap((caddr_t) 0, statbuf.st_size, 
			PROT_READ, MAP_SHARED, 
			fd, (off_t) 0)) == (Audio_filehdr *) -1) {
		send_message(ERSYS, ERROR, er_mmap, SYSMSG);
	}
	close(fd);

	/*
	 *  Make sure it's a valid audio file.
	 */
	if (ah->magic != AUDIO_FILE_MAGIC) {
		send_message(ERAUDIO, ERROR, er_afile);
	}

	/*
	 * set play and monitor gain, open audio device
	 */
	AUDIO_INITINFO(&ai);
	ai.play.channels = ah->channels;

	switch(ah->encoding) {
	  case AUDIO_FILE_ENCODING_MULAW_8:
		ai.play.precision = 8;
		ai.play.encoding = AUDIO_ENCODING_ULAW;
		break;

	  case AUDIO_FILE_ENCODING_LINEAR_16:
		ai.play.precision = 16;
		ai.play.encoding = AUDIO_ENCODING_LINEAR;
		break;

	  default:
		send_message(ERAUDIO, ERROR, er_afile);
		break;
	}
	precision = ai.play.precision;
	ai.play.gain = music_gain;
	ai.play.port = AUDIO_SPEAKER;
	ai.play.sample_rate = ah->sample_rate;
	ai.monitor_gain = AUDIO_MIN_GAIN;
	play_open(&ai);

	/*
	 *  play the file's data
	 */
	play_sig = (short *) ((caddr_t) ah + ah->hdr_size);
	(void) play(play_sig, ah->data_size, precision);

	/*
	 *  unmap
	 */
	(void) munmap(ah, statbuf.st_size);

	close(audiofd);
	close(audioctlfd);

	TRACE_OUT
	return(0);

}

/*
 * This function is called just before executing
 * some critical part of the code. The SIGINT
 * signal when/if received while executing this
 * part of the code will be temporarily blocked
 * and serviced at a later stage(in exit_critical()).
 */
enter_critical()
{
	int oldmask=0;
        oldmask = sigblock(sigmask(SIGINT));
	return(oldmask);
}
 
/*
 * This function is called just after executing
 * some critical part of the code. The SIGINT
 * signal will be  unblocked and the old signal
 * mask will be restored, so that the interrupts
 * may be serviced as originally intended.
 */
exit_critical(omask)
int omask;
{
        (void) sigsetmask(omask);
}
