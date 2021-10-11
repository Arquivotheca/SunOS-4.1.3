/*
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)list.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

#define LIST_MAX 200

char *
add_to_list(old, new)
	char *old;
	char *new;
{
	char *cp;
	char *malloc();

	if (old == NULL || *old == '\0') {
		if (new == NULL || *new == '\0') 
			return (NULL);
		cp = malloc(strlen(new) + 1);
		sprintf(cp, "%s", new);
	}
	else {
		if (new == NULL || *new == '\0') 
			return (old);
		cp = malloc(strlen(old) + strlen(new) + 2);
		sprintf(cp, "%s,%s", old, new);
	}
	return (cp);
}

extern char *program;
static char *list_start[LIST_MAX];
static char *list_at[LIST_MAX];


char *
list_reset(list)
	char *list;
{
	int which;
	char *to_comma();

	if (list == NULL)
		return (NULL);

	for (which = 0; which < LIST_MAX; which++) {
		if (list_start[which] == NULL) {
			list_start[which] = list;
			list_at[which] = list;
			return (to_comma(which));
		}
		if (list_start[which] == list) {
			list_at[which] = list;
			return (to_comma(which));
		}
	}
	fprintf(stderr, "%s: fatal list error\n", program);
	exit(1);
}

char *
list_next(list)
	char *list;
{
	char *to_comma();
	int which;
	
	if (list == NULL)
		return (NULL);

	for (which = 0; which < LIST_MAX; which++) {
		if (list_start[which] == list)
			return (to_comma(which));
	}
	fprintf(stderr, "%s: fatal list error\n", program);
	exit(2);
}

list_contains(string, list)
	char *string;
	char *list;
{
	char *cp;

	for (cp = list_reset(list); cp; cp = list_next(list)) {
		if (strcmp(cp, string) == 0) {
			free(cp);
			return (1);
		}
		free(cp);
	}
	return (0);
}


char *
to_comma(which)
	int which;
{
	char *cp;
	char *result;
	char *index();
	int len;

	if (*list_at[which] == '\0')
		return (NULL);

	result = malloc(strlen(list_at[which]) + 1 );
	strcpy(result, list_at[which]); 
	if ((cp = index(result, ',')) != NULL)
		*cp = '\0';
	while (*list_at[which] != '\0') {
		if (*list_at[which] == ',') {
			list_at[which]++;
			break;
		}
		list_at[which]++;
	}
	return (result);
}
