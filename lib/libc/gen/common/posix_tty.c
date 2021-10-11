#if	!defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)posix_tty.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/*
 * wrappers for posix tty manipulation functions
 */

#include <errno.h>
#include <termios.h>
#include <termio.h>
#include <sys/types.h>

extern	errno;

speed_t	cfgetospeed(/*struct termios* p*/);
int	cfsetospeed(/*struct termios* p, speed_t speed*/);
speed_t	cfgetispeed(/*struct termios* p*/);
int	cfsetispeed(/*struct termios* p, speed_t speed*/);
int	tcgetattr(/*int fd, struct termios* p*/);
int	tcsetattr(/*int fd, opts, struct termios* p*/);
int	tcsendbreak(/*int fd, howlong*/);
int	tcdrain(/*int fd*/);
int	tcflush(/*int fd, which*/);
int	tcflow(/*int fd, what*/);
pid_t	tcgetpgrp(/*int fd*/);
int	tcsetpgrp(/*int fd, pid_t pgrp*/);

/*
 * return the output speed from the struct
 */
speed_t
cfgetospeed(termios_p)
	struct termios *termios_p;
{
	return (termios_p->c_cflag & CBAUD);
}

/*
 * set the speed in the struct
 */
cfsetospeed(termios_p, speed)
	struct termios *termios_p;
	speed_t speed;
{
	if ((speed & ~CBAUD) != 0) {
		errno = EINVAL;
		return (-1);
	}
	termios_p->c_cflag &= ~CBAUD;
	termios_p->c_cflag |= speed;
	return (0);
}

/*
 * return the input speed from the struct
 */
speed_t
cfgetispeed(termios_p)
	struct termios *termios_p;
{
	return ((termios_p->c_cflag >> IBSHIFT) & CBAUD);
}

/*
 * set the input speed in the struct
 */
cfsetispeed(termios_p, speed)
	struct termios *termios_p;
	speed_t speed;
{
	if ((speed & ~CBAUD) != 0) {
		errno = EINVAL;
		return  (-1);
	}
	termios_p->c_cflag &= ~CIBAUD;
	termios_p->c_cflag |= speed << IBSHIFT;
	return (0);
}

/*
 * grab the modes
 */
tcgetattr(fd, termios_p)
	struct termios *termios_p;
{
	return (ioctl(fd, TCGETS, termios_p));
}

/*
 * set the modes
 */
tcsetattr(fd, option, termios_p)
	struct termios *termios_p;
{
	struct termios	work_area;

	/* If input speed is zero, set it to the output speed. */
	if (((termios_p->c_cflag >> IBSHIFT) & CBAUD) == 0) {
		work_area = *termios_p;
		work_area.c_cflag |= (work_area.c_cflag & CBAUD) << IBSHIFT;
		termios_p = &work_area;
	}
	switch (option) {
	case TCSADRAIN:
		return (ioctl(fd, TCSETSW, termios_p));
	case TCSAFLUSH:
		return (ioctl(fd, TCSETSF, termios_p));
	case TCSANOW:
		return (ioctl(fd, TCSETS, termios_p));
	default:
		errno = EINVAL;
		return (-1);
	}
	/*NOTREACHED*/
}

/*
 * send a break
 * This is kludged for duration != 0; it should do something like crank the
 * baud rate down and then send the break if the duration != 0.
 */
tcsendbreak(fd, duration)
{
	register unsigned d = (unsigned)duration;

	do
		if (ioctl(fd, TCSBRK, 0) == -1)
			return (-1);
	while (d--);
	return (0);
}

/*
 * wait for all output to drain from fd
 */
tcdrain(fd)
{
	return (ioctl(fd, TCSBRK, !0));
}

/*
 * flow control
 */
tcflow(fd, action)
{
	switch (action) {
	default:
		errno = EINVAL;
		return (-1);
	case TCOOFF:
	case TCOON:
	case TCIOFF:
	case TCION:
		return (ioctl(fd, TCXONC, action));
	}
	/*NOTREACHED*/
}

/*
 * flush read/write/both
 */
tcflush(fd, queue)
{
	switch (queue) {
	default:
		errno = EINVAL;
		return (-1);
	case TCIFLUSH:
	case TCOFLUSH:
	case TCIOFLUSH:
		return (ioctl(fd, TCFLSH, queue));
	}
	/*NOTREACHED*/
}

/*
 * get the foreground process group id
 */
pid_t
tcgetpgrp(fd)
{
	int grp_id;

	if (ioctl(fd, TIOCGETPGRP, &grp_id) == -1)
		return ((pid_t)-1);
	else
		return ((pid_t)grp_id);
}

/*
 * set the foreground process group id
 */
tcsetpgrp(fd, grp_id)
{
	return (ioctl(fd, TIOCSETPGRP, &grp_id));
}
