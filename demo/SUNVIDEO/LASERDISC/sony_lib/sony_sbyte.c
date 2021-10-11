#ifndef lint
static	char sccsid[] = "@(#)sony_sbyte.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


/*
 * Send a single byte, receive a single byte laser disk commands.
 */
#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_audio_mute_on(device)
FILE *device;
{
	unsigned char rc;

	putc(AUDIO_MUTE_ON,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_audio_mute_off(device)
FILE *device;
{
	unsigned char rc;

	putc(AUDIO_MUTE_OFF,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * Valid only for numerical inputs.
 */
unsigned char
sony_clear_entry(device)
FILE *device;
{
	unsigned char rc;

	putc(CLEAR_ENTRY,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_ch1_on(device)
FILE *device;
{
	unsigned char rc;

	putc(CH1_ON,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_ch1_off(device)
FILE *device;
{
	unsigned char rc;

	putc(CH1_OFF,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_ch2_on(device)
FILE *device;
{
	unsigned char rc;

	putc(CH2_ON,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_ch2_off(device)
FILE *device;
{
	unsigned char rc;

	putc(CH2_OFF,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_chapter_mode(device)
FILE *device;
{
	unsigned char rc;

	putc(CHAPTER_MODE,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_clear_all(device)
FILE *device;
{
	unsigned char rc;

	putc(CLEAR_ALL,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_cont(device)
FILE *device;
{
	unsigned char rc;

	putc(CONTINUE,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_cx_on(device)
FILE *device;
{
	unsigned char rc;

	putc(CX_ON,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_cx_off(device)
FILE *device;
{
	unsigned char rc;

	putc(CX_OFF,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_eject_enable(device)
FILE *device;
{
	unsigned char rc;

	putc(EJECT_ENABLE,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_eject_disable(device)
FILE *device;
{
	unsigned char rc;

	putc(EJECT_DISABLE,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * CAV Disks Only
 */
unsigned char
sony_f_fast(device)
FILE *device;
{
	unsigned char rc;
	
	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(F_FAST,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * CAV Disks Only
 */
unsigned char
sony_r_fast(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(R_FAST,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_f_play(device)
FILE *device;
{
	unsigned char rc;

	putc(F_PLAY,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}
	
/*
 * CAV Disks Only
 */
unsigned char
sony_r_play(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(R_PLAY,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_f_scan(device)
FILE *device;
{
	unsigned char rc;

	putc(F_SCAN,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_r_scan(device)
FILE *device;
{
	unsigned char rc;

	putc(R_SCAN,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * CAV Disks Only
 */
unsigned char
sony_f_slow(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(F_SLOW,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * CAV Disks Only
 */
unsigned char
sony_r_slow(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(R_SLOW,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * CAV Disks Only
 */
unsigned char
sony_f_step_still(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(F_STEP_STILL,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * CAV Disks Only
 */
unsigned char
sony_r_step_still(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(R_STEP_STILL,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_frame_mode(device)
FILE *device;
{
	unsigned char rc;

	putc(FRAME_MODE,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_index_on(device)
FILE *device;
{
	unsigned char rc;

	putc(INDEX_ON,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_index_off(device)
FILE *device;
{
	unsigned char rc;

	putc(INDEX_OFF,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_memory(device)
FILE *device;
{
	unsigned char rc;

	putc(MEMORY,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_menu(device)
FILE *device;
{
	unsigned char rc;

	putc(MENU,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * Applicable to PAL only, NTSC normal play mode
 */
unsigned char
sony_non_cf_play(device)
FILE *device;
{
	unsigned char rc;

	putc(NON_CF_PLAY,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_psc_enable(device)
FILE *device;
{
	unsigned char rc;
	
	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(PSC_ENABLE,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_psc_disable(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(PSC_DISABLE,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

/*
 * CAV Disks Only
 */
unsigned char
sony_still(device)
FILE *device;
{
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	putc(STILL,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_stop(device)
FILE *device;
{
	unsigned char rc;

	putc(STOP,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_video_on(device)
FILE *device;
{
	unsigned char rc;

	putc(VIDEO_ON,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}

unsigned char
sony_video_off(device)
FILE *device;
{
	unsigned char rc;

	putc(VIDEO_OFF,device);
	rc = sony_handshake(device,ACK);
	return(rc);
}
