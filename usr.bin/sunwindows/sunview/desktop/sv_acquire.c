#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)sv_acquire.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * sv_acquire.c -- change owner/group/permissions of window system devices
 *                 to uid/gid/<mode in SVDTAB>, if mode is listed in SVDTAB.
 *                 exit 0 on success, -1 on failure
 *
 * Arguments:
 *     sv_acquire [startwin [nwins [nbackground]]]
 *
 *     startwin    -- the first win to change
 *     nwins       -- the number of windows to change
 *     nbackground -- the number of wins to change in background
 *
 * Example:
 *     sv_acquire 0 256 240
 *
 *     This will set the uid/gid/mode of /dev/win0 through /dev/win15 in
 *     the foreground and will spawn a child to set /dev/win16 through
 *     /dev/win256 and exit with error status.  The purpose of spawning
 *     a background process is to allow the invoker to avoid waiting
 *     for all wins to be changed.  Note: a diskless 3/50 sets ~16 wins
 *     per second.
 *
 * Design Notes:
 *     This is a setuid root program spawned by suntools.c.
 *     Exit if user does not own console devices listed in /dev/fbtab.
 *
 *     Format of SVDTAB:
 *     mode
 *     # begins a comment and may appear anywhere on a line
 *
 *     Normal contents of SVDTAB:
 *     # /etc/svdtab -- SunView device table
 *     0660
 */

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>

#define SV_ACQUIRE	"/bin/sunview1/sv_acquire"
#define SVDTAB		"/etc/svdtab"	/* SunView device table */
#define FIELD_DELIMS	" \t\n"
#define MAX_LINELEN	128
#define MAX_WINDEVS	256


main(argc, argv)
    int argc;
    char **argv;
{
    struct passwd *pwd;
    long strtol();
    FILE *fp;
    char line[MAX_LINELEN];
    char path[MAX_LINELEN];
    char start_str[4];
    char nwins_str[4];
    char *mode_str;
    char *ptr;
    int  mode; 
    int  num;
    int  console_owner;
    int  startwin;
    int  nwins;
    int  nforeground;
    int  nbackground;

    (void)signal(SIGQUIT, SIG_IGN);
    (void)signal(SIGINT, SIG_IGN);

    /* default: do all windows in foreground */
    startwin = 0;
    nwins = MAX_WINDEVS;
    nbackground = 0;

    /* get starting window number */
    if (argc > 1) {
	startwin = atoi(argv[1]);
	if (startwin < 0)
	    startwin = 0;
    }

    /* get number wins to do */
    if (argc > 2) {
	nwins = atoi(argv[2]);
	if (nwins < 0 || nwins > MAX_WINDEVS)
	    nwins = MAX_WINDEVS;
    }

    /* get number of wins to do in background */
    if (argc > 3) {
	nbackground = atoi(argv[3]);
	if (nbackground < 0)
	    nbackground = 0;
    }

    nforeground = nwins-nbackground;

    /*
     * Get user information
     */
    if ((pwd = getpwuid(getuid())) == NULL) {
	(void)fprintf(stderr, "suntools: cannot access user information\n");
	return(-1);
    }

    /*
     * Make sure the user owns the console
     */
    console_owner = get_console_owner();
    if (console_owner != -1 && console_owner != pwd->pw_uid) {
	(void)fprintf(stderr, "You are not the owner of the console\n");
	return(-1);
    }

    /*
     * Set owner/group/permissions of devices listed in SVDTAB (/dev/win*)
     */
    if ((fp=fopen(SVDTAB, "r")) != NULL) {
	while (fgets(line, MAX_LINELEN, fp)) {
	    if ((ptr=strchr(line, '#')) != NULL) /* handle comments */
		*ptr = '\0';
	    if ((mode_str = strtok(line, FIELD_DELIMS)) == NULL)
		continue;
	    mode = (int)strtol(mode_str, (char **)NULL, 8);
	    if (mode <= 0000 || mode > 0777 ) {
		(void)fprintf(stderr, "%s: invalid mode -- %s\n",
			      SVDTAB, mode_str);
		continue;
	    }
	    for (num=0; num < nforeground; num++) {
		(void)sprintf(path, "%s%d", "/dev/win", startwin+num);
		if (chown(path, pwd->pw_uid, pwd->pw_gid) < 0)
		    break;
		(void)chmod(path, mode);
	    }

	    /* do the rest in the background */
	    if (nforeground < nwins) {
		switch (vfork()) {
		    case -1:		/* ERROR */
			(void)fprintf(stderr, "sv_acquire: Fork failed.\n");
			_exit(-1);

		    case 0:		/* CHILD */
			(void)sprintf(start_str, "%d", num);
			(void)sprintf(nwins_str, "%d", nwins-num);
			(void)execl(SV_ACQUIRE, "sv_acquire", start_str, nwins_str, "0", 0);
			/* should not return */
			(void)fprintf(stderr, "sv_acquire: exec for sv_acquire failed\n");
			_exit(-1);

		    default:		/* PARENT */
			/* do nothing */
			break;
		}
	    }
	}
	(void)fclose(fp);
    }

    (void)signal(SIGQUIT, SIG_DFL);
    (void)signal(SIGINT, SIG_DFL);

    return(0);
}


/*
 * get_console_owner -- get owner of first console listed in /etc/fbtab.
 *                      return uid of console owner
 *                      return -1 if no console in /etc/fbtab
 *
 * File format:
 * console mode device_name[:device_name ...]
 * # begins a comment and may appear anywhere on a line.
 *
 * Example:
 * /dev/console 0600 /dev/fb:/dev/cgtwo0:/dev/bwtwo0
 * /dev/console 0600 /dev/gpone0a:/dev/gpone0b 
 */

#define FIELD_DELIMS 	" \t\n"
#define FBTAB 		"/etc/fbtab"

int
get_console_owner()
{
    char line[MAX_LINELEN];
    char *console;
    char *ptr;
    struct stat buf;
    FILE *fp;
    int  uid;

    uid = -1;
    if ((fp = fopen(FBTAB, "r")) != NULL) {
	while (fgets(line, MAX_LINELEN, fp)) {
            if (ptr = strchr(line, '#')) 
                *ptr = '\0';
	    if ((console = strtok(line, FIELD_DELIMS)) == NULL)
		continue;		/* ignore blank lines */
	    if (stat(console, &buf) < 0)
		continue;
	    uid = buf.st_uid;
	    break;
	}
        (void)fclose(fp);
    }
    return(uid);
}
