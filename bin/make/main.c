#ident "@(#)main.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	main.c
 *
 *	make program main routine plus some helper routines
 */

/*
 * Included files
 */
#include "defs.h"
#include "report.h"
#include <signal.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/dk.h>
#include <rpcsvc/rstat.h>

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Static variables
 */
static	Boolean		env_wins;			/* `-e' */
static	Boolean		ignore_default_mk;		/* `-r' */
static	Dependency_rec  not_auto_depen_struct;
static	Dependency 	not_auto_depen = &not_auto_depen_struct;
static	Boolean		trace_status;			/* `-p' */

/*
 * File table of contents
 */
extern	void		main();
extern	int		cleanup_after_exit();
extern	void		handle_interrupt();
extern	void		doalarm();
extern	Name		read_command_options();
extern	int		parse_command_option();
extern	void		set_target_host_arch();
extern	void		setup_for_projectdir();
extern	void		read_files_and_state();
extern	void		read_environment();
extern	Boolean		read_makefile();
extern	void		make_targets();
extern	void		print_dependencies();
extern	void		print_more_deps();
extern	void		print_deps();
extern	Boolean		should_print_dep();
extern	void		report_recursion();


/*
 *	main(argc, argv)
 *
 *	Parameters:
 *		argc			You know what this is
 *		argv			You know what this is
 *
 *	Static variables used:
 *		trace_status		make -p seen
 *
 *	Global variables used:
 *		debug_level		Should we trace make actions?
 *		keep_state		Set if .KEEP_STATE seen
 *		makeflags		The Name "MAKEFLAGS", used to get macro
 *		remote_command_name	Name of remote invocation cmd ("on")
 *		running_list		List of parallel running processes
 *		stdout_stderr_same	true if stdout and stderr are the same
 *		auto_dependencies	The Name "SUNPRO_DEPENDENCIES"
 *		temp_file_directory	Set to the dir where we create tmp file
 *		trace_reader		Set to reflect tracing status
 *		working_on_targets	Set when building user targets
 */
void
main(argc, argv)
	register int		argc;
	register char		*argv[];
{
	register char		*cp;
	register Property	macro;
	struct stat		out_stat, err_stat;
	Boolean			parallel_flag = false;
	struct sigvec		vec;
	Name			make_machines;

	/* find out if stdout and stderr point to the same place */
	if (fstat(1, &out_stat) < 0) {
		fatal("fstat of standard out failed: %s", errmsg(errno));
	}
	if (fstat(2, &err_stat) < 0) {
		fatal("fstat of standard error failed: %s", errmsg(errno));
	}
	if ((out_stat.st_dev == err_stat.st_dev) &&
	    (out_stat.st_ino == err_stat.st_ino)) {
		stdout_stderr_same = true;
	} else {
		stdout_stderr_same = false;
	}
	/* Make the vroot package scan the path using shell semantics */
	set_path_style(0);

	setup_char_semantics();

	setup_for_projectdir();

	/* if running with .KEEP_STATE curdir will be set with */
	/* the connected directory */
	(void) on_exit(cleanup_after_exit, (char *) NULL);

	load_cached_names();

	set_target_host_arch();

/*
 *	Set command line flags
 */
	for (cp = getenv(makeflags->string);
	     (cp != NULL) && (*cp != (int) nul_char);
	     cp++) {
		(void) parse_command_option(*cp);
	}
	make_machines = read_command_options(argc, argv);
	if (debug_level > 0) {
		cp = getenv(makeflags->string);
		(void) printf("MAKEFLAGS value: %s\n", cp == NULL ? "":cp);
	}
			     

	read_files_and_state(argc, argv);

/*
 *	Set the parallel variables in the environment
 */
	macro = get_prop(remote_command_name->prop, macro_prop);
	if (macro != NULL) {
		macro->body.macro.exported = true;
		(void) SETVAR(remote_command_name,
			      macro->body.macro.value,
			      false);
	}

#ifdef PARALLEL
	parallel_flag = read_make_machines(make_machines);
#else
	parallel_flag = false;
#endif
		
	setup_interrupt();

/*
 *	Enable interrupt handler for alarms
 */
	vec.sv_handler = doalarm;
	vec.sv_mask = 0;
	vec.sv_flags = SV_INTERRUPT;
	if (sigvec(SIGALRM, &vec, (struct sigvec *) NULL) < 0) {
		fatal("sigvec for alarm failed: %s", errmsg(errno));
	}

/*
 *	Check if make should report
 */
	if (getenv(sunpro_dependencies->string) != NULL) {
		FILE *report_file;
		report_dependency("");
		report_file = get_report_file();
		if ((report_file != NULL) && (report_file != (FILE*)-1)) {
			(void) fprintf(report_file, "\n");
		}
	}

/*
 *	Make sure SUNPRO_DEPENDENCIES is exported (or not) properly
 *      and NSE_DEP.
 */
	if (keep_state) {
		maybe_append_prop(sunpro_dependencies, macro_prop)->
		  body.macro.exported = true;
		(void) SETVAR(sunpro_dependencies,
			     GETNAME("", FIND_LENGTH),
			     false);
		(void) setenv("NSE_DEP", get_current_path());
	} else {
		maybe_append_prop(sunpro_dependencies, macro_prop)->
		  body.macro.exported = false;
	}

	working_on_targets = true;
	if (trace_status) {
		dump_make_state();
		exit(0);
	}
	trace_reader = false;
	temp_file_directory = get_current_path();

	make_targets(argc, argv, parallel_flag);

	exit(0);
	/* NOTREACHED */
}

/*
 *	cleanup_after_exit()
 *
 *	Called from exit(), performs cleanup actions.
 *
 *	Parameters:
 *		status		The argument exit() was called with
 *
 *	Global variables used:
 *		command_changed	Set if we think .make.state should be rewritten
 *		current_line	Is set we set commands_changed
 *		do_not_exec_rule True if -n flag on
 *		done		The Name ".DONE", rule we run
 *		keep_state	Set if .KEEP_STATE seen
 *		parallel	True if building in parallel
 *		quest		If -q is on we do not run .DONE
 *		report_dependencies True if -P flag on
 *		running_list	List of parallel running processes
 *		temp_file_name	The temp file is removed, if any
 */
int
cleanup_after_exit(status)
	int	status;
{
	Running		rp;
	Property	line;
	char		push_cmd[MAXPATHLEN + MAXPATHLEN +
				 NSE_TFS_PUSH_LEN + 3];
	char		*active;

	parallel = false;
	/* Build the target .DONE or .FAILED if we caught an error */
	if (!quest) {
		Name		failed_name;

		failed_name = GETNAME(".FAILED", FIND_LENGTH);
		if ((status != 0) && (failed_name->prop != NULL)) {
			(void) doname(failed_name, false, true);
		} else {
			(void) doname(done, false, true);
		}
	}
	/* Remove the temp file utilities report dependencies thru if it is */
	/* still around */
	if (temp_file_name != NULL) {
		(void) unlink(temp_file_name->string);
	}
	/* Do not save the current command in .make.state if make */
	/* was interrupted */
	if (current_line != NULL) {
		command_changed = true;
		current_line->body.line.command_used = NULL;
	}
	/*
	 * For each parallel build process running, remove the temp files
	 * and zap the command line so it won't be put in .make.state
	 */
	for (rp = running_list; rp != NULL; rp = rp->next) {
		if (rp->temp_file != NULL) {
			(void) unlink(rp->temp_file->string);
		}
		if (rp->stdout_file != NULL) {
			(void) unlink(rp->stdout_file->string);
		}
		if (rp->stderr_file != NULL) {
			(void) unlink(rp->stderr_file->string);
		}
		command_changed = true;
		line = get_prop(rp->target->prop, line_prop);
		if (line != NULL) {
			line->body.line.command_used = NULL;
		}
	}
	/* Remove the statefile lock file */
	if (make_state_lockfile != NULL) {
		(void) unlink(make_state_lockfile);
	}
	/* Write .make.state */
	write_state_file(1);
        /* If running inside an activated environment, push the */
	/* .nse_depinfo file (if written) */
	active = getenv(NSE_VARIANT_ENV);
	if (keep_state &&
	    (NULL != active) &&
	    !IS_EQUAL(active, NSE_RT_SOURCE_NAME) &&
	    !do_not_exec_rule &&
	    !report_dependencies_only) {
		(void) sprintf(push_cmd,
			       "%s %s/%s",
			       NSE_TFS_PUSH,
			       get_current_path(),
			       NSE_DEPINFO);
		(void) system(push_cmd);
	}
}

/*
 *	handle_interrupt()
 *
 *	This is where C-C traps are caught.
 *
 *	Parameters:
 *
 *	Global variables used:
 *		current_target		Sometimes the current target is removed
 *		do_not_exec_rule	But not if -n is on
 *		quest			or -q
 *		running_list		List of parallel running processes
 *		touch			Current target is not removed if -t on
 */
void
handle_interrupt()
{
	Property		member;
	Running			rp;

	(void) fflush(stdout);
#ifdef PARALLEL
	/* Clean up all parallel children already finished */
        finish_children(false);
#endif
	/* Make sure the processes running under us terminate first */
	while (wait((union wait *) NULL) != -1);
	/* Delete the current targets unless they are precious */
	if ((current_target != NULL) &&
	    current_target->is_member &&
	    ((member = get_prop(current_target->prop, member_prop)) != NULL)) {
		current_target = member->body.member.library;
	}
	if (!do_not_exec_rule &&
	    !touch &&
	    !quest &&
	    (current_target != NULL) &&
	    !current_target->stat.is_precious) {
		if (exists(current_target) != (int) file_doesnt_exist) {
			(void) fprintf(stderr,
				       "\n*** %s ",
				       current_target->string);
			if (current_target->stat.is_dir) {
				(void) fprintf(stderr,
					       "not removed.\n",
					       current_target->string);
			} else if (unlink(current_target->string) == 0) {
				(void) fprintf(stderr,
					       "removed.\n",
					       current_target->string);
			} else {
				(void) fprintf(stderr,
					       "could not be removed: %s.\n",
					       current_target->string,
					       errmsg(errno));
			}
		}
	}
	for (rp = running_list; rp != NULL; rp = rp->next) {
		if (rp->state != build_running) {
			continue;
		}
		if (rp->target->is_member &&
		    ((member = get_prop(rp->target->prop, member_prop)) !=
		     NULL)) {
			rp->target = member->body.member.library;
		}
		if (!do_not_exec_rule &&
		    !touch &&
		    !quest &&
		    !rp->target->stat.is_precious) {
			if (exists(rp->target) != (int) file_doesnt_exist) {
				(void) fprintf(stderr,
					       "\n*** %s ",
					       rp->target->string);
				if (rp->target->stat.is_dir) {
					(void) fprintf(stderr,
						       "not removed.\n",
						       rp->target->string);
				} else if (unlink(rp->target->string) == 0) {
					(void) fprintf(stderr,
						       "removed.\n",
						       rp->target->string);
				} else {
					(void) fprintf(stderr,
						       "could not be removed: %s.\n",
						       rp->target->string,
						       errmsg(errno));
				}
			}
		}
	}
	exit(2);
}

/*
 *	doalarm(sig, code, scp)
 *
 *	Handle the alarm interrupt but do nothing.  Side effect is to
 *	cause return from wait3.
 *
 *	Parameters:
 *		sig
 *		code
 *		scp
 *
 *	Global variables used:
 */
/*ARGSUSED*/
static void
doalarm(sig, code, scp)
	int			sig;
	int			code;
	struct sigcontext	*scp;
{
	return;
}


/*
 *	read_command_options(argc, argv)
 *
 *	Scan the command line options and process the ones that start with "-"
 *
 *	Return value:
 *				-M argument, if any
 *
 *	Parameters:
 *		argc		You know what this is
 *		argv		You know what this is
 *
 *	Global variables used:
 */
static Name
read_command_options(argc, argv)
	register int		argc;
	register char		**argv;
{
	register int		i;
	register int		j;
	register char		ch;
	register int		makefile_next = 0; /* flag to note -f option */
	Name			make_machines = NULL;

	for (i = 1; i < argc; i++) {
		switch (makefile_next) {
		case 1:
			makefile_next = 0;
			continue;
		case 2:
			make_state = GETNAME(argv[i], FIND_LENGTH);
			makefile_next = 0;
			continue;
		case 4:
			make_machines = GETNAME(argv[i], FIND_LENGTH);
			makefile_next = 0;
			continue;
		}
		if ((argv[i] != NULL) && (argv[i][0] == (int) hyphen_char)) {
			for (j = 1; (ch = argv[i][j]) != (int) nul_char; j++) {
				makefile_next |=
				  parse_command_option(ch);
			}
			switch (makefile_next) {
			case 0:
				argv[i] = NULL;
				break;
			case 1:	 /* -f seen */
				argv[i] = "-f";
				break;
			case 2:	/* -F seen */
				argv[i] = "-F";
				break;
			case 4:	/* -M seen */
				argv[i] = "-M";
				break;
			default:	 /* more than one of -f, -F, -M seen */
				fatal("Illegal command line. More than one option requiring\nfile argument given in the same argument group");
			}
		}
	}
	return make_machines;
}

/*
 *	parse_command_option(ch)
 *
 *	Parse make command line options.
 *
 *	Return value:
 *				Indicates if any -f -F or -M were seen
 *
 *	Parameters:
 *		ch		The character to parse
 *
 *	Static variables used:
 *		env_wins		Set for make -e
 *		ignore_default_mk	Set for make -r
 *		trace_status		Set for make -p
 *
 *	Global variables used:
 *		continue_after_error	Set for make -k
 *		debug_level		Set for make -d
 *		do_not_exec_rule	Set for make -n
 *		filter_stderr		Set for make -X
 *		ignore_errors		Set for make -i
 *		no_parallel		Set for make -R
 *		quest			Set for make -q
 *		read_trace_level	Set for make -D
 *		report_dependencies	Set for make -P
 *		silent			Set for make -s
 *		touch			Set for make -t
 */
static int
parse_command_option(ch)
	register char		ch;
{
	static int		invert_next = 0;
	int			invert_this = invert_next;

	invert_next = 0;
	switch (ch) {
	case '~':			 /* Invert next option */
		invert_next = 1;
		return 0;
	case 'B':			 /* Obsolete */
		return 0;
	case 'b':			 /* Obsolete */
		return 0;
	case 'D':			 /* Show lines read */
		if (invert_this) {
			read_trace_level--;
		} else {
			read_trace_level++;
		}
		return 0;
	case 'd':			 /* debug flag */
		if (invert_this) {
			debug_level--;
		} else {
			debug_level++;
		}
		return 0;
	case 'e':			 /* environment override flag */
		if (invert_this) {
			env_wins = false;
		} else {
			env_wins = true;
		}
		return 0;
	case 'F':			 /* Read alternative .make.state */
		return 2;
	case 'f':			 /* Read alternative makefile(s) */
		return 1;
	case 'g':			 /* sccs get files not found */
		return 0;
	case 'i':			 /* ignore errors */
		if (invert_this) {
			ignore_errors = false;
		} else {
			ignore_errors = true;
		}
		return 0;
	case 'k':			 /* Keep making even after errors */
		if (invert_this) {
			continue_after_error = false;
		} else {
			continue_after_error = true;
		}
		return 0;
	case 'm':			 /* Obsolete */
		return 0;
#ifdef PARALLEL
	case 'M':			 /* Read alt make.machines file */
		return 4;
#endif PARALLEL
	case 'n':			 /* print, not exec commands */
		if (invert_this) {
			do_not_exec_rule = false;
		} else {
			do_not_exec_rule = true;
		}
		return 0;
	case 'N':			 /* Reverse -n */
		if (invert_this) {
			do_not_exec_rule = true;
		} else {
			do_not_exec_rule = false;
		}
		return 0;
	case 'P':			 /* print for selected targets */
		if (invert_this) {
			report_dependencies_only = false;
		} else {
			report_dependencies_only = true;
		}
		return 0;
	case 'p':			 /* print description */
		if (invert_this) {
			trace_status = false;
		} else {
			trace_status = true;
		}
		return 0;
	case 'q':			 /* question flag */
		if (invert_this) {
			quest = false;
		} else {
			quest = true;
		}
		return 0;
	case 'r':			 /* turn off internal rules */
		if (invert_this) {
			ignore_default_mk = false;
		} else {
			ignore_default_mk = true;
		}
		return 0;
#ifdef PARALLEL
	case 'R':			/* don't run in parallel */
		if (invert_this) {
			no_parallel = false;
		} else {
			no_parallel = true;
		}
		return 0;
#endif
	case 'S':			 /* Reverse -k */
		if (invert_this) {
			continue_after_error = true;
		} else {
			continue_after_error = false;
		}
		return 0;
	case 's':			 /* silent flag */
		if (invert_this) {
			silent = false;
		} else {
			silent = true;
		}
		return 0;
	case 't':			 /* touch flag */
		if (invert_this) {
			touch = false;
		} else {
			touch = true;
		}
		return 0;
	case 'X':			/* Filter stdout */
		if (invert_this) {
			filter_stderr = true;
		} else {
			filter_stderr = true;
		}
		return 0;
	default:
		fatal("Unknown option `-%c'", ch);
	}
	return 0;
}

/*
 *	set_target_host_arch()
 *
 *	Set the magic macros TARGET_ARCH, HOST_ARCH,
 *	TARGET_MACH & HOST_MACH
 *
 *	Parameters:
 *
 *	Global variables used:
 */
static void
set_target_host_arch()
{
	String_rec	result_string;
	char		buffer[STRING_BUFFER_LENGTH];
	FILE		*pipe;
	Name		name;
	register int	ch;

	INIT_STRING_FROM_STACK(result_string, buffer);
	append_char((int) hyphen_char, &result_string);
	if ((pipe = popen("/bin/arch", "r")) == NULL) {
		fatal("Execute of /bin/arch failed");
	}
	while ((ch = getc(pipe)) != EOF) {
		append_char(ch, &result_string);
	}
	if (pclose(pipe) != NULL) {
		fatal("Execute of /bin/arch failed");
	}

	name = GETNAME(result_string.buffer.start,
		       strlen(result_string.buffer.start)-1);
	(void) SETVAR(host_arch, name, false);
	(void) SETVAR(target_arch, name, false);
	INIT_STRING_FROM_STACK(result_string, buffer);
	append_char((int) hyphen_char, &result_string);
	if ((pipe = popen("/bin/mach", "r")) == NULL) {
		fatal("Execute of /bin/mach failed");
	}
	while ((ch = getc(pipe)) != EOF) {
		append_char(ch, &result_string);
	}
	if (pclose(pipe) != NULL) {
		fatal("Execute of /bin/mach failed");
	}

	name = GETNAME(result_string.buffer.start,
		       strlen(result_string.buffer.start)-1);
	(void) SETVAR(GETNAME("HOST_MACH", FIND_LENGTH), name, false);
	(void) SETVAR(GETNAME("TARGET_MACH", FIND_LENGTH), name, false);
}

/*
 *	setup_for_projectdir()
 *
 *	Read the PROJECTDIR variable, if defined, and set the sccs path
 *
 *	Parameters:
 *
 *	Global variables used:
 *		sccs_dir_path	Set to point to SCCS dir to use
 */
static void
setup_for_projectdir()
{
	char			path[MAXPATHLEN];

	/* Check if we should use PROJECTDIR when reading the SCCS dir */
	sccs_dir_path = getenv("PROJECTDIR");
	if ((sccs_dir_path != NULL) &&
	    (sccs_dir_path[0] != (int) slash_char)) {
		struct passwd *pwent;

		if ((pwent = getpwnam(sccs_dir_path)) == NULL) {
			fatal("Bogus PROJECTDIR '%s'", sccs_dir_path);
		}
		(void) sprintf(path, "%s/src", pwent->pw_dir);
		if (access(path, F_OK) == 0) {
			sccs_dir_path = path;
		} else {
			(void) sprintf(path, "%s/source", pwent->pw_dir);
			if (access(path, F_OK) == 0) {
				sccs_dir_path = path;
			} else {
				sccs_dir_path = NULL;
			}
		}
	}
}

/*
 *	read_files_and_state(argc, argv)
 *
 *	Read the makefiles we care about and the environment
 *	Also read the = style command line options
 *
 *	Parameters:
 *		argc		You know what this is
 *		argv		You know what this is
 *
 *	Static variables used:
 *		env_wins	make -e, determines if env vars are RO
 *		ignore_default_mk make -r, determines if default.mk is read
 *		not_auto_depen	dwight
 *
 *	Global variables used:
 *		default_target_to_build	Set to first proper target from file
 *		do_not_exec_rule Set to false when makfile is made
 *		dot		The Name ".", used to read current dir
 *		empty_name	The Name "", use as macro value
 *		keep_state	Set if KEEP_STATE is in environment
 *		make_state	The Name ".make.state", used to read file
 *		makefile_type	Set to type of file being read
 *		makeflags	The Name "MAKEFLAGS", used to set macro value
 *		not_auto	dwight
 *		nse		Set if NSE_ENV is in the environment
 *		read_trace_level Checked to se if the reader should trace
 *		report_dependencies If -P is on we do not read .make.state
 *		trace_reader	Set if reader should trace
 *		virtual_root	The Name "VIRTUAL_ROOT", used to check value
 */
static void
read_files_and_state(argc, argv)
	int			argc;
	char			**argv;
{
	register int		i;
	register char		*cp;
	register Name		name;
	register Boolean	makefile_read = false;
	Boolean			save_do_not_exec_rule;
	register Property	macro;
	Name			keep_state_name;
	Name			Makefile;
	Name			makefile_name;
	Boolean			temp;
	String_rec		makeflags_string;
	char			buffer[100];

	keep_state_name = GETNAME("KEEP_STATE", FIND_LENGTH);
	Makefile = GETNAME("Makefile", FIND_LENGTH);
	makefile_name = GETNAME("makefile", FIND_LENGTH);

/*
 *	Set flag if NSE is active
 */
	if (getenv("NSE_ENV") != NULL) {
		nse = true;
	}

/*
 *	initialize global dependency entry for .NOT_AUTO
 */
	not_auto_depen->next = NULL;
	not_auto_depen->name = not_auto;
	not_auto_depen->automatic = not_auto_depen->stale = false;

/*
 *	Read internal definitions and rules.
 */
	if (read_trace_level > 1) {
		trace_reader = true;
	}
	if (!ignore_default_mk) {
		Name		default_makefile;

		default_makefile = GETNAME("default.mk", FIND_LENGTH);
		default_makefile->stat.is_file = true;

		(void) read_makefile(default_makefile,
				     true,
				     false,
				     false);
	}
	trace_reader = false;

/*
 *	Read environment args.  Let file args which follow override unless
 *	 -e option seen. If -e option is not mentioned.
 */
	read_environment(env_wins);
	if (getvar(virtual_root)->hash.length == 0) {
		maybe_append_prop(virtual_root, macro_prop)
		  ->body.macro.exported = true;
		(void) SETVAR(virtual_root,
			     GETNAME("/", FIND_LENGTH),
			      false);
	}

/*
 *	Read state file
 */
	if (!report_dependencies_only) {
		makefile_type = reading_statefile;
		if (read_trace_level > 1) {
			trace_reader = true;
		}
		(void) read_simple_file(make_state,
					false,
					false,
					false,
					false,
					false,
					true);
		trace_reader = false;
	}
	default_target_to_build = NULL;

/*
 *	Set MFLAGS and MAKEFLAGS
 */
	INIT_STRING_FROM_STACK(makeflags_string, buffer);
	append_char((int) hyphen_char, &makeflags_string);

	switch (read_trace_level) {
	case 2:
		append_char('D', &makeflags_string);
	case 1:
		append_char('D', &makeflags_string);
	}
	switch (debug_level) {
	case 2:
		append_char('d', &makeflags_string);
	case 1:
		append_char('d', &makeflags_string);
	}
	if (env_wins) {
		append_char('e', &makeflags_string);
	}
	if (ignore_errors) {
		append_char('i', &makeflags_string);
	}
	if (continue_after_error) {
		append_char('k', &makeflags_string);
	}
	if (do_not_exec_rule) {
		append_char('n', &makeflags_string);
	}
	if (report_dependencies_only) {
		append_char('P', &makeflags_string);
	}
	if (trace_status) {
		append_char('p', &makeflags_string);
	}
	if (quest) {
		append_char('q', &makeflags_string);
	}
#ifdef PARALLEL
	if (no_parallel) {
		append_char('P', &makeflags_string);
	}
#endif
	if (silent) {
		append_char('s', &makeflags_string);
	}
	if (touch) {
		append_char('t', &makeflags_string);
	}
	if (filter_stderr) {
		append_char('X', &makeflags_string);
	}

/*
 *	Make sure MAKEFLAGS is exported
 */
	maybe_append_prop(makeflags, macro_prop)->
	  body.macro.exported = true;
	if (makeflags_string.buffer.start[1] != (int) nul_char) {
		(void) SETVAR(GETNAME("MFLAGS", FIND_LENGTH),
			     GETNAME(makeflags_string.buffer.start,
				     FIND_LENGTH),
			      false);
	}
	macro = maybe_append_prop(makeflags, macro_prop);
	temp = macro->body.macro.read_only;
	macro->body.macro.read_only = false;
	(void) SETVAR(makeflags,
		     GETNAME(makeflags_string.buffer.start + 1, FIND_LENGTH),
		      false);
	macro->body.macro.read_only = temp;
	if (makeflags_string.free_after_use) {
		retmem(makeflags_string.buffer.start);
	}
	makeflags_string.buffer.start = NULL;

/*
 *	Read command line "=" type args and make them readonly.
 */
	makefile_type = reading_nothing;
	for (i = 1; i < argc; ++i) {
		if ((argv[i] != NULL) &&
		    (argv[i][0] != (int) hyphen_char) &&
		    ((cp = strchr(argv[i], (int) equal_char)) != NULL) &&
		    ((argv[i-1] == NULL) ||
		     (argv[i-1][0] != (int) hyphen_char))) {
			while (isspace(*(cp-1))) {
				cp--;
			}
			name = GETNAME(argv[i], cp-argv[i]);
			maybe_append_prop(name, macro_prop)->
			  body.macro.exported = true;
			while (*cp != (int) equal_char) {
				cp++;
			}
			cp++;
			while (isspace(*cp) && (*cp != (int) nul_char)) {
				cp++;
			}
			if (*cp == (int) nul_char) {
				SETVAR(name, (Name) NULL, false)->
				  body.macro.read_only = true;
			} else {
				SETVAR(name, GETNAME(cp, FIND_LENGTH), false)->
				  body.macro.read_only = true;
			}
			argv[i] = NULL;
		}
	}

/*
 *	Read command line "-f" arguments and ignore "-F" and "-M" arguments.
 */
	save_do_not_exec_rule = do_not_exec_rule;
	do_not_exec_rule = false;
	if (read_trace_level > 0) {
		trace_reader = true;
	}
	for (i = 1; i < argc; i++) {
		if (argv[i] &&
		    (argv[i][0] == (int) hyphen_char) &&
		    (argv[i][1] == 'f') &&
		    (argv[i][2] == (int) nul_char)) {
			argv[i] = NULL;		/* Remove -f */
			if (i >= argc - 1) {
				fatal("No filename argument after -f flag");
			}
			(void) read_makefile(GETNAME(argv[++i], FIND_LENGTH),
					     true,
					     true,
					     true);
			argv[i] = NULL;		/* Remove filename */
			makefile_read = true;
		} else if (argv[i] &&
			   (argv[i][0] == (int) hyphen_char) &&
			   (argv[i][1] == 'F') &&
			   (argv[i][2] == (int) nul_char)) {
			argv[i++] = NULL;
			argv[i] = NULL;
		} else if (argv[i] &&
			   (argv[i][0] == (int) hyphen_char) &&
			   (argv[i][1] == 'M') &&
			   (argv[i][2] == (int) nul_char)) {
			argv[i++] = NULL;
			argv[i] = NULL;
		}
	}

/*
 *	If no command line "-f" args then look for "makefile", and then for
 *	"Makefile" if "makefile" isn't found
 */
	if (!makefile_read) {
		(void) read_dir(dot,
				(char *) NULL,
				(Property) NULL,
				(char *) NULL);
		if (makefile_name->stat.is_file) {
			if (Makefile->stat.is_file) {
				warning("Both `makefile' and `Makefile' exists");
			}
			makefile_read = read_makefile(makefile_name,
						      false,
						      false,
						      true);
		}
		if (!makefile_read &&
		    Makefile->stat.is_file) {
			makefile_read = read_makefile(Makefile,
						      false,
						      false,
						      true);
		}
	}
	trace_reader = false;
	do_not_exec_rule = save_do_not_exec_rule;

/*
 *	Make sure KEEP_STATE is in the environment if KEEP_STATE is on
 */
	macro = get_prop(keep_state_name->prop, macro_prop);
	if ((macro != NULL) &&
	    macro->body.macro.exported) {
		keep_state = true;
	}
	if (keep_state) {
		if (macro == NULL) {
			macro = maybe_append_prop(keep_state_name,
						  macro_prop);
		}
		macro->body.macro.exported = true;
		(void) SETVAR(keep_state_name,
			      empty_name,
			      false);
	}
}

/*
 *	read_environment(read_only)
 *
 *	This routine reads the process environment when make starts and enters
 *	it as make macros. The environment variable SHELL is ignored.
 *
 *	Parameters:
 *		read_only	Should we make env vars read only?
 *
 *	Global variables used:
 *		report_pwd	Set if this make was started by other make
 */
static void
read_environment(read_only)
	Boolean			read_only;
{
	register char		**environment;
	register char		*value;
	register char		*name;
	register Name		macro;
	Property		val;

	reading_environment = true;
	environment = environ;
	for (; *environment; environment++) {
		name = *environment;
		value = strchr(name, (int) equal_char);
		if (IS_EQUALN(name, "SHELL=", 6)) {
			continue;
		}
		if (IS_EQUALN(name, "MAKEFLAGS=", 6)) {
			report_pwd = true;
		}
		macro = GETNAME(name, value - name);
		maybe_append_prop(macro, macro_prop)->body.macro.exported =
		  true;
		if ((value == NULL) || ((value + 1)[0] == (int) nul_char)) {
			val = SETVAR(macro, (Name) NULL, false);
		} else {
			val = SETVAR(macro,
				     GETNAME(value + 1, FIND_LENGTH),
				     false);
		}
		val->body.macro.read_only = read_only;
	}
	reading_environment = false;
}

/*
 *	read_makefile(makefile, complain, must_exist, report_file)
 *
 *	Read one makefile and check the result
 *
 *	Return value:
 *				false is the read failed
 *
 *	Parameters:
 *		makefile	The file to read
 *		complain	Passed thru to read_simple_file()
 *		must_exist	Passed thru to read_simple_file()
 *		report_file	Passed thru to read_simple_file()
 *
 *	Global variables used:
 *		makefile_type	Set to indicate we are reading main file
 *		recursion_level	Initialized
 */
static Boolean
read_makefile(makefile, complain, must_exist, report_file)
	register Name		makefile;
	Boolean			complain;
	Boolean			must_exist;
	Boolean			report_file;
{
	makefile_type = reading_makefile;
	recursion_level = 0;
	return read_simple_file(makefile,
				true,
				true,
				complain,
				must_exist,
				report_file,
				false);
}

/*
 *	make_targets(argc, argv, parallel_flag)
 *
 *	Call doname on the specified targets
 *
 *	Parameters:
 *		argc		You know what this is
 *		argv		You know what this is
 *		parallel_flag	True if building in parallel
 *
 *	Global variables used:
 *		build_failed_seen Used to generated message after failed -k
 *		commands_done	Used to generate message "Up to date"
 *		default_target_to_build	First proper target in makefile
 *		init		The Name ".INIT", use to run command
 *		parallel	Global parallel building flag
 *		quest		make -q, suppresses messages
 *		recursion_level	Initialized, used for tracing
 *		report_dependencies make -P, regroves whole process
 */
static void
make_targets(argc, argv, parallel_flag)
	int			argc;
	char			**argv;
	Boolean			parallel_flag;
{
	int			i;
	char			*cp;
	Doname			result;
	register Boolean	target_to_make_found = false;

	(void) doname(init, true, true);
	recursion_level = 1;
	parallel = parallel_flag;
/*
 *	make remaining args
 */
#ifdef PARALLEL
	if (!report_dependencies_only && parallel) {
		/*
		 * If building targets in parallel, start all of the
		 * remaining args to build in parallel.
		 */
		for (i = 1; i < argc; i++) {
			if ((cp = argv[i]) != NULL) {
				commands_done = false;
				if ((cp[0] == (int) period_char) &&
				    (cp[1] == (int) slash_char)) {
					cp += 2;
				}
				default_target_to_build = GETNAME(cp,
								  FIND_LENGTH);
				result = doname_check(default_target_to_build,
						      true,
						      false,
						      false);
				if (!commands_done &&
				    (result == build_ok) &&
				    !quest &&
				    (exists(default_target_to_build) >
				     (int) file_doesnt_exist)) {
					(void) printf("`%s' is up to date.\n",
						      default_target_to_build->
						      string);
				}
			}
		}
		/* Now wait for all of the targets to finish running */
		finish_running();
	}
#endif PARALLEL
	for (i = 1; i < argc; i++) {
		if ((cp = argv[i]) != NULL) {
			target_to_make_found = true;
			if ((cp[0] == (int) period_char) &&
			    (cp[1] == (int) slash_char)) {
				cp += 2;
			}
			default_target_to_build = GETNAME(cp, FIND_LENGTH);
			commands_done = false;
			if (parallel) {
				result = default_target_to_build->state;
			} else {
				result = doname_check(default_target_to_build,
						      true,
						      false,
						      false);
			}
			if (result != build_failed) {
				report_recursion(default_target_to_build);
			}
			if (build_failed_seen) {
				warning("Target `%s' not remade because of errors",
					default_target_to_build->string);
			}
			build_failed_seen = false;
			if (report_dependencies_only) {
				print_dependencies(default_target_to_build,
						   get_prop(default_target_to_build->prop,
							    line_prop));
			}
			default_target_to_build->stat.time =
			  (int) file_no_time;
			if (default_target_to_build->colon_splits > 0) {
				default_target_to_build->state =
				  build_dont_know;
			}
			if (!parallel &&
			    !commands_done &&
			    (result == build_ok) &&
			    !quest &&
			    !report_dependencies_only &&
			    (exists(default_target_to_build) >
			     (int) file_doesnt_exist)) {
				(void) printf("`%s' is up to date.\n",
					      default_target_to_build->string);
			}
		}
	}

/*
 *	If no file arguments have been encountered,
 *	make the first name encountered that doesnt start with a dot
 */
	if (!target_to_make_found) {
		if (default_target_to_build == NULL) {
			fatal("No arguments to build");
		}
		commands_done = false;
#ifdef PARALLEL
		result = doname_parallel(default_target_to_build, true, false);
#else
		result = doname_check(default_target_to_build, true,
				      false, false);
#endif PARALLEL
		report_recursion(default_target_to_build);
		if (build_failed_seen) {
			warning("Target `%s' not remade because of errors",
				default_target_to_build->string);
		}
		build_failed_seen = false;
		if (report_dependencies_only) {
			print_dependencies(default_target_to_build,
					   get_prop(default_target_to_build->
						    prop,
						    line_prop));
		}
		default_target_to_build->stat.time = (int) file_no_time;
		if (default_target_to_build->colon_splits > 0) {
			default_target_to_build->state = build_dont_know;
		}
		if (!commands_done &&
		    (result == build_ok) &&
		    !quest &&
		    !report_dependencies_only &&
		    (exists(default_target_to_build) >
		     (int) file_doesnt_exist)) {
			(void) printf("`%s' is up to date.\n",
				      default_target_to_build->string);
		}
	}
}

/*
 *	print_dependencies(target, line)
 *
 *	Print all the dependencies of a target. First print all the Makefiles.
 *	Then print all the dependencies. Finally, print all the .INIT
 *	dependencies.
 *
 *	Parameters:
 *		target		The target we print dependencies for
 *		line		We get the dependency list from here
 *
 *	Global variables used:
 *		done		The Name ".DONE"
 *		init		The Name ".INIT"
 *		makefiles_used	List of all makefiles read
 */
static void
print_dependencies(target, line)
	register Name           target;
	register Property	line;
{
	register Dependency	dependencies;

        for (dependencies = makefiles_used; dependencies != NULL;
    	     dependencies = dependencies->next) {
    		(void) printf("%s ", dependencies->name->string);
	}
	(void)  printf("\n");
	print_deps(target, line, true);
	print_more_deps(target, init);
	print_more_deps(target, done);
}

/*
 *	print_more_deps(target, name)
 *
 *	Print some special dependencies.
 *	These are the dependencies for the .INIT and .DONE targets.
 *
 *	Parameters:
 *		target		Target built during make run
 *		name		Special target to print dependencies for
 *
 *	Global variables used:
 */
static void
print_more_deps(target, name)
	Name		target;
	Name		name;
{
	Property	line;
	register Dependency	dependencies;

	line = get_prop(name->prop, line_prop);
	if (line != NULL && line->body.line.dependencies != NULL) {
		(void) printf("%s:\t", target->string);
		for (dependencies = line->body.line.dependencies;
		     dependencies != NULL;
		     dependencies = dependencies->next) {
			(void) printf("%s ", dependencies->name->string);
		}
		(void) printf("\n");
		for (dependencies = line->body.line.dependencies;
		     dependencies != NULL;
		     dependencies = dependencies->next) {
	                 print_deps(dependencies->name,
				    get_prop(dependencies->name->prop,
					     line_prop),
				    true);
		}
	}
}

/*
 *	print_deps(target, line, go_recursive)
 *
 *	Print a regular dependency list.
 *
 *	Parameters:
 *		target		target to print dependencies for
 *		line		We get the dependency list from here
 *		go_recursive	Should we show all dependencies recursively?
 *
 *	Global variables used:
 *		recursive_name	The Name ".RECURSIVE", printed
 */
static void
print_deps(target, line, go_recursive)
	register Name		target;
	register Property	line;
	register Boolean	go_recursive;
{
	register Dependency	dependencies;
	register Property	recursive;

	if (target->dependency_printed) {
		return;
	}
	target->dependency_printed = true;
	if (target->has_recursive_dependency) {
		for (recursive = get_prop(target->prop, recursive_prop);
		     recursive != NULL;
		     recursive = get_prop(recursive->next, recursive_prop)) {
			(void) printf("%s:\t%s %s %s",
				      target->string,
				      recursive_name->string,
				      recursive->body.
				      recursive.directory->string,
				      recursive->body.
				      recursive.target->string);
			for (dependencies =
			     recursive->body.recursive.makefiles;
			     dependencies != NULL;
			     dependencies = dependencies->next) {
				(void) printf(" %s",
					      dependencies->name->string);
			}
			(void) printf("\n");
		}
	}
	/* only print entries that are actually derived and are not leaf
	 * files and are not the result of sccs get.
	 */
	if (should_print_dep(line)) {
		(void) printf("%s:\t", target->string);
		for (dependencies = line->body.line.dependencies;
		     dependencies != NULL;
		     dependencies = dependencies->next) {
			(void) printf("%s ", dependencies->name->string);
		}
		(void) printf("\n");
		if (go_recursive) {
			for (dependencies = line->body.line.dependencies;
			     dependencies != NULL;
			     dependencies = dependencies->next) {
				print_deps(dependencies->name,
					   get_prop(dependencies->name->prop,
						    line_prop),
					   go_recursive);
			}
		}
	}
}

/*
 *	should_print_dep(line)
 *
 *	Test if we should print the dependencies of this target.
 *	The line must exist and either have children dependencies
 *	or have a command that is not an SCCS command.
 *
 *	Return value:
 *				true if the dependencies should be printed
 *
 *	Parameters:
 *		line		We get the dependency list from here
 *
 *	Global variables used:
 */
static Boolean
should_print_dep(line)
	Property	line;
{
	if (line == NULL) {
		return false;
	}
	if (line->body.line.dependencies != NULL) {
		return true;
	}
	if (!line->body.line.sccs_command &&
	    (line->body.line.command_template != NULL)) {
		return true;
	}
	return false;
}

/*
 *	report_recursion(target)
 *
 *	If this is a recursive make and the parent make has KEEP_STATE on
 *	this routine reports the dependency to the parent make
 *
 *	Parameters:
 *		target		Target to report
 *
 *	Global variables used:
 *		makefiles_used		List of makefiles read
 *		recursive_name		The Name ".RECURSIVE", printed
 *		report_dependency	dwight
 */
static void
report_recursion(target)
	register Name		target;
{
	register Dependency	dp;
	register FILE		*report_file = get_report_file();

	if ((report_file == NULL) || (report_file == (FILE*)-1)) {
		return;
	}
	(void) fprintf(report_file,
		      "%s: %s ",
		      get_target_being_reported_for(),
		      recursive_name->string);
	report_dependency(get_current_path());
	report_dependency(target->string);
	for (dp = makefiles_used; dp != NULL; dp = dp->next) {
		report_dependency(dp->name->string);
	}
	(void) fprintf(report_file, "\n");
}

