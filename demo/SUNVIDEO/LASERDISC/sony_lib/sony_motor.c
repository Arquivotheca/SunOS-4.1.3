#ifndef lint
static	char sccsid[] = "@(#)sony_motor.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"
#include <sys/termios.h>

#define ON	1
#define OFF	0

unsigned char
sony_motor_on(device)
FILE *device;
{
	int rci;
	unsigned char rc;

	if((rci = sony_motor_stat(device)) == ON)
		return(MOT_STAT);

	/*
	 * Motor on takes at least 20s set the serial port 
	 * to do a blocking read to be on the safe side
	 */
	sony_blockingread(device);
	putc(MOTOR_ON,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	rc = sony_handshake(device,ACK);
	sony_settimeout(device, 5);
	return(rc);
}

unsigned char
sony_motor_off(device)
FILE *device;
{
	int rci;
	unsigned char rc;

	if((rci = sony_motor_stat(device)) == OFF)
		return(MOT_STAT);

	/*
	 * Motor off takes about 5s set the serial port 
	 * to do a 10s time out 
	 */
	sony_settimeout(device, 10);
	putc(MOTOR_OFF,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	rc = sony_handshake(device,ACK);
	sony_settimeout(device, 5);
	return(rc);
}

int
sony_motor_stat(device)
FILE *device;
{
	int rc;
	unsigned char mot_stat[5];

	sony_status_inq(device,mot_stat);
	if(mot_stat[0]&MOT_STAT)
		return(OFF);
	else
		return(ON);
}
