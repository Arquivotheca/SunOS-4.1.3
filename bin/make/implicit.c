#ident "@(#)implicit.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	implicit.c
 *
 *	Handle suffix and percent rules
 */

/*
 * Included files
 */
#include "defs.h"
#include <sys/file.h>

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
extern	Doname		find_suffix_rule();
extern	Doname		find_ar_suffix_rule();
extern	Doname		find_double_suffix_rule();
extern	void		build_suffix_list();
extern	Doname		find_percent_rule();
extern	Boolean		dependency_exists();

/*
 *	find_suffix_rule(target, target_body, target_suffix, command)
 * 
 *	Does the lookup for single and double suffix rules.
 *	It calls build_suffix_list() to build the list of possible suffixes
 *	for the given target.
 *	It then scans the list to find the first possible source file that
 *	exists. This is done by concatenating the body of the target name
 *	(target name less target suffix) and the source suffix and checking
 *	if the resulting file exists.
 *
 *	Return value:
 *				Indicates if search failed or not
 *
 *	Parameters:
 *		target		The target we need a rule for
 *		target_body	The target name without the suffix
 *		target_suffix	The suffix of the target
 *		command		Pointer to slot to deposit cmd in if found
 *
 *	Global variables used:
 *		debug_level	Indicates how much tracing to do
 *		recursion_level	Used for tracing
 */
Doname
find_suffix_rule(target, target_body, target_suffix, command)
	Name			target;
	Name			target_body;
	Name			target_suffix;
	Property		*command;
{
	register Name		source;
	register Property	source_suffix;
	char			sourcename[MAXPATHLEN];
	register char		*put_suffix;
	register Property	line;
	Name			true_target = target;
	Doname			result;

	/* If the target is a constructed one for a "::" target we need to */
	/* consider that */
	if (target->has_target_prop) {
		true_target = get_prop(target->prop,
				       target_prop)->body.target.target;
	}
	if (debug_level > 1) {
		(void) printf("%*sfind_suffix_rule(%s,%s,%s)\n",
			      recursion_level,
			      "",
			      true_target->string,
			      target_body->string,
			      target_suffix->string);
	}
	if ((true_target->suffix_scan_done == true) && (*command == NULL)) {
		return build_ok;
	}
	true_target->suffix_scan_done = true;
	/* Enter all names from the directory where the target lives as */
	/* files in makes sense */
	/* This will make finding the synthesized source possible */
	read_directory_of_file(target_body);
	/* Cache the suffixes for this target suffix if not done */
	if (!target_suffix->has_read_suffixes) {
		build_suffix_list(target_suffix);
	}
	/* Preload the sourcename vector with the head of the target name */
	(void) strncpy(sourcename,
		       target_body->string,
		       (int) target_body->hash.length);
	put_suffix = sourcename + target_body->hash.length;
	/* Scan the suffix list for the target if one exists */
	if (target_suffix->has_suffixes) {
		for (source_suffix = get_prop(target_suffix->prop,
					      suffix_prop);
		     source_suffix != NULL;
		     source_suffix = get_prop(source_suffix->next,
					      suffix_prop)) {
			/* Build the synthesized source name */
			(void) strncpy(put_suffix,
				       source_suffix->body.
				       suffix.suffix->string,
				       (int) source_suffix->body.
				       suffix.suffix->hash.length);
			put_suffix[source_suffix->body.
				   suffix.suffix->hash.length] =
			  (int) nul_char;
			if (debug_level > 1) {
				(void) printf("%*sTrying %s\n",
					      recursion_level,
					      "",
					      sourcename);
			}
			source = GETNAME(sourcename, FIND_LENGTH);
			/* If the synth source file is not registered as a */
			/* file this source suffix did not match */
			if (!source->stat.is_file) {
				continue;
			}
			/* The synth source is a file. Make sure it is up to */
			/* date */
			if (dependency_exists(source,
					      get_prop(target->prop,
						       line_prop))) {
				result = source->state;
			} else {
				result = doname(source,
						source_suffix->body.
						suffix.suffix->with_squiggle,
						true);
			}
			switch (result) {
			case build_dont_know:
				/* If we still cant build the synth source */
				/* this rule is not a match, try the next one*/
				if (source->stat.time ==
				    (int) file_doesnt_exist) {
					continue;
				}
			case build_running:
				true_target->suffix_scan_done = false;
				enter_dependency(maybe_append_prop(target,
								   line_prop),
						 source,
						 false);
				return build_running;
			case build_ok:
				break;
			case build_failed:
				return build_failed;
			}
			if (debug_level > 1) {
				(void) printf("%*sFound %s\n",
					      recursion_level,
					      "",
					      sourcename);
			}
			if (source->depends_on_conditional) {
				target->depends_on_conditional = true;
			}
/* Since it is possible that the same target is built several times during */
/* the make run we have to patch the target with all information we found */
/* here . Thus the target will have an explicit rule the next time around */
			line = maybe_append_prop(target, line_prop);
			if (*command == NULL) {
				*command = line;
			}
			if ((source->stat.time >
			     (*command)->body.line.dependency_time) &&
			    (debug_level > 1)) {
				(void) printf("%*sDate(%s)=%s Date-dependencies(%s)=%s\n",
					      recursion_level,
					      "",
					      source->string,
					      time_to_string(source->
							     stat.time),
					      true_target->string,
					      time_to_string((*command)->
							     body.line.
							     dependency_time));
			}
			/* Determine if this new dependency made the target */
			/* out of date */
			(*command)->body.line.dependency_time =
			  MAX((*command)->body.line.dependency_time,
			      source->stat.time);
			if (OUT_OF_DATE(target->stat.time,
					(*command)->body.
					line.dependency_time)) {
				line->body.line.is_out_of_date = true;
				if (debug_level > 0) {
					(void) printf("%*sBuilding %s using suffix rule for %s%s because it is out of date relative to %s\n",
						      recursion_level,
						      "",
						      true_target->string,
						      source_suffix->body.
						      suffix.suffix->string,
						      target_suffix->string,
						      source->string);
				}
			}
			/* Add the implicit rule as the targets explicit rule
			 * if none actually given and register dependency.
			 * the time checking above really should be conditional
			 * on actual use of implicit rule as well
			 */
			line->body.line.sccs_command = false;
			if (line->body.line.command_template == NULL) {
				line->body.line.command_template =
				  source_suffix->body.suffix.command_template;
			}
			enter_dependency(line, source, false);
			line->body.line.target = true_target;
			/* Also make sure the rule is build with $* and $< */
			/* bound properly */
			line->body.line.star = target_body;
			line->body.line.less = source;
			line->body.line.percent = NULL;
			if (line->body.line.query == NULL) {
				line->body.line.query = ALLOC(Chain);
				line->body.line.query->next = NULL;
				line->body.line.query->name =
				  line->body.line.less;
			} else {
				Chain ch = line->body.line.query;

				for (; ch->next != NULL; ch = ch->next);
				ch->next = ALLOC(Chain);
				ch->next->next = NULL;
				ch->next->name = line->body.line.less;
			}
			return build_ok;
		}
	}
	/* Return here in case no rule matched the target */
	return build_dont_know;
}

/*
 *	find_ar_suffix_rule(target, true_target, command)
 *
 *	Scans the .SUFFIXES list and tries
 *	to find a suffix on it that matches the tail of the target member name.
 *	If it finds a matching suffix it calls find_suffix_rule() to find
 *	a rule for the target using the suffix ".a".
 *
 *	Return value:
 *				Indicates if search failed or not
 *
 *	Parameters:
 *		target		The target we need a rule for
 *		true_target	The proper name
 *		command		Pointer to slot where we stuff cmd, if found
 *
 *	Global variables used:
 *		debug_level	Indicates how much tracing to do
 *		dot_a		The Name ".a", compared against
 *		recursion_level	Used for tracing
 *		suffixes	List of suffixes used for scan (from .SUFFIXES)
 */
Doname
find_ar_suffix_rule(target, true_target, command)
	register Name		target;
	Name			true_target;
	Property		*command;
{
	register Dependency	suffix;
	register int		suffix_length;
	register int		target_end;
	Property		line;
	Name			body;
static 	Name			dot_a;

	if (dot_a == NULL) {
		dot_a = GETNAME(".a", FIND_LENGTH);
	}

	/* We compare the tail of the target name with the suffixes */
	/* from .SUFFIXES */
	target_end = (int) true_target->string + true_target->hash.length;
	if (debug_level > 1) {
		(void) printf("%*sfind_ar_suffix_rule(%s)\n",
			      recursion_level,
			      "",
			      true_target->string);
	}
	/* Scan the .SUFFIXES list to see if the target matches any of */
	/* those suffixes */
	for (suffix = suffixes; suffix != NULL; suffix = suffix->next) {
		/* Compare one suffix */
		suffix_length = suffix->name->hash.length;
		if (!IS_EQUALN(suffix->name->string,
			       (char *)(target_end - suffix_length),
			       suffix_length)) {
			goto not_this_one;
		}
		/* The target tail matched a suffix from the .SUFFIXES list. */
		/* Now check for a rule to match. */
		target->suffix_scan_done = false;
		body = GETNAME(true_target->string,
			       (int)(true_target->hash.length -
				     suffix_length));
		switch (find_suffix_rule(target,
					 body,
					 dot_a,
					 command)) {
		case build_ok:
			line = get_prop(target->prop, line_prop);
			line->body.line.star = body;
			return build_ok;
		case build_running:
			return build_running;
		}
		/* If no rule is found we try the next suffix to see if it */
		/* matched the target tail. And so on. */
		/* Go here if the suffix did not match the target tail */
	not_this_one:;			 
	}
	return build_dont_know;
}

/*
 *	find_double_suffix_rule(target, command)
 *
 *	Scans the .SUFFIXES list and tries
 *	to find a suffix on it that matches the tail of the target name.
 *	If it finds a matching suffix it calls find_suffix_rule() to find
 *	a rule for the target.
 *
 *	Return value:
 *				Indicates if scan failed or not
 *
 *	Parameters:
 *		target		Target we need a rule for
 *		command		Pointer to slot where we stuff cmd, if found
 *
 *	Global variables used:
 *		debug_level	Indicates how much tracing to do
 *		recursion_level	Used for tracing
 *		suffixes	List of suffixes used for scan (from .SUFFIXES)
 */
Doname
find_double_suffix_rule(target, command)
	register Name		target;
	Property		*command;
{
	register Dependency	suffix;
	register int		suffix_length;
	register int		target_end;
	Name			true_target = target;

	/* If the target is a constructed one for a "::" target we need to */
	/* consider that */
	if (target->has_target_prop) {
		true_target = get_prop(target->prop,
				       target_prop)->body.target.target;
	}
	/* We compare the tail of the target name with the */
	/* suffixes from .SUFFIXES */
	target_end = (int) true_target->string + true_target->hash.length;
	if (debug_level > 1) {
		(void) printf("%*sfind_double_suffix_rule(%s)\n",
			      recursion_level,
			      "",
			      true_target->string);
	}
	/* Scan the .SUFFIXES list to see if the target matches */
	/* any of those suffixes */
	for (suffix = suffixes; suffix != NULL; suffix = suffix->next) {
		/* Compare one suffix */
		suffix_length = suffix->name->hash.length;
		if (!IS_EQUALN(suffix->name->string,
			       (char *)(target_end - suffix_length),
			       suffix_length)) {
			goto not_this_one;
		}
		/* The target tail matched a suffix from the .SUFFIXES list. */
		/* Now check for a rule to match. */
		switch (find_suffix_rule(target,
					 GETNAME(true_target->string,
						 (int)(true_target->
						       hash.length -
						       suffix_length)),
					 suffix->name,
					 command)) {
		case build_ok:
			return build_ok;
		case build_running:
			return build_running;
		}
		target->suffix_scan_done = false;
		true_target->suffix_scan_done = false;
		/* If no rule is found we try the next suffix to see if it */
		/* matched the target tail. And so on. */
		/* Go here if the suffix did not match the target tail */
	not_this_one:;			 
	}
	return build_dont_know;
}

/*
 *	build_suffix_list(target_suffix)
 *
 *	Scans the .SUFFIXES list and figures out
 *	which suffixes this target can be derived from.
 *	The target itself is not know here, we just know the suffix of the
 *	target. For each suffix on the list the target can be derived iff
 *	a rule exists for the name "<suffix-on-list><target-suffix>".
 *	A list of all possible building suffixes is built, with the rule for
 *	each, and tacked to the target suffix nameblock.
 *
 *	Parameters:
 *		target_suffix	The suffix we build a match list for
 *
 *	Global variables used:
 *		debug_level	Indicates how much tracing to do
 *		recursion_level	Used for tracing
 *		suffixes	List of suffixes used for scan (from .SUFFIXES)
 *		working_on_targets Indicates that this is a real target
 */
void
build_suffix_list(target_suffix)
	register Name		target_suffix;
{
	register Dependency	source_suffix;
	char			rule_name[MAXPATHLEN];
	register Property	line;
	register Property	suffix;
	Name			rule;

	/* If this is before default.mk has been read we just return to try */
	/* again later */
	if ((suffixes == NULL) || !working_on_targets) {
		return;
	}
	if (debug_level > 1) {
		(void) printf("%*sbuild_suffix_list(%s) ",
			      recursion_level,
			      "",
			      target_suffix->string);
	}
	/* Mark the target suffix saying we cashed its list */
	target_suffix->has_read_suffixes = true;
	/* Scan the .SUFFIXES list */
	for (source_suffix = suffixes;
	     source_suffix != NULL;
	     source_suffix = source_suffix->next) {
		/* Build the name "<suffix-on-list><target-suffix>" */
		/* (a popular one would be ".c.o") */
		(void) strncpy(rule_name,
			       source_suffix->name->string,
			       (int) source_suffix->name->hash.length);
		(void) strncpy(rule_name + source_suffix->name->hash.length,
			       target_suffix->string,
			       (int) target_suffix->hash.length);
		/* Check if that name has a rule, if not it cannot match any */
		/* implicit rule scan and is ignored */
		/* The GETNAME() call only check for presence, it will not */
		/* enter the name if it is not defined */
		if (((rule = getname_fn(rule_name,
					(int) (source_suffix->name->
					       hash.length +
					       target_suffix->hash.length),
					true)) != NULL) &&
		    ((line = get_prop(rule->prop, line_prop)) != NULL)) {
			if (debug_level > 1) {
				(void) printf("%s ", rule->string);
			}
			/* This makes it possible to quickly determine it it */
			/* will pay to look for a suffix property */
			target_suffix->has_suffixes = true;
			/* Add the suffix property to the target suffix and */
			/* save the rule with it */
			/* All information the implicit rule scanner need is */
			/* save in the suffix property */
			suffix = append_prop(target_suffix, suffix_prop);
			suffix->body.suffix.suffix = source_suffix->name;
			suffix->body.suffix.command_template =
			  line->body.line.command_template;
		}
	}
	if (debug_level > 1) {
		(void) printf("\n");
	}
}

/*
 *	find_percent_rule(target, command)
 *
 *	Tries to find a rule from the list of wildcard matched rules.
 *	It scans the list attempting to match the target.
 *	For each target match it checks if the corresponding source exists.
 *	If it does the match is returned.
 *	The percent_list is built at makefile read time.
 *	Each percent rule get one entry on the list.
 *
 *	Return value:
 *				Indicates if the scan failed or not
 *
 *	Parameters:
 *		target		The target we need a rule for
 *		command		Pointer to slot where we stuff cmd, if found
 *
 *	Global variables used:
 *		debug_level	Indicates how much tracing to do
 *		percent_list	List of all percent rules
 *		recursion_level	Used for tracing
 */
Doname
find_percent_rule(target, command)
	register Name		target;
	Property		*command;
{
	register Percent	pat_list;
	String_rec		source_string;
	char			buffer[STRING_BUFFER_LENGTH];
	register Name		source;
	register Property	line;
	Name			true_target = target;
	register int		prefix_length;
	register int		suffix_length;
	Doname			result;
	Dependency		dp;

	/* If the target is constructed for a "::" target we consider that */
	if (target->has_target_prop) {
		true_target = get_prop(target->prop,
				       target_prop)->body.target.target;
	}
	if (debug_level > 1) {
		(void) printf("%*sLooking for %% rule for %s\n",
			      recursion_level,
			      "",
			      true_target->string);
	}
	for (pat_list = percent_list;
	     pat_list != NULL;
	     pat_list = pat_list->next) {
		/* Avoid infinite recursion when expanding patterns */
		if (pat_list->being_expanded == true) {
			continue;
		}
		/* Compare the target name with the head/tail of pattern */
		/* If the pattern head/tail refs macros they are expanded */
		if (!pat_list->target_prefix->dollar) {
			prefix_length =
			  (int) pat_list->target_prefix->hash.length;
			if (!IS_EQUALN(true_target->string,
				       pat_list->target_prefix->string,
				       prefix_length)) {
				continue;
			}
		} else {
			INIT_STRING_FROM_STACK(source_string, buffer);
			expand_value(pat_list->target_prefix,
				     &source_string,
				     false);
			prefix_length =
			  source_string.text.p-source_string.buffer.start;
			if (!IS_EQUALN(true_target->string,
				       source_string.buffer.start,
				       prefix_length)) {
				continue;
			}
		}
		if (!pat_list->target_suffix->dollar) {
			suffix_length = pat_list->target_suffix->hash.length;
			if (!IS_EQUAL(true_target->string +
				       true_target->hash.length -
				       pat_list->target_suffix->hash.length,
				      pat_list->target_suffix->string)) {
				continue;
			}
		} else {
			INIT_STRING_FROM_STACK(source_string, buffer);
			expand_value(pat_list->target_suffix,
				     &source_string,
				     false);
			suffix_length =
			  source_string.text.p-source_string.buffer.start;
			if (!IS_EQUAL(true_target->string +
				       true_target->hash.length -
				       suffix_length,
				      source_string.buffer.start)) {
				continue;
			}
		}
		/* The rule matched the target. Construct the source name as */
		/* "source head" + "target body" + "source tail" */
		INIT_STRING_FROM_STACK(source_string, buffer);
		if (!pat_list->source_prefix->dollar) {
			append_string(pat_list->source_prefix->string,
				      &source_string,
				      (int) pat_list->source_prefix->
				      hash.length);
		} else {
			expand_value(pat_list->source_prefix,
				     &source_string,
				     false);
		}
		if (pat_list->source_percent) {
			append_string(true_target->string + prefix_length,
				      &source_string,
				      (int) (true_target->hash.length -
					     prefix_length -
					     suffix_length));
		}
		if (!pat_list->source_suffix->dollar) {
			append_string(pat_list->source_suffix->string,
				      &source_string,
				      (int) pat_list->source_suffix->
				      hash.length);
		} else {
			expand_value(pat_list->source_suffix,
				     &source_string,
				     false);
		}
		/* Internalize the synthesized source name */
		source = GETNAME(source_string.buffer.start, FIND_LENGTH);
		if (source_string.free_after_use) {
			retmem(source_string.buffer.start);
		}
		if (debug_level > 1) {
			(void) printf("%*sTrying %s\n",
				      recursion_level,
				      "",
				      source->string);
		}
		/* Try to build the source */
		pat_list->being_expanded = true;
		if (dependency_exists(source,
				      get_prop(target->prop,
					       line_prop))) {
			result = source->state;
		} else {
			result = doname(source, true, true);
		}
		switch (result) {
		case build_dont_know:
			/* If make cant figure out how to build the source we*/
			/* just try the next percent rule */
			pat_list->being_expanded = false;
			if (source->stat.time == (int) file_doesnt_exist) {
				continue;
			}
		case build_running:
			pat_list->being_expanded = false;
			enter_dependency(maybe_append_prop(target, line_prop),
					 source,
					 false);
			return build_running;
		case build_ok:
			/* If we managed to build the source this rule */
			/* is a match */
			break;
		case build_failed:
			/* If the build of the source failed we give up */
			/* looking for a percent rule and propagate the error*/
			pat_list->being_expanded = false;
			return build_failed;
		}
		/* We matched the rule since the source exists */
		/* Now make sure "%.o: %.c" behaves the same as "foo.o:foo.c"*/
		/* by saying that the target we matched has been */
		/* mentioned in the makefile */
		if (true_target->colons == no_colon) {
			true_target->colons = one_colon;
		}
		pat_list->being_expanded = false;
		if (debug_level > 1) {
			(void) printf("%*sMatched %s: %s from %s%%%s: %s%s%s\n",
				      recursion_level,
				      "",
				      true_target->string,
				      source->string,
				      pat_list->target_prefix->string,
				      pat_list->target_suffix->string,
				      pat_list->source_prefix->string,
				      pat_list->source_percent ?
				        "%" : "",
				      pat_list->source_suffix->string);
		}
/* Since it is possible that the same target is built several times during */
/* the make run we have to patch the target with all information we found */
/* here . Thus the target will have an explicit rule the next time around */
/* Enter the synthesized source as a dependency for the target in case the */
/* target is built again */
		line = maybe_append_prop(target, line_prop);
		*command = line;
		if ((source->stat.time >
		     (*command)->body.line.dependency_time) &&
		    (debug_level > 1)) {
			(void) printf("%*sDate(%s)=%s Date-dependencies(%s)=%s\n",
				      recursion_level,
				      "",
				      source->string,
				      time_to_string(source->stat.time),
				      true_target->string,
				      time_to_string((*command)->
						   body.line.dependency_time));
		}
		/* Add all dependencies from the % rule */
		for (dp = pat_list->dependencies; dp != NULL; dp = dp->next) {
			enter_dependency(line, dp->name, false);
			if (doname_check(dp->name,
					 true,
					 true,
					 false) == build_failed) {
				return build_failed;
			}
			/* Determine if this new dependency make the */
			/* target out of date */
			(*command)->body.line.dependency_time =
			  MAX((*command)->body.line.dependency_time,
			      dp->name->stat.time);
			if (OUT_OF_DATE(true_target->stat.time,
					(*command)->body.line.dependency_time)) {
				line->body.line.is_out_of_date = true;
				if (debug_level > 0) {
					(void) printf("%*sBuilding %s using percent rule for %s%%%s: %s%s%s because it is out of date relative to %s\n",
						      recursion_level,
						      "",
						      true_target->string,
						      pat_list->target_prefix->string,
						      pat_list->target_suffix->string,
						      pat_list->source_prefix->string,
						      pat_list->source_percent ?
						      "%" : "",
						      pat_list->source_suffix->string,
						      dp->name->string);
				}
			}
		}
		/* Determine if this new dependency make the target */
		/* out of date */
		(*command)->body.line.dependency_time =
		  MAX((*command)->body.line.dependency_time,
		      source->stat.time);
		if (OUT_OF_DATE(true_target->stat.time,
				(*command)->body.line.dependency_time)) {
			line->body.line.is_out_of_date = true;
			if (debug_level > 0) {
				(void) printf("%*sBuilding %s using percent rule for %s%%%s: %s%s%s because it is out of date relative to %s\n",
					      recursion_level,
					      "",
					      true_target->string,
					      pat_list->target_prefix->string,
					      pat_list->target_suffix->string,
					      pat_list->source_prefix->string,
					      pat_list->source_percent ?
					        "%" : "",
					      pat_list->source_suffix->string,
					      source->string);
			}
		}
		/* And stuff the rule we found as an explicit rule for target*/
		line->body.line.sccs_command = false;
		line->body.line.target = true_target;
		if (line->body.line.command_template == NULL) {
			line->body.line.command_template =
			  pat_list->command_template;
			enter_dependency(line, source, false);
			/* Also make sure the rule is build with $* and $< */
			/* $* is bound to the stuff that matched the "%" */
			line->body.line.star =
			  GETNAME(true_target->string + prefix_length,
				  (int)(true_target->hash.length -
					prefix_length - suffix_length));
			line->body.line.less = source;
		}
		if (true_target->parenleft) {
			char	*left;
			char	*right;

			left = strchr(true_target->string,
				      (int) parenleft_char);
			right = strchr(true_target->string,
				       (int) parenright_char);
			if ((left == NULL) || (right == NULL)) {
				line->body.line.percent = NULL;
			} else {
				line->body.line.percent =
				  GETNAME(left+1, right-left-1);
			}
		} else {
			line->body.line.percent = NULL;
		}
		return build_ok;
	}
	/* This return is taken if no percent rule was found for the target */
	return build_dont_know;
}

/*
 *	dependency_exists(target, line)
 *
 *	Returns true if the target exists in the
 *	dependency list of the line.
 *
 *	Return value:
 *				True if target is on dependency list
 *
 *	Parameters:
 *		target		Target we scan for
 *		line		We get the dependency list from here
 *
 *	Global variables used:
 */
static Boolean
dependency_exists(target, line)
	Name		target;
	Property	line;
{
	Dependency	dp;

	if (line == NULL) {
		return false;
	}
	for (dp = line->body.line.dependencies; dp != NULL; dp = dp->next) {
		if (dp->name == target) {
			return true;
		}
	}
	return false;
}

