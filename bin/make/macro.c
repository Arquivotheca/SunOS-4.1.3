#ident "@(#)macro.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	macro.c
 *
 *	Handle expansion of make macros
 */

/*
 * Included files
 */
#include "defs.h"

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Static variables
 */
static	Envvar		envvar = NULL;

/*
 * File table of contents
 */
extern	Property	setvar_daemon();
extern	Name		getvar();
extern	void		setvar_envvar();
extern	void		expand_value_with_daemon();
extern	void		expand_value();
extern	void		expand_macro();

/*
 *	setvar_daemon(name, value, append, daemon)
 *
 *	Set a macro value, possibly supplying a daemon to be used
 *	when referencing the value.
 *
 *	Return value:
 *				The property block with the new value
 *
 *	Parameters:
 *		name		Name of the macro to set
 *		value		The value to set
 *		append		Should we reset or append to the current value?
 *		daemon		Special treatment when reading the value
 *
 *	Static variables used:
 *		envvar		A list of environment vars with $ in value
 *
 *	Global variables used:
 *		debug_level	Indicates how much tracing we should do
 *		makefile_type	Used to check if we should enforce read only
 *		path_name	The Name "PATH", compared against
 *		virtual_root	The Name "VIRTUAL_ROOT", compared against
 *		vpath_defined	Set if the macro VPATH is set
 *		vpath_name	The Name "VPATH", compared against
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
	register Chain		chain;
	int			length;

	if ((makefile_type != reading_nothing) &&
	    macro->body.macro.read_only) {
		return macro;
	}
	/* Strip spaces from the end of the value */
	if (daemon == no_daemon) {
		length = (value == NULL) || (value->string == NULL) ?
			0 : strlen(value->string);
		if ((length > 0) && isspace(value->string[length-1])) {
			INIT_STRING_FROM_STACK(destination, buffer);
			buffer[0] = 0;
			append_string(value->string, &destination, length);
			while ((length > 0) &&
			       isspace(destination.buffer.start[length-1])) {
				destination.buffer.start[--length] = 0;
			}
			value = GETNAME(destination.buffer.start, FIND_LENGTH);
		}
	}
		
	if (append) {
		/* If we are appending we just tack the new value after */
		/* the old one with a space in between */
		INIT_STRING_FROM_STACK(destination, buffer);
		buffer[0] = 0;
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
	/* Debugging trace */
	if (debug_level > 1) {
		if (value != NULL) {
			switch (daemon) {
			case chain_daemon:
				(void) printf("%s =", name->string);
				for (chain = (Chain) value;
				     chain != NULL;
				     chain = chain->next) {
					(void) printf(" %s",
						      chain->name->string);
				}
				(void) printf("\n");
				break;
			case no_daemon:
				(void) printf("%s= %s\n",
					      name->string,
					      value->string);
				break;
			}
		} else {
			(void) printf("%s =\n", name->string);
		}
	}
	/* Set the new values in the macro property block */
	macro->body.macro.value = value;
	macro->body.macro.daemon = daemon;
	/* If the user changes the VIRTUAL_ROOT we need to flush */
	/* the vroot package cache */
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
	/* For environment variables we also set the */
	/* environment value each time */
	if (macro->body.macro.exported) {
		char	*env;

		if (!reading_environment && (value != NULL) && value->dollar) {
			Envvar	p;

			for (p = envvar; p != NULL; p = p->next) {
				if (p->name == name) {
					p->value = value;
					goto found_it;
				}
			}

			p = ALLOC(Envvar);
			p->name = name;
			p->value = value;
			p->next = envvar;
			envvar = p;
		    found_it:;
		} else {
			length = 2 + strlen(name->string);
			if (value != NULL) {
				length += strlen(value->string);
			}
			env = getmem(length);
			(void) sprintf(env,
				       "%s=%s",
				       name->string,
				       value == NULL ? "" : value->string);
			(void) putenv(env);
		}
	}
	if (name == target_arch) {
		Name	ha = getvar(host_arch);
		Name	ta = getvar(target_arch);
		Name	vr = getvar(virtual_root);
		int	length;
		char	*new_value;
		char	*old_vr;

		length = 32 +
		  strlen(ha->string) +
		    strlen(ta->string) +
		      strlen(vr->string);
		new_value = alloca(length);
		old_vr = vr->string;
		if (IS_EQUALN(old_vr,
			      "/usr/arch/",
			      strlen("/usr/arch/"))) {
			old_vr = strchr(old_vr, (int) colon_char) + 1;
		}
		if (ha == ta) {
			new_value = old_vr;
		} else {
			(void) sprintf(new_value,
				       "/usr/arch/%s/%s:%s",
				       ha->string + 1,
				       ta->string + 1,
				       old_vr);
		}
		if (new_value[0] != 0) {
			(void) SETVAR(virtual_root,
				      GETNAME(new_value, FIND_LENGTH),
				      false);
		}
	}
	return macro;
}

/*
 *	getvar(name)
 *
 *	Return expanded value of macro.
 *
 *	Return value:
 *				The expanded value of the macro
 *
 *	Parameters:
 *		name		The name of the macro we want the value for
 *
 *	Global variables used:
 */
Name
getvar(name)
	register Name		name;
{
	String_rec		destination;
	char			buffer[STRING_BUFFER_LENGTH];
	register Name		result;

	INIT_STRING_FROM_STACK(destination, buffer);
	expand_value(maybe_append_prop(name, macro_prop)->body.macro.value,
		     &destination,
		     false);
	result = GETNAME(destination.buffer.start, FIND_LENGTH);
	if (destination.free_after_use) {
		retmem(destination.buffer.start);
	}
	return result;
}

/*
 *	setvar_envvar()
 *
 *	This function scans the list of environment variables that have
 *	dynamic values and sets them.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		envvar		A list of environment vars with $ in value
 */
void
setvar_envvar()
{
	Envvar		p;
	int		length;
	char		*env;
	String_rec	value;
	char		buffer[STRING_BUFFER_LENGTH];

	for (p = envvar; p != NULL; p = p->next) {
		INIT_STRING_FROM_STACK(value, buffer);		
		expand_value(p->value, &value, false);
		length = 2 + strlen(p->name->string);
		length += strlen(value.buffer.start);
		env = getmem(length);
		(void) sprintf(env,
			       "%s=%s",
			       p->name->string,
			       value.buffer.start);
		(void) putenv(env);
	}
}

/*
 *	expand_value_with_daemon(macro, destination, cmd)
 *
 *	Checks for daemons and then maybe calls expand_value().
 *
 *	Parameters:
 *		macro		The property block with the value to expand
 *		destination	Where the result should be deposited
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *
 *	Global variables used:
 */
static void
expand_value_with_daemon(macro, destination, cmd)
	register Property	macro;
	register String		destination;
	Boolean			cmd;
{
	register Chain		chain;

	switch (macro->body.macro.daemon) {
	case no_daemon:
		expand_value(macro->body.macro.value, destination, cmd);
		return;
	case chain_daemon:
		/* If this is a $? value we call the daemon to translate the */
		/* list of names to a string */
		for (chain = (Chain) macro->body.macro.value;
		     chain != NULL;
		     chain = chain->next) {
			append_string(chain->name->string,
				      destination,
				      (int) chain->name->hash.length);
			if (chain->next != NULL) {
				append_char((int) space_char, destination);
			}
		}
		return;
	}
}

/*
 *	expand_value(value, destination, cmd)
 *
 *	Recursively expands all macros in the string value.
 *	destination is where the expanded value should be appended.
 *
 *	Parameters:
 *		value		The value we are expanding
 *		destination	Where to deposit the expansion
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *
 *	Global variables used:
 */
void
expand_value(value, destination, cmd)
	Name			value;
	register String		destination;
	Boolean			cmd;
{
	Source_rec		sourceb;
	register Source		source = &sourceb;
	register char		*source_p;
	register char		*source_end;
	char			*block_start;
	int			quote_seen;

	if (value == NULL) {
		/* Make sure to get a string allocated even if it */
		/* will be empty */
		append_string("", destination, FIND_LENGTH);
		destination->text.end = destination->text.p;
		return;
	}
	if (!value->dollar) {
		/* If the value we are expanding does not contain */
		/* any $ we dont have to parse it */
		append_string(value->string,
			      destination,
			      (int) value->hash.length);
		destination->text.end = destination->text.p;
		return;
	}

	if (value->being_expanded) {
		fatal_reader("Loop detected when expanding macro value `%s'",
			     value->string);
	}
	value->being_expanded = true;
	/* Setup the structure we read from */
	sourceb.string.text.p = sourceb.string.buffer.start = value->string;
	sourceb.string.text.end =
	  sourceb.string.buffer.end =
	    sourceb.string.text.p + value->hash.length;
	sourceb.string.free_after_use = false;
	sourceb.previous = NULL;
	sourceb.fd = -1;
	/* Lift some pointers from the struct to local register variables */
	CACHE_SOURCE(0);
/* We parse the string in segments */
/* We read chars until we find a $, then we append what we have read so far */
/* (since last $ processing) to the destination. When we find a $ we call */
/* expand_macro() and let it expand that particular $ reference into dest */
	block_start = source_p;
	quote_seen = 0;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case backslash_char:
			/* Quote $ in macro value */
			if (!cmd) {
				quote_seen = ~quote_seen;
			}
			continue;
		case dollar_char:
			/* Save the plain string we found since */
			/* start of string or previous $ */
			if (quote_seen) {
				append_string(block_start,
					      destination,
					      source_p - block_start - 1);
				block_start = source_p;
				break;
			}
			append_string(block_start,
				      destination,
				      source_p - block_start);
			source->string.text.p = ++source_p;
			UNCACHE_SOURCE();
			/* Go expand the macro reference */
			expand_macro(source, destination, value->string, cmd);
			CACHE_SOURCE(1);
			block_start = source_p + 1;
			break;
		case nul_char:
			/* The string ran out. Get some more */
			append_string(block_start,
				      destination,
				      source_p - block_start);
			GET_NEXT_BLOCK(source);
			if (source == NULL) {
				destination->text.end = destination->text.p;
				value->being_expanded = false;
				return;
			}
			block_start = source_p;
			source_p--;
			continue;
		}
		quote_seen = 0;
	}
}

/*
 *	expand_macro(source, destination, current_string, cmd)
 *
 *	Should be called with source->string.text.p pointing to
 *	the first char after the $ that starts a macro reference.
 *	source->string.text.p is returned pointing to the first char after
 *	the macro name.
 *	It will read the macro name, expanding any macros in it,
 *	and get the value. The value is then expanded.
 *	destination is a String that is filled in with the expanded macro.
 *	It may be passed in referencing a buffer to expand the macro into.
 *
 *	Parameters:
 *		source		The source block that references the string
 *				to expand
 *		destination	Where to put the result
 *		current_string	The string we are expanding, for error msg
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *
 *	Global variables used:
 *		funny		Vector of semantic tags for characters
 *		is_conditional	Set if a conditional macro is refd
 *		make_word_mentioned Set if the word "MAKE" is mentioned
 *		makefile_type	We deliver extra msg when reading makefiles
 *		query		The Name "?", compared against
 *		query_mentioned	Set if the word "?" is mentioned
 */
void
expand_macro(source, destination, current_string, cmd)
	register Source		source;
	register String		destination;
	char			*current_string;
	Boolean			cmd;
{
	register char		*source_p = source->string.text.p;
	register char		*source_end = source->string.text.end;
	Property		macro = NULL;
	Name			name;
	char			*block_start;
	char			*colon;
	char			*eq = NULL;
	char			*percent;
	char			*p;
	register int		closer;
	register int		closer_level = 1;
	String_rec		string;
	char			buffer[STRING_BUFFER_LENGTH];
	String_rec		extracted;
	char			extracted_string[MAXPATHLEN];
	char			*left_head;
	char			*left_tail;
	char			*right_tail;
	int			left_head_len;
	int			left_tail_len;
	char			*right_hand[128];
	int			quote_seen;
	int			i;
	enum {
		no_replace,
		suffix_replace,
		pattern_replace,
		sh_replace,
	}			replacement = no_replace;
	enum {
		no_extract,
		dir_extract,
		file_extract,
	}                       extraction = no_extract;
static	Name			make;

	if (make == NULL) {
		make = GETNAME("MAKE", FIND_LENGTH);
	}

	/* First copy the (macro-expanded) macro name into string */
	INIT_STRING_FROM_STACK(string, buffer);
recheck_first_char:
	/* Check the first char of the macro name to figure out what to do */
	switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK(source);
		if (source == NULL) {
			fatal_reader("'$' at end of string `%s'",
				     current_string);
		}
		goto recheck_first_char;
	case parenleft_char:
		/* Multi char name */
		closer = (int) parenright_char;
		break;
	case braceleft_char:
		/* Multi char name */
		closer = (int) braceright_char;
		break;
	case newline_char:
		fatal_reader("'$' at end of line");
	default:
		/* Single char macro name. Just suck it up */
		append_char(*source_p, &string);
		source->string.text.p = source_p + 1;
		goto get_macro_value;
	}

	/* Handle multi-char macro names */
	block_start = ++source_p;
	quote_seen = 0;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case nul_char:
			append_string(block_start,
				      &string,
				      source_p - block_start);
			GET_NEXT_BLOCK(source);
			if (source == NULL) {
				if (current_string != NULL) {
					fatal_reader("Unmatched `%c' in string `%s'",
						     closer ==
						     (int) braceright_char ?
						     (int) braceleft_char :
						     (int) parenleft_char,
						     current_string);
				} else {
					fatal_reader("Premature EOF");
				}
			}
			block_start = source_p;
			source_p--;
			continue;
		case newline_char:
			fatal_reader("Unmatched `%c' on line",
				     closer == (int) braceright_char ?
				     (int) braceleft_char :
				     (int) parenleft_char);
		case backslash_char:
			/* Quote dollar in macro value */
			if (!cmd) {
				quote_seen = ~quote_seen;
			}
			continue;
		case dollar_char:
			/* Macro names may reference macros. This expands the*/
			/* value of such macros into the macro name string */
			if (quote_seen) {
				append_string(block_start,
					      &string,
					      source_p - block_start - 1);
				block_start = source_p;
				break;
			}
			append_string(block_start,
				      &string,
				      source_p - block_start);
			source->string.text.p = ++source_p;
			UNCACHE_SOURCE();
			expand_macro(source, &string, current_string, cmd);
			CACHE_SOURCE(0);
			block_start = source_p;
			source_p--;
			break;
		case parenleft_char:
			/* Allow nested pairs of () in the macro name */
			if (closer == (int) parenright_char) {
				closer_level++;
			}
			break;
		case braceleft_char:
			/* Allow nested pairs of {} in the macro name */
			if (closer == (int) braceright_char) {
				closer_level++;
			}
			break;
		case parenright_char:
		case braceright_char:
			/* End of the name. Save the string in the macro name*/
			/* string */
			if ((*source_p == closer) && (--closer_level <= 0)) {
				source->string.text.p = source_p + 1;
				append_string(block_start,
					      &string,
					      source_p - block_start);
				goto get_macro_value;
			}
			break;
		}
		quote_seen = 0;
	}
	/* We got the macro name. We now inspect it to see if it */
	/* specifies any translations of the value */
get_macro_value:
	name = NULL;
	/* First check if we have a $(@D) type translation */
	if ((char_semantics[string.buffer.start[0]] &
	     (int) special_macro_sem) &&
	    (string.text.p - string.buffer.start >= 2) &&
	    ((string.buffer.start[1] == 'D') ||
	     (string.buffer.start[1] == 'F'))) {
		switch (string.buffer.start[1]) {
		case 'D':
			extraction = dir_extract;
			break;
		case 'F':
			extraction = file_extract;
			break;
		default:
			fatal_reader("Illegal macro reference `%s'",
				     string.buffer.start);
		}
		/* Internalize the macro name using the first char only */
		name = GETNAME(string.buffer.start, 1);
		(void) strcpy(string.buffer.start, string.buffer.start+2);
	}
	/* Check for other kinds of translations */
	if ((colon = strchr(string.buffer.start, (int) colon_char)) != NULL) {
		/* We have a $(FOO:.c=.o) type translation. Get the */
		/* name of the macro proper */
		if (name == NULL) {
			name = GETNAME(string.buffer.start,
				       colon - string.buffer.start);
		}
		/* Pickup all the translations */
		if ((colon[1] == 's') &&
		    (colon[2] == 'h') &&
		    (colon[3] == (int) nul_char)){
			replacement = sh_replace;
		} else if ((percent = strchr(colon + 1, (int) percent_char))
			   == NULL) {
			while (colon != NULL) {
				if ((eq = strchr(colon + 1,
						 (int) equal_char)) == NULL) {
					fatal("= missing from := transformation");
				}
				left_tail_len = eq - colon - 1;
				left_tail = alloca(left_tail_len + 1);
				(void) strncpy(left_tail,
					       colon + 1,
					       eq - colon - 1);
				left_tail[eq - colon - 1] = (int) nul_char;
				replacement = suffix_replace;
				if ((colon = strchr(eq + 1,
						    (int) colon_char)) !=
				    NULL) {
					right_tail = alloca(colon - eq);
					(void) strncpy(right_tail,
						       eq + 1,
						       colon - eq - 1);
					right_tail[colon - eq - 1] =
					  (int) nul_char;
				} else {
					right_tail = alloca(strlen(eq) + 1);
					(void) strcpy(right_tail, eq + 1);
				}
			}
		} else {
			if ((eq = strchr(colon + 1, (int) equal_char)) ==
			    NULL) {
				fatal("= missing from := transformation");
			}
			if ((percent = strchr(colon + 1,
					      (int) percent_char)) == NULL) {
				fatal("%% missing from := transformation");
			}
			if (eq < percent) {
				fatal("%% missing from pattern of := transformation");
			}

			if (percent > colon+1) {
				left_head = alloca(percent-colon);
				(void) strncpy(left_head,
					       colon+1,
					       percent-colon-1);
				left_head[percent-colon-1] = (int) nul_char;
				left_head_len = percent-colon-1;
			} else {
				left_head = NULL;
				left_head_len = 0;
			}

			if (eq > percent+1) {
				left_tail = alloca(eq-percent);
				(void) strncpy(left_tail,
					       percent+1,
					       eq-percent-1);
				left_tail[eq-percent-1] = (int) nul_char;
				left_tail_len = eq-percent-1;
			} else {
				left_tail = NULL;
				left_tail_len = 0;
			}

			if ((percent = strchr(++eq,
					      (int) percent_char)) == NULL) {

				right_hand[0] = alloca(strlen(eq)+1);
				right_hand[1] = NULL;
				(void) strcpy(right_hand[0], eq);
			} else {
				i = 0;
				do {
					right_hand[i] = alloca(percent-eq+1);
					(void) strncpy(right_hand[i],
						       eq,
						       percent-eq);
					right_hand[i][percent-eq] =
					  (int) nul_char;
					if (i++ >= VSIZEOF(right_hand)) {
						fatal("Too many %% in pattern");
					}
					eq = percent + 1;
					if (eq[0] == (int) nul_char) {
						right_hand[i] = "";
						i++;
						break;
					}
				} while ((percent =
					  strchr(eq,
						 (int) percent_char)) != NULL);
				if (eq[0] != (int) nul_char) {
					right_hand[i] = alloca(strlen(eq)+1);
					(void) strcpy(right_hand[i], eq);
					i++;
				}
				right_hand[i] = NULL;
			}
			replacement = pattern_replace;
		}
	}
	if (name == NULL) {
		/* No translations found. Use the whole string as */
		/* the macro name */
		name = GETNAME(string.buffer.start,
			      string.text.p - string.buffer.start);
	}
	if (string.free_after_use) {
		retmem(string.buffer.start);
	}
	if (name == make) {
		make_word_mentioned = true;
	}
	if (name == query) {
		query_mentioned = true;
	}
	/* Get the macro value */
	macro = get_prop(name->prop, macro_prop);
	if ((macro != NULL) && macro->body.macro.is_conditional) {
		conditional_macro_used = true;
		if (makefile_type == reading_makefile) {
			warning("Possibly conditional macro `%s' referenced when reading makefile",
					name->string);
		}
	}
	/* Macro name read and parsed. Expand the value. */
	if ((macro == NULL) || (macro->body.macro.value == NULL)) {
		/* If the value is empty we just get out of here */
		goto exit;
	}
	if (replacement == sh_replace) {
		/* If we should do a :sh transform we expand the command */
		/* and process it */
		INIT_STRING_FROM_STACK(string, buffer);
		/* Expand the value into a local string buffer and run cmd */
		expand_value_with_daemon(macro, &string, cmd);
		sh_command2string(&string, destination);
	} else if ((replacement != no_replace) || (extraction != no_extract)) {
		/* If there were any transforms specified in the macro */
		/* name we deal with them here */
		INIT_STRING_FROM_STACK(string, buffer);
		/* Expand the value into a local string buffer */
		expand_value_with_daemon(macro, &string, cmd);
		/* Scan the expanded string */
		p = string.buffer.start;
		while (*p != (int) nul_char) {
			char	chr;

			/* First skip over any white space and append */
			/* that to the destination string */
			block_start = p;
			while ((*p != (int) nul_char) && isspace(*p)) {
				p++;
			}
			append_string(block_start,
				      destination,
				      p - block_start);
			/* Then find the end of the next word */
			block_start = p;
			while ((*p != (int) nul_char) && !isspace(*p)) {
				p++;
			}
			/* If we cant find another word we are done */
			if (block_start == p) {
				break;
			}
			/* Then apply the transforms to the word */
			INIT_STRING_FROM_STACK(extracted, extracted_string);
			switch (extraction) {
			case dir_extract:
				/* $(@D) type transform. Extract the */
				/* path from the word. Deliver "." if */
				/* none is found */
				if (p != NULL) {
					chr = *p;
					*p = (int) nul_char;
				}
				eq = strrchr(block_start, (int) slash_char);
				if (p != NULL) {
					*p = chr;
				}
				if ((eq == NULL) || (eq > p)) {
					append_string(".", &extracted, 1);
				} else {
					append_string(block_start,
						      &extracted,
						      eq - block_start);
				}
				break;
			case file_extract:
				/* $(@F) type transform. Remove the path */
				/* from the word if any */
				if (p != NULL) {
					chr = *p;
					*p = (int) nul_char;
				}
				eq = strrchr(block_start, (int) slash_char);
				if (p != NULL) {
					*p = chr;
				}
				if ((eq == NULL) || (eq > p)) {
					append_string(block_start,
						      &extracted,
						      p - block_start);
				} else {
					append_string(eq + 1,
						      &extracted,
						      p - eq - 1);
				}
				break;
			case no_extract:
				append_string(block_start,
					      &extracted,
					      p - block_start);
				break;
			}
			switch (replacement) {
			case suffix_replace:
				/* $(FOO:.o=.c) type transform. Maybe */
				/* replace the tail of the word */
				if (((extracted.text.p -
				      extracted.buffer.start) >=
				     left_tail_len) &&
				    IS_EQUALN(extracted.text.p - left_tail_len,
					      left_tail,
					      left_tail_len)) {
					append_string(extracted.buffer.start,
						      destination,
						      (extracted.text.p -
						       extracted.buffer.start)
						      - left_tail_len);
					append_string(right_tail,
						      destination,
						      FIND_LENGTH);
				} else {
					append_string(extracted.buffer.start,
						      destination,
						      FIND_LENGTH);
				}
				break;
			case pattern_replace:
				/* $(X:a%b=c%d) type transform */
				if (((extracted.text.p -
				      extracted.buffer.start) >=
				     left_head_len+left_tail_len) &&
				    IS_EQUALN(left_head,
					      extracted.buffer.start,
					      left_head_len) &&
				    IS_EQUALN(left_tail,
					      extracted.text.p - left_tail_len,
					      left_tail_len)) {
					i = 0;
					while (right_hand[i] != NULL) {
						append_string(right_hand[i],
							      destination,
							      FIND_LENGTH);
						i++;
						if (right_hand[i] != NULL) {
							append_string(extracted.buffer.
								      start +
								      left_head_len,
								      destination,
								      (extracted.text.p - extracted.buffer.start)-left_head_len-left_tail_len);
						}
					}
				} else {
					append_string(extracted.buffer.start,
						      destination,
						      FIND_LENGTH);
				}
				break;
			case no_replace:
				append_string(extracted.buffer.start,
					      destination,
					      FIND_LENGTH);
				break;
			    }
		}
		if (string.free_after_use) {
			retmem(string.buffer.start);
		}
	} else {
		/* This is for the case when the macro name did not */
		/* specify transforms */
		expand_value_with_daemon(macro, destination, cmd);
	}
exit:
	*destination->text.p = (int) nul_char;
	destination->text.end = destination->text.p;
}
