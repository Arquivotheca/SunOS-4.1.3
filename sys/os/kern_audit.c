/*	@(#)kern_audit.c 1.1 92/07/30 SMI	*/
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/pathname.h>
#include <sys/acct.h>
#include <sys/varargs.h>

#define	AU_NBUFS	4
#define	AU_NLBUFS	4
#define	AU_BFSZ		1024
#define	AU_HDSZ		(sizeof (au_buf_t))
#define	CKSTATE(state)	(state == au_auditstate)
/*
 * Check for free space on audit filesystem no more than this often.
 */
#define	AU_INTERVAL	120
/*
 * Opcodes for auditsys system call
 */
#define	AUC_AUDITSVC		1
#define	AUC_AUDIT		2
#define	AUC_GETAUID		3
#define	AUC_SETAUID		4
#define	AUC_SETUSERAUDIT	5
#define	AUC_SETAUDIT		6
#define	AUC_AUDITON		7

static int au_auditstate;
static int au_prevstate;

struct au_buf_s {
	struct au_buf_s	*ab_next;
	u_int		ab_size;
	/* An audit record structure follows immdiately after this point */
};
typedef struct au_buf_s au_buf_t;

struct au_fq_s {
	au_buf_t	*af_free;
	int		af_wait;
};
typedef struct au_fq_s	au_fq_t;

struct {
	au_buf_t	*ai_head;
	au_buf_t	*ai_tail;
	au_fq_t		ai_free;
	short		ai_busy;
	short		ai_nlbufs;
	short		ai_wlbufs;
} au_iocb;

/*
 * Central dispatching for all audit system calls.
 */
auditsys()
{
	register struct a {
		int	code;
		int	a1;
		int	a2;
	} *uap;
	register int result;

	uap = (struct a *)u.u_ap;

	switch (uap->code) {
	case AUC_AUDITSVC:
		result = au_auditsvc(uap->a1, uap->a2);
		break;
	case AUC_AUDIT:
		result = au_audit((audit_record_t *)uap->a1);
		break;
	case AUC_GETAUID:
		result = au_getauid();
		break;
	case AUC_SETAUID:
		result = au_setauid(uap->a1);
		break;
	case AUC_SETUSERAUDIT:
		result = au_setuseraudit((uid_t)uap->a1,
		    (audit_state_t *)uap->a2);
		break;
	case AUC_SETAUDIT:
		result = au_setaudit((audit_state_t *)uap->a1);
		break;
	case AUC_AUDITON:
		if (!suser() && !rsuser())
			return (EPERM);
		result = au_auditon(uap->a1);
		if (result == 0)
			u.u_r.r_val1 = au_prevstate;
		break;
	default:
		result = EINVAL;
		break;
	}
	u.u_error = result;
	return (0);
}

/*
 * Audit service.  This call is passed a file descriptor to which audit records
 * are to be written.
 */
au_auditsvc(fd, limit)
	int fd;
	int limit;
{
	register au_buf_t *buf;
	register audit_record_t *rec;
	struct file *fp;
	register struct vnode *vp;
	register int error;

	if (!suser() && !rsuser())
		return (EPERM);
	if (limit < 0 || !CKSTATE(AUC_AUDITING))
		return (EINVAL);
	/*
	 * Get the vnode for which to write audit records.
	 */
	if (error = getvnodefp(fd, &fp))
		return (error);

	vp = (struct vnode *)fp->f_data;
	/*
	 * Prevent a second audit daemon from running
	 * this code
	 */
	if (au_iocb.ai_busy)
		return (EBUSY);
	au_iocb.ai_busy++;

	while (!error) {
		/*
		 * Wait for work, until a signal arrives
		 * or if auditing has been disabled
		 */
		if ((CKSTATE(AUC_AUDITING)) &&
		    ((buf = au_iocb.ai_head) == NULL)) {
			if (sleep((caddr_t)&au_iocb.ai_head,
					(PZERO+1 | PCATCH))) {
				/*
				 * Exit because of a signal but first
				 * free buffer if auditing is not on.
				 */
				if (!CKSTATE(AUC_AUDITING)) {
					if ((buf = au_iocb.ai_head) != NULL) {
						kmem_free((caddr_t)buf,
						    buf->ab_size);
						au_iocb.ai_busy--;
						return (EINVAL);
					}
				}
				au_iocb.ai_busy--;
				return (EINTR);
			}
			continue;
		}
		/*
		 * free buffer if auditing is disabled
		 */
		if (!CKSTATE(AUC_AUDITING)) {
			if ((au_iocb.ai_head) != NULL)
				kmem_free((caddr_t)buf, buf->ab_size);
			au_iocb.ai_busy--;
			return (EINVAL);
		} else {
			rec = (audit_record_t *)&(buf[1]);
			error = au_write(vp, limit, rec, rec->au_record_size);
			if (!CKSTATE(AUC_AUDITING)) {
				if ((au_iocb.ai_head) != NULL) {
					kmem_free((caddr_t)buf, buf->ab_size);
					au_iocb.ai_busy--;
					return (EINVAL);
				}
				break;
			}
			if (!error || error == EDQUOT) {
				if ((au_iocb.ai_head = buf->ab_next) == NULL)
					au_iocb.ai_tail = (au_buf_t *)0;
				au_freebuf(buf);
			}
		}
	}
	au_iocb.ai_busy--;
	return (error);
}

/*
 * Initialize the audit environment.
 */
au_init()
{
	register au_fq_t *queue;
	register au_buf_t **bufp;
	register int i;

	queue = &au_iocb.ai_free;
	bufp = &(queue->af_free);
	for (i = 0; i < AU_NBUFS; i++) {
		*bufp = (au_buf_t *)new_kmem_alloc(AU_BFSZ, KMEM_SLEEP);
		(*bufp)->ab_size = AU_BFSZ;
		bufp = &(*bufp)->ab_next;
	}
	*bufp = (au_buf_t *)0;
	return;
}

/*
 * Allocate a buffer for storing an audit record to be written.  If there
 * are none available, then wait for one to be freed.
 */
au_buf_t *
au_getbuf(size)
	register u_int size;
{
	register au_buf_t *buf;
	register au_fq_t *queue;

	size += AU_HDSZ;

	if (size > AU_BFSZ) {
		while ((CKSTATE(AUC_AUDITING)) &&
		    (au_iocb.ai_nlbufs >= AU_NLBUFS)) {
			au_iocb.ai_wlbufs++;
			(void)sleep((caddr_t)&au_iocb.ai_wlbufs, PZERO);
		}
		if (!CKSTATE(AUC_AUDITING))
			return (NULL);

		au_iocb.ai_nlbufs++;
		buf = (au_buf_t *)new_kmem_alloc(size, KMEM_SLEEP);
		buf->ab_size = size;
	} else {
		queue = &au_iocb.ai_free;
		while ((CKSTATE(AUC_AUDITING)) &&
		    ((buf = queue->af_free) == NULL)) {
			queue->af_wait++;
			(void)sleep((caddr_t)queue, PZERO);
		}
		if (!CKSTATE(AUC_AUDITING))
			return (NULL);

		queue->af_free = buf->ab_next;
	}
	buf->ab_next = (au_buf_t *)0;
	return (buf);
}

/*
 * Free up an audit record buffer, wake up processes waiting for audit
 * record buffers.
 */
au_freebuf(buf)
	register au_buf_t *buf;
{
	register au_fq_t *queue;

	if (buf->ab_size != AU_BFSZ) {
		kmem_free((caddr_t)buf, buf->ab_size);
		au_iocb.ai_nlbufs--;
		if (au_iocb.ai_wlbufs > 0) {
			au_iocb.ai_wlbufs--;
			wakeup_one((caddr_t)&au_iocb.ai_wlbufs);
		}
	} else {
		queue = &au_iocb.ai_free;
		buf->ab_next = queue->af_free;
		queue->af_free = buf;
		if (queue->af_wait > 0) {
			queue->af_wait--;
			wakeup_one((caddr_t)queue);
		}
	}
}

/*
 * Put an audit record buffer on the outbound queue and wake up the audit
 * daemon to write out the record.
 */
au_putbuf(buf)
	register au_buf_t *buf;
{

	buf->ab_next = (au_buf_t *)0;
	if (au_iocb.ai_tail) {
		au_iocb.ai_tail->ab_next = buf;
	} else {
		au_iocb.ai_head = buf;
	}
	au_iocb.ai_tail = buf;
	wakeup_one((caddr_t)&au_iocb.ai_head);
}

time_t au_checktime;
/*
 * Check to insure enough free space on audit device.
 */
au_checkfree(vp, limit)
	register struct vnode *vp;
	register int limit;
{
	struct statfs sb;

	if (((time_t)time.tv_sec - au_checktime) > AU_INTERVAL) {
		(void)VFS_STATFS(vp->v_vfsp, &sb);
		if (limit > sb.f_bavail)
			return (EDQUOT);

		au_checktime = (time_t)time.tv_sec;
	}
	return (0);
}

/*
 * The audit system call. Trust what the user has sent down and save it
 * away in the audit file.
 */
au_audit(record)
	audit_record_t *record;
{
	register au_buf_t *abuf;
	audit_record_t trec;
	audit_record_t *myrecord;
	register int error;
	register u_int size;

	if (!suser())
		return (EPERM);

	if (!CKSTATE(AUC_AUDITING))
		return (0);

	/* Read in the audit record header */
	if (error = copyin((caddr_t)record, (caddr_t)&trec,
	    sizeof (audit_record_t)))
		return (error);

	/* Certify audit record size is valid */
	size = trec.au_record_size;
	if ((size > MAXAUDITDATA) || (size < sizeof (audit_record_t)))
		return (EINVAL);

	/*
	 * Check the user's audit state against the passed event.
	 */
	switch (trec.au_errno) {
	case AU_SUCCESS:
		if (!(trec.au_event & u.u_cred->cr_audit.as_success))
			return (0);
		break;
	case AU_EITHER:
		if (!(trec.au_event &
		    (u.u_cred->cr_audit.as_success |
		    u.u_cred->cr_audit.as_failure)))
			return (0);
		break;
	default:
		if (!(trec.au_event & u.u_cred->cr_audit.as_failure))
			return (0);
		break;
	}

	/*
	 * Get buffer for audit record; returns null if
	 * auditing has been turned off.
	 */
	if ((abuf = au_getbuf(size)) == NULL)
		return (0);

	myrecord = (audit_record_t *)&(abuf[1]);

	/* Copy in the rest of the audit record */
	bcopy((caddr_t)&trec, (caddr_t)myrecord, sizeof (audit_record_t));
	size -= sizeof (audit_record_t);
	error = copyin((caddr_t)&record[1], (caddr_t)&myrecord[1],
			size);
	if (!error) {
		/* Give buffer to audit daemon to write the record */
		myrecord->au_time = (time_t)time.tv_sec;
		au_putbuf(abuf);
	} else {
		au_freebuf(abuf);
	}
	return (error);
}

/*
 * Produce a kernel audit record.
 * call:  au_sysaudit(rtype, event, errorno, result, nargs, args ...)
 */
/*VARARGS*/
au_sysaudit(va_alist)
	va_dcl
{
	va_list ap;		/* Arguments		*/
	int rtype;		/* Audit record type	*/
	int event;		/* Audit event type	*/
	int errorno;		/* Error for event	*/
	int result;		/* Result for event	*/
	int nargs;		/* # additional fields	*/
	int ngroups;
	register au_buf_t *abuf;
	register audit_record_t *myrecord;
	caddr_t buf;
	u_int size;
	short *link;
	register u_int len;
	register caddr_t data;

	/*
	 * Make sure auditing is enabled
	 */
	if (!CKSTATE(AUC_AUDITING))
		return;

	va_start(ap);

	/* Get initial arguments */
	rtype = va_arg(ap, int);
	event = va_arg(ap, int);
	errorno = va_arg(ap, int);
	result = va_arg(ap, int);
	nargs = va_arg(ap, int);
	/*
	 * Check the user's audit state against the passed event.
	 */
	switch (errorno) {
	case AU_SUCCESS:
		if (!(event & u.u_cred->cr_audit.as_success))
			return;
		break;
	case AU_EITHER:
		if (!(event &
		    (u.u_cred->cr_audit.as_success |
		    u.u_cred->cr_audit.as_failure)))
			return;
		break;
	default:
		if (!(event & u.u_cred->cr_audit.as_failure))
			return;
		break;
	}

	/* Compute size for audit record */
	size = sizeof (audit_record_t) + ((nargs + 1) * sizeof (short));
	while (nargs--) {
		len = va_arg(ap, u_int);
		data = va_arg(ap, caddr_t);
		size += au_sizearg(len, data);
	}
	va_end(ap);

	for (ngroups=0; ngroups<NGROUPS && u.u_groups[ngroups] != NOGROUP;
	    ngroups++)
		size += sizeof (int);
	/*
	 * Get buffer for audit record - return if auditing is disabled
	 */
	if ((abuf = au_getbuf(size)) == NULL)
		return;
	myrecord = (audit_record_t *)&(abuf[1]);

	va_start(ap);
	rtype = va_arg(ap, int);
	event = va_arg(ap, int);
	errorno = va_arg(ap, int);
	result = va_arg(ap, int);
	nargs = va_arg(ap, int);

	/* Fill in the standard fields */
	myrecord->au_record_type = rtype;
	myrecord->au_event = event;
	myrecord->au_uid = u.u_ruid;
	myrecord->au_auid = u.u_auid;
	myrecord->au_euid = u.u_uid;
	myrecord->au_gid = u.u_rgid;
	myrecord->au_pid = u.u_procp->p_pid;
	myrecord->au_errno = errorno;
	myrecord->au_return = result;
	myrecord->au_label = u.u_cred->cr_label;
	myrecord->au_param_count = nargs + 1; /* + 1 for group list */

	/* Fill in the group list */
	link = (short *)&myrecord[1];
	buf = (caddr_t)&(link[nargs + 1]);
	size = ((caddr_t)abuf + abuf->ab_size) - buf;
	au_doarg(&buf, &size, &link, (u_int)(ngroups * sizeof (int)),
	    (caddr_t)u.u_groups);

	/* Fill in extra data */
	while (nargs--) {
		len = va_arg(ap, u_int);
		data = va_arg(ap, caddr_t);
		au_doarg(&buf, &size, &link, len, data);
	}
	va_end(ap);
	/* Compute length. */
	myrecord->au_record_size = buf - (caddr_t)myrecord;

	/* Give buffer to audit daemon to write the record */
	myrecord->au_time = (time_t)time.tv_sec;
	au_putbuf(abuf);
}

/*
 * Compute the size of specified data.
 * The len parameter indicates the size and address space of the data:
 *   AUP_USER	Data is in user address space.
 *   0		Data is a string terminated with a NULL.
 */
au_sizearg(len, data)
	u_int len;		/* Specifies size of data */
	register caddr_t data;	/* Address of audit data */
{
	register int uflag = 0;
	register int error = 0;
	u_int step;
	char buf[10];

	if (len & AUP_USER) {
		uflag = 1;
		len &= ~AUP_USER;
	}
	if (len == 0) {
		if (uflag) {
			do {
				error = copyinstr(data, buf,
						sizeof (buf), &step);
				data += step;
				len += step;
			} while (error == ENAMETOOLONG);
		} else {
			len = strlen(data) + 1;
		}
	}
	len = AU_ALIGN(len);
	/* Bad fields are truncated to length 0 */
	if (error)
		len = 0;

	return (len);
}

/*
 * Copy additional audit record data into audit record buffer.
 * The len parameter indicates the size and address space of the data:
 *   AUP_USER	Data is in user address space.
 *   0		Data is a string terminated with a NULL.
 */
au_doarg(bufp, sizep, linkp, len, data)
	register caddr_t *bufp;	/* Audit record buffer pos	*/
	register u_int *sizep;	/* Space left in audit record	*/
	register short **linkp;	/* Set size of field		*/
	u_int len;		/* Specifies size of data	*/
	register caddr_t data;	/* Address of audit data	*/
{
	register int uflag = 0;
	register int error = 0;

	if (len & AUP_USER) {
		uflag = 1;
		len &= ~AUP_USER;
	}
	if (len == 0) {
		if (uflag) {
			error = copyinstr(data, *bufp, *sizep, &len);
		} else {
			error = copystr(data, *bufp, *sizep, &len);
		}
	} else {
		if (len > *sizep)
			error = ENAMETOOLONG;
		else if (uflag) {
			error = copyin(data, *bufp, len);
		} else {
			bcopy(data, *bufp, len);
		}
	}
	len = AU_ALIGN(len);
	/* Bad fields are truncated to length 0 */
	if (error) len = 0;

	*(*linkp)++ = len;

	*bufp += len;
	*sizep -= len;
}

/*
 * au_write
 *	writes to the current audit file, changing if necessary.
 *
 */
au_write(vp, limit, record, size)
	register struct vnode *vp;
	int limit;
	audit_record_t *record;
	short size;

{
	caddr_t write_from = (caddr_t)record;
	int size_as_int = size;
	int aresid;
	int error;

	/*
	 * Check for incomplete writing; repeat as necessary
	 */
	do {
		error = vn_rdwr(UIO_WRITE, vp, write_from, size_as_int,
		    0, UIO_SYSSPACE, IO_UNIT|IO_APPEND, &aresid);
		if (aresid != 0) {
			write_from += size_as_int - aresid;
			size_as_int = aresid;
		}
	} while (aresid != 0 && error == 0);
	/*
	 * Check to see if the file space threshhold has been exceeded,
	 * in which case the next audit directory should be used.
	 */
	if (!error)
		error = au_checkfree(vp, limit);
	return (error);
}

/*
 * Return the audit user ID for the current process.
 */
au_getauid()
{
	if (!suser() && !rsuser())
		return (EPERM);

	u.u_r.r_val1 = u.u_auid;
	return (0);
}

/*
 * Set the audit userid, for a process.  This can only be changed by
 * super-user.  The audit userid is inherited across forks & execs.
 */
au_setauid(auid)
	register int auid;
{
	if (!suser() && !rsuser())
		return (EPERM);

	if (auid == -1)				/* Not a valid audit user ID */
		return (EINVAL);
	u.u_cred = crcopy(u.u_cred);
	u.u_auid = auid;
	return (0);
}

/*
 * Set the audit state information for all processes of the specified user.
 */
au_setuseraudit(uid, state_p)
	register uid_t uid;
	register audit_state_t *state_p;
{
	register struct proc *p;
	register int error;
	audit_state_t state;

	if (!suser() && !rsuser())
		return (EPERM);
	if (uid == (unsigned short)AU_NOAUDITID)
		return (EINVAL);

	if (error = copyin((caddr_t)state_p, (caddr_t)&state,
			sizeof (audit_state_t)))
		return (error);

	/* Change all matching credentials, so no crcopy is needed */

	/* Change audit state for crendentials referenced by process table */
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_cred->cr_auid == uid) {
			p->p_cred->cr_audit = state;
		}
	}
	return (0);
}

/*
 * Set the audit state information for the current process.
 */
au_setaudit(state_p)
	register audit_state_t *state_p;
{
	audit_state_t state;
	register int error;

	if (!suser() && !rsuser())
		return (EPERM);

	if (error = copyin((caddr_t)state_p, (caddr_t)&state,
			sizeof (audit_state_t)))
		return (error);

	/* Get a private copy of credentials */
	u.u_cred = crcopy(u.u_cred);

	/* Set audit state specified */
	u.u_cred->cr_audit = state;
	return (0);
}

/*
 * sets condition of audit mechanism
 */
au_auditon(condition)
	register int condition;
{
	register au_buf_t *buf, *nextbuf;

	switch (condition) {
	case AUC_UNSET:
	default:
		return (EINVAL);
	case AUC_AUDITING:
		if (CKSTATE(AUC_FCHDONE))
			return (EINVAL);
		/*
		 * return state before change
		 *  and set new state
		 */
		au_prevstate = au_auditstate;
		if (!CKSTATE(AUC_AUDITING)) {
			/*
			 * initialize global values
			 */
			au_checktime = (time_t)0;
			au_iocb.ai_head = (au_buf_t *)0;
			au_iocb.ai_tail = (au_buf_t *)0;
			au_iocb.ai_free.af_free = (au_buf_t *)0;
			au_iocb.ai_free.af_wait = 0;
			au_iocb.ai_nlbufs = 0;
			au_iocb.ai_wlbufs = 0;
			au_init();
		}
		au_auditstate = AUC_AUDITING;
		break;

	case AUC_NOAUDIT:
		if (CKSTATE(AUC_FCHDONE))
			return (EINVAL);

		/* save state */
		au_prevstate = au_auditstate;
		if (CKSTATE(AUC_NOAUDIT))
			break;

		/* set new state */
		au_auditstate = AUC_NOAUDIT;
		/*
		 * no need to keep processes waiting.
		 * wake up processes  waiting for buffers.
		 */
		wakeup((caddr_t)&au_iocb.ai_wlbufs);
		wakeup((caddr_t)&au_iocb.ai_free);
		/*
		 * free space for unused empty buffers
		 */
		if ((buf = au_iocb.ai_free.af_free) != NULL) {
			do {
				nextbuf = buf->ab_next;
				kmem_free((caddr_t)buf, buf->ab_size);
				buf = nextbuf;
			} while (buf != NULL);
		}
		/*
		 * Free buffers on queue.  If auditd is not active
		 * free all buffers; if it is, don't free the current
		 * buffer.
		*/
		wakeup((caddr_t)&au_iocb.ai_head);

		if (au_iocb.ai_head != NULL) {
			if (au_iocb.ai_busy == 0)
				buf = au_iocb.ai_head;
			else
				buf = au_iocb.ai_head->ab_next;
			while (buf != NULL) {
				nextbuf = buf->ab_next;
				kmem_free((caddr_t)buf, buf->ab_size);
				buf = nextbuf;
			}
		}
		break;

	case AUC_FCHDONE:
		/*
		 * This value is only called from the kernel.
		 * Save state whether switch is made or not.
		 */
		au_prevstate = au_auditstate;
		if (!CKSTATE(AUC_NOAUDIT) && !CKSTATE(AUC_FCHDONE))
			return (EINVAL);
		/*
		 *  set new state
		 */
		au_auditstate = AUC_FCHDONE;
		break;
	}
	return (0);
}

rsuser()
{
	if (u.u_ruid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}

/*
 * These are the routines which keep the u_cwd structure up to date.
 * In a system without auditing they are (mostly) stubs.
 */

void
cwincr(cwp)
	struct ucwd *cwp;
{
	cwp->cw_ref++;
}

void
cwfree(cwp)
	struct ucwd *cwp;
{
	if (--(cwp->cw_ref) == 0)
		kmem_free((caddr_t)cwp, cwp->cw_len);
}

/*
 * Duplicate cwd structure, replacing root (or dir, if root == NULL).
 */
void
cwdup(ucwd, root, dir, cwdp)
	struct ucwd *ucwd;
	caddr_t root;
	caddr_t dir;
	struct ucwd **cwdp;
{
	struct ucwd *cwp;
	u_int loroot;
	u_int lroot;
	u_int ldir;
	u_int ltotal;

	if (root == NULL)
		root = "";
	if (dir == NULL)
		dir = ucwd->cw_dir;

	loroot = strlen(ucwd->cw_root);
	lroot = strlen(root) + 1;
	ldir = strlen(dir) + 1;
	ltotal = sizeof (struct ucwd) + loroot + lroot + ldir;
	cwp = (struct ucwd *)new_kmem_alloc(ltotal, KMEM_SLEEP);
	cwp->cw_ref = 1;
	cwp->cw_len = ltotal;
	cwp->cw_root = (caddr_t)&(cwp[1]);
	cwp->cw_dir = cwp->cw_root + loroot + lroot;
	bcopy(ucwd->cw_root, cwp->cw_root, loroot);
	bcopy(root, (cwp->cw_root) + loroot, lroot);
	bcopy(dir, cwp->cw_dir, ldir);
	*cwdp = cwp;
}

/*
 * Expand the size of the audit path buffer, preserving the pathname
 * generated so far.
 */
void
au_pathexpand(au_path)
	register au_path_t *au_path;
{
	register u_int newsize;
	register caddr_t newbuf;
	register u_int len;

	newsize = au_path->ap_size + MAXPATHLEN;
	newbuf = new_kmem_alloc(newsize, KMEM_SLEEP);
	len = au_path->ap_ptr - au_path->ap_buf;
	bcopy(au_path->ap_buf, newbuf, len);
	kmem_free(au_path->ap_buf, au_path->ap_size);
	au_path->ap_size = newsize;
	au_path->ap_buf = newbuf;
	au_path->ap_ptr = newbuf + len;
}

/*
 * Insert a character into the audit path buffer, expanding the buffer
 * size if necessary.
 */
#define	AU_ADDC(p, x) { \
	if ((p)->ap_ptr >= (p)->ap_buf + (p)->ap_size) \
		au_pathexpand(p); \
	*((p)->ap_ptr++) = (x); \
}

/*
 * Append component to audit pathname.  Handle components '..' and '.'
 * specialy.
 */
void
au_pathbuild(au_path, comp)
	register au_path_t *au_path;
	register caddr_t comp;
{
	if (*comp == '/')
		comp++;
	/* Initialize buffer */
	if (au_path->ap_size == 0) {
		au_path->ap_size = MAXPATHLEN;
		au_path->ap_buf = new_kmem_alloc(au_path->ap_size, KMEM_SLEEP);
		au_path->ap_ptr = au_path->ap_buf;
	}
	if (*comp == '\0') {
		/* Nothing */
	} else if (bcmp(comp, "..\0", 3) == 0) {
		while (au_path->ap_ptr > au_path->ap_buf &&
				*(--au_path->ap_ptr) != '/')
			;
	} else if (bcmp(comp, ".\0", 2) == 0) {
		/* Nothing */
	} else {
		AU_ADDC(au_path, '/');
		while (*comp) {
			AU_ADDC(au_path, *comp++);
		}
	}
	AU_ADDC(au_path, '\0');
	au_path->ap_ptr--;
}

/*
 * Free up audit pathname buffer.
 */
void
au_pathfree(au_path)
	register au_path_t *au_path;
{

	if (au_path->ap_size != 0) {
		kmem_free(au_path->ap_buf, au_path->ap_size);
		au_path->ap_size = 0;
	}
}
