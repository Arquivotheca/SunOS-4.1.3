#ident "@(#)read2.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	read.c
 *
 *	This file contains the makefile reader.
 */

/*
 * Included files
 */
#include "defs.h"
#include <fcntl.h>
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
static	Boolean		built_last_make_run_seen;

/*
 * File table of contents
 */
extern	Name_vector	enter_name();
extern	Name_vector	enter_member_name();
extern	Name		normalize_name();
extern	void		find_target_groups();
extern	void		enter_dependencies();
extern	void		enter_dependency();
extern	void		enter_percent();
extern	void		special_reader();
extern	void		read_suffixes_list();
extern	void		make_relative();
extern	void		print_rule();
extern	void		enter_conditional();
extern	void		enter_equal();
extern	void		sh_transform();
extern	void		fatal_reader();


/*
 *	enter_name(string, tail_present, string_start, string_end,
 *	      current_names, extra_names, target_group_seen)
 *
 *	Take one string and enter it as a name. The string is passed in
 *	two parts. A make string and possibly a C string to append to it.
 *	The result is stuffed in the vector current_names.
 *	extra_names points to a vector that is used if current_names overflows.
 *	This is allocad in the calling routine.
 *	Here we handle the "lib.a[members]" notation.
 *
 *	Return value:
 *				The name vector that was used
 *
 *	Parameters:
 *		tail_present	Indicates if both C and make string was passed
 *		string_start	C string
 *		string_end	Pointer to char after last in C string
 *		string		make style string with head of name
 *		current_names	Vector to deposit the name in
 *		extra_names	Where to get next name vector if we run out
 *		target_group_seen Pointer to boolean that is set if "+" is seen
 *
 *	Global variables used:
 *		makefile_type	When we read a report file we normalize paths
 *		plus		Points to the Name "+"
 */
Name_vector
enter_name(string, tail_present, string_start, string_end,
	      current_names, extra_names, target_group_seen)
	String			string;
	Boolean			tail_present;
	register char		*string_start;
	register char		*string_end;
	Name_vector		current_names;
	Name_vector		*extra_names;
	Boolean			*target_group_seen;
{
	Name			name;
	register char		*cp;
	char			ch;

	/* If we were passed a separate tail of the name we append it to the */
	/* make string with the rest of it */
	if (tail_present) {
		append_string(string_start, string, string_end - string_start);
		string_start = string->buffer.start;
		string_end = string->text.p;
	}
	/* Strip "./" from the head of the name */
	if ((string_start[0] == (int) period_char) &&
	    (string_start[1] == (int) slash_char)) {
		string_start += 2;
	}
	ch = *string_end;
	*string_end = (int) nul_char;
	/* Check if there are any parens that are not prefixed with "$" */
	/* or any "[" */
	/* If there are we have to deal with the "lib.a[members]" format */
	for (cp = strchr(string_start, (int) parenleft_char);
	     cp != NULL;
	     cp = strchr(cp + 1, (int) parenleft_char)) {
		if (*(cp - 1) != (int) dollar_char) {
			*string_end = ch;
			return enter_member_name(string_start,
						 cp,
						 string_end,
						 current_names,
						 extra_names);
		}
	}
	*string_end = ch;
	/* If the current_names vector is full we patch in the one from */
	/* extra_names */
	if (current_names->used == VSIZEOF(current_names->names)) {
		if (current_names->next != NULL) {
			current_names = current_names->next;
		} else {
			current_names->next = *extra_names;
			*extra_names = NULL;
			current_names = current_names->next;
			current_names->used = 0;
			current_names->next = NULL;
		}
	}
	current_names->target_group[current_names->used] = NULL;
	/* Remove extra ../ constructs if we are reading from a report file */
	if (makefile_type == reading_cpp_file) {
		name = normalize_name(string_start, string_end - string_start);
	} else {
		name = GETNAME(string_start, string_end - string_start);
	}
	/* Internalize the name. Detect the name "+" (target group here) */
	current_names->names[current_names->used++] = name;
	if (name == plus) {
		*target_group_seen = true;
	}
	if (tail_present && string->free_after_use) {
		retmem(string->buffer.start);
	}
	return current_names;
}

/*
 *	enter_member_name(lib_start, member_start, string_end,
 *		  current_names, extra_names)
 *
 *	A string has been found to contain member names.
 *	(The "lib.a[members]" and "lib.a(members)" notation)
 *	Handle it pretty much as enter_name() does for simple names.
 *
 *	Return value:
 *				The name vector that was used
 *
 *	Parameters:
 *		lib_start	Points to the of start of "lib.a(member.o)"
 *		member_start	Points to "member.o" from above string.
 *		string_end	Points to char after last of above string.
 *		current_names	Vector to deposit the name in
 *		extra_names	Where to get next name vector if we run out
 *
 *	Global variables used:
 */
static Name_vector
enter_member_name(lib_start, member_start, string_end,
		  current_names, extra_names)
	register char		*lib_start;
	register char		*member_start;
	register char		*string_end;
	Name_vector		current_names;
	Name_vector		*extra_names;
{
	register Boolean	entry = false;
	char			buffer[STRING_BUFFER_LENGTH];
	Name			lib;
	Name			member;
	Name			name;
	Property		prop;
	char			*memberp;
	char			*q;
	char			chr;
	register int		paren_count;
	register Boolean	has_dollar;
	register char		*cq;
	Name			long_member_name = NULL;

	/* Internalize the name of the library */
	lib = GETNAME(lib_start, member_start - lib_start);
	lib->is_member = true;
	member_start++;
	if (*member_start == (int) parenleft_char) {
		/* This is really the "lib.a((entries))" format */
		entry = true;
		member_start++;
	}
	/* Move the library name to the buffer where we intend to build the */
	/* "lib.a(member)" for each member */
	(void) strncpy(buffer, lib_start, member_start-lib_start);
	memberp = buffer + (member_start-lib_start);
	while (1) {
		long_member_name = NULL;
		/* Skip leading spaces */
		for (;
		     (member_start < string_end) && isspace(*member_start);
		     member_start++);
		/* Find the end of the member name. Allow nested (). Detect $*/
		for (cq = memberp, has_dollar = false, paren_count = 0;
		     (member_start < string_end) &&
		     ((*member_start != (int) parenright_char) ||
		      (paren_count > 0)) &&
		     !isspace(*member_start);
		     *cq++ = *member_start++) {
			switch (*member_start) {
			case parenleft_char:
				paren_count++;
				break;
			case parenright_char:
				paren_count--;
				break;
			case dollar_char:
				has_dollar = true;
			}
		}
		/* Internalize the member name */
		member = GETNAME(memberp, cq - memberp);
		chr = *cq;
		*cq = 0;
		if ((q = strrchr(memberp, (int) slash_char)) == NULL) {
			q = memberp;
		}
		if ((cq - q > (int) ar_member_name_len) &&
		    !has_dollar) {
			*cq++ = (int) parenright_char;
			if (entry) {
				*cq++ = (int) parenright_char;
			}
			long_member_name = GETNAME(buffer, cq - buffer);
			cq = q + (int) ar_member_name_len;
		}
		*cq = chr;
		*cq++ = (int) parenright_char;
		if (entry) {
			*cq++ = (int) parenright_char;
		}
		/* Internalize the "lib.a(member)" notation for this member */
		name = GETNAME(buffer, cq - buffer);
		name->is_member = lib->is_member;
		if (long_member_name != NULL) {
			prop = append_prop(name, long_member_name_prop);
			name->has_long_member_name = true;
			prop->body.long_member_name.member_name =
			  long_member_name;
		}
		/* And add the member prop */
		prop = append_prop(name, member_prop);
		prop->body.member.library = lib;
		if (entry) {
			/* "lib.a((entry))" notation */
			prop->body.member.entry = member;
			prop->body.member.member = NULL;
		} else {
			/* "lib.a(member)" Notation */
			prop->body.member.entry = NULL;
			prop->body.member.member = member;
		}
		/* Handle overflow of current_names */
		if (current_names->used == VSIZEOF(current_names->names)) {
			if (current_names->next != NULL) {
				current_names = current_names->next;
			} else {
				if (*extra_names == NULL) {
					current_names =
					  current_names->next =
					    ALLOC(Name_vector);
				} else {
					current_names =
					  current_names->next =
					    *extra_names;
					*extra_names = NULL;
				}
				current_names->used = 0;
				current_names->next = NULL;
			}
		}
		current_names->target_group[current_names->used] = NULL;
		current_names->names[current_names->used++] = name;
		while (isspace(*member_start)) {
			member_start++;
		}
		/* Check if there are more members */
		if ((*member_start == (int) parenright_char) ||
		    (member_start >= string_end)) {
			return current_names;
		}
	}
	/* NOTREACHED */
}

/*
 *	normalize_name(name_string, length)
 *
 *	Take a namestring and remove redundant ../, // and ./ constructs
 *
 *	Return value:
 *				The normalized name
 *
 *	Parameters:
 *		name_string	Path string to normalize
 *		length		Length of that string
 *
 *	Global variables used:
 *		dot		The Name ".", compared against
 *		dotdot		The Name "..", compared against
 */
static Name
normalize_name(name_string, length)
	register char		*name_string;
	register int		length;
{
	register char		*string = alloca(length + 1);
	register char		*cdp;
	char			*current_component;
	Name			name;
	register int		count;
static	Name			dotdot;

	if (dotdot == NULL) {
		dotdot = GETNAME("..", FIND_LENGTH);
	}

	/* Copy string removing ./ and // */
	/* First strip leading ./ */
	while ((length > 1) &&
	       (name_string[0] == (int) period_char) &&
	       (name_string[1] == (int) slash_char)) {
		name_string += 2;
		length -= 2;
		while ((length > 0) && (name_string[0] == (int) slash_char)) {
			name_string++;
			length--;
		}
	}
	/* Then copy the rest of the string removing /./ & // */
	cdp = string;
	while (length > 0) {
		if (((length > 2) &&
		     (name_string[0] == (int) slash_char) &&
		     (name_string[1] == (int) period_char) &&
		     (name_string[2] == (int) slash_char)) ||
		    ((length == 2) &&
		     (name_string[0] == (int) slash_char) &&
		     (name_string[1] == (int) period_char))) {
			name_string += 2;
			length -= 2;
			continue;
		}
		if ((length > 1) &&
		    (name_string[0] == (int) slash_char) &&
		    (name_string[1] == (int) slash_char)) {
			name_string++;
			length--;
			continue;
		}
		*cdp++ = *name_string++;
		length--;
	}
	*cdp = (int) nul_char;
	/* Now scan for <name>/../ and remove such combinations iff <name> */
	/* is not another .. */
	/* Each time something is removed the whole process is restarted */
 removed_one:
	name_string = string;
	current_component =
	  cdp =
	    string =
	      alloca((length = strlen(name_string)) + 1);
	while (length > 0) {
		if (((length > 3) &&
		     (name_string[0] == (int) slash_char) &&
		     (name_string[1] == (int) period_char) &&
		     (name_string[2] == (int) period_char) &&
		     (name_string[3] == (int) slash_char)) ||
		    ((length == 3) &&
		     (name_string[0] == (int) slash_char) &&
		     (name_string[1] == (int) period_char) &&
		     (name_string[2] == (int) period_char))) {
			/* Positioned on the / that starts a /.. sequence */
			if (((count = cdp - current_component) != 0) &&
			    (exists(name = GETNAME(current_component, count)) >
			     0) &&
			    (name != dotdot)) {
				cdp = current_component;
				name_string += 3;
				length -= 3;
				if (length > 0) {
					name_string++;	/* skip slash */
					length--;
					while (length > 0) {
						*cdp++ = *name_string++;
						length--;
					}
				}
				*cdp = (int) nul_char;
				goto removed_one;
			}
		}
		if ((*cdp++ = *name_string++) == (int) slash_char) {
			current_component = cdp;
		}
		length--;
	}
	*cdp = (int) nul_char;
	if (string[0] == (int) nul_char) {
		return dot;
	}
	return GETNAME(string, FIND_LENGTH);
}

/*
 *	find_target_groups(target_list)
 *
 *	If a "+" was seen when the target list was scanned we need to extract
 *	the groups. Each target in the name vector that is a member of a
 *	group gets a pointer to a chain of all the members stuffed in its
 *	target_group vector slot
 *
 *	Parameters:
 *		target_list	The list of targets that contains "+"
 *
 *	Global variables used:
 *		plus		The Name "+", compared against
 */
void
find_target_groups(target_list)
	register Name_vector	target_list;
{
	register Chain		target_group = NULL;
	register Chain		tail_target_group = NULL;
	register Name		*next;
	register Boolean	clear_target_group = false;
	register int		i;

	/* Scan the list of targets */
	for (; target_list != NULL; target_list = target_list->next) {
		for (i = 0; i < target_list->used; i++) {
			if (target_list->names[i] != NULL) {
				/* If the previous target terminated a group */
				/* we flush the pointer to that member chain */
				if (clear_target_group) {
					clear_target_group = false;
					target_group = NULL;
				}
				/* Pick up a pointer to the cell with */
				/* the next target */
				if (i + 1 != target_list->used) {
					next = &target_list->names[i + 1];
				} else {
					next = (target_list->next != NULL) ?
					  &target_list->next->names[0] : NULL;
				}
/* We have four states here */
/*	0:	No target group started and next element is not a "+" */
/*		This is not interesting */
/*	1:	A target group is being built and the next element is not "+"*/
/*		This terminates the group */
/*	2:	No target group started and the next member is "+" */
/*		This is the first target in a group */
/*	3:	A target group started and the next member is a "+" */
/*		The group continues */
				switch ((target_group ? 1 : 0) +
					(next && (*next == plus) ?
					 2 : 0)) {
				case 0:	/* Not target_group */
					break;
				case 1:	/* Last group member */
					/* We need to keep this pointer so */
					/* we can stuff it for last member */
					clear_target_group = true;
					/* fall into */
				case 3:	/* Middle group member */
					/* Add this target to the */
					/* current chain */
					tail_target_group->next = ALLOC(Chain);
					tail_target_group =
					  tail_target_group->next;
					tail_target_group->next = NULL;
					tail_target_group->name =
					  target_list->names[i];
					break;
				case 2:	/* First group member */
					/* Start a new chain */
					target_group =
					  tail_target_group =
					    ALLOC(Chain);
					target_group->next = NULL;
					target_group->name =
					  target_list->names[i];
					break;
				}
				/* Stuff the current chain, if any, in the */
				/* targets group slot */
				target_list->target_group[i] = target_group;
				if ((next != NULL) &&
				    (*next == plus)) {
					*next = NULL;
				}
			}
		}
	}
}

/*
 *	enter_dependencies(target, target_group, depes, command, separator)
 *
 *	Take one target and a list of dependencies and process the whole thing.
 *	The target might be special in some sense in which case that is handled
 *
 *	Parameters:
 *		target		The target we want to enter
 *		target_group	Non-NULL if target is part of a group this time
 *		depes		A list of dependencies for the target
 *		command		The command the target should be entered with
 *		separator	Indicates if this is a ":" or a "::" rule
 *
 *	Static variables used:
 *		built_last_make_run_seen If the previous target was
 *					.BUILT_LAST_MAKE_RUN we say to rewrite
 *					the state file later on
 *
 *	Global variables used:
 *		command_changed	Set to indicate if .make.state needs rewriting
 *		default_target_to_build Set to the target if reading makefile
 *					and this is the first regular target
 *		force		The Name " FORCE", used with "::" targets
 *		makefile_type	We do different things for makefile vs. report
 *		not_auto	The Name ".NOT_AUTO", compared against
 *		recursive_name	The Name ".RECURSIVE", compared against
 *		temp_file_number Used to figure out when to clear stale
 *					automatic dependencies
 *		trace_reader	Indicates that we should echo stuff we read
 */
void
enter_dependencies(target, target_group, depes, command, separator)
	register Name		target;
	Chain			target_group;
	register Name_vector	depes;
	register Cmd_line	command;
	register Separator	separator;
{
	register int		i;
	register Property	line;
	Name			name;
	Name			directory;
	char			*namep;
	Dependency		dp;
	Dependency		*dpp;
	Property		line2;
	char			relative[MAXPATHLEN];
	register int		recursive_state;
	Boolean			register_as_auto;
	Boolean			not_auto_found;
	char			*slash;
	Name_vector		nvp;

	/* If we saw it in the makefile it must be a file */
	target->stat.is_file = true;
	/* Make sure that we use dependencies entered for makefiles */
	target->state = build_dont_know;
	/* If the target is special we delegate the processing */
	if (target->special_reader != no_special) {
		special_reader(target, depes, command);
		return;
	}

	/* Check if this is a "a%b : x%y" type rule */
	if (target->percent) {
		enter_percent(target, depes, command);
		return;
	}
	/* Check if this is a .RECURSIVE line */
	if ((depes->used >= 3) &&
	    (depes->names[0] == recursive_name)) {
		target->has_recursive_dependency = true;
		depes->names[0] = NULL;
		recursive_state = 0;
		dp = NULL;
		dpp = &dp;
		/* Read the dependencies. They are "<directory> <target-made>*/
		/* <makefile>*" */
		for (; depes != NULL; depes = depes->next) {
			for (i = 0; i < depes->used; i++) {
				if (depes->names[i] != NULL) {
					switch (recursive_state++) {
					case 0:	/* Directory */
						make_relative(depes->names[i]->
							      string,
							      relative);
						directory =
						  GETNAME(relative,
							  FIND_LENGTH);
						break;
					case 1:	/* Target */
						name = depes->names[i];
						break;
					default:	/* Makefiles */
						*dpp = ALLOC(Dependency);
						(*dpp)->next = NULL;
						(*dpp)->name = depes->names[i];
						(*dpp)->automatic = false;
						(*dpp)->stale = false;
						(*dpp)->built = false;
						dpp = &((*dpp)->next);
						break;
					}
				}
			}
		}
		/* Check if this recursion already has been reported else */
		/* enter the recursive prop for the target */
		/* The has_built flag is used to tell if this .RECURSIVE */
		/* was discovered from this run (read from a tmp file) */
		/* or was from discovered from the original .make.state */
		/* file */
		for (line = get_prop(target->prop, recursive_prop);
		     line != NULL;
		     line = get_prop(line->next, recursive_prop)) {
			if ((line->body.recursive.directory == directory) &&
			    (line->body.recursive.target == name)) {
				line->body.recursive.makefiles = dp;
				line->body.recursive.has_built = 
				  (Boolean)
				    (makefile_type == reading_cpp_file);
				return;
			}
		}
		line2 = append_prop(target, recursive_prop);
		line2->body.recursive.directory = directory;
		line2->body.recursive.target = name;
		line2->body.recursive.makefiles = dp;
		line2->body.recursive.has_built = 
		    (Boolean) (makefile_type == reading_cpp_file);
		return;
	}
	/* If this is the first target that doesnt start with a "." in the */
	/* makefile we remember that */
	if ((makefile_type == reading_makefile) &&
	    (default_target_to_build == NULL) &&
	    ((target->string[0] != (int) period_char) ||
	     strchr(target->string, (int) slash_char))) {
		default_target_to_build = target;
	}
	/* Check if the line is ":" or "::" */
	if (makefile_type == reading_makefile) {
		if (target->colons == no_colon) {
			target->colons = separator;
		} else {
			if (target->colons != separator) {
				fatal_reader(":/:: conflict for target `%s'",
					     target->string);
			}
		}
		if (target->colons == two_colon) {
			if (depes->used == 0) {
				/* If this is a "::" type line with no */
				/* dependencies we add one "FRC" type */
				/* dependency for free */
				depes->used = 1; /* Force :: targets with no
						  * depes to always run */
				depes->names[0] = force;
			}
			/* Do not delete "::" type targets when interrupted */
			target->stat.is_precious = true;
			/* Build a synthetic target "<number>%target" */
			/* for "target" */
			namep = getmem((int) (target->hash.length + 10));
			slash = strrchr(target->string, (int) slash_char);
			if (slash == NULL) {
				(void) sprintf(namep,
					       "%d@%s",
					       target->colon_splits++,
					       target->string);
			} else {
				*slash= 0;
				(void) sprintf(namep,
					       "%s/%d@%s",
					       target->string,
					       target->colon_splits++,
					       slash+1);
				*slash= (int) slash_char;
			}
			name = GETNAME(namep, FIND_LENGTH);
			if (trace_reader) {
				(void) printf("%s:\t", target->string);
			}
			/* Make "target" depend on "<number>%target */
			line2 = maybe_append_prop(target, line_prop);
			enter_dependency(line2, name, true);
			line2->body.line.target = target;
			/* Put a prop on "<number>%target that makes */
			/* appear as "target" */
			/* when it is processed */
			maybe_append_prop(name, target_prop)->
			  body.target.target = target;
			target->is_double_colon_parent = true;
			name->is_double_colon = true;
			name->has_target_prop = true;
			if (trace_reader) {
				(void) printf("\n");
			}
			(target = name)->stat.is_file = true;
		}
	}
	/* This really is a regular dependency line. Just enter it */
	line = maybe_append_prop(target, line_prop);
	line->body.line.target = target;
	/* Depending on what kind of makefile we are reading we have to */
	/* treat things differently */
	switch (makefile_type) {
	case reading_makefile:
		/* Reading regular makefile. Just notice whether this */
		/* redefines the rule for the  target */
		if (command != NULL) {
			if (line->body.line.command_template != NULL) {
				line->body.line.command_template_redefined =
				  true;
				if (target->string[0] == (int) period_char) {
					line->body.line.command_template =
					  command;
				}
			} else {
				line->body.line.command_template = command;
			}
		} else {
			if (target->string[0] == (int) period_char) {
				line->body.line.command_template = command;
			}
		}
		break;
	case rereading_statefile:
		/* Rereading the statefile. We only enter thing that changed */
		/* since the previous time we read it */
		if (!built_last_make_run_seen) {
			return;
		}
		built_last_make_run_seen = false;
		command_changed = true;
		target->has_built = true;
	case reading_statefile:
		/* Reading the statefile for the first time. Enter the rules */
		/* as "Commands used" not "templates to use" */
		if (command != NULL) {
			line->body.line.command_used = command;
		}
		break;
	case reading_cpp_file:
		/* Reading report file from programs that reports */
		/* dependencies. If this is the first time the target is */
		/* read from this reportfile we clear all old */
		/* automatic depes */
		if (target->temp_file_number == temp_file_number) {
			break;
		}
		target->temp_file_number = temp_file_number;
		command_changed = true;
		if (line != NULL) {
			for (dpp = &line->body.line.dependencies, dp = *dpp;
			     dp != NULL;
			     dp = dp->next) {
				if (dp->automatic) {
					*dpp = dp->next;
				} else {
					dpp = &dp->next;
				}
			}
		}
		break;
	default:
		fatal_reader("Internal error. Unknown makefile type %d",
			     makefile_type);
	}
	/* A target may only be involved in one target group */
	if (line->body.line.target_group != NULL) {
		if (target_group != NULL) {
			fatal_reader("Too many target groups for target `%s'",
				     target->string);
		}
	} else {
		line->body.line.target_group = target_group;
	}

	if (trace_reader) {
		(void) printf("%s:\t", target->string);
	}
	/* Enter the dependencies */
	register_as_auto = BOOLEAN(makefile_type != reading_makefile);
	not_auto_found = false;
	for (;
	     (depes != NULL) && !not_auto_found;
	     depes = depes->next) {
		for (i = 0; i < depes->used; i++) {
			/* the dependency .NOT_AUTO signals beginning of
			 * explicit dependancies which were put at end of
			 * list in .make.state file - we stop entering
			 * dependencies at this point
			 */
			if (depes->names[i] == not_auto) {
				not_auto_found = true;
				break;
			}
			enter_dependency(line,
					 depes->names[i],
					 register_as_auto);
		}
	}
	if (trace_reader) {
		(void) printf("\n");
		print_rule(command);
	}
}

/*
 *	enter_dependency(line, depe, automatic)
 *
 *	Enter one dependency. Do not enter duplicates.
 *
 *	Parameters:
 *		line		The line block that the dependeny is
 *				entered for
 *		depe		The dependency to enter
 *		automatic	Used to set the field "automatic"
 *
 *	Global variables used:
 *		makefile_type	We do different things for makefile vs. report
 *		trace_reader	Indicates that we should echo stuff we read
 *		wait_name	The Name ".WAIT", compared against
 */
void
enter_dependency(line, depe, automatic)
	Property		line;
	register Name		depe;
	Boolean			automatic;
{
	register Dependency	dp;
	register Dependency	*insert;

	if (trace_reader) {
		(void) printf("%s ", depe->string);
	}
	/* Find the end of the list and check for duplicates */
	for (insert = &line->body.line.dependencies, dp = *insert;
	     dp != NULL;
	     insert = &dp->next, dp = *insert) {
		if ((dp->name == depe) && (depe != wait_name)) {
			if (dp->automatic) {
				dp->automatic = automatic;
			}
			dp->stale = false;
			return;
		}
	}
	/* Insert the new dependency since we couldnt find it */
	dp = *insert = ALLOC(Dependency);
	dp->name = depe;
	dp->next = NULL;
	dp->automatic = automatic;
	dp->stale = false;
	dp->built = false;
	depe->stat.is_file = true;

	if ((makefile_type == reading_makefile) &&
	    (line != NULL) &&
	    (line->body.line.target != NULL)) {
		line->body.line.target->has_regular_dependency = true;
	}
}

/*
 *	enter_percent(target, depes, command)
 *
 *	Enter "x%y : a%b" type lines
 *	% patterns are stored in four parts head and tail for target and source
 *
 *	Parameters:
 *		target		Left hand side of pattern
 *		depes		The dependency list with the rh pattern
 *		command		The command for the pattern
 *
 *	Global variables used:
 *		empty_name	The Name "", compared against
 *		percent_list	The list of all percent rules, added to
 *		trace_reader	Indicates that we should echo stuff we read
 */
static void
enter_percent(target, depes, command)
	register Name		target;
	register Name_vector	depes;
	Cmd_line		command;
{
	register Percent	result = ALLOC(Percent);
	Percent			p;
	Percent			*insert;
	register char		*cp;
	Name_vector		nvp;
	Name			source;
	Dependency		*depe_tail = &result->dependencies;
	Dependency		dp;
	int			i;

	result->next = NULL;
	result->command_template = command;

	/*
	 * Find the right hand side pattern and build depe list
	 */
	result->dependencies = NULL;
	for (source = NULL, nvp = depes; nvp != NULL; nvp = nvp->next) {
		for (i = 0; i < nvp->used; i++) {
			if (nvp->names[i]->percent) {
				if (source != NULL) {
					fatal_reader("More than one %% pattern on left hand side");
				}
				source = nvp->names[i];
			} else {
				dp = ALLOC(Dependency);
				dp->next = NULL;
				dp->name = nvp->names[i];
				dp->automatic = false;
				dp->stale = false;
				dp->built = false;
				*depe_tail = dp;
				depe_tail = &dp->next;
			}
		}
	}

	/* Find the % in the target string and split it */
	if ((cp = strchr(target->string, (int) percent_char)) != NULL) {
		result->target_prefix = GETNAME(target->string,
						cp - target->string);
		result->target_suffix =
		  GETNAME(cp + 1, (int)(target->hash.length - 1 -
					(cp - target->string)));
	} else {
		fatal_reader("Illegal pattern with %% in source but not in target");
	}

	/* Same thing for the source pattern */
	if ((source != NULL) &&
	    (cp = strchr(source->string, (int) percent_char)) != NULL) {
		result->source_prefix = GETNAME(source->string,
						cp - source->string);
		result->source_suffix =
		    GETNAME(cp + 1, (int)(source->hash.length - 1 -
					  (cp - source->string)));
		result->source_percent = true;
	} else {
		if (source == NULL) {
			if (result->dependencies == NULL) {
				result->source_prefix = empty_name;
			} else {
				result->source_prefix =
				  result->dependencies->name;
				result->dependencies =
				  result->dependencies->next;
			}
		} else {
			result->source_prefix = source;
		}
		result->source_suffix = empty_name;
		result->source_percent = false;
	}
	if ((empty_name == result->target_prefix) &&
	    (empty_name == result->target_suffix) &&
	    (empty_name == result->source_prefix) &&
	    (empty_name == result->source_suffix)) {
		fatal_reader("Illegal pattern: `%%:%%'");
	}
	/* Find the end of the percent list and append the new pattern */
	for (insert = &percent_list, p = *insert;
	     p != NULL;
	     insert = &p->next, p = *insert);
	*insert = result;

	if (trace_reader) {
		(void) printf("%s%%%s:\t%s%s%s\n",
			      result->target_prefix->string,
			      result->target_suffix->string,
			      result->source_prefix->string,
			      result->source_percent ? "%" : "",
			      result->source_suffix->string);
		print_rule(command);
	}
}

/*
 *	special_reader(target, depes, command)
 *
 *	Read the pseudo targets make knows about
 *	This handles the special targets that should not be entered as regular
 *	target/dependency sets.
 *
 *	Parameters:
 *		target		The special target
 *		depes		The list of dependencies it was entered with
 *		command		The command it was entered with
 *
 *	Static variables used:
 *		built_last_make_run_seen Set to indicate .BUILT_LAST... seen
 *
 *	Global variables used:
 *		all_parallel	Set to indicate that everything runs parallel
 *		current_make_version The Name "<current version number>"
 *		default_rule	Set when ".DEFAULT" target is read
 *		default_rule_name The Name ".DEFAULT", used for tracing
 *		dot_keep_state	The Name ".KEEP_STATE", used for tracing
 *		ignore_errors	Set if ".IGNORE" target is read
 *		ignore_name	The Name ".IGNORE", used for tracing
 *		keep_state	Set if ".KEEP_STATE" target is read
 *		no_parallel_name The Name ".NO_PARALLEL", used for tracing
 *		only_parallel	Set to indicate only some targets runs parallel
 *		parallel_name	The Name ".PARALLEL", used for tracing
 *		precious	The Name ".PRECIOUS", used for tracing
 *		sccs_get_name	The Name ".SCCS_GET", used for tracing
 *		sccs_get_rule	Set when ".SCCS_GET" target is read
 *		silent		Set when ".SILENT" target is read
 *		silent_name	The Name ".SILENT", used for tracing
 *		trace_reader	Indicates that we should echo stuff we read
 */
static void
special_reader(target, depes, command)
	Name			target;
	register Name_vector	depes;
	Cmd_line		command;
{
	register int		n;

	switch (target->special_reader) {

	case built_last_make_run_special:
		built_last_make_run_seen = true;
		break;

	case default_special:
		if (depes->used != 0) {
			warning("Illegal dependency list for target `%s'",
				target->string);
		}
		default_rule = command;
		if (trace_reader) {
			(void) printf("%s:\n",
				      default_rule_name->string);
			print_rule(command);
		}
		break;

	case ignore_special:
		if (depes->used != 0) {
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		}
		ignore_errors = true;
		if (trace_reader) {
			(void) printf("%s:\n", ignore_name->string);
		}
		break;

	case keep_state_special:
		if (depes->used != 0) {
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		}
		keep_state = true;
		if (trace_reader) {
			(void) printf("%s:\n",
				      dot_keep_state->string);
		}
		break;

	case make_version_special:
		if (depes->used != 1) {
			fatal_reader("Illegal dependency list for target `%s'",
				     target->string);
		}
		if (depes->names[0] != current_make_version) {
			/*
			 * Special case the fact that version 1.0 and 1.1
			 * are identical.
			 */
			if (!IS_EQUAL(depes->names[0]->string,
				      "VERSION-1.1") ||
			    !IS_EQUAL(current_make_version->string,
				     "VERSION-1.0")) {
				/*
				 * Version mismatches should cause the
				 * .make.state file to be skipped.
				 * This is currently not true - it is read
				 * anyway.
				 */
				warning("Version mismatch between current version `%s' and `%s'",
					current_make_version->string,
					depes->names[0]->string);
			}
		}
		break;

	case no_parallel_special:
		/* Set the no_parallel bit for all the targets on */
		/* the dependency list */
		if (depes->used == 0) {
			/* only those explicitly made parallel */
			only_parallel = true;
			all_parallel = false;
		}
		for (; depes != NULL; depes = depes->next) {
			for (n = 0; n < depes->used; n++) {
				if (trace_reader) {
					(void) printf("%s:\t%s\n",
						      no_parallel_name->string,
						      depes->names[n]->string);
				}
				depes->names[n]->no_parallel = true;
				depes->names[n]->parallel = false;
			}
		}
		break;

	case parallel_special:
		if (depes->used == 0) {
			/* everything runs in parallel */
			all_parallel = true;
			only_parallel = false;
		}
		/* Set the parallel bit for all the targets on */
		/* the dependency list */
		for (; depes != NULL; depes = depes->next) {
			for (n = 0; n < depes->used; n++) {
				if (trace_reader) {
					(void) printf("%s:\t%s\n",
						      parallel_name->string,
						      depes->names[n]->string);
				}
				depes->names[n]->parallel = true;
				depes->names[n]->no_parallel = false;
			}
		}
		break;

	case precious_special:
		/* Set the precious bit for all the targets on */
		/* the dependency list */
		for (; depes != NULL; depes = depes->next) {
			for (n = 0; n < depes->used; n++) {
				if (trace_reader) {
					(void) printf("%s:\t%s\n",
						      precious->string,
						      depes->names[n]->string);
				}
				depes->names[n]->stat.is_precious = true;
			}
		}
		break;

	case sccs_get_special:
		if (depes->used != 0) {
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		}
		sccs_get_rule = command;
		if (trace_reader) {
			(void) printf("%s:\n", sccs_get_name->string);
			print_rule(command);
		}
		break;

	case silent_special:
		if (depes->used != 0) {
			fatal_reader("Illegal dependencies for target `%s'",
				     target->string);
		}
		silent = true;
		if (trace_reader) {
			(void) printf("%s:\n", silent_name->string);
		}
		break;

	case suffixes_special:
		read_suffixes_list(depes);
		break;

	default:

		fatal_reader("Internal error: Unknown special reader");
	}
}

/*
 *	read_suffixes_list(depes)
 *
 *	Read the special list .SUFFIXES. If it is empty the old list is
 *	cleared. Else the new one is appended. Suffixes with ~ are extracted
 *	and marked.
 *
 *	Parameters:
 *		depes		The list of suffixes
 *
 *	Global variables used:
 *		hashtab		The central hashtable for Names.
 *		suffixes	The list of suffixes, set or appended to
 *		suffixes_name	The Name ".SUFFIXES", used for tracing
 *		trace_reader	Indicates that we should echo stuff we read
 */
static void
read_suffixes_list(depes)
	register Name_vector	depes;
{
	register int		n;
	register Dependency	dp;
	register Dependency	*insert_dep;
	register Name		np;
	Name			np2;
	register Boolean	first = true;

	if (depes->used == 0) {
		/* .SUFFIXES with no dependency list clears the */
		/* suffixes list */
		for (n = hashsize - 1; n >= 0; n--) {
			for (np = hashtab[n]; np != NULL; np = np->next) {
				np->with_squiggle =
				  np->without_squiggle =
				    false;
			}
		}
		suffixes = NULL;
		if (trace_reader) {
			(void) printf("%s:\n", suffixes_name->string);
		}
		return;
	}
	/* Otherwise we append to the list */
	for (; depes != NULL; depes = depes->next) {
		for (n = 0; n < depes->used; n++) {
			np = depes->names[n];
			/* Find the end of the list and check if the */
			/* suffix already has been entered */
			for (insert_dep = &suffixes, dp = *insert_dep;
			     dp != NULL;
			     insert_dep = &dp->next, dp = *insert_dep) {
				if (dp->name == np) {
					goto duplicate_suffix;
				}
			}
			if (trace_reader) {
				if (first) {
					(void) printf("%s:\t",
						      suffixes_name->string);
					first = false;
				}
				(void) printf("%s ", depes->names[n]->string);
			}
			/* If the suffix is suffixed with "~" we */
			/* strip that and mark the suffix nameblock */
			if (np->string[np->hash.length - 1] ==
			    (int) tilde_char) {
				np2 = GETNAME(np->string,
					      (int)(np->hash.length - 1));
				np2->with_squiggle = true;
				if (np2->without_squiggle) {
					continue;
				}
				np = np2;
			}
			np->without_squiggle = true;
			/* Add the suffix to the list */
			dp = *insert_dep = ALLOC(Dependency);
			insert_dep = &dp->next;
			dp->next = NULL;
			dp->name = np;
			dp->built = false;
		duplicate_suffix:;
		}
	}
	if (trace_reader) {
		(void) printf("\n");
	}
}

/*
 *	make_relative(to, result)
 *
 *	Given a file name compose a relative path name from it to the
 *	current directory.
 *
 *	Parameters:
 *		to		The path we want to make relative
 *		result		Where to put the resulting relative path
 *
 *	Global variables used:
 */
static void
make_relative(to, result)
	char			*to;
	char			*result;
{
	char			*from;
	int			ncomps;
	int			i;
	int			len;
	char			*tocomp;
	char			*cp;

	/* Check if the path already is relative */
	if (to[0] != (int) slash_char) {
		(void) strcpy(result, to);
		return;
	}

	from = get_current_path();

	/* Find the number of components in the from name. ncomp = number of */
	/* slashes + 1. */
	ncomps = 1;
	for (cp = from; *cp != (int) nul_char; cp++) {
		if (*cp == (int) slash_char) {
			ncomps++;
		}
	}

	/* See how many components match to determine how many, if any, ".."s*/
	/* will be needed. */
	result[0] = (int) nul_char;
	tocomp = to;
	while (*from != (int) nul_char && *from == *to) {
		if (*from == (int) slash_char) {
			ncomps--;
			tocomp = &to[1];
		}
		from++;
		to++;
	}

	/* Now for some special cases. Check for exact matches and for either*/
	/* name terminating exactly. */
	if (*from == (int) nul_char) {
		if (*to == (int) nul_char) {
			(void) strcpy(result, ".");
			return;
		}
		if (*to == (int) slash_char) {
			ncomps--;
			tocomp = &to[1];
		}
	} else if (*from == (int) slash_char && *to == (int) nul_char) {
		ncomps--;
		tocomp = to;
	}
	/* Add on the ".."s. */
	for (i = 0; i < ncomps; i++) {
		(void) strcat(result, "../");
	}

	/* Add on the remainder, if any, of the to name. */
	if (*tocomp == (int) nul_char) {
		len = strlen(result);
		result[len - 1] = (int) nul_char;
	} else {
		(void) strcat(result, tocomp);
	}
	return;
}

/*
 *	print_rule(command)
 *
 *	Used when tracing the reading of rules
 *
 *	Parameters:
 *		command		Command to print
 *
 *	Global variables used:
 */
static void
print_rule(command)
	register Cmd_line	command;
{
	for (; command != NULL; command = command->next) {
		(void) printf("\t%s\n", command->command_line->string);
	}
}

/*
 *	enter_conditional(target, name, value, append)
 *
 *	Enter "target := MACRO= value" constructs
 *
 *	Parameters:
 *		target		The target the macro is for
 *		name		The name of the macro
 *		value		The value for the macro
 *		append		Indicates if the assignment is appending or not
 *
 *	Global variables used:
 *		conditionals	A special Name that stores all conditionals
 *				where the target is a % pattern
 *		trace_reader	Indicates that we should echo stuff we read
 */
void
enter_conditional(target, name, value, append)
	register Name		target;
	Name			name;
	Name			value;
	register Boolean	append;
{
	register Property	conditional;
	static int		sequence;
	Name			orig_target = target;

	if (name == target_arch) {
		enter_conditional(target, virtual_root, virtual_root, false);
	}

	if (target->percent) {
		target = conditionals;
	}
	
	if (name->colon) {
		sh_transform(&name, &value);
	}

	/* Count how many conditionals we must activate before building the */
	/* target */
	if (target->percent) {
		target = conditionals;
	}

	target->conditional_cnt++;
	maybe_append_prop(name, macro_prop)->body.macro.is_conditional = true;
	/* Add the property for the target */
	conditional = append_prop(target, conditional_prop);
	conditional->body.conditional.target = orig_target;
	conditional->body.conditional.name = name;
	conditional->body.conditional.value = value;
	conditional->body.conditional.sequence = sequence++;
	conditional->body.conditional.append = append;
	if (trace_reader) {
		if (value == NULL) {
			(void) printf("%s := %s %c=\n",
				      target->string,
				      name->string,
				      append ?
				      (int) plus_char : (int) space_char);
		} else {
			(void) printf("%s := %s %c= %s\n",
				      target->string,
				      name->string,
				      append ?
				      (int) plus_char : (int) space_char,
				      value->string);
		}
	}
}

/*
 *	enter_equal(name, value, append)
 *
 *	Enter "MACRO= value" constructs
 *
 *	Parameters:
 *		name		The name of the macro
 *		value		The value for the macro
 *		append		Indicates if the assignment is appending or not
 *
 *	Global variables used:
 *		trace_reader	Indicates that we should echo stuff we read
 */
void
enter_equal(name, value, append)
	Name			name;
	Name			value;
	register Boolean	append;
{
	if (name->colon) {
		sh_transform(&name, &value);
	}
	(void) SETVAR(name, value, append);
	if (trace_reader) {
		if (value == NULL) {
			(void) printf("%s %c=\n",
				      name->string,
				      append ?
				      (int) plus_char : (int) space_char);
		} else {
			(void) printf("%s %c= %s\n",
				      name->string,
				      append ?
				      (int) plus_char : (int) space_char,
				      value->string);
		}
	}
}

/*
 *	sh_transform(name, value)
 *
 *	Parameters:
 *		name	The name of the macro we might transform
 *		value	The value to transform
 *
 */
static void
sh_transform(name, value)
	Name		*name;
	Name		*value;
{
	/* Check if we need :sh transform */
	char		*colon;
	String_rec	command;
	String_rec	destination;
	char		buffer[1000];

	colon = strrchr((*name)->string, (int) colon_char);
	if ((colon != NULL) &&
	    (colon[1] == 's') &&
	    (colon[2] == 'h') &&
	    (colon[3] == (int) nul_char)) {
		command.text.p = (*value)->string + (*value)->hash.length;
		command.text.end = command.text.p;
		command.buffer.start = (*value)->string;
		command.buffer.end = command.text.p;

		INIT_STRING_FROM_STACK(destination, buffer);

		sh_command2string(&command, &destination);

		(*value) = GETNAME(destination.buffer.start, FIND_LENGTH);
		*colon = (int) nul_char;
		(*name) = GETNAME((*name)->string, FIND_LENGTH);
		*colon = (int) colon_char;
	}
}

/*
 *	fatal_reader(format, args...)
 *
 *	Parameters:
 *		format		printf style format string
 *		args		arguments to match the format
 *
 *	Global variables used:
 *		file_being_read	Name of the makefile being read
 *		line_number	Line that is being read
 *		report_pwd	Indicates whether current path should be shown
 *		temp_file_name	When reading tempfile we report that name
 */
/*VARARGS*/
void
fatal_reader(va_alist)
	va_dcl
{
	va_list args;
	char	message[1000];
	char	*pattern;

	va_start(args);
	pattern = va_arg(args, char *);
	if (file_being_read != NULL) {
		if (line_number != 0) {
			(void) sprintf(message,
				       "%s, line %d: %s",
				       file_being_read,
				       line_number,
				       pattern);
		} else {
			(void) sprintf(message,
				       "%s: %s",
				       file_being_read,
				       pattern);
		}
		pattern = message;
	}

	(void) fflush(stdout);
	(void) fprintf(stderr, "make: Fatal error in reader: ");
	(void) vfprintf(stderr, pattern, args);
	(void) fprintf(stderr, "\n");
	va_end(args);

	if (temp_file_name != NULL) {
		(void) fprintf(stderr,
			       "make: Temp-file %s not removed\n",
			       temp_file_name->string);
		temp_file_name = NULL;
	}

	if (report_pwd) {
		(void) fprintf(stderr,
			       "Current working directory %s\n",
			       get_current_path());
	}
	(void) fflush(stderr);
	exit(1);
}
