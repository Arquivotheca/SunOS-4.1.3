#ident "@(#)dosys.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	dosys.c
 *
 *	Execute one commandline
 */

/*
 * Included files
 */
#include "defs.h"
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Static variables
 */
static	int		filter_file;
static	char		*filter_file_name;

/*
 * File table of contents
 */
extern	Doname		dosys();
extern	int		doshell();
extern	Boolean		exec_vp();
extern	int		doexec();
extern	void		redirect_stderr();
extern	Boolean		await();
extern	void		sh_command2string();

/*
 *	dosys(command, ignore_error, call_make, silent_error, target)
 *
 *	Check if arg string contains meta chars and dispatch to
 *	the proper routine for executing one command line
 *
 *	Return value:
 *				Indicates if the command execution failed
 *
 *	Parameters:
 *		command		The command to run
 *		ignore_error	Should make abort when an error is seen?
 *		call_make	Did command reference $(MAKE) ?
 *		silent_error	Should error messages be suppressed for pmake?
 *		target		Target we are building
 *
 *	Global variables used:
 *		do_not_exec_rule Is -n on?
 *		working_on_targets We started processing real targets
 */
Doname
dosys(command, ignore_error, call_make, silent_error, target)
	register Name		command;
	register Boolean	ignore_error;
	register Boolean	call_make;
	Boolean			silent_error;
	Name			target;
{
	register char		*p = command->string;
	register char		*q;
	register int		length = command->hash.length;
	Doname			result;
	struct stat		before;

	/* Strip spaces from head of command string */
	while (isspace(*p)) {
		p++, length--;
	}
	if (*p == (int) nul_char) {
		return build_failed;
	}
	/* If we are faking it we just return */
	if (do_not_exec_rule &&
	    working_on_targets &&
	    !call_make) {
		return build_ok;
	}

	/* Copy string to make it OK to write it */
	q = alloca(length + 1);
	(void) strcpy(q, p);
	/* Write the state file iff this command uses make */
	if (call_make && command_changed) {
		write_state_file(0);
	}
	(void)stat(make_state->string, &before);
	/* Run command directly if it contains no shell meta chars else */
	/* run it using the shell */
	if (await(ignore_error,
		  silent_error,
		  target,
		  command->string,
		  command->meta ? doshell(q, ignore_error) :
				   doexec(q, ignore_error))) {
		result = build_ok;
	} else {
		result = build_failed;
	}

	if (!report_dependencies_only &&
	    call_make) {
		make_state->stat.time = (time_t)file_no_time;
		(void)exists(make_state);
		if (before.st_mtime == make_state->stat.time) {
			return result;
		}
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
	return result;
}

/*
 *	doshell(command, ignore_error)
 *
 *	Used to run command lines that include shell meta-characters.
 *	The make macro SHELL is supposed to contain a path to the shell.
 *
 *	Return value:
 *				The pid of the process we started
 *
 *	Parameters:
 *		command		The command to run
 *		ignore_error	Should we abort on error?
 *
 *	Global variables used:
 *		filter_stderr	If -X is on we redirect stderr
 *		shell_name	The Name "SHELL", used to get the path to shell
 */
static int
doshell(command, ignore_error)
	char			*command;
	register Boolean	ignore_error;
{
	register char		*shellname;
	char			*argv[4];
	register Name		shell = getvar(shell_name);
	register int		running_pid;

	if ((shellname = strrchr(shell->string, (int) slash_char)) == NULL) {
		shellname = shell->string;
	} else {
		shellname++;
	}
	argv[0] = shellname;
	argv[1] = ignore_error ? "-c" : "-ce";
	argv[2] = command;
	argv[3] = NULL;
	(void) fflush(stdout);
	if ((running_pid = vfork()) == 0) {
		enable_interrupt((void (*) ()) SIG_DFL);
		if (filter_stderr) {
			redirect_stderr();
		}
		shell->string[shell->hash.length] = (int) nul_char;
		(void) execve(shell->string, argv, environ);
		fatal("Could not load Shell from `%s': %s",
		      shell->string,
		      errmsg(errno));
	}
	return running_pid;
}

/*
 *	exec_vp(name, argv, envp, ignore_error)
 *
 *	Like execve, but does path search.
 *	This starts command when make invokes it directly (without a shell).
 *
 *	Return value:
 *				Returns false if the exec failed
 *
 *	Parameters:
 *		name		The name of the command to run
 *		argv		Arguments for the command
 *		envp		The environment for it
 *		ignore_error	Should we abort on error?
 *
 *	Global variables used:
 *		shell_name	The Name "SHELL", used to get the path to shell
 *		vroot_path	The path used by the vroot package
 */
static Boolean
exec_vp(name, argv, envp, ignore_error)
	register char		*name;
	register char		**argv;
	char			*envp[NCARGS];
	register Boolean	ignore_error;
{
	register int		etxtbsy = 1;
	register char		*shellname;
	char			*shargv[4];
	register Name		shell = getvar(shell_name);

	while (1) {
		(void) execve_vroot(name,
				    argv + 1,
				    envp,
				    vroot_path,
				    VROOT_DEFAULT);
		switch (errno) {
		case ENOEXEC:
		case ENOENT:
			/* That failed. Let the shell handle it */
			shellname = strrchr(shell->string, (int) slash_char);
			if (shellname == NULL) {
				shellname = shell->string;
			} else {
				shellname++;
			}
			shargv[0] = shellname;
			shargv[1] = ignore_error ? "-c" : "-ce";
			shargv[2] = argv[0];
			shargv[3] = NULL;
			(void) execve_vroot(getvar(shell_name)->string,
					    shargv,
					    envp,
					    vroot_path,
					    VROOT_DEFAULT);
			return failed;
		case ETXTBSY:
			/* The program is busy (debugged?). */
			/* Wait and then try again */
			if (++etxtbsy > 5) {
				return failed;
			}
			(void) sleep((unsigned) etxtbsy);
			break;
		case EACCES:
		case ENOMEM:
		case E2BIG:
			return failed;
		}
	}
	/* NOTREACHED */
}

/*
 *	doexec(command, ignore_error)
 *
 *	Will scan an argument string and split it into words
 *	thus building an argument list that can be passed to exec_ve()
 *
 *	Return value:
 *				The pid of the process started here
 *
 *	Parameters:
 *		command		The command to run
 *		ignore_error	Should we abort on error?
 *
 *	Global variables used:
 *		filter_stderr	If -X is on we redirect stderr
 */
static int
doexec(command, ignore_error)
	register char		*command;
	register Boolean	ignore_error;
{
	register char		*t;
	register char		**p;
	char			**argv;
	register int		running_pid;
	int			arg_count = 10;

	for (t = command; *t != 0; t++) {
		if (isspace(*t)) {
			arg_count++;
		}
	}
	argv = (char **) alloca(arg_count * sizeof (char *) * 2);

	p = &argv[1];			 /* reserve argv[0] for sh in case of
					  * exec_vp failure */
	argv[0] = alloca(strlen(command)+2);
	(void) strcpy(argv[0], command);

	/* Build list of argument words */
	for (t = command; *t;) {
		if (p >= &argv[arg_count]) {
			fatal("Command `%s' has more than %d arguments",
			      command,
			      arg_count);
		}
		*p++ = t;
		while (!isspace(*t) && (*t != (int) nul_char)) {
			t++;
		}
		if (*t) {
			for (*t++ = (int) nul_char; isspace(*t); t++);
		}
	}
	*p = NULL;

	/* Then exec the command with that argument list */
	(void) fflush(stdout);
	if ((running_pid = vfork()) == 0) {
		enable_interrupt((void (*) ()) SIG_DFL);
		if (filter_stderr) {
			redirect_stderr();
		}
		(void) exec_vp(command, argv, environ, ignore_error);
		fatal("Cannot load command `%s': %s", command, errmsg(errno));
	}
	return running_pid;
}

/*
 *	redirect_stderr()
 *
 *	Will setup to filter stderr messages for -X option.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		filter_file		The fd for the filter file
 *		filter_file_name	The name of the filter file
 */
static void
redirect_stderr()
{
	filter_file_name = getmem(32);
	(void) sprintf(filter_file_name, "/tmp/make.filter.%d", getpid());
	if ((filter_file = open(filter_file_name,
				O_WRONLY|O_CREAT|O_TRUNC,
				0777)) == -1) {
		fatal("Could not open filter file for -X");
	}

	(void) dup2(filter_file, fileno(stderr));
}

/*
 *	await(ignore_error, silent_error, target, command, running_pid)
 *
 *	Wait for one child process and analyzes
 *	the returned status when the child process terminates.
 *
 *	Return value:
 *				Returns true if commands ran OK
 *
 *	Parameters:
 *		ignore_error	Should we abort on error?
 *		silent_error	Should error messages be suppressed for pmake?
 *		target		The target we are building, for error msgs
 *		command		The command we ran, for error msgs
 *		running_pid	The pid of the process we are waiting for
 *		
 *	Static variables used:
 *		filter_file	The fd for the filter file
 *		filter_file_name The name of the filter file
 *
 *	Global variables used:
 *		filter_stderr	Set if -X is on
 */
static Boolean
await(ignore_error, silent_error, target, command, running_pid)
	register Boolean	ignore_error;
	register Boolean	silent_error;
	Name			target;
	char			*command;
	int			running_pid;
{
	union wait		status;
	register int		pid;
	struct stat		stat_buff;
	char			*buffer;
	FILE			*outfp;

	enable_interrupt(handle_interrupt);
	while ((pid = wait(&status)) != running_pid) {
		if (pid == -1) {
			fatal("wait() failed: %s", errmsg(errno));
		}
	}

	if (filter_stderr) {
		(void) close(filter_file);
		filter_file = open(filter_file_name, O_RDONLY, 0);
		if (filter_file == -1) {
			fatal("Could not open filter file for -X");
		}
		if (fstat(filter_file, &stat_buff) != 0) {
			fatal("Could not stat filter file for -X");
		}
		if (stat_buff.st_size > 0) {
			buffer = alloca((int) stat_buff.st_size);
			(void) read(filter_file,
				    buffer,
				    (int) stat_buff.st_size);
			if (status.w_T.w_Retcode != 0) {
				(void) fprintf(stderr,
					       "\n**** Error: Directory %s Target %s:\n%s\n",
					       get_current_path(),
					       target->string,
					       command);
				(void) fwrite(buffer,
					      1,
					      (int) stat_buff.st_size,
					      stdout);
				(void) fprintf(stdout,
					       "**** Error: Directory %s Target %s\n",
					       get_current_path(),
					       target->string);
			}
			(void) fwrite(buffer,
				      1,
				      (int) stat_buff.st_size,
				      stderr);
		}
		(void) close(filter_file);
		(void) unlink(filter_file_name);
		retmem(filter_file_name);
		filter_file_name = NULL;
		outfp = stdout;
	} else {
		outfp = stderr;
	}

	(void) fflush(stdout);
	(void) fflush(stderr);

	if (status.w_status == 0) {
		return succeeded;
	}

	/* If the child returned an error we now try to print a nice message */
	/* about that */
	if (!silent_error) {
		if (status.w_T.w_Retcode != 0) {
			(void) fprintf(outfp,
				       "*** Error code %d",
				       status.w_T.w_Retcode);
		} else {
			if (status.w_T.w_Termsig > NSIG) {
				(void) fprintf(outfp,
					       "*** Signal %d",
					       status.w_T.w_Termsig);
			} else {
				(void) fprintf(outfp,
					       "*** %s",
					       sys_siglist[status.
							   w_T.w_Termsig]);
			}
			if (status.w_T.w_Coredump) {
				(void) fprintf(outfp, " - core dumped");
			}
		}
		if (ignore_error) {
			(void) fprintf(outfp, " (ignored)");
		}
		(void) fprintf(outfp, "\n");
		(void) fflush(outfp);
	}
	return failed;
}

/*
 *	sh_command2string(command, destination)
 *
 *	Run one sh command and capture the output from it.
 *
 *	Return value:
 *
 *	Parameters:
 *		command		The command to run
 *		destination	Where to deposit the output from the command
 *		
 *	Static variables used:
 *
 *	Global variables used:
 */
void
sh_command2string(command, destination)
	register String		command;
	register String		destination;
{
	register FILE		*fd;
	register int		chr;
	int			status;

	command->text.p = (int) nul_char;
	if ((fd = popen(command->buffer.start, "r")) == NULL) {
		fatal("Could not run command `%s' for :sh transformation",
		      command->buffer.start);
	}
	while ((chr = getc(fd)) != EOF) {
		if (chr == (int) newline_char) {
			chr = (int) space_char;
		}
		append_char(chr, destination);
	}
	if ((status = pclose(fd)) != 0) {
		fatal("The command `%s' returned status `%d'",
		      command->buffer.start,
		      status);
	}
}
