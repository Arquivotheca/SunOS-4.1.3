#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_ttyenv.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Implement the win_environ.h & win_sig.h interfaces.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_environ.h>
extern char *strcat();

#ifdef	TTYS_SET_TERMCAP
static	char *_we_tcformat1 = "Mu|sun:li#";

static	char *_we_tcformat2 = ":co#";

static	char *_we_tcformat3 =
":cl=^L:cm=\\E[%i%d;%dH:nd=\\E[C:up=\\E[A:am:bs:km:mi:ms:pt:ce=\\E[K:cd=\\E[J:so=\\E[7m:se=\\E[m:kd=\\E[B:kl=\\E[D:ku=\\E[A:kr=\\E[C:kh=\\E[H:k1=\\EOP:k2=\\EOQ:k3=\\EOR:k4=\\EOS:al=\\E[L:dl=\\E[M:im=:ei=:ic=\\E[@:dc=\\E[P:";

sigwinch_sizeinchars(width, height, setTERMCAP)
	int	*width, *height, setTERMCAP;
{
#ifdef	DEBUG
	char	*error_msg = NULL;
#endif
	char	windowname[WIN_NAMESIZE], *tcstring, *calloc();
	int	margin, charwidth, charheight, windowfd;
	struct	rect rect;

	/*
	 * Get window handle
	 */
	if (we_getmywindow(windowname)) {
#ifdef	DEBUG
	    error_msg = "can't get window from environment"
#endif	DEBUG
	    goto Return;
	}
	if ((windowfd = open(windowname, 0))<0) {
#ifdef	DEBUG
	    perror(windowname);
	    error_msg = "trouble opening window"
#endif	DEBUG
	    goto Return;
	}
	/*
	 * Get font height and width set in parent process.
	 */
	if (we_getterminalsizes(&charwidth, &charheight, &margin)) {
#ifdef	DEBUG
	    error_msg = "can't get term sizes from environ"
#endif	DEBUG
	    goto Done;
	}
	/*
	 * Calculate width & height in characters.
	 */
	if (ioctl(windowfd, WINGETRECT, &rect) == -1) {
#ifdef	DEBUG
	    extern	errno;

	    printf("problem getting window size %ld\n", errno);
#endif	DEBUG
	    goto Done;
	}
	*width = (rect.r_width-2*margin)/charwidth;
	*height = (rect.r_height-2*margin)/charheight;
	/*
	 * Set termcap
	 */
	if (setTERMCAP) {
		tcstring = (char *) calloc(
		    (strlen(_we_tcformat1)+5+strlen(_we_tcformat2)+5+
		    strlen(_we_tcformat3))*sizeof(char), 1);
		if (tcstring == 0) {
#ifdef	DEBUG
			error_msg = "temp string NULL"
#endif	DEBUG
			goto Done;
		}
		strcpy(tcstring, _we_tcformat1);
		_we_catdecimal(tcstring, *height);
		(void)strcat(tcstring, _we_tcformat2);
		_we_catdecimal(tcstring, *width);
		(void)strcat(tcstring, _we_tcformat3);
		/*
		 * Note: Have to do the previous bull instead of a nice tidy:
		 *	sprintf(tcstring, _we_tcformat, *height, *width);
		 * to avoid bringing in stdio stuff.
		 * (& the csh has own printf package that puts sprintf
		 * onto the standard out).
		 */
		setenv("TERMCAP", tcstring);
		free(tcstring);
	}
Done:
	close(windowfd);
Return:
#ifdef	DEBUG
	if (error_msg != NULL)
	    fprintf(stderr, "sigwinch_sizeinchars %s\n", error_msg);
#endif
	return;
}
#endif	TTYS_SET_TERMCAP

int
we_getmywindow(windevname)
	char	*windevname;
{
	return(_we_setstrfromenvironment(WE_ME, windevname));
}

#ifdef	TTYS_SET_TERMCAP
int
we_getterminalsizes(char_width, char_height, margin)
	int *char_width, *char_height, *margin;
{
	char sizestr[60], *s = &sizestr[0];

	if (_we_setstrfromenvironment(WE_TERMINALSIZES, sizestr))
		return(-1);
	else {
		if (_we_scanpositivedecimal(&s, char_width))
			return(-1);
		if (_we_scanpositivedecimal(&s, char_height))
			return(-1);
		if (_we_scanpositivedecimal(&s, margin))
			return(-1);
		/*
		 * Note: Have to the previous bull instead of a nice tidy:
		 *	if (sscanf(sizestr, "%hd,%hd,%hd",
		 *	    char_width, char_height, margin)!=3)
		 *		return(-1);
		 * to avoid bringing in stdio stuff.
		 */
		return(0);
	}
}
#endif	TTYS_SET_TERMCAP

/*
 * Private utilities
 */
char	*
_we_getenvironment(tag)
	char	*tag;
{
	char *en_str, *getenv();

	en_str=getenv(tag);
	return(en_str);
}

int
_we_setstrfromenvironment(tag, target)
	char	*tag, *target;
{
	char *en_str;

	*target = 0;
	if (en_str = _we_getenvironment(tag)) {
		(void)strcat(target, en_str);
		return(0);
	} else {
		return(-1);
	}
}

#ifdef	TTYS_SET_TERMCAP
_we_catdecimal(string, val)
	char	*string;
	int	val;
{
#define	HIBIT	0x80000000L
#define	MAXDIGS 20
	int	hradix = 5;
	char	buf[MAXDIGS], *bp;
	int	lowbit;
	char	*tab = "0123456789abcdef";

	bp = &buf[0] + MAXDIGS;
	*(--bp) = '\0';
	while (val) {
		lowbit = val & 1;
		val = (val >> 1) & ~HIBIT;
		*(--bp) = tab[val % hradix * 2 + lowbit];
		val /= hradix;
	}
	(void)strcat(string, bp);
}

int
_we_scanpositivedecimal(string, val)
	char	**string;
	int	*val;
{
	int	lcval = 0, digitfound = 0;
	char	c;

	if (string == 0 || *string == 0)
		return(1);
	for (c = **string; c != '\0'; c = *(++(*string))) {
		/*
		 * Check if found a digit
		 */
		if (c >= '0' && c <= '9') {
			c -= '0';
			digitfound = 1;
		} else {
			if (digitfound)
				/*
				 * Encountered trailing non-digits
				 */
				break;
			else
				/*
				 * Removing leading non-digits
				 */
				continue;
		}
		/*
		 * Change lcval if lcval is not zero or c is not zero
		 * (leading blank removal)
		 */
		if (lcval || c) {
			lcval *= 10;
			lcval += c;
		}
	}
	*val = lcval;
	return(!digitfound);
}
#endif	TTYS_SET_TERMCAP

