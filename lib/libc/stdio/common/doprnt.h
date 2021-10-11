/*	@(#)doprnt.h 1.1 92/07/30 SMI	*/

/* Maximum number of digits in any integer (long) representation */
#define	MAXDIGS	11

/* Convert a digit character to the corresponding number */
#define	tonumber(x)	((x)-'0')

/* Convert a number between 0 and 9 to the corresponding digit */
#define	todigit(x)	((x)+'0')

/* Data type for flags */
typedef	char	bool;
