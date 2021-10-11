/*
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)read.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

char *add_to_list();

char *read_base(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_clients(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_roots(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_devices(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_directories(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_filesystems(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_flags(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_minfree(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}


char *read_sysadmin(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}

char *read_execs(input)
	char *input;
{
	char *cp;

	cp = add_to_list(NULL, input);
	return (cp);
}
