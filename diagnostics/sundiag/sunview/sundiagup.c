#ifndef	lint
static	char sccsid[] = "@(#)sundiagup.c 1.1 92/07/30 Copyright Sun Micro";
#endif

#include <stdio.h>
#include <errno.h>
#include <signal.h>

#define	ATTACH_TTY	"/tmp/sundiag.tty"
#define	SUNDIAG_PID	"/tmp/sundiag.pid"

extern	char	*fgets();
extern	char	*ttyname();
extern	char	*getenv();
extern	FILE	*fopen();

void handler()		/* signal handler for SIGTERM */
{
  (void)unlink(ATTACH_TTY);
  exit(0);		/* exits gracefully */
}

main(argc, argv)
int	argc;
char	*argv[];
{
  FILE	*fp;
  char	*msg, tmp[22];
  char	*term_type;
  int	pid=0;

  if (argc == 1)			/* no command line argument */
  {
    fp = fopen(SUNDIAG_PID, "r");

    if (fp == (FILE *)0)		/* no saved pid file */
    {
      fprintf(stderr,
		"%s: Sundiag is not running in the background.\n", argv[0]);
      exit(1);
    }
    else
    {
	pid = atoi(fgets(tmp, 20, fp));
	if (strncmp(fgets(tmp, 20, fp), "TTY", 3) != 0)
	{
	  fprintf(stderr,
		"%s: Sundiag is running in the SunView mode, can't grab.\n",
		argv[0]);
	  (void)fclose(fp);
	  exit(1);
	}
	(void)fclose(fp);
    }
  }
  else					/* ignore rest of the arg's for now */
    if (strcmp(argv[1], "sd") != 0)
      pid = atoi(argv[1]);

  if (pid > 0)
  {
    fp = fopen(ATTACH_TTY, "w");

    if (fp == (FILE *)0)
    {
	(void)fprintf(stderr,
		"%s: Must be a super user to run Sundiag.\n", argv[0]);
	exit(1);
    }

    (void)fprintf(fp, "%d\n%s", getpid(), ttyname(0));
    term_type = getenv("TERM");
    if (term_type != NULL)
	(void)fprintf(fp, "\n%s", term_type);
    (void)fclose(fp);

    if (kill(pid, SIGUSR1) != 0)
    {
      switch (errno)
      {
	case EINVAL:
	  msg = "Invalid signal number";
	  break;
	case ESRCH:
	  msg = "Invalid process id";
	  break;
	case EPERM:
	  msg = "Must be a super user to run Sundiag";
	  break;
	default:
	  msg = "Failed restarting Sundiag";
	  break;
      }
      (void)fprintf(stderr, "%s: %s.\n", argv[0], msg);
      exit(0);
    }

    if (argc > 1)
    {
	fp = fopen(SUNDIAG_PID, "w");	/* write out the PID, for next time */
	(void)fprintf(fp, "%d\nTTY\n", pid);
	(void)fclose(fp);
    }
  }

  (void)signal(SIGINT, SIG_IGN);
  (void)signal(SIGHUP, SIG_IGN);
  (void)signal(SIGTERM, handler);

  while (1)
    pause();		/* hold up the shell here until killed */
}
