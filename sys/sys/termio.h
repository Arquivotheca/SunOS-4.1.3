/*	@(#)termio.h 1.1 92/07/30 SMI; from S5R2 6.2	*/

#ifndef _sys_termio_h
#define _sys_termio_h

#include	<sys/ioccom.h>
#include	<sys/termios.h>

#define	NCC	8

#define	SSPEED	7	/* default speed: 300 baud */

/*
 * Ioctl control packet
 */
struct termio {
	unsigned short	c_iflag;	/* input modes */
	unsigned short	c_oflag;	/* output modes */
	unsigned short	c_cflag;	/* control modes */
	unsigned short	c_lflag;	/* line discipline modes */
	char	c_line;			/* line discipline */
	unsigned char	c_cc[NCC];	/* control chars */
};

#define	TCGETA	_IOR(T, 1, struct termio)
#define	TCSETA	_IOW(T, 2, struct termio)
#define	TCSETAW	_IOW(T, 3, struct termio)
#define	TCSETAF	_IOW(T, 4, struct termio)
#define	TCSBRK	_IO(T, 5)

#endif /*!_sys_termio_h*/
