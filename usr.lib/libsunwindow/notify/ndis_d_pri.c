#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndis_d_pri.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndis_d_pri.c - Default prioritizer for dispatcher.
 */
#include <sunwindow/ntfy.h>
#include <sunwindow/ndis.h>
#include <signal.h>

typedef	enum notify_error (*Notify_error_func)();

static	void ndis_send_ascending_sig();
static	void ndis_send_ascending_fd();

pkg_private Notify_value
ndis_default_prioritizer(nclient, nfd, ibits_ptr, obits_ptr, ebits_ptr,
    nsig, sigbits_ptr, auto_sigbits_ptr, event_count_ptr, events, args)
	register Notify_client nclient;
	fd_set *ibits_ptr, *obits_ptr, *ebits_ptr;
	int  nsig, *sigbits_ptr, *event_count_ptr;
	register int *auto_sigbits_ptr, nfd;
	Notify_event *events;
	Notify_arg *args;
{
	register int i;

	if (*auto_sigbits_ptr) {
		/* Send itimers */
		if (*auto_sigbits_ptr & SIG_BIT(SIGALRM)) {
			(void) notify_itimer(nclient, ITIMER_REAL);
			*auto_sigbits_ptr &= ~SIG_BIT(SIGALRM);
		}
		if (*auto_sigbits_ptr & SIG_BIT(SIGVTALRM)) {
			(void) notify_itimer(nclient, ITIMER_VIRTUAL);
			*auto_sigbits_ptr &= ~SIG_BIT(SIGVTALRM);
		}
		/* Send child process change */
		if (*auto_sigbits_ptr & SIG_BIT(SIGCHLD)) {
			(void) notify_wait3(nclient);
			*auto_sigbits_ptr &= ~SIG_BIT(SIGCHLD);
		}
	}
	if (*sigbits_ptr)
		/* Send signals (by ascending signal numbers) */
		ndis_send_ascending_sig(nclient, nsig, sigbits_ptr,
		    notify_signal);
	if (ntfy_fd_anyset(ebits_ptr))
		/* Send exception fd activity (by ascending fd numbers) */
		ndis_send_ascending_fd(nclient, nfd, ebits_ptr,
		    notify_exception);
	/* Send client event (in order received) */
	for (i = 0; i < *event_count_ptr; i++)
		(void) notify_event(nclient, *(events+i), *(args+i));
	*event_count_ptr = 0;
	if (ntfy_fd_anyset(obits_ptr))
		/* Send output fd activity (by ascending fd numbers) */
		ndis_send_ascending_fd(nclient, nfd, obits_ptr, notify_output);
	if (ntfy_fd_anyset(ibits_ptr))
		/* Send input fd activity (by ascending fd numbers) */
		ndis_send_ascending_fd(nclient, nfd, ibits_ptr, notify_input);
	if (*auto_sigbits_ptr) {
		/* Send destroy checking */
		if (*auto_sigbits_ptr & SIG_BIT(SIGTSTP)) {
			if ((notify_destroy(nclient, DESTROY_CHECKING) ==
			    NOTIFY_DESTROY_VETOED) &&
			    (*auto_sigbits_ptr & SIG_BIT(SIGTERM))) {
				/* Remove DESTROY_CLEANUP from dispatch list. */
				notify_flush_pending(nclient);
				/* Prevent DESTROY_CLEANUP in this call */
				*auto_sigbits_ptr &= ~SIG_BIT(SIGTERM);
			}
			*auto_sigbits_ptr &= ~SIG_BIT(SIGTSTP);
		}
		/* Send destroy (only one of them) */
		if (*auto_sigbits_ptr & SIG_BIT(SIGTERM)) {
			(void) notify_destroy(nclient, DESTROY_CLEANUP);
			*auto_sigbits_ptr &= ~SIG_BIT(SIGTERM);
		} else if (*auto_sigbits_ptr & SIG_BIT(SIGKILL)) {
			(void) notify_destroy(nclient, DESTROY_PROCESS_DEATH);
			*auto_sigbits_ptr &= ~SIG_BIT(SIGKILL);
		}
	}
	return(NOTIFY_DONE);
}

static	void
ndis_send_ascending_fd(nclient, nbits, bits_ptr, func)
	Notify_client nclient;
	register int nbits;
	fd_set *bits_ptr;
	Notify_error_func func;
{
	register base, offset, bit;
	register fd, i, byteNum;
	unsigned long byte;

	/* Send fd (by ascending numbers) */
	for (i = 0; i < nbits/NFDBITS; i++) {
        	if (bits_ptr->fds_bits[i]) {  
			/* 
			 * For each fd_mask set in bits_ptr, mask off all but
			 * one byte and see if anything is set
			 */
			base = i * NFDBITS;
			for (byte = 0xff, byteNum = 0; byte != 0L;
                                          byte <<= NBBY, byteNum++) {
				if (bits_ptr->fds_bits[i] & byte) {
					offset = byteNum * NBBY;
					for (bit = 0; bit < NBBY; bit++) {
						fd = base + offset + bit;
                                        	if (FD_ISSET(fd, bits_ptr))  {
                                                	(void)func(nclient,fd);
                                                        FD_CLR(fd, bits_ptr);
                                                }
					}
				}
			}
		}
	}
}

static	void
ndis_send_ascending_sig(nclient, nbits, bits_ptr, func)
	Notify_client nclient;
	register int nbits, *bits_ptr;
	Notify_error_func func;
{
	register sig, bit;

	/* Send func (by ascending numbers) */
	for (sig = 1; sig <= nbits; sig++) {
		bit = SIG_BIT(sig);
		if (*bits_ptr & bit) {
			(void) func(nclient, sig);
			*bits_ptr &= ~bit;
		}
	}
}
