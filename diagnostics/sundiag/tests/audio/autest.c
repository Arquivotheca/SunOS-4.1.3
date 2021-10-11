/*
 *	@(#)autest.c 1.1 92/07/30 SMI
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 *	Deyoung Hong
 */


#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>

/*
 * These local header files are copied from SunOS 4.0.3c so that the code can
 * be compiled on any Sun-4 architectures with SunOS 4.0.3.  However, under
 * SunOS 4.0.3c and 4.1, the system include files should be used instead.
 */
#ifdef	NEW
#define	OS_4_1			/* OS with new audio driver */
#define	AUDIO_CHIP
#define AMD_CHIP		/* add for new audio_79C30.h change */
#include <sun/audioio.h>
#include <sbusdev/audio_79C30.h>
#else
#define	OS_4_0_3		/* OS with older audio driver */
#ifdef	sun4c
#include <sun/audioio.h>
#include <sbusdev/audioreg.h>
#else
#include "audioio.h"
#include "audioreg.h"
#endif	sun4c
#endif	NEW

#include "sdrtns.h"
#include "../../../lib/include/libonline.h"


/* Program macros */
#define	DEVICENAME	"/dev/audio"		/* default device name */
#define	IOCTLDEVICE	"/dev/audioctl"		/* ioctl device name */
#define	DATA_FILE	"autest.data"		/* audio data file */
#define	VDEBUG		VERBOSE+DEBUG		/* verbose and debug */
#define	OUT_SPEAKER	0			/* output to speaker */
#define	OUT_JACK	1			/* output to jack */
#define	MIN_VOLUME	0			/* min volume */
#define	MAX_VOLUME	255			/* max volume */
#define	NORM_VOLUME	(MAX_VOLUME/2)		/* normal volume */
#define	MAXBYTE		0xFF			/* max byte value */
#define	QUICKDATALEN	0x10000			/* max datalen for quick test */
#define	SYSMSG		errmsg(errno)		/* pointer to system message */
#define	AUDIO_MUX_CONNECT(port1, port2)	((port1) << 4 | (port2))


/* Error codes */
#define	ERSYS		10
#define	ERIOCTL		11
#define	ERWRITE		12
#define	ERREAD		13
#define	ERSEEK		14
#define	ERLOOP		15
#define	ERDATA		16


/* Error messages */
char er_open[] = "Fail to open device %s (%s)";
char er_ioctl[] = "Fail ioctl %s (%s)";
char er_write[] = "Fail to write %d bytes to audio device (%s)";
char er_read[] = "Fail to read %d bytes from audio device (%s)";
char er_readf[] = "Fail to read %d bytes of data file (%s)";
char er_mem[] = "Fail to allocate %d bytes of memory (%s)";
char er_seek[] = "Fail to seek to position in data file (%s)";
char er_datafile[] = "File %s not found, skip the audio play test";
char er_reg[] = "Register %s number %d failed";
char er_inloop[] = "Internal loopback failed, write 0x%02X, read 0x%02X";


/* Global data declarations */
char test_usage[] = "[V=0..255] [O=speaker/jack]";
int audio_volume = NORM_VOLUME;		/* audio volume */
int audio_output = OUT_SPEAKER;		/* audio output */
char *audio_file = DATA_FILE;		/* audio data file */
int fd = 0;				/* device descriptor */
extern int errno;			/* system error code */
#ifdef	OS_4_1
int fdio = 0;				/* ioctl device descriptor */
#endif


/*
 * Forward declarations.
 */
extern off_t lseek();
extern char *malloc();
extern char *strcpy();
extern char *sprintf();
extern char *strchr();
int process_test_args();
int routine_usage();


/*
 * The following data tables contain the values to be loaded into the gain
 * registers of the audio chip.
 */
#define	MAX_GER_ENTRIES	28
static u_char ger_table[][2] = {
	0xaa, 0xaa,	/* -10db */
	0x79, 0xac,
	0x41, 0x99,
	0x9c, 0xde,
	0x74, 0x9c,	/* -6db */
	0x6a, 0xae,
	0xab, 0xdf,
	0x64, 0xab,
	0x2a, 0xbd,
	0x5c, 0xce,
	0x00, 0x99,	/* 0db */
	0x43, 0xdd,
	0x52, 0xef,
	0x55, 0x42,
	0x31, 0xdd,
	0x43, 0x1f,
	0x40, 0xdd,	/* 6.0db */
	0x44, 0x0f,
	0x31, 0x1f,
	0x10, 0xdd,
	0x41, 0x0f,
	0x60, 0x0b,
	0x42, 0x10,	/* 12db */
	0x11, 0x0f,
	0x72, 0x00,
	0x21, 0x10,
	0x22, 0x00,
	0x00, 0x0b,
	0x00, 0x0f,	/* 18db */
};

#define	MAX_GR_ENTRIES	30
#define	MAX_GX_ENTRIES	30
static u_char gr_gx_table[][2] = {
	0x8b, 0x7c,	/* -18db */
	0x8b, 0x35,
	0x8b, 0x24,
	0x91, 0x23,
	0x91, 0x2a,
	0x91, 0x3b,
	0x91, 0xf9,	/* -12db */
	0x91, 0xb6,
	0x91, 0xa4,
	0x92, 0x32,
	0x92, 0xaa,
	0x93, 0xb3,
	0x9f, 0x91,	/* -6db */
	0x9b, 0xf9,
	0x9a, 0x4a,
	0xa2, 0xa2,
	0xaa, 0xa3,
	0xbb, 0x52,
	0x08, 0x08,	/* 0db */
	0x3d, 0xac,
	0x25, 0x33,
	0x21, 0x22,
	0x12, 0xa2,
	0x11, 0x3b,
	0x10, 0xf2,	/* 6db */
	0x02, 0xca,
	0x01, 0x5a,
	0x01, 0x12,
	0x00, 0x32,
	0x00, 0x13,
	0x00, 0x0e,	/* 12db */
};


/*
 * Main entry to audio test.
 */
main(argc, argv)
int argc;
char **argv;
{
	versionid = "%I";		/* SCCS version ID */
	device_name = DEVICENAME;	/* default device name */

	/* Sundiag test start up initialization */
	test_init(argc, argv, process_test_args, routine_usage, test_usage);

	/* open devices */
	if ((fd = open(device_name, O_RDWR|O_NDELAY)) < 0)
		send_message(ERSYS, ERROR, er_open, device_name, SYSMSG);
#ifdef	OS_4_1
	if ((fdio = open(IOCTLDEVICE, O_RDWR|O_NDELAY)) < 0)
		send_message(ERSYS, ERROR, er_open, IOCTLDEVICE, SYSMSG);
#endif

	/* start test */
	run_audio_test();

	/* Sundiag test end condition */
	clean_up();			/* this should be in test_end */
	test_end();
	return(0);
}


/*
 * Function to parse specific test arguments.
 * Function returns true if options are valid, else false.
 */
process_test_args(argv, index)
char **argv;
int index;
{
	int match = TRUE;

	if (!strncmp(argv[index], "V=", 2)) {
		audio_volume = atoi(argv[index]+2);
		if (audio_volume < MIN_VOLUME || audio_volume > MAX_VOLUME)
			match = FALSE;
	}
	else if (!strncmp(argv[index], "O=", 2)) {
		if (!strcmp(argv[index]+2, "speaker"))
			audio_output = OUT_SPEAKER;
		else if (!strcmp(argv[index]+2, "jack"))
			audio_output = OUT_JACK;
		else match = FALSE;
	}
	else match = FALSE;

	return(match);
}


/*
 * Function to display the test specific options.
 */
routine_usage()
{
	send_message(NULL, CONSOLE, "Audio test specific options:\n");
	send_message(NULL, CONSOLE, "\tV = audio volume (0..255)\n");
	send_message(NULL, CONSOLE, "\tO = audio output (speaker|jack)\n\n");
}


/*
 * Function to perform the audio tests.
 */
run_audio_test()
{
	/* send output to jack to keep quiet when testing registers */
	audio_set_output(OUT_JACK);
	audio_register_test();

	/* set user specified volume and output before play audio */
	audio_set_volume(audio_volume);
	audio_set_output(audio_output);
	audio_play_test();

	/* record any input then play */
	audio_record_test();

	/* internal loopback test */
	audio_inloop_test();
}


/*
 * Function to test the audio chip registers.
 */
audio_register_test()
{
	send_message(NULL,VDEBUG,"Audio register test");

	/* test MAP registers */
	reg_test(AUDIO_MAP_MMR1, 1, MAXBYTE, "MMR1");
	reg_test(AUDIO_MAP_MMR2, 1, 0x7F, "MMR2");
	reg_test(AUDIO_MAP_X, 16, MAXBYTE, "X");
	reg_test(AUDIO_MAP_R, 16, MAXBYTE, "R");
	reg_test(AUDIO_MAP_GX, 2, MAXBYTE, "GX");
	reg_test(AUDIO_MAP_GR, 2, MAXBYTE, "GR");
	reg_test(AUDIO_MAP_GER, 2, MAXBYTE, "GER");
	reg_test(AUDIO_MAP_STG, 2, MAXBYTE, "STG");
	reg_test(AUDIO_MAP_FTGR, 2, MAXBYTE, "FTGR");
	reg_test(AUDIO_MAP_ATGR, 2, MAXBYTE, "ATGR");

	/* test MUX registers */
	reg_test(AUDIO_MUX_MCR1, 1, MAXBYTE, "MCR1");
	reg_test(AUDIO_MUX_MCR2, 1, MAXBYTE, "MCR2");
	reg_test(AUDIO_MUX_MCR3, 1, MAXBYTE, "MCR3");
}


/*
 * Function to test a specified set of register.
 */
reg_test(reg, regcount, mask, regname)
int reg, regcount, mask;
char *regname;
{
	static char s1[] = "AUDIOSETREG               ";
	static char s2[] = "AUDIOGETREG               ";
	char tmp[MESSAGE_SIZE];
	struct audio_ioctl wreg, rreg;
	struct audio_ioctl savreg;
	register int pat, k, n;

	send_message(NULL, DEBUG, "Register %s, count %d", regname, regcount);
	(void)strcpy(s1+12, regname);
	(void)strcpy(s2+12, regname);

	/* save original register values */
	savreg.control = reg;
	audioctl(fd, AUDIOGETREG, &savreg, s2);

	/* test register */
	wreg.control = rreg.control = reg;
	for (pat = mask; pat >= 0; pat--) {
		for (k = 0; k < regcount; k++) {
			wreg.data[k] = (pat + k) & mask;
                        rreg.data[k] = 0;
                }
		audioctl(fd, AUDIOSETREG, &wreg, s1);
                for (k = 0; k < regcount; k++) 
                        rreg.data[k] = 0;
		audioctl(fd, AUDIOGETREG, &rreg, s2);
		for (k = 0; k < regcount; k++) {
			if (rreg.data[k] != wreg.data[k]) {
				(void) sprintf(tmp, er_reg, regname, k+1);
				(void) sprintf(strchr(tmp,NULL), "\n\t wrote ");
				for (n = 0; n < regcount; n++)
					(void) sprintf(strchr(tmp,NULL),
							"%02x ", wreg.data[n]);
				(void) sprintf(strchr(tmp,NULL), "hex\n\t read  ");
				for (n = 0; n < regcount; n++)
					(void) sprintf(strchr(tmp,NULL),
							"%02x ", rreg.data[n]);
				(void) sprintf(strchr(tmp,NULL), "hex\n");
				send_message(ERDATA, ERROR, tmp);
			}
		}
	}

	/* restore original register values */
	savreg.control = reg;
	audioctl(fd, AUDIOSETREG, &savreg, s1);
}


/*
 * Function to play an audio tune from content of a file.
 */
audio_play_test()
{
	int fdata, datalen;
	char *databuf;

	send_message(NULL,VDEBUG,"Audio play test");

	/* open and read data file into buffer */
	if ((fdata = open(audio_file, O_RDONLY)) < 0) {
		send_message(NULL, WARNING, er_datafile, audio_file);
		datalen = 0;
	} else {
		if ((datalen = (int) lseek(fdata, 0L, L_XTND)) < 0)
			send_message(ERSEEK, ERROR, er_seek, SYSMSG);
		if (lseek(fdata, 0L, L_SET) < 0)
			send_message(ERSEEK, ERROR, er_seek, SYSMSG);

		/* set limit on data size for quick test */
		if (quick_test) {
			if (datalen > QUICKDATALEN)
				datalen = QUICKDATALEN;
		}

		if (!(databuf = malloc((u_int)datalen)))
			send_message(ERSYS, ERROR, er_mem, datalen, SYSMSG);
		if (read(fdata, databuf, datalen) != datalen)
			send_message(ERREAD, ERROR, er_readf, datalen, SYSMSG);
		(void) close(fdata);

		/* play the audio */
		audio_play_data(databuf, datalen);

		/* free data buffer */
		(void) free(databuf);
	}
}


/*
 * Function to play the audio data in buffer.
 */
audio_play_data(databuf, datalen)
char *databuf;
int datalen;
{
	int count;
#ifdef	OS_4_0_3
	int qsize, qbytes;
#endif
	send_message(NULL, DEBUG, "Write %d bytes to audio device", datalen);

#ifdef	OS_4_1
	/* write data to audio buffer then wait till buffer empty */
	while (datalen) {
		if ((count = write(fd, databuf, datalen)) < 0)
			send_message(ERWRITE, ERROR, er_write, datalen, SYSMSG);
		databuf += count;
		datalen -= count;
		sleep(1);
	}
	audioctl(fd, AUDIO_DRAIN, NULL, "AUDIO_DRAIN");
#else
	/* get size of queue */
	audioctl(fd, AUDIOGETQSIZE, &qsize, "AUDIOGETQSIZE");

	/* write data from buffer to audio device */
	do {
		/* check the write queue */
		audioctl(fd, AUDIOWRITEQ, &qbytes, "AUDIOWRITEQ");

		/* write data to fill up the write queue */
		count = (qsize - qbytes) > datalen ? datalen : (qsize - qbytes);
		if (count) {
			if (write(fd, databuf, count) != count)
				send_message(ERWRITE, ERROR, er_write, count, SYSMSG);
			databuf += count;
			datalen -= count;
		}
		sleep(1);
	} while (count || qbytes);
#endif
}


/*
 * Function to test the audio recording and playback.
 * Note that it will try to record input from the jack, and likely there
 * will be quiet sound when play back.
 */
audio_record_test()
{
	char *databuf;
	int datalen;

	send_message(NULL,VDEBUG,"Audio recording test");

	/* get size of read queue */
#ifdef	OS_4_1
	audioctl(fd, FIONREAD, &datalen, "FIONREAD");
#else
	audioctl(fd, AUDIOREADQ, &datalen, "AUDIOREADQ");
#endif

	send_message(NULL, DEBUG, "Read %d bytes from audio device", datalen);

	/* allocate memory */
	if (!(databuf = malloc((u_int)datalen)))
		send_message(ERSYS, ERROR, er_mem, datalen, SYSMSG);

#ifdef	OS_4_1
	/* flush old recording data */
	audioctl(fd, I_FLUSH, FLUSHRW, "I_FLUSH");
	sleep(1);
#endif

	/* record new data */
	if (read(fd, databuf, datalen) != datalen)
		send_message(ERREAD, ERROR, er_read, datalen, SYSMSG);

	/* play back */
	audio_play_data(databuf, datalen);

	/* free memory */
	(void) free(databuf);
}


/*
 * Function to test the audio internal loopback.
 */
audio_inloop_test()
{
	u_char wpat, rpat;
        int i;
#ifdef	OS_4_0_3
	int qsize, readq;
#endif

	send_message(NULL,VDEBUG,"Audio internal loopback test");
	audio_set_inloop();

#ifdef	OS_4_0_3
	audioctl(fd, AUDIOREADQ, &qsize, "AUDIOREADQ");
#endif
	wpat = quick_test ? MAXBYTE - 5 : 0;

	do {
		/* write a byte of data */
#ifdef  OS_4_1
                audioctl(fd, I_FLUSH, FLUSHRW, "I_FLUSH");
#endif
		if (write(fd, (u_char *)&wpat, 1) != 1)
			send_message(ERWRITE, ERROR, er_write, 1, SYSMSG);

		/* wait for data ready */

#ifdef	OS_4_1
		audioctl(fd, I_FLUSH, FLUSHR, "I_FLUSH");
#endif
		/* read back data */
		if (read(fd, (u_char *)&rpat, 1) != 1)
			send_message(ERREAD, ERROR, er_read, 1, SYSMSG);
#ifdef	OS_4_0_3
		do {
			audioctl(fd, AUDIOREADQ, &readq, "AUDIOREADQ");
		} while (readq != qsize);
#endif
		/* check data */
		send_message(NULL, DEBUG, "Write %d, read %d", wpat, rpat);

		if (wpat != rpat)
			send_message(ERLOOP, ERROR, er_inloop, wpat, rpat);
	} while (++wpat);
}


/*
 * Function to set the audio chip in internal loopback mode.
 */
audio_set_inloop()
{
	struct audio_ioctl ioreg;

	ioreg.control = AUDIO_MUX_MCR2;
	ioreg.data[0] = AUDIO_MUX_CONNECT(AUDIO_MUX_PORT_BB, AUDIO_MUX_PORT_BB);
	audioctl(fd, AUDIOSETREG, &ioreg, "AUDIOSETREG MCR2");
}


/*
 * Function to set the audio volume.
 */
audio_set_volume(volume)
int volume;
{
	struct audio_ioctl ioreg;

	/* set up GR, GER, and GX registers */
	audio_set_gr(volume * MAX_GR_ENTRIES / MAX_VOLUME);
	audio_set_ger(volume * MAX_GER_ENTRIES / MAX_VOLUME);
	audio_set_gx(volume * MAX_GX_ENTRIES / MAX_VOLUME);

	/* load values from registers */
	ioreg.control = AUDIO_MAP_MMR1;
	ioreg.data[0] = AUDIO_MMR1_BITS_LOAD_GX | AUDIO_MMR1_BITS_LOAD_GR |
						AUDIO_MMR1_BITS_LOAD_GER;
	audioctl(fd, AUDIOSETREG, &ioreg, "AUDIOSETREG MMR1");
}


/*
 * Function to set the audio output to either speaker or jack.
 */
audio_set_output(where)
int where;
{
	struct audio_ioctl ioreg;

	ioreg.control = AUDIO_MAP_MMR2;
	audioctl(fd, AUDIOGETREG, &ioreg, "AUDIOGETREG MMR2");

	if (where == OUT_SPEAKER)
		ioreg.data[0] |= AUDIO_MMR2_BITS_LS;
	else
		ioreg.data[0] &= ~AUDIO_MMR2_BITS_LS;
	audioctl(fd, AUDIOSETREG, &ioreg, "AUDIOSETREG MMR2");
}


/*
 * Function to set the GER gain coefficient value by indexing to the table.
 */
audio_set_ger(index)
int index;
{
	struct audio_ioctl ioreg;

	ioreg.control = AUDIO_MAP_GER;
	ioreg.data[0] = ger_table[index][1];
	ioreg.data[1] = ger_table[index][0];
	audioctl(fd, AUDIOSETREG, &ioreg, "AUDIOSETREG GER");
}


/*
 * Function to set the GER gain coefficient value by indexing to the table.
 */
audio_set_gr(index)
int index;
{
	struct audio_ioctl ioreg;

	ioreg.control = AUDIO_MAP_GR;
	ioreg.data[0] = gr_gx_table[index][1];
	ioreg.data[1] = gr_gx_table[index][0];
	audioctl(fd, AUDIOSETREG, &ioreg, "AUDIOSETREG GR");
}


/*
 * Function to set the GER gain coefficient value by indexing to the table.
 */
audio_set_gx(index)
int index;
{
	struct audio_ioctl ioreg;

	ioreg.control = AUDIO_MAP_GX;
	ioreg.data[0] = gr_gx_table[index][1];
	ioreg.data[1] = gr_gx_table[index][0];
	audioctl(fd, AUDIOSETREG, &ioreg, "AUDIOSETREG GX");
}


/*
 * Function to issue an ioctl command and print error message upon failure.
 * (VARARGS1)
 */
audioctl(desc, command, argp, cmd_name)
int desc, command;
caddr_t argp;
char *cmd_name;
{
	if (ioctl(desc, command, argp) < 0)
		send_message(ERIOCTL, ERROR, er_ioctl, cmd_name, SYSMSG);
}


/*
 * Dummy clean up procedure.
 */
clean_up()
{
}


/******************************************************************************/
#ifdef  lint

char *device_name;
char *versionid;
int quick_test;
int errno;

/* (VARARGS3 & ARGSUSED) */
void send_message(exitcode, msgtype, format)
int exitcode, msgtype;
char *format;
{
}

#endif
/******************************************************************************/


