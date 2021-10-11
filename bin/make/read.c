#ident "@(#)read.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

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
#include <errno.h>

/*
 * Defined macros
 */
#define GOTO_STATE(new_state) { \
				  SET_STATE(new_state); \
				    goto enter_state; \
			      }
#define SET_STATE(new_state) state = (new_state)


/*
 * typedefs & structs
 */

/*
 * Static variables
 */

/*
 * File table of contents
 */
extern	Boolean		read_simple_file();
extern	void		parse_makefile();
extern	Source		get_next_block_fn();
extern	Source		push_macro_value();

/*
 *	read_simple_file(makefile_name, chase_path, doname_it,
 *		 complain, must_exist, report_file, lock_makefile)
 *
 *	Make the makefile and setup to read it. Actually read it if it is stdio
 *
 *	Return value:
 *				false if the read failed
 *
 *	Parameters:
 *		makefile_name	Name of the file to read
 *		chase_path	Use the makefile path when opening file
 *		doname_it	Call doname() to build the file first
 *		complain	Print message if doname/open fails
 *		must_exist	Generate fatal if file is missing
 *		report_file	Report file when running -P
 *		lock_makefile	Lock the makefile when reading
 *
 *	Static variables used:
 *
 *	Global variables used:
 *		do_not_exec_rule Is -n on?
 *		file_being_read	Set to the name of the new file
 *		line_number	The number of the current makefile line
 *		makefiles_used	A list of all makefiles used, appended to
 */
Boolean
read_simple_file(makefile_name, chase_path, doname_it,
		 complain, must_exist, report_file, lock_makefile)
	register Name           makefile_name;
	register Boolean        chase_path;
	register Boolean        doname_it;
	Boolean			complain;
	Boolean			must_exist;
	Boolean                 report_file;
	Boolean			lock_makefile;
{
	register Source         source = (Source) getmem(sizeof (Source_rec));
	register Property       makefile = maybe_append_prop(makefile_name,
							     makefile_prop);
	Property                orig_makefile = makefile;
	register char          *p;
	register int            length;
	register int            n;
	Dependency              dp;
	Dependency             *dpp;
	char                   *path;
	char			*previous_file_being_read = file_being_read;
	int			previous_line_number = line_number;
static	short			max_include_depth;
static	pathpt			makefile_path;

	if (max_include_depth++ >= 40) {
		fatal("Too many nested include statements");
	}
	makefile->body.makefile.contents = NULL;
	makefile->body.makefile.size = 0;
	if ((makefile_name->hash.length != 1) ||
	    (makefile_name->string[0] != (int) hyphen_char)) {
		if ((makefile->body.makefile.contents == NULL) &&
		    doname_it) {
			if (makefile_path == NULL) {
				add_dir_to_path(".", &makefile_path, -1);
				add_dir_to_path("/usr/include/make",
						&makefile_path,
						-1);
			}
			if (doname(makefile_name, true, false) ==
			    build_dont_know) {
				n = access_vroot(makefile_name->string,
						 4,
						 chase_path ?
						 makefile_path : NULL,
						 VROOT_DEFAULT);
				if (n == 0) {
					get_vroot_path((char **) NULL,
						       &path,
						       (char **) NULL);
					if ((path[0] == (int) period_char) &&
					    (path[1] == (int) slash_char)) {
						path += 2;
					}
					makefile_name = GETNAME(path,
								FIND_LENGTH);
				}
			}
		}
		source->string.free_after_use = false;
		source->previous = NULL;
		source->already_expanded = false;
		if (makefile->body.makefile.contents == NULL) {
			if (doname_it &&
			    (doname(makefile_name, true, false) ==
			     build_failed)) {
				if (complain) {
					(void) fprintf(stderr,
						       "make: Couldn't make `%s'\n",
						       makefile_name->string);
				}
				max_include_depth--;
				return failed;
			}
			if (exists(makefile_name) == (int) file_doesnt_exist) {
				if (complain ||
				    makefile_name->stat.errno != ENOENT) {
					if (must_exist) {
						fatal("Can't find `%s': %s",
						      makefile_name->string,
						      errmsg(makefile_name->
							     stat.errno));
					} else {
						warning("Can't find `%s': %s",
							makefile_name->string,
							errmsg(makefile_name->
							       stat.errno));
					}
				}
				max_include_depth--;
				return failed;
			}
			orig_makefile->body.makefile.size =
			  makefile->body.makefile.size =
			    source->bytes_left_in_file =
			      makefile_name->stat.size;
			if (report_file) {
				for (dpp = &makefiles_used;
				     *dpp != NULL;
				     dpp = &(*dpp)->next);
				dp = ALLOC(Dependency);
				dp->next = NULL;
				dp->name = makefile_name;
				dp->automatic = false;
				dp->stale = false;
				dp->built = false;
				*dpp = dp;
			}
			source->fd = open_vroot(makefile_name->string,
						O_RDONLY,
						0,
						NULL,
						VROOT_DEFAULT);
			if (source->fd < 0) {
				if (complain || (errno != ENOENT)) {
					if (must_exist) {
						fatal("Can't open `%s': %s",
						      makefile_name->string,
						      errmsg(errno));
					} else {
						warning("Can't open `%s': %s",
							makefile_name->string,
							errmsg(errno));
					}
				}
				max_include_depth--;
				return failed;
			}
			(void) fcntl(source->fd, F_SETFD, 1);
			/* Lock the file for read, but not when -n */
			if (lock_makefile && 
			    !do_not_exec_rule) {
				make_state_lockfile = ".make.state.lock";
				(void) file_lock(make_state->string,
						 make_state_lockfile,
						 0);
			}
			orig_makefile->body.makefile.contents =
			  makefile->body.makefile.contents =
			    source->string.text.p =
			      source->string.buffer.start =
				getmem((int) (makefile_name->stat.size + 1));
			source->string.text.end = source->string.text.p;
			source->string.buffer.end =
			  source->string.text.p + makefile_name->stat.size;
		} else {
			source->fd = -1;
			source->string.text.p =
			  source->string.buffer.start =
			    makefile->body.makefile.contents;
			source->string.text.end =
			  source->string.text.p + makefile->body.makefile.size;
			source->bytes_left_in_file =
			  makefile->body.makefile.size;
			source->string.buffer.end =
			  source->string.text.p + makefile->body.makefile.size;
		}
		file_being_read = makefile_name->string;
	} else {
		makefile_name = GETNAME("Standard in", FIND_LENGTH);
		source->string.free_after_use = false;
		source->previous = NULL;
		source->bytes_left_in_file = 0;
		source->already_expanded = false;
		source->fd = -1;
		source->string.buffer.start =
		  source->string.text.p = getmem(length = 1024);
		source->string.buffer.end = source->string.text.p + length;
		file_being_read = "standard input";
		line_number = 0;
		while ((n = read(fileno(stdin),
				 source->string.text.p,
				 length)) > 0) {
			length -= n;
			source->string.text.p += n;
			if (length == 0) {
				p = getmem(length = 1024 +
					   (source->string.buffer.end -
					    source->string.buffer.start));
				(void) strncpy(p,
					       source->string.buffer.start,
					       source->string.buffer.end -
					       source->string.buffer.start);
				retmem(source->string.buffer.start);
				source->string.text.p = p +
				  (source->string.buffer.end -
				   source->string.buffer.start);
				source->string.buffer.start = p;
				source->string.buffer.end =
				  source->string.buffer.start + length;
				length = 1024;
			}
		}
		if (n < 0) {
			fatal("Error reading standard input: %s",
			      errmsg(errno));
		}
		source->string.text.p = source->string.buffer.start;
		source->string.text.end = source->string.buffer.end - length;
	}
	line_number = 1;
	if (trace_reader) {
		(void) printf(">>>>>>>>>>>>>>>> Reading makefile %s\n",
			      makefile_name->string);
	}
	parse_makefile(source);
	if (trace_reader) {
		(void) printf(">>>>>>>>>>>>>>>> End of makefile %s\n",
			      makefile_name->string);
	}
	file_being_read = previous_file_being_read;
	line_number = previous_line_number;
	makefile_type = reading_nothing;
	max_include_depth--;
	return succeeded;
}

/*
 *	parse_makefile(source)
 *
 *	Strings are read from Sources.
 *	When macros are found their value is represented by a Source that is
 *	pushed on a stack. At end of string (that is returned from GET_CHAR()
 *	as 0) the block is popped.
 *

 *	Parameters:
 *		source		The source block to read from
 *
 *	Global variables used:
 *		do_not_exec_rule Is -n on?
 *		line_number	The number of the current makefile line
 *		makefile_type	What kind of makefile are we reading?
 *		empty_name	The Name ""
 */
static void
parse_makefile(source)
	register Source         source;
{
	register char		*source_p;
	register char		*source_end;
	register char		*string_start;
	char			*string_end;
	register Boolean	macro_seen_in_string;
	Boolean			append;
	String_rec		name_string;
	char			name_buffer[STRING_BUFFER_LENGTH];
	register int		distance;
	register int		paren_count;
	int			brace_count;
	Cmd_line		command;
	Cmd_line		command_tail;
	Name			macro_value;

	Name_vector_rec		target;
	Name_vector_rec		depes;
	Name_vector_rec		extra_name_vector;
	Name_vector		current_names;
	Name_vector		extra_names = &extra_name_vector;
	Name_vector		nvp;
	Boolean			target_group_seen;
	int			i;

	register Reader_state   state;
	register Reader_state   on_eoln_state;
	register Separator	separator;

	char                    buffer[4 * STRING_BUFFER_LENGTH];
	Source			extrap;

	Boolean                 save_do_not_exec_rule = do_not_exec_rule;
	Name                    makefile_name;

static	Name			sh_name;

	target.next = depes.next = NULL;
	/* Move some values from their struct to register declared locals */
	CACHE_SOURCE(0);

 start_new_line:
	/* Read whitespace on old line. Leave pointer on first char on next */
	/* line. */
	on_eoln_state = exit_state;
	for (; 1; source_p++) switch (GET_CHAR()) {
	case nul_char:
		/* End of this string. Pop it and return to the previous one */
		GET_NEXT_BLOCK(source);
		source_p--;
		if (source == NULL) {
			GOTO_STATE(on_eoln_state);
		}
		break;
	      case newline_char:
	end_of_line:
		source_p++;
		if (source->fd >= 0) {
			line_number++;
		}
		switch (GET_CHAR()) {
		case nul_char:
			GET_NEXT_BLOCK(source);
			source_p--;
			if (source == NULL) {
				GOTO_STATE(on_eoln_state);
			}
			/* Go back to the top of this loop */
			goto start_new_line;
		case newline_char:
		case numbersign_char:
		case dollar_char:
		case space_char:
		case tab_char:
			/* Go back to the top of this loop since the */
			/* new line does not start with a regular char */
			goto start_new_line;
		default:
			/* We found the first proper char on the new line */
			goto start_new_line_no_skip;
		}
	case tab_char:
	case space_char:
		/* Whitespace. Just keep going in this loop */
		break;
	case numbersign_char:
		/* Comment. Skip over it */
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case nul_char:
				GET_NEXT_BLOCK(source);
				source_p--;
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
				break;
			case backslash_char:
				/* Comments can be continued */
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
				}
				break;
			case newline_char:
				/* After we skip the comment we go to */
				/* the end of line handler since end of */
				/* line terminates comments */
				goto end_of_line;
			}
		}
	case dollar_char:
		/* Macro reference */
		if (source->already_expanded) {
			/* If we are reading from the expansion of a */
			/* macro we already expanded everything enough */
			goto start_new_line_no_skip;
		}
		/* Expand the value and push the Source on the stack of */
		/* things being read */
		source_p++;
		UNCACHE_SOURCE();
		{
			Source t = (Source) alloca((int) sizeof (Source_rec));
			source = push_macro_value(t,
						  buffer,
						  sizeof buffer,
						  source);
		}
		CACHE_SOURCE(1);
		break;
	default:
		/* We found the first proper char on the new line */
		goto start_new_line_no_skip;
	}

	/* We found the first normal char (one that starts an identifier) */
	/* on the newline */
 start_new_line_no_skip:
	/* Inspect that first char to see if it maybe is special anyway */
	switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK(source);
		source_p--;
		if (source == NULL) {
			GOTO_STATE(on_eoln_state);
		}
		goto start_new_line_no_skip;
	case newline_char:
		/* Just in case */
		goto start_new_line;
	case exclam_char:
		/* Evaluate the line before it is read */
		string_start = source_p + 1;
		macro_seen_in_string = false;
		/* Stuff the line in a string so we can eval it */
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case newline_char:
				goto eoln_1;
			case nul_char:
				if (source->fd > 0) {
					if (!macro_seen_in_string) {
						macro_seen_in_string = true;
						INIT_STRING_FROM_STACK(name_string,
								       name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					source_p--;
					break;
				}
			eoln_1:
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				extrap = (Source)
				  alloca((int) sizeof (Source_rec));
				extrap->string.buffer.start = NULL;
				if (*source_p == (int) nul_char) {
					source_p++;
				}
				/* Eval the macro */
				expand_value(GETNAME(name_string.buffer.start,
						     FIND_LENGTH),
					     &extrap->string,
					     false);
				if (name_string.free_after_use) {
					retmem(name_string.buffer.start);
				}
				UNCACHE_SOURCE();
				extrap->string.text.p =
				  extrap->string.buffer.start;
				extrap->fd = -1;
				/* And push the value */
				extrap->previous = source;
				source = extrap;
				CACHE_SOURCE(0);
				goto line_evald;
			}
		}
	default:
		goto line_evald;
	}

	/* We now have a line we can start reading */
 line_evald:
	if (source == NULL) {
		GOTO_STATE(exit_state);
	}
	/* Check if this an include command */
	if ((makefile_type == reading_makefile) &&
	    !source->already_expanded &&
	    (IS_EQUALN(source_p, "include", 7))) {
		source_p += 7;
		if (isspace(*source_p)) {
			Makefile_type save_makefile_type;
			char *name_start;
			int name_length;

			/* Yes this is an include. Skip spaces to get to the */
			/* filename */
			while (isspace(*source_p) ||
			       (*source_p == (int) nul_char)) {
				switch (GET_CHAR()) {
				case nul_char:
					GET_NEXT_BLOCK(source);
					source_p--;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;

				default:
					source_p++;
					break;
				}
			}

			string_start = source_p;
			/* Find the end of the filename */
			macro_seen_in_string = false;
			while (!isspace(*source_p) ||
			       (*source_p == (int) nul_char)) {
				switch (GET_CHAR()) {
				case nul_char:
					if (!macro_seen_in_string) {
						INIT_STRING_FROM_STACK(name_string,
								       name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					macro_seen_in_string = true;
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;

				default:
					source_p++;
					break;
				}
			}

			source->string.text.p = source_p;
			if (macro_seen_in_string) {
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				name_start = name_string.buffer.start;
				name_length = name_string.text.p - name_start;
			} else {
				name_start = string_start;
				name_length = source_p - string_start;
			}

			/* Even when we run -n we want to create makefiles */
			do_not_exec_rule = false;
			makefile_name = GETNAME(name_start, name_length);
			if (makefile_name->dollar) {
				String_rec		destination;
				char			buffer[STRING_BUFFER_LENGTH];
				char			*p;
				char			*q;

				INIT_STRING_FROM_STACK(destination, buffer);
				expand_value(makefile_name,
					     &destination,
					     false);
				for (p = destination.buffer.start;
				     (*p != (int) nul_char) && isspace(*p);
				     p++);
				for (q = p;
				     (*q != (int) nul_char) && !isspace(*q);
				     q++);
				makefile_name = GETNAME(p, q-p);
				if (destination.free_after_use) {
					retmem(destination.buffer.start);
				}
			}
			source_p++;
			UNCACHE_SOURCE();
			/* Read the file */
			save_makefile_type = makefile_type;
			if (read_simple_file(makefile_name,
					     true,
					     true,
					     true,
					     false,
					     true,
					     false) == failed) {
				fatal_reader("Read of include file `%s' failed",
					     makefile_name->string);
			}
			makefile_type = save_makefile_type;
			do_not_exec_rule = save_do_not_exec_rule;
			CACHE_SOURCE(0);
			goto start_new_line;
		} else {
			source_p -= 7;
		}
	}

	/* Reset the status in preparation for the new line */
	for (nvp = &target; nvp != NULL; nvp = nvp->next) {
		nvp->used = 0;
	}
	for (nvp = &depes; nvp != NULL; nvp = nvp->next) {
		nvp->used = 0;
	}
	target_group_seen = false;
	command = command_tail = NULL;
	macro_value = NULL;
	append = false;
	current_names = &target;
	SET_STATE(scan_name_state);
	on_eoln_state = illegal_eoln_state;
	separator = none_seen;

	/* The state machine starts here */
 enter_state:
	while (1) switch (state) {

/****************************************************************
 *	Scan name state
 */
case scan_name_state:
	/* Scan an identifier. We skip over chars until we find a break char */
	/* First skip white space. */
	for (; 1; source_p++) switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK(source);
		source_p--;
		if (source == NULL) {
			GOTO_STATE(on_eoln_state);
		}
		break;
	case newline_char:
		/* We found the end of the line. */
		/* Do postprocessing or return error */
		source_p++;
		if (source->fd >= 0) {
			line_number++;
		}
		GOTO_STATE(on_eoln_state);
	case backslash_char:
		/* Continuation */
		if (*++source_p == (int) nul_char) {
			GET_NEXT_BLOCK(source);
			if (source == NULL) {
				GOTO_STATE(on_eoln_state);
			}
		}
		/* Skip over any number of newlines */
		while (*source_p == (int) newline_char) {
			if (source->fd >= 0) {
				line_number++;
			}
			if (*++source_p == (int) nul_char) {
				GET_NEXT_BLOCK(source);
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
			}
		}
		source_p--;
		break;
	case tab_char:
	case space_char:
		/* Whitespace is skipped */
		break;
	case numbersign_char:
		/* Comment. Skip over it */
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case nul_char:
				GET_NEXT_BLOCK(source);
				source_p--;
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
				break;
			case backslash_char:
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
				}
				break;
			case newline_char:
				source_p++;
				if (source->fd >= 0) {
					line_number++;
				}
				GOTO_STATE(on_eoln_state);
			}
		}
	case dollar_char:
		/* Macro reference. Expand and push value */
		if (source->already_expanded) {
			goto scan_name;
		}
		source_p++;
		UNCACHE_SOURCE();
		{
			Source t = (Source) alloca((int) sizeof (Source_rec));
			source = push_macro_value(t,
						  buffer,
						  sizeof buffer,
						  source);
		}
		CACHE_SOURCE(1);
		break;
	default:
		/* End of white space */
		goto scan_name;
	}

	/* First proper identifier character */
 scan_name:
	string_start = source_p;
	paren_count = brace_count = 0;
	macro_seen_in_string = false;
	resume_name_scan:
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case nul_char:
			/* Save what we have seen so far of the identifier */
			if (!macro_seen_in_string) {
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			macro_seen_in_string = true;
			/* Get more text to read */
			GET_NEXT_BLOCK(source);
			string_start = source_p;
			source_p--;
			if (source == NULL) {
				GOTO_STATE(on_eoln_state);
			}
			break;
		case newline_char:
			if (paren_count > 0) {
				fatal_reader("Unmatched `(' on line");
			}
			if (brace_count > 0) {
				fatal_reader("Unmatched `{' on line");
			}
			source_p++;
			/* Enter name */
			current_names = enter_name(&name_string,
						   macro_seen_in_string,
						   string_start,
						   source_p - 1,
						   current_names,
						   &extra_names,
						   &target_group_seen);
			if (extra_names == NULL) {
				extra_names = (Name_vector)
				  alloca((int) sizeof (Name_vector_rec));
			}
			/* Do postprocessing or return error */
			if (source->fd >= 0) {
				line_number++;
			}
			GOTO_STATE(on_eoln_state);
		case backslash_char:
			/* Check if this is a quoting backslash */
			if (*++source_p == (int) nul_char) {
				GET_NEXT_BLOCK(source);
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
			}
			if (*source_p != (int) newline_char) {
				/* Save the identifier so far */
				append_string(string_start,
					      &name_string,
					      source_p - string_start - 1);
				macro_seen_in_string = true;
				string_start = source_p;
				break;
			} else {
				source_p--;
			}
			/* Enter name, skip any number of newlines and */
			/* continue reading names */
			if (paren_count > 0) {
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				macro_seen_in_string = true;
			} else {
				current_names = enter_name(&name_string,
							   macro_seen_in_string,
							   string_start,
							   source_p,
							   current_names,
							   &extra_names,
							   &target_group_seen);
				if (extra_names == NULL) {
					extra_names = (Name_vector)
					  alloca((int) sizeof (Name_vector_rec));
				}
			}
			if (*++source_p == (int) nul_char) {
				GET_NEXT_BLOCK(source);
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
			}
			while (*source_p == (int) newline_char) {
				if (source->fd >= 0) {
					line_number++;
				}
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
				}
			}
			if (paren_count > 0) {
				string_start = source_p;
				break;
			} else {
				goto enter_state;
			}
		case numbersign_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			fatal_reader("Unexpected comment seen");
		case dollar_char:
			if (source->already_expanded) {
				break;
			}
			/* Save the identifier so far */
			if (!macro_seen_in_string) {
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			macro_seen_in_string = true;
			/* Eval and push the macro */
			source_p++;
			UNCACHE_SOURCE();
			{
				Source t =
				  (Source) alloca((int) sizeof (Source_rec));
				source = push_macro_value(t,
							  buffer,
							  sizeof buffer,
							  source);
			}
			CACHE_SOURCE(1);
			string_start = source_p + 1;
			break;
		case parenleft_char:
			paren_count++;
			break;
		case parenright_char:
			if (--paren_count < 0) {
				fatal_reader("Unmatched `)' on line");
			}
			break;
		case braceleft_char:
			brace_count++;
			break;
		case braceright_char:
			if (--brace_count < 0) {
				fatal_reader("Unmatched `}' on line");
			}
			break;
		case ampersand_char:
		case greater_char:
		case bar_char:
			if (paren_count + brace_count == 0) {
				source_p++;
			}
			/* Fall into */
		case tab_char:
		case space_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			current_names = enter_name(&name_string,
						   macro_seen_in_string,
						   string_start,
						   source_p,
						   current_names,
						   &extra_names,
						   &target_group_seen);
			if (extra_names == NULL) {
				extra_names = (Name_vector)
				  alloca((int) sizeof (Name_vector_rec));
			}
			goto enter_state;
		case colon_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			if (separator == conditional_seen) {
				break;
			}
			/* End of the target list. We now start reading */
			/* dependencies or a conditional assignment */
			if (separator != none_seen) {
				fatal_reader("Extra `:', `::', or `:=' on dependency line");
			}
			/* Enter the last target */
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				current_names =
				  enter_name(&name_string,
					     macro_seen_in_string,
					     string_start,
					     source_p,
					     current_names,
					     &extra_names,
					     &target_group_seen);
				if (extra_names == NULL) {
					extra_names = (Name_vector)
					  alloca((int)
						 sizeof (Name_vector_rec));
				}
			}
			/* Check if it is ":" "::" or ":=" */
		scan_colon_label:
			switch (*++source_p) {
			case nul_char:
				GET_NEXT_BLOCK(source);
				source_p--;
				if (source == NULL) {
					GOTO_STATE(enter_dependencies_state);
				}
				goto scan_colon_label;
			case equal_char:
				separator = conditional_seen;
				source_p++;
				current_names = &depes;
				GOTO_STATE(scan_name_state);
			case colon_char:
				separator = two_colon;
				source_p++;
				break;
			default:
				separator = one_colon;
			}
			current_names = &depes;
			on_eoln_state = enter_dependencies_state;
			GOTO_STATE(scan_name_state);
		case semicolon_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			/* End of reading names. Start reading the rule */
			if ((separator != one_colon) &&
			    (separator != two_colon)) {
				fatal_reader("Unexpected command seen");
			}
			/* Enter the last dependency */
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				current_names =
				  enter_name(&name_string,
					     macro_seen_in_string,
					     string_start,
					     source_p,
					     current_names,
					     &extra_names,
					     &target_group_seen);
				if (extra_names == NULL) {
					extra_names = (Name_vector)
					  alloca((int)
						 sizeof (Name_vector_rec));
				}
			}
			source_p++;
			/* Make sure to enter a rule even if the is */
			/* no text here */
			if (*source_p == (int) newline_char) {
				command = command_tail = ALLOC(Cmd_line);
				command->next = NULL;
				command->command_line = empty_name;
				command->make_refd = false;
				command->ignore_command_dependency = false;
				command->assign = false;
				command->ignore_error = false;
				command->silent = false;
			}
			GOTO_STATE(scan_command_state);
		case plus_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			/* We found "+=" construct */
			if (source_p != string_start) {
				/* "+" is not a break char. */
				/* Ignore it if it is part of an identifier */
				source_p++;
				goto resume_name_scan;
			}
			/* Make sure the "+" is followed by a "=" */
		scan_append:
			switch (*++source_p) {
			case nul_char:
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				macro_seen_in_string = true;
				GET_NEXT_BLOCK(source);
				string_start = source_p;
				source_p--;
				if (source == NULL) {
					GOTO_STATE(illegal_eoln_state);
				}
				goto scan_append;
			case equal_char:
				append = true;
				break;
			default:
				/* The "+" just starts a regular name. */
				/* Start reading that name */
				goto resume_name_scan;
			}
			/* Fall into */
		case equal_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			/* We found macro assignment. */
			/* Check if it is legal and if it is appending */
			switch (separator) {
			case none_seen:
				separator = equal_seen;
				on_eoln_state = enter_equal_state;
				break;
			case conditional_seen:
				on_eoln_state = enter_conditional_state;
				break;
			default:
				/* Reader must special check for "MACRO:sh=" */
				/* notation */
				if (sh_name == NULL) {
					sh_name = GETNAME("sh", FIND_LENGTH);
				}
				if (((target.used == 1) &&
				     (depes.used == 1) &&
				     (depes.names[0] == sh_name)) ||
				    ((target.used == 1) &&
				     (depes.used == 0) &&
				     (separator == one_colon) &&
				     (string_start + 2 == source_p) &&
				     (string_start[0] == 's') &&
				     (string_start[1] == 'h'))) {
					String_rec	macro_name;
					char		buffer[100];

					INIT_STRING_FROM_STACK(macro_name,
							       buffer);
					append_string(target.names[0]->string,
						      &macro_name,
						      FIND_LENGTH);
					append_char((int) colon_char,
						    &macro_name);
					append_string(sh_name->string,
						      &macro_name,
						      FIND_LENGTH);
					target.names[0] =
					  GETNAME(macro_name.buffer.start,
						  FIND_LENGTH);
					separator = equal_seen;
					on_eoln_state = enter_equal_state;
					break;
				}
				fatal_reader("Macro assignment on dependency line");
			}
			if (append) {
				source_p--;
			}
			/* Enter the macro name */
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				current_names =
				  enter_name(&name_string,
					     macro_seen_in_string,
					     string_start,
					     source_p,
					     current_names,
					     &extra_names,
					     &target_group_seen);
				if (extra_names == NULL) {
					extra_names = (Name_vector)
					  alloca((int)
						 sizeof (Name_vector_rec));
				}
			}
			if (append) {
				source_p++;
			}
			macro_value = NULL;
			source_p++;
			distance = 0;
			/* Skip whitespace to the start of the value */
			macro_seen_in_string = false;
			for (; 1; source_p++) {
				switch (GET_CHAR()) {
				case nul_char:
					GET_NEXT_BLOCK(source);
					source_p--;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;
				case backslash_char:
					if (*++source_p == (int) nul_char) {
						GET_NEXT_BLOCK(source);
						if (source == NULL) {
							GOTO_STATE(on_eoln_state);
						}
					}
					if (*source_p != (int) newline_char) {
						if (!macro_seen_in_string) {
							macro_seen_in_string =
							  true;
							INIT_STRING_FROM_STACK(name_string,
									       name_buffer);
						}
						append_char((int)
							    backslash_char,
							    &name_string);
						append_char(*source_p,
							    &name_string);
						string_start = source_p+1;
						goto macro_value_start;
					}
					break;
				case newline_char:
				case numbersign_char:
					string_start = source_p;
					goto macro_value_end;
				case tab_char:
				case space_char:
					break;
				default:
					string_start = source_p;
					goto macro_value_start;
				}
			}
		macro_value_start:
			/* Find the end of the value */
			for (; 1; source_p++) {
				if (distance != 0) {
					*source_p = *(source_p + distance);
				}
				switch (GET_CHAR()) {
				case nul_char:
					if (!macro_seen_in_string) {
						macro_seen_in_string = true;
						INIT_STRING_FROM_STACK(name_string,
								       name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					source_p--;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;
				case backslash_char:
					source_p++;
					if (distance != 0) {
						*source_p =
						  *(source_p + distance);
					}
					if (*source_p == (int) nul_char) {
						if (!macro_seen_in_string) {
							macro_seen_in_string =
							  true;
							INIT_STRING_FROM_STACK(name_string,
									       name_buffer);
						}
						append_string(string_start,
							      &name_string,
							      source_p -
							      string_start);
						GET_NEXT_BLOCK(source);
						string_start = source_p;
						if (source == NULL) {
							GOTO_STATE(on_eoln_state);
						}
						if (distance != 0) {
							*source_p =
							  *(source_p +
							    distance);
						}
					}
					if (*source_p == (int) newline_char) {
						source_p--;
						line_number++;
						distance++;
						*source_p = (int) space_char;
						while (*(source_p +
							 distance + 1) ==
						       (int) newline_char) {
							line_number++;
							distance++;
						}
						while ((*(source_p +
							  distance + 1) ==
							(int) tab_char) ||
						       (*(source_p +
							  distance + 1) ==
							(int) space_char)) {
							distance++;
						}
					}
					break;
				case newline_char:
				case numbersign_char:
					goto macro_value_end;
				}
			}
		macro_value_end:
			/* Complete the value in the string */
			if (!macro_seen_in_string) {
				macro_seen_in_string = true;
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			if (name_string.buffer.start != name_string.text.p) {
					macro_value =
					  GETNAME(name_string.buffer.start,
						  FIND_LENGTH);
				}
			if (name_string.free_after_use) {
				retmem(name_string.buffer.start);
			}
			for (; distance > 0; distance--) {
				*source_p++ = (int) space_char;
			}
			GOTO_STATE(on_eoln_state);
		}
	}

/****************************************************************
 *	enter dependencies state
 */
 case enter_dependencies_state:
 enter_dependencies_label:
/* Expects pointer on first non whitespace char after last dependency. (On */
/* next line.) We end up here after having read a "targets : dependencies" */
/* line. The state checks if there is a rule to read and if so dispatches */
/* to scan_command_state scan_command_state reads one rule line and the */
/* returns here */

	/* First check if the first char on the next line is special */
	switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK(source);
		if (source == NULL) {
			break;
		}
		goto enter_dependencies_label;
	case exclam_char:
		/* The line should be evaluate before it is read */
		macro_seen_in_string = false;
		string_start = source_p + 1;
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case newline_char:
				goto eoln_2;
			case nul_char:
				if (source->fd > 0) {
					if (!macro_seen_in_string) {
						macro_seen_in_string = true;
						INIT_STRING_FROM_STACK(name_string,
								       name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					source_p--;
					break;
				}
			eoln_2:
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				extrap = (Source)
				  alloca((int) sizeof (Source_rec));
				extrap->string.buffer.start = NULL;
				expand_value(GETNAME(name_string.buffer.start,
						     FIND_LENGTH),
					     &extrap->string,
					     false);
				if (name_string.free_after_use) {
					retmem(name_string.buffer.start);
				}
				UNCACHE_SOURCE();
				extrap->string.text.p =
				  extrap->string.buffer.start;
				extrap->fd = -1;
				extrap->previous = source;
				source = extrap;
				CACHE_SOURCE(0);
				goto enter_dependencies_label;
			}
		}
	case dollar_char:
		if (source->already_expanded) {
			break;
		}
		source_p++;
		UNCACHE_SOURCE();
		{
			Source t = (Source) alloca((int) sizeof (Source_rec));
			source = push_macro_value(t,
						  buffer,
						  sizeof buffer,
						  source);
		}
		CACHE_SOURCE(0);
		goto enter_dependencies_label;
	case numbersign_char:
		if (makefile_type != reading_makefile) {
			source_p++;
			GOTO_STATE(scan_command_state);
		}
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case nul_char:
				GET_NEXT_BLOCK(source);
				source_p--;
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
				break;
			case backslash_char:
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
				}
				break;
			case newline_char:
				source_p++;
				if (source->fd >= 0) {
					line_number++;
				}
				goto enter_dependencies_label;
			}
		}
	case tab_char:
		GOTO_STATE(scan_command_state);
	}

	/* We read all the command lines for the target/dependency line. */
	/* Enter the stuff */
	if (target_group_seen) {
		find_target_groups(&target);
	}
	for (nvp = &target; nvp != NULL; nvp = nvp->next) {
		for (i = 0; i < nvp->used; i++) {
			if (nvp->names[i] != NULL) {
				enter_dependencies(nvp->names[i],
						   nvp->target_group[i],
						   &depes,
						   command,
						   separator);
			}
		}
	}
	goto start_new_line;

/****************************************************************
 *	scan command state
 */
case scan_command_state:
	/* We need to read one rule line. Do that and return to */
	/* the enter dependencies state */
	string_start = source_p;
	macro_seen_in_string = false;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case backslash_char:
			if (!macro_seen_in_string) {
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			macro_seen_in_string = true;
			if (*++source_p == (int) nul_char) {
				GET_NEXT_BLOCK(source);
				if (source == NULL) {
					string_start = source_p;
					goto command_newline;
				}
			}
			append_char((int) backslash_char, &name_string);
			append_char(*source_p, &name_string);
			if (*source_p == (int) newline_char) {
				if (source->fd >= 0) {
					line_number++;
				}
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						string_start = source_p;
						goto command_newline;
					}
				}
				if (*source_p == (int) tab_char) {
					source_p++;
				}
			} else {
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						string_start = source_p;
						goto command_newline;
					}
				}
			}
			string_start = source_p;
			if ((*source_p == (int) newline_char) ||
			    (*source_p == (int) backslash_char) ||
			    (*source_p == (int) nul_char)) {
				source_p--;
			}
			break;
		case newline_char:
		command_newline:
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				if (macro_seen_in_string) {
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					string_start =
					  name_string.buffer.start;
					string_end = name_string.text.p;
				} else {
					string_end = source_p;
				}
				while ((*string_start != (int) newline_char) &&
				       isspace(*string_start)){
					string_start++;
				}
				if ((string_end > string_start) ||
				    (makefile_type == reading_statefile)) {
					if (command_tail == NULL) {
						command =
						  command_tail =
						    ALLOC(Cmd_line);
					} else {
						command_tail->next =
						  ALLOC(Cmd_line);
						command_tail =
						  command_tail->next;
					}
					command_tail->next = NULL;
					command_tail->command_line =
					  GETNAME(string_start,
						  string_end - string_start);
					if (macro_seen_in_string &&
					    name_string.free_after_use) {
						retmem(name_string.
						       buffer.start);
					}
				}
			}
			do {
				if ((source != NULL) && (source->fd >= 0)) {
					line_number++;
				}
				if ((source != NULL) &&
				    (*++source_p == (int) nul_char)) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
				}
			} while (*source_p == (int) newline_char);

			GOTO_STATE(enter_dependencies_state);
		case nul_char:
			if (!macro_seen_in_string) {
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			macro_seen_in_string = true;
			GET_NEXT_BLOCK(source);
			string_start = source_p;
			source_p--;
			if (source == NULL) {
				GOTO_STATE(enter_dependencies_state);
			}
			break;
		}
	}

/****************************************************************
 *	enter equal state
 */
case enter_equal_state:
	if (target.used != 1) {
		GOTO_STATE(poorly_formed_macro_state);
	}
	enter_equal(target.names[0], macro_value, append);
	goto start_new_line;

/****************************************************************
 *	enter conditional state
 */
case enter_conditional_state:
	if (depes.used != 1) {
		GOTO_STATE(poorly_formed_macro_state);
	}
	for (nvp = &target; nvp != NULL; nvp = nvp->next) {
		for (i = 0; i < nvp->used; i++) {
			enter_conditional(nvp->names[i],
					  depes.names[0],
					  macro_value,
					  append);
		}
	}
	goto start_new_line;

/****************************************************************
 *	Error states
 */
case illegal_eoln_state:
	fatal_reader("Unexpected end of line seen");
case poorly_formed_macro_state:
	fatal_reader("Badly formed macro assignment");
case exit_state:
	return;
default:
	fatal_reader("Internal error. Unknown reader state");
}
}

/*
 *	get_next_block(source)
 *
 *	Will get the next block of text to read either
 *	by popping one source bVSIZEOFlock of the stack of Sources
 *	or by reading some more from the makefile.
 *
 *	Return value:
 *				The new source block to read from
 *
 *	Parameters:
 *		source		The old source block
 *
 *	Global variables used:
 *		file_being_read	The name of the current file, error msg
 */
Source
get_next_block_fn(source)
	register Source		source;
{
	register int		length;
	register int		to_read;

	if (source == NULL) {
		return source;
	}
	if ((source->fd < 0) || (source->bytes_left_in_file <= 0)) {
		/* We cant read from the makefile so we pop the source block */
		if (source->fd > 2) {
			(void) close(source->fd);
			if (make_state_lockfile != NULL) {
				(void) unlink(make_state_lockfile);
				make_state_lockfile = NULL;
			}
		}
		if (source->string.free_after_use &&
		    (source->string.buffer.start != NULL)) {
			retmem(source->string.buffer.start);
			source->string.buffer.start = NULL;
		}
		return source->previous;
	}
	/* Read some more from the makefile. Hopefully the kernel managed to */
	/* prefetch the stuff */
	to_read = 8 * 1024;
	if (to_read > source->bytes_left_in_file) {
		to_read = source->bytes_left_in_file;
	}
	length = read(source->fd,
		      source->string.buffer.end - source->bytes_left_in_file,
		      to_read);
	if (length != to_read) {
		if (length == 0) {
			fatal("Error reading `%s': Premature EOF",
			      file_being_read);
		} else {
			fatal("Error reading `%s': %s", file_being_read,
			      errmsg(errno));
		}
	}
	source->bytes_left_in_file -= length;
	source->string.text.end += length;
	*source->string.text.end = 0;
	return source;
}

/*
 *	push_macro_value(bp, buffer, size, source)
 *
 *	Macro and function that evaluates one macro
 *	and makes the reader read from the value of it
 *
 *	Return value:
 *				The source block to read the macro from
 *
 *	Parameters:
 *		bp		The new source block to fill in
 *		buffer		Buffer to read from
 *		size		size of the buffer
 *		source		The old source block
 *
 *	Global variables used:
 */
static Source
push_macro_value(bp, buffer, size, source)
	register Source         bp;
	register char          *buffer;
	int                     size;
	register Source         source;
{
	bp->string.buffer.start = bp->string.text.p = buffer;
	bp->string.text.end = NULL;
	bp->string.buffer.end = buffer + size;
	bp->string.free_after_use = false;
	expand_macro(source, &bp->string, (char *) NULL, false);
	bp->string.text.p = bp->string.buffer.start;
	bp->fd = -1;
	bp->already_expanded = true;
	bp->previous = source;
	return bp;
}

