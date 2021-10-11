#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)set_alarm.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif
#endif

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include 	<sys/time.h>
#include 	<suntool/window.h>

#define		MILLISECONDS	1000

/*
 *  set_alarm.c
 *  This program sets the WINDOW_ALARM env var.
 */
main (argc, argv)
	int		 argc;
	char		*argv[];
{
	extern void	usage();
	extern char	*getenv();
	extern int	win_get_alarm(), getopt(), optind, opterr;
	extern char	*optarg;
	int		c, status, dur, flashes, beeps, csh=0 ;
	int		new_beeps, new_flashes, new_dur;
	char            strbeeps[BUFSIZ], strflashes[BUFSIZ], strdur[BUFSIZ];
	char		wa_envstr[BUFSIZ];
	char		*shell;
	Win_alarm	sa_alarm;

	opterr = 0;		/* Suppress getopt() default error handler. */

	/* get the current values */
	win_get_alarm(&sa_alarm);
	beeps = sa_alarm.beep_num;
	flashes = sa_alarm.flash_num;
	dur = sa_alarm.beep_duration.tv_sec * MILLISECONDS +
	      sa_alarm.beep_duration.tv_usec / MILLISECONDS ;

	while ( (c = getopt (argc, argv, "b:f:d:")) != -1 ) {
		switch (c) {
		case 'b':
			new_beeps = atoi (optarg);
			if (new_beeps >= 0)
				beeps= new_beeps;
			else 
				usage (argv[0]);
			break;
		case 'f':
                        new_flashes = atoi (optarg);
                        if (new_flashes >= 0)
                                flashes = new_flashes;
                        else
                                usage (argv[0]);
                        break;
		case 'd':
                        new_dur = atoi (optarg);
                        if (new_dur >= 0)
                                dur = new_dur;
                        else
                                usage (argv[0]);
                        break;
		default:
			usage (argv[0]);
			break;
		}
	}

	if (dur == 0) dur= 1000;

	(void)strcpy (wa_envstr, ":");

	(void)sprintf(strbeeps, "beeps=%d:", beeps);
	(void)strcat (wa_envstr, strbeeps);

	(void)sprintf(strflashes, "flashes=%d:", flashes);
	(void)strcat (wa_envstr, strflashes);

	(void)sprintf(strdur, "dur=%d:", dur);
	(void)strcat (wa_envstr, strdur);

	if ( (shell = getenv ("SHELL")) == NULL ) {	/* No SHELL?  Assume /bin/csh */
		csh++;
	} else {
		if ( (!strcmp (shell, "/bin/csh")) || (!strcmp (shell, "/bin/tcsh")) ) {
			csh++;
		}
	}

	/*
	 * Put the current parameters in the environment.
	 * If csh is non-zero, use the  C Shell manner of setting environment variables.
	 * If csh is zero, use the Bourne Shell manner of setting environment variables.
	 */
	if (csh) {
		(void)fprintf (stdout, "set noglob; setenv WINDOW_ALARM '");
		(void)fprintf (stdout, "%s", wa_envstr);
		(void)fprintf (stdout, "'; unset noglob;\n");
		(void)fflush (stdout);
	} else {
		(void)fprintf (stdout, "export WINDOW_ALARM; WINDOW_ALARM='");
		(void)fprintf (stdout, "%s", wa_envstr);
		(void)fprintf (stdout, "';\n");
		(void)fflush (stdout);
	}

	exit (0);
}



static void
usage (name)
	char *name;
{
	(void)fprintf (stdout,"Usage:  %s [-bfd] [Integer] \n", name); 
	exit (1);
}
