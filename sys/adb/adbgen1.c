#ifndef lint
static  char sccsid[] = "@(#)adbgen1.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Read in "high-level" adb script and emit C program.
 * The input may have specifications within {} which
 * we analyze and then emit C code to generate the
 * ultimate adb acript.
 * We are just a filter; no arguments are accepted.
 */

#include <stdio.h>

#define	streq(s1, s2)	(strcmp(s1, s2) == 0)

#define	LINELEN  	1024	/* max line length expected in input */
#define	STRLEN		128	/* for shorter strings */
#define	NARGS		5	/* number of emitted subroutine arguments */

/*
 * Types of specifications in {}.
 */
#define	PRINT   	0	/* print member name with format */
#define	INDIRECT	1	/* fetch member value */
#define	OFFSETOK	2	/* insist that the offset is ok */
#define	SIZEOF		3	/* print sizeof struct */
#define	END		4	/* get offset to end of struct */
#define	OFFSET		5	/* just emit offset */
#define	EXPR		6	/* arbitrary C expression */

/*
 * Special return code from nextchar.
 */
#define CPP		-2	/* cpp line, restart parsing */

char struct_name[STRLEN];	/* struct name */
char member[STRLEN];		/* member name */
char format[STRLEN];		/* adb format spec */
char arg[NARGS][STRLEN];	/* arg list for called subroutine */

int line_no = 1;		/* input line number - for error messages */
int specsize;			/* size of {} specification - 1 or 2 parts */
int state;			/* XXX 1 = gathering a printf */
				/* This is a kludge so we emit pending */
				/* printf's when we see a CPP line */

int nextchar();
extern char *index();

main(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	char *start_printf();
	register int c;

	/*
	 * Get structure name.
	 */
	cp = struct_name;
	while ((c = nextchar(NULL)) != '\n') {
		if (c == EOF) {
			fprintf(stderr, "Premature EOF\n");
			exit(1);
		}
		if (c == CPP)
			continue;
		*cp++ = (char) c;
	}
	*cp = '\0';
	/*
	 * Basically, the generated program is just an ongoing printf
	 * with breaks for {} format specifications.
	 */
	printf("\n");
	printf("main()\n");
	printf("{\n");
	if (argc > 1 && strcmp(argv[1], "-w") == 0) {
		printf("\textern int warnings;\n\n\twarnings = 0;\n");
	}
	cp = start_printf();
	while ((c = nextchar(cp)) != EOF) {
		switch (c) {
		case '"':
			*cp++ = '\\';	/* escape ' in string */
			*cp++ = '"';
			break;
		case '\n':
			*cp++ = '\\';	/* escape newline in string */
			*cp++ = 'n';
			break;
		case '{':
			emit_printf(cp);
			read_spec();
			generate();
			cp = start_printf();
			break;
		case CPP:
			/*
			 * Restart printf after cpp line.
			 */
			cp = start_printf();
			break;
		default:
			*cp++ = c;
			break;
		}
		if (cp - arg[1] >= STRLEN - 10) {
			emit_printf(cp);
			cp = start_printf();
		}
	}
	emit_printf(cp);
	/* terminate program */
	printf("}\n");
	exit(0);
}

int
nextchar(cp)
	char *cp;
{
	register int c;
	static int newline = 1;

	c = getchar();
	/*
	 * Lines beginning with '#' and blank lines are passed right through.
	 */
	while (newline) {
		switch(c) {
		case '#':
			if (state)
				emit_printf(cp);
			do {
				putchar(c);
				c = getchar();
				if (c == EOF)
					return (c);
			} while (c != '\n');
			putchar(c);
			line_no++;
			return (CPP);
		case '\n':
			if (state)
				emit_printf(cp);
			putchar(c);
			c = getchar();
			line_no++;
			break;
		default:
			newline = 0;
			break;
		}
	}
	if (c == '\n') {
		newline++;
		line_no++;
	}
	return (c);
}

/*
 * Get started on printf of ongoing adb script.
 */
char *
start_printf()
{
	register char *cp;

	strcpy(arg[0], "\"%s\"");
	cp = arg[1];
	*cp++ = '"';
	state = 1;			/* XXX */
	return(cp);
}

/*
 * Emit call to printf to print part of ongoing adb script.
 */
emit_printf(cp)
	char *cp;
{
	*cp++ = '"';
	*cp = '\0';
	emit_call("printf", 2);
	state = 0;			/* XXX */
}

/*
 * Read {} specification.
 * The first part (up to a comma) is put into "member".
 * The second part, if present, is put into "format".
 */
read_spec()
{
	register char *cp;
	register int c;

	cp = member;
	specsize = 1;
	while ((c = nextchar(NULL)) != '}') {
		switch (c) {
		case EOF:
			fprintf(stderr, "Unexpected EOF inside {}\n");
			exit(1);
		case '\n':
			fprintf(stderr, "Newline not allowed in {}, line %d\n",
				line_no);
			exit(1);
		case '#':
			fprintf(stderr, "# not allowed in {}, line %d\n",
				line_no);
			exit(1);
		case ',':
			if (specsize == 2) {
				fprintf(stderr, "Excessive commas in {}, ");
				fprintf(stderr, "line %d\n", line_no);
				exit(1);
			}
			specsize = 2;
			*cp = '\0';
			cp = format;
			break;
		default:
			*cp++ = c;
			break;
		}
	}
	*cp = '\0';
	if (cp == member) {
		specsize = 0;
	}
}

/*
 * Decide what type of input specification we have.
 */
get_type()
{
	if (specsize == 1) {
		if (streq(member, "SIZEOF")) {
			return(SIZEOF);
		}
		if (streq(member, "OFFSETOK")) {
			return(OFFSETOK);
		}
		if (streq(member, "END")) {
			return(END);
		}
		return(OFFSET);
	}
	if (specsize == 2) {
		if (member[0] == '*') {
			return(INDIRECT);
		}
		if (streq(member, "EXPR")) {
			return(EXPR);
		}
		return(PRINT);
	}
	fprintf(stderr, "Invalid specification, line %d\n", line_no);
	exit(1);
}

/*
 * Generate the appropriate output for an input specification.
 */
generate()
{
	switch (get_type()) {
	case PRINT:
		emit_print();
		break;
	case OFFSET:
		emit_offset();
		break;
	case INDIRECT:
		emit_indirect();
		break;
	case OFFSETOK:
		emit_offsetok();
		break;
	case SIZEOF:
		emit_sizeof();
		break;
	case EXPR:
		emit_expr();
		break;
	case END:
		emit_end();
		break;
	default:
		fprintf(stderr, "Internal error in generate\n");
		exit(1);
	}
}

/*
 * Emit calls to set the offset and print a member.
 */
emit_print()
{
	emit_offset();
	/*
	 * Emit call to "format" subroutine
	 */
	sprintf(arg[0], "\"%s\"", member);
	sprintf(arg[1], "sizeof ((struct %s *)0)->%s",
		struct_name, member);
	sprintf(arg[2], "\"%s\"", format);
	emit_call("format", 3);
}

/*
 * Emit calls to set the offset and print a member.
 */
emit_offset()
{
	/* 
	 * Emit call to "offset" subroutine
	 */
	sprintf(arg[0], "(int) &(((struct %s *)0)->%s)", struct_name, member);
	emit_call("offset", 1);
}

/*
 * Emit call to indirect routine.
 */
emit_indirect()
{
	sprintf(arg[0], "(int) &(((struct %s *)0)->%s)", struct_name,member+1);
	sprintf(arg[1], "sizeof ((struct %s *)0)->%s", struct_name, member+1);
	sprintf(arg[2], "\"%s\"", format);	/* adb register name */
	sprintf(arg[3], "\"%s\"", member);
	emit_call("indirect", 4);
}

/*
 * Emit call to "offsetok" routine.
 */
emit_offsetok()
{
	emit_call("offsetok", 0);
}

/*
 * Emit call to printf the sizeof the structure.
 */
emit_sizeof()
{
	sprintf(arg[0], "\"0t%%d\"");
	sprintf(arg[1], "sizeof (struct %s)", struct_name);
	emit_call("printf", 2);
}

/*
 * Emit call to printf an arbitrary C expression.
 */
emit_expr()
{
	sprintf(arg[0], "\"0t%%d\"");
	sprintf(arg[1], "(%s)", format);
	emit_call("printf", 2);
}

/*
 * Emit call to set offset to end of struct.
 */
emit_end()
{
	sprintf(arg[0], "sizeof (struct %s)", struct_name);
	emit_call("offset", 1);
}

/*
 * Emit call to subroutine name with nargs arguments from arg array.
 */
emit_call(name, nargs)
	char *name;
	int nargs;
{
	register int i;

	printf("\t%s(", name);		/* name of subroutine */
	for (i = 0; i < nargs; i++) {
		if (i > 0) {
			printf(", ");	/* argument separator */
		}
		printf("%s", arg[i]);	/* argument */
	}
	printf(");\n");			/* end of call */
}
