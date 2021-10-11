/*
 *	@(#)cdtest.c 1.1 92/07/30 SMI
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 *
 *	CD ROM test, which will read the CD ROM table of contents then
 *	read all existing data tracks and play all existing audio tracks
 *	on the CD.  If a supported known CD is used, both the TOC and the
 *	first block of all data tracks will also be compared.  Upon reading
 *	data tracks, a random number of blocks will be read each time.
 */


#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include "cd.h"
#include "sdrtns.h"
#include "../../../lib/include/libonline.h"	/* online library include */


/* Default device name */
#define	DEVICENAME	"/dev/rsr0"

/* Data buffer size allocation */
#define	MAX_DATABUF	(128*CDROM_MODE2_SIZE)	/* max data buffer size */


/* For verbose and debug messages */
#define	VDEBUG		VERBOSE+DEBUG


/* Test to be skipped */
#define	SKIP_NONE	0			/* skip no tracks */
#define	SKIP_DATA1	1			/* skip data mode 1 tracks */
#define	SKIP_DATA2	2			/* skip data mode 2 tracks */
#define	SKIP_AUDIO	4			/* skip audio tracks */


/* Data read access mode */
#define	RANDOM_READ	0			/* random read test */
#define	SEQUENTIAL_READ	1			/* sequential read test */


/* Audio volume limits */
#define	MIN_VOLUME	0			/* minimum audio volume */
#define	MAX_VOLUME	255			/* maximum audio volume */


/* Error codes */
#define	ERSYS		10
#define	ERIOCTL		12
#define	ERREAD		13
#define	ERSEEK		14
#define	ERDATA		15
#define	ERSTATUS	16


/* Global information declarations */
char *test_usage = "[D=/dev/raw] [S=data1|data2|audio] [R=random|sequential] [P=0..100] [V=0..255] [T=sony2|hitachi4|cdassist|pdo|other]";
int cd_type = CD_DEFAULT;		/* CD type under test */
int skip_test = SKIP_NONE;		/* skip test mask value */
int audio_volume = MAX_VOLUME;		/* audio volume control */
int read_access = RANDOM_READ;		/* read access mode */
int percent_data = 100;			/* percent of data to be tested */
u_int randval = 0;			/* random value generated */
int fd = 0;				/* device descriptor */
char *databuf = 0;			/* data buffer read */
struct cdrom_tochdr tochdr;		/* CD TOC header */
struct cdrom_tocentry *cdtoc = 0;	/* CD table of contents */
extern char *device_name;		/* device name */
extern int errno;			/* system error code */


/* Error messages */
char er_ioctl[] = "Fail ioctl %s (%s)";
char er_seek[] = "Fail to seek at block %d (%s)";
char er_read[] = "Fail to read %d bytes at block %d (%s)";
char er_datamode[] = "Data mode %d not supported";
char er_open[] = "Fail to open device %s (%s)";
char er_mem[] = "Fail to allocate %d bytes of memory";
char er_tochdr[] = "Invalid TOC header, first track %d, last track %d";
char er_toccmp[] = "CD contains incorrect TOC info";
char er_audioerr[] = "Error occurred during audio play at track %d";
char er_audioplay[] = "Audio play exceeded %d second time limit at track %d";
char er_audiostop[] = "Audio stop before complete at track %d";
char er_audiostat[] = "Unexpected audio status %d received at track %d";
char er_datacmp[] = "Data miscompare %d bytes at block %d";
char warn_play[] = "The play time %d sec exceeds time usually expected (%d sec) at track %d";  


/* Forward declarations */
extern int errno;
int routine_usage();
int process_test_args();
char *errmsg();
char *malloc();
#ifndef SVR4
long random();
#endif
long lseek();


/*
 * Main entry to CD test.
 */
main(argc,argv)
int argc;
char **argv;
{
        versionid = "1.1";		/* SCCS version id */
	device_name = DEVICENAME;	/* set up default device name */

	/* Sundiag test start up initialization */
	test_init(argc,argv,process_test_args,routine_usage,test_usage);

	if ((fd = open(device_name,O_RDONLY)) < 0)
		send_message(ERSYS,ERROR,er_open,device_name,errmsg(errno));
	run_cd_test();

	/* Sundiag test end condition */
	clean_up();	/* expecting this to be in test_end() */
	test_end();
	return(0);
}


/*
 * Function to parse the specific test arguments.
 * It returns true if option is expected, else returns false.
 */
process_test_args(argv,index)
char **argv;
int index;
{
	int match = TRUE;

	if (!strncmp(argv[index],"D=",2))
	{
		device_name = argv[index]+2;
	}
	else if (!strncmp(argv[index],"T=",2))
	{
		for (match = 0; cd_name[match]; match++)
		{
			if (!(strcmp(argv[index]+2,cd_name[match])))
				break;
		}
		if (cd_name[match])
		{
			cd_type = match;
			match = TRUE;
		}
		else match = FALSE;
	}
	else if (!strncmp(argv[index],"S=",2))
	{
		if (!strcmp(argv[index]+2,"data1"))
			skip_test |= SKIP_DATA1;
		else if (!strcmp(argv[index]+2,"data2"))
			skip_test |= SKIP_DATA2;
		else if (!strcmp(argv[index]+2,"audio"))
			skip_test |= SKIP_AUDIO;
		else match = FALSE;
	}
	else if (!strncmp(argv[index],"R=",2))
	{
		if (!strcmp(argv[index]+2,"random"))
			read_access = RANDOM_READ;
		else if (!strcmp(argv[index]+2,"sequential"))
			read_access = SEQUENTIAL_READ;
		else match = FALSE;
	}
	else if (!strncmp(argv[index],"P=",2))
	{
		percent_data = atoi(argv[index]+2);
		if (percent_data < 0 || percent_data > 100)
			match = FALSE;
	}
	else if (!strncmp(argv[index],"V=",2))
	{
		audio_volume = atoi(argv[index]+2);
		if (audio_volume < 0 || audio_volume > MAX_VOLUME)
			match = FALSE;
	}
	else match = FALSE;

	return(match);
}


/*
 * Function to display the test specific options.
 */
routine_usage()
{
	send_message(NULL,CONSOLE,"CD-ROM test specific options:\n");
	send_message(NULL,CONSOLE,"\tD = name of raw device (/dev/rsr?)\n");
	send_message(NULL,CONSOLE,"\tS = skip tracks (data1|data2|audio)\n");
	send_message(NULL,CONSOLE,"\tR = read access (random|sequential)\n");
	send_message(NULL,CONSOLE,"\tP = percentage of data to be tested\n");
	send_message(NULL,CONSOLE,"\tV = audio volume (0..255, default 255)\n");
	send_message(NULL,CONSOLE,"\tT = type of CD used for test \
(sonny2|hitachi4|cdassist|pdo|other)\n\n");
}


/*
 * Routine which performs the CD ROM testing.  It will read the TOC header
 * to determine the type of each track, then perform the appropriate test
 * for data mode 1, data mode2, or audio track.
 */
run_cd_test()
{
	struct cdrom_tocentry *toc;
	int track;

	/* issue START to restart CDROM unit */
	if (ioctl(fd,CDROMSTART) < 0)
		send_message(ERIOCTL,ERROR,er_ioctl,"CDROMSTART",errmsg(errno));
	if (!quick_test)
		sleep(5);

	/* build and check TOC */
	build_check_toc();

	/* allocate memory for data read buffer */
	if (!(databuf = malloc((u_int)MAX_DATABUF)))
		send_message(ERSYS,ERROR,er_mem,MAX_DATABUF);

	/* generate unique random seed */
#	ifdef SVR4
	srand(getpid());
#	else
	srandom(getpid());
#	endif SVR4

	/* handle special case for PDO disc before performing test */
#define	PDO_NBLOCKS	330000
	if (cd_type == CD_PDO)
	{
		toc = cdtoc;
		track = tochdr.cdth_trk1;
		(toc+track)->cdte_addr.lba = PDO_NBLOCKS;
	}

	/* read data and play audio in sequential track order */
	toc = cdtoc;
	for (track = tochdr.cdth_trk0; track <= tochdr.cdth_trk1; track++,toc++)
	{
		if (toc->cdte_ctrl & CDTRK_DATA)
		{
			if (toc->cdte_datamode == CD_DATAMODE1)
			{
				/* check if need to skip data mode 1 test */
				if (!(skip_test & SKIP_DATA1))
					read_data_track(toc);
			}
			else
			{
				/* check if need to skip data mode 2 test */
				if (!(skip_test & SKIP_DATA2))
					read_data_track(toc);
			}
		}
		else
		{
			/* check if need to skip audio test */
			if (!(skip_test & SKIP_AUDIO))
				play_audio_track(toc);
		}
	}

	/* issue STOP to reset CDROM unit */
	if (ioctl(fd,CDROMSTOP) < 0)
		send_message(ERIOCTL,ERROR,er_ioctl,"CDROMSTOP",errmsg(errno));
}


/*
 * Function to read and save the TOC then check against known TOC data.
 */
build_check_toc()
{
	u_int size;
	int track;
	struct cdrom_tocentry *toc;
	struct toc_info *info;

	/* read TOC header */
	if (ioctl(fd,CDROMREADTOCHDR,&tochdr) < 0)
		send_message(ERIOCTL,ERROR,er_ioctl,"CDROMREADTOCHDR",errmsg(errno));
	
	/* check the TOC header info */
	if (tochdr.cdth_trk0 < MIN_TRACK || tochdr.cdth_trk1 > MAX_TRACK ||
	    tochdr.cdth_trk0 > tochdr.cdth_trk1)
		send_message(ERDATA,ERROR,er_tochdr,tochdr.cdth_trk0,tochdr.cdth_trk1);
	send_message(NULL,VDEBUG,"CD table of contents (track %d thru %d)",
					tochdr.cdth_trk0,tochdr.cdth_trk1);

	/* allocate memory for TOC entry */
	size = (tochdr.cdth_trk1 - tochdr.cdth_trk0 + 2) * sizeof(struct cdrom_tocentry);
	if (!(cdtoc = (struct cdrom_tocentry *)malloc((u_int)size)))
		send_message(ERSYS,ERROR,er_mem,size);

	/* read TOC entry include the lead out track */
	toc = cdtoc;
	for (track = tochdr.cdth_trk0; track <= (tochdr.cdth_trk1+1); track++)
	{
		toc->cdte_track = track>tochdr.cdth_trk1 ? CDROM_LEADOUT : track;
		toc->cdte_format = CDROM_LBA;
		if (ioctl(fd,CDROMREADTOCENTRY,toc) < 0)
			send_message(ERIOCTL,ERROR,er_ioctl,"CDROMREADTOCENTRY",errmsg(errno));
		send_message(NULL,VDEBUG,"Track %03d, type %d, mode %03d, lba %06d",
					toc->cdte_track,toc->cdte_ctrl,
					toc->cdte_datamode,toc->cdte_addr.lba);
		toc++;
	}

	/* check TOC entry if a known supported CD is tested */
	if (cd_type != CD_OTHER)
	{
		send_message(NULL,VDEBUG,"Compare TOC information (%s)",cd_name[cd_type]);
		toc = cdtoc - 1;
		info = cd_tochdr[cd_type] - 1;
		do
		{
			info++;
			toc++;
			if (info->track != toc->cdte_track ||
			    info->ctrl != toc->cdte_ctrl ||
			    info->lba != toc->cdte_addr.lba ||
			    info->datamode != toc->cdte_datamode)
				send_message(ERDATA,ERROR,er_toccmp);
		} while (toc->cdte_track != CDROM_LEADOUT);
	}
}


/*
 * Function to read the whole specified data track.
 * Depending on the read access flag, either random or sequential read will
 * be performed.  Each time a random number of blocks will be read.
 * The first chunk of data in each track will be compared against
 * expected data, if CD type is known.
 */
read_data_track(toc)
struct cdrom_tocentry *toc;
{
	struct datacon_info *info;
	int lba, start, end, blocksize, nblk, maxnblk, k;
	int *mark;

	info = cd_datacon[cd_type];
	blocksize = toc->cdte_datamode == CD_DATAMODE1 ? CDROM_MODE1_SIZE :
							CDROM_MODE2_SIZE;
	maxnblk = MAX_DATABUF / blocksize;

	start = toc->cdte_addr.lba;		/* first logical block */
	end = (toc+1)->cdte_addr.lba - 1;	/* last logical block */

	/* skip gaps between certain tracks */
	if (toc->cdte_track != tochdr.cdth_trk1)
	{
		/*
		 * if data mode 1 then skip gaps on all but the last track,
		 * if data mode 2 then skip only if the next track is audio.
		 */
		if (toc->cdte_datamode == CD_DATAMODE1)
			end -= CD_POSTGAP;
		else
		{
			if (!((toc+1)->cdte_ctrl & CDTRK_DATA))
				end -= CD_POSTGAP;
		}
	}
	if (end < start) return;

	send_message(NULL,VDEBUG,"Read CD track %d (data mode %d, %d blocks)",
	    toc->cdte_track,toc->cdte_datamode,(end-start+1)*percent_data/100);

	/* if CD type is known then read and compare data of first block */
	if (cd_type != CD_OTHER)
	{
		/* searching for the expected track data in table */
		while (info->track != toc->cdte_track &&
		       info->track != CDROM_LEADOUT) info++;

		/* if found then read and compare the beginning data stream */
		if (info->track == toc->cdte_track)
		{
			nblk = info->size / blocksize;
			if (info->size % blocksize) nblk++;
			send_message(NULL,VDEBUG,"Read block %d and compare %d bytes of data",start,info->size);
			read_data_blocks(toc->cdte_datamode,start,nblk,blocksize);
			if (bcmp(databuf,info->expect,info->size))
				send_message(ERDATA,ERROR,er_datacmp,info->size,start);
		}
	}

	/*
	 * This condition allows testing a partial number of data blocks in
	 * a track by changing the start and end logical block addresses.
	 */
	if (percent_data < 100)
	{
		k = end - start + 1;
		nblk = k * percent_data / 100;
#		ifdef SVR4
		lba = abs((int)rand());
#		else
		lba = abs((int)random());
#		endif SVR4
		lba %= (k - nblk + 1);
		start += lba;
		end = start + nblk - 1;
	}

	if (read_access == RANDOM_READ)
	{
		send_message(NULL,VDEBUG,"Randomly read block %d thru %d",start,end);

		/* allocate memory to maintain read/unread block mark */
		k = (end - start) / 8 + 8;
		if (!(mark = (int *)malloc((u_int)k)))
			send_message(ERSYS,ERROR,er_mem,k);
		bzero((char *)mark,k);

		/* randomly read all blocks once */
		for (k = start; k <= end; k += nblk)
		{
			lba  = rnd_blkpos(mark,start,end);
			nblk = rnd_blkcnt(mark,start,end,lba,maxnblk);
			read_data_blocks(toc->cdte_datamode,lba,nblk,blocksize);

			/* if quick test then only read one chunk */
			if (quick_test) break;
		}

		/* free mark memory */
		free((char *)mark);
	}
	else
	{
		send_message(NULL,VDEBUG,"Sequentially read block %d thru %d",start,end);

		/* sequential read all blocks once */
		for (lba = start; lba <= end; lba += nblk)
		{
#			ifdef SVR4
			nblk = abs((int)rand());
#			else
			nblk = abs((int)random());
#			endif SVR4
			nblk = (nblk % maxnblk) + 1;
			if (nblk > (end - lba + 1))
				nblk = end - lba + 1;
			read_data_blocks(toc->cdte_datamode,lba,nblk,blocksize);

			/* if quick test then only read one chunk */
			if (quick_test) break;
		}
	}
}


/*
 * Function to read number of blocks at specified logical block address.
 */
read_data_blocks(datamode,lba,nblk,blocksize)
u_char datamode;
int lba, nblk, blocksize;
{
	struct cdrom_read readinfo;
	int size;

	send_message(NULL,DEBUG,"Read block %d thru %d, block size %d",
						lba,lba+nblk-1,blocksize);

	switch (datamode)
	{
	case CD_DATAMODE1:
		/* CD data mode 1 can be read with normal read */
		size = nblk * blocksize;
		if (lseek(fd,(long)(lba*blocksize),L_SET) == -1)
			send_message(ERSEEK,ERROR,er_seek,lba,errmsg(errno));
		if (read(fd,databuf,size) != size)
			send_message(ERREAD,ERROR,er_read,size,lba,errmsg(errno));
		break;
	case CD_DATAMODE2:
		/* CD data mode 2 can only be read thru special ioctl */
		size = nblk * blocksize;
		readinfo.cdread_lba = lba;
		readinfo.cdread_bufaddr = databuf;
		readinfo.cdread_buflen = size;
		if (ioctl(fd,CDROMREADMODE2,&readinfo) < 0)
			send_message(ERREAD,ERROR,er_read,size,lba,errmsg(errno));
		break;
	default:
		send_message(ERREAD,ERROR,er_datamode,datamode);
		break;
	}
}


/*
 * Function returns a random logical block address that has not yet been read,
 * where mark is the bit stream indicating blocks that are read/unread, start
 * and end are the first and last block boundaries.  Function will also mark
 * the bit position that is selected as being read.
 *
 * Note that, rnd_blkpos() and rnd_blkcnt() are used to randomly select
 * all blocks, each time a random number of blocks, and each block will
 * be selected once.
 */
rnd_blkpos(mark,start,end)
int *mark, start, end;
{
	u_int count, index, bit, pos;

	/* select logical block within 0 and end-start+1 boundaries */
	count = end - start + 1;
#	ifdef SVR4
	randval = (u_int)rand();
#	else
	randval = (u_int)random();
#	endif SVR4
	pos = randval % count;

	/* check if block has been read then use the next unread ones */
	for (;;)
	{
		index = pos >> 5;
		bit = pos & 31;
		if (!((1 << bit) & mark[index])) break;
		if (++pos >= count) pos = 0;
	}
	mark[index] |= (1 << bit);

	/* return the random position with offset from start */
	return(pos+start);
}


/*
 * Function returns a random number of contiguous blocks that are not read,
 * where mark is the bit stream indicating blocks that are read/unread, start
 * and end are the first and last logical block boundaries, lba is the current
 * logical block, and maxnblk is the maximum number of blocks expected to
 * return.  Function will also mark the returned number of bits following
 * bit position from lba as being read.
 *
 * Note that, rnd_blkpos() and rnd_blkcnt() are used to randomly select
 * all blocks, each time a random number of blocks, and each block will
 * be selected once.
 */
rnd_blkcnt(mark,start,end,lba,maxnblk)
int *mark, start, end, lba, maxnblk;
{
	u_int nblk, count, index, bit;

	/* select random number of blocks based on max number of blocks */
	if ((lba + maxnblk) > end) maxnblk = end - lba + 1;
	count = (randval % maxnblk) + 1;

	/* adjust random number of blocks to contiguous unread blocks */
	lba -= start;
	for (nblk = 1; nblk < count; nblk++)
	{
		lba++;
		index = lba >> 5;
		bit = lba & 31;
		if ((1 << bit) & mark[index]) return(nblk);
		mark[index] |= (1 << bit);
	}
	return(count);
}


/*
 * Function to play specified audio track.
 * The audio status will be checked during audio play, and a time limit
 * will be set for each track play according to the TOC information.
 */
play_audio_track(toc)
struct cdrom_tocentry *toc;
{
	struct cdrom_ti ti;
	struct cdrom_subchnl subchnl;
	struct cdrom_volctrl volctrl;
	u_long timelimit, timestart, timeout;
	struct timeval t;

	timelimit = ((toc+1)->cdte_addr.lba - toc->cdte_addr.lba) / NBLOCKS_SEC;
	send_message(NULL,DEBUG,"Play CD track %d (%d seconds)",toc->cdte_track,timelimit);

	/* Add extra seconds for error recovery: Hai 10/24/90 */
	/* Change 45 sec to 60 to avoid unnecessary restriction during  */
	/* heavy system load on scsi bus. Patrick 7/24/90  */
	timeout = timelimit + 600; 	/* Add timeout for play fatal */
	timelimit += 60 ;

	/* set audio volume control */
	volctrl.channel0 = volctrl.channel1 = audio_volume;
	volctrl.channel2 = volctrl.channel3 = audio_volume;
	if (ioctl(fd,CDROMVOLCTRL,&volctrl) < 0)
		send_message(ERIOCTL,ERROR,er_ioctl,"CDROMVOLCTRL",errmsg(errno));

	/* play audio track/index */
	ti.cdti_trk0 = ti.cdti_trk1 = toc->cdte_track;
	ti.cdti_ind0 = ti.cdti_ind1 = MIN_INDEX;
	if (ioctl(fd,CDROMPLAYTRKIND,&ti) < 0)
		send_message(ERIOCTL,ERROR,er_ioctl,"CDROMPLAYTRKIND",errmsg(errno));

	/* if quick test then don't need to check audio status */
	if (quick_test) return;

	/* wait while audio is still playing */
	(void) gettimeofday(&t,(struct timezone *)0);
	timestart = t.tv_sec;
	do
	{
		(void) gettimeofday(&t,(struct timezone *)0);
		sleep(3);
		if (ioctl(fd,CDROMSUBCHNL,&subchnl) < 0)
			send_message(ERIOCTL,ERROR,er_ioctl,"CDROMSUBCHNL",errmsg(errno));
		if (subchnl.cdsc_audiostatus != CDROM_AUDIO_PLAY)
			break;
	} while ((t.tv_sec - timestart) <= timeout);

	/* check the audio status received */
	switch (subchnl.cdsc_audiostatus)
	{
	case CDROM_AUDIO_COMPLETED:			/* completion ok */
		if((t.tv_sec - timestart) > timelimit) 
			send_message(0, INFO, warn_play, t.tv_sec-timestart, timelimit, ti.cdti_trk0); 
		break;
	case CDROM_AUDIO_PLAY:			/* still play after time out */
		send_message(ERSTATUS,ERROR,er_audioplay,timeout,ti.cdti_trk0);
		break;
	case CDROM_AUDIO_PAUSED:			/* pause before complete */
		send_message(ERSTATUS,ERROR,er_audiostop,ti.cdti_trk0);
		break;
	case CDROM_AUDIO_ERROR:			/* error during play */
		send_message(ERSTATUS,ERROR,er_audioerr,ti.cdti_trk0);
		break;

	case CDROM_AUDIO_NO_STATUS:             /* no status to return */
		break;

	default:				/* unknown audio status */
		send_message(ERSTATUS,ERROR,er_audiostat,
					subchnl.cdsc_audiostatus,ti.cdti_trk0);
		break;
	}
}


/*
 * Dummy clean up as required by libsdrtns.a
 */
clean_up()
{
	if (cdtoc) free((char *)cdtoc);
	if (databuf) free(databuf);
	if (fd > 0)
	{
		(void)ioctl(fd,CDROMSTOP);
		(void)close(fd);
	}
}


/******************************************************************************/
#ifdef	lint

int errno;
int quick_test;
char *device_name;
char *versionid;

/* VARARGS0 */
void send_message()
{
}

#endif
/******************************************************************************/
