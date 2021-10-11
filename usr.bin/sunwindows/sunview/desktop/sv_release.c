#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)sv_release.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * sv_release.c -- change owner/group/permissions of window system devices
 *                 to 0/0/<mode in SVDTAB>, if mode is listed in SVDTAB.
 *                 exit 0 on success, -1 on failure
 *
 * Design Notes:
 *     This is a setuid root program spawned by suntools.c.
 *     Exit if user does not own console device listed in /etc/fbtab
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

#define SV_RELEASE	"/bin/sunview1/sv_release"
#define SVDTAB		"/etc/svdtab"	/* SunView device table */
#define FIELD_DELIMS	" \t\n"
#define MAX_LINELEN	128
#define MAX_WINDEVS	256


main()
{
    struct passwd *pwd;
    long strtol();
    FILE *fp;
    char line[MAX_LINELEN];
    char path[MAX_LINELEN];
    char *mode_str;
    char *ptr;
    int  mode; 
    int  num;
    int  console_owner;

    (void)signal(SIGQUIT, SIG_IGN);
    (void)signal(SIGINT, SIG_IGN);

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
	    for (num=0; num < MAX_WINDEVS; num++) {
		(void)sprintf(path, "%s%d", "/dev/win", num);
		if (chown(path, 0, 0) < 0)
		    break;
		(void)chmod(path, mode);
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
