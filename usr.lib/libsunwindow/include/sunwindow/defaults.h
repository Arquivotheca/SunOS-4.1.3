/*	"@(#)defaults.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef	defaults_defined
#define	defaults_defined

/*
 * This file contains routine definitions for defaults.c
 */

/*
 * NOTE: Any returned string pointers should be considered temporary at best.
 * If you want to hang onto the data, make your own private copy of the string!
 */

#include <sunwindow/sun.h>	/* Definition for Bool */

struct _default_pairs {
	char	*name;	/* Name of pair */
	int	value;	/* Value of pair */
};
typedef struct _default_pairs Defaults_pairs;

#define	DEFAULTS_UNDEFINED	"\255Undefined\255"	/* Undefined string */

/*
 * defaults_exists(path_name, status) will return TRUE if Path_Name exists
 * in the database.
 */
Bool defaults_exists();

/*
 * defaults_get_boolean(path_name, default, status) will lookup Path_Name
 * in the defaults database and return TRUE if the value is "True", "Yes",
 * "On", "Enabled", "Set", "Activated", or "1".  FALSE will be returned if
 * the value is "False", "No", "Off", "Disabled", "Reset", "Cleared",
 * "Deactivated", or "0".  If the value is none of the above, a warning
 * message will be displayed and Default will be returned.
 */
Bool defaults_get_boolean();

/*
 * defaults_get_character(path_name, default, status) will lookup Path_Name in
 * the defaults database and return the resulting character value.
 * Default will be returned if any error occurs.
 */
int defaults_get_character();

/*
 * defaults_get_child(path_name, status) will return a pointer to the simple
 * name assoicated with the first child of Path_Name.  NULL will be returned,
 * if Path_Name does not exist or if Path_Name does not have a first child.
 */
char *defaults_get_child();

/*
 * defaults_get_default(path_name, default, status) will return the value
 * associated with Path_Name prior to being overridden by the clients private
 * database.  Default is returned if any error occurs.
 */
char *defaults_get_default();

/*
 * defaults_get_enum(path_name, pairs, status) will lookup the value associated
 * with Path_Name and scan the Pairs table and return the associated value.
 * If no match, can be found an error will be generated and the value
 * associated with last entry (i.e. the NULL entry) will be returned.  See,
 * defaults_lookup().
 */
int defaults_get_enum();

/*
 * defaults_get_enumeration(path_name, default, status) will lookup
 * "Path_Name/VALUE(Path_Name)" in the defaults database and return the
 * resulting string value.  Default will be returned if any error occurs.
 */
char *defaults_get_enumeration();

/*
 * defaults_get_integer(path_name, default, status) will lookup Path_Name in
 * the defaults database and return the resulting integer value.
 * Default will be returned if any error occurs.
 */
int defaults_get_integer();

/*
 * defaults_get_integer_check(path_name, default, mininum, maximum, status)
 * will lookup Path_Name in the defaults database and return the resulting
 * integer value. If the value in the database is not between Minimum and
 * Maximum (inclusive), an error message will be printed.  Default will be
 * returned if any error occurs.
 */
int defaults_get_integer_check();

/*
 * defaults_get_sibling(path_name, status) will return a pointer to the simple
 * name assoicated with the next sibling of Path_Name.  NULL will be returned,
 * if Path_Name does not exist or if Path_Name does not have an
 * next sibling.
 */
char *defaults_get_sibling();

/*
 * defaults_get_string(path_name, default, status) will lookup and return the
 * string value assocatied with Path_Name in the defaults database.
 * Default will be returned if any error occurs.
 */
char *defaults_get_string();

void defaults_init();
/*
 * initializes (reads in) the defaults database. defaults_init() will read in
 * the user's private database, i.e., .defaults file, and then read in the
 * master database as well if the value of /Defaults/Read_Defaults_Database
 * is True. defaults_init(True) will read in the master database regardless. 
 */

/*
 * defaults_lookup(name, pairs) will linearly scan the Pairs data structure
 * looking for Name.  The value associated with Name will be returned.
 * If Name can not be found in Pairs, the value assoicated with NULL will
 * be returned.  (The Pairs data structure must be terminated with NULL.)
 */
int defaults_lookup();

/*
 * defaults_move(to_path_name, from_path_name, before_flag, status) will
 * rearrange the defaults database so that From_Path_Name will internally
 * be next to To_Path_Name.  If Before_Flag is True, a call to
 * defaults_get_sibling(To_Path_Name) will return From_Path_Name.
 * If Before_Flag is False, a call to defaults_get_child(From_Path_Name)
 * will return To_Path_Name.
 */
void defaults_move();

/*
 * defaults_remove(path_name, status) will remove Path_Name and its children
 * from the defaults database.
 */
void defaults_remove();

/*
 * defaults_remove_private(path_name, status) will remove Path_Name and its
 * children from the private portion of the defaults database.  The master
 * database entries are not touched.
 */
void defaults_remove_private();

/*
 * defaults_reread(path_name, status) will reread the portion of the database
 * associated with Path_Name.
 */
void defaults_reread();

/*
 * defaults_set_character(path_name, value, status) will set Path_name to
 * Value.  Value is a character.
 */
void defaults_set_character();

/*
 * defaults_set_enumeration(path_name, value, status) will set Path_Name to
 * Value.  Value is a pointer to a string.
 */
void defaults_set_enumeration();

/*
 * defaults_set_integer(path_name, value, status) will set Path_Name to Value.
 * Value is an integer.
 */
void defaults_set_integer();

/*
 * defaults_set_prefix(prefix, status) will cause all subsequent node lookup
 * to first a node under Prefix first.  For example, if the prefix has been
 * set to "/Mumble/Frotz" and the user accesses "/Fee/Fie/Fo", first
 * "/Mumble/Frotz/Fee/Fie/Fo" will looked at and then if it is not available
 * "/Fee/Fie/Fo" will be looked at.  This is used to permit individual programs
 * to permit overriding of defaults.  If Prefix is NULL, the prefix will be
 * cleared.
 */
void defaults_set_prefix();

/*
 * defaults_set_string(path_name, value, status) will set Path_Name to Value.
 * Value is a poitner to a string.
 */
void defaults_set_string();

/*
 * defaults_special_mode() will cause the database to behave as if the entire
 * master database has been read into memory prior to reading in the private
 * database.
 */
void defaults_special_mode();

/*
 * defaults_write_all(path_name, file_name, status) will write the all of the
 * database nodes from Path_Name and below into File_Name.  Out_File is the
 * string name of the file to create.  If File_Name is NULL, "~/.defaults" will
 * be used.
 */
void defaults_write_all();

/*
 * defaults_write_changed(file_name, status) will write out all of the private
 * database entries to File_Name.  Any time a database node is set it becomes
 * part of the private database.  Out_File is the string  name of the file to
 * create.  If File_Name is NULL, "~/.defaults" will be used.
 */
void defaults_write_changed();

/*
 * defaults_write_differences(file_name, status) will write out all of the
 * database entries that differ from the master database.  Out_File is the
 * string name of the file to create.  If File_Name is NULL, "~/.defaults"
 * be used.
 */
void defaults_write_differences();

#endif
