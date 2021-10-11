
/*	@(#)nlsstr.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlsadmin:nlsstr.c	1.4"

char *erropen = "%s: net_spec %s invalid\n";
char *errclose = "%s: error on close of %s, errno = %d\n";
char *errmalloc = "could not malloc enough memory";
char *errchown = "%s: could not change ownership on %s, errno = %d\n";
char *errscode = "%s: service code %s not found in %s\n";
char *errperm = "must be super user";
char *errdbf = "error in reading database file";
char *errsvc = "bad service code";
char *errarg = "embedded newlines illegal in arguments";
char *errcorrupt = "database file has been corrupted";

int Version = 2;	/* data base version, must match version # below */

/*
 * IMPORTANT NOTE: the line containing the version stamp below MUST NOT
 *  change, the version checker knows its format
 */

char *init[] = {
"# Network Listener Data Base.",
"#",
"# VERSION=2",
"#",
"# Entries have the following format:",
"#",
"#  service_code: flags: id: reserved: modules_to_push: servers_path_and_args # Service Description",
"#",
"#      where service code is ASCII.",
"#	    flags are characters which are described below.",
"#	    id is the entry from /etc/passwd (set to listen if not specified.",
"#	    reserved is reserved for future use.",
"#	    modules_to_push is a comma-separated list of modules to be",
"#				pushed before execing the server.  It must",
"#				end in a comma and contain at least 'NULL,'.",
"#	    servers_path_and_args are the full pathname of the server",
"#				  and optional arguments for the server.",
"#	    Service description is used by administration.",
"#",
"#      Allowable flag characters are:",
"#      'a' = Administrative entry.",
"#      'x' = Service turned off.",
"#      'n' = null flag (a place holder, that must be present if no other",
"#	    flags.  ('n' is always ignored.)",
"#",
"#      THIS FILE IS PROGRAM MODIFIED BY ADMINISTRATION AND PACKAGE",
"#      INSTALL/REMOVE SCRIPTS AND SHOULD NOT BE MANUALLY EDITED.",
"#",
"#      NOTES:",
"#	      Each entry MUST be newline terminated.",
"#",
"#	      All white space is ignored.",
"#",
"#	      No line may contain more than 512 characters including",
"#	      white space and the terminating newline character.",
"#      ",
"#",
" ",
(char *)0};
