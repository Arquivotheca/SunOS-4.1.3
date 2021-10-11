/*    @(#)defs.h 1.1 92/07/30 SMI      */

/*
 *	Copyright (c) 1986 Sun Microsystems, Inc.	[Remotely from S5R2]
 */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <alloca.h>
#include <values.h>
#include <vroot.h>
#include <ctype.h>

/*
 * Typedefs for all structs
 */
typedef struct Chain		*Chain, Chain_rec;
typedef struct Cmd_line		*Cmd_line, Cmd_line_rec;
typedef struct Dependency	*Dependency, Dependency_rec;
typedef struct Envvar		*Envvar, Envvar_rec;
typedef struct Name_vector	*Name_vector, Name_vector_rec;
typedef struct Name		*Name, Name_rec;
typedef struct Percent		*Percent, Percent_rec;
typedef struct Property		*Property, Property_rec;
typedef struct Running		*Running, Running_rec;
typedef struct Source		*Source, Source_rec;
typedef struct String		*String, String_rec;

/*
 * Some random constants (in an enum so dbx knows their values)
 */
enum {
	update_delay = 30,		/* time between rstat checks */
	max_fails = 3,			/* max no. of rstat failures before */
					/* we call host dead */
#ifdef sun386
	ar_member_name_len = 14,
#else
	ar_member_name_len = 15,
#endif
	hashsize = 2048,		/* size of hash table */
};

/*
 * Symbols that defines all the different char constants make uses
 */
enum {
	ampersand_char =	'&',
	asterisk_char =		'*',
	at_char =		'@',
	backslash_char =	'\\',
	bar_char =		'|',
	braceleft_char =	'{',
	braceright_char =	'}',
	bracketleft_char =	'[',
	bracketright_char =	']',
	colon_char =		':',
	comma_char =		',',
	dollar_char =		'$',
	equal_char =		'=',
	exclam_char =		'!',
	greater_char =		'>',
	hyphen_char =		'-',
	newline_char =		'\n',
	nul_char =		'\0',
	numbersign_char =	'#',
	parenleft_char =	'(',
	parenright_char =	')',
	percent_char =		'%',
	period_char =		'.',
	plus_char =		'+',
	question_char =		'?',
	semicolon_char =	';',
	slash_char =		'/',
	space_char =		' ',
	tab_char =		'\t',
	tilde_char =		'~',
};

/*
 * Some utility macros
 */
#define OUT_OF_DATE(a,b)	(((a) < (b)) || (((a) == 0) && ((b) == 0)))
#define SETVAR(name, value, append) \
				setvar_daemon(name, value, append, no_daemon)
#define GETNAME(a,b)		getname_fn((a), (b), false)
#define FIND_LENGTH		-1
#define IS_EQUAL(a,b)		(!strcmp((a), (b)))
#define IS_EQUALN(a,b,n)	(!strncmp((a), (b), (n)))
#define VSIZEOF(v)		(sizeof (v) / sizeof ((v)[0]))
#define ALLOC(x)		((struct x *)getmem(sizeof (struct x)))


/*
 * A type and some utilities for boolean values
 */
typedef enum {
	false =		0,
	true =		1,
	failed =	0,
	succeeded =	1
} Boolean;
#define BOOLEAN(expr)		((expr) ? true : false)

/*
 * Some stuff for the reader
 */
typedef enum {
	no_state,
	scan_name_state,
	scan_command_state,
	enter_dependencies_state,
	enter_conditional_state,
	enter_equal_state,
	illegal_eoln_state,
	poorly_formed_macro_state,
	exit_state
}                       Reader_state;

struct Name_vector {
	Name                    names[64];
	Chain                   target_group[64];
	short                   used;
	Name_vector		next;
};


/*
 * Bits stored in funny vector to classify chars
 */
enum {
	dollar_sem =		0001,
	meta_sem =		0002,
	percent_sem =		0004,
	wildcard_sem =		0010,
	command_prefix_sem =	0020,
	special_macro_sem =	0040,
	colon_sem =		0100,
	parenleft_sem =		0200
};

/*
 * Type returned from doname class functions
 */
typedef enum {
	build_dont_know = 0,
	build_failed,
	build_ok,
	build_in_progress,
	build_running,		/* PARALLEL */
	build_pending,		/* PARALLEL */
	build_serial,		/* PARALLEL */
	build_subtree		/* PARALLEL */
} Doname;

/* The String struct defines a string with the following layout
 *	"xxxxxxxxxxxxxxxCxxxxxxxxxxxxxxx________________"
 *	^		^		^		^
 *	|		|		|		|
 *	buffer.start	text.p		text.end	buffer.end
 *	text.p points to the next char to read/write.
 */
struct String {
	struct Text {
		char		*p;	/* Read/Write pointer */
		char		*end;	/* Read limit pointer */
	}		text;
	struct Physical_buffer {
		char		*start;	/* Points to start of buffer */
		char		*end;	/* End of physical buffer */
	}		buffer;
	Boolean		free_after_use:1;
};

#define STRING_BUFFER_LENGTH	1024
#define INIT_STRING_FROM_STACK(str, buf) { \
			str.buffer.start = str.text.p = (buf); \
			str.text.end = NULL; \
			str.buffer.end = (buf) + sizeof (buf); \
			str.free_after_use = false; \
		  }

/*
 * Used for storing the $? list and also for the "target + target:"
 * construct.
 */
struct Chain {
	Chain			next;
	Name			name;
};

/*
 * Stores one command line for a rule
 */
struct Cmd_line {
	Cmd_line		next;
	Name			command_line;
	Boolean			make_refd:1;	/* $(MAKE) referenced? */
	/*
	 * Remember any command line prefixes given
	 */
	Boolean			ignore_command_dependency:1;	/* `?' */
	Boolean			assign:1;			/* `=' */
	Boolean			ignore_error:1;			/* `-' */
	Boolean			silent:1;			/* `@' */
};

/*
 * Linked list of targets/files
 */
struct Dependency {
	Dependency		next;
	Name			name;
	Boolean			automatic:1;
	Boolean			stale:1;
	Boolean			built:1;
};

/*
 * The specials are markers for targets that the reader should special case
 */
typedef enum {
	no_special,
	built_last_make_run_special,
	default_special,
	ignore_special,
	keep_state_special,
	make_version_special,
	no_parallel_special,
	parallel_special,
	precious_special,
	sccs_get_special,
	silent_special,
	suffixes_special,
} Special;

typedef enum {
	no_colon,
	one_colon,
	two_colon,
	equal_seen,
	conditional_seen,
	none_seen
} Separator;

/*
 * Magic values for the timestamp stored with each name object
 */
enum {
	file_no_time =		-1,
	file_doesnt_exist =	0,
	file_is_dir =		1,
	file_max_time =		MAXINT,
};

struct Name {
	Name			next;		/* pointer to next Name */
	Property		prop;		/* List of properties */
	char			*string;	/* ASCII name string */
	struct {
		unsigned int		length;
		unsigned int		sum;
	}                       hash;
	struct {
		time_t			time;		/* Modification */
		int			errno;		/* error from "stat" */
		unsigned int		size;		/* Of file */
		unsigned short		mode;		/* Of file */
		Boolean			is_file:1;
		Boolean			is_dir:1;
		Boolean			is_precious:1;
		enum {
			dont_know_sccs = 0,
			no_sccs,
			has_sccs
		}			has_sccs:2;
	}                       stat;
	/*
	 * Count instances of :: definitions for this target
	 */
	short			colon_splits;
	/*
	 * We only clear the automatic depes once per target per report
	 */
	short			temp_file_number;
	/*
	 * Count how many conditional macros this target has defined
	 */
	short			conditional_cnt;
	/*
	 * A conditional macro was used when building this target
	 */
	Boolean			depends_on_conditional:1;
	Boolean			has_member_depe:1;
	Boolean			is_member:1;
	/*
	 * This target is a directory that has been read
	 */
	Boolean			has_read_dir:1;
	/*
	 * This name is a macro that is now being expanded
	 */
	Boolean			being_expanded:1;
	/*
	 * This name is a magic name that the reader must know about
	 */
	Special			special_reader:4;
	Doname			state:3;
	Separator		colons:2;
	Boolean			has_depe_list_expanded:1;
	Boolean			suffix_scan_done:1;
	Boolean			has_complained:1;	/* For sccs */
	/*
	 * This target has been built during this make run
	 */
	Boolean			has_built:1;
	Boolean			with_squiggle:1;	/* for .SUFFIXES */
	Boolean			without_squiggle:1;	/* for .SUFFIXES */
	Boolean			has_read_suffixes:1;	/* Suffix list cached*/
	Boolean			has_suffixes:1;
	Boolean			has_target_prop:1;
	Boolean			has_vpath_alias_prop:1;
	Boolean			dependency_printed:1;	/* For dump_make_state() */
	Boolean			dollar:1;		/* In namestring */
	Boolean			meta:1;			/* In namestring */
	Boolean			percent:1;		/* In namestring */
	Boolean			wildcard:1;		/* In namestring */
	Boolean			colon:1;		/* In namestring */
	Boolean			parenleft:1;		/* In namestring */
	Boolean			has_recursive_dependency:1;
	Boolean			has_regular_dependency:1;
	Boolean			is_double_colon:1;
	Boolean			is_double_colon_parent:1;
	Boolean			has_long_member_name:1;
	/*
	 * allowed to run in parallel
	 */
	Boolean			parallel:1;
	/*
	 * not allowed to run in parallel
	 */
	Boolean			no_parallel:1;
	/*
	 * used in dependency_conflict
	 */
	Boolean			checking_subtree:1;
	Boolean			added_pattern_conditionals:1;
};

/*
 * Stores the % matched default rules
 */
struct Percent {
	Percent			next;
	Name			target_prefix;
	Name			target_suffix;
	Name			source_prefix;
	Name			source_suffix;
	Dependency		dependencies;
	Cmd_line		command_template;
	Boolean			being_expanded:1;
	Boolean			source_percent:1;
};

/*
 * Each Name has a list of properties
 * The properties are used to store information that only
 * a subset of the Names need
 */
typedef enum {
	no_prop,
	conditional_prop,
	line_prop,
	macro_prop,
	makefile_prop,
	member_prop,
	recursive_prop,
	sccs_prop,
	suffix_prop,
	target_prop,
	time_prop,
	vpath_alias_prop,
	long_member_name_prop,
} Property_id;

typedef enum {
	no_daemon = 0,
	chain_daemon
} Daemon;

#define PROPERTY_HEAD_SIZE (sizeof (struct Property)-sizeof (union Body))
struct Property {
	Property		next;
	Property_id		type:4;
	union Body {
		struct Conditional {
			/*
			 * For "foo := ABC [+]= xyz" constructs
			 * Name "foo" gets one conditional prop
			 */
			Name			target;
			Name			name;
			Name			value;
			int			sequence;
			Boolean			append:1;
		}			conditional;
		struct Line {
			/*
			 * For "target : dependencies" constructs
			 * Name "target" gets one line prop
			 */
			Cmd_line		command_template;
			Cmd_line		command_used;
			Dependency		dependencies;
			time_t			dependency_time;
			Chain			target_group;
			Boolean			is_out_of_date:1;
			Boolean			sccs_command:1;
			Boolean			command_template_redefined:1;
			/*
			 * Values for the dynamic macros
			 */
			Name			target;
			Name			star;
			Name			less;
			Name			percent;
			Chain			query;
		}			line;
		struct Macro {
			/*
			 * For "ABC = xyz" constructs
			 * Name "ABC" get one macro prop
			 */
			Name			value;
			Boolean			exported:1;
			Boolean			read_only:1;
			/*
			 * This macro is defined conditionally
			 */
			Boolean			is_conditional:1;
			/*
			 * The list for $? is stored as a structured list that
			 * is translated into a string iff it is referenced.
			 * This is why  some macro values need a daemon. 
			 */
			Daemon			daemon:2;
		}			macro;
		struct Makefile {
			/*
			 * Names that reference makefiles gets one prop
			 */
			char			*contents;
			int			size;
		}			makefile;
		struct Member {
			/*
			 * For "lib(member)" and "lib((entry))" constructs
			 * Name "lib(member)" gets one member prop
			 * Name "lib((entry))" gets one member prop
			 * The member field is filled in when the prop is refd
			 */
			Name			library;
			Name			entry;
			Name			member;
		}			member;
		struct Recursive {
			/*
			 * For "target: .RECURSIVE dir makefiles" constructs
			 * Used to keep track of recursive calls to make
			 * Name "target" gets one recursive prop
			 */
			Name			directory;
			Name			target;
			Dependency		makefiles;
			Boolean			has_built;
		}			recursive;
		struct Sccs {
			/*
			 * Each file that has a SCCS s. file gets one prop
			 */
			Name			file;
		}			sccs;
		struct Suffix {
			/*
			 * Cached list of suffixes that can build this target
			 * suffix is built from .SUFFIXES
			 */
			Name			suffix;
			Cmd_line		command_template;
		}			suffix;
		struct Target {
			/*
			 * For "target:: dependencies" constructs
			 * The "::" construct is handled by converting it to
			 * "foo: 1@foo" + "1@foo: dependecies"
			 * "1@foo" gets one target prop
			 * This target prop cause $@ to be bound to "foo"
			 * not "1@foo" when the rule is evaluated
			 */
			Name			target;
		}			target;
		struct Time {
			/*
			 * Save the original time for :: targets
			 */
			time_t			time;
		}			time;
		struct Vpath_alias {
			/*
			 * If a file was found using the VPATH it gets
			 * a vpath_alias prop
			 */
			Name			alias;
		}			vpath_alias;
		struct Long_member_name {
			/*
			 * Targets with a truncated member name carries
			 * the full lib(member) name for the state file
			 */
			Name			member_name;
		}			long_member_name;
	}			body;
};

/*
 * Macros for the reader
 */
#define UNCACHE_SOURCE()	if (source != NULL) { \
					source->string.text.p = source_p; \
				  }
#define CACHE_SOURCE(comp)	if (source != NULL) { \
					source_p = source->string.text.p - \
					  (comp); \
					source_end = source->string.text.end; \
				  }
#define GET_NEXT_BLOCK(source)	{ UNCACHE_SOURCE(); \
				 source = get_next_block_fn(source); \
				 CACHE_SOURCE(0) \
			   }
#define GET_CHAR()		((source == NULL) || \
				(source_p >= source_end) ? 0 : *source_p)

struct Source {
	String_rec		string;
	Source			previous;
	int			bytes_left_in_file;
	short			fd;
	Boolean			already_expanded:1;
};

struct Running {
	Running			next;
	Doname			state;
	Name			target;
	Name			true_target;
	int			recursion_level;
	Boolean			do_get;
	Boolean			implicit;
	Boolean			redo;
	int			auto_count;
	Name			*automatics;
	int			pid;
	int			host;
	Name			stdout_file;
	Name			stderr_file;
	Name			temp_file;
	int			conditional_cnt;
	Name			*conditional_targets;
};

struct Envvar {
	Name			name;
	Name			value;
	Envvar			next;
};

typedef enum {
	reading_nothing,
	reading_makefile,
	reading_statefile,
	rereading_statefile,
	reading_cpp_file
} Makefile_type;

/*
 *	extern declarations for all global variables.
 *	The actual declarations are in globals.c
 */
extern	Boolean		all_parallel;
extern	Boolean		assign_done;
extern	Boolean		build_failed_seen;
extern	Name		built_last_make_run;
extern	Name		c_at;
extern	char		char_semantics[256];
extern	Boolean		command_changed;
extern	Boolean		commands_done;
extern	Boolean		conditional_macro_used;
extern	Chain		conditional_targets;
extern	Name		conditionals;
extern	Boolean		continue_after_error;
extern	Property	current_line;
extern	Name		current_make_version;
extern	Name		current_target;
extern	short		debug_level;
extern	Cmd_line	default_rule;
extern	Name		default_rule_name;
extern	Name		default_target_to_build;
extern	Boolean		do_not_exec_rule;
extern	Name		done;
extern	Name		dot;
extern	Name		dot_keep_state;
extern	Name		empty_name;
extern	Boolean		fatal_in_progress;
extern	char		*file_being_read;
extern	int		file_number;
extern	Boolean		filter_stderr;
extern	Name		force;
extern	Name		hashtab[hashsize];
extern	Name		host_arch;
extern	Name		ignore_name;
extern	Boolean		ignore_errors;
extern	Name		init;
extern	Boolean		keep_state;
extern	int		line_number;
extern	Name		make_state;
extern	char		*make_state_lockfile;
extern	Boolean		make_word_mentioned;
extern	Makefile_type	makefile_type;
extern	Dependency	makefiles_used;
extern	Name		makeflags;
extern	Name		make_version;
extern	Boolean		no_parallel;
extern	Name		no_parallel_name;
extern	Name		not_auto;
extern	Boolean		nse;
extern	Boolean		only_parallel;
extern	Boolean		parallel;
extern	Name		parallel_name;
extern	int		parallel_process_cnt;
extern	Name		path_name;
extern	Percent		percent_list;
extern	Name		plus;
extern	Name		precious;
extern	Name		query;
extern	Boolean		query_mentioned;
extern	Boolean		quest;
extern	short		read_trace_level;
extern	Boolean		reading_environment;
extern	int		recursion_level;
extern	Name		recursive_name;
extern	Name		remote_command_name;
extern	Boolean		report_dependencies_only;
extern	Boolean		report_pwd;
extern	Boolean		rewrite_statefile;
extern	Running		running_list;
extern	char		*sccs_dir_path;
extern	Name		sccs_get_name;
extern	Cmd_line	sccs_get_rule;
extern	Name		shell_name;
extern	Boolean		silent;
extern	Name		silent_name;
extern	Name		stderr_file;
extern	Name		stdout_file;
extern	Boolean		stdout_stderr_same;
extern	Dependency	suffixes;
extern	Name		suffixes_name;
extern	Name		sunpro_dependencies;
extern	Name		target_arch;
extern	char		*temp_file_directory;
extern	Name		temp_file_name;
extern	short		temp_file_number;
extern	Boolean		touch;
extern	Boolean		trace_reader;
extern	Name		virtual_root;
extern	Boolean		vpath_defined;
extern	Name		vpath_name;
extern	pathpt		vroot_path;
extern	Name		wait_name;
extern	Boolean		working_on_targets;

/*
 * Declarations of system defined variables
 */
extern	char		**environ;
extern	int		errno;
extern	char		*sys_siglist[];

/*
 * Declarations of system supplied functions
 */
extern	char		*alloca();
extern	char		*getenv();
extern	long		lseek();
extern	int		sleep();
extern	char		*sprintf();
extern	long		strtol();
extern	char		*file_lock();

/*
 * Declarations of functions declared and used by make
 */
extern	void		add_pending();
extern	void		add_running();
extern	void		add_serial();
extern	void		add_subtree();
extern	void		append_char();
extern	Property	append_prop();
extern	void		append_string();
extern	void		await_parallel();
extern	void		build_suffix_list();
extern	Boolean		check_auto_dependencies();
extern	void		check_state();
extern	Doname		doname();
extern	Doname		doname_check();
extern	Doname		doname_parallel();
extern	Doname		dosys();
extern	void		dump_make_state();
extern	void		enable_interrupt();
extern	void		enter_conditional();
extern	void		enter_dependencies();
extern	void		enter_dependency();
extern	void		enter_equal();
extern	Name_vector	enter_name();
extern	char		*errmsg();
extern	Boolean		exec_vp();
extern	Doname		execute_parallel();
extern	Doname		execute_serial();
extern	time_t  	exists();
extern	void		expand_macro();
extern	void		expand_value();
extern	void		fatal();
extern	void		fatal_reader();
extern	Doname		find_ar_suffix_rule();
extern	Doname		find_double_suffix_rule();
extern	Doname		find_percent_rule();
extern	Doname		find_suffix_rule();
extern	void		find_target_groups();
extern	void		finish_children();
extern	void		finish_running();
extern	char		*get_current_path();
extern	Source		get_next_block_fn();
extern	Property	get_prop();
extern	char		*getmem();
extern	Name		getname_fn();
extern	Name		getvar();
extern	char		*getwd();
extern	void		handle_interrupt();
extern	Boolean		is_running();
extern	void		load_cached_names();
extern	Property	maybe_append_prop();
extern	Boolean		parallel_ok();
extern	time_t		read_archive();
extern	void		read_directory_of_file();
extern	Boolean		read_make_machines();
extern	Boolean		read_simple_file();
extern	void		remove_recursive_dep();
extern	void		report_recursive_dep();
extern	void		report_recursive_done();
extern	void		report_recursive_init();
extern	void		reset_locals();
extern	void		retmem();
extern	void		sh_command2string();
extern	void		set_locals();
extern	void		setup_char_semantics();
extern	void		setup_interrupt();
extern	Property	setvar_daemon();
extern	void		setvar_envvar();
extern	char		*time_to_string();
extern	void		update_target();
extern	void		warning();
extern	void		write_state_file();
extern	Name		vpath_translation();
