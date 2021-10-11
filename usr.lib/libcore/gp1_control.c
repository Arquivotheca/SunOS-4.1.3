/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)gp1_control.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include <sys/ioctl.h>
#include <sun/gpio.h>
#include "coretypes.h"
#include "corevars.h"
#include "gp1_pwpr.h"
#include <pixrect/gp1cmds.h>
#include <stdio.h>
#include <signal.h>

extern int _core_client;


_core_gp1_winupdate(ws, port, attr)
	register struct windowstruct *ws;
	register porttype *port;
	register struct gp1_attr *attr;
{
    int xpixmin, xpixmax, ypixmin, ypixmax;

    _core_gp1_pw_updclplst( ws->pixwin, attr);

	xpixmin = (port->xmin * ws->xscale >> 15) + ws->xoff + attr->org_x;
	xpixmax = (port->xmax * ws->xscale >> 15) + ws->xoff + attr->org_x;
	attr->xscale = ((float) (xpixmax - xpixmin)) / (float) 2.0;
	attr->xoffset = ((float) (xpixmax + xpixmin)) / (float) 2.0;

	ypixmin = ws->yoff - (port->ymin * ws->yscale >> 15) + attr->org_y;
	ypixmax = ws->yoff - (port->ymax * ws->yscale >> 15) + attr->org_y;
	attr->yscale = ((float) (ypixmax - ypixmin)) / (float) 2.0;
	attr->yoffset = ((float) (ypixmax + ypixmin)) / (float) 2.0;

	attr->zscale = (float)(port->zmax - port->zmin);
	attr->zoffset = (float) port->zmin;

	/* non screen (ndc) scaling */
	attr->ndcxscale  = ((float)(port->xmax - port->xmin)) / (float) 2.0;
	attr->ndcxoffset  = ((float)(port->xmax + port->xmin)) / (float) 2.0;

	attr->ndcyscale  = ((float)(port->ymax - port->ymin)) / (float) 2.0;
	attr->ndcyoffset  = ((float)(port->ymax + port->ymin)) / (float) 2.0;

	attr->ndczscale = (float)(port->zmax - port->zmin);
	attr->ndczoffset = (float) port->zmin;
	attr->xfrmtype = XFORMTYPE_CHANGE;
}


/*
 * _core_gp1_pw_lock and _core_gp1_pw_updclplst return:
 *	0 if org_x and org_y are unchanged from last call
 *	1 if either org_x or org_y has changed
 *
 * Note that these can change even though no SIGWINCH is received, but
 * they cannot change when the screen is locked.  Thus it should suffice
 * to check the return code from _core_gp1_pw_lock and if it is non-zero
 * update the gp1 NDC to screen transform.
 */

_core_gp1_pw_lock(pw, r, ptr)
struct pixwin *pw;
struct rect *r;
struct gp1_attr *ptr;
	{
	int retcode;

	pw_lock(pw, r);
	if (pw->pw_clipdata->pwcd_clipid == ptr->clipid)
		return(0);
	retcode = _core_gp1_pw_updclplst(pw, ptr);
	if (ptr->hwzbuf) {
		pw_unlock(pw);
		ptr->forcerepaint = TRUE;
		(void)_core_winsig();
		pw_lock(pw, r);
		}

	return (retcode);
	}

int _core_gp1_stblk_alloc(fd)
int fd;
	{
	int i;

	if( ioctl(fd, GP1IO_GET_STATIC_BLOCK, &i) )
		return (-1);
	else
		return(i);
	}

_core_gp1_stblk_free(fd, num)
int num;
	{
	if ( ioctl(fd, GP1IO_FREE_STATIC_BLOCK, &num) )
		return(-1);
	else
		return(0);
	}

_core_gp1_chkloc(addr, fd)
short *addr;
int fd;
	{
	int i;

	/* about a 5 second timeout */
	for (i=0; i<600000; i++) {
		if (!*addr) {
			return(0);
			}
		}
	gp1_kern_gp_restart(fd, 0);
	return(-1);
	}

Notify_value
_core_sigxcpu()
	{
	/* Receive this signal whenever kernel resets GP */
	/* Must assume that all GP state has been lost for this process */

        int gpsentsignal;
	viewsurf *sp;
	ddargtype ddstruct;
 
        gpsentsignal = FALSE;
        for (sp = &_core_surface[0]; sp < &_core_surface[MAXVSURF]; sp++) {
                if (!sp->vinit)
                        continue;
                if (sp->hphardwr) {
			ddstruct.opcode = CHKGPRESETCOUNT;
			ddstruct.instance = sp->vsurf.instance;
			if ((* sp->vsurf.dd)(&ddstruct))
				gpsentsignal = TRUE;
                }
        }
 
        /* if SIGXCPU was not sent by the GP pass it on */
        if (gpsentsignal) {
		return (NOTIFY_DONE);
	} else {
		return (NOTIFY_IGNORED);
	}
}

_core_gp1_pw_updclplst(pw, ptr)
struct pixwin *pw;
struct gp1_attr *ptr;
	{
	int nrect;
	unsigned int bitvec;
	short offset, *gp1_addr, *nptr;
	register short *bufptr;
	struct pixwin_prlist *prl;
	struct gp1pr *dmd;
	int x, y, retcode, cnt;

	ptr->clipid = pw->pw_clipdata->pwcd_clipid;
Restart:
	cnt = ptr->resetcnt;
	retcode = 0;
	dmd = gp1_d(pw->pw_pixrect);
	gp1_addr = (short *) dmd->gp_shmem;
	while((offset = gp1_alloc(gp1_addr, 1, &bitvec, dmd->minordev,
	    dmd->ioctl_fd)) == 0)
		;
	bufptr = &gp1_addr[offset];
	*bufptr++ = GP1_USEFRAME | (ptr->statblkindx & 0x7);
	*bufptr++ = GP1_SETCLPLST;
	nptr = bufptr++;
	if (pw->pw_clipdata->pwcd_state == PWCD_NULL)
		{
		*nptr = 1;
		*bufptr++ = 0;
		*bufptr++ = 0;
		*bufptr++ = 0;
		*bufptr++ = 0;
		}
	else
		{	/* better be PWCD_SINGLERECT or PWCD_MULTIRECTS */
		nrect = 0;
		dmd = gp1_d(pw->pw_clipdata->pwcd_prmulti);
		x = dmd->cgpr_offset.x;
		y = dmd->cgpr_offset.y;
		if (x != ptr->org_x | y != ptr->org_y)
			{
			ptr->org_x = x;
			ptr->org_y = y;
			retcode = 1;
			}
		for(prl = pw->pw_clipdata->pwcd_prl; prl; prl = prl->prl_next)
			{
			dmd = gp1_d(prl->prl_pixrect);
			*bufptr++ = dmd->cgpr_offset.x;
			*bufptr++ = dmd->cgpr_offset.y;
			*bufptr++ = prl->prl_pixrect->pr_size.x;
			*bufptr++ = prl->prl_pixrect->pr_size.y;
			if(++nrect == 60)
				break;
			}
		*nptr = nrect;
		}
	*bufptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(bufptr, &bitvec);
	if (gp1_post(gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == ptr->resetcnt)
			;
		goto Restart;
		}
	ptr->clplstchg = FALSE;
	return(retcode);
	}

_core_gp1_reset(wind, gp1attr, port)
struct windowstruct *wind;
struct gp1_attr *gp1attr;
porttype *port;
	{
	struct gp1pr *dmd;
	short * gp1_addr;
	register short *shmptr;
	int offset;
	unsigned int bitvec;
	int i, j, cnt;
	register float *matptr;
	struct pixwin *pw = wind->pixwin;

Restart:
	cnt = gp1attr->resetcnt;
	if (_core_gp1_pw_updclplst(pw, gp1attr))
		_core_gp1_winupdate(wind, port, gp1attr);
	gp1attr->attrchg = TRUE;
	_core_gp1_snd_attr(pw, gp1attr);
	_core_gp1_snd_xfrm_attr(pw, gp1attr, gp1attr->xfrmtype);
	dmd = gp1_d(pw->pw_pixrect);
	gp1_addr = gp1attr->gp1_addr;
	while((offset = gp1_alloc(gp1_addr, 1, &bitvec, dmd->minordev,
	    dmd->ioctl_fd)) == 0)
		;
	shmptr = &gp1_addr[offset];
	*shmptr++ = GP1_USEFRAME | (gp1attr->statblkindx);
	matptr = (float *) gp1attr->mtxlist[0];
	for (i = 0; i < 6; i++)
		{
		*shmptr++ = GP1_SET_MATRIX_3D | i;
		for (j = 0; j < 16; j++)
			CORE_GP_PUT_FLOAT(shmptr, matptr);
			matptr++;
		}
	*shmptr++ = GP1_SELECTMATRIX | 1;
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(shmptr, &bitvec);
	if (gp1_post(gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == gp1attr->resetcnt)
			;
		goto Restart;
		}
	gp1attr->needreset = FALSE;
	}
