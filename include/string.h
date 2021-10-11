/*	@(#)string.h 1.1 92/07/30 SMI; from S5R2 1.2	*/

#ifndef	__string_h
#define	__string_h

#include <sys/stdtypes.h>	/* for size_t */

#ifndef NULL
#define	NULL		0
#endif

extern char *	strcat(/* char *s1, const char *s2 */);
extern char *	strchr(/* const char *s, int c */);
extern int	strcmp(/* const char *s1, const char *s2 */);
extern char *	strcpy(/* char *s1, const char *s2 */);
extern size_t	strcspn(/* const char *s1, const char *s2 */);
#ifndef	_POSIX_SOURCE
extern char *	strdup(/* char *s1 */);
#endif
extern size_t	strlen(/* const char *s */);
extern char *	strncat(/* char *s1, const char *s2, size_t n */);
extern int	strncmp(/* const char *s1, const char *s2, size_t n */);
extern char *	strncpy(/* char *s1, const char *s2, size_t n */);
extern char *	strpbrk(/* const char *s1, const char *s2 */);
extern char *	strrchr(/* const char *s, int c */);
extern size_t	strspn(/* const char *s1, const char *s2 */);
extern char *	strstr(/* const char *s1, const char *s2 */);
extern char *	strtok(/* char *s1, const char *s2 */);

#endif	/* !__string_h */
