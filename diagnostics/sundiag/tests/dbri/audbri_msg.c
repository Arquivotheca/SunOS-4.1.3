#ident   "@(#)audbri_msg.c 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */

/*
 *  Error messages for the DBRI/MMCODEC audio tests.
 */

char  *test_usage_msg = "[CDFILSTX]\n\
    C (Loopback Calibration)\n\
    D (Audio Device: D=/dev/audio)\n\
    F (Reference File: F=<path>)\n\
    I (Audio IOCTL Device: I=/dev/audioctl)\n\
    L (Loopback Test)\n\
    M (Music Test)\n\
    S (Speakerbox Controls and Mute Indicator Test)\n\
    T (Calibration/Loopback Test Type: [T=0|1|2])\n\
      0=Speaker/Microphone\n\
      1=Line-in/Line-out\n\
      2=Line-in/Headphone\n\
    X (Audio Crystal Test)\n";

char *altth_msg = "Using alternate threshold";
char *vdown_msg = "Press Volume down button on speakerbox";
char *vup_msg = "Press Volume up button on speakerbox";
char *mute_msg = "Press Mute button on speakerbox";
char *unmute_msg = "Press Mute button on speakerbox again";
char *ctlpass_msg = "Controls test passed";
char *dump_msg = "Dumped signal into file %s";
char *marker_msg = "XXX - Line %d of file %s";

char *clip_msg = "Signal is clipped";
char *noclip_msg = "Signal not clipped, amplitude = %d";
char *lclip_msg = "Left channel of signal is clipped";
char *rclip_msg = "Right channel of signal is clipped";
char *left_msg = "Left channel";
char *right_msg = "Right channel";

char *failed_msg = "failed.";
char *err_limit_msg= "failed due to exceeding error limit.";
char *ref_msg = "Play Gain = %d, Record Gain = %d, RMS = %f";
char *rms_msg = "RMS = %f, Ref RMS = %f, Lower = %f, Upper = %f";
char *tmprms_msg = "Composite RMS = %f";
char *tmpmag_msg = "Composite FFT mag = %f";
char *fft_msg = "FFT %s Band %d Hz - %d Hz VALID TestBandRMS = %f, RefBandRMS = %f, Threshold = %f";
char *rrms_msg = "Ref RMS = %f, Lower = %f, Upper = %f";
char *rfft_msg = "FFT %s Band %d Hz - %d Hz RefBandRMS = %f, Threshold = %f";
char *au_msg = "Sample Rate = %d, L-Freq = %d, R-Freq = %d, Channels = %d, Play Gain = %d, Record Gain = %d, Precision = %d, Encoding = %s";
char *pgain_msg = "Play Gain = %d";
char *rgain_msg = "Record Gain = %d";
char *sr_msg = "Sample Rate = %d";
char *drain_msg = "Doing AUDIO_DRAIN";
char *aioread_msg = "aioread() result.aio_return = %d";
char *aiowrite_msg = "aiowrite() result.aio_return = %d";
char *crystal_msg = "Audio crystal test, rate = %d Hz, time = %d usecs";


char *er_device = "Invalid audio device: %s";
char *er_open = "Failed to open %s (%s)";
char *er_ioctl = "Failed ioctl %s (%s)";
char *er_setitimer = "Failed setitimer (%s)";
char *er_mmap = "Failed mmap (%s)";
char *er_stat = "Failed fstat (%s)";
char *er_truncate = "Failed ftruncate (%s)";
char *er_write = "Failed to write to audio device (%s)";
char *er_read = "Failed to read from audio device (%s)";
char *er_aiowrite = "Failed aiowrite to audio device (%s)";
char *er_aioread = "Failed aioread from audio device (%s)";
char *er_aiowritep = "aiowrite completed only %d bytes out of %d";
char *er_aioreadp = "aioread completed only %d bytes out of %d";
char *er_aiowait = "Failed aiowait (%s)";
char *er_readf = "Failed to read of data file (%s)";
char *er_mem = "Failed to allocate %d bytes of memory (%s)";
char *er_ur = "Play underrun on audio device";
char *er_or = "Record overrun on audio device";
char *er_auclip = "Signal still clipped";
char *er_aucrystal = "Audio crystal failed, rate = %d Hz, time = %d usecs, could be due to system overload.  Disable all other tests and try again";
char *er_notclip = "Signal not clipped";
char *er_badlbtype = "Invalid loopback test";
char *er_notvalid = "Signal is invalid";
char *er_lnotvalid = "Left channel of signal is invalid";
char *er_rnotvalid = "Right channel of signal is invalid";
char *er_fftmag = "FFT calculation failed";
char *er_rms = "RMS = %f out of tolerance, Lower Limit = %f, Upper Limit = %f, Reference RMS = %f";
char *er_fft = "FFT %s Band %d Hz - %d Hz INVALID TestBandRMS = %f, RefBandRMS = %f, Threshold = %f";
char *er_refspec = "Reference file name must be specified for calibration";
char *er_afile = "Invalid audio file format";
char *er_precision = "A-law and u-law require 8 bit precision";
char *er_encoding = "Invalid audio encoding";
