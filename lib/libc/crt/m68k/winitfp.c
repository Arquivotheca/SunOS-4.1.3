#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)winitfp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include <sys/file.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include "fpcrttypes.h"
#include <floatingpoint.h>

extern int      errno;
int             _fpa_fd;	/* FPA file descriptor - needed outside at
				 * times. */

extern int
                fpa_handler();

int
winitfp_()
/*
 * Procedure to determine if a physical FPA and 68881 are present and set
 * fp_state_sunfpa and fp_state_mc68881 accordingly. Also returns 1 if both
 * present, 0 otherwise. 
 */

{
	int             mode81;
	long           *fpaptr;
	int             savederrno = errno;

	if (fp_state_sunfpa == fp_unknown) {
		if (minitfp_() != 1)
			fp_state_sunfpa = fp_absent;
		else {
			_fpa_fd = open("/dev/fpa", O_RDWR);
			if ((_fpa_fd < 0) && (errno != EEXIST)) {	/* _fpa_fd < 0 */
				if (errno == EBUSY) {
					fprintf(stderr, "\n No Sun FPA contexts available - all in use\n");
					fflush(stderr);
				}
				fp_state_sunfpa = fp_absent;
			}
			/* _fpa_fd < 0 */
			else {	/* _fpa_fd >= 0 */
				if (errno == EEXIST) {
					fprintf(stderr, "\n  FPA was already open \n");
					fflush(stderr);
				}
				/* to close FPA context on execve() */
				fcntl(_fpa_fd, F_SETFD, 1);
				sigfpe(FPE_FPA_ERROR, (sigfpe_handler_type) fpa_handler);
				fpaptr = (int *) 0xe00008d0;	/* write mode register */
				*fpaptr = 2;	/* set to round integers
						 * toward zero */
				fpaptr = (int *) 0xe0000f14;
				*fpaptr = 1;	/* set imask to one */
				fp_state_sunfpa = fp_enabled;
			}	/* _fpa_fd >= 0 */
		}
	}
	if (fp_state_sunfpa == fp_enabled)
		fp_switch = fp_sunfpa;
	errno = savederrno;
	return ((fp_state_sunfpa == fp_enabled) ? 1 : 0);
}
