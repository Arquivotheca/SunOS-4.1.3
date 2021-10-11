#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)help_file.c 1.1 92/07/30 Copyright 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sunwindow/defaults.h>
#include <suntool/help.h>

#define HELPNAMELENGTH 256

static FILE *help_file;
static char help_buffer[HELPNAMELENGTH+128];

static char *
help_search_file(key)
    char *key;
{
    char *entry, *arg;
    static char last_arg[64];

    if (key == NULL)
	return (NULL);
    fseek(help_file, 0, 0);

    while (entry = fgets(help_buffer, sizeof(help_buffer), help_file))
	if (*entry++ == ':') {
	    strtok(entry, ":\n");
	    arg = strtok(NULL, "\n");
	    entry = strtok(entry, " \t");

	    while (entry && strcmp(entry, key))
		entry = strtok(NULL, " \t");

	    if (entry) {
		strncpy(last_arg, (arg ? arg : ""), sizeof(last_arg) - 1);
		return (last_arg);
	    }
	}

    return (NULL);
}

char *
help_get_arg(data)
    char *data;
{
    char *client, *key, copy_data[64], *ptr;
    static char last_client[64];
    static char help_directory[HELPNAMELENGTH];
    int len;

    if (data == NULL)
	return (NULL);
    strncpy(copy_data, data, sizeof(copy_data));
    copy_data[sizeof(copy_data) - 1] = '\0';
    if (!(client = strtok(copy_data, ":")) || !(key = strtok(NULL, "")))
	return (NULL);
    if (strcmp(last_client, client)) {
	if (help_directory[0] == '\0')
	    strncpy(help_directory,
		    defaults_get_string("/Help/Directory", HELPDIRECTORY, 0),
		    HELPNAMELENGTH-1);
        sprintf(help_buffer, "%s/%s.info", help_directory, client);
	if (help_file) {
	    fclose(help_file);
	    last_client[0] = '\0';
	}
	if ((help_file = fopen(help_buffer, "r")) == NULL)
	    return (NULL);
	strcpy(last_client, client);
    }
    return (help_search_file(key));
}

char *
help_get_text()
{
    char *ptr = fgets(help_buffer, sizeof(help_buffer), help_file);

    return ((ptr && *ptr != ':' && *ptr != '#') ? ptr : NULL);
}
