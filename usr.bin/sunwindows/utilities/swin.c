#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)swin.c 1.1 92/07/30";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Set/get runtime SunView controls.
 *			This is supposed to be the window equivalent of stty.
 */

#include <stdio.h>
#include <sunwindow/window_hs.h>
#include <sys/file.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define	DONT_CARE_SHIFT		-1
#define	SHIFT_MASK(bit) (1 << (bit))

#ifdef STANDALONE
main(argc, argv)
#else
swin_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	int	parent_fd;
	char	parentname[WIN_NAMESIZE];
	char	*prog_name = *argv;
	Firm_event fe_focus;
	int fe_focus_shift = DONT_CARE_SHIFT;
	Firm_event fe_restore_focus;
	int fe_restore_focus_shift;
	struct timeval tv;

	/* Determine parent */
	if (we_getparentwindow(parentname)) {
		(void)fprintf(stderr,
		    "%s not passed parent window in environment\n", prog_name);
		EXIT(1);
	}
	/* Open parent window */
	if ((parent_fd = open(parentname, O_RDONLY, 0)) < 0) {
		(void)fprintf(stderr, "%s (parent) would not open\n", parentname);
		EXIT(1);
	}
	argv++;
	argc--;
	if (argc == 0)
		show_settings(parent_fd);
	/* Get application related args */
	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {

		case 'c': 	/* Click-to-type */
			fe_focus.id = MS_LEFT;
			fe_focus.value = 1;
			fe_focus_shift = DONT_CARE_SHIFT;
			if (win_set_focus_event(parent_fd, &fe_focus,
			    fe_focus_shift)) {
				perror("win_set_focus_event");
				EXIT(1);
			}
			fe_restore_focus.id = MS_MIDDLE;
			fe_restore_focus.value = 1;
			fe_restore_focus_shift = DONT_CARE_SHIFT;
			if (win_set_swallow_event(parent_fd,
			    &fe_restore_focus, fe_restore_focus_shift)) {
				perror("win_set_swallow_event");
				EXIT(1);
			}
			tv.tv_usec = 0;
			tv.tv_sec = 10;
			if (win_set_event_timeout(parent_fd, &tv)) {
				perror("win_set_event_timeout");
				EXIT(1);
			}
			break;

		case 'g':	/* Get state */
			show_settings(parent_fd);
			break;

		case 'm':	/* Keyboard follows 'm'ouse */
			fe_focus.id = LOC_WINENTER;
			fe_focus.value = 1;
			fe_focus_shift = DONT_CARE_SHIFT;
			if (win_set_focus_event(parent_fd, &fe_focus,
			    fe_focus_shift)) {
				perror("win_set_focus_event");
				EXIT(1);
			}
			fe_restore_focus.id = LOC_WINENTER;
			fe_restore_focus.value = 1;
			fe_restore_focus_shift = DONT_CARE_SHIFT;
			if (win_set_swallow_event(parent_fd,
			    &fe_restore_focus, fe_restore_focus_shift)) {
				perror("win_set_swallow_event");
				EXIT(1);
			}
			break;

		case 'r':	/* Set focus restorer */
			get_focus_from_args(&argc, &argv, (short *)&fe_restore_focus.id,
			    &fe_restore_focus.value, &fe_restore_focus_shift);
			if (win_set_swallow_event(parent_fd,
			    &fe_restore_focus, fe_restore_focus_shift)) {
				perror("win_set_swallow_event");
				EXIT(1);
			}
			break;

		case 's':	/* Set focus changer */
			get_focus_from_args(&argc, &argv, (short *)&fe_focus.id,
			    &fe_focus.value, &fe_focus_shift);
			if (win_set_focus_event(parent_fd, &fe_focus,
			    fe_focus_shift)) {
				perror("win_set_focus_event");
				EXIT(1);
			}
			break;

		case 't': {	/* Set event lock timeout */
			if (argc < 2) {
				(void)fprintf(stderr, "-t takes a seconds arg\n");
				EXIT(1);
			}
			argv++;
			argc--;
			tv.tv_usec = 0;
			tv.tv_sec = atoi(*argv);
			if (win_set_event_timeout(parent_fd, &tv)) {
				perror("win_set_event_timeout");
				EXIT(1);
			}
			break;
			}

		case 'h':	/* Help */
		case 'H':	/* Help */
		case '?':	/* Help */
			usage();
			EXIT(0);
		default:
			;
		}
		argv++;
		argc--;
	}
	EXIT(0);
}

static
show_settings(parent_fd)
	int	parent_fd;
{
	Firm_event focus;
	int shift;
	struct timeval tv;

	if (win_get_focus_event(parent_fd, &focus, &shift)) {
		perror("win_get_focus_event");
		exit(1);
	}
	show_focus(focus, shift, "keyboard focus set event");
	if (win_get_swallow_event(parent_fd, &focus, &shift)) {
		perror("win_get_swallow_event");
		exit(1);
	}
	show_focus(focus, shift, "keyboard focus restore event");
	if (win_get_event_timeout(parent_fd, &tv)) {
		perror("win_get_event_timeout");
		exit(1);
	}
	(void)fprintf(stderr, "Event timeout secs: %ld\n", tv.tv_sec);
}

static
usage()
{
	(void)fprintf(stderr, "Options are\t-c (set `c'lick-to-type)\n");
	(void)fprintf(stderr,
	  "\t\t-g (`g'et the state of settable options)\n");
	(void)fprintf(stderr, "\t\t-h (`h'elp)\n");
	(void)fprintf(stderr, "\t\t-m (keyboard follows `m'ouse)\n");
	(void)fprintf(stderr,
	  "\t\t-r event value shift_state (`r'estore kbd focus event)\n");
	(void)fprintf(stderr,
	  "\t\t-s event value shift_state (`s'et kbd focus event)\n");
	(void)fprintf(stderr, "\t\t-t secs (event lock `t'imeout)\n");
	(void)fprintf(stderr,
	  "Event samples:\tLOC_WINENTER, MS_LEFT, MS_MIDDLE\n");
	(void)fprintf(stderr, "Value samples:\tDOWN, ENTER, UP\n");
	(void)fprintf(stderr,
	  "Shift_state samples:\tSHIFT_DONT_CARE, SHIFT_ALL_UP, SHIFT_LEFT\n");
}

static
get_focus_from_args(argc_ptr, argv_ptr, event, value, shift)
	int *argc_ptr;
	char ***argv_ptr;
	register short *event;
	register int *value;
	register int *shift;
{
	char str[200];
	register char *arg;

	if (*argc_ptr < 4) {
		(void)fprintf(stderr, "%s arg count error\n", *argv_ptr);
		usage();
		exit (-1);
	}
	(*argc_ptr)--;
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "LOC_WINENTER") == 0)
		*event = LOC_WINENTER;
	else if (strcmp(arg, "MS_LEFT") == 0)
		*event = MS_LEFT;
	else if (strcmp(arg, "MS_MIDDLE") == 0)
		*event = MS_MIDDLE;
	else if (strcmp(arg, "MS_RIGHT") == 0)
		*event = MS_RIGHT;
	else if (sscanf(arg, "BUT%s", str) == 1)
		*event = atoi(str)+BUT_FIRST;
	else if (sscanf(arg, "KEY_LEFT%s", str) == 1)
		*event = atoi(str)+KEY_LEFTFIRST-1;
	else if (sscanf(arg, "KEY_RIGHT%s", str) == 1)
		*event = atoi(str)+KEY_RIGHTFIRST-1;
	else if (sscanf(arg, "KEY_TOP%s", str) == 1)
		*event = atoi(str)+KEY_TOPFIRST-1;
	else if (strcmp(arg, "KEY_BOTTOMLEFT") == 0)
		*event = KEY_BOTTOMLEFT;
	else if (strcmp(arg, "KEY_BOTTOMRIGHT") == 0)
		*event = KEY_BOTTOMRIGHT;
	else
		*event = atoi(arg);
	(*argc_ptr)--;
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "DOWN") == 0 || strcmp(arg, "Down") == 0 ||
	    strcmp(arg, "down") == 0)
		*value = 1;
	else if (strcmp(arg, "ENTER") == 0 || strcmp(arg, "Enter") == 0 ||
	    strcmp(arg, "enter") == 0)
		*value = 1;
	else if (strcmp(arg, "UP") == 0 || strcmp(arg, "Up") == 0 ||
	    strcmp(arg, "up") == 0)
		*value = 0;
	else
		*value = atoi(arg);
	(*argc_ptr)--;
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "SHIFT_LEFT") == 0)
		*shift = SHIFT_MASK(LEFTSHIFT);
	else if (strcmp(arg, "SHIFT_RIGHT") == 0)
		*shift = SHIFT_MASK(RIGHTSHIFT);
	else if (strcmp(arg, "SHIFT_LEFTCTRL") == 0)
		*shift = SHIFT_MASK(LEFTCTRL);
	else if (strcmp(arg, "SHIFT_RIGHTCTRL") == 0)
		*shift = SHIFT_MASK(RIGHTCTRL);
	else if (strcmp(arg, "SHIFT_DONT_CARE") == 0)
		*shift = DONT_CARE_SHIFT;
	else if (strcmp(arg, "SHIFT_ALL_UP") == 0)
		*shift = 0;
	else
		*shift = atoi(arg);

}

static
show_focus(focus, shift, tag)
	Firm_event	focus;
	int		shift;
	char		*tag;
{
	(void)fprintf(stdout, "State of %s:\n", tag);
	(void)fprintf(stdout, "\tId: ");
	switch (focus.id) {
	case LOC_WINENTER:
		(void)fprintf(stdout, "LOC_WINENTER\n");
		break;

	case MS_LEFT:
		(void)fprintf(stdout, "MS_LEFT\n");
		break;

	case MS_MIDDLE:
		(void)fprintf(stdout, "MS_MIDDLE\n");
		break;

	default: 
		(void)fprintf(stdout, "%ld\n", focus.id);
		break;
	}
	(void)fprintf(stdout, "\tValue: ");
	switch (focus.value) {
	case 0:
		(void)fprintf(stdout, "UP\n");
		break;

	case 1:
		(void)fprintf(stdout, "DOWN or ENTER\n");
		break;

	default:
		(void)fprintf(stdout, "%ld\n", focus.value);
	}
	(void)fprintf(stdout, "\tShift_state: ");
	switch (shift) {
	case DONT_CARE_SHIFT:
		(void)fprintf(stdout, "SHIFT_DONT_CARE\n");
		break;

	case SHIFT_MASK(LEFTSHIFT):
		(void)fprintf(stdout, "SHIFT_LEFT\n");
		break;

	case SHIFT_MASK(LEFTCTRL):
		(void)fprintf(stdout, "SHIFT_LEFTCTRL\n");
		break;

	case SHIFT_MASK(RIGHTSHIFT):
		(void)fprintf(stdout, "SHIFT_RIGHT\n");
		break;

	case SHIFT_MASK(RIGHTCTRL):
		(void)fprintf(stdout, "SHIFT_RIGHTCTRL\n");
		break;

	case 0:
		(void)fprintf(stdout, "SHIFT_ALL_UP\n");
		break;

	default:
		(void)fprintf(stdout, "%lx\n", shift);
	}
}

