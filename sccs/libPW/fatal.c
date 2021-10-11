#ifndef lint
static	char sccsid[] = "@(#)fatal.c 1.1 92/07/30 SMI"; /* from System III 3.4 */
#endif

# include	<sys/types.h>
# include	"../hdr/macros.h"
# include	"../hdr/fatal.h"
# include	"../hdr/had.h"
# include	<string.h>

extern	char	*malloc();

/*
	General purpose error handler.
	Typically, low level subroutines which detect error conditions
	(an open or create routine, for example) return the
	value of calling fatal with an appropriate error message string.
	E.g.,	return(fatal("can't do it"));
	Higher level routines control the execution of fatal
	via the global word Fflags.
	The macros FSAVE and FRSTR in <fatal.h> can be used by higher
	level subroutines to save and restore the Fflags word.
 
	The argument to fatal is a pointer to an error message string.
	The action of this routine is driven completely from
	the "Fflags" global word (see <fatal.h>).
	The following discusses the interpretation of the various bits
	of Fflags.
 
	The FTLMSG bit controls the writing of the error
	message on file descriptor 2.  The message is preceded
	by the string "ERROR: ", unless the global character pointer
	"Ffile" is non-zero, in which case the message is preceded
	by the string "ERROR [<Ffile>]: ".  A newline is written
	after the user supplied message.
 
	If the FTLCLN bit is on, clean_up is called with an
	argument of 0 (see clean.c).
 
	If the FTLFUNC bit is on, the function pointed to by the global
	function pointer "Ffunc" is called with the user supplied
	error message pointer as argument.
	(This feature can be used to log error messages).
 
	The FTLACT bits determine how fatal should return.
	If the FTLJMP bit is on longjmp(Fjmp) is
	called (Fjmp is a global jmp_buf, see setjmp(3)).
 
	If the FTLEXIT bit is on the value of userexit(1) is
	passed as an argument to exit(II)
	(see userexit.c).
 
	If none of the FTLACT bits are on
	(the default value for Fflags is 0), the global word
	"Fvalue" (initialized to -1) is returned.
 
	If all fatal globals have their default values, fatal simply
	returns -1.
*/

int	Fcnt;
int	Fflags;
char	*Ffile;
int	Fvalue = -1;
int	(*Ffunc)();
jmp_buf	Fjmp;
char	*nsedelim = (char *) 0;

/* default value for NSE delimiter (currently correct, if NSE ever
 * changes implementation it will have to pass new delimiter as
 * value for -q option)
 */
# define NSE_VCS_DELIM "vcs"

# define NSE_VCS_GENERIC_NAME "<history file>"

static	int	delimlen = 0;
int
fatal(msg)
char *msg;
{
	char	*ffile;

	++Fcnt;
	if (Fflags & FTLMSG) {
		write(2,"ERROR",5);
		if (Ffile) {
			ffile = nse_file_trim(Ffile, 0);
			write(2," [",2);
			write(2,ffile,length(ffile));
			write(2,"]",1);
		}
		write(2,": ",2);
		write(2,msg,length(msg));
		write(2,"\n",1);
	}
	if (Fflags & FTLCLN)
		clean_up(0);
	if (Fflags & FTLFUNC)
		(*Ffunc)(msg);
	switch (Fflags & FTLACT) {
	case FTLJMP:
		longjmp(Fjmp, 1);	/* NOTREACHED */
	case FTLEXIT:
		exit(userexit(1));	/* NOTREACHED */
	case FTLRET:
	default:
		return(Fvalue);
	}
}

/* if running under NSE, the path to the directory which heads the
 * vcs hierarchy and the "s." is removed from the names of s.files
 *
 * if `vcshist' is true, a generic name for the history file is returned.
 */

char *
nse_file_trim(f, vcshist)
	char	*f;
	int	vcshist;
{
	register char	*p;
	char		*q;
	char		*r;
	char		c;

	r = f;
	if (HADQ) {
		if (vcshist && Ffile && (0 == strcmp(Ffile, f))) {
			return NSE_VCS_GENERIC_NAME;
		}
		if (!nsedelim) {
			nsedelim = NSE_VCS_DELIM;
		}
		if (delimlen == 0) {
			delimlen = strlen(nsedelim);
		}
		p = f;
		while (c = *p++) {
			/* find the NSE delimiter path component */
			if ((c == '/') && (*p == nsedelim[0]) &&
			    (0 == strncmp(p, nsedelim, delimlen)) &&
			    (*(p + delimlen) == '/')) {
				break;
			}
		}
		if (c) {
			/* if found, new name starts with second "/" */
			p += delimlen;
			q = strrchr(p, '/');
			/* find "s." */
			if (q && (q[1] == 's') && (q[2] == '.')) {
				/* build the trimmed name.
				 * <small storage leak here> */
				q[1] = '\0';
				r = (char *) malloc((unsigned) (strlen(p) +
						strlen(q+3) + 1));
				(void) strcpy(r, p);
				q[1] = 's';
				(void) strcat(r, q+3);
			}
		}
	}
	return r;
}
