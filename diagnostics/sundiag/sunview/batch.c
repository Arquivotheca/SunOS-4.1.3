#ifndef lint
static  char sccsid[] = "@(#)batch.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/file.h>
#include <sys/time.h>
#include <malloc.h>
#include <assert.h>
#include "sundiag.h"
#include "sundiag_msg.h"

#define CONFOPTFILELEN          256     /* max # of chars in option filename */
#define SUNDIAGCONFPATHPFX      "/usr/adm/sundiaglog/configs/"
#define CONFIG_DIR      	"/usr/adm/sundiaglog/configs"
#define MAXCONFLINELEN          100     /* max line length of a config file */

typedef struct _conf_t {
        char    conf_opt_file[CONFOPTFILELEN]; /* name of the option file */
        int     conf_test_mins;		/* time to run a particular option file */
        int     conf_delay_mins;	/* additional delay in-between test cycles.*/
} conf_t;

extern FILE *conf_open (/* char *conffile */);
extern conf_t *conf_getnext (/* FILE *fp */);
extern void conf_close (/* FILE *fp */);
extern void exit (/*int status*/);
FILE    *batch_fp;	/* FILE pointer to the batch file in "/var/adm/sundiaglog/configs" */
conf_t  *batch_cf;	/* pointer to the conf_t structure */

/* Initialize flags first */
static int runtime=0;
int batch_mode = FALSE;
int batch_ready = FALSE;

/* Batch file processing routines */

/*
 * This is the first place it comes to
 * when batch_mode is TRUE.
 */
batch()
{
	batch_opt_file_proc();
	if (batch_mode) {
		if (batch_cf != NULL) {
			if (batch_cf->conf_test_mins > 0)
				batch_start_proc();
			else if (!(batch_cf->conf_delay_mins))
				batch();
		}
	}
}
/*
 * Come here every minute to examine the flags
 * Perform the appropriate actions if necessary.
 */
batch_pong(elapse_mins)
int elapse_mins;
{
	if (batch_cf != NULL) {
		if (running == GO) {
			batch_cf->conf_test_mins -= elapse_mins;
			if (batch_cf->conf_test_mins < 0)
				batch_cf->conf_test_mins = 0;
			
			if (batch_cf->conf_test_mins == 0)
				batch_stop_proc();
		}

		if (running != GO) {
			if (batch_cf->conf_delay_mins == 0) {
				if (running == IDLE)
					batch();
				else
					batch_ready = TRUE;
			} else {
				batch_cf->conf_delay_mins--;
				if (batch_cf->conf_delay_mins < 0)
					batch_cf->conf_delay_mins = 0;
			}
		}
	}
}

/*
 * Send the message to start sundiag testing
 */
batch_start_proc()
{
	if (running == IDLE) {
		start_proc();
		batch_mode = TRUE;	/* Enable it again */
	}
}

/*
 * Send the message to stop sundiag testing
 */
batch_stop_proc()
{
	char buff[250];
 
        if (running == GO) {
		stop_proc();
		sprintf(buff, batch_period_info, batch_cf->conf_opt_file, runtime, batch_cf->conf_delay_mins);
		logmsg(buff, -1, BATCH_PERIOD_INFO); 
		batch_mode = TRUE;	/* Enable it again */
	}
}

/*
 * Set the various batch flags depending on the
 * settings in the configuration file.
 * Send the message to load a specified option file.
 */
batch_opt_file_proc()
{
	if ((batch_cf = conf_getnext (batch_fp)) != (conf_t *) 0)
	{
		if (batch_cf->conf_test_mins < 0)
			batch_cf->conf_test_mins = 0;

		/* save runtime for printing purposes later */
		runtime = batch_cf->conf_test_mins;

		if (batch_cf->conf_delay_mins < 0)
			batch_cf->conf_delay_mins = 0;
		load_option(batch_cf->conf_opt_file, 2);
	}
	else     /* No more entries in batch file */
		batch_mode = FALSE;
	
}

/*
 * Create the /usr/adm/sundiaglog/configs directory
 * if it does not already exist.
 */
create_config_dir()
{
  char	temp[82];

  if (access(CONFIG_DIR, F_OK) != 0) /* create the configuration directory */
    if (mkdir(CONFIG_DIR, 0755) != 0)
    {
	(void)sprintf(temp, "mkdir %s", CONFIG_DIR);
	perror(temp);
	return(FALSE);
    }
  return(TRUE);
}

/*
 * Come here when disabling batch mode is the only option.
 */
disable_batch()
{
	if (batch_mode)
		batch_mode = FALSE;
}


/*
 * routines to parse test configuration files.
 *
 * the format is distressingly simple:
 *
 * # in first column denotes a comment, otherwise syntax is simply
 *
 * [space]* option-file-name [space]*1 test-time-in-minutes [space]*
 */

/*
 * open a configuration file and return a file pointer.
 *
 * configuration files can be specified by an absolute pathname
 * or are interpreted relative to the SUNDIAGCONFPATHPFX
 * directory (usually /var/adm/sundiaglog/configs)
 */

FILE *
conf_open (conffile)
	char *conffile;
{
	static  char    pathpfx[] = SUNDIAGCONFPATHPFX;
	char            *fullpath;
	FILE            *fp;
	unsigned        pathlen;

	if (*conffile != '/')
		pathlen = strlen (pathpfx) + strlen (conffile) + 1;
	else
		pathlen = strlen (conffile) + 1;

	fullpath = malloc (pathlen);

	if (fullpath == (char *) 0) {
                (void) fprintf (stderr, "conf_open: out of heap!\n");
                exit (1);

		/*NOTREACHED*/
	}

	if (*conffile != '/') {
		(void) strcpy (fullpath, pathpfx);
		(void) strcat (fullpath, conffile);
	}
	else
		(void) strcpy (fullpath, conffile);

	if (access (fullpath, R_OK) == -1) {
		fp = (FILE *) 0;
	}
	else
		fp = fopen (fullpath, "r");

	free (fullpath);
	return fp;
}


/*
 * close a configuration file - this is dead easy!
 */

void
conf_close (fp)
	FILE    *fp;
{
	if (fp != (FILE *) 0)
		(void) fclose (fp);
}


/*
 * move to the next valid entry in the configuration file
 * and return a pointer to a static variable containing
 * the data therein.
 */

conf_t *
conf_getnext (fp)
	FILE    *fp;
{
	static  char    linebuf[MAXCONFLINELEN];
	static  conf_t  conf;
	char            *lbuf;
	int             nmatched;
	conf_t          *cp = &conf;

	if (fp == (FILE *) 0) {
		return (conf_t *) 0;
		/*NOTREACHED*/
	}

#if !defined(lint)
	assert (CONFOPTFILELEN > MAXCONFLINELEN);
#endif

	while ((lbuf = fgets (linebuf, MAXCONFLINELEN, fp)) != (char *) 0) {
		if (*lbuf == '#' || *lbuf == '\n') {
			continue;
			/*NOTREACHED*/
		}
		nmatched = sscanf (lbuf, "%s %d %d",
				   cp->conf_opt_file, &cp->conf_test_mins, &cp->conf_delay_mins);
		if (nmatched == 3) {
			/* XXX probably should check that file exists */
			return cp;
			/*NOTREACHED*/
		}
		(void) fprintf (stderr,
                                "configuration file botch:\n>%s", linebuf);
                return (conf_t *) 0;
		/*NOTREACHED*/
	}

	return (conf_t *) 0;
}
