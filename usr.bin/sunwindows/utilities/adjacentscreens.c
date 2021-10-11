#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)adjacentscreens.c 1.1 92/07/30";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * adjacentscreens.c: Notifies win sys of physical screen positions
 * of "nextto" screens to a "center" screen:
 *	adjacentscreens [-cm] center [[-neswtrbl] nextto] [-x]
 * where center & next are device file names (e.g., /dev/fb).
 * -x means suppress notification of "nextto" screens that "center" is
 * their only neighbor (default is to do the notification).
 */
 
#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif 

#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <stdio.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_struct.h>

static	char	devaux[SCR_POSITIONS][SCR_NAMESIZE], devbase[SCR_NAMESIZE];
static	struct	screen base, neighbors[SCR_POSITIONS];
enum	search_status {FOUND=0,FIND=2};
static	enum search_status search[SCR_POSITIONS], base_search;

#ifdef STANDALONE
main(argc, argv)
#else
int adjacentscreens_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	*progname = "adjacentscreens";
	char    **args;
	int	nextaux = -1, auxupdate = 1, i;
	int	adj_matchname();

	/*
	 * Initialize local variables.
	 */
	devbase[0] = '\0';
	base_search = FIND;
	for (i = 0; i < SCR_POSITIONS; ++i) {
		devaux[i][0] = '\0';
		search[i] = FOUND;
	}
	/*
	 * Parse cmd line.
	 */
	if (argc < 2)
		goto badinput;
	for (args = ++argv;*args;args++) {
		if (nextaux != -1) {
			(void) sscanf(*args, "%s", &devaux[nextaux][0]);
			search[nextaux] = FIND;
			nextaux = -1;
		} else if (*args[0] == '-') {
			switch ((*args)[1]) {
			case 'm': case 'M': case 'c': case 'C':
				if (*(args+1))
				    (void)sscanf(*(++args), "%s", devbase);
				break;
			case 'l': case 'L':
				nextaux = SCR_WEST;
				break;
			case 'r': case 'R':
				nextaux = SCR_EAST;
				break;
			case 't': case 'T':
				nextaux = SCR_NORTH;
				break;
			case 'b': case 'B':
				nextaux = SCR_SOUTH;
				break;
			case 'x':
				auxupdate = 0;
			default: {}
			}
		} else {
			(void) sscanf(*args, "%s", devbase);
		}
	}
	if (devbase[0] == '\0' ||  nextaux != -1) {
badinput:
		(void)printf("Bad cmd line.  Should be:\n");
		(void)printf("	%s [-cm] center [[-lrtb] nextto] [-x]\n",
		    progname);
		(void)printf("where center & nextto are device file names\n");
		(void)printf("(e.g., /dev/cgone0, /dev/fb).\n");
		EXIT(1);
	}
	/*
	 * Find window numbers associated with each device.
	 */
	if (win_enumall(adj_matchname) == -1) {
		(void)fprintf(stderr, "problem enumerating windows\n");
		perror(progname);
		EXIT(1);
	}
	if (base_search == FIND) {
		(void)fprintf(stderr, "%s has no windows on it\n", devbase);
		EXIT(1);
	}
	for (i = 0; i < SCR_POSITIONS; ++i)
		if (search[i] == FIND) {
			(void)fprintf(stderr, "%s has no windows on it\n",
				      devaux[i]);
			EXIT(1);
		}
	if (win_screenadj(&base, neighbors, auxupdate) == -1) {
		(void)fprintf(stderr, "problem making windows adjacent\n");
		perror(progname);
		EXIT(1);
	}
	EXIT(0);
}

static
adj_matchname(windowfd)
	int	windowfd;
{
	struct	screen screen;
	register int	i;

	(void)win_screenget(windowfd, &screen);
	/*
	 * Match device names.
	 */
	if (strncmp(devbase, screen.scr_fbname, SCR_NAMESIZE) == 0) {
		base = screen;
		base_search = FOUND;
	}
	for (i = 0; i < SCR_POSITIONS; ++i) {
		if (strncmp(devaux[i], screen.scr_fbname,
		    SCR_NAMESIZE) == 0) {
			neighbors[i] = screen;
			search[i] = FOUND;
		}
	}
	/* Avoid enumerating more than you have to */
	for (i = 0; i < SCR_POSITIONS; ++i)
		if (base_search == FIND || search[i] == FIND)
			return(0); /* Continue */
	return(1); /* Done */
}
