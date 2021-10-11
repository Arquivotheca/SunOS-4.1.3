/*	"@(#)parse.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef	profile_defined
#define	profile_defined

/*
 * This file contains the procedure definitions for the parse.c package.
 */

/*
 * parse_chr(in_file) will read in a C-style character constant from In_File.
 * -1 will be returned if any error occurs.
 */
int parse_chr();

/*
 * parse_comments(in_file) will skip over a C-style comment from In_File.
 */
void parse_comments();

/*
 * parse_eol(in_file) will read characters from In_File until a new-line is
 * encountered.  TRUE will be returned when an end-of-file is encountered.
 */
Bool parse_eol();

/*
 * parse_int(in_file, error) will parse an integer from In_File.  *Error will
 * be set to TRUE if any error occurs.  If Error is NULL, an error message will
 * be printed and the program will be terminated.
 */
int parse_int();

/*
 * parse_string(in_file, text) will parse a C-style string from In_File and
 * return the result into Text.  If Text is NULL, a string will be malloc'ed
 * and returned.
 */
char *parse_string();

/*
 * parse_symbol(infile, symbol) will read in symbol from In_File and store
 * the result into Symbol.  A symbol contains an alphanumeric or an
 * underscore.  Storage will be malloc'ed if Symbol is NULL.  -1 will be
 * returned if an error occurs.
 */
char *parse_symbol();

/*
 * parse_whitespace(in_file) will skip over any spaces and tabs from In_File.
 * The character that terminated the whitespace is returned.
 */
int parse_whitespace();

#endif

