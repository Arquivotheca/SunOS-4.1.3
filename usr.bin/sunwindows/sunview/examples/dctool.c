
/****************************************************************/
#ifndef lint
static char sccsid[] = "@(#)dctool.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/****************************************************************/

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>

static Frame    frame;
static Panel    panel;
static Panel_item digit_item[10], enter_item;
static Panel_item add_item, sub_item, mul_item, div_item;
static Panel_item display_item;

static char     display_buf[512] = "";	/* storage for the
					 * numbers currently on
					 * the display (stored as
					 * a string) */

static FILE    *fp_tochild;	/* fp of pipe to child (write
				 * data on it) */
static FILE    *fp_fromchild;	/* fp of pipe from child (read
				 * data from it) */
static int      tochild;	/* associated file descriptors */
static int      fromchild;

static int      childpid;	/* pid of child process */

static int      dead = 0;	/* set to 1 if child process has
				 * died */

main(argc, argv)
    int             argc;
    char          **argv;
{
    static Notify_value pipe_reader();
    static Notify_value dead_child();

    frame = window_create(NULL, FRAME,
			  FRAME_ARGS, argc, argv,
			  WIN_ERROR_MSG, "Cannot create frame",
			  FRAME_LABEL, "dctool - RPN Calculator",
			  0);

    panel = window_create(frame, PANEL,
			  0);
    create_panel_items(panel);

    window_fit(panel);
    window_fit(frame);

    /* start the child process and tell the notifier about it */
    start_dc();
    /*
     * note that notify_set_input_func takes a file descriptor,
     * not a file pointer used by the standard I/O library 
     */
    (void) notify_set_input_func(frame, pipe_reader, fromchild);
    (void) notify_set_wait3_func(frame, dead_child, childpid);

    window_main_loop(frame);
    exit(0);
	/* NOTREACHED */
}

static
create_panel_items(panel)
    Panel           panel;
{
    int             c;
    char            name[2];
    static void     digit_proc(), op_proc();
    static struct {
	int             col, row;
    }               positions[10] = {
		{ 0, 3 }, { 0, 0 }, { 6, 0 }, { 12, 0 }, 
			  { 0, 1 }, { 6, 1 }, { 12, 1 }, 
			  { 0, 2 }, { 6, 2 }, { 12, 2 }
   		 };

    name[1] = '\0';
    for (c = 0; c < 10; c++) {
	name[0] = c + '0';
	digit_item[c] = panel_create_item(panel, PANEL_BUTTON,
	  PANEL_LABEL_IMAGE, panel_button_image(panel, name, 3, 0),
	  PANEL_NOTIFY_PROC, digit_proc,
	  PANEL_CLIENT_DATA, (caddr_t) (c + '0'),
	  PANEL_LABEL_X,     ATTR_COL(positions[c].col),
	  PANEL_LABEL_Y,     ATTR_ROW(positions[c].row),
	  0);
    }
    add_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "+", 3, 0),
	PANEL_NOTIFY_PROC, op_proc,
	PANEL_CLIENT_DATA, (caddr_t) '+',
	PANEL_LABEL_X,     ATTR_COL(18),
	PANEL_LABEL_Y,     ATTR_ROW(0),
	0);
    sub_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "-", 3, 0),
	PANEL_NOTIFY_PROC, op_proc,
	PANEL_CLIENT_DATA, (caddr_t) '-',
	PANEL_LABEL_X,     ATTR_COL(18),
	PANEL_LABEL_Y,     ATTR_ROW(1),
	0);
    mul_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "*", 3, 0),
	PANEL_NOTIFY_PROC, op_proc,
	PANEL_CLIENT_DATA, (caddr_t) '*',
	PANEL_LABEL_X,     ATTR_COL(18),
	PANEL_LABEL_Y,     ATTR_ROW(2),
	0);
    div_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "/", 3, 0),
	PANEL_NOTIFY_PROC, op_proc,
	PANEL_CLIENT_DATA, (caddr_t) '/',
	PANEL_LABEL_X,     ATTR_COL(18),
	PANEL_LABEL_Y,     ATTR_ROW(3),
	0);
    enter_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "Enter", 7, 0),
	PANEL_NOTIFY_PROC, op_proc,
	PANEL_CLIENT_DATA, (caddr_t) ' ',
	PANEL_LABEL_X,     ATTR_COL(6),
	PANEL_LABEL_Y,     ATTR_ROW(3),
	0);
    display_item = panel_create_item(panel, PANEL_MESSAGE,
	PANEL_LABEL_STRING, "0",
	PANEL_LABEL_X,      ATTR_COL(0),
	PANEL_LABEL_Y,      ATTR_ROW(4),
	0);
}

/* callback procedure called whenever a digit button is pressed */

static void
digit_proc(item, event)
    Panel_item      item;
    Event          *event;
{
    int             digit_name = (int) panel_get(item,
					     PANEL_CLIENT_DATA);
    char            buf[2];

    buf[0] = digit_name;	/* display digit */
    buf[1] = '\0';
    strcat(display_buf, buf);
    panel_set(display_item, PANEL_LABEL_STRING, display_buf, 0);
    send_to_dc(digit_name);	/* send digit to dc */
}

/*
 * callback procedure called whenever an operation button is
 * pressed 
 */

static void
op_proc(item, event)
    Panel_item      item;
    Event          *event;
{
    int             op_name = (int) panel_get(item,
					      PANEL_CLIENT_DATA);

    display_buf[0] = '\0';	/* don't erase display yet; wait
				 * until the answer comes back */
    send_to_dc(op_name);
    if (item != enter_item)
	send_to_dc('p');	/* send a p so the answer will be
				 * printed by dc */
    send_to_dc('\n');
}

/*
 * start the child process 
 */

static
start_dc()
{
    int             pipeto[2], pipefrom[2];
    int             c, numfds;

    if (pipe(pipeto) < 0 || pipe(pipefrom) < 0) {
	perror("dctool");
	exit(1);
    }
    switch (childpid = fork()) {

    case -1:
	perror("dctool");
	exit(1);

    case 0:			/* this is the child process */
	/*
	 * use dup2 to set the child's stdin and stdout to the
	 * pipes 
	 */

	dup2(pipeto[0], 0);
	dup2(pipefrom[1], 1);

	/*
	 * close all other fds (except stderr) since the child
	 * process doesn't know about or need them 
	 */

	numfds = getdtablesize();
	for (c = 3; c < numfds; c++)
	    close(c);

	/* exec the child process */

	execl("/usr/bin/dc", "dc", 0);
	perror("dctool (child)");	/* shouldn't get here */
	exit(1);

    default:			/* this is the parent */
	close(pipeto[0]);
	close(pipefrom[1]);
	tochild = pipeto[1];
	fp_tochild = fdopen(tochild, "w");
	fromchild = pipefrom[0];
	fp_fromchild = fdopen(fromchild, "r");

	/*
	 * the pipe to dc must be unbuffered or dc will not get
	 * any data until 1024 characters have been sent 
	 */

	setbuf(fp_tochild, NULL);
	break;
    }
}

/*
 * notify proc called whenever there is data to read on the pipe
 * from the child process; in this case it is an answer from dc,
 * so we display it 
 */

static          Notify_value
pipe_reader(frame, fd)
    Frame           frame;
    int             fd;
{
    char            buf[512];

    fgets(buf, 512, fp_fromchild);
    buf[strlen(buf) - 1] = '\0';/* remove newline */
    panel_set(display_item, PANEL_LABEL_STRING, buf, 0);
    display_buf[0] = '\0';
    return (NOTIFY_DONE);
}

/*
 * notify proc called if the child dies 
 */

static          Notify_value
dead_child(frame, pid, status, rusage)
    Frame           frame;
    int             pid;
    union wait     *status;
    struct rusage  *rusage;
{
    panel_set(display_item, PANEL_LABEL_STRING, "Child died!", 0);
    dead = 1;

    /*
     * tell the notifier to stop reading the pipe (since it is
     * invalid now) 
     */

    (void) notify_set_input_func(frame, NOTIFY_FUNC_NULL,
				 fromchild);
    close(tochild);
    close(fromchild);
    return (NOTIFY_DONE);
}

/* send a character over the pipe to dc */

static
send_to_dc(c)
    char            c;
{
    if (dead)
	panel_set(display_item,
		  PANEL_LABEL_STRING, "Child is dead!",
		  0);
    else
	putc(c, fp_tochild);
}
