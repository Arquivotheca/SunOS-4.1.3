#ifndef lint
static  char sccsid[] = "@(#)kbd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (C) 1987 by Sun Microsystems, Inc.
 */
/*
 * Keyboard input streams module - handles conversion of up/down codes to
 * ASCII or event format.
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/termios.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/reboot.h>
#include <machine/vm_hat.h>
#ifdef	OPENPROMS
#include <mon/obpdefs.h>
#else	OPENPROMS
#include <mon/sunromvec.h>
#endif	OPENPROMS
#include <sun/consdev.h>
#include <sundev/kbd.h>
#include <sundev/kbio.h>
#include <sundev/kbdreg.h>
#include <sundev/vuid_event.h>
#include <debug/debug.h>



#ifndef AT386
#ifndef	OPENPROMS
#define	prom_enter_mon()	montrap(*romp->v_abortent)
#else	OPENPROMS
#ifndef	MULTIPROCESSOR
extern void	prom_enter_mon();
#else	MULTIPROCESSOR
#define	prom_enter_mon()	mpprom_enter()
#endif	MULTIPROCESSOR
#endif	OPENPROMS
#endif	AT386

/*
 * For now these are shared.
 */
extern int nkeytables;
extern struct keyboard	*keytables[];
extern char keystringtab[16][KTAB_STRLEN];
extern struct compose_sequence_t kb_compose_table[];
extern struct fltaccent_sequence_t kb_fltaccent_table[];
extern u_char kb_numlock_table[];

/*
 * Keyboard instance data.
 */
int	kbd_downs_size = 15;	/* Approx. corresponds to max 10 fingers */
typedef	struct	key_event {
	u_char	key_station;	/* Physical key station associated with event */
	Firm_event event;	/* Event that sent out on down */
} Key_event;

struct	kbddata {
	queue_t	*kbdd_readq;
	queue_t *kbdd_writeq;
	mblk_t	*kbdd_iocpending;	/* "ioctl" awaiting buffer */
	mblk_t	*kbdd_replypending;	/* "ioctl" reply awaiting result */
	int	kbdd_flags;		/* random flags */
	int	kbdd_iocid;		/* ID of "ioctl" being waited for */
	int	kbdd_iocerror;		/* error return from "ioctl" */
	int	kbdd_wbufcid;		/* ID of pending write-side bufcall */
	/*
	 * State of keyboard & keyboard specific settings, e.g., tables
	 */
	struct	keyboardstate kbdd_state;
	int	kbdd_translate;		/* Translate keycodes? */
	int	kbdd_translatable;	/* Keyboard is translatable? */
	int	kbdd_compat;		/* Generating pre-4.1 events? */
	short	kbdd_ascii_addr;	/* Vuid_id_addr for ascii events */
	short	kbdd_top_addr;		/* Vuid_id_addr for top events */
	short	kbdd_vkey_addr;		/* Vuid_id_addr for vkey events */
	/*
	 * Table of key stations currently down that have firm events
	 * that need to be matched with up transitions when kbdd_translate
	 *  is TR_*EVENT
	 */
	struct	key_event *kbdd_downs;
	int	kbdd_downs_entries;	/* # of possible entries in kbdd_downs*/
	u_int	kbdd_downs_bytes;	/* # of bytes allocated for kbdd_downs*/
	u_char	compose_key;		/* first ASCII compose key (xlated) */
	u_short	fltaccent_entry;	/* floating accent keymap entry */
	char	led_state;		/* current state of LEDs */
};

#define	KBD_OPEN	0x00000001 /* keyboard is open for business */
#define	KBD_IOCWAIT	0x00000002 /* "open" waiting for "ioctl" to finish */

/*
 * Constants setup during the first open of a kbd (so that hz is defined).
 */
int	kbd_repeatrate;
int	kbd_repeatdelay;

int	kbd_overflow_cnt;	/* Number of times kbd overflowed input q */
int	kbd_overflow_msg = 1;	/* Whether to print message on q overflow */

#ifdef	KBD_DEBUG
int	kbd_debug = 0;
int	kbd_ra_debug = 0;
int	kbd_raw_debug = 0;
int	kbd_rpt_debug = 0;
int	kbd_input_debug = 0;
#endif	KBD_DEBUG

/*
 * Should be "void", not "int" - they return no values!
 */
static int	kbdopen(/*queue_t *q, int dev, int oflag, int sflag*/);
static int	kbdclose(/*queue_t *q*/);
static int	kbdwput(/*queue_t *q, mblk_t *mp*/);
static int	kbdrput(/*queue_t *q, mblk_t *mp*/);

static struct module_info kbdmiinfo = {
	0,
	"kbd",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit kbdrinit = {
	kbdrput,
	NULL,
	kbdopen,
	kbdclose,
	NULL,
	&kbdmiinfo
};

static struct module_info kbdmoinfo = {
	0,
	"kbd",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit kbdwinit = {
	kbdwput,
	NULL,
	kbdopen,
	kbdclose,
	NULL,
	&kbdmoinfo
};

struct streamtab kbd_info = {
	&kbdrinit,
	&kbdwinit,
	NULL,
	NULL,
	NULL
};

static int	kbdreioctl(/*long kbddptr*/);
static void	kbdioctl(/*queue_t *q, mblk_t *mp*/);
static void	kbdflush(/*struct kbddata kbdd*/);
static void	kbduse(/*struct kbddata kbdd, u_char keycode*/);
static void	kbdsetled(/*struct kbddata *kbdd*/);
static void	kbdcmd(/*queue_t *q, char cmd*/);
static void	kbdreset(/*struct kbddata *kbdd*/);
static int	kbdsetkey(/*struct kbddata *kbdd, struct kiockey *key*/);
static int	kbdgetkey(/*struct kbddata *kbdd, struct kiockey *key*/);
static int	kbdskey(/*struct kbddata *kbdd, struct kiockeymap *key*/);
static int	kbdgkey(/*struct kbddata *kbdd, struct kiockeymap *key*/);
static int	kbdidletimeout(/*struct kbddata *kbdd*/);
static int	kbdlayouttimeout(/*struct kbddata *kbdd*/);
static void	kbdinput(/*struct kbddata *kbdd, u_char key*/);
static void	kbdid(/*struct kbddata *kbdd, int id*/);
static struct keymap *settable(/*struct kbddata *kbdd, u_int mask*/);
static int	kbdrpt(/*struct kbddata *kbdd*/);
static void	kbdcancelrpt(/*struct kbddata *kbdd*/);
static void	kbdtranslate(/*struct kbddata *kbdd, u_char keycode,
	queue_t *q*/);
static void	kbd_send_esc_event(/*char c, struct kbddata *kbdd*/);
char *strsetwithdecimal(/*char *buf, u_int val, u_int maxdigs*/);
static void	kbdkeypressed(/*struct kbddata *kbdd, u_char key_station,
	Firm_event *fe, u_short base*/);
static void	kbdqueuepress(/*struct kbddata *kbdd, u_char key_station,
	Firm_event *fe*/);
static void	kbdkeyreleased(/*struct kbddata *kbdd, u_char key_station*/);
static void	kbdreleaseall(/*struct kbddata *kbdd*/);
static void	kbdputcode(/*u_int code, queue_t *q*/);
static void	kbdqueueevent(/*struct kbddata *kbdd, Firm_event *fe*/);

/*
 * Open a keyboard.
 * Ttyopen sets line characteristics
 */
/* ARGSUSED */
static int
kbdopen(q, dev, oflag, sflag)
	queue_t *q;
	int dev, oflag, sflag;
{
	register struct	kbddata *kbdd;
	mblk_t *mp;
	mblk_t *datap;
	register struct iocblk *iocb;
	register struct termios *cb;

	/* Set these up only once so that they could be changed from adb */
	if (!kbd_repeatrate) {
		kbd_repeatrate = (hz+29)/30;
		kbd_repeatdelay = hz/2;
	}

	if (q->q_ptr != NULL)
		return (0);		/* already attached */

	/* We must be called with MODOPEN */
	if (sflag != MODOPEN)
		return (OPENFAIL);

	/*
	 * Allocate a kbddata structure.
	 */
	kbdd = (struct kbddata *)new_kmem_zalloc(
				sizeof (struct kbddata), KMEM_SLEEP);

	/*
	 * Set up queue pointers, so that the "put" procedure will accept
	 * the reply to the "ioctl" message we send down.
	 */
	q->q_ptr = (caddr_t)kbdd;
	WR(q)->q_ptr = (caddr_t)kbdd;


	/*
	 * Setup tty modes.
	 */
	if ((mp = allocb(sizeof (struct iocblk), BPRI_HI)) == NULL) {
		if (strwaitbuf(sizeof (struct iocblk), BPRI_HI, 1))
			return (OPENFAIL);
	}
	if ((datap = allocb(sizeof (struct termios), BPRI_HI)) == NULL) {
		if (strwaitbuf(sizeof (struct termios), BPRI_HI, 1)) {
			freemsg(mp);
			return (OPENFAIL);
		}
	}
	iocb = (struct iocblk *)mp->b_wptr;
	iocb->ioc_cmd = TCSETSF;
	iocb->ioc_uid = 0;
	iocb->ioc_gid = 0;
	iocb->ioc_id = getiocseqno();
	iocb->ioc_count = sizeof (struct iocblk);
	iocb->ioc_error = 0;
	iocb->ioc_rval = 0;
	mp->b_wptr += (sizeof *iocb)/(sizeof *datap->b_wptr);
	mp->b_datap->db_type = M_IOCTL;
	cb = (struct termios *)datap->b_wptr;
	cb->c_iflag = 0;
	cb->c_oflag = 0;
	cb->c_cflag = CREAD|CS8|B1200;
	cb->c_lflag = 0;
	cb->c_line = 0;
	bzero((caddr_t)cb->c_cc, NCCS);
	datap->b_wptr += (sizeof *cb)/(sizeof *datap->b_wptr);
	mp->b_cont = datap;
	kbdd->kbdd_flags |= KBD_IOCWAIT; /* indicate that we're waiting for */
	kbdd->kbdd_iocid = iocb->ioc_id; /* this response */
	putnext(WR(q), mp);

	/*
	 * Now wait for it.  Let our read queue put routine wake us up
	 * when it arrives.
	 */
	while (kbdd->kbdd_flags & KBD_IOCWAIT) {
		if (sleep((caddr_t)&kbdd->kbdd_iocerror, STOPRI|PCATCH)) {
			u.u_error = EINTR;
			goto error;
		}
	}
	if (u.u_error = kbdd->kbdd_iocerror)
		goto error;

	/*
	 * Set up private data.
	 */
	kbdd->kbdd_readq = q;
	kbdd->kbdd_writeq = WR(q);
	kbdd->kbdd_iocpending = NULL;
	kbdd->kbdd_wbufcid = 0;
	kbdd->kbdd_translatable = TR_CAN;
	kbdd->kbdd_translate = TR_ASCII;
	kbdd->kbdd_compat = 1;
	kbdd->kbdd_ascii_addr = ASCII_FIRST;
	kbdd->kbdd_top_addr = TOP_FIRST;
	kbdd->kbdd_vkey_addr = VKEY_FIRST;
	/* Allocate dynamic memory for downs table */
	kbdd->kbdd_downs_entries = kbd_downs_size;
	kbdd->kbdd_downs_bytes = kbd_downs_size * sizeof (Key_event);
	kbdd->kbdd_downs = (Key_event *)new_kmem_alloc(
				kbdd->kbdd_downs_bytes, KMEM_SLEEP);
	kbdd->kbdd_flags = KBD_OPEN;
	kbdd->led_state = 0;

	/*
	 * Reset kbd.
	 */
	kbdreset(kbdd);
	return (0);

error:
	kmem_free((caddr_t)kbdd, sizeof (struct kbddata));
	return (OPENFAIL);
}

/*
 * Close a keyboard.
 */
static int
kbdclose(q)
	register queue_t *q;
{
	register struct kbddata *kbdd = (struct kbddata *)q->q_ptr;
	register mblk_t *mp;
	int s;

	/*
	 * Since we're about to destroy our private data, turn off
	 * our open flag first, so we don't accept any more input
	 * and try to use that data.
	 */
	kbdd->kbdd_flags = 0;

	s = splstr();
	if ((mp = kbdd->kbdd_iocpending) != NULL) {
		/*
		 * There was an "ioctl" pending the availability of an
		 * "mblk".  Release the message since the "ioctl" must
		 * have timed out or been aborted.
		 */
		kbdd->kbdd_iocpending = NULL;
		freemsg(mp);
	}
	if ((mp = kbdd->kbdd_replypending) != NULL) {
		/*
		 * There was a KIOCLAYOUT pending; presumably, it timed out.
		 * Throw the reply away.
		 */
		kbdd->kbdd_replypending = NULL;
		freemsg(mp);
	}
	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (kbdd->kbdd_wbufcid)
		unbufcall(kbdd->kbdd_wbufcid);
	(void) splx(s);

	/*
	 * Free kbddata structure.
	 */
	kmem_free((caddr_t)kbdd->kbdd_downs, kbdd->kbdd_downs_bytes);
	kmem_free((caddr_t)kbdd, sizeof (*kbdd));
}

/*
 * Line discipline output queue put procedure: handles M_IOCTL
 * messages.
 */
static int
kbdwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{

	/*
	 * Process M_FLUSH, and some M_IOCTL, messages here; pass
	 * everything else down.
	 */
	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(q), FLUSHDATA);

	default:
		putnext(q, mp);	/* pass it down the line */
		return;

	case M_IOCTL:
		kbdioctl(q, mp);
		break;
	}
}

static int
kbdreioctl(kbddptr)
	long kbddptr;
{
	struct kbddata *kbdd = (struct kbddata *)kbddptr;
	register queue_t *q;
	register mblk_t *mp;

	/*
	 * The bufcall is no longer pending.
	 */
	kbdd->kbdd_wbufcid = 0;
	if ((q = kbdd->kbdd_writeq) == 0)
		return;
	if ((mp = kbdd->kbdd_iocpending) != NULL) {
		kbdd->kbdd_iocpending = NULL;	/* not pending any more */
		kbdioctl(q, mp);
	}
}

static void
kbdioctl(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct kbddata *kbdd = (struct kbddata *)q->q_ptr;
	register struct iocblk *iocp;
	caddr_t  data;
	register short	new_translate;
	register Vuid_addr_probe *addr_probe;
	register short	*addr_ptr;
	u_int	ioctlrespsize;
	int	pri;
	int	err = 0;

	iocp = (struct iocblk *)mp->b_rptr;
	if (mp->b_cont != NULL)
		data = (caddr_t)mp->b_cont->b_rptr;

	pri = spl5();
	switch (iocp->ioc_cmd) {

	case VUIDSFORMAT:
		new_translate = (*(int *)data == VUID_NATIVE)?
		    TR_ASCII: TR_EVENT;
		if (new_translate == kbdd->kbdd_translate)
			break;
		kbdd->kbdd_translate = new_translate;
		goto output_format_change;

	case KIOCTRANS:
		new_translate = *(int *)data;
		if (new_translate == kbdd->kbdd_translate)
			break;
		kbdd->kbdd_translate = new_translate;
		goto output_format_change;

	case KIOCCMD:
		kbdcmd(q, (char)(*(int *)data));
		break;

	case KIOCSLED:
		kbdd->led_state = *(u_char *)data;
		kbdsetled(kbdd);
		break;

	case KIOCGLED: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (u_char), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(u_char *)datap->b_wptr = kbdd->led_state;
		datap->b_wptr += sizeof (u_char);
		mp->b_cont = datap;
		iocp->ioc_count = sizeof (u_char);
		break;
	}

	case VUIDGFORMAT: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(int *)datap->b_wptr =
		    (kbdd->kbdd_translate == TR_EVENT ||
		    kbdd->kbdd_translate == TR_UNTRANS_EVENT) ?
			VUID_FIRM_EVENT: VUID_NATIVE;
		datap->b_wptr += sizeof (int);
		mp->b_cont = datap;
		iocp->ioc_count = sizeof (int);
		break;
	}

	case KIOCGTRANS: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(int *)datap->b_wptr = kbdd->kbdd_translate;
		datap->b_wptr += sizeof (int);
		mp->b_cont = datap;
		iocp->ioc_count = sizeof (int);
		break;
	}

	case VUIDSADDR:
		addr_probe = (Vuid_addr_probe *)data;
		switch (addr_probe->base) {

		case ASCII_FIRST:
			addr_ptr = &kbdd->kbdd_ascii_addr;
			break;

		case TOP_FIRST:
			addr_ptr = &kbdd->kbdd_top_addr;
			break;

		case VKEY_FIRST:
			addr_ptr = &kbdd->kbdd_vkey_addr;
			break;

		default:
			err = ENODEV;
		}
		if ((err == 0) && (*addr_ptr != addr_probe->data.next)) {
			*addr_ptr = addr_probe->data.next;
			goto output_format_change;
		}
		break;

	case VUIDGADDR:
		addr_probe = (Vuid_addr_probe *)data;
		switch (addr_probe->base) {

		case ASCII_FIRST:
			addr_probe->data.current = kbdd->kbdd_ascii_addr;
			break;

		case TOP_FIRST:
			addr_probe->data.current = kbdd->kbdd_top_addr;
			break;

		case VKEY_FIRST:
			addr_probe->data.current = kbdd->kbdd_vkey_addr;
			break;

		default:
			err = ENODEV;
		}
		break;

	case KIOCTRANSABLE:
		if (kbdd->kbdd_translatable != *(int *)data) {
			kbdd->kbdd_translatable = *(int *)data;
			goto output_format_change;
		}
		break;

	case KIOCGTRANSABLE: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(int *)datap->b_wptr = kbdd->kbdd_translatable;
		datap->b_wptr += sizeof (int);
		mp->b_cont = datap;
		iocp->ioc_count = sizeof (int);
		break;
	}

	case KIOCSCOMPAT:
		kbdd->kbdd_compat = *(int *)data;
		break;

	case KIOCGCOMPAT: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(int *)datap->b_wptr = kbdd->kbdd_compat;
		datap->b_wptr += sizeof (int);
		mp->b_cont = datap;
		iocp->ioc_count = sizeof (int);
		break;
	}

	case KIOCSETKEY:
		err = kbdsetkey(kbdd, (struct kiockey *)data);
		/*
		 * Since this only affects any subsequent key presses,
		 * don't goto output_format_change.  One might want to
		 * toggle the keytable entries dynamically.
		 */
		break;

	case KIOCGETKEY:
		err = kbdgetkey(kbdd, (struct kiockey *)data);
		break;

	case KIOCSKEY:
		err = kbdskey(kbdd, (struct kiockeymap *)data);
		/*
		 * Since this only affects any subsequent key presses,
		 * don't goto output_format_change.  One might want to
		 * toggle the keytable entries dynamically.
		 */
		break;

	case KIOCGKEY:
		err = kbdgkey(kbdd, (struct kiockeymap *)data);
		break;

	case KIOCSDIRECT:
		goto output_format_change;

	case KIOCGDIRECT: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(int *)datap->b_wptr = 1;	/* always direct */
		datap->b_wptr += sizeof (int);
		mp->b_cont = datap;
		iocp->ioc_count = sizeof (int);
		break;
	}

	case KIOCTYPE: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(int *)datap->b_wptr = (kbdd->kbdd_state.k_idstate == KID_OK)?
		    kbdd->kbdd_state.k_id: -1;
		datap->b_wptr += sizeof (int);
		mp->b_cont = datap;
		iocp->ioc_count = sizeof (int);
		break;
	}

	case KIOCLAYOUT: {
		register mblk_t *datap;

		if ((datap = kbdd->kbdd_replypending) != NULL) {
			/*
			 * There was an earlier KIOCLAYOUT pending; presumably,
			 * it timed out.  Throw the reply away.
			 */
			kbdd->kbdd_replypending = NULL;
			freemsg(datap);
		}

		if (kbdd->kbdd_state.k_id == KB_SUN4) {
			if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
				ioctlrespsize = sizeof (int);
				goto allocfailure;
			}
			iocp->ioc_rval = 0;
			iocp->ioc_error = 0;	/* brain rot */
			iocp->ioc_count = sizeof (int);
			mp->b_cont = datap;
			mp->b_datap->db_type = M_IOCACK;
			kbdd->kbdd_replypending = mp;
			kbdcmd(q, (char)KBD_CMD_GETLAYOUT);
			timeout(kbdlayouttimeout, (caddr_t)kbdd, hz/2);
			(void) splx(pri);
			return;		/* wait for reply from keyboard */
		} else {
			/*
			 * Not a Type 4 keyboard; return an immediate error.
			 */
			err = EINVAL;
			break;
		}
	}

	default:
		(void) splx(pri);
		putnext(q, mp);	/* pass it down the line */
		return;
	}
	goto done;

output_format_change:
	kbdflush(kbdd);
	if (iocp->ioc_cmd != KIOCTRANS)
		kbdreset(kbdd);

done:
	(void) splx(pri);
	if (err != 0) {
		iocp->ioc_rval = 0;
		iocp->ioc_error = err;
		mp->b_datap->db_type = M_IOCNAK;
	} else {
		iocp->ioc_rval = 0;
		iocp->ioc_error = 0;	/* brain rot */
		mp->b_datap->db_type = M_IOCACK;
	}
	qreply(q, mp);
	return;

allocfailure:
	/*
	 * We needed to allocate something to handle this "ioctl", but
	 * couldn't; save this "ioctl" and arrange to get called back when
	 * it's more likely that we can get what we need.
	 * If there's already one being saved, throw it out, since it
	 * must have timed out.
	 */
	pri = splstr();
	if (kbdd->kbdd_iocpending != NULL)
		freemsg(kbdd->kbdd_iocpending);
	kbdd->kbdd_iocpending = mp;
	if (kbdd->kbdd_wbufcid)
		unbufcall(kbdd->kbdd_wbufcid);
	kbdd->kbdd_wbufcid = bufcall(ioctlrespsize, BPRI_HI, kbdreioctl,
	    (long)kbdd);
	(void) splx(pri);
	return;
}

static void
kbdflush(kbdd)
	register struct kbddata *kbdd;
{
	register queue_t *q;

	/* Flush pending data already sent upstream */
	if ((q = kbdd->kbdd_readq) != NULL && (q = q->q_next) != NULL)
		(void) putctl1(q, M_FLUSH, FLUSHR);
	/* Flush pending ups */
	bzero((caddr_t)(kbdd->kbdd_downs), kbdd->kbdd_downs_bytes);
	kbdcancelrpt(kbdd);
}

/*
 * Pass keycode upstream, either translated or untranslated.
 */
static void
kbduse(kbdd, keycode)
	register struct kbddata *kbdd;
	u_char	keycode;
{
	register queue_t *readq;

#ifdef	KBD_DEBUG
	if (kbd_input_debug) printf("KBD USE key=%d\n", keycode);
#endif

	if ((readq = kbdd->kbdd_readq) == NULL)
		return;
	if (!kbdd->kbdd_translatable || kbdd->kbdd_translate == TR_NONE)
		kbdputcode(keycode, readq);
	else
		kbdtranslate(kbdd, keycode, readq);
}

/*
 * kbdclick is used to remember the current click value of the
 * Sun-3 keyboard.  This brain damaged keyboard will reset the
 * clicking to the "default" value after a reset command and
 * there is no way to read out the current click value.  We
 * cannot send a click command immediately after the reset
 * command or the keyboard gets screwed up.  So we wait until
 * we get the ID byte before we send back the click command.
 * Unfortunately, this means that there is a small window
 * where the keyboard can click when it really shouldn't be.
 * A value of -1 means that kbdclick has not been initialized yet.
 */
int kbdclick = -1;

/*
 * Send command byte to keyboard, if you can.
 */
static void
kbdcmd(q, cmd)
	register queue_t *q;
	char cmd;
{
	register mblk_t *bp;

	if (canput(q)) {
		if ((bp = allocb(1, BPRI_MED)) == NULL)
			printf("kbdcmd: can't allocate block for command\n");
		else {
			*bp->b_wptr++ = cmd;
			putnext(q, bp);
			if (cmd == KBD_CMD_NOCLICK)
				kbdclick = 0;
			else if (cmd == KBD_CMD_CLICK)
				kbdclick = 1;
		}
	}
}

/*
 * Update the keyboard LEDs to match the current keyboard state.
 * Do this only on Type 4 keyboards; other keyboards don't support the
 * KBD_CMD_SETLED command (nor, for that matter, the appropriate LEDs).
 */
static void
kbdsetled(kbdd)
	register struct kbddata *kbdd;
{
	if (kbdd->kbdd_state.k_id == KB_SUN4) {
		kbdcmd(kbdd->kbdd_writeq, KBD_CMD_SETLED);
		kbdcmd(kbdd->kbdd_writeq, kbdd->led_state);
	}
}

/*
 * Reset the keyboard
 */
static void
kbdreset(kbdd)
	register struct kbddata *kbdd;
{
	register struct keyboardstate *k;

	k = &kbdd->kbdd_state;
	if (kbdd->kbdd_translatable) {
		k->k_idstate = KID_NONE;
		k->k_state = NORMAL;
		kbdcmd(kbdd->kbdd_writeq, KBD_CMD_RESET);
	} else {
		bzero((caddr_t)k, sizeof (struct keyboardstate));
		k->k_id = KB_ASCII;
		k->k_idstate = KID_OK;
	}
}


/*
 * Old special codes.
 */
#define	OLD_SHIFTKEYS	0x80
#define	OLD_BUCKYBITS	0x90
#define	OLD_FUNNY	0xA0
#define	OLD_FA_UMLAUT	0xA9
#define	OLD_FA_CFLEX	0xAA
#define	OLD_FA_TILDE	0xAB
#define	OLD_FA_CEDILLA	0xAC
#define	OLD_FA_ACUTE	0xAD
#define	OLD_FA_GRAVE	0xAE
#define	OLD_ISOCHAR	0xAF
#define	OLD_STRING	0xB0
#define	OLD_LEFTFUNC	0xC0
#define	OLD_RIGHTFUNC	0xD0
#define	OLD_TOPFUNC	0xE0
#define	OLD_BOTTOMFUNC	0xF0

/*
 * Map old special codes to new ones.
 * Indexed by ((old special code) >> 4) & 0x07; add (old special code) & 0x0F.
 */
u_short	special_old_to_new[] = {
	SHIFTKEYS,
	BUCKYBITS,
	FUNNY,
	STRING,
	LEFTFUNC,
	RIGHTFUNC,
	TOPFUNC,
	BOTTOMFUNC,
};

/*
 * Set individual keystation translation from old-style entry.
 * TODO: Have each keyboard own own translation tables.
 */
static int
kbdsetkey(kbdd, key)
	register struct kbddata *kbdd;
	struct	kiockey *key;
{
	int	strtabindex, i;
	struct	keymap *km;
	register int tablemask;
	register u_short entry;

	if (key->kio_station > 127)
		return (EINVAL);
	if (kbdd->kbdd_state.k_curkeyboard == NULL)
		return (EINVAL);
	tablemask = key->kio_tablemask;
	if (tablemask == KIOCABORT1) {
		kbdd->kbdd_state.k_curkeyboard->k_abort1 = key->kio_station;
		return (0);
	}
	if (tablemask == KIOCABORT2) {
		kbdd->kbdd_state.k_curkeyboard->k_abort2 = key->kio_station;
		return (0);
	}
	if ((tablemask & ALTGRAPHMASK) ||
	    (km = settable(kbdd, (u_int)tablemask)) == NULL)
		return (EINVAL);
	if (key->kio_entry >= OLD_STRING && key->kio_entry <= OLD_STRING+15) {
		strtabindex = key->kio_entry-OLD_STRING;
		for (i = 0; i < KTAB_STRLEN; i++)
			keystringtab[strtabindex][i] = key->kio_string[i];
		keystringtab[strtabindex][KTAB_STRLEN-1] = '\0';
	}
	entry = key->kio_entry;
	/*
	 * There's nothing we need do with OLD_ISOCHAR.
	 */
	if (entry != OLD_ISOCHAR) {
		if (entry & 0x80) {
			if (entry >= OLD_FA_UMLAUT && entry <= OLD_FA_GRAVE)
				entry = FA_CLASS + (entry & 0x0F) - 9;
			else
				entry =
				    special_old_to_new[entry >> 4 & 0x07]
				    + (entry & 0x0F);
		}
	}
	km->keymap[key->kio_station] = entry;
	return (0);
}

/*
 * Map new special codes to old ones.
 * Indexed by (new special code) >> 8; add (new special code) & 0xFF.
 */
u_char	special_new_to_old[] = {
	0,			/* normal */
	OLD_SHIFTKEYS,		/* SHIFTKEYS */
	OLD_BUCKYBITS,		/* BUCKYBITS */
	OLD_FUNNY,		/* FUNNY */
	OLD_FA_UMLAUT,		/* FA_CLASS */
	OLD_STRING,		/* STRING */
	OLD_LEFTFUNC,		/* FUNCKEYS */
};

/*
 * Get individual keystation translation as old-style entry.
 */
static int
kbdgetkey(kbdd, key)
	register struct kbddata *kbdd;
	struct	kiockey *key;
{
	int	strtabindex, i;
	struct	keymap *km;
	register u_short entry;

	if (key->kio_station > 127)
		return (EINVAL);
	if (kbdd->kbdd_state.k_curkeyboard == NULL)
		return (EINVAL);
	if (key->kio_tablemask == KIOCABORT1) {
		key->kio_station = kbdd->kbdd_state.k_curkeyboard->k_abort1;
		return (0);
	}
	if (key->kio_tablemask == KIOCABORT2) {
		key->kio_station = kbdd->kbdd_state.k_curkeyboard->k_abort2;
		return (0);
	}
	if ((km = settable(kbdd, (u_int)key->kio_tablemask)) == NULL)
		return (EINVAL);
	entry = km->keymap[key->kio_station];
	if (entry & 0xFF00)
		key->kio_entry =
		    special_new_to_old[(entry & 0xFF00) >> 8]
		    + (entry & 0x00FF);
	else if (entry & 0x80)
		key->kio_entry = OLD_ISOCHAR;	/* you lose */
	else
		key->kio_entry = entry;

	if (entry >= STRING && entry <= STRING+15) {
		strtabindex = entry-STRING;
		for (i = 0; i < KTAB_STRLEN; i++)
			key->kio_string[i] = keystringtab[strtabindex][i];
	}
	return (0);
}

/*
 * Set individual keystation translation from new-style entry.
 * TODO: Have each keyboard own own translation tables.
 */
static int
kbdskey(kbdd, key)
	register struct kbddata *kbdd;
	struct	kiockeymap *key;
{
	int	strtabindex, i;
	struct	keymap *km;

	if (key->kio_station > 127)
		return (EINVAL);
	if (kbdd->kbdd_state.k_curkeyboard == NULL)
		return (EINVAL);
	if (key->kio_tablemask == KIOCABORT1) {
		kbdd->kbdd_state.k_curkeyboard->k_abort1 = key->kio_station;
		return (0);
	}
	if (key->kio_tablemask == KIOCABORT2) {
		kbdd->kbdd_state.k_curkeyboard->k_abort2 = key->kio_station;
		return (0);
	}
	if ((km = settable(kbdd, (u_int)key->kio_tablemask)) == NULL)
		return (EINVAL);
	if (key->kio_entry >= STRING && key->kio_entry <= STRING+15) {
		strtabindex = key->kio_entry-STRING;
		for (i = 0; i < KTAB_STRLEN; i++)
			keystringtab[strtabindex][i] = key->kio_string[i];
		keystringtab[strtabindex][KTAB_STRLEN-1] = '\0';
	}
	km->keymap[key->kio_station] = key->kio_entry;
	return (0);
}

/*
 * Get individual keystation translation as new-style entry.
 */
static int
kbdgkey(kbdd, key)
	register struct kbddata *kbdd;
	struct	kiockeymap *key;
{
	int	strtabindex, i;
	struct	keymap *km;

	if (key->kio_station > 127)
		return (EINVAL);
	if (kbdd->kbdd_state.k_curkeyboard == NULL)
		return (EINVAL);
	if (key->kio_tablemask == KIOCABORT1) {
		key->kio_station = kbdd->kbdd_state.k_curkeyboard->k_abort1;
		return (0);
	}
	if (key->kio_tablemask == KIOCABORT2) {
		key->kio_station = kbdd->kbdd_state.k_curkeyboard->k_abort2;
		return (0);
	}
	if ((km = settable(kbdd, (u_int)key->kio_tablemask)) == NULL)
		return (EINVAL);
	key->kio_entry = km->keymap[key->kio_station];
	if (key->kio_entry >= STRING && key->kio_entry <= STRING+15) {
		strtabindex = key->kio_entry-STRING;
		for (i = 0; i < KTAB_STRLEN; i++)
			key->kio_string[i] = keystringtab[strtabindex][i];
	}
	return (0);
}

static int
kbdidletimeout(kbdd)
	register struct kbddata *kbdd;
{
	register struct keyboardstate *k;

	untimeout(kbdidletimeout, (caddr_t)kbdd);

	/*
	 * Double check that was waiting for idle timeout.
	 */
	k = &kbdd->kbdd_state;
	if (k->k_idstate == KID_IDLE)
		kbdinput(kbdd, IDLEKEY);
}

static int
kbdlayouttimeout(kbdd)
	register struct kbddata *kbdd;
{
	register mblk_t *mp;
	int s;

	untimeout(kbdlayouttimeout, (caddr_t)kbdd);

	s = splstr();
	/*
	 * Timed out waiting for reply to "get keyboard layout" command.
	 * Return an ETIME error.
	 */
	if ((mp = kbdd->kbdd_replypending) != NULL) {
		kbdd->kbdd_replypending = NULL;
		(void) splx(s);
		mp->b_datap->db_type = M_IOCNAK;
		((struct iocblk *)mp->b_rptr)->ioc_error = ETIME;
		putnext(kbdd->kbdd_readq, mp);
	} else
		(void) splx(s);
}

/*
 * Put procedure for input from driver end of stream (read queue).
 */
static int
kbdrput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	struct kbddata *kbdd = (struct kbddata *)q->q_ptr;
	register mblk_t *bp;
	register u_char *readp;
	struct iocblk *iocp;

	if (kbdd == 0) {
		freemsg(mp);	/* nobody's listening */
		return;
	}

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(WR(q), FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);

	default:
		putnext(q, mp);
		return;

	case M_BREAK:
		kbdreset(kbdd);
		freemsg(mp);
		return;

	case M_IOCACK:
	case M_IOCNAK:
		/*
		 * If we are doing an "ioctl" ourselves, check if this
		 * is the reply to that code.  If so, wake up the
		 * "open" routine, and toss the reply, otherwise just
		 * pass it up.
		 */
		iocp = (struct iocblk *)mp->b_rptr;
		if (!(kbdd->kbdd_flags & KBD_IOCWAIT) ||
		    iocp->ioc_id != kbdd->kbdd_iocid) {
			/*
			 * This isn't the reply we're looking for.  Move along.
			 */
			if (kbdd->kbdd_flags & KBD_OPEN)
				putnext(q, mp);
			else
				freemsg(mp);	/* not ready to listen */
		} else {
			kbdd->kbdd_flags &= ~KBD_IOCWAIT;
			kbdd->kbdd_iocerror = iocp->ioc_error;
			wakeup((caddr_t)&kbdd->kbdd_iocerror);
			freemsg(mp);
		}
		return;

	case M_DATA:
		if (!(kbdd->kbdd_flags & KBD_OPEN)) {
			freemsg(mp);	/* not read to listen */
			return;
		}
		break;
	}

	/*
	 * A data message, consisting of bytes from the keyboard.
	 * Ram them through our state machine.
	 */
	bp = mp;

	do {
		readp = bp->b_rptr;
		while (readp < bp->b_wptr)
			kbdinput(kbdd, *readp++);
		bp->b_rptr = readp;
	} while ((bp = bp->b_cont) != NULL);	/* next block, if any */

	freemsg(mp);
}

/*
 * A keypress was received (from a parallel or serial keyboard).
 * Process it through the state machine to check for aborts.
 */
static void
kbdinput(kbdd, key)
	register struct kbddata *kbdd;
	register u_char key;
{
	register struct keyboardstate *k;
	register mblk_t *mp;
	int s;

	k = &kbdd->kbdd_state;
#ifdef	KBD_DEBUG
if (kbd_input_debug) printf("kbdinput key %X\n", key);
#endif
	switch (k->k_idstate) {

	case KID_NONE:
		if (key == IDLEKEY) {
			k->k_idstate = KID_IDLE;
			timeout(kbdidletimeout, (caddr_t)kbdd, hz/10);
		} else if (key == RESETKEY)
			k->k_idstate = KID_LKSUN2;
		return;

	case KID_IDLE:
		if (key == IDLEKEY)
			kbdid(kbdd, KB_KLUNK);
		else if (key == RESETKEY)
			k->k_idstate = KID_LKSUN2;
		else if (key & 0x80)
			kbdid(kbdd, (int)(KB_VT100 | (key&0x40)));
		else
			kbdreset(kbdd);
		return;

	case KID_LKSUN2:
		if (key == KB_SUN2) {	    /* Sun-2 keyboard */
			kbdid(kbdd, KB_SUN2);
			return;
		}
		if (key == KB_SUN3 || key == KB_SUN4 || key == KB_VT220 ||
			key == KB_VT220I) {
			/*
			 * Type 3 or 4 keyboard or vt220 emulation
			 */
			kbdid(kbdd, (int)key);
			/*
			 * We just did a reset command to a Type 3 or Type 4
			 * keyboard which sets the click back to the default
			 * (which is currently ON!).  We use the kbdclick
			 * variable to see if the keyboard should be turned on
			 * or off.  If it has not been set, then on a non-sun2
			 * we use the eeprom to determine if the default value
			 * is on or off.  In the sun2 case, we default to off.
			 */
			switch (kbdclick) {
			case 0:
				kbdcmd(kbdd->kbdd_writeq, KBD_CMD_NOCLICK);
				break;
			case 1:
				kbdcmd(kbdd->kbdd_writeq, KBD_CMD_CLICK);
				break;
			case -1:
			default:
				{
#if !defined(sun2) && !defined (AT386)
#ifndef OPENPROMS
#include <machine/eeprom.h>

				if (EEPROM->ee_diag.eed_keyclick ==
				    EED_KEYCLICK)
#else OPENPROMS
				extern int keyclick;

				if (keyclick)
#endif OPENPROMS
					kbdcmd(kbdd->kbdd_writeq,
					    KBD_CMD_CLICK);
				else
#endif !sun2 && !AT386
					kbdcmd(kbdd->kbdd_writeq,
					    KBD_CMD_NOCLICK);
				}
				break;
			}
			/*
			 * A keyboard reset clears the LEDs.
			 * Restore the LEDs from the last value we set
			 * them to.
			 */
			kbdsetled(kbdd);
			return;
		}
		kbdreset(kbdd);
		return;

	case KID_OK:
		if ((key == 0 && k->k_state != LAYOUT) || key == RESETKEY) {
			kbdreset(kbdd);
			return;
		}
		break;
	}

	switch (k->k_state) {

	normalstate:
		k->k_state = NORMAL;
	case NORMAL:
		if (k->k_curkeyboard) {
			if (key == k->k_curkeyboard->k_abort1) {
				k->k_state = ABORT1;
				break;
			}
		}
		if (key == LAYOUTKEY)
			k->k_state = LAYOUT;
		else {
			kbduse(kbdd, key);
			if (key == IDLEKEY)
				k->k_state = IDLE1;
		}
		break;

	case IDLE1:
		if (key & 0x80)	{	/* ID byte or 'up' event */
			if (k->k_id == KB_VT100) {
				k->k_state = IDLE2;
				break;
			} else if (key == LAYOUTKEY) {
				k->k_state = LAYOUT;
				break;
			}
			/* Must be an 'up' event. Fall through */
		}
		if (key != IDLEKEY)
			goto normalstate;	/* real data */
		break;

	case IDLE2:
		if (key == IDLEKEY)
			k->k_state = IDLE1;
		else
			goto normalstate;
		break;

	case LAYOUT:
		s = splstr();
		untimeout(kbdlayouttimeout, (caddr_t)kbdd);
		if ((mp = kbdd->kbdd_replypending) != NULL) {
			kbdd->kbdd_replypending = NULL;
			(void) splx(s);
			*(int *)mp->b_cont->b_wptr = key;
			mp->b_cont->b_wptr += sizeof (int);
			putnext(kbdd->kbdd_readq, mp);
		} else
			(void) splx(s);
		k->k_state = NORMAL;
		break;

	case ABORT1:
		if (k->k_curkeyboard) {
			if (key == k->k_curkeyboard->k_abort2) {
				DELAY(100000);
				if (boothowto & RB_DEBUG) {
					s = splzs(); /* prevent re-entrance */
					(void) setjmp(&u.u_pcb.pcb_regs);
					CALL_DEBUG();
					(void)splx(s);
				} else {
#ifndef AT386
					prom_enter_mon();
#endif
				}
				k->k_state = NORMAL;
				kbduse(kbdd, (u_char)IDLEKEY);	/* fake */
				return;
			} else {
				kbduse(kbdd, k->k_curkeyboard->k_abort1);
				goto normalstate;
			}
		}
		break;

	case COMPOSE1:
	case COMPOSE2:
	case FLTACCENT:
		if (key != IDLEKEY)
			kbduse(kbdd, key);
		break;
	}
}

static void
kbdid(kbdd, id)
	register struct kbddata *kbdd;
	int	id;
{
	register struct keyboardstate *k;

	k = &kbdd->kbdd_state;

#ifdef  KBD_DEBUG
	if (kbd_debug)
		printf("kbdid: id %x \n", id);
#endif
	if (id == KB_VT220 || id == KB_VT220I) {
		/* Allow loadable keytables a la type4 */
		k->k_curkeyboard = keytables[KB_SUN4];
		/* Don't mask the top 4 bits of the byte */
		k->k_id = id;
	} else {
		k->k_id = id & 0xF;
		k->k_curkeyboard = keytables[k->k_id];
	}
	k->k_idstate = KID_OK;
	k->k_shiftmask = 0;
	if (id & 0x40)
		/* Not a transition so don't send event */
		k->k_shiftmask |= CAPSMASK;
	k->k_buckybits = 0;
	k->k_rptkey = IDLEKEY;  /* Nothing happening now */
}

/*
 * This routine determines which table we should look in to decode
 * the current keycode.
 */
static struct keymap *
settable(kbdd, mask)
	register struct kbddata *kbdd;
	register u_int mask;
{
	register struct keyboard *kp;

	kp = kbdd->kbdd_state.k_curkeyboard;
	if (kp == NULL)
		return (NULL);
	if (mask & UPMASK)
		return (kp->k_up);
	if (mask & NUMLOCKMASK)
		return (kp->k_numlock);
	if (mask & CTRLMASK)
		return (kp->k_control);
	if (mask & ALTGRAPHMASK)
		return (kp->k_altgraph);
	if (mask & SHIFTMASK)
		return (kp->k_shifted);
	if (mask & CAPSMASK)
		return (kp->k_caps);
	return (kp->k_normal);
}

static int
kbdrpt(kbdd)
	register struct kbddata *kbdd;
{
	register struct keyboardstate *k;
	int	pri;

	k = &kbdd->kbdd_state;
#ifdef	KBD_DEBUG
if (kbd_rpt_debug) printf("kbdrpt key %X\n", k->k_rptkey);
#endif
	/*
	 * Since timeout is at low priority (interruptable),
	 * protect code with spl's.
	 */
	pri = spl5();
	kbdkeyreleased(kbdd, KEYOF(k->k_rptkey));
	kbduse(kbdd, k->k_rptkey);
	if (k->k_rptkey != IDLEKEY)
		timeout(kbdrpt, (caddr_t)kbdd, kbd_repeatrate);
	(void) splx(pri);
}

static void
kbdcancelrpt(kbdd)
	register struct kbddata *kbdd;
{
	register struct keyboardstate *k;

	k = &kbdd->kbdd_state;
	if (k->k_rptkey != IDLEKEY) {
		untimeout(kbdrpt, (caddr_t)kbdd); /* Ignored if not found */
		k->k_rptkey = IDLEKEY;
	}
}

static void
kbdtranslate(kbdd, keycode, q)
	register struct kbddata *kbdd;
	register u_char keycode;
	queue_t *q;
{
	register u_char key, newstate;
	register u_short entry, entrytype;
	register char *cp;
	register struct keyboardstate *k;
	struct keymap *km;
	Firm_event fe;
	int i;
	char buf[10];

	k = &kbdd->kbdd_state;
	newstate = STATEOF(keycode);
	key = KEYOF(keycode);

#ifdef	KBD_DEBUG
	if (kbd_input_debug) printf("KBD TRANSLATE key=%d\n", keycode);
#endif

	if (kbdd->kbdd_translate == TR_UNTRANS_EVENT) {
		if (newstate == PRESSED) {
			bzero((caddr_t) &fe, sizeof fe);
			fe.id = key;
			fe.value = 1;
			kbdqueuepress(kbdd, key, &fe);
		} else {
			kbdkeyreleased(kbdd, key);
		}
		return;
	}
	km = settable(kbdd, (u_int)(k->k_shiftmask | newstate));
	if (km == NULL) {		/* gross error */
		kbdcancelrpt(kbdd);
		return;
	}
	entry = km->keymap[key];
	if (entry == NONL) {
		/*
		 * NONL appears only in the Num Lock table, and indicates that
		 * this key is not affected by Num Lock.  This means we should
		 * ask for the table we would have gotten had Num Lock not been
		 * down, and translate using that table.
		 */
		km = settable(kbdd,
		    (u_int)((k->k_shiftmask | newstate) & ~NUMLOCKMASK));
		if (km == NULL) {		/* gross error */
			kbdcancelrpt(kbdd);
			return;
		}
		entry = km->keymap[key];
	}
	entrytype = (entry & 0xFF00) >> 8;

	if (entrytype == (SHIFTKEYS >> 8)) {
		/*
		 * Handle the state of toggle shifts specially.
		 * Ups should be ignored, and downs should be mapped to ups if
		 * that shift is currently on.
		 */
		if ((1 << (entry & 0x0F)) & k->k_curkeyboard->k_toggleshifts) {
			if ((1 << (entry & 0x0F)) & k->k_togglemask) {
				newstate = RELEASED;	/* toggling off */
			} else {
				newstate = PRESSED;	/* toggling on */
			}
		}
	} else {
		/*
		 * Handle Compose and floating accent key sequences
		 */
		if (k->k_state == COMPOSE1) {
			if (newstate == RELEASED)
				return;
			for (i = 0; kb_compose_table[i].first != entry &&
				kb_compose_table[i].second != entry; i++) {
				if (kb_compose_table[i].first == 0) {
					/*
					 * Invalid first key; ignore key and
					 * return to normal state.
					 */
					k->k_state = NORMAL;
					kbdd->led_state &= ~LED_COMPOSE;
					kbdsetled(kbdd);
					return;
				}
			}
			kbdd->compose_key = entry;
			k->k_state = COMPOSE2;
			return;
		} else if (k->k_state == COMPOSE2) {
			if (newstate == RELEASED)
				return;
			k->k_state = NORMAL;	/* next state is "normal" */
			kbdd->led_state &= ~LED_COMPOSE;
			kbdsetled(kbdd);
			for (i = 0;
			    (kb_compose_table[i].first != kbdd->compose_key) ||
			    (kb_compose_table[i].second != entry); i++) {
				if (kb_compose_table[i].second == 0)
					return;
				/* Try the reverse composition */
				if ((kb_compose_table[i].second ==
				    kbdd->compose_key) &&
				    (kb_compose_table[i].first == entry))
					break;
			}
			if (kbdd->kbdd_translate == TR_EVENT) {
				fe.id = (kbdd->kbdd_compat ?
				    ISO_FIRST : EUC_FIRST)
				    + kb_compose_table[i].iso;
				fe.value = 1;
				kbdkeypressed(kbdd, key, &fe, fe.id);
			} else if (kbdd->kbdd_translate == TR_ASCII)
				kbdputcode(kb_compose_table[i].iso, q);
			return;
		} else if (k->k_state == FLTACCENT) {
			if (newstate == RELEASED)
				return;
			k->k_state = NORMAL;	/* next state is "normal" */
			for (i = 0;
			    (kb_fltaccent_table[i].fa_entry
				!= kbdd->fltaccent_entry) ||
			    (kb_fltaccent_table[i].ascii != entry);
			    i++) {
				if (kb_fltaccent_table[i].fa_entry == 0)
					/* Invalid second key: ignore key */
					return;
			}
			if (kbdd->kbdd_translate == TR_EVENT) {
				fe.id = (kbdd->kbdd_compat ?
				    ISO_FIRST : EUC_FIRST)
				    + kb_fltaccent_table[i].iso;
				fe.value = 1;
				kbdkeypressed(kbdd, key, &fe, fe.id);
			} else if (kbdd->kbdd_translate == TR_ASCII)
				kbdputcode(kb_fltaccent_table[i].iso, q);
			return;
		}
	}

	/*
	 * If the key is going down, and it's not one of the keys that doesn't
	 * auto-repeat, set up the auto-repeat timeout.
	 *
	 * The keys that don't auto-repeat are NOSCROLL, the Compose key,
	 * the shift keys, the "bucky bit" keys, the "floating accent" keys,
	 * and the function keys when in TR_EVENT mode.
	 * We might need to add the RESET/IDLE/ERROR entries here.  FIXME?
	 */
	if (newstate == PRESSED && entry != NOSCROLL && entry != COMPOSE &&
	    entrytype != (SHIFTKEYS >> 8) && entrytype != (BUCKYBITS >> 8) &&
	    entrytype != (FA_CLASS >> 8) &&
	    !((entrytype == (FUNCKEYS >> 8) || entrytype == (PADKEYS >> 8)) &&
	    kbdd->kbdd_translate == TR_EVENT)) {
		if (k->k_rptkey != keycode) {
			kbdcancelrpt(kbdd);
			timeout(kbdrpt, (caddr_t)kbdd, kbd_repeatdelay);
			k->k_rptkey = keycode;
		}
	} else if (key == KEYOF(k->k_rptkey))		/* key going up */
		kbdcancelrpt(kbdd);
	if ((newstate == RELEASED) && (kbdd->kbdd_translate == TR_EVENT))
		kbdkeyreleased(kbdd, key);

	/*
	 * We assume here that keys other than shift keys and bucky keys have
	 * entries in the "up" table that cause nothing to be done, and thus we
	 * don't have to check for newstate == RELEASED.
	 */
	switch (entrytype) {

	case 0x0:		/* regular key */
		switch (kbdd->kbdd_translate) {

		case TR_EVENT:
			fe.id = entry | k->k_buckybits;
			fe.value = 1;
			kbdkeypressed(kbdd, key, &fe, entry);
			break;

		case TR_ASCII:
/*
 * ugly hack: if capslock is on and shift is down, send up
 * lowercase alfa characters not uppercase.
 */
	if ((k->k_shiftmask & CAPSMASK) &&
	    (k->k_shiftmask & SHIFTMASK) &&
	    (entry >= 'A') && (entry <= 'Z'))
		entry ^= 'a' ^ 'A';

			kbdputcode(entry | k->k_buckybits, q);
			break;
		}
		break;

	case SHIFTKEYS >> 8: {
		u_int shiftbit = 1 << (entry & 0x0F);

		/* Modify toggle state (see toggle processing above) */
		if (shiftbit & k->k_curkeyboard->k_toggleshifts) {
			if (newstate == RELEASED) {
				if (shiftbit == CAPSMASK) {
					kbdd->led_state &= ~LED_CAPS_LOCK;
					kbdsetled(kbdd);
				} else if (shiftbit == NUMLOCKMASK) {
					kbdd->led_state &= ~LED_NUM_LOCK;
					kbdsetled(kbdd);
				}
				k->k_togglemask &= ~shiftbit;
				k->k_shiftmask &= ~shiftbit;
			} else {
				if (shiftbit == CAPSMASK) {
					kbdd->led_state |= LED_CAPS_LOCK;
					kbdsetled(kbdd);
				} else if (shiftbit == NUMLOCKMASK) {
					kbdd->led_state |= LED_NUM_LOCK;
					kbdsetled(kbdd);
				}
				k->k_togglemask |= shiftbit;
				k->k_shiftmask |= shiftbit;
			}
		} else
			k->k_shiftmask ^= shiftbit;
		if (kbdd->kbdd_translate == TR_EVENT && newstate == PRESSED){
			/*
			 * Relying on ordinal correspondence between
			 * vuid_event.h SHIFT_CAPSLOCK-SHIFT_RIGHTCTRL &
			 * kbd.h CAPSLOCK-RIGHTCTRL in order to
			 * correctly translate entry into fe.id.
			 */
			fe.id = SHIFT_CAPSLOCK + (entry & 0x0F);
			fe.value = 1;
			kbdkeypressed(kbdd, key, &fe, fe.id);
		}
		break;
		}

	case BUCKYBITS >> 8:
		k->k_buckybits ^= 1 << (7 + (entry & 0x0F));
		if (kbdd->kbdd_translate == TR_EVENT && newstate == PRESSED){
			/*
			 * Relying on ordinal correspondence between
			 * vuid_event.h SHIFT_META-SHIFT_TOP &
			 * kbd.h METABIT-SYSTEMBIT in order to
			 * correctly translate entry into fe.id.
			 */
			fe.id = SHIFT_META + (entry & 0x0F);
			fe.value = 1;
			kbdkeypressed(kbdd, key, &fe, fe.id);
		}
		break;

	case FUNNY >> 8:
		switch (entry) {
		case NOP:
			break;

		/*
		 * NOSCROLL/CTRLS/CTRLQ exist so that these keys, on keyboards
		 * with NOSCROLL, interact smoothly.  If a user changes
		 * his tty output control keys to be something other than those
		 * in keytables for CTRLS & CTRLQ then he effectively disables
		 * his NOSCROLL key.  However, 1) this is also what happens
		 * on a VT100, 2) users should't change their stop and start
		 * characters anyway, and 3) there's nothing we can do about
		 * it.
		 */
		case NOSCROLL:
			if (k->k_shiftmask & CTLSMASK)
				goto sendcq;
			else
				goto sendcs;

		case CTRLS:
		sendcs:
			/* A CTLSMASK change is not a vuid transition */
			k->k_shiftmask |= CTLSMASK;
			switch (kbdd->kbdd_translate) {

			case TR_EVENT:
				fe.id = ('S'-0x40) | k->k_buckybits;
				fe.value = 1;
				kbdkeypressed(kbdd, key, &fe,
				    (u_short)('S'-0x40));
				break;

			case TR_ASCII:
				kbdputcode(('S'-0x40) | k->k_buckybits, q);
				break;
			}
			break;

		case CTRLQ:
		sendcq:
			/* A CTLSMASK change is not a vuid transition */
			switch (kbdd->kbdd_translate) {

			case TR_EVENT:
				fe.id = ('Q'-0x40) | k->k_buckybits;
				fe.value = 1;
				kbdkeypressed(kbdd, key, &fe,
				    (u_short)('Q'-0x40));
				break;

			case TR_ASCII:
				kbdputcode(('Q'-0x40) | k->k_buckybits, q);
				break;
			}
			k->k_shiftmask &= ~CTLSMASK;
			break;

		case IDLE:
			/*
			 * Minor hack to prevent keyboards unplugged
			 * in caps lock from retaining their capslock
			 * state when replugged.  This should be
			 * solved by using the capslock info in the
			 * KBDID byte.
			 */
			if (keycode == NOTPRESENT)
				k->k_shiftmask = 0;
			/* Fall thru into RESET code */

		case RESET:
		gotreset:
			k->k_shiftmask &= k->k_curkeyboard->k_idleshifts;
			k->k_shiftmask |= k->k_togglemask;
			k->k_buckybits &= k->k_curkeyboard->k_idlebuckys;
			kbdcancelrpt(kbdd);
			kbdreleaseall(kbdd);
			break;

		case ERROR:
			printf("kbd: Error detected\n");
			goto gotreset;

		case COMPOSE:
			k->k_state = COMPOSE1;
			kbdd->led_state |= LED_COMPOSE;
			kbdsetled(kbdd);
			break;
		/*
		 * Remember when adding new entries that,
		 * if they should NOT auto-repeat,
		 * they should be put into the IF statement
		 * just above this switch block.
		 */
		default:
			goto badentry;
		}
		break;

	case FA_CLASS >> 8:
		if (k->k_state == NORMAL) {
			kbdd->fltaccent_entry = entry;
			k->k_state = FLTACCENT;
		}
		return;

	case STRING >> 8:
		cp = &keystringtab[entry & 0x0F][0];
		while (*cp != '\0') {
			switch (kbdd->kbdd_translate) {

			case TR_EVENT:
				kbd_send_esc_event(*cp, kbdd);
				break;

			case TR_ASCII:
				kbdputcode((u_char)*cp, q);
				break;
			}
			cp++;
		}
		break;

	case FUNCKEYS >> 8:
		switch (kbdd->kbdd_translate) {

		case TR_ASCII:
			cp = strsetwithdecimal(&buf[0],
			    (u_int) ((entry & 0x003F) + 192),
			    sizeof (buf) - 1);
			kbdputcode('\033', q); /* Escape */
			kbdputcode('[', q);
			while (*cp != '\0') {
				kbdputcode((u_char)*cp, q);
				cp++;
			}
			kbdputcode('z', q);
			break;

		case TR_EVENT:
			/*
			 * Take advantage of the similar
			 * ordering of kbd.h function keys and
			 * vuid_event.h function keys to do a
			 * simple translation to achieve a
			 * mapping between the 2 different
			 * address spaces.
			 */
			fe.id = (entry & 0x003F) + KEY_LEFTFIRST;
			fe.value = 1;
			/*
			 * Assume "up" table only generates
			 * shift changes.
			 */
			kbdkeypressed(kbdd, key, &fe, fe.id);
			/*
			 * Function key events can be expanded
			 * by terminal emulator software to
			 * produce the standard escape sequence
			 * generated by the TR_ASCII case above
			 * if a function key event is not used
			 * by terminal emulator software
			 * directly.
			 */
			break;
		}
		break;

	/*
	 * Remember when adding new entries that,
	 * if they should NOT auto-repeat,
	 * they should be put into the IF statement
	 * just above this switch block.
	 */
	case PADKEYS >> 8:
		switch (kbdd->kbdd_translate) {

		case TR_ASCII:
			kbdputcode(kb_numlock_table[entry&0x1F], q);
			break;

		case TR_EVENT:
			/*
			 * Take advantage of the similar
			 * ordering of kbd.h keypad keys and
			 * vuid_event.h keypad keys to do a
			 * simple translation to achieve a
			 * mapping between the 2 different
			 * address spaces.
			 */
			fe.id = (entry & 0x001F) + VKEY_FIRSTPAD;
			fe.value = 1;
			/*
			 * Assume "up" table only generates
			 * shift changes.
			 */
			kbdkeypressed(kbdd, key, &fe, fe.id);
			/*
			 * Keypad key events can be expanded
			 * by terminal emulator software to
			 * produce the standard ascii character
			 * generated by the TR_ASCII case above
			 * if a keypad key event is not used
			 * by terminal emulator software
			 * directly.
			 */
			break;
		}

	badentry:
		break;
	}
}

static void
kbd_send_esc_event(c, kbdd)
	char c;
	struct kbddata *kbdd;
{
	Firm_event fe;

	fe.id = c;
	fe.value = 1;
	fe.pair_type = FE_PAIR_NONE;
	fe.pair = 0;
	/*
	 * Pretend as if each cp pushed and released
	 * Calling kbdqueueevent avoids addr translation
	 * and pair base determination of kbdkeypressed.
	 */
	kbdqueueevent(kbdd, &fe);
	fe.value = 0;
	kbdqueueevent(kbdd, &fe);
}

char *
strsetwithdecimal(buf, val, maxdigs)
	char	*buf;
	u_int	val, maxdigs;
{
	int	hradix = 5;
	char	*bp;
	int	lowbit;
	char	*tab = "0123456789abcdef";

	bp = buf + maxdigs;
	*(--bp) = '\0';
	while (val) {
		lowbit = val & 1;
		val = (val >> 1);
		*(--bp) = tab[val % hradix * 2 + lowbit];
		val /= hradix;
	}
	return (bp);
}

static void
kbdkeypressed(kbdd, key_station, fe, base)
	register struct kbddata *kbdd;
	u_char	key_station;
	Firm_event *fe;
	register u_short base;
{
	register struct keyboardstate *k;
	register short id_addr;

	/* Set pair values */
	if (fe->id < VKEY_FIRST) {
		/*
		 * If CTRLed, find the ID that would have been used had it
		 * not been CTRLed.
		 */
		k = &kbdd->kbdd_state;
		if (k->k_shiftmask & (CTRLMASK | CTLSMASK)) {
			struct keymap *km;

			km = settable(kbdd,
			    k->k_shiftmask & ~(CTRLMASK | CTLSMASK | UPMASK));
			if (km == NULL)
				return;
			base = km->keymap[key_station];
		}
		if (base != fe->id) {
			fe->pair_type = FE_PAIR_SET;
			fe->pair = base;
			goto send;
		}
	}
	fe->pair_type = FE_PAIR_NONE;
	fe->pair = 0;
send:
	/* Adjust event id address for multiple keyboard/workstation support */
	switch (vuid_id_addr(fe->id)) {
	case ASCII_FIRST:
		id_addr = kbdd->kbdd_ascii_addr;
		break;
	case TOP_FIRST:
		id_addr = kbdd->kbdd_top_addr;
		break;
	case VKEY_FIRST:
		id_addr = kbdd->kbdd_vkey_addr;
		break;
	default:
		id_addr = vuid_id_addr(fe->id);
	}
	fe->id = vuid_id_offset(fe->id) | id_addr;
	kbdqueuepress(kbdd, key_station, fe);
}

static void
kbdqueuepress(kbdd, key_station, fe)
	register struct kbddata *kbdd;
	u_char key_station;
	Firm_event *fe;
{
	register struct key_event *ke;
	register int i;

	if (key_station == IDLEKEY)
		return;
#ifdef	KBD_DEBUG
	if (kbd_input_debug) printf("KBD PRESSED key=%d\n", key_station);
#endif
	/* Scan table of down key stations */
	if (kbdd->kbdd_translate == TR_EVENT ||
	    kbdd->kbdd_translate == TR_UNTRANS_EVENT) {
		for (i = 0, ke = kbdd->kbdd_downs;
		    i < kbdd->kbdd_downs_entries;
		    i++, ke++) {
			/* Keycode already down? */
			if (ke->key_station == key_station) {
#ifdef	KBD_DEBUG
	printf("kbd: Double entry in downs table (%d, %d)!\n", key_station, i);
#endif	KBD_DEBUG
				goto add_event;
			}
			if (ke->key_station == 0)
				goto add_event;
		}
		printf("kbd: Too many keys down!\n");
		ke = kbdd->kbdd_downs;
	}
add_event:
	ke->key_station = key_station;
	ke->event = *fe;
	kbdqueueevent(kbdd, fe);
}

static void
kbdkeyreleased(kbdd, key_station)
	register struct kbddata *kbdd;
	u_char key_station;
{
	register struct key_event *ke;
	register int i;

	if (key_station == IDLEKEY)
		return;
#ifdef	KBD_DEBUG
	if (kbd_input_debug)
		printf("KBD RELEASE key=%d\n", key_station);
#endif
	if (kbdd->kbdd_translate != TR_EVENT &&
	    kbdd->kbdd_translate != TR_UNTRANS_EVENT)
		return;
	/* Scan table of down key stations */
	for (i = 0, ke = kbdd->kbdd_downs;
	    i < kbdd->kbdd_downs_entries;
	    i++, ke++) {
		/* Found? */
		if (ke->key_station == key_station) {
			ke->key_station = 0;
			ke->event.value = 0;
			kbdqueueevent(kbdd, &ke->event);
		}
	}

	/*
	 * Ignore if couldn't find because may be called twice
	 * for the same key station in the case of the kbdrpt
	 * routine being called unnecessarily.
	 */
	return;
}

static void
kbdreleaseall(kbdd)
	struct kbddata *kbdd;
{
	register struct key_event *ke;
	register int i;

#ifdef	KBD_DEBUG
	if (kbd_debug && kbd_ra_debug) printf("KBD RELEASE ALL\n");
#endif
	/* Scan table of down key stations */
	for (i = 0, ke = kbdd->kbdd_downs;
	    i < kbdd->kbdd_downs_entries; i++, ke++) {
		/* Key station not zero */
		if (ke->key_station)
			kbdkeyreleased(kbdd, ke->key_station);
			/* kbdkeyreleased resets kbdd_downs entry */
	}
}

/*
 * Pass a keycode up the stream, if you can, otherwise throw it away.
 */
static void
kbdputcode(code, q)
	u_int code;
	register queue_t *q;
{
	register mblk_t *bp;

	if (!canput(q))
		printf("kbdputcode: can't put block for keycode\n");
	else {
		if ((bp = allocb(sizeof (u_int), BPRI_HI)) == NULL)
		    printf("kbdputcode: can't allocate block for keycode\n");
		else {
			*bp->b_wptr++ = code;
			putnext(q, bp);
		}
	}
}

/*
 * Pass a VUID "firm event" up the stream, if you can.
 */
static void
kbdqueueevent(kbdd, fe)
	register struct kbddata *kbdd;
	Firm_event *fe;
{
	register queue_t *q;
	register mblk_t *bp;

	if ((q = kbdd->kbdd_readq) == NULL)
		return;
	if (!canput(q)) {
		if (kbd_overflow_msg)
			printf("kbd: Buffer flushed when overflowed.\n");
		kbdflush(kbdd);
		kbd_overflow_cnt++;
	} else {
		if ((bp = allocb(sizeof (Firm_event), BPRI_HI)) == NULL)
		    printf("kbdqueueevent: can't allocate block for event\n");
		else {
			uniqtime(&fe->time);
			*(Firm_event *)bp->b_wptr = *fe;
			bp->b_wptr += sizeof (Firm_event);
			putnext(q, bp);
		}
	}
}
