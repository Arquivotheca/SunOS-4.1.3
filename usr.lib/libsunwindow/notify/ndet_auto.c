#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)ndet_auto.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Ndet_loop.c - Notification loop.
 */
#include <sys/types.h>
#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/ndis.h>	/* For ndis_enqueue */
#include <errno.h>
#include <signal.h>

extern	errno;

pkg_private_data u_int ndet_sigs_auto = 0;
pkg_private_data Notify_client ndet_auto_nclient =
	(Notify_client)&ndet_sigs_auto;

pkg_private void
ndet_toggle_auto(old_bits, sig)
	u_int old_bits;
	register int sig;
{
	u_int old_bit = old_bits & SIG_BIT(sig);
	u_int new_bit = ndet_sigs_auto & SIG_BIT(sig);

	if (old_bit && !new_bit)
		/* Turn auto signal catcher off for sig */
		(void)notify_set_signal_func(ndet_auto_nclient, NOTIFY_FUNC_NULL,
		    sig, NOTIFY_SYNC);
	else if (!old_bit && new_bit)
		/* Turn auto signal catcher on for sig */
		(void)notify_set_signal_func(ndet_auto_nclient, ndet_auto_sig_func,
		    sig, NOTIFY_SYNC);
}

/* ARGSUSED */
pkg_private Notify_value
ndet_auto_sig_func(nclient, sig, mode)
	Notify_client nclient;
	int sig;
	Notify_signal_mode mode;
{
	return(NOTIFY_DONE);
}

/* ARGSUSED */
pkg_private NTFY_ENUM
ndet_auto_sig_send(client, condition, context)
	NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	NTFY_ENUM ndet_auto_sigchld(), ndet_auto_sigterm();
	register NDET_ENUM_SEND *enum_send = (NDET_ENUM_SEND *)context;

	ntfy_assert(condition->type == NTFY_SYNC_SIGNAL, "Auto sig is async");
	/* Sweep all conditions & send notifications (recursive enumeration) */
	switch (condition->data.signal) {
	case SIGIO:
	case SIGURG: {
		NDET_ENUM_SEND enum_send2;
		register int loop_limiter = 0;
		fd_set	ibits, obits, ebits, tfd;
		int	nfds;

		/* Remember who was already notified (block copy get *bits) */
		enum_send2 = *enum_send;
		while (loop_limiter++ < 5) {
			FD_ZERO(&ibits); FD_ZERO(&obits); FD_ZERO(&ebits);
			if (condition->data.signal == SIGIO)
				ibits = obits = ndet_fasync_mask;
			else
				ebits = ndet_fasync_mask;
			/* Do a select to see who needs service */
			nfds = notify_select(FD_SETSIZE,
			    &ibits, &obits, &ebits, &ndet_polling_tv);
			/* See if select returned unconventionally */
			if (nfds == -1) {
				/* Note: *bits are undefined here */
				switch (errno) {
				case EINTR:
					/* Signals received so try again */
					continue;
				case EBADF:
					ntfy_fatal_error("2ndary select EBADF");
					return(NTFY_ENUM_NEXT);
				default:
					ntfy_fatal_error("2ndary select error");
					return(NTFY_ENUM_NEXT);
				}
			}
			/*
			 * Modify secondary enumeration record to be those
			 * selected but not already notified.
			 */
			/*
			  enum_send2.ibits = (ibits ^ enum_send2.ibits) & ibits;
			  enum_send2.obits = (obits ^ enum_send2.obits) & obits;
			  enum_send2.ebits = (ebits ^ enum_send2.ebits) & ebits;
			*/

			(void)ntfy_fd_cpy_xor(&enum_send2.ibits, &ibits);
			(void)ntfy_fd_cpy_and(&enum_send2.ibits, &ibits);
			(void)ntfy_fd_cpy_xor(&enum_send2.obits, &obits);
			(void)ntfy_fd_cpy_and(&enum_send2.obits, &obits);
			(void)ntfy_fd_cpy_xor(&enum_send2.ebits, &ebits);
			(void)ntfy_fd_cpy_and(&enum_send2.ebits, &ebits);
			
			/* Send notifications */
			if (ntfy_fd_anyset(&enum_send2.ibits) ||
			    ntfy_fd_anyset(&enum_send2.obits) ||
			    ntfy_fd_anyset(&enum_send2.ebits))
				(void) ntfy_enum_conditions(ndet_clients,
				    ndet_fd_send, (NTFY_ENUM_DATA)&enum_send2);
			/*
			 * Update those who have been notified
			 * (Note: tidy, but is this required?)
			 */
			(void)ntfy_fd_cpy_or(&enum_send->ibits, &enum_send2.ibits);
			(void)ntfy_fd_cpy_or(&enum_send->obits, &enum_send2.obits);
			(void)ntfy_fd_cpy_or(&enum_send->ebits, &enum_send2.ebits);
			break;
		}
		break;
		}
	case SIGCHLD: {
		NTFY_WAIT3_DATA wd;
		NTFY_ENUM ndet_wait_send();

		enum_send->wait3 = &wd;
		/* Look for as many children as have changed state */
		while ((wd.pid = wait3(&wd.status, WNOHANG | WUNTRACED,
		    &wd.rusage)) > 0)
			(void) ntfy_enum_conditions(ndet_clients,
			    ndet_auto_sigchld, context);
		break;
		}
	case SIGTERM:
		/* Terminate the notifier */
		(void) notify_stop();
		/* Set up to exit(1) after dispatch all notifications */
		ndet_flags |= NDET_EXIT_SOON;
		return(NTFY_ENUM_TERM);
	case SIGALRM:
		ndet_update_real_itimer();
		break;
	case SIGVTALRM:
		ndet_update_virtual_itimer();
		break;
	default:
		ntfy_fatal_error("Nclient unprepared to handle signal");
		break;
	}
	return(NTFY_ENUM_NEXT);
}

/*
 * Enqueue wait3 notification if the waited for pid matches the pid that
 * the notifier has noticed changing state.
 */
pkg_private NTFY_ENUM
ndet_auto_sigchld(client, condition, context)
	NTFY_CLIENT *client;
	register NTFY_CONDITION *condition;
	NTFY_ENUM_DATA context;
{
	register NDET_ENUM_SEND *enum_send = (NDET_ENUM_SEND *)context;
	NTFY_CLIENT client_tmp;
	NTFY_CONDITION condition_tmp;
	NTFY_WAIT3_DATA wait3_tmp;
	Notify_func functions[NTFY_FUNCS_MAX];

	if (condition->type == NTFY_WAIT3 &&
	    condition->data.pid == enum_send->wait3->pid) {
		/*
		 * Remember contents of following for call to ndis_enqueue
		 * because call to notify_set_wait3_func can invalidate.
		 */
		client_tmp = *client;
		condition_tmp = *condition;
		wait3_tmp = *enum_send->wait3;
		condition_tmp.data.wait3 = &wait3_tmp;
		if (condition->func_count > 1) {
			condition_tmp.callout.functions = functions;
			bcopy((caddr_t)condition->callout.functions,
			    (caddr_t)condition_tmp.callout.functions,
			    sizeof(NTFY_NODE));
		} else
			condition_tmp.callout.function =
			    condition->callout.function;
		/* Remove conditon if child process exited/killed */
		if (WIFEXITED(enum_send->wait3->status) ||
		    WIFSIGNALED(enum_send->wait3->status)) {
			/*
			 * Enumerator (which is calling this) is designed to
			 * survive having condition disappear in middle of
			 * enumeration.
			 */
			(void)notify_set_wait3_func(client->nclient, 
			    NOTIFY_FUNC_NULL, condition->data.pid);
		}
		/* Dispatch notification */
		if (ndis_enqueue(&client_tmp, &condition_tmp) != NOTIFY_OK)
			ntfy_fatal_error("Error when enq condition");
	}
	return(NTFY_ENUM_NEXT);
}

