/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)chrtbl.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.2 */
#endif

/*
 * 	chrtbl - generate character class definition table
 */
#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <string.h>
#include  <signal.h>
#include "codeset.h"
#include "euc.h"
#include "iso2022.h"

/*	Definitions	*/

#define HEX    1
#define OCTAL  2
#define RANGE  1
#define UL_CONV 2
#define SIZE	2 * 257

#define	INVALID		0	/* invalid string */
#define	CTYPE_BIT	1	/* bit for "ctype" */
#define UL		2	/* case translation */
#define CHRCLASS	3	/* character class name */
#define MODEL		4	/* code set name */


/*	Static variables	*/

struct  classname	{
	char	*name;
	int	type;
	int	value;
}  cln[]  =  {
	"isupper",	CTYPE_BIT,	_U,
	"islower",	CTYPE_BIT,	_L,
	"isdigit",	CTYPE_BIT,	_N,
	"isspace",	CTYPE_BIT,	_S,
	"ispunct",	CTYPE_BIT,	_P,
	"iscntrl",	CTYPE_BIT,	_C,
	"isblank",	CTYPE_BIT,	_B,
	"isxdigit",	CTYPE_BIT,	_X,
	"ul",		UL,		0,
	"chrclass",	CHRCLASS,	0,
	"model",	MODEL,		0,
	NULL,		INVALID,	0
};

int	readstd;			/* Process the standard input */
char	linebuf[256], *p = linebuf;	/* Current line in input file */
int	chrclass = 0;			/* set if chrclass is defined */
char	item_name[20];			/* save current item name */
int	item_type;			/* save current item type */
int	ul_conv = 0;			/* set when left angle bracket
					 * is encountered. 
					 * cleared when right angle bracket
					 * is encountered */
int	cont = 0;			/* set if the description continues
					 * on another line */
int	action = 0;			/*  action = RANGE when the range
					 * character '-' is ncountered.
					 *  action = UL_CONV when it creates
					 * the conversion tables.  */
int	in_range = 0;			/* the first number is found 
					 * make sure that the lower limit
					 * is set  */
int	ctype[SIZE];			/* character class and character
					 * conversion table */
int	range = 0;			/* set when the range character '-'
					 * follows the string */
char	tablename[24];			/* save data file name */  
char	*cmdname;			/* Save command name */
char	input[24];			/* Save input file name */

/* Error  messages */
/* vprintf() is used to print the error messages */

char	*msg[] = {
/*    0    */ "Usage: chrtbl [ - | file ] ",
/*    1    */ "the character class \"chrclass\" is not defined",
/*    2    */ "incorrect character class name \"%s\"",
/*    3    */ "--- %s --- left angle bracket  \"<\" is missing",
/*    4    */ "--- %s --- right angle bracket  \">\" is missing",
/*    5    */ "--- %s --- wrong input specification \"%s\"",
/*    6    */ "--- %s --- number out of range \"%s\"",
/*    7    */ "--- %s --- nonblank character after (\"\\\") on the same line",
/*    8    */ "--- %s --- wrong upper limit \"%s\"",
/*    9    */ "--- %s --- wrong character \"%c\"",
/*   10    */ "--- %s --- number expected",
/*   11    */ "--- %s --- too many range \"-\" characters",
/*   12    */ "--- %s --- incorrect syntax for \"model\""
};


main(argc, argv)
int	argc;
char	**argv;
{
	int	clean();

	if (cmdname = strrchr(*argv, '/'))
		++cmdname;
	else
		cmdname = *argv;
	if ( (argc != 1)  && (argc != 2) )
		error(cmdname, msg[0]);
	if ( argc == 1 || strcmp(argv[1], "-") == 0 )
		{
		readstd++;
		strcpy(input, "standard input");
		}
	else
		strcpy(input, argv[1]);
	if (signal(SIGINT, SIG_IGN) == SIG_DFL)
		signal(SIGINT, clean);
	if (!readstd && freopen(argv[1], "r", stdin) == NULL)
		perror(argv[1]), exit(1);
	init();
	process();
	if (!chrclass) 
		error(input, msg[1]);
	create();
	return(0);
}


/* Initialize the ctype array */

init()
{
	register i;
	for(i=0; i<256; i++)
		(ctype + 1)[257 + i] = i;
}

/* Read line from the input file and check for correct
 * character class name */

process()
{

	char	*token();
	register  struct  classname  *cnp;
	register char *c;
	for (;;) {
		if (fgets(linebuf, sizeof linebuf, stdin) == NULL ) {
			if (ferror(stdin)) {
				perror("chrtbl (stdin)");
				exit(1);	
				}
				break;	
	        }
		p = linebuf;
		/*  comment line   */
		if ( *p == '#' ) 
			continue; 
		/*  empty line  */
		if ( empty_line() )  
			continue; 
		if ( ! cont ) 
			{
			c = token();
			for (cnp = cln; cnp->name != NULL; cnp++) 
				if(strcmp(cnp->name, c) == NULL) 
					break; 
			}	
		switch(cnp->type) {
		default:
		case INVALID:
			error(input, msg[2], c);
		case CTYPE_BIT:
		case UL:
				strcpy(item_name, cnp->name);
				item_type = cnp->type;
				parse(cnp->value);
				break;
		case CHRCLASS:
				chrclass++;
				if ( (c = token()) == NULL )
					error(input, msg[1]);
				strcpy(tablename, c);
				break;
		case MODEL:
			create_codeset(tablename);
			break;
		}
	} /* for loop */
	return;
}

empty_line()
{
	register char *cp;
	cp = p;
	for (;;) {
		if ( (*cp == ' ') || (*cp == '\t') ) {
				cp++;
				continue; }
		if ( (*cp == '\n') || (*cp == '\0') )
				return(1); 
		else
				return(0);
	}
}

/* 
 * token() performs the parsing of the input line. It is sensitive
 * to space characters and returns a character pointer to the
 * function it was called from. The character string itself
 * is terminated by the NULL character.
 */ 

char *
token()
{
	register  char  *cp;
	for (;;) {
	check_item(p);
	switch(*p) {
		case '\0':
		case '\n':
			in_range = 0;
			cont = 0;
			return(NULL);
		case ' ':
		case '\t':
			p++;
			continue;
		case '>':
			if (action == UL)
				error(input, msg[10], item_name);
			ul_conv = 0;
			p++;
			continue;
		case '-':
			if (action == RANGE)
				error(input, msg[11], item_name);
			action = RANGE;
			p++;
			continue;
		case '<':
			if (ul_conv)
				error(input, msg[4], item_name);
			ul_conv++;
			p++;
			continue;
		default:
			cp = p;
			while(*p!=' ' && *p!='\t' && *p!='\n' && *p!='>' && *p!='-')  
				p++;   
			check_item(p);
			if (*p == '>')
				ul_conv = 0;
			if (*p == '-')
				range++;
			*p++ = '\0';
			return(cp);

		}
	}
}


/* conv_num() is the function which converts a hexadecimal or octal
 * string to its equivalent integer represantation. Error checking
 * is performed in the following cases:
 *	1. The input is not in the form 0x<hex-number> or 0<octal-mumber>
 *	2. The input is not a hex or octal number.
 *	3. The number is out of range.
 * In all error cases a descriptive message is printed with the character
 * class and the hex or octal string.
 * The library routine sscanf() is used to perform the conversion.
 */


int conv_num(s)
char	*s;
{
	char	*cp;
	int	i, j;
	int	num;
	cp = s;
	if ( *cp != '0' ) 
		error(input, msg[5], item_name, s);
	if ( *++cp == 'x' )
		num = HEX;
	else
		num = OCTAL;
	switch (num) {
	case	HEX:
			cp++;
			for (j=0; cp[j] != '\0'; j++) 
				if ((cp[j] < '0' || cp[j] > '9') && (cp[j] < 'a' || cp[j] > 'f'))
					break;
				
				break;
	case   OCTAL:
			for (j=0; cp[j] != '\0'; j++)
				if (cp[j] < '0' || cp[j] > '7')
					break;
			break;
	default:
			error(input, msg[5], item_name, s);
	}
	if ( num == HEX )  { 
		if (cp[j] != '\0' || sscanf(s, "0x%x", &i) != 1)  
			error(input, msg[5], item_name, s);
		if ( i > 0xff ) 
			error(input, msg[6], item_name, s);
		else
			return(i);
	}
	if (cp[j] != '\0' || sscanf(s, "0%o", &i) != 1) 
		error(input, msg[5], item_name, s);
	if ( i > 0377 ) 
		error(input, msg[6], item_name, s);
	return(i);
}

/* parse() gets the next token and based on the character class
 * assigns a value to corresponding table entry.
 * It also handles ranges of consecutive numbers and initializes
 * the uper-to-lower and lower-to-upper conversion tables.
 */

parse(type)
int type;
{
	char	*c;
	int	ch1, ch2;
	int 	lower, upper;
	while ( (c = token()) != NULL) {
		if ( *c == '\\' ) {
			if ( ! empty_line()  || strlen(c) != 1) 
				error(input, msg[7], item_name);
			cont = 1;
			break;
			}
		switch(action) {
		case	RANGE:
			upper = conv_num(c);
			if ( (!in_range) || (in_range && range) ) 
				error(input, msg[8], item_name, c);
			((ctype + 1)[upper]) |= type;
			if ( upper <= lower ) 
				error(input, msg[8], item_name, c);
			while ( ++lower <= upper ) 
				((ctype + 1)[lower]) |= type;
			action = 0;
			range = 0;
			in_range = 0;
			break;
		case	UL_CONV:
			ch2 = conv_num(c);
			(ctype + 1)[ch1 + 257] = ch2;
			(ctype + 1)[ch2 + 257] = ch1;
			action = 0;
			break;   
		default:
			lower = ch1 = conv_num(c);
			in_range ++;
			if (item_type == UL) 
				if (ul_conv)
					{
					action = UL_CONV;
					break;
					}
				else
					error(input, msg[3], item_name);
			else
				if (range)
					{
					action = RANGE;
					range = 0;
					}
				else
					;
			
			((ctype + 1)[lower]) |= type;
			break;
		}
	}
	if (action)
		error(input, msg[10], item_name);
}

/* create() produces a data file containing the ctype array
 * elements. The name of the file is the value specified by the "chrclass"
 * line.
 */


create()
{
	register   i=0;
	if (freopen(tablename, "w", stdout) == NULL)
		perror(tablename), exit(1);
	for (i=0; i< SIZE; i++)
		printf("%c", ctype[i]);
}

error(va_alist)
va_dcl
{
	va_list	args;
	char	*fmt;

	va_start(args);
	fprintf(stderr, "ERROR in %s: ", va_arg(args, char *));
	fmt = va_arg(args, char *);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	clean();
}

check_item(cp)
char	*cp;
{
	if (item_type != UL)
		if (*cp == '<' || *cp == '>')
			error(input, msg[9], item_name, *cp);
		else
			;
	else
		if (*cp == '-')
			error(input, msg[9], item_name, *cp);
		else
			;
}

clean()
{
	signal(SIGINT, SIG_IGN);
	unlink(tablename);
	exit(1);
}

create_codeset(tabname)
	char *tabname;
{
	char *p;
	struct _code_header code_header;
	eucwidth_t eucwidth;
	isowidth_t isowidth;
	char code_tablename[24*2];
	int fd;
	int len1 = 0;
	int len2 = 0;
	int len3 = 0;
	int len4 = 0;
	int len5 = 0;

	strcpy(code_tablename, tabname);
	strcat(code_tablename, ".ci");
	fd = creat (code_tablename, 0644);
	if (fd == -1) {
		perror(code_tablename);
		exit(1);
	}
		

	/*
	 * get code set name
	 */

	p = token();
	if (p == NULL) {
		/*
		 * no code set specified, ignore
		 */
		error(cmdname, msg[12]);
		unlink(code_tablename);
		return;
	}
	if (strcmp(p, EUC_SET) == 0) {
		/*
		 * EUC code set
		 */
		p = token();
		if (p == NULL) {
			error(cmdname, msg[12], EUC_SET);
			unlink(code_tablename);
			return;
		}	
		strcpy(code_header.code_name, EUC_SET);
		code_header.code_id = CODESET_EUC;
		code_header.code_info_size = sizeof (eucwidth);
		/* 
		 * Only the first three are used
		 */
		if (sscanf(p, "%d,%d,%d,%d", &len1, &len2, &len3, &len4) != 3) {
			error(cmdname, msg[12], p);
			unlink(code_tablename);
			return;
		}	
		write(fd, (char *)&code_header, sizeof(code_header));
		eucwidth._eucw1 = len1;
		eucwidth._eucw2 = len2;
		eucwidth._eucw3 = len3;
		write(fd, (char *)&eucwidth, sizeof(eucwidth));
		 
	}
	else if (strcmp(p, ISO2022_SET) == 0) {
		/*
		 * ISO2022 code set
		 */
		p = token();
		if (p == NULL) {
			error(cmdname, msg[12], ISO2022_SET);
			unlink(code_tablename);
			return;
		}	
		strcpy(code_header.code_name, ISO2022_SET);
		code_header.code_id = CODESET_ISO2022;
		code_header.code_info_size = sizeof (isowidth);
		/* 
		 * Only the first four are used
		 */
		if (sscanf(p, "%d,%d,%d,%d,%d", &len1, &len2, &len3, &len4, &len5) != 4) {
			error(cmdname, msg[12], p);
			unlink(code_tablename);
			return;
		}	
		/* Get 7bit or 8 bit environment
		 */
		p = token ();
		if (p == NULL) {
			error(cmdname, msg[12], ISO2022_SET);
			unlink(code_tablename);
			return;
		}	
		if (sscanf(p, "%d", &len5) != 1) {
			error(cmdname, msg[12], p);
			unlink(code_tablename);
			return;
		}
		/* 
		 * In our implementation ISO 2022 multibyte sequences are 
		 * no longer than 2 bytes
		 */
		if (len1 > 2 || len2 > 2 || len3 > 2 || len4 > 2) {
			error(cmdname, msg[12], p);
			unlink(code_tablename);
			return;
		}	
		if (len5 != 7 && len5 != 8) {
			error(cmdname, msg[12], p);
			unlink(code_tablename);
			return;
		}

		/* Now get the default bit significance (7 or 8 bits) */

		write(fd, (char *)&code_header, sizeof(code_header));
		isowidth.g0_len = len1;
		isowidth.g1_len = len2;
		isowidth.g2_len = len3;
		isowidth.g3_len = len4;
		isowidth.bit_env = len5;
		write(fd, (char *)&isowidth, sizeof(isowidth));
	}
	else if (strcmp(p, XCCS_SET) == 0) {
		/*
		 * XCCS code set
		 */
		strcpy(code_header.code_name, XCCS_SET);
		code_header.code_id = CODESET_XCCS;
		code_header.code_info_size = sizeof (isowidth);

		write(fd, (char *)&code_header, sizeof(code_header));
		write(fd, (char *)&isowidth, sizeof(isowidth));
	}
	else {
		/*
		 * other code set
		 */
		fprintf(stderr,"Warning - Non-standard model name (%s) selected\n" ,p);
		strcpy(code_header.code_name, p);
		code_header.code_id = CODESET_USER;
		code_header.code_info_size = 0;
		write(fd, (char *)&code_header, sizeof(code_header));
	}
	close(fd);
}

