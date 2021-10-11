#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)labeltostring.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <sys/label.h> 

char *
labeltostring(part, value, verbose)
int part;
blabel_t *value;
int verbose;
{
	char *string;

	string = (char *)malloc(sizeof(char));
	strcpy(string, "");
	return (string);
}

labelfromstring(part, label_string, value)
int part;
char *label_string;
blabel_t *value;
{
	bzero(value, sizeof(value));
}
