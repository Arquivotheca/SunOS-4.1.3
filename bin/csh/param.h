/*	@(#)param.h 1.1 92/07/30 SMI	*/

/*
 * Despite its name, this file has nothing in particular to
 * do with <sys/param.h>.  It contains definitions that the
 * C version of printf needs.  (The Vax version doesn't need
 * this file, but instead uses an assembly language version
 * of _doprnt, with printf simply arranging to call this
 * version.)
 *
 * This business of the C shell having a private copy of
 * printf is a real pain and should be fixed.
 */

/* Maximum number of digits in any integer (long) representation */
#define	MAXDIGS	11

/* Convert a digit character to the corresponding number */
#define	tonumber(x)	((x)-'0')

/* Convert a number between 0 and 9 to the corresponding digit */
#define	todigit(x)	((x)+'0')

/* Data type for flags */
typedef char bool;

/* Maximum total number of digits in E format */
#define	MAXECVT	17

/* Maximum number of digits after decimal point in F format */
#define	MAXFCVT	60

/* Maximum significant figures in a floating-point number */
#define	MAXFSIG	17

/* Maximum number of characters in an exponent */
#define	MAXESIZ	4

/* Maximum (positive) exponent or greater */
#define	MAXEXP	40
