/*	@(#)strings.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/26	*/

/*
 * External function definitions
 * for routines described in string(3).
 */

#ifndef _strings_h
#define _strings_h

char	*strcat();
char	*strncat();
int	strcmp();
int	strncmp();
int	strcasecmp();
char	*strcpy();
char	*strncpy();
int	strlen();
char	*index();
char	*rindex();

#endif /*!_strings_h*/
