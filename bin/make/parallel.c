#ident "@(#)parallel.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

#ifdef PARALLEL
/*
 *	parallel.c
 *
 *	Deal with the parallel processing
 */

/*
 * Included files
 */
#include "defs.h"
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/dk.h>
#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

/*
 * Defined macros
 */
#define MAXRULES 100
#define BUFSIZE 1024

#define SKIPSPACE(x) while (*x && \
			    (*x == (int) space_char || \
			     *x == (int) tab_char || \
			     *x == (int) comma_char)) { \
						       x++; \
						   }

#define SKIPWORD(x) while (*x && \
			   (*x != (int) space_char) && \
			   (*x != (int) tab_char) && \
			   (*x != (int) newline_char) && \
			   (*x != (int) comma_char) && \
			   (*x != (int) equal_char)) { \
					   x++; \
				     }
#define SKIPTOEND(x) while (*x && (*x != (int) newline_char)) { \
						   x++; \
					     }

/*
 * typedefs & structs
 */

/*
 * Static variables
 */
static	Boolean		first_time;
static	Boolean		local;
static	char		**machines;
static	u_long		*machine_addr;
static	long		*machine_boot;
static	int		machine_cnt;
static	int		*machine_failcnt;
static	int		*machine_limit;
static	int		*machine_load;
static	int		*machine_max;
static	u_long		myaddr;
static	char		myhost[33];
static	int		mymachine;
static	struct timeval	next_load_check;
static	Boolean		parent;
static	int		process_machine = -1;
static	int		process_running = -1;
static	Running		*running_tail = &running_list;
static	Name		subtree_conflict;
static	Name		subtree_conflict2;

/*
 * File table of contents
 */
extern	Doname		execute_parallel();
extern	void		distribute_process();
extern	void		redirect_io();
extern	Doname		doname_parallel();
extern	void		doname_subtree();
extern	void		finish_running();
extern	void		process_next();
extern	Property	*set_conditionals();
extern	void		reset_conditionals();
extern	Boolean		dependency_conflict();
extern	void		await_parallel();
extern	void		update_machine_max();
extern	void		finish_children();
extern	void		dump_out_file();
extern	void		finish_doname();
extern	void		add_running();
extern	void		add_pending();
extern	void		add_serial();
extern	void		add_subtree();
extern	void		store_conditionals();
extern	Boolean		parallel_ok();
extern	Boolean		read_make_machines();
extern	Boolean		host_stat();
extern	Boolean		is_running();

/*
 *	execute_parallel(line)
 *
 *	Spawns a parallel process to execute the command group.
 *
 *	Return value:
 *				The result of the execution
 *
 *	Parameters:
 *		line		The command group to execute
 *
 *	Global variables used:
 *		file_number	Temporary file number
 *		parallel_process_cnt Number of processes currently running
 *		shell_name	Shell to use for execution
 *		silent		Silent flag
 *		stderr_file	Temporary output file for stderr
 *		stdout_file	Temporary output file for stdout
 *		stdout_stderr_same Flag set true if stderr and stdout the same
 *		vpath_defined	True if vpath translation needed
 */
Doname
execute_parallel(line)
	Property		line;
{
	Cmd_line		rule;
	Name			target = line->body.line.target;
	char			*argv[MAXRULES + 5];
	char			**p;
	char			*cp;
	Boolean			silent_flag;
	int			ignore;
	int			argcnt;
	char			string[MAXPATHLEN];

	/* Command invoked: makesh path shell -silent/nosilent command ... */
	p = argv;
	*p++ = "/usr/lib/makesh";
	*p++ = get_current_path();
	*p++ = getvar(shell_name)->string;
	*p++ = silent ? "-silent" : "-nosilent";
	argcnt = 0;
	for (rule = line->body.line.command_used;
	     rule != NULL;
	     rule = rule->next) {
		if (vpath_defined) {
			rule->command_line =
			  vpath_translation(rule->command_line);
		}
		silent_flag = false;
		ignore = 0;
		if (rule->command_line->hash.length > 0) {
			if (++argcnt == MAXRULES) {
				/* Too many rules, run serially instead */
				return build_serial;
			}
			if (rule->silent && !silent) {
				silent_flag = true;
			}
			if (rule->ignore_error) {
				ignore++;
			}
			if (silent_flag || ignore) {
				*p = (char *) alloca(strlen(rule->
							    command_line->
							    string) +
						     (silent_flag ? 1 : 0) +
						     ignore + 1);
				cp = *p++;
				if (silent_flag) {
					*cp++ = (int) at_char;
				}
				if (ignore) {
					*cp++ = (int) hyphen_char;
				}
				(void) strcpy(cp, rule->command_line->string);
			} else {
				*p++ = rule->command_line->string;
			}
		}
	}
	if (argcnt == 0) {
		return build_ok;
	}
	*p = NULL;
	(void) sprintf(string,
		       "/tmp/make.stdout.%d.%d",
		       getpid(),
		       file_number++);
	stdout_file = GETNAME(string, FIND_LENGTH);
	stdout_file->stat.is_file = true;
	if (!stdout_stderr_same) {
		(void) sprintf(string,
			       "/tmp/make.stderr.%d.%d",
			      getpid(),
			       file_number++);
		stderr_file = GETNAME(string, FIND_LENGTH);
		stderr_file->stat.is_file = true;
	} else {
		stderr_file = NULL;
	}
	distribute_process(argv);
	parallel_process_cnt++;
	return build_running;
}

/*
 *	distribute_process(command)
 *
 *	Finds a machine on which to run the given command.
 *
 *	Parameters:
 *		command		Argv vector of command to execute
 *
 *	Static variables used:
 *		local		True if builds must be done locally
 *		machines	Array of string machine names to run commands
 *		machine_addr	Array of integer host addresses for machines
 *		machine_cnt	Number of machines available
 *		machine_load	Array of load numbers for each machine
 *		machine_max	Array of maximum processes per each machine
 *		myaddr		The host address of this machine
 *		mymachine	The index of this machine into the machine ary
 *		parent		True if we are the top most make running
 *		process_running	Set to the pid of the process set running
 *		process_machine	Set to the index of the machine used
 *
 *	Global variables used:
 *		nse		dwight
 *		parallel_process_cnt Number of parallel process running
 *		remote_command_name Set to the name of the remote execution cmd
 */
static void
distribute_process(command)
	char		**command;
{
	int		i;
	int		*lp;
	int		min_load;
	int		*mm;
	int		available;
	char		**argv;
	char		**ap;
	char		*program;
	Property	remote_command;
	u_long		address;
	Boolean		warned = false;

	min_load = 1000;
	process_machine = -1;
	while (process_machine == -1) {
		update_machine_max();
		/*
		 * If nothing is running, we're the parent make, and this
		 * machine is on the list, use it.
		 */
		if (parallel_process_cnt == 0 &&
		    parent &&
		    mymachine != -1) {
			process_machine = mymachine;
			break;
		}
		/*
		 * If we're building this target locally, wait for all
		 * the children to finish.
		 */
		if (local) {
			if (parallel_process_cnt == 0) {
				break;
			} else {
				await_parallel(false);
				finish_children(true);
				continue;
			}
		}
		/* Look for the machine with the lowest load */
		for (i = 0, lp = machine_load, mm = machine_max;
		     i < machine_cnt;
		     i++, lp++, mm++) {
			if ((*lp < min_load) && (*lp < *mm)) {
				process_machine = i;
				min_load = *lp;
			}
		}
		if (process_machine == -1) {
			/*
			 * if no machine available and nothing running,
			 * give warning and sleep.
			 */
			if (parallel_process_cnt == 0) {
				update_machine_max();
				available = 0;
				for (i = 0, mm = machine_max;
				     i < machine_cnt;
				     i++, mm++) {
					if (*mm > 0) {
						available += *mm;
					}
				}
				if (available == 0) {
					if (isatty(2)) {
						warning("No machines available, waiting for load to come down");
						warned = true;
					}
					sleep((int) update_delay);
				}
			} else {
				await_parallel(false);
				finish_children(true);
			}
		}
	}
	/* If warning previously given, retract it */
	if (warned) {
		warned = false;
		warning("Machines available, continuing...");
	}
	if (local) {
		address = myaddr;
	} else {
		address = *(machine_addr + process_machine);
		*(machine_load + process_machine) += 1;
	}
	/*
	 * If we are building locally, just fork makesh; otherwise
	 * use the remote execution program to send the command.
	 */
	if (address == myaddr) {
		argv = command;
		program = "/usr/lib/makesh";
	} else {
		for (i = 0, argv = command; *argv; i++, argv++);
		argv = (char **) alloca((int) ((i+4) * sizeof (char *)));
		program = NULL;
		ap = argv;
		remote_command =
		  get_prop(remote_command_name->prop, macro_prop);
		if (remote_command != NULL) {
			program = remote_command->body.macro.value->string;
		}
		if (program != NULL && strlen(program) > 0) {
			*ap++ = program;
		} else {
			program = nse ?
			  	     "/usr/nse/bin/nse_on" : "/usr/bin/on";
			*ap++ = program;
			*ap++ = "-i";
		}
		*ap++ = *(machines + process_machine);
		while (*command) {
			*ap++ = *command++;
		}
		*ap = NULL;
	}
	setvar_envvar();
	if ((process_running = vfork()) == 0) {
		enable_interrupt((void (*) ()) SIG_DFL);
		redirect_io();
		(void) execve(program, argv, environ);
		(void) fprintf(stderr,
			       "make: Cannot load command `%s': %s\n",
			       program,
			       errmsg(errno));
		(void) fflush(stderr);
		_exit(1);
	}
	enable_interrupt(handle_interrupt);
}

/*
 *	redirect_io()
 *
 *	Redirects stdin, stdout, and stderr for a parallel child.
 *
 *	Parameters:
 *
 *	Global variables used:
 *		stderr_file	Temporary output file for stderr
 *		stdout_file	Temporary output file for stdout
 *		stdout_stderr_same Flag set if stdout and stderr are the same
 */
static void
redirect_io()
{
	(void) close(0);
	(void) open("/dev/null", O_RDONLY);
	(void) close(1);
	(void) close(2);
	if (open(stdout_file->string,
		 O_WRONLY | O_CREAT,
		 S_IREAD|S_IWRITE) < 0) {
		fatal("Couldn't open standard out temp file `%s': %s", 
		      stdout_file->string,
		      errmsg(errno));
	}
	if (stdout_stderr_same) {
		(void) dup2(1, 2);
	}
	else if (open(stderr_file->string,
		      O_WRONLY | O_CREAT,
		      S_IREAD|S_IWRITE) < 0) {
		fatal("Couldn't open standard error temp file `%s': %s",
		      stderr_file->string,
		      errmsg(errno));
	}
}

/*
 *	doname_parallel(target, do_get, implicit)
 *
 *	Processes the given target and finishes up any parallel
 *	processes left running.
 *
 *	Return value:
 *				Result of target build
 *
 *	Parameters:
 *		target		Target to build
 *		do_get		True if sccs get to be done
 *		implicit	True if this is an implicit target
 *
 *	Global variables used:
 */
Doname
doname_parallel(target, do_get, implicit)
	Name		target;
	Boolean		do_get;
	Boolean		implicit;
{
	Doname		result;

	result = doname_check(target, do_get, implicit, false);
	if (result == build_ok || result == build_failed) {
		return result;
	}
	finish_running();
	return target->state;
}

/*
 *	doname_subtree(target, do_get, implicit)
 *
 *	Completely computes an object and its dependents for a
 *	serial subtree build.
 *
 *	Parameters:
 *		target		Target to build
 *		do_get		True if sccs get to be done
 *		implicit	True if this is an implicit target
 *
 *	Static variables used:
 *		running_tail	Tail of the list of running processes
 *
 *	Global variables used:
 *		running_list	The list of running processes
 */
static void
doname_subtree(target, do_get, implicit)
	Name		target;
	Boolean		do_get;
	Boolean		implicit;
{
	Running		save_running_list;
	Running		*save_running_tail;

	save_running_list = running_list;
	save_running_tail = running_tail;
	running_list = NULL;
	running_tail = &running_list;
	target->state = build_subtree;
	target->checking_subtree = true;
	while(doname_check(target, do_get, implicit, false) == build_running) {
		target->checking_subtree = false;
		finish_running();
		target->state = build_subtree;
	}
	target->checking_subtree = false;
	running_list = save_running_list;
	running_tail = save_running_tail;
}

/*
 *	finish_running()
 *
 *	Keeps processing until the running_list is emptied out.
 *
 *	Parameters:
 *
 *	Global variables used:
 *		running_list	The list of running processes
 */
void
finish_running()
{
	while (running_list != NULL) {
		await_parallel(true);
		finish_children(true);
		if (running_list != NULL) {
			process_next();
		}
	}
}

/*
 *	process_next()
 *
 *	Searches the running list for any targets which can
 *	start processing.  This can be either a pending target, a serial target
 *	or a subtree target.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		running_tail		The end of the list of running procs
 *		subtree_conflict	A target which conflicts with a subtree
 *		subtree_conflict2	The other target which conflicts
 *
 *	Global variables used:
 *		commands_done		True if commands executed
 *		debug_level		Controls debug output
 *		parallel_process_cnt	Number of parallel process running
 *		recursion_level		Indentation for debug output
 *		running_list		List of running processes
 */
static void
process_next()
{
	Running		rp;
	Running		*rp_prev;
	Property	line;
	Chain		target_group;
	Dependency	dep;
	Boolean		quiescent = true;
	Running		*subtree_target;
	Boolean		saved_commands_done;
	Property	*conditionals;

	subtree_target = NULL;
	subtree_conflict = NULL;
	subtree_conflict2 = NULL;
	/*
	 * If nothing currently running, build a serial target, if any.
	 */
	for (rp_prev = &running_list, rp = running_list;
	     rp != NULL && parallel_process_cnt == 0;
	     rp = rp->next) {
		if (rp->state == build_serial) {
			*rp_prev = rp->next;
			if (rp->next == NULL) {
				running_tail = rp_prev;
			}
			recursion_level = rp->recursion_level;
			rp->target->state = build_pending;
			conditionals =
				set_conditionals(rp->conditional_cnt,
						 rp->conditional_targets);
			(void) doname_check(rp->target,
					    rp->do_get,
					    rp->implicit,
					    false);
			reset_conditionals(rp->conditional_cnt,
					   rp->conditional_targets,
					   conditionals);
			quiescent = false;
			break;
		} else {
			rp_prev = &rp->next;
		}
	}
	/*
	 * Find a target to build.  The target must be pending, have all
	 * its dependencies built, and not be in a target group with a target
	 * currently building.
	 */
	for (rp_prev = &running_list, rp = running_list;
	     rp != NULL;
	     rp = rp->next) {
		if (!(rp->state == build_pending ||
		      rp->state == build_subtree)) {
			quiescent = false;
			rp_prev = &rp->next;
		} else if (rp->state == build_pending) {
			line = get_prop(rp->target->prop, line_prop);
			for(dep = line->body.line.dependencies;
			    dep != NULL;
			    dep = dep->next) {
				if (dep->name->state == build_running ||
				    dep->name->state == build_pending ||
				    dep->name->state == build_serial) {
					break;
				}
			}
			if (dep == NULL) {
				for (target_group = line->body.
				     line.target_group;
				     target_group != NULL;
				     target_group = target_group->next) {
					if (is_running(target_group->name)) {
						break;
					}
				}
				if (target_group == NULL) {
					*rp_prev = rp->next;
					if (rp->next == NULL) {
						running_tail = rp_prev;
					}
					recursion_level = rp->recursion_level;
					rp->target->state = rp->redo ?
					  build_dont_know : build_pending;
					saved_commands_done = commands_done;
					conditionals =
						set_conditionals
						    (rp->conditional_cnt,
						     rp->conditional_targets);
					if ((doname_check(rp->target,
							  rp->do_get,
							  rp->implicit,
							  false) !=
					     build_running) &&
					    !commands_done) {
						commands_done =
						  saved_commands_done;
					}
					reset_conditionals
						(rp->conditional_cnt,
						 rp->conditional_targets,
						 conditionals);
					quiescent = false;
					break;
				} else {
					rp_prev = &rp->next;
				}
			} else {
				rp_prev = &rp->next;
			}
		} else {
			rp_prev = &rp->next;
		}
	}
	/*
	 * If nothing has been found to build and there exists a subtree
	 * target with no dependency conflicts, build it.
	 */
	if (quiescent) {
		for(rp_prev = &running_list, rp = running_list;
		    rp != NULL;
		    rp = rp->next) {
			if (rp->state == build_subtree) {
				if (!dependency_conflict(rp->target)) {
					*rp_prev = rp->next;
					if (rp->next == NULL) {
						running_tail = rp_prev;
					}
					recursion_level = rp->recursion_level;
					conditionals =
						set_conditionals
						  (rp->conditional_cnt,
						   rp->conditional_targets);
					doname_subtree(rp->target,
						       rp->do_get,
						       rp->implicit);
					reset_conditionals
						(rp->conditional_cnt,
						 rp->conditional_targets,
						 conditionals);
					quiescent = false;
					break;
				} else {
					subtree_target = rp_prev;
					rp_prev = &rp->next;
				}
			} else {
				rp_prev = &rp->next;
			}
		}
	}
	/*
	 * If still nothing found to build, we either have a deadlock or
	 * a subtree with a dependency conflict with something waiting to
	 * build.
	 */
	if (quiescent) {
		if (subtree_target == NULL) {
			fatal("Internal error: deadlock detected in process_next");
		} else {
			rp = *subtree_target;
			if (debug_level > 0) {
				warning("Conditional macro conflict encountered for %s between %s and %s",
					subtree_conflict2->string,
					rp->target->string,
					subtree_conflict->string);
			}
			*subtree_target = (*subtree_target)->next;
			if (rp->next == NULL) {
				running_tail = subtree_target;
			}
			recursion_level = rp->recursion_level;
			conditionals =
				set_conditionals(rp->conditional_cnt,
						 rp->conditional_targets);
			doname_subtree(rp->target, rp->do_get, rp->implicit);
			reset_conditionals(rp->conditional_cnt,
					   rp->conditional_targets,
					   conditionals);
		}
	}
}

/*
 *	set_conditionals(cnt, targets)
 *
 *	Sets the conditional macros for the targets given in the array of
 *	targets.  The old macro values are returned in an array of
 *	Properties for later resetting.
 *
 *	Return value:
 *					Array of conditional macro settings
 *
 *	Parameters:
 *		cnt			Number of targets
 *		targets			Array of targets
 */
static Property *
set_conditionals(cnt, targets)
	int cnt;
	Name *targets;
{
	Property *locals, *lp;
	Name *tp;

	locals = (Property *) getmem(cnt * sizeof(Property));
	for (lp=locals, tp=targets; cnt>0; cnt--, lp++, tp++) {
		*lp = (Property) getmem((*tp)->conditional_cnt *
					sizeof(struct Property));
		set_locals(*tp, *lp);
	}
	return locals;
}

/*
 *	reset_conditionals(cnt, targets, locals)
 *
 *	Resets the conditional macros as saved in the given array of
 *	Properties.  The resets are done in reverse order.  Afterwards the
 *	data structures are freed.
 *
 *	Parameters:
 *		cnt			Number of targets
 *		targets			Array of targets
 *		locals			Array of dependency macro settings
 *					
 */
static void
reset_conditionals(cnt, targets, locals)
	int cnt;
	Name *targets;
	Property *locals;
{
	Name *tp;
	Property *lp;

	for (tp=targets+(cnt-1), lp=locals+(cnt-1); cnt>0; cnt--, tp--, lp--) {
		reset_locals(*tp, *lp,
			     get_prop((*tp)->prop, conditional_prop), 0);
		retmem((char *) *lp);
	}
	retmem((char *) locals);
}

/*
 *	dependency_conflict(target)
 *
 *	Returns true if there is an intersection between
 *	the subtree of the target and any dependents of the pending targets.
 *
 *	Return value:
 *					True if conflict found
 *
 *	Parameters:
 *		target			Subtree target to check
 *
 *	Static variables used:
 *		subtree_conflict	Target conflict found
 *		subtree_conflict2	Second conflict found
 *
 *	Global variables used:
 *		running_list		List of running processes
 *		wait_name		.WAIT, not a real dependency
 */
static Boolean
dependency_conflict(target)
	Name		target;
{
	Property	line;
	Property	pending_line;
	Dependency	dp;
	Dependency	pending_dp;
	Running		rp;

	/* Return if we are already checking this target */
	if (target->checking_subtree) {
		return false;
	}
	target->checking_subtree = true;
	line = get_prop(target->prop, line_prop);
	if (line == NULL) {
		target->checking_subtree = false;
		return false;
	}
	/* Check each dependency of the target for conflicts */
	for (dp = line->body.line.dependencies; dp != NULL; dp = dp->next) {
		/* Ignore .WAIT dependency */
		if (dp->name == wait_name) {
			continue;
		}
		/*
		 * For each pending target, look for a dependency which
		 * is the same as a dependency of the subtree target.  Since
		 * we can't build the subtree until all pending targets have
		 * finished which depend on the same dependency, this is
		 * a conflict.
		 */
		for (rp = running_list; rp != NULL; rp = rp->next) {
			if (rp->state == build_pending) {
				pending_line = get_prop(rp->target->prop,
							line_prop);
				if (pending_line == NULL) {
					continue;
				}
				for(pending_dp = pending_line->
				    			body.line.dependencies;
				    pending_dp != NULL;
				    pending_dp = pending_dp->next) {
					if (dp->name == pending_dp->name) {
						target->checking_subtree
						  		= false;
						subtree_conflict = rp->target;
						subtree_conflict2 = dp->name;
						return true;
					}
				}
			}
		}
		if (dependency_conflict(dp->name)) {
			target->checking_subtree = false;
			return true;
		}
	}
	target->checking_subtree = false;
	return false;
}

/*
 *	await_parallel(waitflg)
 *
 *	Waits for parallel children to exit and finishes their processing.
 *	If waitflg is false, the function returns after update_delay.
 *
 *	Parameters:
 *		waitflg		dwight
 *
 *	Global variables used:
 */
void
await_parallel(waitflg)
	Boolean		waitflg;
{
	int		pid;
	int		waiterr;
	Running		rp;
	union wait	status;
	Boolean		nohang;

	nohang = false;
	for(;;) {
		if (!nohang) {
			(void) alarm((int) update_delay);
		}
		pid = wait3(&status,
			    nohang ? WNOHANG : 0,
			    (struct rusage *) NULL);
		waiterr = errno;
		if (!nohang) {
			(void) alarm(0);
		}
		if (pid <= 0) {
			if (waiterr == EINTR) {
				if (waitflg) {
					update_machine_max();
					continue;
				} else {
					return;
				}
			} else {
				return;
			}
		}
		for (rp = running_list;
		     rp != NULL && pid != rp->pid;
		     rp = rp->next);
		if (rp == NULL) {
			fatal("Internal error: returned child pid not in running_list");
		} else {
			rp->state = status.w_status ? build_failed : build_ok;
		}
		nohang = true;
		parallel_process_cnt--;
		if (rp->host >= 0) {
			*(machine_load + rp->host) -= 1;
		}
	}
}

/*
 *	update_machine_max()
 *
 *	Checks the load every so often and adjusts the
 *	maximum allowed processes per processor.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		first_time	Set if this is the first time through
 *		local		Set if we are only building locally
 *		machines	Array of string machine names to run commands
 *		machine_addr	Array of integer host addresses for machines
 *		machine_load	Array of load numbers for each machine
 *		machine_max	Array of maximum processes per each machine
 *		machine_boot	Array of boot times for each machine
 *		machine_failcnt	Array of stat fail count for each machine
 *		machine_limit	Array of process limits for each machine
 *
 *	Static variables used:
 *		myaddr		The host address of this machine
 *
 *	Global variables used:
 *		next_load_check	Time for next check of load averages
 *		running_list	List of running processes
 */
static void
update_machine_max()
{
	struct timeval		now;
	int			i;
	int			load;
	int			*lp;
	int			*ml;
	int			*mm;
	int			*fp;
	int			used;
	int			idle;
	char			**mp;
	u_long			*ap;
	struct statsswtch	stat;
	Running			rp;
	long			*bp;

	if (first_time) {
		/*
		 * If this is the first time, set up the load averages.
		 */
		for (mp = machines,
		     ap = machine_addr,
		     lp = machine_load,
		     ml = machine_limit,
		     mm = machine_max,
		     bp = machine_boot,
		     fp = machine_failcnt;
		     *mp;
		     mp++, ap++, lp++, ml++, mm++, bp++, fp++) {
			if (*ap == myaddr) {
				*mm = *ml;
			} else {
				if (!host_stat(*ap, &stat)) {
					/* host may be dead */
					if (*fp > 0) {
						*fp = (int) max_fails;
					}
					*mm = -1;
					*lp = 0;
					continue;
				}
				if (*mm == -1) {
					*mm = 0;
					*lp = 0;
					continue;
				}
				if (stat.boottime.tv_sec != *bp) {
					*bp = stat.boottime.tv_sec;
					*mm = 0;
					*lp = 0;
					continue;
				}
				for (i = 0, used = 0; i < CP_IDLE; i++) {
					used += stat.cp_time[i];
				}
				used -= *lp;
				idle = stat.cp_time[CP_IDLE] - *mm;
				if (used == 0) {
					load = 0;
				} else if (idle > 0) {
					load = (used * 100)/(used + idle);
				} else {
					load = 100;
				}
				if (load < 30) {
					*mm = *ml;
				} else if (load < 80) {
					*mm = 1;
				} else {
					*mm = 0;
				}
			}
			*lp = 0;
		}
		first_time = false;
		(void) gettimeofday(&now, (struct timezone *) NULL);
		next_load_check.tv_sec = now.tv_sec + (int) update_delay;
	} else {
		/*
		 * If it's time, update the load averages.
		 */
		(void) gettimeofday(&now, (struct timezone *) NULL);
		if (now.tv_sec < next_load_check.tv_sec) {
			return;
		}
		next_load_check.tv_sec = now.tv_sec + (int) update_delay - 1;
		for (i = 0,
		     mp = machines,
		     ap = machine_addr,
		     ml = machine_limit,
		     mm = machine_max,
		     lp = machine_load,
		     bp = machine_boot,
		     fp = machine_failcnt;
		     *mp;
		     i++,
		     mp++,
		     ap++,
		     ml++,
		     mm++,
		     lp++,
		     bp++,
		     fp++) {
			long timediff;

			if (*ap == myaddr) {
				continue;
			}
			if (!host_stat(*ap, &stat)) {
				if (++(*fp) == (int) max_fails) {
					warning("No answer from host %s", *mp);
					/* Kill all of the process there */
					for (rp = running_list;
					     rp != NULL;
					     rp = rp->next) {
						if (rp->host == i) {
							(void) kill(rp->pid,
								    SIGKILL);
						}
					}
				}
				*mm = -1;
				continue;
			}
			if (*mm == -1) {
				*mm = 0;
			}
			*fp = 0;
			timediff = stat.boottime.tv_sec - *bp;
			if (timediff > 10 || timediff < -10) {
				if (*bp != 0) {
					warning("Host %s rebooted", *mp);
					for (rp = running_list;
					     rp != NULL;
					     rp = rp->next) {
						if (rp->host == i) {
							(void) kill(rp->pid,
								    SIGKILL);
						}
					}
				}
			}
			*bp = stat.boottime.tv_sec;
			load = (int) stat.avenrun[0];
			/*
			 * If the load is less than 0.50, we assume the
			 * machine is unloaded and allocate the limit.  If the
			 * load is less than the limit then we add one more.
			 * If the load is more than the current max, the load
			 * is too high and we back off one.
			 */
			if (load < (FSCALE/2)) {
				*mm = *ml;
			} else if (load < *ml * FSCALE) {
				if (*mm < *ml) {
					(*mm)++;
				}
			} else if (load > (*ml * FSCALE)) {
				if (*mm > 0) {
					(*mm)--;
				}
			}
		}
	}
	/* Check to make sure at least one machine is usable */
	for (mp = machines, mm = machine_max, fp = machine_failcnt;
	     *mp;
	     mp++, mm++, fp++) {
		if (!(*mm == -1 && *fp >= (int) max_fails)) {
			break;
		}
	}
	if (*mp == NULL) {
		if (!local) {
			warning("No answer from any host on list, using local machine");
			local = true;
		}
	} else if (local) {
		warning("Host responded, not running local anymore");
		local = false;
	}
}

/*
 *	finish_children(docheck)
 *
 *	Finishes the processing for all targets which were running
 *	and have now completed.
 *
 *	Parameters:
 *		docheck		Completely check the finished target
 *
 *	Static variables used:
 *		machines	Array of machine names
 *		myhost		The name of this machine
 *		running_tail	The tail of the running list
 *
 *	Global variables used:
 *		continue_after_error  -k flag
 *		fatal_in_progress  True if we are finishing up after fatal err
 *		running_list	List of running processes
 */
void
finish_children(docheck)
	Boolean		docheck;
{
	Running		rp;
	Running		*rp_prev;
	Property	line;
	char		*host;

	for(rp_prev = &running_list, rp = running_list;
	    rp != NULL;
	    rp = rp->next) {
		/*
		 * If the state is ok or failed then this target has finished
		 * building.  Output the accumulated stdout and stderr,
		 * read the auto dependency stuff, handle a failed build,
		 * update the target, then finish the doname process for
		 * that target.
		 */
		if (rp->state == build_ok || rp->state == build_failed) {
			*rp_prev = rp->next;
			if (rp->next == NULL) {
				running_tail = rp_prev;
			}
			if (rp->stdout_file != NULL) {
				dump_out_file(rp->stdout_file->string, 1);
				rp->stdout_file = NULL;
			}
			if (rp->stderr_file != NULL) {
				dump_out_file(rp->stderr_file->string, 2);
				rp->stderr_file = NULL;
			}
			check_state(rp->state, rp->temp_file);
			rp->temp_file = NULL;
			if (rp->state == build_failed) {
				line = get_prop(rp->target->prop, line_prop);
				if (line != NULL) {
					line->body.line.command_used = NULL;
				}
				if (rp->host == -1) {
					host = myhost;
				} else {
					host = *(machines + rp->host);
				}
				if (continue_after_error ||
				    fatal_in_progress ||
				    !docheck) {
					warning("Command failed for target `%s' (on host %s)",
						rp->target->string,
						host);
					build_failed_seen = true;
				} else {
					fatal("Command failed for target `%s' (on host %s)",
					      rp->target->string,
					      host);
				}
			}
			if (!docheck) {
				continue;
			}
			update_target(get_prop(rp->target->prop, line_prop),
				      rp->state);
			finish_doname(rp);
		} else {
			rp_prev = &rp->next;
		}
	}
}

/*
 *	dump_out_file(filename, out_fd)
 *
 *	Write the contents of the file to output then unlink the file.
 *
 *	Parameters:
 *		filename	Name of temp file containing output
 *		out_fd		File descriptor to write to.
 *
 *	Global variables used:
 */
static void
dump_out_file(filename, out_fd)
	char		*filename;
	int		out_fd;
{
	int		fd;
	int		cc;
	char		copybuf[BUFSIZE];
	struct stat	filestat;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		fatal("Couldn't open output temporary file: %s",
		      errmsg(errno));
	}
	/* the on command puts a final, extra <cr><lf> at the end which */
	/* we'll remove */
	if (stat(filename, &filestat) < 0) {
		fatal("Couldn't stat temporary file: %s", errmsg(errno));
	}
	if (filestat.st_size > 2) {
		if (lseek(fd, (long) (filestat.st_size-3), L_SET) < 0) {
			fatal("Failed lseek on temporary file: %s",
			      errmsg(errno));
		}
		if (read(fd, copybuf, 3) < 0) {
			fatal("Failed read on temporary file: %s",
			      errmsg(errno));
		}
		if (copybuf[0] == 012 &&
		    copybuf[1] == 015 && copybuf[2] == 012) {
			if (truncate(filename, filestat.st_size-2) < 0) {
				fatal("Couldn't truncate temporary file: %s",
				      errmsg(errno));
			}
		}
		if (lseek(fd, (long) 0, L_SET) < 0) {
			fatal("Failed lseek on temporary file: %s",
			      errmsg(errno));
		}
	}
	for (cc = read(fd, copybuf, BUFSIZE);
	     cc > 0;
	     cc = read(fd, copybuf, BUFSIZE)) {
		/*
		 * read buffers from the source file until end or error.
		 */
		if (write(out_fd, copybuf, cc) < 0) {
			fatal("Write failed to output: %s", errmsg(errno));
		}
	}
	(void) close(fd);
	(void) unlink(filename);
}

/*
 *	finish_doname(rp)
 *
 *	Completes the processing for a target which was left running.
 *
 *	Parameters:
 *		rp		Running list entry for target
 *
 *	Global variables used:
 *		debug_level	Debug flag
 *		recursion_level	Indentation for debug output
 */
static void
finish_doname(rp)
	Running		rp;
{
	Doname		result = rp->state;
	Name		target = rp->target;
	int		auto_count = rp->auto_count;
	Name		*automatics = rp->automatics;
	Name		true_target = rp->true_target;

	recursion_level = rp->recursion_level;
	if (result == build_ok) {
		/* If all went OK set a nice timestamp */
		if (true_target->stat.time == (int) file_doesnt_exist) {
			true_target->stat.time = (int) file_max_time;
		}
	}
	target->state = result;
	if (target->is_member) {
		Property member;

		/* Propagate the timestamp from the member file to the member*/
		if ((target->stat.time != (int) file_max_time) &&
		    ((member = get_prop(target->prop, member_prop)) != NULL)) {
			target->stat.time = exists(member->body.member.member);
		}
	}
	/* Check if we found any new auto dependencies when we built */
	/* the target */
	if ((result == build_ok) && check_auto_dependencies(target,
							    auto_count,
							    automatics)) {
		if (debug_level > 0) {
			(void) printf("%*sTarget `%s' acquired new dependencies from build, checking those\n",
				      recursion_level,
				      "",
				      true_target->string);
		}
		target->state = build_running;
		add_pending(target,
			    recursion_level,
			    rp->do_get,
			    rp->implicit,
			    true);
	}
}

/*
 *	add_running(target, true_target, recursion_level, auto_count,
 *					automatics, do_get, implicit)
 *
 *	Adds a record on the running list for this target, which
 *	was just spawned and is running.
 *
 *	Parameters:
 *		target		Target being built
 *		true_target	True target for target
 *		recursion_level	Debug indentation level
 *		auto_count	Count of automatic dependencies
 *		automatics	List of automatic dependencies
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *
 *	Static variables used:
 *		process_machine	Index of machine building target
 *		running_tail	Tail of running list
 *		process_running	PID of process
 *
 *	Global variables used:
 *		current_line	Current line for target
 *		current_target	Current target being built
 *		stderr_file	Temporary file for stdout
 *		stdout_file	Temporary file for stdout
 *		temp_file_name	Temporary file for auto dependencies
 */
void
add_running(target, true_target, recursion_level, auto_count,
					automatics, do_get, implicit)
	Name		target;
	Name		true_target;
	int		recursion_level;
	int		auto_count;
	Name		*automatics;
	Boolean		do_get;
	Boolean		implicit;
{
	Running		rp;
	Name		*p;

	rp = ALLOC(Running);
	rp->state = build_running;
	rp->target = target;
	rp->true_target = true_target;
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->auto_count = auto_count;
	if (auto_count > 0) {
		rp->automatics = (Name *) getmem(auto_count * sizeof (Name));
		for (p = rp->automatics; auto_count > 0; auto_count--) {
			*p++ = *automatics++;
		}
	} else {
		rp->automatics = NULL;
	}
	rp->pid = process_running;
	rp->host = process_machine;
	rp->stdout_file = stdout_file;
	rp->stderr_file = stderr_file;
	rp->temp_file = temp_file_name;
	rp->redo = false;
	rp->next = NULL;
	store_conditionals(rp);
	process_running = -1;
	stdout_file = NULL;
	stderr_file = NULL;
	temp_file_name = NULL;
	current_target = NULL;
	current_line = NULL;
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	add_pending(target, recursion_level, do_get, implicit, redo)
 *
 *	Adds a record on the running list for a pending target
 *	(waiting for its dependents to finish running).
 *
 *	Parameters:
 *		target		Target being built
 *		recursion_level	Debug indentation level
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *		redo		True if this target is being redone
 *
 *	Static variables used:
 *		running_tail	Tail of running list
 */
void
add_pending(target, recursion_level, do_get, implicit, redo)
	Name		target;
	int		recursion_level;
	Boolean		do_get;
	Boolean		implicit;
	Boolean		redo;
{
	Running		rp;

	rp = ALLOC(Running);
	rp->state = build_pending;
	rp->target = target;
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->next = NULL;
	rp->stdout_file = NULL;
	rp->stderr_file = NULL;
	rp->temp_file = NULL;
	rp->redo = redo;
	store_conditionals(rp);
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	add_serial(target, recursion_level, do_get, implicit)
 *
 *	Adds a record on the running list for a target which must be
 *	executed in serial after others have finished.
 *
 *	Parameters:
 *		target		Target being built
 *		recursion_level	Debug indentation level
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *
 *	Static variables used:
 *		running_tail	Tail of running list
 */
void
add_serial(target, recursion_level, do_get, implicit)
	Name		target;
	Boolean		do_get;
	Boolean		implicit;
{
	Running		rp;

	rp = ALLOC(Running);
	rp->target = target;
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->state = build_serial;
	rp->next = NULL;
	rp->stdout_file = NULL;
	rp->stderr_file = NULL;
	rp->temp_file = NULL;
	rp->redo = false;
	store_conditionals(rp);
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	add_subtree(target, recursion_level, do_get, implicit)
 *
 *	Adds a record on the running list for a target which must be
 *	executed in isolation after others have finished.
 *
 *	Parameters:
 *		target		Target being built
 *		recursion_level	Debug indentation level
 *		do_get		Sccs get flag
 *		implicit	Implicit flag
 *
 *	Static variables used:
 *		running_tail	Tail of running list
 */
void
add_subtree(target, recursion_level, do_get, implicit)
	Name		target;
	Boolean		do_get;
	Boolean		implicit;
{
	Running		rp;

	rp = ALLOC(Running);
	rp->target = target;
	rp->recursion_level = recursion_level;
	rp->do_get = do_get;
	rp->implicit = implicit;
	rp->state = build_subtree;
	rp->next = NULL;
	rp->stdout_file = NULL;
	rp->stderr_file = NULL;
	rp->temp_file = NULL;
	rp->redo = false;
	store_conditionals(rp);
	*running_tail = rp;
	running_tail = &rp->next;
}

/*
 *	store_conditionals(rp)
 *
 *	Creates an array of the currently active targets with conditional
 *	macros (found in the chain conditional_targets) and puts that
 *	array in the Running struct.
 *
 *	Parameters:
 *		rp		Running struct for storing chain
 *
 *	Global variables used:
 *		conditional_targets  Chain of current dynamic conditionals
 */
static void
store_conditionals(rp)
	Running rp;
{
	int cnt;
	Chain cond_name;

	if (conditional_targets == NULL) {
		rp->conditional_cnt = 0;
		rp->conditional_targets = NULL;
		return;
	}
	cnt = 0;
	for (cond_name = conditional_targets;
	     cond_name != NULL;
	     cond_name = cond_name->next) {
		cnt++;
	}
	rp->conditional_cnt = cnt;
	rp->conditional_targets = (Name *)getmem(cnt * sizeof(Name));
	for (cond_name = conditional_targets;
	     cond_name != NULL;
	     cond_name = cond_name->next) {
		rp->conditional_targets[--cnt] = cond_name->name;
	}
}

/*
 *	parallel_ok(target)
 *
 *	Returns true if the target can be run in parallel
 *
 *	Return value:
 *				True if can run in parallel
 *
 *	Parameters:
 *		target		Target being tested
 *
 *	Global variables used:
 *		all_parallel	True if all targets default to parallel
 *		only_parallel	True if no targets default to parallel
 */
Boolean
parallel_ok(target)
	Name		target;
{
	Boolean		assign;
	Boolean		make_refd;
	Property	line;
	Cmd_line	rule;

	assign = make_refd = false;
	if ((line = get_prop(target->prop, line_prop)) == NULL) {
		return false;
	}
	for (rule = line->body.line.command_used;
	     rule != NULL;
	     rule = rule->next) {
		if (rule->assign) {
			assign = true;
		} else if (rule->make_refd) {
			make_refd = true;
		}
	}
	if (assign) {
		return false;
	} else if (target->parallel) {
		return true;
	} else if (target->no_parallel) {
		return false;
	} else if (all_parallel) {
		return true;
	} else if (only_parallel) {
		return false;
	} else if (make_refd) {
		return false;
	}
	return true;
}

/*
 *	read_make_machines(make_machines)
 *
 *	If this is a parallelizable run, read the MAKE_MACHINES environment
 *	variable or read the .make.machines file.
 *	Set up the array of remote machines and machine process loads
 *
 *	Return value:
 *				True if information found
 *
 *	Parameters:
 *		make_machines	Name of .make.machines file
 *
 *	Static variables used:
 *		first_time	Set to indicate we are starting
 *		machines	Array of machine names read from file
 *		machine_addr	Array of address for the machines
 *		machine_boot	Array of boot times for the machines
 *		machine_cnt	Cound of machines in file
 *		machine_failcnt	Array of fail count for the machines
 *		machine_limit	Array of process limits for the machines
 *		machine_load	Array of load averages for the machines
 *		machine_max	Array of maximum processes for the machines
 *		myaddr		Host address of this machine
 *		myhost		Name of this host
 *		mymachine	Index of this host in array of machines
 *		parent		True if this is the top most make
 *
 *	Global variables used:
 *		do_not_exec_rule -n flag
 *		no_parallel	-R flag
 *		quest		-q flag
 *		report_dependencies -P flag
 *		touch		-t flag
 */
Boolean
read_make_machines(make_machines)
	Name			make_machines;
{
	char			machines_path[MAXPATHLEN];
	Boolean			default_make_machines;
	char			*machines_list;
	Boolean			parallel_flag = false;
	struct stat		machines_buf;
	FILE			*machines_file;
	char			*machine_string;
	char			*ms;
	char			**mp;
	char			*arg;
	int			i;
	int			*ml;
	int			*lp;
	int			*mm;
	int			*fp;
	int			limit;
	Boolean			foundeq;
	Boolean			foundend;
	long	 		*bp;
	u_long			*ap;
	struct statsswtch	stat;
	struct hostent		*hp;
	Name			MAKE_MACHINES;

	MAKE_MACHINES = GETNAME("MAKE_MACHINES", FIND_LENGTH);
	parent = true;
	if (make_machines == NULL) {
		(void) sprintf(machines_path,
			       "%s/.make.machines",
			       getenv("HOME"));
		make_machines = GETNAME(machines_path, FIND_LENGTH);
		default_make_machines = true;
	} else {
		default_make_machines = false;
	}
	if (no_parallel ||
	    touch ||
	    quest ||
	    do_not_exec_rule ||
	    report_dependencies_only) {
		parallel_flag = false;
	} else if ((machines_list =
		    getenv(MAKE_MACHINES->string)) != NULL) {
		parallel_flag = true;
		parent = false;
	} else if ((machines_file =
		    fopen(make_machines->string, "r")) != NULL) {
		if (fstat(fileno(machines_file), &machines_buf) < 0) {
			fatal("fstat of %s failed: %s",
			      make_machines->string,
			      errmsg(errno));
		}
		machines_list = (char *) getmem((int) (machines_buf.st_size +
						       2 +
						       MAKE_MACHINES->
						       hash.length));
		(void) sprintf(machines_list,
			       "%s=",
			       MAKE_MACHINES->string);
		if (fread(machines_list + MAKE_MACHINES->hash.length + 1,
			  sizeof (char),
			  (int) machines_buf.st_size,
			  machines_file) != machines_buf.st_size) {
			warning("Unable to read %s",
				make_machines->string);
			(void) fclose(machines_file);
		} else {
			(void) fclose(machines_file);
			*(machines_list + MAKE_MACHINES->hash.length
			     + 1 + machines_buf.st_size) = (int) nul_char;
			if (putenv(machines_list) != 0) {
				warning("Couldn't put %s in environment",
					make_machines->string);
			} else {
				machines_list +=
				  MAKE_MACHINES->hash.length + 1;
				parallel_flag = true;
			}
		}
	} else if (!default_make_machines) {
		fatal("Open of `%s' failed: %s",
		      make_machines->string,
		      errmsg(errno));
		}

	if (!parallel_flag) {
		return false;
	}


	(void) gethostname(myhost, 32);
	myhost[32] = (int) nul_char;
	hp = gethostbyname(myhost);
	bcopy(hp->h_addr, (char *) &myaddr, hp->h_length);
	ms = machines_list;
	while (*ms && isspace(*ms)) {
		ms++;
	}
	ms = machine_string = strdup(ms);
	while (*ms) {
		machine_cnt++;
		SKIPTOEND(ms);
		if (*ms) {
			ms++;
		}
	}
	machines = (char **) getmem((machine_cnt + 1) * sizeof (char *));
	machine_addr = (u_long *) getmem(machine_cnt * sizeof (u_long));
	machine_load = (int *) getmem(machine_cnt * sizeof (int));
	machine_limit = (int *) getmem(machine_cnt * sizeof (int));
	machine_max = (int *) getmem(machine_cnt * sizeof (int));
	machine_boot = (long *) getmem(machine_cnt * sizeof (long));
	machine_failcnt = (int *) getmem(machine_cnt * sizeof (int));
	for (ms = machine_string, mp = machines, ml = machine_limit,
	     ap = machine_addr, lp = machine_load, mm = machine_max,
	     bp = machine_boot, fp = machine_failcnt;
	     *ms != NULL;
	     ) {
		SKIPSPACE(ms);
		*mp = ms;
		SKIPWORD(ms);
		if (*ms && *ms == (int) newline_char) {
			foundend = true;
		} else {
			foundend = false;
		}
		if (*ms) {
			*ms++ = (int) nul_char;
		}
		if (!foundend) {
			SKIPSPACE(ms);
			if (*ms && *ms == (int) newline_char) {
				ms++;
				foundend = true;
			}
		}
		if ((**mp == (int) nul_char) ||
		    (hp = gethostbyname(*mp)) == NULL) {
			if (**mp != (int) nul_char) {
				warning("Ignoring unknown host `%s'", *mp);
			}
			machine_cnt--;
			if (!foundend) {
				SKIPTOEND(ms);
				if (*ms) {
					ms++;
				}
			}
		} else {
			limit = 2;
			while (*ms && !foundend) {
				foundeq = false;
				arg = ms;
				SKIPWORD(ms);
				if (*ms && (*ms == (int) equal_char)) {
					foundeq = true;
				}
				if (*ms && (*ms == (int) newline_char)) {
					foundend = true;
				}
				if (*ms) {
					*ms++ = (int) nul_char;
				}
				SKIPSPACE(ms);
				if (IS_EQUAL(arg, "max")) {
					if (*ms &&
					    !foundend &&
					    (*ms == (int) equal_char)) {
						foundeq = true;
						ms++;
						SKIPSPACE(ms);
					}
					if (foundeq &&
					    !foundend) {
						arg = ms;
						SKIPWORD(ms);
						if (*ms &&
						    (*ms == (int) newline_char)) {
							foundend = true;
						}
						if (*ms) {
							*ms++ = (int)nul_char;
						}
						limit =
						  (int) strtol(arg, &arg, 10);
						if (*arg) {
							warning("Expected number for max option for host %s",
								*mp);
							limit = 2;
						} else if (limit < 1) {
							warning("Max option value must be positive number for host %s",
								*mp);
						}
						if (limit < 1) {
							limit = 2;
						}
					}
				}
				/* else if ... for each option */
				else {
					warning("Unknown option: `%s'", arg);
					if (*ms &&
					    !foundend &&
					    (*ms == (int) equal_char)) {
						ms++;
						SKIPWORD(ms);
						SKIPSPACE(ms);
					}
				}
				if (!foundend) {
					SKIPSPACE(ms);
				}
				if (*ms && (*ms == (int) newline_char)) {
					foundend = true;
					ms++;
				}
			}
			bcopy(hp->h_addr, (char *) ap, hp->h_length);
			if (*ap != myaddr) {
				if (!host_stat(*ap, &stat)) {
					*fp = 1;
					*mm = -1;
					*bp = 0;
				} else {
					for (i = 0, *lp = 0;
					     i < CP_IDLE;
					     i++) {
						*lp += stat.cp_time[i];
					}
					*mm = stat.cp_time[CP_IDLE];
					*bp = stat.boottime.tv_sec;
					*fp = 0;
				}
			}
			*ml = limit;
			mp++;
			ml++;
			ap++;
			lp++;
			mm++;
			bp++;
			fp++;
		}
	}
	if (machine_cnt < 1) {
		parallel_flag = false;
	} else {
		*mp = NULL;
		mymachine = -1;
		for (mp = machines, ap = machine_addr, i = 0;
		     *mp;
		     mp++, ap++, i++) {
			if (*ap == myaddr) {
				mymachine = i;
				break;
			}
		}
		first_time = true;
	}

	return true;
}

/*
 *	host_stat(addr, statsswtch)
 *
 *	Does an rstat() with a timeout of 5 seconds.  False is
 *	returned on failure.
 *
 *	Return value:
 *				True if host_stat succeeded
 *
 *	Parameters:
 *		addr		Address of host to stat
 *		statsswtch	Stat struct
 *
 *	Global variables used:
 *		xdr_statsswtch	dwight
 */
static Boolean
host_stat(addr, statsswtch)
	u_long			addr;
	struct statsswtch	*statsswtch;
{
	CLIENT			*client;
	struct timeval		timeout;
	struct sockaddr_in	serveradr;
	int			snum;
	enum clnt_stat		clnt_stat;

	snum = RPC_ANYSOCK;
	serveradr.sin_addr.s_addr = addr;
	serveradr.sin_family = AF_INET;
	serveradr.sin_port = 0;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if ((client = clntudp_bufcreate(&serveradr,
					(unsigned long) RSTATPROG,
					(unsigned long) RSTATVERS_SWTCH,
					timeout,
					&snum,
					sizeof (struct rpc_msg),
					sizeof (struct rpc_msg) +
					sizeof (struct statsswtch))) == NULL) {
		return false;
	}
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	clnt_stat = clnt_call(client,
			      RSTATPROC_STATS,
			      xdr_void,
			      0,
			      xdr_statsswtch,
			      statsswtch,
			      timeout);
	clnt_destroy(client);
	(void) close(snum);
	if (clnt_stat != RPC_SUCCESS) {
		return false;
	}
	return true;
}

/*
 *	is_running(target)
 *
 *	Returns true if the target is running.
 *
 *	Return value:
 *				True if target is running
 *
 *	Parameters:
 *		target		Target to check
 *
 *	Global variables used:
 *		running_list	List of running processes
 */
Boolean
is_running(target)
	Name		target;
{
	Running		rp;

	if (target->state != build_running) {
		return false;
	}
	for (rp = running_list;
	     rp != NULL && target != rp->target;
	     rp = rp->next);
	if (rp == NULL) {
		return false;
	} else {
		return (rp->state == build_running) ? true : false;
	}
}

#endif PARALLEL
