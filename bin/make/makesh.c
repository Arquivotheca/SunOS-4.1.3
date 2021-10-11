#ident "@(#)makesh.c 1.1 92/07/30 Copyright 1987,1988 Sun Micro"

/*
 *	makesh.c
 *
 *	This program is what make runs on remote systems to execute a command
 *	when running in parallel mode.
 */


#define PARALLEL

/*
 * Included files
 */
#include "defs.h"
#include "report.h"
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define COMPILING_MAKESH
#include "globals.c"
#undef COMPILING_MAKESH

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Static variables
 */

/*
 * File table of contents
 */
extern	void		main();
extern	void		handle_interrupt();
extern	Name		getvar();
extern	Property	setvar_daemon();
extern	void		await_parallel();
extern	void		build_suffix_list();
extern	void		finish_children();

/*
 *	main(argc, argv)
 *
 *	This program is used when make is running remote commands.
 *	It handles running the make command on the remote system.
 *
 *	Parameters:
 *		argc		You know what this is
 *		argv		You know what this is
 *
 *	Global variables referenced:
 *		cached_names	Names refd from it
 *		vroot_path	Set to path we use if env contaions no PATH
 */
void
main(argc, argv)
	register int		argc;
	register char		*argv[];
{
	Boolean			silent_flag;
	Boolean			ignore;
	Name			command;
	Doname			result;

	/* Usage: makesh path shell -silent/nosilent command command ... */
	if (argc < 4) {
		(void) fprintf(stderr, "makesh: Too few arguments\n");
		exit(1);
	}
	/* Make the vroot package scan the path using shell semantics */
	set_path_style(0);

	setup_char_semantics();

	load_cached_names();

	setup_interrupt();

	argv++;
#ifdef CHECK_CD
	if (chdir(*argv) < 0) {
		(void) sprintf(str,
			       "makesh: Unable to connect to %s from %s",
			       *argv,
			       get_current_path());
		perror(str);
		exit(1);
	}
#endif
	shell_name = GETNAME(*++argv, FIND_LENGTH);

	/*
	 * Make sure that the NSE_DEP environment variable reflects the current
	 * directory on this machine (which may be different due to symlinks).
	 */
	if (getenv("NSE_DEP") != NULL) {
		(void) setenv("NSE_DEP", get_current_path());
	}
	if ((*++argv)[1] == 's') {
		silent = true;
	} else {
		silent = false;
	}
	for (argv++; *argv; argv++) {
		if (**argv == (int) at_char) {
			silent_flag = true;
			(*argv)++;
		} else {
			silent_flag = false;
		}
		if (**argv == (int) hyphen_char) {
			ignore = true;
			(*argv)++;
		} else {
			ignore = false;
		}
		command = GETNAME(*argv, FIND_LENGTH);
		if ((command->hash.length > 0) &&
		    !silent_flag &&
		    !silent) {
			(void) printf("%s\n", command->string);
		}
		result = dosys(command,
			       ignore,
			       false,
			       BOOLEAN(silent_flag && ignore),
			       (Name) NULL);
		if (result == build_failed) {
			if (silent) {
				(void) printf("The following command caused the error:\n%s\n",
					      command->string);
			}
			if (!ignore) {
				exit(1);
			}
		}
	}
	exit(0);
	/* NOTREACHED */
}

/*
 *	handle_interrupt()
 *
 *	This is where C-C traps are caught
 *
 *	Parameters:
 *
 *	Global variables used:
 */
void
handle_interrupt()
{
	(void) fflush(stdout);
	/* Make sure the process running under us terminates first */
	while (wait((union wait *) NULL) != -1);
	exit(2);
}

/*
 *	getvar(name)
 *
 *	Return expanded value of macro. (simplified version from macro.c)
 *
 *	Return value:
 *				The Name passed in
 *	Parameters:
 *				A Name
 *	Global variables used:
 */
Name
getvar(name)
	register Name		name;
{
	return name;
}

/*
 *	setvar_daemon(name, value, append, daemon)
 *
 *	Set a macro value, possibly supplying a daemon to be used when
 *	referencing the value. (Simplified version from macro.c)
 *
 *	Return value:
 *				The macro value cell
 *
 *	Parameters:
 *		name		The name of the macro to set
 *		value		The value to set
 *		append		Should the value be appended to or set
 *		daemon		Daemon function to run when reading value
 *
 *	Global variables used:
 *		makefile_type	If reading a makefile some stuff is read only
 *		path_name	The Name "PATH", compared against
 *		virtual_root	The Name "VIRTUAL_ROOT", compared against
 *		vpath_name	The Name "VPATH against
 *		vpath_defined	Set if the macro VPATH is set
 */
Property
setvar_daemon(name, value, append, daemon)
	register Name		name;
	register Name		value;
	Boolean			append;
	Daemon			daemon;
{
	register Property	macro = maybe_append_prop(name, macro_prop);
	String_rec		destination;
	char			buffer[STRING_BUFFER_LENGTH];

	if ((makefile_type != reading_nothing) &&
	    macro->body.macro.read_only) {
		return macro;
	}
	if (append) {
		/* If we are appending we just tack the new value after the */
		/* old one with a space in between */
		INIT_STRING_FROM_STACK(destination, buffer);
		if ((macro != NULL) && (macro->body.macro.value != NULL)) {
			append_string(macro->body.macro.value->string,
				      &destination,
				      (int) macro->body.macro.value->
				      hash.length);
			if (value != NULL) {
				append_char((int) space_char, &destination);
			}
		}
		if (value != NULL) {
			append_string(value->string,
				      &destination,
				      (int) value->hash.length);
		}
		value = GETNAME(destination.buffer.start, FIND_LENGTH);
		if (destination.free_after_use) {
			retmem(destination.buffer.start);
		}
	}
	/* Set the new values in the macro property block */
	macro->body.macro.value = value;
	macro->body.macro.daemon = daemon;
	/* If the user changes the VIRTUAL_ROOT we need to flush the vroot */
	/* package cache */
	if (name == path_name) {
		flush_path_cache();
	}
	if (name == virtual_root) {
		flush_vroot_cache();
	}
	/* If this sets the VPATH we remember that */
	if ((name == vpath_name) &&
	    (value != NULL) &&
	    (value->hash.length > 0)) {
		vpath_defined = true;
	}
	return macro;
}

/* Misc.c calls these but we don't need them */

/*ARGSUSED*/
void
await_parallel(waitflg)
Boolean waitflg;
{
}

/*ARGSUSED*/
void
build_suffix_list(target_suffix)
Name target_suffix;
{
}

/*ARGSUSED*/
void
finish_children(docheck)
Boolean docheck;
{
}
