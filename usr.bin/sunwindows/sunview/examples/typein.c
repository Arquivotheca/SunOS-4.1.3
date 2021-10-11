/*********************************************************/
#ifndef lint
static char sccsid[] = "@(#)typein.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*********************************************************/

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <suntool/tty.h>
#include <ctype.h>

static Frame	 frame;
static Canvas	 canvas;
static Tty	 tty;
static Pixwin	*pw;

static Notify_client	my_client;

#define STDIN_FD	0
#define STDOUT_FD	1
#define BUFSIZE		1000

main(argc, argv)
int	argc;
char  **argv;
{
	static Notify_value read_input();
	int	tty_fd;

	frame = window_create(NULL, FRAME, 
		FRAME_ARGS,	argc, argv, 
		WIN_ERROR_MSG,	"Cannot create frame", 
		FRAME_LABEL,	"typein",
		0);

	tty = window_create(frame, TTY, 
		WIN_PERCENT_HEIGHT,	50, 
		TTY_ARGV,		TTY_ARGV_DO_NOT_FORK,
		0);

	tty_fd = (int)window_get(tty, TTY_TTY_FD);
	dup2(tty_fd, STDOUT_FD);
	dup2(tty_fd, STDIN_FD);

	canvas = window_create(frame, CANVAS,
			0);
	pw = canvas_pixwin(canvas);

	/*
	 * Set up a notify proc so that whenever there is input to read on
	 * stdin (fd 0), we are called to read it.
	 * Notifier needs a unique handle: give it the address of tty.
	 */
	my_client = (Notify_client) &tty;
	notify_set_input_func(my_client, read_input, STDIN_FD);

	printf("Enter first coordinate:\nx? ");

	window_main_loop(frame);
	exit(0);
    /* NOTREACHED */
}

/*
 * This section implements a simple application which writes prompts to
 * stdin and reads coordinates from stdout, drawing vectors with the
 * supplied coordinates.  It uses a state machine to keep track of what
 * number to read next.
 */
#define GET_X_1		0
#define GET_Y_1		1
#define GET_X_2		2
#define GET_Y_2		3
int state = GET_X_1;
int x1, y1, x2, y2;


/* ARGSUSED */
static Notify_value
read_input(client, in_fd)
Notify_client	client;		/* unused since this must be from ttysw */ 
int		in_fd;		/* unused since this is stdin */
{
	char	buf[BUFSIZE];
	char   *ptr, *gets();

	ptr = gets(buf);   /* read one line per call so that we
				   don't ever block */
				  /* ^^^^^ does this matter any more?? */
	/* handle end of file */
	if (ptr==NULL) {
		/* Note: could have been a read error */
		window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);
		window_done(tty);
	} else {
		switch (state) {
		case GET_X_1:
			if (sscanf(buf, "%d", &x1) != 1) {
				printf("Illegal value!\nx? ");
				fflush(stdout);
			} else {
				printf("y? ");
				fflush(stdout);
				state++;
			}
			break;
		case GET_Y_1:
			if (sscanf(buf, "%d", &y1) != 1) {
				printf("Illegal value!\ny? ");
				fflush(stdout);
			} else {
				printf("Enter second coordinate:\nx? ");
				fflush(stdout);
				state++;
			}
			break;
		case GET_X_2:
			if (sscanf(buf, "%d", &x2) != 1) {
				printf("Illegal value!\nx? ");
				fflush(stdout);
			} else {
				printf("y? ");
				fflush(stdout);
				state++;
			}
			break;
		case GET_Y_2:
			if (sscanf(buf, "%d", &y2) != 1) {
				printf("Illegal value!\ny? ");
				fflush(stdout);
			} else {
				printf("Vector from (%d, %d) to (%d, %d)\n",
					x1, y1, x2, y2);
				pw_vector(pw, x1, y1, x2, y2, PIX_SET, 1);
				printf("\nEnter first coordinate:\nx? ");
				fflush(stdout);
				state = GET_X_1;
			}
			break;
		}
	}
	return(NOTIFY_DONE);
}
