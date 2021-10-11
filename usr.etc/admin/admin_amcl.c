#ifndef lint
static	char sccsid[] = "@(#)admin_amcl.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 *	This file contains the functions implementing the Administrative
 *	Methods Client Library.  This library is used within the administrative
 *	framework to invoke administrative class methods.
 *	Exported interfaces currently supported:
 *
 *	    admin_perf_method()
 *
 *		Perform a requested administrative class method on a
 *		specified host and domain.
 *
 * 	    admin_write_err()
 *
 *		Error messaging interface alternative to perror.
 *
 *	Bugs:	admin_perf_method() will hang if the requested method
 *		attempts to generate more than {PIPE_BUF} (see pipe(2v))
 *		bytes of error information on STDERR.
 */

#include <sys/types.h>
#include <malloc.h>
#include <varargs.h>
#include <stdio.h>
#include <sys/wait.h>

#include "admin_amcl.h"		/* Exported AMCL definitions */
#include "admin_amcl_impl.h"	/* Private AMCL definitions */
#include "admin_messages.h"	/* Message definitions */

extern int errno;

void	admin_write_err();
caddr_t	admin_make_errbuf();
static int	build_argv();
static void	close_pipe();
static int	collect_results();
static int	exec_method_local();

/*
 * admin_perf_method(class, method_name, host, domain, argp, output_pp, error_pp,
 *		     [ option1, option2, ..., ] ADMIN_END_OPTIONS):
 *
 *	Perform the requested administrative class method on the specified
 *	host and domain.  The method is passed the administrative arguments
 *	contained in the linked list pointed to by argp.  The results of the
 *	method are placed in a buffer and a pointer to the buffer is returned
 *	at output_pp.  Similarly, error information from the method is placed
 *	in a buffer and a pointer to the buffer is returned at error_pp.
 *
 *	For Ice Cream, only local method execution is supported and no
 *	framework control options are supported.
 *
 *	The function returns a status code indicating the completion status
 *	of the requested method.  The valid status codes are listed in
 *	admin_amcl.h.
 */
int
admin_perf_method(va_alist)
va_dcl
{
	va_list req_ap;		/* Varargs argument list */
	caddr_t class;		/* Object class to access */
	caddr_t method_name;	/* Method on class to invoke */
	caddr_t host;		/* Host on which to invoke method */
	caddr_t domain;		/* Domain in which host exists */
	Admin_arg *argp;	/* Linked list of arguments to the method */
	char **output_pp;	/* Location of pointer to hold address of */
				/* method results */
	char **error_pp;	/* Location of pointer to hold address of */
				/* error information from method */
	int option;		/* Framework control option type */

	va_start(req_ap);

	/* Get info on the class method to perform */

	class = va_arg(req_ap, caddr_t);
	method_name = va_arg(req_ap, caddr_t);
	host = va_arg(req_ap, caddr_t);
	domain = va_arg(req_ap, caddr_t);
	argp = va_arg(req_ap, Admin_arg *);
	output_pp = va_arg(req_ap, char **);
	error_pp = va_arg(req_ap, char **);

	/* Initialize the output and error buffer pointers. */

	if (output_pp != NULL) *output_pp = NULL;
	if (error_pp != NULL) *error_pp = NULL;

	/* For Ice Cream, methods may only be invoked locally */

	if ((strcmp(host, ADMIN_LOCAL) != 0) || (domain != NULL)) {
		admin_make_errbuf(error_pp, AMCL_ERR_ONLYLOCAL);
		return(FAIL_ERROR);
	}

	/*
	 * Check for optional framework control options.
	 * For Ice Cream, no optional controls are supported.
	 */

	option = va_arg(req_ap, int);
	if (option != ADMIN_END_OPTIONS) {
		admin_make_errbuf(error_pp, AMCL_ERR_BADOPTION, option);
		return(FAIL_ERROR);
	}

	/* Invoke the requested method locally. */

	return(exec_method_local(class, method_name, argp, output_pp, error_pp));
	
}

/*
 * exec_method_local( class, method_name, argp, output_pp, error_pp ):
 *
 *	Execute the specified administrative class method on the local
 *	host.  A child process is forked and that process exec's the
 *	executable for the method.  The method is passed (through argv)
 *	the administrative arguments contained in the linked list pointed
 *	at by argp.  STDOUT and STDERR from the child are redirected through
 *	pipes to the parent so that the results of the method can be obtained
 *	by the parent.  These results (from STDOUT and STDERR) are placed
 *	in buffers (by the parent) and pointers to the buffers are returned
 *	at output_pp and error_pp respectively.
 *
 *	This function returns the completion status for the invoked method
 *	as outlined in admin_amcl.h.
 */
static
int
exec_method_local(class, method_name, argp, output_pp, error_pp)
    caddr_t class;		/* Object class to access */
    caddr_t method_name;	/* Method on class to invoke */
    Admin_arg *argp;		/* Linked list of arguments to the method */
    char **output_pp;		/* Location of pointer to hold address of */
				/* method results */
    char **error_pp;		/* Location of pointer to hold address of */
				/* error information from method */
{
	caddr_t method_path;	/* Pathname to executable for requested method */
	char **argv;		/* argv struct containing method inputs */
	int output_des[2];	/* Pipe to return method output */
	int error_des[2];	/* Pipe to return error info */
	pid_t pid;		/* Process ID of child method */
	int statusp;		/* Status of child method process */
	int stat;

	extern caddr_t find_method();	/* Class mgmt. interface for finding */
					/* a method's executable */

	/* Look up the pathname to the executable for the requested method */

	method_path = find_method(class, method_name);
	if (method_path == NULL) {
		admin_make_errbuf(error_pp, AMCL_ERR_BADMETHOD);
		return(FAIL_ERROR);
	}

	/*
	 * Fork and execute the requested method. Collect the results
	 * from STDOUT and STDERR through the respective pipes.
	 */
	
	stat = pipe(output_des);   /* Create pipe to capture method output */
	if (stat != 0) {
		free(method_path);
		admin_make_errbuf(error_pp, AMCL_ERR_OUTPIPE, errno);
		return(FAIL_ERROR);
	}

	stat = pipe(error_des);   /* Create pipe to capture method error info */
	if (stat != 0) {
		free(method_path);
		close_pipe(output_des);
		admin_make_errbuf(error_pp, AMCL_ERR_ERRPIPE, errno);
		return(FAIL_ERROR);
	}

	pid = fork();		/* Fork child to execv() the method */
	switch(pid) {

	    case -1:	/* Fork failed */
		free(method_path);
		close_pipe(output_des);
		close_pipe(error_des);
		admin_make_errbuf(error_pp, AMCL_ERR_FORK, errno);
		return(FAIL_ERROR);

	    case 0:	/* Child process: execute method */

		/* Pipe STDERR to parent. */
		/* Pipe STDERR before doing anything else so that there is */
		/* and error channel between parent and child. */
		if (dup2(error_des[1], STDERR_DES) == -1) {
			admin_write_err(AMCL_ERR_ERRDUP, errno);
			exit(FAIL_ERROR);
		}
		/* Pipe STDOUT to parent */
		if (dup2(output_des[1], STDOUT_DES) == -1) {
			admin_write_err(AMCL_ERR_OUTDUP, errno);
			exit(FAIL_ERROR);
		}
		/* Build the argv list and exec the method */
		stat = build_argv(method_path, argp, &argv);
		if (stat != SUCCESS) exit(stat);
		execv(method_path, argv);

		/* execv() should never return */
		admin_write_err(AMCL_ERR_EXEC, errno);
		exit(FAIL_ERROR);

	    default:	/* Parent process: collect method results */

		/* Read method results from child's STDOUT and STDERR */
		/* and place them in the appropriate buffers. */

		close(output_des[1]);	/* Close parent output end of pipes */
		close(error_des[1]);	/* so that read on pipe won't block */
					/* waiting for output from parent. */
					/* This is needed for collect_results() */
					/* below. */

		stat = collect_results(output_des, output_pp, "STDOUT", error_pp);
		if (stat != 0) {
			close(output_des[0]);
			close(error_des[0]);
			free(method_path);
			return(FAIL_ERROR);
		}
		stat = collect_results(error_des, error_pp, "STDERR", error_pp);
		if (stat != 0) {
			close(output_des[0]);
			close(error_des[0]);
			free(method_path);
			return(FAIL_ERROR);
		}

		/* Clean up from method invocation */
		close(output_des[0]);
		close(error_des[0]);
		free(method_path);

		/* Determine if method exited normally and its exit status */
		if (waitpid(pid, &statusp, 0) == -1) {
			admin_make_errbuf(error_pp, AMCL_ERR_WAIT, errno);
			return(FAIL_ERROR);
		}
		if (WIFSTOPPED(statusp)) {
			admin_make_errbuf(error_pp, AMCL_ERR_STOPPED,
					  WSTOPSIG(statusp));
			return(FAIL_ERROR);
		}
		if (WIFSIGNALED(statusp)) {
			admin_make_errbuf(error_pp, AMCL_ERR_SIGNALED,
					  WTERMSIG(statusp));
			return(FAIL_ERROR);
		}

		return(WEXITSTATUS(statusp));
	}
}

/*
 * build_argv( pathname, argp, argv_p):
 *
 *	Build an argv structure to be used as input to an exec'd method
 *	and place a pointer to that structure at argv_p.  "pathname"
 *	should be the pathname to the method's executable and "argp" should
 *	point to a linked list of administrative arguments.  The resulting
 *	argv structure will contain the pathname in element 0 and a
 *	"name=value" string for each (named) argument and value in the linked
 *	list.  The "name=value" pairs will be placed in the argv structure
 *	in the same order that they appear in the linked list.
 *
 *	This function returns SUCCESS upon normal completion.  If an error
 *	occurs building the argv structure, the function will return an error
 *	status as outlined in admin_amcl.h.
 */
static
int
build_argv(pathname, argp, argv_p)
    caddr_t pathname;	/* Pathname to executable for the method */
    Admin_arg *argp;	/* Linked list of admin arguments to a method */
    char ***argv_p;	/* Pointer to an address for returning an argv struct */
{
	int arg_no;		/* Argument number in argv */
	int count;		/* Number of arguments in arg-list "argp" */
	Admin_arg *nargp;	/* Pointer to next argument in arg-list */
	char **argv;		/* argv structure containing method arguments */
	int arg_size;		/* Number of bytes required to store  */
				/* "name=value" representation for an arg */

	/*
	 * Count the number of arguments in arg-list "argp" .
	 * For Ice Cream, verify that all arguments are of type STRING.
	 */

	count = 0;
	nargp = argp;
	while (nargp != NULL) {
		if (nargp->type != ADMIN_STRING) {
			admin_write_err(AMCL_ERR_BADTYPE, nargp->type);
			return(FAIL_ERROR);
		}
		count++;
		nargp = nargp->next;
	}

	/* Create an argv structure containing "name=value" entries */

	argv = (char **) calloc((unsigned)(count+2), sizeof(char *));

	argv[0] = pathname;	/* First argument is always command name */

	for (arg_no=1, nargp=argp; arg_no<=count; arg_no++, nargp=nargp->next) {

		arg_size = strlen(nargp->name) + nargp->length + 1;
		argv[arg_no] = malloc((unsigned)(arg_size + 1));
		strcpy(argv[arg_no], nargp->name);
		strcat(argv[arg_no], "=");
		if (nargp->value != NULL)
			strncat(argv[arg_no], nargp->value, (int) nargp->length);
	}
	argv[count + 1] = NULL;	/* argv is always NULL-terminated */

	/* Return the argv structure */

	*argv_p = argv;
	return(SUCCESS);
}

/*
 * collect_results( filedes, buffer_pp, pipe_desc, error_pp ):
 *
 *	Read output from a pipe and store it in memory.  This function is
 *	used to receive the results of an exec'd method from its STDOUT and
 *	STDERR.
 *
 *	Output is read from the pipe specified by "filedes" until EOF.  The
 *	output is placed in a buffer in memory and a pointer to the buffer
 *	is returned at "buffer_pp".  "pipe_desc" should be a textual description
 *	of the pipe being reading (this is used for error reporting purposes).
 *
 *	Upon normal completion, this function returns 0.  If a error occurs,
 *	the function will return -1 and error_pp will be set of a pointer
 *	to detailed information about the error.
 */
static
int
collect_results(filedes, buffer_pp, pipe_desc, error_pp)
    int filedes[2];	/* File descriptors for pipe on which to rec'v results */
    char **buffer_pp;	/* Address of pointer to hold location of results */
    caddr_t pipe_desc;	/* Description of pipe being read */
    char **error_pp;	/* Location of pointer to hold address of */
			/* error information from method */
{
	int res_size;	/* Number of bytes in returned results */
	int blk_size;	/* Size of result block read from file descriptor */

	if (buffer_pp == NULL) return(0);
	if (*buffer_pp != NULL) free(*buffer_pp);

	/*
	 * Read result blocks of size ADMIN_RESBLKSIZE from the file descriptor
	 * and place them consecutively in a memory buffer (extend the buffer
	 * as necessary to hold all of the results.  Read until EOF is found.
	 */

	res_size = 0;
	blk_size = ADMIN_RESBLKSIZE;
	*buffer_pp = malloc(1);
	while(blk_size != 0) {
		*buffer_pp = realloc(*buffer_pp,
				     (unsigned)(res_size + ADMIN_RESBLKSIZE + 1));
		blk_size = read(filedes[0], (*buffer_pp + res_size),
			        ADMIN_RESBLKSIZE);
		if (blk_size == -1) {
			free(*buffer_pp);
			*buffer_pp = NULL;
			admin_make_errbuf(error_pp, AMCL_ERR_BADREAD,
					  pipe_desc, errno);
			return(-1);
		}
		res_size += blk_size;
	}

	/*
	 * Strip off any extra allocated memory from buffer and place a NULL
	 * at the end.
	 */

	*buffer_pp = realloc(*buffer_pp, (unsigned)(res_size + 1));
	*(*buffer_pp + res_size) = NULL;

	return(0);
}

/*
 * admin_make_errbuf( buf_ptr, format [ , args... ] ):
 *
 *	Malloc() a buffer of size ADMIN_MSGSIZE and write the specified
 *	formatted output to the buffer.  A pointer to the buffer will be
 *	placed at the address specified by buf_ptr (If this address contains
 *	a non-null pointer, then the space it points to will be freed before
 *	the pointer is over-written).  A pointer to the resulting buffer
 *	is returned.
 *
 *	If buf_ptr is NULL, this function returns without taking any actions
 *	(i.e. it will be a no-op).
 */
caddr_t
admin_make_errbuf(va_alist)
va_dcl
{
	va_list msg_p;		/* Varargs argument list */
	char **buf_pp;		/* Pointer to message buffer being created */
	caddr_t format;		/* Format string to use for message */
	caddr_t return_p;	/* Value to return as result of function */

	va_start(msg_p);
	buf_pp = va_arg(msg_p, char **);
	if (buf_pp == NULL) {
		return_p = NULL;
		}
	   else	{
		if (*buf_pp != NULL) free(*buf_pp);
		*buf_pp = malloc(ADMIN_MSGSIZE);
		format = va_arg(msg_p, caddr_t);
		return_p = (caddr_t) vsprintf(*buf_pp, format, msg_p);
		}
	va_end(msg_p);

	return(return_p);
}

/*
 * admin_write_err( format [ , args... ] ):
 *
 *	Write the specified formatted output to STDERR.  This function
 *	serves as a front end to fprintf(stderr, ...) so that the internal
 *	handling of error messages can easily be changed.
 */
void
admin_write_err(va_alist)
va_dcl
{
	va_list msg_p;		/* Varargs argument list */
	caddr_t format;		/* Format string to use for message */

	va_start(msg_p);
	format = va_arg(msg_p, caddr_t);
	vfprintf(stderr, format, msg_p);
	va_end(msg_p);
}

/*
 * close_pipe( filedes ):
 *
 *	Close the specified pipe.  This functions closes both the input
 *	and output ends of the pipe
 */
static
void
close_pipe(filedes)
    int filedes[2];
{
	close(filedes[0]);
	close(filedes[1]);
}
