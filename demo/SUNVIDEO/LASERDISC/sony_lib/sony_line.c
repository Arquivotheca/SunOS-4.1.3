#ifndef lint
static	char sccsid[] = "@(#)sony_line.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


/*	
 * Open, close and read a single character from the serial
 * line also set uo various types of reads
 */
#include <stdio.h> 
#include <sys/termios.h>

FILE *
sony_open(port, alarm_set)
char *port;
int alarm_set;
{
	FILE *device;
	struct termios tty;
	int fd;

	if(port == NULL) {
		if((device = fopen("/dev/ttya", "r+")) == NULL)
			return(NULL);
	} else {
		if((device = fopen(port, "r+")) == NULL)
			return(NULL);
	}

	fd = fileno(device);
	setbuf(device,(char *)NULL);

	/*
	 * Control Baud rate = 9600, 8 bits/char, 1 stop bit,
	 * no parity, set DTR low on close.
	 *
	 * Turn off echo, canonical processing, signals
	 * Read will time out after 5 seconds
	 */
	ioctl(fd,TCGETS,&tty);
	tty.c_cflag = B9600;
	tty.c_cflag |= (CS8|HUPCL|CREAD);
	tty.c_cflag &= ~(PARENB|CSTOPB);
	tty.c_lflag &= ~(ECHO|ICANON|ISIG);
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 50;
	ioctl(fd, TCSETS, &tty);

	return(device);
}

/*
 * Close the serial line 
 */
void
sony_close(device)
FILE *device;
{
	fclose(device);
}

/*
 * Read a character back from the serial line
 */
unsigned char
sony_read(device)
FILE *device;
{
	int read_tty, count, rc;
	unsigned char read_value;

	read_tty = fileno(device); 
	rc = read(read_tty, &read_value, 1); 
	if (rc != 1) {
		perror("sony_read : read failed");
		fprintf(stderr,"Laser disk not responding\n");
	} else 
		return(read_value);
}

/*
 * Set the timeout on the serial port to time in seconds
 */
void
sony_settimeout(device, time)
FILE *device;
int time;
{
	struct termios tty;
	int fd;

	fd = fileno(device);
	ioctl(fd,TCGETS,&tty);
	/*
	tty.c_cflag = B9600;
	tty.c_cflag |= (CS8|HUPCL|CREAD);
	tty.c_cflag &= ~(PARENB|CSTOPB);
	tty.c_lflag &= ~(ECHO|ICANON|ISIG);
	*/
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 10*time;
	ioctl(fd, TCSETS, &tty);
	return;
}

/*
 * Set the serial port to do a blocking read
 */
void
sony_blockingread(device)
FILE *device;
{
	struct termios tty;
	int fd;

	fd = fileno(device);
	ioctl(fd,TCGETS,&tty);
	/*
	tty.c_cflag = B9600;
	tty.c_cflag |= (CS8|HUPCL|CREAD);
	tty.c_cflag &= ~(PARENB|CSTOPB);
	tty.c_lflag &= ~(ECHO|ICANON|ISIG);
	*/
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	ioctl(fd, TCSETS, &tty);
	return;
}
