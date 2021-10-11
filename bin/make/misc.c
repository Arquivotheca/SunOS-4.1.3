#ident "@(#)misc.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	misc.c
 *
 *	This file contains various unclassified routines. Some main groups:
 *		getname
 *		Memory allocation
 *		String handling
 *		Property handling
 *		Error message handling
 *		Make internal state dumping
 *		main routine support
 */

/*
 * Included files
 */
#include "defs.h"
#include <report.h>
#include <varargs.h>

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Static variables
 */
static	void		(*sigivalue)() = SIG_DFL;
static	void		(*sigqvalue)() = SIG_DFL;

/*
 * File table of contents
 */
extern	Name		getname_fn();
extern	void		retmem();
extern	char		*getmem();
extern	void		append_string();
extern	void		append_char();
extern	void		expand_string();
extern	Property	append_prop();
extern	Property	maybe_append_prop();
extern	Property	get_prop();
extern	void		fatal();
extern	void		warning();
extern	char		*errmsg();
extern	char		*time_to_string();
extern	char		*get_current_path();
extern	void		dump_make_state();
extern	void		print_value();
extern	void		print_rule();
extern	void		setup_char_semantics();
extern	void		load_cached_names();
extern	void		setup_interrupt();
extern	void		enable_interrupt();

/*****************************************
 *
 *	getname
 */

/*
 *	getname_fn(name, len, dont_enter)
 *
 *	Hash a name string to the corresponding nameblock.
 *
 *	Return value:
 *				The Name block for the string
 *
 *	Parameters:
 *		name		The string we want to internalize
 *		len		The length of that string
 *		dont_enter	Don't enter the name if it does not exist
 *
 *	Global variables used:
 *		funny		The vector of semantic tags for characters
 *		hashtab		The hashtable used for the nametable
 */
Name
getname_fn(name, len, dont_enter)
	char			*name;
	register int		len;
	register Boolean	dont_enter;
{
	register unsigned int	hashsum = 0;
	register int		length;
	register Name		*hashslot;
	register unsigned char	*cap = (unsigned char *)name;
	register Name		np;
	static Name_rec		empty_Name;

	/* First figure out how long the string is. If the len argument */
	/* is -1 we count the chars here */
	if (len == FIND_LENGTH) {
		for (;
		     *cap != (int) nul_char;
		     hashsum <<= 1, hashsum += *cap++);
		length = (char *)cap - name;
	} else {
		length = len;
		for (; --len >= 0; hashsum <<= 1, hashsum += *cap++);
	}
	/* Run down the chain looking for the string */
	for (hashslot = &hashtab[hashsum % (int) hashsize], np = *hashslot;
	     (np != NULL) && ((len = np->hash.length) <= length);
	     hashslot = &(np->next), np = *hashslot) {
		if ((hashsum == np->hash.sum) && (length == len)) {
			if (!IS_EQUALN(np->string, name, length)) {
				goto not_this_one;
			}
			/* We found it. Just return the Name */
			return np;
		}
not_this_one:	;
	}
	if (dont_enter) {
		return NULL;
	}
	/* New name. Enter it */
	np = ALLOC(Name);
	*np = empty_Name;
	/* Get some memory for the namestring and copy it */
	np->string = getmem(length + 1);
	(void) bcopy(name, np->string, length);
	/* Fill in the new Name */
	np->string[length] = (int) nul_char;
	np->hash.length = length;
	np->hash.sum = hashsum;
	np->stat.time = (int) file_no_time;
	if (*hashslot != NULL) {
		np->next = *hashslot;
	}
	*hashslot = np;
	/* Scan the namestring to classify it */
	for (cap = (unsigned char *)name, len = 0; --length >= 0;) {
		len |= char_semantics[*cap++];
	}
	np->dollar = BOOLEAN((len & (int) dollar_sem) != 0);
	np->meta = BOOLEAN((len & (int) meta_sem) != 0);
	np->percent = BOOLEAN((len & (int) percent_sem) != 0);
	np->wildcard = BOOLEAN((len & (int) wildcard_sem) != 0);
	np->colon = BOOLEAN((len & (int) colon_sem) != 0);
	np->parenleft = BOOLEAN((len & (int) parenleft_sem) != 0);
	return np;
}

/*****************************************
 *
 *	Memory allocation
 */

/*
 *	retmem(p)
 *
 *	Cover funtion for free() to make it possible to insert advises.
 *
 *	Parameters:
 *		p		The memory block to free
 *
 *	Global variables used:
 */
void
retmem(p)
	register char		*p;
{
	(void) free(p);
}

/*
 *	getmem(size)
 *
 *	malloc() version that checks the returned value.
 *
 *	Return value:
 *				The memory chunk we allocated
 *
 *	Parameters:
 *		size		The size of the chunk we need
 *
 *	Global variables used:
 */
char *
getmem(size)
	register int		size;
{
	extern char            *malloc();
	register char          *result = malloc((unsigned) size);

	if (result == NULL) {
		fatal("Out of memory");
	}
	return result;
}

/*****************************************
 *
 *	String manipulation
 */

/*
 *	append_string(from, to, length)
 *
 *	Append a C string to a make string expanding it if nessecary
 *
 *	Parameters:
 *		from		The source (C style) string
 *		to		The destination (make style) string
 *		length		The length of the from string
 *
 *	Global variables used:
 */
void
append_string(from, to, length)
	register char		*from;
	register String		to;
	register int		length;
{
	if (length == FIND_LENGTH) {
		length = strlen(from);
	}
	if (to->buffer.start == NULL) {
		expand_string(to, 32 + length);
	}
	if (to->text.p + length >= to->buffer.end) {
		expand_string(to,
			      (to->buffer.end - to->buffer.start) * 2 +
			      length);
	}
	if (length > 0) {
		(void) bcopy(from, to->text.p, length);
		to->text.p += length;
	}
	*(to->text.p) = (int) nul_char;
}

/*
 *	append_char(from, to)
 *
 *	Append one char to a make string expanding it if nessecary
 *
 *	Parameters:
 *		from		Single character to append to string
 *		to		The destination (make style) string
 *
 *	Global variables used:
 */
void
append_char(from, to)
	char			from;
	register String		to;
{
	if (to->buffer.start == NULL) {
		expand_string(to, 32);
	}
	if (to->text.p + 2 >= to->buffer.end) {
		expand_string(to, to->buffer.end - to->buffer.start + 32);
	}
	*(to->text.p)++ = from;
	*(to->text.p) = (int) nul_char;
}

/*
 *	expand_string(string, length)
 *
 *	Allocate more memory for strings that run out of space.
 *
 *	Parameters:
 *		string		The make style string we want to expand
 *		length		The new length we need
 *
 *	Global variables used:
 */
static void
expand_string(string, length)
	register String		string;
	register int		length;
{
	register char		*p;

	if (string->buffer.start == NULL) {
		/* For strings that have no memory allocated */
		string->buffer.start =
		  string->text.p =
		    string->text.end =
		      getmem(length);
		string->buffer.end = string->buffer.start + length;
		string->text.p[0] = (int) nul_char;
		string->free_after_use = true;
		return;
	}
	if (string->buffer.end - string->buffer.start >= length) {
		/* If we really dont need more memory */
		return;
	}
	/* Get more memory, copy the string and free the old buffer if */
	/* it is was malloc()ed */
	p = getmem(length);
	(void) strcpy(p, string->buffer.start);
	string->text.p = p + (string->text.p - string->buffer.start);
	string->text.end = p + (string->text.end - string->buffer.start);
	string->buffer.end = p + length;
	if (string->free_after_use) {
		retmem(string->buffer.start);
	}
	string->buffer.start = p;
	string->free_after_use = true;
}

/*****************************************
 *
 *	Nameblock property handling
 */

/*
 *	append_prop(target, type)
 *
 *	Create a new property and append it to the property list of a Name.
 *
 *	Return value:
 *				A new property block for the target
 *
 *	Parameters:
 *		target		The target that wants a new property
 *		type		The type of property being requested
 *
 *	Global variables used:
 */
Property
append_prop(target, type)
	register Name		target;
	register Property_id	type;
{
	register Property	*insert = &target->prop;
	register Property	prop = *insert;
	register int		size;

	switch (type) {
	case conditional_prop:
		size = sizeof (struct Conditional);
		break;
	case line_prop:
		size = sizeof (struct Line);
		break;
	case macro_prop:
		size = sizeof (struct Macro);
		break;
	case makefile_prop:
		size = sizeof (struct Makefile);
		break;
	case member_prop:
		size = sizeof (struct Member);
		break;
	case recursive_prop:
		size = sizeof (struct Recursive);
		break;
	case sccs_prop:
		size = sizeof (struct Sccs);
		break;
	case suffix_prop:
		size = sizeof (struct Suffix);
		break;
	case target_prop:
		size = sizeof (struct Target);
		break;
	case time_prop:
		size = sizeof (struct Time);
		break;
	case vpath_alias_prop:
		size = sizeof (struct Vpath_alias);
		break;
	case long_member_name_prop:
		size = sizeof (struct Long_member_name);
		break;
	default:
		fatal("Internal error. Unknown prop type %d", type);
	}
	for (; prop != NULL; insert = &prop->next, prop = *insert);
	size += PROPERTY_HEAD_SIZE;
	*insert = prop = (Property) getmem(size);
	bzero((char *) prop, size);
	prop->type = type;
	prop->next = NULL;
	return prop;
}

/*
 *	maybe_append_prop(target, type)
 *
 *	Append a property to the Name if none of this type exists
 *	else return the one already there
 *
 *	Return value:
 *				A property of the requested type for the target
 *
 *	Parameters:
 *		target		The target that wants a new property
 *		type		The type of property being requested
 *
 *	Global variables used:
 */
Property
maybe_append_prop(target, type)
	register Name		target;
	register Property_id	type;
{
	register Property	prop;

	if ((prop = get_prop(target->prop, type)) != NULL) {
		return prop;
	}
	return append_prop(target, type);
}

/*
 *	get_prop(start, type)
 *
 *	Scan the property list of a Name to find the next property
 *	of a given type.
 *
 *	Return value:
 *				The first property of the type, if any left
 *
 *	Parameters:
 *		start		The first property block to check for type
 *		type		The type of property block we need
 *
 *	Global variables used:
 */
Property
get_prop(start, type)
	register Property	start;
	register Property_id	type;
{
	for (; start != NULL; start = start->next) {
		if (start->type == type) {
			return start;
		}
	}
	return NULL;
}

/*****************************************
 *
 *	Error message handling
 */

/*
 *	fatal(format, args...)
 *
 *	Print a message and die
 *
 *	Parameters:
 *		format		printf type format string
 *		args		Arguments to match the format
 *
 *	Global variables used:
 *		fatal_in_progress Indicates if this is a recursive call
 *		parallel_process_cnt Do we need to wait for anything?
 *		report_pwd	Should we report the current path?
 */
/*VARARGS*/
void
fatal(va_alist)
	va_dcl
{
	va_list args;
	char	*message;

	va_start(args);
	(void) fflush(stdout);
	(void) fprintf(stderr, "make: Fatal error: ");
	message = va_arg(args, char *);
	(void) vfprintf(stderr, message, args);
	(void) fprintf(stderr, "\n");
	va_end(args);
	if (report_pwd) {
		(void) fprintf(stderr,
			       "Current working directory %s\n",
			       get_current_path());
	}
	(void) fflush(stderr);
	if (fatal_in_progress) {
		_exit(1);
	}
	fatal_in_progress = true;
#ifdef PARALLEL
	/* Let all parallel children finish */
	if (parallel_process_cnt > 0) {
		(void) fprintf(stderr,
			       "Waiting for %d %s to finish\n",
			       parallel_process_cnt,
			       parallel_process_cnt == 1 ?
			       "child" : "children");
		(void) fflush(stderr);
	}
	while (parallel_process_cnt > 0) {
		await_parallel(true);
		finish_children(false);
	}
#endif PARALLEL
	exit(1);
}

/*
 *	warning(format, args...)
 *
 *	Print a message and continue.
 *
 *	Parameters:
 *		format		printf type format string
 *		args		Arguments to match the format
 *
 *	Global variables used:
 *		report_pwd	Should we report the current path?
 */
/*VARARGS*/
void
warning(va_alist)
	va_dcl
{
	va_list args;
	char	*message;

	va_start(args);
	(void) fflush(stdout);
	(void) fprintf(stderr, "make: Warning: ");
	message = va_arg(args, char *);
	(void) vfprintf(stderr, message, args);
	(void) fprintf(stderr, "\n");
	va_end(args);
	if (report_pwd) {
		(void) fprintf(stderr,
			       "Current working directory %s\n",
			       get_current_path());
	}
	(void) fflush(stderr);
}

/*
 *	errmsg(errnum)
 *
 *	Return the error message for a system call error
 *
 *	Return value:
 *				An error message string
 *
 *	Parameters:
 *		errnum		The number of the error we want to describe
 *
 *	Global variables used:
 *		sys_errlist	A vector of error messages
 *		sys_nerr	The size of sys_errlist
 */
char *
errmsg(errnum)
	int			errnum;
{
	extern int		sys_nerr;
	extern char 	       *sys_errlist[];
	char			*errbuf;

	if (errnum < 0 || errnum > sys_nerr) {
		errbuf = getmem(6+1+11+1);
		(void) sprintf(errbuf, "Error %d", errnum);
		return errbuf;
	} else {
		return sys_errlist[errnum];
	}
}

/*
 *	time_to_string(time)
 *
 *	Take a numeric time value and produce
 *	a proper string representation.
 *
 *	Return value:
 *				The string representation of the time
 *
 *	Parameters:
 *		time		The time we need to translate
 *
 *	Global variables used:
 */
char *
time_to_string(time)
	time_t			time;
{
	char			*string;

	if (time == (int) file_doesnt_exist) {
		return "File does not exist";
	}
	if (time == (int) file_max_time) {
		return "Younger than any file";
	}
	string = ctime(&time);
	string[strlen(string)-1] = (int) nul_char;
	return strdup(string);
}

/*
 *	get_current_path()
 *
 *	Stuff current_path with the current path if it isnt there already.
 *
 *	Parameters:
 *
 *	Global variables used:
 */
char *
get_current_path()
{
	char			pwd[MAXPATHLEN];
static	char			*current_path;

	if (current_path == NULL) {
		(void) getwd(pwd);
		if (pwd[0] == (int) nul_char) {
			pwd[0] = (int) slash_char;
			pwd[1] = (int) nul_char;
		}
		current_path = strdup(pwd);
	}
	return current_path;
}

/*****************************************
 *
 *	Make internal state dumping
 *
 *	This is a set  of routines for dumping the internal make state
 *	Used for the -p option
 */

/*
 *	dump_make_state()
 *
 *	Dump make's internal state to stdout
 *
 *	Parameters:
 *
 *	Global variables used:
 *		default_rule		Points to the .DEFAULT rule
 *		default_rule_name	The Name ".DEFAULT", printed
 *		default_target_to_build	The first target to print
 *		dot_keep_state		The Name ".KEEP_STATE", printed
 *		hashtab			The make hash table for Name blocks
 *		ignore_errors		Was ".IGNORE" seen in makefile?
 *		ignore_name		The Name ".IGNORE", printed
 *		keep_state		Was ".KEEP_STATE" seen in makefile?
 *		percent_list		The list of % rules
 *		precious		The Name ".PRECIOUS", printed
 *		sccs_get_name		The Name ".SCCS_GET", printed
 *		sccs_get_rule		Points to the ".SCCS_GET" rule
 *		silent			Was ".SILENT" seen in makefile?
 *		silent_name		The Name ".SILENT", printed
 *		suffixes		The suffix list from ".SUFFIXES"
 *		suffixes_name		The Name ".SUFFIX", printed
 */
void
dump_make_state()
{
	register int		n;
	register Name		p;
	register Property	prop;
	register Dependency	dep;
	register Cmd_line	rule;
	Percent			percent;

	/* Default target */
	if (default_target_to_build != NULL) {
		print_rule(default_target_to_build);
	}
	(void) printf("\n");

	/* .DEFAULT */
	if (default_rule != NULL) {
		(void) printf("%s:\n", default_rule_name->string);
		for (rule = default_rule; rule != NULL; rule = rule->next) {
			(void) printf("\t%s\n", rule->command_line->string);
		}
	}

	/* .IGNORE */
	if (ignore_errors) {
		(void) printf("%s:\n", ignore_name->string);
	}

	/* .KEEP_STATE: */
	if (keep_state) {
		(void) printf("%s:\n\n", dot_keep_state->string);
	}

	/* .PRECIOUS */
	(void) printf("%s:", precious->string);
	for (n = hashsize - 1; n >= 0; n--) {
		for (p = hashtab[n]; p != NULL; p = p->next) {
			if (p->stat.is_precious) {
				(void) printf(" %s", p->string);
			}
		}
	}
	(void) printf("\n");

	/* .SCCS_GET */
	if (sccs_get_rule != NULL) {
		(void) printf("%s:\n", sccs_get_name->string);
		for (rule = sccs_get_rule; rule != NULL; rule = rule->next) {
			(void) printf("\t%s\n", rule->command_line->string);
		}
	}

	/* .SILENT */
	if (silent) {
		(void) printf("%s:\n", silent_name->string);
	}

	/* .SUFFIXES: */
	(void) printf("%s:", suffixes_name->string);
	for (dep = suffixes; dep != NULL; dep = dep->next) {
		(void) printf(" %s", dep->name->string);
		build_suffix_list(dep->name);
	}
	(void) printf("\n\n");

	/* % rules */
	for (percent = percent_list;
	     percent != NULL;
	     percent = percent->next) {
		(void) printf("%s%%%s:\t%s%%%s\n",
			      percent->target_prefix->string,
			      percent->target_suffix->string,
			      percent->source_prefix->string,
			      percent->source_suffix->string);
		for (rule = percent->command_template;
		     rule != NULL;
		     rule = rule->next) {
			(void) printf("\t%s\n", rule->command_line->string);
		}
	}

	/* Suffix rules */
	for (n = hashsize - 1; n >= 0; n--) {
		for (p = hashtab[n]; p != NULL; p = p->next) {
			if (p->string[0] == (int) period_char) {
				print_rule(p);
			}
		}
	}

	/* Macro assignments */
	for (n = hashsize - 1; n >= 0; n--) {
		for (p = hashtab[n]; p != NULL; p = p->next) {
			if (((prop = get_prop(p->prop, macro_prop)) != NULL) &&
			    (prop->body.macro.value != NULL)) {
				(void) printf("%s", p->string);
				print_value(prop->body.macro.value,
					    prop->body.macro.daemon);
			}
		}
	}
	(void) printf("\n");

	/* Conditional macro assignments */
	for (n = hashsize - 1; n >= 0; n--) {
		for (p = hashtab[n]; p != NULL; p = p->next) {
			for (prop = get_prop(p->prop, conditional_prop);
			     prop != NULL;
			     prop = get_prop(prop->next, conditional_prop)) {
				(void) printf("%s := %s",
					      p->string,
					      prop->body.conditional.name->
					      string);
				print_value(prop->body.conditional.value,
					    no_daemon);
			}
		}
	}
	(void) printf("\n");

	/* All other dependencies */
	for (n = hashsize - 1; n >= 0; n--) {
		for (p = hashtab[n]; p != NULL; p = p->next) {
			if (p->colons != no_colon) {
				print_rule(p);
			}
		}
	}
	(void) printf("\n");
}

/*
 *	print_value(value, daemon)
 *
 *	Print the value of a macro
 *
 *	Parameters:
 *		value		The macro value to print
 *		daemon		Indicates how value should be printed
 *
 *	Global variables used:
 */
static void
print_value(value, daemon)
	register Name		value;
	Daemon			daemon;
{
	Chain			cp;

	if (value == NULL) {
		(void) printf(" =\n");
	} else {
		switch (daemon) {
		case no_daemon:
			(void) printf("= %s\n", value->string);
			break;
		case chain_daemon:
			for (cp = (Chain) value; cp != NULL; cp = cp->next) {
				(void) printf(cp->next == NULL ? "%s" : "%s ",
					      cp->name->string);
			}
			(void) printf("\n");
			break;
		}
	}
}

/*
 *	print_rule(target)
 *
 *	Print the rule for one target
 *
 *	Parameters:
 *		target		Target we print rule for
 *
 *	Global variables used:
 */
static void
print_rule(target)
	register Name		target;
{
	register Cmd_line	rule;
	register Property	line;
	register Dependency	dependency;

	if (target->dependency_printed ||
	    ((line = get_prop(target->prop, line_prop)) == NULL) ||
	    ((line->body.line.command_template == NULL) &&
	     (line->body.line.dependencies == NULL))) {
		return;
	}
	target->dependency_printed = true;

	(void) printf("%s:", target->string);

	for (dependency = line->body.line.dependencies;
	     dependency != NULL;
	     dependency = dependency->next) {
		(void) printf(" %s", dependency->name->string);
	}

	(void) printf("\n");

	for (rule = line->body.line.command_template;
	     rule != NULL;
	     rule = rule->next) {
		(void) printf("\t%s\n", rule->command_line->string);
	}
}

/*****************************************
 *
 *	main() support
 */

/*
 *	setup_char_semantics()
 *
 *	Load the vector funny[] with lexical markers
 *
 *	Parameters:
 *
 *	Global variables used:
 *		funny		The vector of character semantics that we set
 */
void
setup_char_semantics()
{
	char		*cp;

	for (cp = "=@-?!"; *cp; ++cp) {
		char_semantics[*cp] |= (int) command_prefix_sem;
	}
	char_semantics[(int) dollar_char] |= (int) dollar_sem;
	for (cp = "#|=^();&<>*?[]:$`'\"\\\n"; *cp; ++cp) {
		char_semantics[*cp] |= (int) meta_sem;
	}
	char_semantics[(int) percent_char] |= (int) percent_sem;
	for (cp = "@*<%?"; *cp; ++cp) {
		char_semantics[*cp] |= (int) special_macro_sem;
	}
	for (cp = "?[*"; *cp; ++cp) {
		char_semantics[*cp] |= (int) wildcard_sem;
	}
	char_semantics[(int) colon_char] |= (int) colon_sem;
	char_semantics[(int) parenleft_char] |= (int) parenleft_sem;
}

/*
 *	load_cached_names()
 *
 *	Load the vector of cached names
 *
 *	Parameters:
 *
 *	Global variables used:
 *		Many many pointers to Name blocks.
 */
void
load_cached_names()
{
	char		*cp;
	Name		dollar;

	/* Load the cached_names struct */
	built_last_make_run = GETNAME(".BUILT_LAST_MAKE_RUN", FIND_LENGTH);
	c_at = GETNAME("@", FIND_LENGTH);
	conditionals = GETNAME(" *conditionals* ", FIND_LENGTH);
	/*
	 * A version of make was released with NSE 1.0 that used
	 * VERSION-1.1 but this version is identical to VERSION-1.0.
	 * The version mismatch code makes a special case for this
	 * situation.  If the version number is changed from 1.0
	 * it should go to 1.2.
	 */
	current_make_version = GETNAME("VERSION-1.0", FIND_LENGTH);
	default_rule_name = GETNAME(".DEFAULT", FIND_LENGTH);
	dollar = GETNAME("$", FIND_LENGTH);
	done = GETNAME(".DONE", FIND_LENGTH);
	dot = GETNAME(".", FIND_LENGTH);
	dot_keep_state = GETNAME(".KEEP_STATE", FIND_LENGTH);
	empty_name = GETNAME("", FIND_LENGTH);
	force = GETNAME(" FORCE", FIND_LENGTH);
	host_arch = GETNAME("HOST_ARCH", FIND_LENGTH);
	ignore_name = GETNAME(".IGNORE", FIND_LENGTH);
	init = GETNAME(".INIT", FIND_LENGTH);
	make_state = GETNAME(".make.state", FIND_LENGTH);
	makeflags = GETNAME("MAKEFLAGS", FIND_LENGTH);
	make_version = GETNAME(".MAKE_VERSION", FIND_LENGTH);
	no_parallel_name = GETNAME(".NO_PARALLEL", FIND_LENGTH);
	not_auto = GETNAME(".NOT_AUTO", FIND_LENGTH);
	parallel_name = GETNAME(".PARALLEL", FIND_LENGTH);
	path_name = GETNAME("PATH", FIND_LENGTH);
	plus = GETNAME("+", FIND_LENGTH);
	precious = GETNAME(".PRECIOUS", FIND_LENGTH);
	query = GETNAME("?", FIND_LENGTH);
	recursive_name = GETNAME(".RECURSIVE", FIND_LENGTH);
	remote_command_name = GETNAME("REMOTE_COMMAND", FIND_LENGTH);
	sccs_get_name = GETNAME(".SCCS_GET", FIND_LENGTH);
	shell_name = GETNAME("SHELL", FIND_LENGTH);
	silent_name = GETNAME(".SILENT", FIND_LENGTH);
	suffixes_name = GETNAME(".SUFFIXES", FIND_LENGTH);
	sunpro_dependencies = GETNAME(SUNPRO_DEPENDENCIES, FIND_LENGTH);
	target_arch = GETNAME("TARGET_ARCH", FIND_LENGTH);
	virtual_root = GETNAME("VIRTUAL_ROOT", FIND_LENGTH);
	vpath_name = GETNAME("VPATH", FIND_LENGTH);
	wait_name = GETNAME(".WAIT", FIND_LENGTH);

	wait_name->state = build_ok;

	/* Mark special targets so that the reader treats them properly */
	built_last_make_run->special_reader = built_last_make_run_special;
	default_rule_name->special_reader = default_special;
	dot_keep_state->special_reader = keep_state_special;
	ignore_name->special_reader = ignore_special;
	make_version->special_reader = make_version_special;
	no_parallel_name->special_reader = no_parallel_special;
	parallel_name->special_reader = parallel_special;
	precious->special_reader = precious_special;
	sccs_get_name->special_reader = sccs_get_special;
	silent_name->special_reader = silent_special;
	suffixes_name->special_reader = suffixes_special;

	/* The value of $$ is $ */
	(void) SETVAR(dollar, dollar, false);
	dollar->dollar = false;

	/* Set the value of $(SHELL) */
	(void) SETVAR(shell_name, GETNAME("/bin/sh", FIND_LENGTH), false);

	/* Use " FORCE" to simulate a FRC dependency for :: type */
	/* targets with no dependencies */
	(void) append_prop(force, line_prop);
	force->stat.time = (int) file_max_time;

	/* Make sure VPATH is defined before current dir is read */
	if ((cp = getenv(vpath_name->string)) != NULL) {
		(void) SETVAR(vpath_name,
			      GETNAME(cp, FIND_LENGTH),
			      false);
	}

	/* Check if there is NO PATH variable. If not we construct one. */
	if (getenv(path_name->string) == NULL) {
		vroot_path = NULL;
		add_dir_to_path(".", &vroot_path, -1);
		add_dir_to_path("/bin", &vroot_path, -1);
		add_dir_to_path("/usr/bin", &vroot_path, -1);
	}
}

/*
 *	setup_interrupt(handler)
 *
 *	This routine saves the oroginal interrupt handler pointers
 *
 *	Parameters:
 *
 *	Static variables used:
 *		sigivalue	The original signal handler
 *		sigqvalue	The original signal handler
 */
void
setup_interrupt()
{
	sigivalue = signal(SIGINT, (void (*) ()) SIG_IGN);
	sigqvalue = signal(SIGQUIT, (void (*) ()) SIG_IGN);
	enable_interrupt(handle_interrupt);
}

/*
 *	enable_interrupt(handler)
 *
 *	This routine sets a new interrupt handler for the signals makesh
 *	want to deal with.
 *
 *	Parameters:
 *		handler		The function installed as interrupt handler
 *
 *	Static variables used:
 *		sigivalue	The original signal handler
 *		sigqvalue	The original signal handler
 */
void
enable_interrupt(handler)
	register void		(*handler)();

{
	if (sigivalue != SIG_IGN) {
		(void) signal(SIGINT, handler);
	}
	if (sigqvalue != SIG_IGN) {
		(void) signal(SIGQUIT, handler);
	}
}

