
#ifndef lint
static  char sccsid[] = "@(#)io.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains I/O related functions.
 */
#include "global.h"
#include <ctype.h>
#include <varargs.h>
#include "startup.h"

extern	void cmdabort();
extern	int _doprnt();
extern	int data_lineno;
#ifdef	not
extern	char *space2str();
#endif	not

/*
 * This variable is used to determine whether a token is present in the pipe
 * already.
 */
static	char token_present = 0;

/*
 * This routine pushes the given character back onto the input stream.
 */
pushchar(c)
	int	c;
{

	(void) ungetc(c, stdin);
}

/*
 * This routine checks the input stream for an eof condition.
 */
int
checkeof()
{

	return (feof(stdin));
}

/*
 * This routine gets the next token off the input stream.  A token is
 * basically any consecutive non-white characters.
 */
char *
gettoken(inbuf)
	char	*inbuf;
{
	char	*ptr = inbuf;
	int	c, quoted = 0;

retoke:
	/*
	 * Remove any leading white-space.
	 */
	while ((isspace(c = getchar())) && (c != '\n'))
		;
	/*
	 * If we are at the beginning of a line and hit the comment character,
	 * flush the line and start again.
	 */
	if (!token_present && c == COMMENT_CHAR) {
		token_present = 1;
		flushline();
		goto retoke;
	}
	/*
	 * Loop on each character until we hit unquoted white-space.
	 */
	while (!isspace(c) || quoted && (c != '\n')) {
		/*
		 * If we hit eof, get out.
		 */
		if (checkeof())
			return (NULL);
		/*
		 * If we hit a double quote, change the state of quotedness.
		 */
		if (c == '"')
			quoted = !quoted;
		/*
		 * If there's room in the buffer, add the character to the end.
		 */
		else if (ptr - inbuf < TOKEN_SIZE)
			*ptr++ = (char)c;
		/*
		 * Get the next character.
		 */
		c = getchar();
	}
	/*
	 * Null terminate the token.
	 */
	*ptr = '\0';
	/*
	 * Peel off white-space still in the pipe.
	 */
	while (isspace(c) && (c != '\n'))
		c = getchar();
	/*
	 * If we hit another token, push it back and set state.
	 */
	if (c != '\n') {
		pushchar(c);
		token_present = 1;
	} else
		token_present = 0;
	/*
	 * Return the token.
	 */
	return (inbuf);
}

/*
 * This routine removes the leading and trailing spaces from a token.
 */
clean_token(cleantoken, token)
	char	*cleantoken, *token;
{
	char	*ptr;

	/*
	 * Strip off leading white-space.
	 */
	for (ptr = token; isspace(*ptr); ptr++)
		;
	/*
	 * Copy it into the clean buffer.
	 */
	(void) strcpy(cleantoken, ptr);
	/*
	 * Strip off trailing white-space.
	 */
	for (ptr = cleantoken + strlen(cleantoken) - 1;
	    isspace(*ptr) && (ptr >= cleantoken); ptr--)
		*ptr = '\0';
}

/*
 * This routine flushes the rest of an input line if there is known
 * to be data in it.  The flush has to be qualified because the newline
 * may have already been swallowed by the last gettoken.
 */
flushline()
{

	if (token_present) {
		/*
		 * Flush the pipe to eol or eof.
		 */
		while ((getchar() != '\n') && !checkeof())
			;
		/*
		 * Mark the pipe empty.
		 */
		token_present = 0;
	}
}

/*
 * This routine returns the number of characters that are identical
 * between s1 and s2, stopping as soon as a mismatch is found.
 */
int
strcnt(s1, s2)
	char	*s1, *s2;
{
	int	i = 0;

	while ((*s1 != '\0') && (*s1++ == *s2++))
		i++;
	return (i);
}

/*
 * This routine converts the given token into an integer.  The token
 * must convert cleanly into an integer with no unknown characters.
 * If the token is the wildcard string, and the wildcard parameter
 * is present, the wildcard value will be returned.
 */
int
geti(str, iptr, wild)
	char	*str;
	int	*iptr, *wild;
{
	char	*str2;

	/*
	 * If there's a wildcard value and the string is wild, return the
	 * wildcard value.
	 */
	if (wild != NULL && strcmp(str, WILD_STRING) == 0)
		*iptr = *wild;
	else {
		/*
		 * Conver the string to an integer.
		 */
		*iptr = (int)strtol(str, &str2, 0);
		/*
		 * If any characters didn't convert, it's an error.
		 */
		if (*str2 != '\0') {
			eprint("`%s' is not an integer.\n", str);
			return (-1);
		}
	}
	return (0);
}

/*
 * This routine converts the given string into a block number on the
 * current disk.  The format of a block number is either a self-based
 * number, or a series of self-based numbers separated by slashes.
 * Any number preceeding the first slash is considered a cylinder value.
 * Any number succeeding the first slash but preceeding the second is
 * considered a head value.  Any number succeeding the second slash is
 * considered a sector value.  Any of these numbers can be wildcarded
 * to the highest possible legal value.
 */
int
getbn(str, iptr)
	char	*str;
	daddr_t	*iptr;
{

	char	*cptr, *hptr, *sptr;
	int	cyl, head, sect, wild;
	TOKEN	buf;

	/*
	 * Set cylinder pointer to beginning of string.
	 */
	cptr = str;
	/*
	 * Look for the first slash.
	 */
	while ((*str != '\0') && (*str != '/'))
		str++;
	/*
	 * If there wasn't one, convert string to an integer and return it.
	 */
	if (*str == '\0') {
		wild = physsects() - 1;
		if (geti(cptr, (int *)iptr, &wild))
			return (-1);
		return (0);
	}
	/*
	 * Null out the slash and set head pointer just beyond it.
	 */
	*str++ = '\0';
	hptr = str;
	/*
	 * Look for the second slash.
	 */
	while ((*str != '\0') && (*str != '/'))
		str++;
	/*
	 * If there wasn't one, sector pointer points to a null.
	 */
	if (*str == '\0')
		sptr = str;
	/*
	 * If there was, null it out and set sector point just beyond it.
	 */
	else {
		*str++ = '\0';
		sptr = str;
	}
	/*
	 * Convert the cylinder part to an integer and store it.
	 */
	clean_token(buf, cptr);
	wild = ncyl + acyl - 1;
	if (geti(buf, &cyl, &wild))
		return (-1);
	if ((cyl < 0) || (cyl >= ncyl + acyl)) {
		eprint("`%d' is out of range.\n", cyl);
		return (-1);
	}
	/*
	 * Convert the head part to an integer and store it.
	 */
	clean_token(buf, hptr);
	wild = nhead - 1;
	if (geti(buf, &head, &wild))
		return (-1);
	if ((head < 0) || (head >= nhead)) {
		eprint("`%d' is out of range.\n", head);
		return (-1);
	}
	/*
	 * Convert the sector part to an integer and store it.
	 */
	clean_token(buf, sptr);
	wild = sectors(head) - 1;
	if (geti(buf, &sect, &wild))
		return (-1);
	if ((sect < 0) || (sect >= sectors(head))) {
		eprint("`%d' is out of range.\n", sect);
		return (-1);
	}
	/*
	 * Combine the pieces into a block number and return it.
	 */
	*iptr = chs2bn(cyl, head, sect);
	return (0);
}

/*
 * This routine is the basis for all input into the program.  It
 * understands the semantics of a set of input types, and provides
 * consistent error messages for all input.  It allows for default
 * values and prompt strings.
 */
int
input(type, promptstr, delim, param, deflt, cmdflag)
	int	type, *deflt, cmdflag;
	char	*promptstr, delim, *param;
{
	int	interactive, help, i, length, index, tied;
	daddr_t	bn;
	char	**str, **strings, *ostr;
	TOKEN	token, cleantoken;
	struct	bounds *bounds;

reprompt:
	help = interactive = 0;
	/*
	 * If we are inputting a command, flush any current input in the pipe.
	 */
	if (cmdflag == CMD_INPUT)
		flushline();
	/*
	 * Note whether the token is already present.
	 */
	if (!token_present)
		interactive = 1;
	/*
	 * Print the prompt.
	 */
	printf(promptstr);
	/*
	 * If there is a default value, print it in a format appropriate
	 * for the input type.
	 */
	if (deflt != NULL) {
		printf(" [");
		switch (type) {
		    case FIO_BN:
			printf("%d, ", *deflt);
			pr_dblock(printf, (daddr_t)*deflt);
			break;
		    case FIO_INT:
			printf("%d", *deflt);
			break;
		    case FIO_CSTR:
		    case FIO_MSTR:
			strings = (char **)param;
			for (i = 0, str = strings; i < *deflt; i++, str++)
				;
			printf("%s", *str);
			break;
		    default:
			eprint("Error: unkown input type.\n");
			fullabort();
		}
		printf("]");
	}
	/*
	 * Print the delimiter character.
	 */
	printf("%c ", delim);
	/*
	 * Get the token.  If we hit eof, exit the program gracefully.
	 */
	if (gettoken(token) == NULL)
		fullabort();
	/*
	 * Echo the token back to the user if it was in the pipe or we
	 * are running out of a command file.
	 */
	if (!interactive || option_f)
		printf("%s\n", token);
	/*
	 * If we are logging, echo the token to the log file.  The else
	 * is necessary here because the above printf will also put the
	 * token in the log file.
	 */
	else if (log_file)
		lprint("%s\n", token);
	/*
	 * If the token was not in the pipe and it wasn't a command, flush
	 * the rest of the line to keep things in sync.
	 */
	if (interactive && cmdflag != CMD_INPUT)
		flushline();
	/*
	 * Scrub off the white-space.
	 */
	clean_token(cleantoken, token);
	/*
	 * If the input was a blank line and we weren't prompting
	 * specifically for a blank line...
	 */
	if ((strcmp(cleantoken, "") == 0) && (type != FIO_BLNK)) {
		/*
		 * If there's a default, return it.
		 */
		if (deflt != NULL)
			return (*deflt);
		/*
		 * If the blank was not in the pipe, just reprompt.
		 */
		if (interactive)
			goto reprompt;
		/*
		 * If the blank was in the pipe, it's an error.
		 */
		eprint("No default for this entry.\n");
		cmdabort();
	}
	/*
	 * If token is a '?' or a 'h', it is a request for help.
	 */
	if (strcmp(cleantoken, "?") == 0  ||  strcmp(cleantoken, "h") == 0)
		help = 1;
	/*
	 * Switch on the type of input expected.
	 */
	switch (type) {
	    /*
	     * Expecting a disk block number.
	     */
	    case FIO_BN:
		/*
		 * Parameter is the bounds of legal block numbers.
		 */
		bounds = (struct bounds *)param;
		/*
		 * Print help message if required.
		 */
		if (help) {
			printf("Expecting a block number from %d (",
			    bounds->lower);
			pr_dblock(printf, bounds->lower);
			printf(") to %d (", bounds->upper);
			pr_dblock(printf, bounds->upper);
			printf(")\n");
			break;
		}
		/*
		 * Convert token to a disk block number.
		 */
		if (getbn(cleantoken, &bn))
			break;
		/*
		 * Check to be sure it is within the legal bounds.
		 */
		if ((bn < bounds->lower) || (bn > bounds->upper)) {
			eprint("`");
			pr_dblock(eprint, bn);
			eprint("' is out of range.\n");
			break;
		}
		/*
		 * If it's ok, return it.
		 */
		return (bn);
	    /*
	     * Expecting an integer.
	     */
	    case FIO_INT:
		/*
		 * Parameter is the bounds of legal integers.
		 */
		bounds = (struct bounds *)param;
		/*
		 * Print help message if required.
		 */
		if (help) {
			printf("Expecting an integer from %d", bounds->lower);
			printf(" to %d\n", bounds->upper);
			break;
		}
		/*
		 * Convert the token into an integer.
		 */
		if (geti(cleantoken, (int *)&bn, (int *)NULL))
			break;
		/*
		 * Check to be sure it is within the legal bounds.
		 */
		if ((bn < bounds->lower) || (bn > bounds->upper)) {
			eprint("`%d' is out of range.\n", bn);
			break;
		}
		/*
		 * If it's ok, return it.
		 */
		return (bn);
	    /*
	     * Expecting a closed string.  This means that the input
	     * string must exactly match one of the strings passed in
	     * as the parameter.
	     */
	    case FIO_CSTR:
		/*
		 * The parameter is a null terminated array of character
		 * pointers, each one pointing to a legal input string.
		 */
		strings = (char **)param;
		/*
		 * Walk through the legal strings, seeing if any of them
		 * match the token.  If a match is made, return the index
		 * of the string that was matched.
		 */
		for (str = strings; *str!= NULL; str++)
			if (strcmp(cleantoken, *str) == 0)
				return (str - strings);
		/*
		 * Print help message if required.
		 */
		if (help) {
			printf("Expecting one of the following:\n");
			for (str = strings; *str != NULL; str++)
				printf("        %s\n", *str);
		} else {
			eprint("`%s' is not expected.\n", cleantoken);
		}
		break;
	    /*
	     * Expecting a matched string.  This means that the input
	     * string must either match one of the strings passed in,
	     * or be a unique abbreviation of one of them.
	     */
	    case FIO_MSTR:
		/*
		 * The parameter is a null terminated array of character
		 * pointers, each one pointing to a legal input string.
		 */
		strings = (char **)param;
		length = index = tied = 0;
		/*
		 * Loop through the legal input strings.
		 */
		for (str = strings; *str != NULL; str++) {
			/*
			 * See how many characters of the token match
			 * this legal string.
			 */
			i = strcnt(cleantoken, *str);
			/*
			 * If it's not the whole token, then it's not a match.
			 */
			if (i < strlen(cleantoken))
				continue;
			/*
			 * If it ties with another input, remember that.
			 */
			if (i == length)
				tied = 1;
			/*
			 * If it matches the most so far, record that.
			 */
			if (i > length) {
				index = str - strings;
				tied = 0;
				length = i;
			}
		}
		/*
		 * Pring help message if required.
		 */
		if (length == 0) {

			if (help) {
				printf("\n\nExpecting one of the following ");
				printf("(abbreviations ok):\n");
				for (str = strings; *str != NULL; str++)
					printf("        %s\n", *str);
				break;
			} 
			eprint("`%s' is not expected.\n", cleantoken);
			break;
		}
		/*
		 * If the abbreviation was non-unique, it's an error.
		 */
		if (tied) {
			eprint("`%s' is ambiguous.\n", cleantoken);
			break;
		}
		/*
		 * We matched one.  Return the index of the string we matched.
		 */
		return (index);
	    /*
	     * Expecting an open string.  This means that any string is legal.
	     */
	    case FIO_OSTR:
		/*
		 * Print a help message if required.
		 */
		if (help) {
			printf("Expecting a string\n");
			break;
		}
		/*
		 * Malloc the space needed to hold the string and fill it in.
		 */
		ostr = zalloc(strlen(token) + 1);
		(void) strcpy(ostr, token);
		/*
		 * Return the address of the string.
		 */
		return ((int)ostr);
	    /*
	     * Expecting a blank line.
	     */
	    case FIO_BLNK:
		/*
		 * We are always in non-echo mode when we are inputting
		 * this type.  We echo the newline as a carriage return
		 * only so the prompt string will be covered over.
		 */
		nolprint("\015");
		/*
		 * If we are logging, send a newline to the log file.
		 */
		if (log_file)
			lprint("\n");
		/*
		 * There is no value returned for this type.
		 */
		return (0);
	    /*
	     * If we don't recognize the input type, it's bad news.
	     */
	    default:
		eprint("Error: unknown input type.\n");
		fullabort();
	}
	/*
	 * If we get here, it's because some error kept us from accepting
	 * the token.  If we are running out of a command file, gracefully
	 * leave the program.  If we are interacting with the user, simply
	 * reprompt.  If the token was in the pipe, abort the current command.
	 */
	if (option_f)
		fullabort();
	else if (interactive)
		goto reprompt;
	else
		cmdabort();
	/*
	 * Never actually reached.
	 */
	return (-1);
}

/*
 * This routine is a modified version of printf.  It handles the cases
 * of silent mode and logging; other than that it is identical to the
 * library version.
 */
/*VARARGS1*/
printf(format, va_alist)
	char	*format;
	va_dcl
{
	va_list ap;

	/*
	 * If we are running silent, skip it.
	 */
	if (option_s)
		return;
	/*
	 * Do the print to standard out.
	 */
	va_start(ap);
	(void) vprintf(format, ap);
	/*
	 * If we are logging, also print to the log file.
	 */
	if (log_file) {
		(void) vfprintf(log_file, format, ap);
		(void) fflush(log_file);
	}
	va_end(ap);
}

/*
 * This routine is a modified version of printf.  It handles the cases
 * of silent mode; other than that it is identical to the
 * library version.  It differs from the above printf in that it does
 * not print the message to a log file.
 */
/*VARARGS1*/
nolprint(format, va_alist)
	char	*format;
	va_dcl
{
	va_list ap;

	/*
	 * If we are running silent, skip it.
	 */
	if (option_s)
		return;
	/*
	 * Do the print to standard out.
	 */
	va_start(ap);
	(void) vprintf(format, ap);
	va_end(ap);
}

/*
 * This routine is a modified version of printf.  It handles the cases
 * of silent mode, and only prints the message to the log file, not
 * stdout.  Other than that is identical to the library version.
 */
/*VARARGS1*/
lprint(format, va_alist)
	char	*format;
	va_dcl
{
	va_list ap;

	/*
	 * If we are running silent, skip it.
	 */
	if (option_s)
		return;
	/*
	 * Do the print to the log file.
	 */
	va_start(ap);
	(void) vfprintf(log_file, format, ap);
	(void) fflush(log_file);
	va_end(ap);
}

/*
 * This routine is identical in functionality to the fprintf library routine.
 * It is missing only some error checking, and is included here
 * mostly for completeness.
 */
/*VARARGS2*/
fprintf(iop, format, va_alist)
	FILE	*iop;
	char	*format;
	va_dcl
{
	va_list ap;

	/*
	 * Do the print to the given file.
	 */
	va_start(ap);
	(void) vfprintf(iop, format, ap);
	va_end(ap);
}

/*
 * This routine is a modified version of printf.  It prints the message
 * to stderr, and to the log file is appropriate.
 * Other than that is identical to the library version.
 */
/*VARARGS1*/
eprint(format, va_alist)
char *format;
va_dcl
{
	va_list ap;

	/*
	 * Do the print to stderr.
	 */
	va_start(ap);
	(void) vfprintf(stderr, format, ap);
	/*
	 * If we are logging, also print to the log file.
	 */
	if (log_file) {
		(void) vfprintf(log_file, format, ap);
		(void) fflush(log_file);
	}
	va_end(ap);
}

#ifdef	not
/*
 * This routine prints out a message describing the given ctlr.
 * The message is identical to the one printed by the kernel during
 * booting.
 */
pr_ctlrline(ctlr)
	register struct ctlr_info *ctlr;
{

	printf("           %s%d at %s 0x%x ", ctlr->ctlr_cname, ctlr->ctlr_num,
	    space2str(ctlr->ctlr_space), ctlr->ctlr_addr);
	if (ctlr->ctlr_vec != 0)
		printf("vec 0x%x ", ctlr->ctlr_vec);
	else
		printf("pri %d ", ctlr->ctlr_prio);
	printf("\n");
}
#endif	not

/*
 * This routine prints out a message describing the given disk.
 * The message is identical to the one printed by the kernel during
 * booting.
 */
pr_diskline(disk, num)
	register struct disk_info *disk;
	int	num;
{
	struct	ctlr_info *ctlr = disk->disk_ctlr;
	struct	disk_type *type = disk->disk_type;
	if (ctlr->ctlr_ctype->ctype_ctype != DKC_PANTHER) {
		printf("        %d. %s%d at %s%d slave %d\n", num, ctlr->ctlr_dname,
	        disk->disk_unit, ctlr->ctlr_cname, ctlr->ctlr_num,
		disk->disk_slave);
		if (type != NULL) {
		    printf("           %s%d: <%s cyl %d alt %d hd %d sec %d>\n",
			ctlr->ctlr_dname, disk->disk_unit, type->dtype_asciilabel,
			type->dtype_ncyl, type->dtype_acyl, type->dtype_nhead,
			type->dtype_nsect);
		} else {
		    printf("           %s%d: <drive type unknown>\n",
			ctlr->ctlr_dname, disk->disk_unit);
		}
	} else  {
		printf("        %d. %s%03x at %s%d slave %d\n", num, ctlr->ctlr_dname,
	        disk->disk_unit, ctlr->ctlr_cname, ctlr->ctlr_num,
		disk->disk_slave);
		if (type != NULL) {
		    printf("           %s%03x: <%s cyl %d alt %d hd %d sec %d>\n",
			ctlr->ctlr_dname, disk->disk_unit, type->dtype_asciilabel,
			type->dtype_ncyl, type->dtype_acyl, type->dtype_nhead,
			type->dtype_nsect);
		} else {
		    printf("           %s%03x: <drive type unknown>\n",
			ctlr->ctlr_dname, disk->disk_unit);
		}
	}
}

/*
 * This routine prints out a given disk block number in cylinder/head/sector
 * format.  It uses the printing routine passed in to do the actual output.
 */
pr_dblock(func, bn)
	int	(*func)();
	daddr_t	bn;
{

	(void) (*func)("%d/%d/%d", bn2c(bn), bn2h(bn), bn2s(bn));
}

/*
 * This routine inputs a character from the data file.  It understands
 * the use of '\' to prevent interpretation of a newline.  It also keeps
 * track of the current line in the data file via a global variable.
 */
int
sup_inputchar()
{
	int	c;

	/*
	 * Input the character.
	 */
	c = getc(data_file);
	/*
	 * If it's not a backslash, return it.
	 */
	if (c != '\\')
		return (c);
	/*
	 * It was a backslash.  Get the next character.
	 */
	c = getc(data_file);
	/*
	 * If it was a newline, update the line counter and get the next
	 * character.
	 */
	if (c == '\n') {
		data_lineno++;
		c = getc(data_file);
	}
	/*
	 * Return the character.
	 */
	return (c);
}

/*
 * This routine pushes a character back onto the input pipe for the data file.
 */
sup_pushchar(c)
	int	c;
{

	(void) ungetc(c, data_file);
}

/*
 * This routine inputs a token from the data file.  A token is a series
 * of contiguous non-white characters or a recognized special delimiter
 * character.
 */
int
sup_gettoken(buf)
	char	*buf;
{
	char	*ptr = buf;
	int	i, c, quoted = 0;

	for (i = 0; i < TOKEN_SIZE + 1; i++)
		buf[i] = '\0';
	/*
	 * Strip off leading white-space.
	 */
	while ((isspace(c = sup_inputchar())) && (c != '\n'))
		;
	/*
	 * Read in characters until we hit unquoted white-space.
	 */
	for (;!isspace(c) || quoted; c = sup_inputchar()) {
		/*
		 * If we hit eof, that's a token.
		 */
		if (feof(data_file))
			return (SUP_EOF);
		/*
		 * If we hit a double quote, change the state of quoting.
		 */
		if (c == '"') {
			quoted = !quoted;
			continue;
		}
		/*
		 * If we hit a newline, that delimits a token.
		 */
		if (c == '\n')
			break;
		/*
		 * If we hit any nonquoted special delimiters, that delimits
		 * a token.
		 */
		if (!quoted && (c == '=' || c == ',' || c == ':' ||
			c == '#' || c == '|' || c == '&' || c == '~'))
			break;
		/*
		 * Store the character if there's room left.
		 */
		if (ptr - buf < TOKEN_SIZE)
			*ptr++ = (char)c;
	}
	/*
	 * If we stored characters in the buffer, then we inputted a string.
	 * Push the delimiter back into the pipe and return the string.
	 */
	if (ptr - buf > 0) {
		sup_pushchar(c);
		return (SUP_STRING);
	}
	/*
	 * We didn't input a string, so we must have inputted a known delimiter.
	 * store the delimiter in the buffer, so it will get returned.
	 */
	buf[0] = c;
	/*
	 * Switch on the delimiter.  Return the appropriate value for each one.
	 */
	switch(c) {
	    case '=':
		return (SUP_EQL);
	    case ':':
		return (SUP_COLON);
	    case ',':
		return (SUP_COMMA);
	    case '\n':
		return (SUP_EOL);
	    case '|':
		return (SUP_OR);
	    case '&':
		return (SUP_AND);
	    case '~':
		return (SUP_TILDE);
	    case '#':
		/*
		 * For comments, we flush out the rest of the line and return
		 * an eol.
		 */
		while ((c = sup_inputchar()) != '\n' && !feof(data_file))
			;
		if (feof(data_file))
			return (SUP_EOF);
		else
			return (SUP_EOL);
	    /*
	     * Shouldn't ever get here.
	     */
	    default:
		return (SUP_STRING);
	}
}
