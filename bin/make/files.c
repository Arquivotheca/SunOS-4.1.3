#ident "@(#)files.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	files.c
 *
 *	Various file related routines:
 *		Figure out if file exists
 *		Wildcard resolution for directory reader
 *		Directory reader
 */

/*
 * Included files
 */
#include "defs.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dir.h>
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

/*
 * File table of contents
 */
extern	time_t		exists();
extern	time_t		vpath_exists();
extern	int		read_dir();
extern	Name		enter_file_name();
extern	Boolean		star_match();
extern	Boolean		amatch();
  
/*
 *	exists(target)
 *
 *	Figure out the timestamp for one target.
 *
 *	Return value:
 *				The time the target was created
 *
 *	Parameters:
 *		target		The target to check
 *
 *	Global variables used:
 *		debug_level	Should we trace the stat call?
 *		recursion_level	Used for tracing
 *		vpath_defined	Was the variable VPATH defined in environment?
 */
time_t
exists(target)
	register Name		target;
{
	struct	stat             buf;
	register int		result;

	/* We cache stat information */
	if (target->stat.time != (int) file_no_time)
		return target->stat.time;

	/* If the target is a member we have to extract the time */
	/* from the archive */
	if (target->is_member &&
	    (get_prop(target->prop, member_prop) != NULL)) {
		return read_archive(target);
	}

	if (debug_level > 1) {
		(void) printf("%*sstat(%s)\n",
			      recursion_level,
			      "",
			      target->string);
	}

	result = stat_vroot(target->string, &buf, NULL, VROOT_DEFAULT);
	if (result < 0) {
		target->stat.time = (int) file_doesnt_exist;
		target->stat.errno = errno;
		if ((errno == ENOENT) &&
		    vpath_defined &&
		    (strchr(target->string, (int) slash_char) == NULL)) {
			return vpath_exists(target);
		}
	} else {
		/* Save all the information we need about the file */
		target->stat.errno = 0;
		target->stat.is_file = true;
		target->stat.mode = buf.st_mode & 0777;
		target->stat.size = buf.st_size;
		target->stat.is_dir =
			BOOLEAN((buf.st_mode & S_IFMT) == S_IFDIR);
		if (target->stat.is_dir) {
			target->stat.time = (int) file_is_dir;
		} else {
			target->stat.time = buf.st_mtime;
		}
	}
	if ((target->colon_splits > 0) &&
	    (get_prop(target->prop, time_prop) == NULL)) {
		append_prop(target, time_prop)->body.time.time =
		  target->stat.time;
	}
	return target->stat.time;
}

/*
 *	vpath_exists(target)
 *
 *	Called if exists() discovers that there is a VPATH defined.
 *	This function stats the VPATH translation of the target.
 *
 *	Return value:
 *				The time the target was created
 *
 *	Parameters:
 *		target		The target to check
 *
 *	Global variables used:
 *		vpath_name	The Name "VPATH", used to get macro value
 */
static time_t
vpath_exists(target)
	register Name		target;
{
	char			file_name[MAXPATHLEN];
	char			*name_p;
	char			*vpath;
	Name			alias;

	vpath = getvar(vpath_name)->string;

	while (*vpath != (int) nul_char) {
		name_p = file_name;
		while ((*vpath != (int) colon_char) &&
		       (*vpath != (int) nul_char)) {
			*name_p++ = *vpath++;
		}
		*name_p++ = (int) slash_char;
		(void) strcpy(name_p, target->string);
		alias = GETNAME(file_name, FIND_LENGTH);
		if (exists(alias) != (int) file_doesnt_exist) {
			target->stat.is_file = true;
			target->stat.mode = alias->stat.mode;
			target->stat.size = alias->stat.size;
			target->stat.is_dir = alias->stat.is_dir;
			target->stat.time = alias->stat.time;
			maybe_append_prop(target, vpath_alias_prop)->
						body.vpath_alias.alias = alias;
			target->has_vpath_alias_prop = true;
			return alias->stat.time;
		}
		while ((*vpath != (int) nul_char) &&
		       ((*vpath == (int) colon_char) || isspace(*vpath))) {
			vpath++;
		}
	}
	return target->stat.time;
}

/*
 *	read_dir(dir, pattern, line, library)
 *
 *	Used to enter the contents of directories into makes namespace.
 *	Precence of a file is important when scanning for implicit rules.
 *	read_dir() is also used to expand wildcards in dependency lists.
 *
 *	Return value:
 *				Non-0 if we found files to match the pattern
 *
 *	Parameters:
 *		dir		Path to the directory to read
 *		pattern		Pattern for that files should match or NULL
 *		line		When we scan using a pattern we enter files
 *				we find as dependencies for this line
 *		library		If we scan for "lib.a(<wildcard-member>)"
 *
 *	Global variables used:
 *		debug_level	Should we trace the dir reading?
 *		dot		The Name ".", compared against
 *		sccs_dir_path	The path to the SCCS dir (from PROJECTDIR)
 *		vpath_defined	Was the variable VPATH defined in environment?
 *		vpath_name	The Name "VPATH", use to get macro value
 */
int
read_dir(dir, pattern, line, library)
	Name			dir;
	char			*pattern;
	Property		line;
	char			*library;
{
	Name			file;
	char			file_name[MAXPATHLEN];
	char			*file_name_p = file_name;
	Name			plain_file;
	char			plain_file_name[MAXPATHLEN];
	char			*plain_file_name_p;
	register struct direct	*dp;
	DIR			*dir_fd;
	char			*vpath = NULL;
	char			*p;
	int			result = 0;

	/* A directory is only read once unless we need to expand wildcards */
	if (pattern == NULL) {
		if (dir->has_read_dir) {
			return 0;
		}
		dir->has_read_dir = true;
	}
	/* Check if VPATH is active and setup list if it is */
	if (vpath_defined && (dir == dot)) {
		vpath = getvar(vpath_name)->string;
	}

	/* Prepare the string where we build the full name of the */
	/* files in the directory */
	if ((dir->hash.length > 1) || (dir->string[0] != (int) period_char)) {
		(void) strcpy(file_name, dir->string);
		(void) strcat(file_name, "/");
		file_name_p = file_name + strlen(file_name);
	}

	/* Open the directory */
    vpath_loop:
	dir_fd = opendir(dir->string);
	if (dir_fd == NULL) {
		return 0;
	}

	/* Read all the directory entries */
	while ((dp = readdir(dir_fd)) != NULL) {
		/* We ignore "." and ".." */
		if ((dp->d_fileno == 0) ||
		    ((dp->d_name[0] == (int) period_char) &&
		     ((dp->d_name[1] == 0) ||
		      ((dp->d_name[1] == (int) period_char) &&
		       (dp->d_name[2] == 0))))) {
			continue;
		}
		/* Build the full name of the file using whatever */
		/* path supplied to the function  */
		(void) strcpy(file_name_p, dp->d_name);
		file = enter_file_name(file_name, library);
		if ((pattern != NULL) && amatch(dp->d_name, pattern)) {
			/* If we are expanding a wildcard pattern we enter */
			/* the file as a dependency for the target */
			if (debug_level > 0){
				(void) printf("'%s: %s' due to %s expansion\n",
					      line->body.line.target->string,
					      file->string,
					      pattern);
			}
			enter_dependency(line, file, false);
			result++;
		} else {
			/* If the file has an SCCS/s. file we will */
			/* detect that later on  */
			file->stat.has_sccs = no_sccs;
		}
	}
	(void) closedir(dir_fd);
	if ((vpath != NULL) && (*vpath != (int) nul_char)) {
		while ((*vpath != (int) nul_char) &&
		       (isspace(*vpath) || (*vpath == (int) colon_char))) {
			vpath++;
		}
		p = vpath;
		while ((*vpath != (int) colon_char) &&
		       (*vpath != (int) nul_char)) {
			vpath++;
		}
		if (vpath > p) {
			dir = GETNAME(p, vpath - p);
			goto vpath_loop;
		}
	}

/* Now read the SCCS directory */
/* Files in the SCSC directory are considered to be part of the set of files */
/* in the plain directory. They are also entered in their own right */
/* Prepare the string where we build the true name of the SCCS files */
	if (sccs_dir_path != NULL) {
		char		path[MAXPATHLEN];

		(void) strncpy(plain_file_name,
			       file_name,
			       file_name_p - file_name);
		plain_file_name[file_name_p - file_name] = 0;
		plain_file_name_p = plain_file_name + strlen(plain_file_name);
		if (file_name_p - file_name > 0) {
			(void) sprintf(path, "%s/%*s/SCCS",
				       sccs_dir_path,
				       file_name_p - file_name,
				       file_name);
		} else {
			(void) sprintf(path, "%s/SCCS", sccs_dir_path);
		}
		(void) strcpy(file_name, path);
	} else {
		(void) strncpy(plain_file_name,
			       file_name,
			       file_name_p - file_name);
		plain_file_name[file_name_p - file_name] = 0;
		plain_file_name_p = plain_file_name + strlen(plain_file_name);
		(void) strcpy(file_name_p, "SCCS");
	}
	/* Internalize the constructed SCCS dir name */
	(void) exists(dir = GETNAME(file_name, FIND_LENGTH));
	/* Just give up if the directory file doesnt exist. */
	if (!dir->stat.is_file) {
		return result;
	}
	/* Open the directory */
	dir_fd = opendir(dir->string);
	if (dir_fd == NULL) {
		return result;
	}
	(void) strcat(file_name, "/");
	file_name_p = file_name + strlen(file_name);

	while ((dp = readdir(dir_fd)) != NULL) {
		if ((dp->d_fileno == 0) ||
		    ((dp->d_name[0] == (int) period_char) &&
		     ((dp->d_name[1] == 0) ||
		      ((dp->d_name[1] == (int) period_char) &&
		       (dp->d_name[2] == 0))))) {
			continue;
		}
		/* Construct and internalize the true name of the SCCS file */
		(void) strcpy(file_name_p, dp->d_name);
		file = GETNAME(file_name, FIND_LENGTH);
		file->stat.is_file = true;
		file->stat.has_sccs = no_sccs;
		/* If this is a s. file we also enter it as if it existed in */
		/* the plain directory */
		if ((dp->d_name[0] == 's') &&
		    (dp->d_name[1] == (int) period_char)) {
			(void) strcpy(plain_file_name_p, dp->d_name + 2);
			plain_file = GETNAME(plain_file_name, FIND_LENGTH);
			plain_file->stat.is_file = true;
			plain_file->stat.has_sccs = has_sccs;
			/* Enter the s. file as a dependency for the */
			/* plain file */
			maybe_append_prop(plain_file, sccs_prop)->
			  body.sccs.file = file;
			if ((pattern != NULL) &&
			    amatch(dp->d_name+2, pattern)) {
				if (debug_level > 0) {
					(void) printf("'%s: %s' due to %s expansion\n",
						      line->body.line.target->
						      string,
						      plain_file->string,
						      pattern);
				}
				enter_dependency(line, plain_file, false);
				result++;
			}
		}
	}
	(void) closedir(dir_fd);
	return result;
}

/*
 *	enter_file_name(name_string, library)
 *
 *	Helper function for read_dir().
 *
 *	Return value:
 *				The Name that was entered
 *
 *	Parameters:
 *		name_string	Name of the file we want to enter
 *		library		The library it is a member of, if any
 *
 *	Global variables used:
 */
static Name
enter_file_name(name_string, library)
	char		*name_string;
	char		*library;
{
	char		buffer[STRING_BUFFER_LENGTH];
	String_rec	lib_name;
	Name		name;
	Property	prop;

	if (library == NULL) {
		name = GETNAME(name_string, FIND_LENGTH);
		name->stat.is_file = true;
		return name;
	}

	INIT_STRING_FROM_STACK(lib_name, buffer);
	append_string(library, &lib_name, FIND_LENGTH);
	append_char((int) parenleft_char, &lib_name);
	append_string(name_string, &lib_name, FIND_LENGTH);
	append_char((int) parenright_char, &lib_name);

	name = GETNAME(lib_name.buffer.start, FIND_LENGTH);
	name->stat.is_file = true;
	name->is_member = true;
	prop = maybe_append_prop(name, member_prop);
	prop->body.member.library = GETNAME(library, FIND_LENGTH);
	prop->body.member.library->stat.is_file = true;
	prop->body.member.entry = NULL;
	prop->body.member.member = GETNAME(name_string, FIND_LENGTH);
	prop->body.member.member->stat.is_file = true;
	return name;
}

/*
 *	star_match(string, pattern)
 *
 *	This is a regular shell type wildcard pattern matcher
 *	It is used when xpanding wildcards in dependency lists
 *
 *	Return value:
 *				Indication if the string matched the pattern
 *
 *	Parameters:
 *		string		String to match
 *		pattern		Pattern to match it against
 *
 *	Global variables used:
 */
static Boolean
star_match(string, pattern)
	register char		*string;
	register char		*pattern;
{
	register int		pattern_ch;

	switch (*pattern) {
	case 0:
		return succeeded;
	case bracketleft_char:
	case question_char:
	case asterisk_char:
		while (*string) {
			if (amatch(string++, pattern)) {
				return succeeded;
			}
		}
		break;
	default:
		pattern_ch = *pattern++;
		while (*string) {
			if ((*string++ == pattern_ch) &&
			    amatch(string, pattern)) {
				return succeeded;
			}
		}
		break;
	}
	return failed;
}

/*
 *	amatch(string, pattern)
 *
 *	Helper function for shell pattern matching
 *
 *	Return value:
 *				Indication if the string matched the pattern
 *
 *	Parameters:
 *		string		String to match
 *		pattern		Pattern to match it against
 *
 *	Global variables used:
 */
static Boolean
amatch(string, pattern)
	register char		*string;
	register char		*pattern;
{
	register int		string_ch;
	register int		k;
	register int		pattern_ch;
	register int		lower_bound;

top:
	for (; 1; pattern++, string++) {
		lower_bound = 077777;
		string_ch = *string;
		switch (pattern_ch = *pattern) {
		case bracketleft_char:
			k = 0;
			while ((pattern_ch = *++pattern) != 0) {
				switch (pattern_ch) {
				case bracketright_char:
					if (!k) {
						return failed;
					}
					string++;
					pattern++;
					goto top;
				case hyphen_char:
					k |= (lower_bound <= string_ch) &&
					  (string_ch <=
					   (pattern_ch = pattern[1]));
				default:
					if (string_ch ==
					    (lower_bound = pattern_ch)) {
						k++;
					}
				}
			}
			return failed;
		case asterisk_char:
			return star_match(string, ++pattern);
		case 0:
			return BOOLEAN(!string_ch);
		case question_char:
			if (string_ch == 0) {
				return failed;
			}
			break;
		default:
			if (pattern_ch != string_ch) {
				return failed;
			}
			break;
		}
	}
	/* NOTREACHED */
}
