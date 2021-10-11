#ident "@(#)state.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	state.c
 *
 *	This file contains the routines that write the .make.state file
 */

/*
 * Included files
 */
#include "defs.h"
#include <setjmp.h>
#include <sys/file.h>

/*
 * Defined macros
 */
#define LONGJUMP_VALUE 17
#define XFWRITE(string, length, fd) {if (fwrite(string, 1, length, fd) == 0) \
					longjmp(long_jump, LONGJUMP_VALUE);}
#define XPUTC(ch, fd) { \
	if (putc((int) ch, fd) == EOF) \
		longjmp(long_jump, LONGJUMP_VALUE); \
	}
#define XFPUTS(string, fd) fputs(string, fd)

/*
 * typedefs & structs
 */

/*
 * Static variables
 */

/*
 * File table of contents
 */
extern	void		write_state_file();
extern	void		print_auto_depes();

/*
 *	write_state_file(report_recursive)
 *
 *	Write a new version of .make.state
 *
 *	Parameters:
 *		report_recursive	Should only be done at end of run
 *
 *	Global variables used:
 *		built_last_make_run The Name ".BUILT_LAST_MAKE_RUN", written
 *		command_changed	If no command changed we do not need to write
 *		current_make_version The Name "<current version>", written
 *		do_not_exec_rule If -n is on we do not write statefile
 *		hashtab		The hashtable that contains all names
 *		keep_state	If .KEEP_STATE is no on we do not write file
 *		make_state	The Name ".make.state", used for opening file
 *		make_version	The Name ".MAKE_VERSION", written
 *		recursive_name	The Name ".RECURSIVE", written
 *		rewrite_statefile Indicates that something changed
 */
void
write_state_file(report_recursive)
	int			report_recursive;
{
	register int		n;
	register int		m;
	register Boolean	name_printed;
	register Name		np;
	register Cmd_line	cp;
	register FILE		*fd;
	register Property	lines;
	Property		cmd;
	Boolean			has_recursive;
	char			buffer[MAXPATHLEN];
	String_rec		rec;
	char			rec_buf[STRING_BUFFER_LENGTH];
	register int		attempts = 0;
	Dependency		dp;
	Boolean			built_this_run = false;
	int			line_length;
	char			*target_name;
	Dependency		dependency;
	jmp_buf			long_jump;

	if (!rewrite_statefile ||
	    !command_changed ||
	    !keep_state ||
	    do_not_exec_rule ||
	    report_dependencies_only) {
		return;
	}
	if ((fd = fopen(make_state->string, "w")) == NULL) {
		fatal("Could not open statefile `%s': %s",
		      make_state->string,
		      errmsg(errno));
	}
	(void) fchmod(fileno(fd), 0666);
	/* Lock the file for writing */
	make_state_lockfile = ".make.state.lock";
	(void) file_lock(make_state->string, make_state_lockfile, 0);
	/* Set a trap for failed writes. If a write fails the */
	/* routine will try saving the .make.state */
	/* file under another name in /tmp */
	if (setjmp(long_jump)) {
		(void) fclose(fd);
		if (attempts++ > 5) {
			if (make_state_lockfile != NULL) {
				(void) unlink(make_state_lockfile);
				make_state_lockfile = NULL;
			}
			fatal("Giving up on writing statefile");
		}
		sleep(10);
		(void) sprintf(buffer, "/tmp/.make.state.%d", getpid());
		if ((fd = fopen(buffer, "w")) == NULL) {
			fatal("Could not open statefile `%s': %s",
			      buffer,
			      errmsg(errno));
		}
		warning("Initial write of statefile failed. Trying again on %s",
			buffer);
	}

	/* Write the version stamp */
	XFWRITE(make_version->string,
		(int)make_version->hash.length,
		fd);
	XPUTC(colon_char, fd);
	XPUTC(tab_char, fd);
	XFWRITE(current_make_version->string,
		(int)current_make_version->hash.length,
		fd);
	XPUTC(newline_char, fd);

	if (report_recursive) {
		report_recursive_init();
	}
	/* Go thru all targets and dump their dependencies and command used */
	for (n = hashsize - 1; n >= 0; n--)
	    for (np = hashtab[n]; np != NULL; np = np->next) {
		/* Check if any .RECURSIVE lines should be written */
		if (np->has_recursive_dependency) {
			/* Go thru the property list and dump all */
			/* .RECURSIVE lines */
			INIT_STRING_FROM_STACK(rec, rec_buf);
			append_string(np->string, &rec, FIND_LENGTH);
			append_char((int) colon_char, &rec);
			has_recursive = false;
			for (lines = get_prop(np->prop, recursive_prop);
			     lines != NULL;
			     lines = get_prop(lines->next, recursive_prop)) {
				/*
				 * If the target was built during this run
				 * but the .RECURSIVE was not then do not
				 * print it. If the command_template is NULL
				 * then the .RECURSIVE could not have bee built
				 */
				cmd = get_prop(np->prop, line_prop);
				if ((np->has_built && 
				     !lines->body.recursive.has_built) ||
				    cmd->body.line.command_template == NULL) {
					continue;
				}
				has_recursive = true;

				/* If this target was built during this */
				/* make run we mark it */
				if (np->has_built) {
					XFWRITE(built_last_make_run->string,
						(int) built_last_make_run->
						hash.length,
						fd);
					XPUTC(colon_char, fd);
					XPUTC(newline_char, fd);
				}
				/* Write the .RECURSIVE line */
				XFWRITE(np->string, (int)np->hash.length, fd);
				XPUTC(colon_char, fd);
				XFWRITE(recursive_name->string,
					(int)recursive_name->hash.length,
					fd);
				XPUTC(space_char, fd);
				/* Directory the recursive make ran in */
				XFWRITE(lines->body.recursive.directory->
					string,
					(int)lines->body.recursive.directory->
					hash.length,
					fd);
				XPUTC(space_char, fd);
				/* Target made there */
				XFWRITE(lines->body.recursive.target->string,
					(int)lines->body.recursive.target->
					hash.length,
					fd);
				append_char((int) space_char, &rec);
				append_string(recursive_name->string,
					      &rec,
					      FIND_LENGTH);
				append_char((int) space_char, &rec);
				append_string(lines->body.recursive.directory->
					      string,
					      &rec,
					      FIND_LENGTH);
				append_char((int) space_char, &rec);
				append_string(lines->body.recursive.target->
					      string,
					      &rec,
					      FIND_LENGTH);
				/* Complete list of makefiles used */
				for (dp = lines->body.recursive.makefiles;
				     dp != NULL;
				     dp = dp->next) {
					XPUTC(space_char, fd);
					XFWRITE(dp->name->string, 
						(int)dp->name->hash.length,
						fd);
					append_char((int) space_char, &rec);
					append_string(dp->name->string,
						      &rec,
						      FIND_LENGTH);
				}
				XPUTC(newline_char, fd);
			}

			/*
			 * If this target was build during this run,
			 * then remove all its old .RECURSIVE entries
			 * from the .nse_depinfo list.
			 * Then, if the target has any new .RECURSIVEs,
			 * enter them.
			 */
			cmd = get_prop(np->prop, line_prop);
			if (np->has_built ||
			    cmd->body.line.command_template == NULL) {
				remove_recursive_dep(np->string);
			}
			if (has_recursive && report_recursive) {
				report_recursive_dep(rec.buffer.start);
			}
		} else {
			remove_recursive_dep(np->string);
		}
		/* If the target has no command used nor dependencies */
		/* we can go to the next one */
		if ((lines = get_prop(np->prop, line_prop)) == NULL) {
			continue;
		}
		/* If this target is a special target - don't print */
		if (np->special_reader != no_special) {
			continue;
		}
		/* Find out if any of the targets dependencies should */
		/* be written to .make.state */
		for (m = 0, dependency = lines->body.line.dependencies;
		     dependency != NULL;
		     dependency = dependency->next) {
			if (m = !dependency->stale
			    && (dependency->name != force)
#ifndef PRINT_EXPLICIT_DEPEN
			    && dependency->automatic
#endif
			    ) {
				break;
			}
		}
		/* only print if dependencies listed or non-sccs command */
		if (m ||
		    ((lines->body.line.command_used != NULL) &&
		     !lines->body.line.sccs_command)) {
			name_printed = false;
			/* If this target was built during this make */
			/* run we mark it */
			built_this_run = false;
			if (np->has_built) {
				built_this_run = true;
				XFWRITE(built_last_make_run->string,
					(int) built_last_make_run->hash.length,
					fd);
				XPUTC(colon_char, fd);
				XPUTC(newline_char, fd);
			}
			/* If the target has dependencies we dump them */
			target_name = np->string;
			if (np->has_long_member_name) {
				target_name =
				  get_prop(np->prop, long_member_name_prop)
				    ->body.long_member_name.member_name->
				      string;
			}
			if (m) {
				XFPUTS(target_name, fd);
				XPUTC(colon_char, fd);
				XFPUTS("\t", fd);
				name_printed = true;
				line_length = 0;
				for (dependency =
				     lines->body.line.dependencies;
				     dependency != NULL;
				     dependency = dependency->next) {
					print_auto_depes(dependency,
							 fd,
							 built_this_run,
							 &line_length,
							 target_name,
							 long_jump);
				}
				XFPUTS("\n", fd);
			}
			/* If there is a command used we dump it */
			if (lines->body.line.command_used != NULL) {
				/* Only write the target name if it */
				/* wasnt done for the dependencies */
				if (!name_printed) {
					XFPUTS(target_name, fd);
					XPUTC(colon_char, fd);
					XPUTC(newline_char, fd);
				}
				/* Write the command lines. Prefix each */
				/* textual line with a tab */
				for (cp = lines->body.line.command_used;
				     cp != NULL;
				     cp = cp->next) {
					char                   *csp;
					int                     n;
					XPUTC(tab_char, fd);
					if (cp->command_line != NULL) {
						for (csp = cp->command_line->
						     string,
						     n = cp->command_line->
						     hash.length;
						     n > 0;
						     n--, csp++) {
							XPUTC(*csp, fd);
							if (*csp ==
							    (int) newline_char) {
								XPUTC(tab_char,
								      fd);
							}
						}
					}
					XPUTC(newline_char, fd);
				}
			}
		}
	}
	if (report_recursive) {
		report_recursive_done();
	}
	if (fclose(fd) == EOF) {
		longjmp(long_jump, LONGJUMP_VALUE);
	}
	if (make_state_lockfile != NULL) {
		(void) unlink(make_state_lockfile);
		make_state_lockfile = NULL;
	}
}

/*
 *	print_auto_depes(dependency, fd, built_this_run,
 *					line_length, target_name, long_jump)
 *
 *	Will print a dependency list for automatic entries.
 *
 *	Parameters:
 *		dependency	The dependency to print
 *		fd		The file to print it to
 *		built_this_run	If on we prefix each line with .BUILT_THIS...
 *		line_length	Pointer to line length var that we update
 *		target_name	We need this when we restart line
 *		long_jump	setjmp/longjmp buffer used for IO error action
 *
 *	Global variables used:
 *		built_last_make_run The Name ".BUILT_LAST_MAKE_RUN", written
 *		force		The Name " FORCE", compared against
 */
static void
print_auto_depes(dependency, fd, built_this_run,
		 line_length, target_name, long_jump)
	register Dependency	dependency;
	register FILE		*fd;
	register Boolean	built_this_run;
	register int		*line_length;
	register char		*target_name;
	jmp_buf			long_jump;
{
	if (!dependency->automatic ||
	    dependency->stale ||
	    (dependency->name == force)) {
		return;
	}
	XFWRITE(dependency->name->string,
		(int)dependency->name->hash.length,
		fd);
	/* Check if the dependency line is too long. If so break it */
	/* and start a new one */
	if ((*line_length += dependency->name->hash.length + 1) > 450) {
		*line_length = 0;
		XPUTC(newline_char, fd);
		if (built_this_run) {
			XFPUTS(built_last_make_run->string, fd);
			XPUTC(colon_char, fd);
			XPUTC(newline_char, fd);
		}
		XFPUTS(target_name, fd);
		XPUTC(colon_char, fd);
		XPUTC(tab_char, fd);
	} else {
		XFPUTS(" ", fd);
	}
	return;
}

