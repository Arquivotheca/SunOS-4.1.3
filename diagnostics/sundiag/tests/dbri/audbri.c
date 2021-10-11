#ident   "@(#)audbri.c 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */

/*
 *  Main file for the DBRI/MMCODEC audio test.  It has the all the 
 * Sundiag interface routines.  The DBRI and MMCODEC devices are 
 * used on 'Campus2' and 'Synergy' platforms and Sbus 'Batphone' card.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/asynch.h>
/* #include <sun/audioio.h> ...not there yet... */
#include "audioio.h" 

#include "sdrtns.h"
#include "../../../lib/include/libonline.h"

#include "audbri.h"
#include "audbri_msg.h"

static int	lt = 1;

int	high_freq = 0;	/* The highest test frequency requird */
int	param_debug = 0;
int	gen_file = 0;	/* Generate the fft mag file */
extern	band_data_t bdata_48000[];
extern	au_data_t sbmic_audata[],loli_audata[],hdli_audata[],crystal_audata[];

main(argc, argv)
int	argc;
char	*argv[];
{
	versionid = "%I";
	device_name = DEVICE_NAME;

	/*
	 * SunDiag initialization
	 */
	test_init(argc, argv, process_test_args, routine_usage, 
		test_usage_msg); 

	/*
	 * make sure device is there before trying to run test
	 */
	if (!exec_by_sundiag) {
		valid_dev_name();
		probe_audbridev();
	}

	/*
	 * run tests
	 */
	run_tests(single_pass ? 1 : TOTAL_PASS);

	/*
	 * clean up and exit
	 */
	clean_up();
	test_end();				
}

/**
 * Process_test_args() processes test-specific command line aguments.
 */
process_test_args(argv, argindex)
char *argv[];
int  argindex;
{
int	i;

	if (strcmp(argv[argindex], "C") == 0) {
		calibration_test = TRUE;
	}

	else if (strncmp(argv[argindex], "D=", 2) == 0) {
		device_name = argv[argindex]+2;
	}

	else if (strncmp(argv[argindex], "F=", 2) == 0) {
		refname = argv[argindex]+2;
	}

	else if (strncmp(argv[argindex], "G", 2) == 0) {
		gen_file = 1;
	}

	else if (strncmp(argv[argindex], "H=", 2) == 0) {
		high_freq = atoi(argv[argindex]+2);
		bdata_48000[0].end_freq = high_freq/8;
		bdata_48000[1].start_freq = bdata_48000[0].end_freq+1;
		bdata_48000[1].end_freq = (bdata_48000[1].end_freq*high_freq)/48000;
		bdata_48000[2].start_freq = bdata_48000[1].end_freq+1;
		bdata_48000[2].end_freq = (bdata_48000[2].end_freq*high_freq)/48000;
		bdata_48000[3].start_freq = bdata_48000[2].end_freq+1;

		for (i = 0; sbmic_audata[i].sample_rate; i++) {
		    if (sbmic_audata[i].sample_rate == 48000) break;
		}
		sbmic_audata[i].sample_rate = high_freq;
		sbmic_audata[i].lfreq = high_freq/8;

		for (i = 0; loli_audata[i].sample_rate; i++) {
		    if (loli_audata[i].sample_rate == 48000) break;
		}
		loli_audata[i].sample_rate = high_freq;
		loli_audata[i].lfreq = high_freq/8;
		loli_audata[i].rfreq = high_freq/8;
		loli_audata[i+1].sample_rate = high_freq;
		loli_audata[i+1].rfreq = high_freq/8;
		loli_audata[i+2].sample_rate = high_freq;
		loli_audata[i+2].lfreq = high_freq/8;

		for (i = 0; hdli_audata[i].sample_rate; i++) {
		    if (hdli_audata[i].sample_rate == 48000) break;
		}
		hdli_audata[i].sample_rate = high_freq;
		hdli_audata[i].lfreq = high_freq/8;
		hdli_audata[i].rfreq = high_freq/8;
		hdli_audata[i+1].sample_rate = high_freq;
		hdli_audata[i+1].rfreq = high_freq/8;
		hdli_audata[i+2].sample_rate = high_freq;
		hdli_audata[i+2].lfreq = high_freq/8;

		for (i = 0; crystal_audata[i].sample_rate; i++) {
		    if (crystal_audata[i].sample_rate == 48000) break;
		}
		crystal_audata[i].sample_rate = high_freq;

	}

	else if (strncmp(argv[argindex], "I=", 2) == 0) {
		ioctldev_name = argv[argindex]+2;
	}

	else if (strcmp(argv[argindex], "L") == 0) {
		loop_test = TRUE;
	}

	else if (strcmp(argv[argindex], "M") == 0) {
		music_test = TRUE;
	}

	else if (strcmp(argv[argindex], "P") == 0) {
		param_debug = TRUE;
	}

	else if (strcmp(argv[argindex], "S") == 0) {
		controls_test = TRUE;
	}

	else if (strncmp(argv[argindex], "T=", 2) == 0) {
		lt = atoi(argv[argindex]+2);
		if (lt < 0 || lt > 2) {
			return(FALSE);
		}
	}

	else if (quick_test || (strcmp(argv[argindex], "X") == 0)) {
		crystal_test = TRUE;
	}

	else if (strcmp(argv[argindex], "Z") == 0) {
		dump_signals = TRUE;
	}

	else {
		return (FALSE);
	}

   return (TRUE);
}

/*
 * routine_usage() explain the meaning of each test-specific command argument.
 */
routine_usage()
{
  send_message(0, CONSOLE, "%s specific arguments [default is S]:\n%s",
	test_name, test_usage_msg);
}


/*
 * You may also want to consider validating the device name, if your test is
 * also to be run standalone (i.e., without sundiag) under Unix.
 */
valid_dev_name()
{
   func_name = "valid_dev_name";
   TRACE_IN
   TRACE_OUT
   return (0);
}

/*
 * The probing function should check that the specified device is available
 * to be tested(optional if run by Sundiag). Usually, this involves opening
 * the device file, and using an appropriate ioctl to check the status of the
 * device, and then closing the device file. There are several flavors of
 * ioctls: see dkio(4s), fbio(4s), if(4n), mtio(4), and so on. It is nice
 * to put the probe code into a separate code module, because it usually has
 * most of the code which needs to be changed for a new SunOS release or port.
 */
probe_audbridev()
{
	int	device_type;

	func_name = "probe_audbridev";
	TRACE_IN

	/*
	 *  open audio control device
	 */
	if ((audioctlfd = open(ioctldev_name, O_RDWR|O_NDELAY)) < 0) {
		send_message(ERSYS, ERROR, er_open, ioctldev_name, SYSMSG);
	}

	/*
	 *  See if this is an appropriate audio device.
	 */
        if (ioctl(audioctlfd, AUDIO_GETDEV, &device_type) < 0) {
                send_message(ERSYS, ERROR, er_ioctl,
                        "AUDIO_GETDEV", SYSMSG);
        }

	close(audioctlfd);

	if (device_type != AUDIO_DEV_SPEAKERBOX && 
			device_type != AUDIO_DEV_CODEC) {
                send_message(ERAUDIO, ERROR, er_device, ioctldev_name);
        }

	TRACE_OUT
	return(0);
}

/*
 * Run the test while pass < total_pass.
 */
run_tests(total_pass)
int total_pass;
{
  int	pass=0;
  int	errors=0;

	func_name = "run_tests";
	TRACE_IN

	while (++pass <= total_pass) {
		send_message(0, VERBOSE, "Pass= %d, Error= %d", pass, errors);
		if (!audbridev_test()) {
			if (!run_on_error) {
				send_message(TEST_ERROR, ERROR, failed_msg);
			}
			else if (++errors >= ERROR_LIMIT) {
				send_message(TOO_MANY_ERRORS, FATAL, 
					err_limit_msg);
			}
		}
	}
	TRACE_OUT
}

/*
 * The actual test.  Return TRUE if the test passed, otherwise FALSE.
 */
int
audbridev_test()
{
	int errors = 0;
	func_name = "audbridev_test";
	TRACE_IN

	/*
	 *  open audio control device
	 */
	if ((audioctlfd = open(ioctldev_name, O_RDWR|O_NDELAY)) < 0) {
		send_message(ERSYS, ERROR, er_open, ioctldev_name, SYSMSG);
	}

	/*
	 *  Force the user to specify a reference file name when
	 * doing calibration so that they won't inadvertently
	 * remove a reference file.
	 */
	if (calibration_test == TRUE && refname[0] == ' ') {
		strcpy(refname,ldata[lt].refname);
/*		send_message(ERAUDIO, ERROR, er_refspec); */
	}

#ifdef	NEVER
	/*
	 *  If no test is selected then run 'Speakerbox Controls and Mute
	 * Indicator Test.
	 */
	if (!(calibration_test || loop_test || controls_test)) {
		crystal_test = TRUE;
	}
#endif	NEVER

	/*
	 * Run calibration test if requested.
	 */
	if (calibration_test == TRUE) {
		ltype = lt;
		errors += audbri_cal();
	}

	/*
	 * Run loop test if requested.
	 */
	if (loop_test == TRUE) {
		ltype = lt;
		set_ref();
		errors += audbri_loop();
		unset_ref();
	}

	if (music_test == TRUE) {
		errors += audbri_music();
	}

	/*
	 * Run crystal test if requested.
	 */
	if (crystal_test == TRUE) {
		errors += audbri_crystal();
	}

	/*
	 * Run controls test if requested.
	 */
	if (controls_test == TRUE) {
		errors += audbri_controls();
	}

	TRACE_OUT
	return(errors ? FALSE : TRUE);
}


/*
 * clean_up(), contains necessary code to clean up resources before exiting.
 * Note: this function is always required in order to link with sdrtns.c
 * successfully.
 */
clean_up()
{
  func_name = "clean_up";
  TRACE_IN
  TRACE_OUT
  return(0);
}
