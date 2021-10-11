/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)setenv.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

static	short	setenv_made_new_vector= 0;
extern	char	*getenv();
extern	char	**environ;
extern	char	*malloc();
extern	char	*strcpy();

#define NULL 0

char *setenv(name, value)
	char *name, *value;
{	char *p= NULL, **q;
	int length= 0, vl;

	if ((p= getenv(name)) == NULL) {	/* Allocate new vector */
		for (q= environ; *q != NULL; q++, length++);
		q= (char **)malloc((unsigned)(sizeof(char *)*(length+2)));
		bcopy((char *)environ, ((char *)q)+sizeof(char *), sizeof(char *)*(length+1));
		if (setenv_made_new_vector++)
			free((char *)environ);
		length= strlen(name);
		environ= q;}
	else { /* Find old slot */
		length= strlen(name);
		for (q= environ; *q != NULL; q++)
			if (!strncmp(*q, name, length))
				break;};
	vl= strlen(value);
	if (!p || (length+vl+1 > strlen(p)))
		*q= p= malloc((unsigned)(length+vl+2));
	else
		p= *q;
	(void)strcpy(p, name); p+= length;
	*p++= '=';
	(void)strcpy(p, value);
	return(value);
}
