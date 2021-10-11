#ident "@(#)rep.c 1.1 92/07/30 Copyright 1987,1988 Sun Micro"

/*
 *	rep.c
 *
 *	This file handles the .nse_depinfo file
 */

/*
 * Included files
 */ 
#include "defs.h"
#include "report.h"

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */
typedef struct Recursive_make		*Recursive_make, Recursive_make_rec;

struct	Recursive_make {
	Recursive_make next;		/* Linked list */
	char	*target;		/* Name of target */
	char	*oldline;		/* Original line in .nse_depinfo */
	char	*newline;		/* New line in .nse_depinfo */
	Boolean	removed;		/* This target is no longer recursive*/
};

/*
 * Static variables
 */
static	Recursive_make	recursive_list;
static	Recursive_make	*bpatch = &recursive_list;
static	Boolean		changed;

/*
 * File table of contents
 */
extern	void		report_recursive_init();
extern	void		report_recursive_dep();
extern	Recursive_make	find_recursive_target();
extern	void		remove_recursive_dep();
extern	void		report_recursive_done();

/*
 *	report_recursive_init()
 *
 *	Read the .nse_depinfo file and make a list of all the
 *	.RECURSIVE entries.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		bpatch		Points to slot where next cell should be added
 *
 *	Global variables used:
 *		recursive_name	The Name ".RECURSIVE", compared against
 */
void
report_recursive_init()
{
	FILE		*fp;
	char		*search_dir;
	char		*colon;
	char		line[MAXPATHLEN];
	char		nse_depinfo[MAXPATHLEN];
	Recursive_make	rp;

	search_dir = getenv("NSE_DEP");
	if (search_dir == NULL) {
		return;
	}
	(void) sprintf(nse_depinfo, "%s/%s", search_dir, NSE_DEPINFO);
	fp = fopen(nse_depinfo, "r");
	if (fp == NULL) {
		return;
	}
	while (fgets(line, sizeof line, fp) != NULL) {
		colon = strchr(line, (int) colon_char);
		if (colon == NULL) {
			continue;
		}
		line[strlen(line) - 1] = (int) nul_char;
		if (IS_EQUALN(&colon[2],
			      recursive_name->string,
			      (int) recursive_name->hash.length)) {
			rp = ALLOC(Recursive_make);
			(void) bzero((char *) rp, sizeof (Recursive_make_rec));
			rp->oldline = strdup(line);
			*colon = (int) nul_char;
			rp->target = strdup(line);
			*bpatch = rp;
			bpatch = &rp->next;
		}
	}
	(void) fclose(fp);
}

/*
 *	report_recursive_dep(line)
 *
 *	Report a target as recursive.
 *
 *	Parameters:
 *		line		Dependency line reported
 *
 *	Static variables used:
 *		bpatch		Points to slot where next cell should be added
 *		changed		Written if report set changed
 */
void
report_recursive_dep(line)
	char		*line;
{
	char		*colon;
	Recursive_make	rp;

	colon = strchr(line, (int) colon_char);
	if (colon == NULL) {
		return;
	}
	*colon = (int) nul_char;
	rp = find_recursive_target(line);
	if (rp == NULL) {
		rp = ALLOC(Recursive_make);
		(void) bzero((char *) rp, sizeof (Recursive_make_rec));
		rp->target = strdup(line);
		*colon = (int) colon_char;
		rp->newline = strdup(line);
		*bpatch = rp;
		bpatch = &rp->next;
		changed = true;
	} else {
		*colon = (int) colon_char;
		if (!IS_EQUAL(rp->oldline, line)) {
			rp->newline = strdup(line);
			changed = true;
		}
		rp->removed = false;
	}
}

/*
 *	find_recursive_target(target)
 *
 *	Search the list for a given target.
 *
 *	Return value:
 *				The target cell
 *
 *	Parameters:
 *		target		The target we need
 *
 *	Static variables used:
 *		recursive_list	The list of targets
 */
static Recursive_make
find_recursive_target(target)
	char		*target;
{
	Recursive_make	rp;

	for (rp = recursive_list; rp != NULL; rp = rp->next) {
		if (IS_EQUAL(rp->target, target)) {
			return rp;
		}
	}
	return NULL;
}

/*
 *	remove_recursive_dep(target)
 *
 *	Mark a target as no longer recursive.
 *
 *	Parameters:
 *		target		The target we want to remove
 *
 *	Static variables used:
 *		changed		Written if report set changed
 */
void
remove_recursive_dep(target)
	char		*target;
{
	Recursive_make	rp;

	rp = find_recursive_target(target);
	if (rp != NULL) {
		rp->removed = true;
		changed = true;
	}
}

/*
 *	report_recursive_done()
 *
 *	Write the .nse_depinfo file.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		recursive_list	The list of targets
 *		changed		Written if report set changed
 *
 *	Global variables used:
 *		recursive_name	The Name ".RECURSIVE", compared against
 */
void
report_recursive_done()
{
	FILE		*ifp;
	FILE		*ofp;
	char		*space;
	char		*search_dir;
	char		*data;
	char		nse_depinfo[MAXPATHLEN];
	char		tmpfile[MAXPATHLEN];
	char		lockfile[MAXPATHLEN];
	char		line[MAXPATHLEN];
	char		*err;
	Recursive_make	rp;

	if (changed == false) {
		return;
	}
	search_dir = getenv("NSE_DEP");
	if (search_dir == NULL) {
		return;
	}
	(void) sprintf(nse_depinfo, "%s/%s", search_dir, NSE_DEPINFO);
	(void) sprintf(tmpfile, "%s.%d", nse_depinfo, getpid());
	ofp = fopen(tmpfile, "w");
	if (ofp == NULL) {
		(void) fprintf(stderr,
			       "Cannot open `%s' for writing\n",
			       tmpfile);
		return;
	}
	(void) sprintf(lockfile, "%s/%s", search_dir, NSE_DEPINFO_LOCK);
	err = file_lock(nse_depinfo, lockfile, 0);
	if (err) {
		(void) fprintf(stderr, "%s\n", err);
		return;
	}
	ifp = fopen(nse_depinfo, "r");
	if (ifp != NULL) {
		/*
		 * Copy all the non-.RECURSIVE lines from the old file to
		 * the new one.
		 */
		while (fgets(line, sizeof line, ifp) != NULL) {
			space = strchr(line, (int) space_char);
			if (space != NULL && 
			    IS_EQUALN(&space[1],
				      recursive_name->string,
				      (int) recursive_name->hash.length)) {
				continue;
			}
			(void) fprintf(ofp, "%s", line);
		}
		(void) fclose(ifp);
	}

	/*
	 * Write out the .RECURSIVE lines.
	 */
	for (rp = recursive_list; rp != NULL; rp = rp->next) {
		if (rp->removed) {
			continue;
		}
		if (rp->newline != NULL) {
			data = rp->newline;
		} else {
			data = rp->oldline;
		}
		if (data != NULL) {
			(void) fprintf(ofp, "%s\n", data);
		}
	}
	(void) fclose(ofp);
	(void) rename(tmpfile, nse_depinfo);
	(void) unlink(lockfile);
}
