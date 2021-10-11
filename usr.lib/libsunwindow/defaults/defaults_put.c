#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)defaults_put.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985, 1988 by Sun Microsystems, Inc.
 */
 
#include <stdio.h>		/* Standard I/O library */
#include <pwd.h>		/* Password file stuff */
#include <sys/types.h>		/* Get u_long defined for dir.h */
#include <sys/dir.h>		/* Directory access routines */
#include <sys/stat.h>		/* File status information */
#include <sunwindow/sun.h>	/* Define True, False, etc... */
#include <sunwindow/hash.h>	/* Hash table definitions */
#include <sunwindow/parse.h>	/* Parsing routine */
#include <strings.h>
#include <sunwindow/defaults.h>
#include <sunwindow/defaults_impl.h>

/* Externally used routines: */
extern void	bomb();
extern void	clear_status();
extern void	defaults_create();
extern char	*getlogindir();
extern Name	name_copy();
extern void	name_quick_parse();
extern char	*name_unparse();
extern void	node_delete();
extern void	node_delete_private();
extern Node	node_find_name();
extern Node	node_lookup_name();
extern Node	node_lookup_path();
extern void	node_write();
extern void	node_write1();
extern Node	path_lookup();
extern void	read_master();
extern void	read_master_database();
extern void	read_private_database();
extern void	set();
extern char	*slash_append();
extern void	str_write();
extern Symbol	symbol_copy();
extern Bool	symbol_equal();
extern Symbol	symbol_lookup();
extern void	warn();
extern char	*hash_get_memory();
extern char	*getenv();



extern Defaults defaults; 

/*
 * defaults_move(to_path_name, from_path_name, before_flag, status) will
 * rearrange the defaults database so that From_Path_Name will internally
 * be next to To_Path_Name.  If Before_Flag is True, a call to
 * defaults_get_sibling(To_Path_Name) will return From_Path_Name.
 * If Before_Flag is False, a call to defaults_get_child(From_Path_Name)
 * will return To_Path_Name.
 */
void
defaults_move(to_path_name, from_path_name, before_flag, status)
	char		*to_path_name;		/* Destination path name */
	char		*from_path_name;	/* Source path_name */
	Bool		before_flag;		/* True => Insert before */
	int		*status;		/* Status flag */
{
	Node		anchor;			/* Node behind last one */
	Node		from_node;		/* From node */
	Node		parent;			/* Parent node */
	Node		temp;			/* Temporay node */
	Node		to_node;		/* To node */

	/* Check to be sure the nodes can be moved. */
	clear_status(status);
	from_node = node_lookup_path(from_path_name, False, status);
	if (from_node == NULL)
		return;
	to_node = node_lookup_path(to_path_name, False, status);
	if (to_node == NULL)
		return;
	if (from_node == to_node){
		warn("%s and %s are the same node",
			to_path_name, from_path_name);
		return;
	}
	if (from_node->parent != to_node->parent){
		warn("%s and %s are not siblings of one another",
			to_path_name, from_path_name);
		return;
	}
	parent = from_node->parent;


	/* Remove from node from database tree. */
	if (parent->child == from_node)
		parent->child = from_node->next;
	else {
		temp = parent->child;
		do {	anchor = temp;
			temp = temp->next;
		} while ((temp != NULL) && (temp != from_node));
		if (temp == NULL){
			warn("Data structure around %s and %s is messed up",
				to_path_name, from_path_name);
			return;
		}
		anchor->next = temp->next;
	}
	from_node->next = NULL;

	/* Re-insert the node. */
	if (!before_flag){
		from_node->next = to_node->next;
		to_node->next = from_node;
		return;
	}
	if (parent->child == to_node)
		parent->child = from_node;
	else {
		temp = parent->child;
		do {	anchor = temp;
			temp = temp->next;
		} while ((temp != NULL) && (temp != to_node));
		if (temp == NULL){
			warn("Data structure around %s and %s is messed up",
				to_path_name, from_path_name);
			return;
			}
		anchor->next = from_node;
	}
	from_node->next = to_node;
}

/*
 * defaults_remove(path_name, status) will remove Path_Name and its children
 * from the defaults database.
 */
void
defaults_remove(path_name, status)
	char		*path_name;	/* Full path name of node to remove */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Node to delete */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node != NULL)
		node_delete(node);
}

/*
 * defaults_remove_private(path_name, status) will remove Path_Name and its
 * children from the defaults database.
 */
void
defaults_remove_private(path_name, status)
	char		*path_name;	/* Full path name of node to remove */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Node to delete */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node != NULL)
		node_delete_private(node);
}

/*
 * defaults_set_character(path_name, value, status) will set Path_name to
 * Value.  Value is a character.
 */
void
defaults_set_character(path_name, value, status)
	char		*path_name;	/* Name to look up */
	char		value;		/* Character to set */
	int		*status;	/* Status flag */
{
	register char	*new_value;	/* New value */

	clear_status(status);
	new_value = hash_get_memory(2);
	new_value[0] = value;
	new_value[1] = '\0';
	set(path_name, new_value, status);
}

/*
 * defaults_set_enumeration(path_name, value, status) will set Path_Name to
 * Value.  Value is a pointer to a string.
 */
void
defaults_set_enumeration(path_name, value, status)
	char		*path_name;	/* Full node name */
	char		*value;		/* Enumeration value */
	int		*status;	/* Status flag */
{
	Symbol		name[MAX_NAME];	/* Temporary name */
	register Symbol	*name_pointer;	/* Name pointer */
	register Node	node;		/* Temporary Node */
	register int	size;		/* Name size */ 
	register Node	temp_node;	/* Temporary node */

	clear_status(status);
	name_pointer = name;
	name_quick_parse(path_name, name_pointer);
	node = node_find_name(name_pointer, True, status);
	if (node == NULL){
		warn("Can not insert '%s' into database", path_name);
		return;
	}
	/* type check the enum if we've got the master database */
	if (defaults->database_dir){
		size = name_length(name_pointer);
		/* is it an enum? */
		name_pointer[size] = symbol_lookup("$Enumeration");
		name_pointer[size + 1] = NULL;
		temp_node = node_lookup_name(name_pointer, True, status);
		if (temp_node == NULL){
			warn("'%s' is not an enumeration node", path_name);
			return;
		}
		/* is it a valid one? */
		name_pointer[size] = symbol_lookup(value);
		temp_node = node_lookup_name(name_pointer, True, status);
		if (temp_node == NULL){
			warn("%s can not have %s assigned as its value",
							path_name, value);
			return;
		}
	}
	node->value = symbol_copy(value);
	node->deleted = False;
	node->private = True;
}

/*
 * defaults_set_integer(path_name, value, status) will set Path_Name to Value.
 * Value is an integer.
 */
void
defaults_set_integer(path_name, value, status)
	char	*path_name;	/* Full node name */
	int	value;		/* Integer value */
	int	*status;	/* Status flag */
{
	char	temp_value[MAX_STRING];	/* Temporary string value */

	clear_status(status);
	(void)sprintf(temp_value, "%d", value);
	set(path_name, symbol_copy(temp_value), status);
}

/*
 * defaults_set_prefix(prefix, status) will cause all subsequent node lookup
 * to first a node under Prefix first.  For example, if the prefix has been
 * set to "/Mumble/Frotz" and the user accesses "/Fee/Fie/Fo", first
 * "/Mumble/Frotz/Fee/Fie/Fo" will looked at and then if it is not available
 * "/Fee/Fie/Fo" will be looked at.  This is used to permit individual programs
 * to permit overriding of defaults.  If Prefix is NULL, the prefix will be
 * cleared.
 */
void
defaults_set_prefix(prefix, status)
	register char	*prefix;	/* Prefix to use */
	int		*status;	/* Status flag */
{
	Symbol		name[MAX_NAME];	/* Temporary name */

	clear_status(status);
	if (defaults == NULL)
		defaults_init(True);
	if (prefix == NULL){
		defaults->prefix = NULL;
		return;
	}
	name_quick_parse(prefix, name);
	defaults->prefix = name_copy(name);
	defaults->test_mode =
		defaults_get_boolean("/Defaults/Test_Mode", False, (int *)NULL);
}

/*
 * defaults_set_string(path_name, value, status) will set Path_Name to Value.
 * Value is a poitner to a string.
 */
void
defaults_set_string(path_name, value, status)
	char	*path_name;	/* Full node name */
	char	*value;		/* New string value */
	int	*status;	/* Status flag */
{
	set(path_name, symbol_copy(value), status);
}
/*
 * defaults_write_all(path_name, file_name, status) will write the all of the
 * database nodes from Path_Name and below into File_Name.  Out_File is the
 * string name of the file to create.  If File_Name is NULL, env var 
 * DEFAULTS_FILE will be used.
 */
void
defaults_write_all(path_name, file_name, status)
	char	*path_name;	/* Path name */
	char	*file_name;	/* Output file name */
	int	*status;	/* Status flag */
{
	node_write(path_name, file_name, status, 0);
}

/*
 * defaults_write_changed(file_name, status) will write out all of the private
 * database entries to File_Name.  Any time a database node is set it becomes
 * part of the private database.  Out_File is the string  name of the file to
 * create.  If File_Name is NULL, env var DEFAULTS_FILE will be used.
 */
void
defaults_write_changed(file_name, status)
	char	*file_name;	/* Output file name */
	int	*status;	/* Status flag */
{
	node_write("/", file_name, status, 1);
}

/*
 * defaults_write_differences(file_name, status) will write out all of the
 * database entries that differ from the master database.  Out_File is the
 * string name of the file to create.  If File_Name is NULL, env var 
 * DEFAULTS_FILE be used.
 */
void
defaults_write_differences(file_name, status)
	char	*file_name;	/* Output file name */
	int	*status;	/* Status flag */
{
	node_write("/", file_name, status, 2);
}
/*
 * node_write(path_name, file_name, status, flag) will write Path_Name to
 * File_Name.  If File_Name is NULL, env var DEFAULTS_FILE will be used.
 */
static void
node_write(path_name, file_name, status, flag)
	char		*path_name;	/* Path name */
	char		*file_name;	/* File name */
	int		*status;	/* Status flag */
	int		flag;		/* Control flag */
{
	char		name[MAX_STRING]; /* Temporary name */
	register Node	node;		/* Node to output */
	register FILE	*out_file;	/* Output file */
	char		*login_dir;

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node == NULL)
		return;
	if (file_name == NULL){
		if ((file_name = getenv("DEFAULTS_FILE")) == NULL)  {
			file_name = name;
			if ((login_dir = getlogindir()) != NULL ) {
				(void)strcpy(file_name, getlogindir());
			}
			(void)strcat(file_name, "/.defaults");
		}   else   {
			strcpy(name,file_name);
			file_name = name;
		}
	}
	out_file = fopen(file_name, "w");
	if (out_file == NULL){
		warn("Could not open %s", file_name);
		return;
	}
	(void)fprintf(out_file, "%s %d\n", VERSION_TAG, VERSION_NUMBER);
	node_write1(out_file, node, flag);
	(void)fclose(out_file);
}

/*
 * node_write1(out_file, node, flag) will write out the contents of Node to
 * Out_File.  Flag is 0, if the entire database is to be written out.
 * Flag is 1 if only the changed entries are to be written out.  Flag is 2,
 * if only differences from the master database are to be written out.
 */
static void
node_write1(out_file, node, flag)
	register FILE	*out_file;	/* Output file */
	register Node	node;		/* Node to output */
	register int	flag;		/* TRUE => entire database */
{
	char	temp[MAX_STRING];	/* Temporary name */

	if (!node->deleted && ((flag == 0) || ((flag == 2) && node->private)
	   	|| !symbol_equal(node->value, node->default_value)
	   	|| always_save(node))
	   	) {
		(void)name_unparse(node->name, temp);
		(void)fprintf(out_file, "%s", temp);
		if (strcmp(node->value, DEFAULTS_UNDEFINED) != 0){
			(void)fprintf(out_file, "\t");
			str_write(out_file, node->value);
		}
		putc('\n', out_file);
	}
	for (node = node->child; node != NULL; node = node->next)
		node_write1(out_file, node, flag);
}

static int
always_save(node)
	register Node	node;	
{
	char	*buf[29];	/* buffer */
	Symbol	*sym;
	int	i;
	
	if (node->name == NULL)
		return (FALSE);
	for (i = 0; node->name[i] != NULL; i++)
		buf[i] = node->name[i];	/* copy name */
	buf[i++] = symbol_lookup("$Always_Save");
	buf[i] = NULL;
	return (node_lookup_name(buf, False, (int *)NULL) != NULL);
}
/*
 * set(path_name, new_value) will set Full_Name to type
 * Node_Type with value New_Value.
 */
static void
set(path_name, new_value, status)
	char		*path_name;	/* Full node name */
	char		*new_value;	/* New value */
	int		*status;	/* Status flag */
{
	Symbol		name[MAX_NAME];	/* Name */
	register Node	node;		/* Node to set */

	if (defaults == NULL)
		defaults_init(True);	

	/* Get the node assoicated with Full_Name. */
	name_quick_parse(path_name, name);
	node = node_find_name(name, True, status);
	if (node == NULL){
		warn("Can not bind node to '%s'", path_name);
		return;
	}

	/* It is not worth the effort to try to deallocate any storage. */
	node->value = new_value;
	node->private = True;
	node->deleted = False;
}

/*
 * defaults_special_mode() will cause the database to behave as if the entire
 * master database has been read into memory prior to reading in the private
 * database. This is done to insure that the order of nodes that defaultsedit
 * presents is the same as that in the .D files, regardless of what the user
 * happens to have set in his private database. 
 */
void
defaults_special_mode()
{
	char		*database_dir;	/* Database directory */
	register Node	node;		/* Current node to process */
	char		*private_dir;	/* Private directory */

	/* Create database. */
	defaults = NULL;
	defaults_create();

	/* Read in env var DEFAULTS_FILE to find out where database 
	 *directories are. 
	 */
	read_private_database();
	node = node_lookup_path("/Defaults/Directory", False, (int *)NULL);
	database_dir = (node == NULL) ? NULL : slash_append(node->value);
	node = node_lookup_path("/Defaults/Private_Directory", False, (int *)NULL);
	private_dir = (node == NULL) ? NULL : slash_append(node->value);
	/*
	 * The previous five statements may be superfluous. It appears that
	 * read_master_database does this itself. 
	 */
	
	/* Drop the entire private database on the floor! */
	defaults = NULL;
	defaults_create();
	defaults->database_dir = database_dir;
	defaults->private_dir = private_dir;

	/* Read in master database before private database so that node order
	 * is preserved in master database order rather than private order. */
	read_master_database();
	read_private_database();
	for (node = defaults->root->child; node != NULL; node = node->next)
		if (node->file)
			read_master(node->name[0]);
	defaults->test_mode =
		defaults_get_boolean("/Defaults/Test_Mode", False, (int *)NULL);
}




/*
 * str_write(out_file, string) will write String to Out_File as a C-style
 * string.
 */
static void
str_write(out_file, string)
	register FILE	*out_file;	/* Output file */
	register char	*string;	/* String to output */
{
	register int	chr;		/* Temporary character */

	putc('"', out_file);
	while (True){
		chr = *string++;
		if (chr == '\0')
			break;
		if ((' ' <= chr) && (chr <= '~') &&
		    (chr != '\\') && (chr != '"')){
			putc(chr, out_file);
			continue;
		}
		putc('\\', out_file);
		switch (chr){
		    case '\n':
			chr = 'n';
			break;
		    case '\r':
			chr = 'r';
			break;
		    case '\t':
			chr = 't';
			break;
		    case '\b':
			chr = 'b';
			break;
		    case '\\':
			chr = '\\';
			break;
		    case '\f':
			chr = 'f';
			break;
		    case '"':
			chr = '"';
			break;
		}
		if ((' ' <= chr) && (chr <= '~'))
			putc(chr, out_file);
		else	(void)fprintf(out_file, "%03o", chr & 0377);
	}
	putc('"', out_file);
}

/*
 * warn(format, arg1, arg2, arg3) will print out a warning message
 * to the console using Format, Arg1, Arg2 and Arg3.  This is all provided
 * the number of errors has not gotten too excessive.
 */
/*VARARGS1*/
void
warn(format, arg1, arg2, arg3, arg4)
	char	*format;/* Format string */
	long	arg1;	/* First argument */
	long	arg2;	/* Second argument */
	long	arg3;	/* Third argument */
	long	arg4;	/* Fourth argument */
{
        extern Defaults_pairs warn_error_action[];
	register int	action;			/* Action number */
	int		errors;			/* Number of errors */
	int		max_errors;		/* Maximum errors */
	register Node	node;			/* Error action node */
	char 		temp[MAX_STRING];	/* Temporary string */

	action = 1;
	if (defaults == NULL)
		errors = 0;
	else {	errors = ++defaults->errors;
		/* See how many errors to accept. */
		(void)strcpy(temp, SEP_STR);
		(void)strcat(temp, "Defaults");
		(void)strcat(temp, SEP_STR);
		(void)strcat(temp, "Maximum_Errors");
		node = node_lookup_path(temp, True, (int *)NULL);
		if (node == NULL)
			max_errors = MAX_ERRORS;
		else	max_errors = atoi(node->value);
		if (defaults->errors == max_errors)
			format = "Suppressing all subsequent errors";

		/* See what to do on error action. */
		(void)strcpy(temp, SEP_STR);
		(void)strcat(temp, "Defaults");
		(void)strcat(temp, SEP_STR);
		(void)strcat(temp, "Error_Action");
		node = node_lookup_path(temp, True, (int *)NULL);
		if (node != NULL)
			action = defaults_lookup(node->value, warn_error_action);
	}
	if ((errors <= max_errors) && (action < 3)){
		(void)fprintf(stderr, "SunDefaults Error: ");
		(void)fprintf(stderr, format, arg1, arg2, arg3, arg4);
		(void)fprintf(stderr, "\n");
		(void)fflush(stderr);
	}
	if (action == 2)
		bomb();
}

/*****************************************************************/
/* Internal routines:                                            */
/*****************************************************************/

/*
 * bomb() will cause the program to bomb.
 */
static void
bomb()
{
	(void)fflush(stderr);
	usleep(5 << 20);	/* Wait for error message to show up */
	abort();
}
