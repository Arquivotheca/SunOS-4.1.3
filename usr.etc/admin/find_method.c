/*
 * This module provides three functions for searching paths
 * for the administrative object management library
 *
 * find_method()
 *	Given a class and method, return path to executable.
 * find_object_class()
 *	searches a path variable for a directory containing the classes
 * find_class()
 *	Given a class name, find subdirectory
 *
 *	Return conditions for functions are
 *		0	AOK
 *		1	Nope: try again
 *	       -1	Whoops!  Error condition!
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint 
static  char sccsid[] = "@(#)find_method.c 1.1 92/07/30 SMI"; 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/param.h>
	
#include "find_method_impl.h"	/* Defines error strings and numbers */
#include "admin_messages.h"	/* Error message definitions */

extern char *getenv();
extern void admin_write_err();
extern int  errno;

static char *path_to_object;

/* Function declarations */

char *find_class(); 
char *find_method(); 
char *find_object_class();
static int try_path(); 
static int search_dir();
static void build_path();
static void remove_component();

/*
 * find_method() takes two strings: a class name and a method
 * name.  It searches for an image to run, starting with the
 * class directory and working upwards in order to inherit methods
 * Calls function find_class
 *
 * Bugs
 * 	Currently do not check that file is executable
 *	Currently break off the search at the object class:
 *		so nothing inherits from object. Kludge
 */ 

char *
find_method(class_name, method_name)
	char *class_name;
	char *method_name;
{
	static char full_path[MAXPATHLEN];
	static char partial_path[MAXPATHLEN];
	char *p;

#ifdef DEBUG
	(void) printf ( "find method is looking for method %s in class %s\n",
		method_name, class_name );
#endif

	p = find_class(class_name);
	if (p == NULL) {
		admin_write_err(OM_ERRNOCLASS, class_name);
		return ( (char *)NULL );
	}

	(void) strcpy(partial_path, p);

#ifdef DEBUG
	(void) printf ( "find method will try %s\n", partial_path );
#endif

	/*
	 * each time through the loop, we try a directory.  
         * If we fail, we chop off last part of partial path 
	 * and try again.
         */
	while ( !(strcmp(partial_path, "") == 0)) {
		(void) strcpy ( full_path, path_to_object);
		build_path ( full_path, partial_path);
#ifdef DEBUG
		(void) printf ( "find method will try %s\n", full_path );
#endif

		if(search_dir(full_path, method_name) == FOUND) {
			build_path(full_path, method_name);
			return (full_path);
		} else {
			remove_component(partial_path);	
		}
	}

	/* Fell through: partial path is null.  Arrived at object dir */
	/* Note that this prevents inheritance from object class */
	admin_write_err(OM_ERRNOMETHOD, class_name, method_name);
	return ( (char *)NULL );
}

/*
 * find_class() takes a string, and searches for a subdirectory 
 * of the object directory of that name 
 * Returns pathname for class directory, or null string
 *
 * Bugs: currently this assumes that there is a file
 * in the object directory with the right items in it.
 * We may alter this design
 */

char *
find_class(class_name)
	char *class_name;
{
        FILE *fp;
        char *next_class;
        char *path_name;
	static char buf[BUFSIZE];
	static char file_name[MAXPATHLEN];
        static char terminal[] = "\n\t ";		/* Skip white space */

	if (path_to_object == NULL) {
		path_to_object = find_object_class();
	 	if (path_to_object == NULL ) {
			admin_write_err(OM_ERRNOOBJECT);
			return ( (char *)NULL );
		}
	}

#ifdef DEBUG
	(void) printf ("find_class is looking for class %s\n", class_name);
#endif
	(void) strcpy(file_name, path_to_object);
	build_path(file_name, CLASSFILE);
	
        fp = fopen (file_name, READONLY );
        if (fp == NULL) { 
               	admin_write_err ( OM_ERRNOCLASS, class_name);
                return ( (char *)NULL );
        } 

        while ( fgets(buf, BUFSIZE, fp) != NULL) {
                next_class = strtok(buf, terminal);
		if (strcmp(next_class, class_name) == 0) {
			path_name = strtok((char *)NULL, terminal);
			if (path_name == NULL) {
				break;
			} else {
				return (path_name);
			}
		}
        }
	admin_write_err ( OM_ERRNOCLASS, class_name);
	return ( (char *)NULL );
}

/*
 * find_object_class() searches for a directory called object, 
 *
 * Returns path or null string
 */

char *
find_object_class()
{
	static char object_path[MAXPATHLEN];
	static char *next;
	char *source;
	int err;
	static char terminal[] = ":";	/* separator in path variable */ 
 	static char defaultpath[] = "/usr/etc/admin";

	source = getenv(PATHENVV);
	if ( source == NULL) 
		source = defaultpath;

	for(;;) {
	
		next = strtok(source, terminal);
		if (!next)
			return( (char *)NULL );

		source = NULL;

		if ( (err = search_dir(next, OBJ)) == FOUND ) {
			(void) strcpy(object_path, next);
			build_path(object_path, OBJ);
			return (object_path);
		}
		else if (err == ERROR ) {
			return ( (char *)NULL );	/* Error condition */
		}
	}
}

/*
 * search_dir()	takes a path name and a file name and returns
 * a bit telling if the file was found in the directory 
 */

static int
search_dir(path, file_name)
	char *path;
	char *file_name;
{
	DIR *dir_d_p;		/* Pointer to directory descriptor */		
 	struct dirent *dir_e_p;	/* Pointer to directory entry 	*/
	int err;		/* Error condition */
#ifdef DEBUG
	(void) printf ( "search_dir attempts %s for %s \n", path, file_name );
#endif

	if ((dir_d_p = opendir(path)) == NULL)	{
		admin_write_err(OM_ERROPENDIR, path);
		return(ERROR);
	}
	/* 
	 * Step through the directory, looking at each file
	 * Bugs: we don't check the type of the file yet
	 */
	for (errno = 0; (dir_e_p = readdir(dir_d_p)) != NULL; errno = 0) {
		if (strcmp(dir_e_p->d_name, file_name) == 0 ) { 

			err = closedir(dir_d_p);
			if (err != 0)	{
				admin_write_err(OM_ERRCLOSEDIR, path);
			}

			return (FOUND);
		}
	}
	if (errno == 0)	
		return (NOTFOUND);
	else {
		admin_write_err(OM_ERRREADDIR, path);
		return (ERROR);
	}
}

/*
 * remove_component() takes a string, and removes the last
 * path component.  Given /etc/admin/obj/str it returns /etc/admin/obj
 * Destroys path
 */

static void 
remove_component(path)
	char *path;
{
	char *p;

	p = strrchr(path, SEPARATOR); 		/* find last '/' 	*/
	if (p == NULL) {
		*path = '\0';			/* set path to null str	*/
	} else {
		*p = '\0';			/* zap it 		*/
	}
}


/*
 * build_path() takes two strings and glues them together with SLASH
 * It overwrites the first parameter.
 */

static void
build_path( s1, s2)
	char *s1, *s2;
{
	(void) strcat( strcat(s1, SLASH), s2);
}

