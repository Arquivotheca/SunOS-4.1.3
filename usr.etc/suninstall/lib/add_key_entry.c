#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)add_key_entry.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)add_key_entry.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		add_key_entry.c
 *
 *	Description:	This file contains the routines for manipulating
 *		keyed entries in text files.  These routines do not take
 *		NIS into account and they shouldn't do so.
 */

#include <errno.h>
#include <stdio.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();




/*
 *	Local functions:
 */
static	void		filter();




/*
 *	Name:		add_key_entry()
 *
 *	Description:	Add a 'key' and a 'value' to 'file'.  If an entry
 *		matching 'key' already exists, then it is replaced.
 *		If flag is set to KEY_OR VALUE then also check if an entry
 *		matching 'value' exists and replace its key
 */

void
add_key_entry(key, value, file, flag)
	char *		key;
	char *		value;
	char *		file;
	int		flag;
{
	char		cmd[MAXPATHLEN];
	char		cmd2[MAXPATHLEN];
	char            cmd3[MAXPATHLEN];
	char            cmd4[MAXPATHLEN];
	FILE *		pp;


	(void) sprintf(cmd, "grep -s '^%s[ \t]' %s > /dev/null 2>&1",
		       key, file);

	(void) sprintf(cmd2, "grep -s '^%s$' %s > /dev/null 2>&1",
		       key, file);

	(void) sprintf(cmd3, "grep -s '[ \t]%s$' %s > /dev/null 2>&1",
			value, file);

	(void) sprintf(cmd4, "grep -s '^%s$' %s > /dev/null 2>&1",
			value, file);



	/*
	 *	Cannot use x_system() here since we want the return value.
	 */
	if ((system(cmd) == 0) || (system(cmd2) == 0))
		replace_key_entry(key, value, file);
	else if ((flag == KEY_OR_VALUE) && ((system(cmd3) == 0) || (system(cmd4) == 0)))
		replace_value_entry(key, value, file);
	else {
		(void) sprintf(cmd, "ed %s > /dev/null 2>&1", file);

		pp = popen(cmd, "w");
		if (pp == NULL) {
#ifdef TEST_JIG
			menu_log("Adding '%s' entry to %s", key, file);
			return;
#else
			menu_log("%s: '%s': command failed.", progname, cmd);
			menu_abort(1);
#endif
		}

		(void) fprintf(pp, "$\n");
		(void) fprintf(pp,
		    "?^#\tEnd of lines added by the suninstall program.?--\n");
		(void) fprintf(pp, "a\n");

		if (value && *value)
			(void) fprintf(pp, "%s\t%s\n", key, value);
		else
			(void) fprintf(pp, "%s\n", key);

		(void) fprintf(pp, ".\nw\nw\nq\n");
		(void) pclose(pp);
	}
} /* end add_key_entry() */




/*
 *	Filter a source into a destination:
 *
 *		'/'  => '\/'
 *		'\'  => '\\'
 *		'\n' => '\\n'
 */
static	void
filter(src_p, dest_p)
	char *		src_p;
	char *		dest_p;
{
	while (*src_p) {
		switch (*src_p) {
		case '\n':
		case '/':
		case '\\':
			*(dest_p++) = '\\';
			break;
		}

		*(dest_p++) = *(src_p++);
	}
} /* end filter() */




/*
 *	Name:		replace_key_entry()
 *
 *	Description:	Replace an entry matching 'key' in 'file' with
 *		the new 'value'.  This routine is typically called
 *		from add_key_entry().
 */

void
replace_key_entry(key, value, file)
	char *		key;
	char *		value;
	char *		file;
{
	char		cmd[MAXPATHLEN * 3];	/* command buffer */
	char		new_key[MAXPATHLEN];	/* filtered key */
	char		new_value[MAXPATHLEN];	/* filtered value */


	bzero(new_key, sizeof(new_key));
	filter(key, new_key);

	bzero(new_value, sizeof(new_value));
	filter(value, new_value);

	(void) sprintf(cmd,
		       "%s%s%s %s %s %s %s %s %s %s %s%s%s%s%s %s %s > %s.tmp",
		       "sed -e '/^", new_key, "[ \t]/ !b done'",
		       "-e ':top'",
		       "-e '/\\\\$/ !b got_it'",
		       "-e 's/\\\\//g'",
		       "-e 'N'",
		       "-e 's/\\n//g'",
		       "-e 'b top'",
		       "-e ':got_it'",
		       "-e 's~.*~", new_key, "\t", new_value, "~'",
		       "-e ':done'",
		       file, file);

	x_system(cmd);

	(void) sprintf(cmd, "cp %s.tmp %s; rm -f %s.tmp", file, file, file);
	x_system(cmd);
} /* end replace_key_entry() */

/*
 *      Name:           replace_value_entry()
 *
 *      Description:    Replace an entry matching 'value' in 'file' with
 *              the new 'key'.  This routine is typically called
 *              from add_key_entry().
 */

void
replace_value_entry(key, value, file)
	char *          key;
	char *          value;
	char *          file;
{
	char            cmd[MAXPATHLEN * 3];    /* command buffer */
	char            new_key[MAXPATHLEN];    /* filtered key */
	char            new_value[MAXPATHLEN];  /* filtered value */


	bzero(new_key, sizeof(new_key));
	filter(key, new_key);

	bzero(new_value, sizeof(new_value));
	filter(value, new_value);

	(void) sprintf(cmd,
		"%s%s%s %s %s %s %s %s %s %s %s%s%s%s%s %s %s > %s.tmp",
		"sed -e '/[ \t]", new_value, "\$/ !b done'",
		"-e ':top'",
		"-e '/\\\\$/ !b got_it'",
		"-e 's/\\\\//g'",
		"-e 'N'",
		"-e 's/\\n//g'",
		"-e 'b top'",
		"-e ':got_it'",
		"-e 's~.*~", new_key, "\t", new_value, "~'",
		"-e ':done'",
		file, file);

	x_system(cmd);

	(void) sprintf(cmd, "cp %s.tmp %s; rm -f %s.tmp", file, file, file);
	x_system(cmd);
} /* end replace_value_entry() */

