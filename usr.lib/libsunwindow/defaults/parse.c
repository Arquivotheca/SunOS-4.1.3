#ifndef	lint
#ifdef sccs
static char sccsid[] = "@(#)parse.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985, 1988 by Sun Microsystems, Inc.
 */

/*
 * This package contains routines for parsing numbers and strings from a
 * standard I/O stream.
 */

#include <sunwindow/sun.h>	/* True, False, ... */
#include <stdio.h>		/* Standard I/O Library */
#include <sunwindow/parse.h>	/* Exported routine definitions */

#define	STRING_SIZE	255	/* Maximum string size */

/* If character at a time debugging is needed, remove these two defines. */
#define	getchr(fp)	((int)getc(fp))	/* Get next character */
#define	ungetchr	ungetc	/* Unget next character */

extern	char *hash_get_memory();

/*
 * Locally defined procedures in alphabetical order:
 */

int parse_octal();
int parse_backslash_chr();

/*
 * parse_comments(in_file) will skip over a C-style comment from In_File.
 */
void
parse_comments(in_file)
	register FILE	*in_file;	/* Input file */
{
	register int	chr;		/* Temporary character */

	chr = parse_whitespace(in_file);
	if (chr == ';')
		(void)parse_eol(in_file);
	else if (chr == '/'){
		if (getchr(in_file) != '*'){
			(void)ungetchr('*', in_file);
			(void)ungetchr('/', in_file);
			return;
		}
		while (True){
			chr = getchr(in_file);
			if ((chr == '*') && (getchr(in_file) == '/'))
				break;
			if (chr == EOF)
				break;
		}
		(void)parse_whitespace(in_file);
	} else	(void)ungetchr(chr, in_file);
}


/*
 * parse_string(in_file, text, max_size) will parse a C-style string from
 * In_File and return the result into Text.  If Text is NULL, a string will
 * be malloc'ed and returned.  If Text is non-NULL, Max_Size specifies the
 * maximum size string that can be read into Text.  If any error occurs,
 * NULL will be returned.
 */
char *
parse_string(in_file, text, max_size)
	register FILE	*in_file;		/* Input file */
	char		*text;			/* Place to store string */
	int		max_size;		/* Maximum size */
{
	register int	chr;			/* Temporary character */
	register char	*pointer;		/* Pointer into string */
	register int	size;			/* String size */
	char		temp[STRING_SIZE];	/* Temporary string */

	size = 0;
	if (text == NULL){
		pointer = temp;
		max_size = STRING_SIZE;
	} else	pointer = text;
	max_size--;	/* Just in case */
	chr = parse_whitespace(in_file);
	if (chr != '"'){
		(void)ungetchr(chr, in_file);
		return NULL;
	}	
	getchr(in_file);
	while (size < max_size){
		chr = getchr(in_file);
		if (chr == '"')
			break;
		if (chr == '\n'){ 
			(void)ungetchr(chr, in_file);
			return NULL;
		}
		if (chr == EOF)
			return NULL;
		if (chr == '\\'){
			(void)ungetchr(chr, in_file);
			chr = parse_backslash_chr(in_file);
			if (chr == -1)
				return NULL;
		}
		*pointer++ = chr;
		size++;
		if (size >= max_size)
			return NULL;
	}
	if (size == max_size)
		return NULL;
	*pointer = '\0';
	if (text == NULL){
		text = hash_get_memory(size + 1);
		(void)strcpy(text, temp);
	}
	return text;
}

/*
 * parse_chr(in_file) will parse a single C-style character from In_File and
 * return the result.  -1 will be returned on any errors. 
 */
int
parse_chr(in_file)
	register FILE	*in_file;	/* Input file */
{
	register int	chr;		/* Temporary character */
	int	temp_chr;		/* Yet another temporary character */

	chr = getchr(in_file);
	if (chr != '\'')
		return -1;
	chr = parse_backslash_chr(in_file);
	if (chr < 0)
		return -1;
	temp_chr = getchr(in_file);
	if (temp_chr == '\'')
		return chr;
	else	return -1;
}

/*
 * parse_backslash_chr(infile) will parse a C-style backslash character.
 */
static int
parse_backslash_chr(in_file)
	register FILE	*in_file;	/* Input file */
{
	register char	chr;		/* Temporary character */
	int		count;		/* Number of octal digits */
	int		number;		/* Number associated with character */

	chr = getchr(in_file);
	if (chr != '\\')
		return -1;
	chr = getchr(in_file);
	switch (chr){
	    case 't':
		return '\t';
	    case 'n':
		return '\n';
	    case 'b':
		return '\b';
	    case 'r':
		return '\r';
	    case '\\':
		return '\\';
	    case 'f':
	    	return '\f';
	    case '\'':
		return '\'';
	    case '"':
		return '"';
	}
	number = 0;
	count = 0;
	while (count < 3){
		if (('0' <= chr) && (chr <= '7')){
			number = number * 8 + chr - '0';
			chr = getchr(in_file);
			count++;
		} else {
			break;
		}
	}
	(void)ungetchr(chr, in_file);
	return (count == 0) ? -1 : number;
}

/*
 * parse_symbol(infile, symbol) will read in symbol from In_File and store
 * the result into Symbol.  A symbol contains an alphanumeric or an
 * underscore.  Storage will be malloc'ed if Symbol is NULL.  If an error
 * occurs, NULL will be returned.
 */
char *
parse_symbol(in_file, symbol)
	register FILE	*in_file;	/* Input file */
	char		*symbol;	/* Optional place to store symbol */
{
	register int	chr;		/* Temporary character */
	Bool		got_chr;	/* True => got a character */
	char		*pointer;	/* Pointer into string */
	int		size;		/* String size */
	char	temp[STRING_SIZE];	/* Temporary string */

	got_chr = False;
	pointer = (symbol == NULL) ? &temp[0] : symbol;
	size = 0;
	chr = parse_whitespace(in_file);
	chr = getchr(in_file);
	while ((' ' < chr) && (chr <= '~') && (chr != '/')){
		got_chr = True;
		*pointer++ = chr;
		size++;
		chr = getchr(in_file);
	}
	(void)ungetchr(chr, in_file);
	if (!got_chr)
		return NULL;
	*pointer = '\0';
	if (symbol == NULL){
		symbol = hash_get_memory(size + 1);
		(void)strcpy(symbol, &temp[0]);
	}
	return symbol;
}

/*
 * parse_int(in_file, error) will parse an integer from In_File.  *Error will
 * be set to True if any error occurs.  If Error is NULL, an error message will
 * be printed and the program will be terminated.
 */
int
parse_int(in_file, error)
	register FILE	*in_file;	/* Input file */
	Bool		*error;		/* Error flag; NULL => halt on error */
{
	register int	chr;		/* Temporary character */
	Bool		have_error;	/* True => have an error */
	Bool		negative;	/* True => negative number */
	int		number;		/* Resultant number */

	have_error = True;	/* True until first digit is read */
	negative = False;
	number = 0;
	(void)parse_whitespace(in_file);
	chr = getchr(in_file);
	if (chr == '-'){
		negative = True;
		chr = getchr(in_file);
	}
	while (('0' <= chr) && (chr <= '9')){
		have_error = False;
		number = number * 10 - '0' + chr;
		chr = getchr(in_file);
	}
	(void)ungetchr(chr, in_file);
	if (error == NULL){
		if (have_error){
			(void)fprintf(stderr, "PARSE_INT: No integer\n");
			exit(1);
		}
	} else	*error = have_error;
	if (negative)
		number = -number;
	return number;
}

/*
 * parse_eol(in_file) will read characters from In_File until a new-line is
 * encountered.  True will be returned when an end-of-file is encountered.
 */
Bool
parse_eol(in_file)
	register FILE	*in_file;	/* Input file */
{
	register int	chr;		/* Temporary character */

	do	chr = getchr(in_file);
	    while ((chr != '\n') && (chr != EOF));
	return (Bool)(chr == EOF);
}

/*
 * parse_whitespace(in_file) will skip over any spaces and tabs from In_File.
 * The character that terminated the whitespace is returned.
 */
int
parse_whitespace(in_file)
	register FILE 	*in_file;	/* Input file */
{
	register int	chr;		/* Temporary character */

	do	chr = getchr(in_file);
	    while ((chr == ' ') || (chr == '\t'));
	(void)ungetchr(chr, in_file);
	return chr;
}

/*
 * getchar(in_file) will return the next character from In_File.  This routine
 * is used instead of getc so that it will be easier to debug this package.
 */
/*
static int
xgetchr(in_file)
	register FILE	*in_file;*/	/* Input file */
/*
{
	register int	chr;	*/	/* Temporary character */
/*
	chr = getc(in_file);
*/
/*	if ((' ' <= chr) && (chr <= '~'))
		printf("%c", chr);
	else	printf("\\%3o", chr); */
/*
	return chr;
}
*/

/*
 * ungetchr(chr, in_file) will push Chr back onto the In_File stream.  This
 * routine is used instead of ungetc so that it will be easier to debug this
 * package.
 */
/*
static void
xungetchr(chr, in_file)
	register int	chr;*/		/* Character to push back */
/*	register FILE	*in_file;*/	/* Input file */
/*
{
	(void)ungetc(chr, in_file);
*/
/*	if ((' ' <= chr) && (chr <= '~'))
		printf("<%c>", chr);
	else	printf("<\\%3o>", chr); */
/*
}
*/
