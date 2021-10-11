#ident "@(#)globals.c 1.1 92/07/30 Copyright 1988 Sun Micro"

/*
 *	globals.c
 *
 *	This declares all global variables
 */

/*
 * Included files
 */
#ifndef COMPILING_MAKESH
#include "defs.h"
#endif

#include <sys/resource.h>

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Global variables used by make only
 */
#ifndef COMPILING_MAKESH
	Boolean		all_parallel;			/* PARALLEL */
	Boolean		assign_done;
	Boolean		build_failed_seen;
	Boolean		command_changed;
	Boolean		commands_done;
	Boolean		conditional_macro_used;
	Chain		conditional_targets;
	Boolean		continue_after_error;		/* `-k' */
	Property	current_line;
	Name		current_target;
	short		debug_level;
	FILE		*dependency_report_file;
	char		*file_being_read;
	int		file_number;
	int		line_number;
	char		*make_state_lockfile;
	Boolean		make_word_mentioned;
	Dependency	makefiles_used;
	Boolean		no_parallel;			/* PARALLEL */
	Boolean		nse;				/* NSE on */
	Boolean		only_parallel;			/* PARALLEL */
	Boolean		parallel;			/* PARALLEL */
	Boolean		query_mentioned;
	Boolean		quest;				/* `-q' */
	short		read_trace_level;
	Boolean		reading_environment;
	int		recursion_level;
	Boolean		report_dependencies_only;	/* -P */
	Boolean		rewrite_statefile;
	Running		running_list;
	char		*sccs_dir_path;
	Name		stderr_file;
	Name		stdout_file;
	Boolean		stdout_stderr_same;
	char		*temp_file_directory = ".";
	Name		temp_file_name;
	short		temp_file_number;
	Boolean		touch;				/* `-t' */
	Boolean		trace_reader;			/* `-D' */
#endif

/*
 * Global variables used by make and makesh
 */
int foo;
	Name		built_last_make_run;
	Name		c_at;
	char		char_semantics[256];
	Boolean		cleanup;
	Boolean		close_report;
	Name		conditionals;
	Name		current_make_version;
	Cmd_line	default_rule;
	Name		default_rule_name;
	Name		default_target_to_build;
	Boolean		do_not_exec_rule;		/* `-n' */
	Name		done;
	Name		dot;
	Name		dot_keep_state;
	Name		empty_name;
	Boolean		fatal_in_progress;
	Boolean		filter_stderr;			/* `-X' */
	Name		force;
	Name		hashtab[hashsize];
	Name		host_arch;
	Name		ignore_name;
	Boolean		ignore_errors;			/* `-i' */
	Name		init;
	Boolean		keep_state;
	Name		make_state;
	Makefile_type	makefile_type = reading_nothing;
	Name		makeflags;
	Name		make_version;
	Name		no_parallel_name;
	Name		not_auto;
	Name		parallel_name;
	int		parallel_process_cnt;
	Name		path_name;
	Percent		percent_list;
	Name		plus;
	Name		precious;
	Name		query;
	Name		recursive_name;
	Name		remote_command_name;
	Boolean		report_pwd;
	Name		sccs_get_name;
	Cmd_line	sccs_get_rule;
	Name		shell_name;
	Boolean		silent;				/* `-s' */
	Name		silent_name;
	Dependency	suffixes;
	Name		suffixes_name;
	Name		sunpro_dependencies;
	Name		target_arch;
	Boolean		vpath_defined;
	Name		virtual_root;
	Name		vpath_name;
	pathpt		vroot_path = VROOT_DEFAULT;
	Name		wait_name;
	Boolean		working_on_targets;

#ifdef lint
	char		**environ;
	int		errno;
#endif

/*
 * File table of contents
 */
