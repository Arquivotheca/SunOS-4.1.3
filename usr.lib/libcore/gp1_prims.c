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
static char sccsid[] = "@(#)gp1_prims.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif


/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include "coretypes.h"
#include "corevars.h"
#include "gp1_pwpr.h"
#include "colorshader.h"
#include <pixrect/gp1cmds.h>

short *_core_gp1_begin_batch( wind, attr, port)
struct windowstruct *wind;
struct gp1_attr *attr;
porttype *port;
{
	struct gp1pr *dmd;
	struct pixwin *pw = wind->pixwin;

	if (attr->needreset)
		_core_gp1_reset(wind, attr, port);
	if (attr->attrchg)
		_core_gp1_snd_attr(pw, attr);
	if (attr->xfrmtype != XFORMTYPE_IMAGE)
		_core_gp1_snd_xfrm_attr(pw, attr, XFORMTYPE_IMAGE);
	dmd = gp1_d(pw->pw_pixrect);
	while ((attr->offset = gp1_alloc(attr->gp1_addr, 1, &attr->bitvec,
	    dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	return(attr->shmptr = &(attr->gp1_addr)[attr->offset]);
}


_core_gp1_end_batch(pw, attr)
struct pixwin *pw;
struct gp1_attr *attr;
{
	struct gp1pr *dmd;
	int cnt;
	register short *shmptr;

	cnt = attr->resetcnt;
	dmd = gp1_d(pw->pw_pixrect);
	shmptr = attr->shmptr;
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(shmptr, &attr->bitvec);
	if (gp1_post(attr->gp1_addr, attr->offset, dmd->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		return(-1);
	}
	return(0);
}


_core_gp1_clear_zbuffer(pw, attr)
struct pixwin *pw;
register struct gp1_attr *attr;
	{
	int cnt;
	unsigned int bitvec;
	register int offset;
	register short *shmptr;
	struct gp1pr *dmd;

        if (pw->pw_clipdata->pwcd_clipid != attr->clipid)
		_core_gp1_pw_updclplst(pw, attr);
Restart:
	cnt = attr->resetcnt;
	dmd = gp1_d(pw->pw_pixrect);
	while ((offset = gp1_alloc(attr->gp1_addr, 1, &bitvec,
	    dmd->minordev, dmd->ioctl_fd)) == 0)
		;
	shmptr = &(attr->gp1_addr)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_SETZBUF;
	*shmptr++ = 0xFFFF;	/* set depth for each point to max possible */
	*shmptr++ = 0;		/* This rectangle gets clipped to the rect */
	*shmptr++ = 0;		/* clipping list previously sent to GP */
	*shmptr++ = 1152;	/* Cover entire z-buffer for either 1152x900 */
	*shmptr++ = 1024;	/* or 1024x1024 CG2 devices */
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(shmptr, &bitvec);
	if (gp1_post(attr->gp1_addr, offset, dmd->ioctl_fd))
		{
		while (cnt == attr->resetcnt)
			;
		goto Restart;
		}
	return(0);
	}

_core_gp1_polygon_2(wind, port, attr, n, vtxtypeptr)
	struct windowstruct *wind;
	porttype *port;
	struct gp1_attr *attr;
	int n;
	vtxtype *vtxtypeptr;
{
	register int i, cnt, nblocks, offset;
	register short *shmptr;
	unsigned int bitvector;
	struct gp1pr *prptr;
	int *vtxptr;

	/* if too many vertices ... drop on the floor for now*/
	if (n > 180){
		printf("Number of vertices %d exceeded GP limit of 180",n);
	    	return;
	}


	nblocks = (7 + 6*n)/512 + 1;

Restart:
	cnt = attr->resetcnt;
	if (wind->needlock && _core_gp1_pw_lock( wind->pixwin, (Rect *)&wind->rect, attr)) {
		_core_gp1_winupdate(wind, port, attr);
        }

	if (attr->needreset)
		_core_gp1_reset(wind, attr, port);
	if (attr->attrchg)
		_core_gp1_snd_attr(wind->pixwin, attr);
        if (attr->xfrmtype != XFORMTYPE_IMAGE)
                _core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_IMAGE);

	prptr = gp1_d(wind->pixwin->pw_pixrect);

	while ((offset = gp1_alloc(prptr->gp_shmem, nblocks, &bitvector, 
			prptr->minordev, prptr->ioctl_fd)) == 0)
		;

	vtxptr = (int *)vtxtypeptr;
	shmptr = &((short *)prptr->gp_shmem)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_CORENDCPOLY_3D | GP1_SHADE_CONSTANT;
	*shmptr++ = 1;
	*shmptr++ = n;
	for (i=0; i<n; i++) {
		CORE_GP_PUT_INT(shmptr, vtxptr);
		vtxptr++;
		CORE_GP_PUT_INT(shmptr, vtxptr);
		vtxptr++;
		CORE_GP_PUT_INT(shmptr, vtxptr);
		vtxptr++;
		vtxptr += 5;			/* skip w */
	}
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(shmptr, &bitvector);
	if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		if (wind->needlock)
			pw_unlock(wind->pixwin);
		goto Restart;
	}

	if (wind->needlock)
		pw_unlock(wind->pixwin);
}
		
_core_gp1_polygon_3(wind, port, attr, segptr, n, vtxtypeptr, shadestyle)
	struct windowstruct *wind;
	porttype *port;
	struct gp1_attr *attr;
	segstruc *segptr;
	int n;
	vtxtype *vtxtypeptr;
	int shadestyle;
{
	register int i, cnt, nblocks, offset, hue;
	register short *shmptr;
	unsigned int bitvector;
	struct gp1pr *prptr;
	int *vtxptr;
	int hueshift, hueoffset, gpshadestyle;
	int intval;

	/* if too many vertices ... drop on the floor for now*/
	if (n > 180){
	    	printf("Number of vertices %d exceeded GP limit of 180\n",n);
	    	return;
	}
	
	switch (shadestyle) {
		case WARNOCK:
			gpshadestyle = GP1_SHADE_CONSTANT;
			break;
		case GOURAUD:
		case PHONG: /* phong is currently simulated via GOURAUD */
			gpshadestyle = GP1_SHADE_GOURAUD;
			break;
	}

	if (shadestyle == WARNOCK)
		nblocks = (7 + 6*n)/512 + 1;
	else
		nblocks = (7 + 8*n)/512 + 1;
	
	if ( (hue = _core_shading_parameters.hue) == 0) {
		hueshift = 9;
		hueoffset = 0;
	} else if (hue == 1) {
		hueshift = 7;
		hueoffset = 1<<16;
	} else {
		hueshift = 7;
		hueoffset = (hue -1) << 22;
	}

Restart:
	cnt = attr->resetcnt;
	if (wind->needlock && _core_gp1_pw_lock( wind->pixwin, (Rect *)&wind->rect, attr)) {
		_core_gp1_winupdate(wind, port, attr);
        }

	if (attr->needreset)
		_core_gp1_reset(wind, attr, port);
	if (attr->attrchg)
		_core_gp1_snd_attr(wind->pixwin, attr);
        if (attr->xfrmtype != XFORMTYPE_IMAGE)
                _core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_IMAGE);

	prptr = gp1_d(wind->pixwin->pw_pixrect);

	while ((offset = gp1_alloc(prptr->gp_shmem, nblocks, &bitvector, 
			prptr->minordev, prptr->ioctl_fd)) == 0)
		;

	vtxptr = (int *)vtxtypeptr;
	shmptr = &((short *)prptr->gp_shmem)[offset];
	*shmptr++ = GP1_USEFRAME | attr->statblkindx;
	*shmptr++ = GP1_CORENDCPOLY_3D | gpshadestyle;
	*shmptr++ = 1;
	*shmptr++ = n;

	switch (shadestyle) {
	    case WARNOCK:
		for (i=0; i<n; i++) {
			CORE_GP_PUT_INT(shmptr, vtxptr);
			vtxptr++;
			CORE_GP_PUT_INT(shmptr, vtxptr);
			vtxptr++;
			CORE_GP_PUT_INT(shmptr, vtxptr);
			vtxptr++;
			vtxptr += 5;			/* skip w,dx,dy,dz,dw */
		}
		break;
	    case GOURAUD:
		for (i=0; i<n; i++) {
			CORE_GP_PUT_INT(shmptr, vtxptr);
			vtxptr++;
			CORE_GP_PUT_INT(shmptr, vtxptr);
			vtxptr++;
			CORE_GP_PUT_INT(shmptr, vtxptr);
			vtxptr++;
			vtxptr++;			/* skip w */
			intval = hueoffset + ((*vtxptr++)<<hueshift);
			CORE_GP_PUT_INT(shmptr, &intval);
			vtxptr += 3;			/* skip dy,dz,dw */
		}
		break;
	    case PHONG: {
		ipt_type xform_vertex;	/* image transformed vertex */
		ipt_type xfrm_nrml;	/* image "sized" vertex normal */
		ipt_type save_vertex;	/* untransformed vertex */
		int *save_vtxptr;
		int vertex_color, len, ndotl, ndothb, nx, ny, nz;
		shading_parameter_structure *sp = &_core_shading_parameters;
		/* for all vertices */
		for (i=0; i<n; i++) {
			save_vtxptr = vtxptr;

			save_vertex.x = *vtxptr++;	/* x coord */
			save_vertex.y = *vtxptr++;	/* y coord */
			save_vertex.z = *vtxptr++;	/* z coord */
			vtxptr += 5;			/* skip w,dx,dy,dz,dw */

		/* start with ambient */
			vertex_color = sp->ambient;

		/* add in flood */
			if (sp->floodamt != 0) {
			    /* need to image transform vertex */
			    _core_imxfrm3(segptr, (ipt_type *)save_vtxptr, &xform_vertex);
			    vertex_color -= (sp->floodamt*xform_vertex.z)>>15;
			}

		/* transformations for specular and diffuse */
			if (!segptr->idenflag) {
			    /* transform normal vector */
			    _core_imszpt3(segptr,(ipt_type *)(save_vtxptr+4),
				&xfrm_nrml);
			} else {
			    xfrm_nrml.x = *(save_vtxptr+4);
			    xfrm_nrml.y = *(save_vtxptr+5);
			    xfrm_nrml.z = *(save_vtxptr+6);
			}

			/* normalize the normal (what a concept!) */
			len = _core_jsqrt((unsigned int)(((xfrm_nrml.x*xfrm_nrml.x+16384>>15) +
				(xfrm_nrml.y*xfrm_nrml.y+16384>>15) +
				(xfrm_nrml.z*xfrm_nrml.z+16384>>15))<<15));
			if (len) {
			    nx = (xfrm_nrml.x<<15)/len;
			    ny = (xfrm_nrml.y<<15)/len;
			    nz = (xfrm_nrml.z<<15)/len;
			}
			else {
				nx = 0;
				ny = 0;
				nz = 0;
			}

		/* add in diffuse */
			if (sp->diffuseamt != 0) {
				/* compute N dot L */
				ndotl = (nx*sp->lx+16384 >> 15) +
					(ny*sp->ly+16384 >> 15) +
					(nz*sp->lz+16384 >> 15);
				if (ndotl < 0)
					ndotl = -(ndotl>>1);
				if (ndotl > 32767)
					ndotl = 32767;
				vertex_color += (sp->diffuseamt * ndotl) >> 15;
			}

		/* add in specular */
			if (sp->specularamt != 0) {
				/* compute (N dot H) ^ bump */
				ndothb = (nx*sp->hx+16384 >> 15) +
					(ny*sp->hy+16384 >> 15) +
					(nz*sp->hz+16384 >> 15);
				if (ndothb < 0)
					ndothb = 0;
				if (ndothb > 32767)
					ndothb = 32767;
	
				switch ((sp->bump>9)?9:sp->bump) {
					case 9: ndothb = ndothb*ndothb>>15;
					case 8: ndothb = ndothb*ndothb>>15;
					case 7: ndothb = ndothb*ndothb>>15;
					case 6: ndothb = ndothb*ndothb>>15;
					case 5: ndothb = ndothb*ndothb>>15;
					case 4: ndothb = ndothb*ndothb>>15;
					case 3: ndothb = ndothb*ndothb>>15;
					default: ndothb = ndothb*ndothb>>15;
				}
	
				vertex_color += (sp->specularamt * ndothb)>>15;
			}

			/* throw it to the GP */
			CORE_GP_PUT_INT(shmptr, &save_vertex.x);
			CORE_GP_PUT_INT(shmptr, &save_vertex.y);
			CORE_GP_PUT_INT(shmptr, &save_vertex.z);
			intval = hueoffset + ((vertex_color)<<hueshift);
			CORE_GP_PUT_INT(shmptr, &intval);
		}
	    }
	    break;
	}

	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(shmptr, &bitvector);
	if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		if (wind->needlock)
			pw_unlock(wind->pixwin);
		goto Restart;
	}

	if (wind->needlock)
		pw_unlock(wind->pixwin);
}


_core_gp1_wld_vecs_to_screen(wind, port, attr, n, curpt, ptrx, ptry, ptrz)
	struct windowstruct *wind;
	porttype *port;
	struct gp1_attr *attr;
	int n;
	pt_type *curpt;
	float *ptrx, *ptry, *ptrz;
{
	register short *shmptr;
	register int i, j, cnt, nblocks, offset;
	unsigned int bitvector;
	int vecsleft, vecperblk, vecsthisblk;
	float x, y, z;
	float *xptr, *yptr, *zptr;
	struct gp1pr *prptr;
	short clipplanes;

	prptr = gp1_d(wind->pixwin->pw_pixrect);
	vecperblk = 42; /* (8 + 12 * vecperblk) <= 512 */
	nblocks = (n-1)/vecperblk + 1;

Restart:
	x = curpt->x;
	y = curpt->y;
	z = curpt->z;

	xptr = ptrx;
	yptr = ptry;
	zptr = ptrz;

	vecsleft = n;

	cnt = attr->resetcnt;
	if (wind->needlock && _core_gp1_pw_lock( wind->pixwin, (Rect *)&wind->rect, attr)) {
	 _core_gp1_winupdate(wind, port, attr);
        }
	if (attr->needreset)
	 _core_gp1_reset(wind, attr, port);
	if (attr->attrchg)
		_core_gp1_snd_attr(wind->pixwin, attr);
	if (attr->xfrmtype != XFORMTYPE_WLDSCR)
		_core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_WLDSCR);
	if (ptrz == 0)
		clipplanes = attr->curclipplanes & 0x3C;
	else
		clipplanes = attr->curclipplanes;


	for (j=0; j<nblocks; j++) {
	    if (vecsleft < vecperblk)
	    	vecsthisblk = vecsleft;
	    else
		vecsthisblk = vecperblk;
	    while ((offset = gp1_alloc(prptr->gp_shmem, 1, &bitvector, 
		prptr->minordev, prptr->ioctl_fd)) == 0)
		    ;

	    shmptr = &((short *)prptr->gp_shmem)[offset];
	    *shmptr++ = GP1_USEFRAME | attr->statblkindx;
	    *shmptr++ = GP1_SETCLIPPLANES | clipplanes;
	    *shmptr++ = GP1_XFVEC_3D;
	    *shmptr++ = vecsthisblk;

	    if (ptrz != 0) {	/* 3D vectors */
	        for (i=0; i<vecsthisblk; i++) {
		    CORE_GP_PUT_FLOAT(shmptr, &x);
		    CORE_GP_PUT_FLOAT(shmptr, &y);
		    CORE_GP_PUT_FLOAT(shmptr, &z);
		    CORE_GP_PUT_FLOAT(shmptr, xptr);
		    x = *xptr++;
		    CORE_GP_PUT_FLOAT(shmptr, yptr);
		    y = *yptr++;
		    CORE_GP_PUT_FLOAT(shmptr, zptr);
		    z = *zptr++;
		}
	    } else {		/* 2D vectors */
	        for (i=0; i<vecsthisblk; i++) {
		    CORE_GP_PUT_FLOAT(shmptr, &x);
		    CORE_GP_PUT_FLOAT(shmptr, &y);
		    CORE_GP_PUT_FLOAT(shmptr, &z);
		    CORE_GP_PUT_FLOAT(shmptr, xptr);
		    x = *xptr++;
		    CORE_GP_PUT_FLOAT(shmptr, yptr);
		    y = *yptr++;
		    CORE_GP_PUT_FLOAT(shmptr, &z);
		}
	    }

	    *shmptr++ = GP1_SETCLIPPLANES | attr->curclipplanes;
	    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	    CORE_GP_PUT_INT(shmptr, &bitvector);
	    if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
		    while (cnt == attr->resetcnt)
			    ;
		    if (wind->needlock)
		    	pw_unlock(wind->pixwin);
		    goto Restart;
	    }

	vecsleft -= vecperblk;
	}

	if (wind->needlock)
		pw_unlock(wind->pixwin);
}


_core_gp1_wld_vecs_to_ndc(wind, port, attr, n, curpt, ptrxin, ptryin, ptrzin,
    ptrresult, ipt1, ipt2)
	struct windowstruct *wind;
	porttype *port;
	struct gp1_attr *attr;
	int n;
	pt_type *curpt;
	float *ptrxin, *ptryin, *ptrzin;
	short *ptrresult;
	ipt_type *ipt1, *ipt2;
{
	register short *shmptr;
	short *savptr;
	register int i, j, cnt, nblocks, offset;
	unsigned int bitvector;
	int vecsleft, vecperblk, vecsthisblk;
	float x, y, z;
	float *xptr, *yptr, *zptr;
	struct gp1pr *prptr;
	short *resptr;
	ipt_type *ipt1ptr, *ipt2ptr;
	short clipplanes;

	prptr = gp1_d(wind->pixwin->pw_pixrect);
	vecperblk = 36; /* (8 + 14 * vecperblk) <= 512 */
	nblocks = (n-1)/vecperblk + 1;

Restart:
	x = curpt->x;
	y = curpt->y;
	z = curpt->z;

	xptr = ptrxin;
	yptr = ptryin;
	zptr = ptrzin;

	resptr = ptrresult;
	ipt1ptr = ipt1;
	ipt2ptr = ipt2;

	vecsleft = n;

	cnt = attr->resetcnt;
	if (attr->needreset)
	 _core_gp1_reset(wind, attr, port);
	if (attr->xfrmtype != XFORMTYPE_WLDNDC)
		_core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_WLDNDC);
	if (ptrzin == 0)
		clipplanes = attr->curclipplanes & 0x3C;
	else
		clipplanes = attr->curclipplanes;


	for (j=0; j<nblocks; j++) {
	    if (vecsleft < vecperblk)
	    	vecsthisblk = vecsleft;
	    else
		vecsthisblk = vecperblk;
	    while ((offset = gp1_alloc(prptr->gp_shmem, 1, &bitvector, 
		prptr->minordev, prptr->ioctl_fd)) == 0)
		    ;

	    savptr = shmptr = &((short *)prptr->gp_shmem)[offset];
	    *shmptr++ = GP1_USEFRAME | attr->statblkindx;
	    *shmptr++ = GP1_SETCLIPPLANES | clipplanes;
	    *shmptr++ = GP1_COREWLDVECNDC_3D;
	    *shmptr++ = vecsthisblk;

	    if (ptrzin != 0) {	/* 3D vectors */
	        for (i=0; i<vecsthisblk; i++) {
		    shmptr++;				/* resultflag */
		    CORE_GP_PUT_FLOAT(shmptr, &x);
		    CORE_GP_PUT_FLOAT(shmptr, &y);
		    CORE_GP_PUT_FLOAT(shmptr, &z);
		    CORE_GP_PUT_FLOAT(shmptr, xptr);
		    x = *xptr++;
		    CORE_GP_PUT_FLOAT(shmptr, yptr);
		    y = *yptr++;
		    CORE_GP_PUT_FLOAT(shmptr, zptr);
		    z = *zptr++;
		    *shmptr++ = 1;			/* data ready flag */
		}
	    } else {		/* 2D vectors */
	        for (i=0; i<vecsthisblk; i++) {
		    shmptr++;				/* resultflag */
		    CORE_GP_PUT_FLOAT(shmptr, &x);
		    CORE_GP_PUT_FLOAT(shmptr, &y);
		    CORE_GP_PUT_FLOAT(shmptr, &z);
		    CORE_GP_PUT_FLOAT(shmptr, xptr);
		    x = *xptr++;
		    CORE_GP_PUT_FLOAT(shmptr, yptr);
		    y = *yptr++;
		    CORE_GP_PUT_FLOAT(shmptr, &z);
		    *shmptr++ = 1;			/* data ready flag */
		}
	    }

	    *shmptr++ = GP1_EOCL;
	    if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
		    while (cnt == attr->resetcnt)
			    ;
		    goto Restart;
	    }

	    shmptr = savptr + 4;
	    for (i = 0; i < vecsthisblk; i++)
		{
		if (_core_gp1_chkloc(shmptr + 13, prptr->ioctl_fd))
		    {
		    while (cnt == attr->resetcnt)
			    ;
		    goto Restart;
		    }
		*resptr++ = *shmptr++;
		CORE_GP_GET_INT(shmptr, &ipt1ptr->x);
		CORE_GP_GET_INT(shmptr, &ipt1ptr->y);
		CORE_GP_GET_INT(shmptr, &ipt1ptr->z);
		ipt1ptr++;
		CORE_GP_GET_INT(shmptr, &ipt2ptr->x);
		CORE_GP_GET_INT(shmptr, &ipt2ptr->y);
		CORE_GP_GET_INT(shmptr, &ipt2ptr->z);
		ipt2ptr++;
		shmptr++;
		}

	    shmptr = savptr;
	    *shmptr++ = GP1_USEFRAME | attr->statblkindx;
	    *shmptr++ = GP1_SETCLIPPLANES | attr->curclipplanes;
	    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	    CORE_GP_PUT_INT(shmptr, &bitvector);
	    if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
		    while (cnt == attr->resetcnt)
			    ;
		    goto Restart;
	    }

	vecsleft -= vecperblk;
	}
}



_core_gp1_wld_polygon_to_screen(wind, port, attr, n, curpt, ptrx, ptry, ptrz)
	struct windowstruct *wind;
	porttype *port;
	struct gp1_attr *attr;
	int n;
	pt_type *curpt;
	float *ptrx, *ptry, *ptrz;
{
	register short *shmptr;
	register int i, cnt, nblocks, offset;
	unsigned int bitvector;
	struct gp1pr *prptr;
	float *xptr, *yptr, *zptr;
	int hue, hueshift, hueoffset, gpshadestyle;
	int *colorptr;
	short clipplanes;
	int intval;

	if (ptrz == 0) { /* twod vectors */
            /* if too many vertices ... drop on the floor for now*/
            if (n > 180){
		printf("Number of vertices %d exceeded GP limit of 180\n",n);
	    	return;
	    }
                

            nblocks = (9 + 6*n)/512 + 1;

            prptr = gp1_d(wind->pixwin->pw_pixrect);
 
Restart2d:
	    xptr = ptrx;
	    yptr = ptry;

            cnt = attr->resetcnt;
            if (wind->needlock && _core_gp1_pw_lock( wind->pixwin, (Rect *)&wind->rect, attr)) {
                _core_gp1_winupdate(wind, port, attr);
            }
 
            if (attr->needreset)
                _core_gp1_reset(wind, attr, port);
            if (attr->attrchg)
                _core_gp1_snd_attr(wind->pixwin, attr);
            if (attr->xfrmtype != XFORMTYPE_WLDSCR)
                _core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_WLDSCR);
	    clipplanes = attr->curclipplanes & 0x3C;
 
            while ((offset = gp1_alloc(prptr->gp_shmem, nblocks, &bitvector,
                        prptr->minordev, prptr->ioctl_fd)) == 0)
                ;
 
            shmptr = &((short *)prptr->gp_shmem)[offset];
            *shmptr++ = GP1_USEFRAME | attr->statblkindx;
            *shmptr++ = GP1_SETCLIPPLANES | clipplanes;
            *shmptr++ = GP1_XFPOLYGON_3D | GP1_SHADE_CONSTANT;
            *shmptr++ = 1;
            *shmptr++ = n;
            for (i=0; i<n; i++) {
		CORE_GP_PUT_FLOAT(shmptr, xptr);
		xptr++;
		CORE_GP_PUT_FLOAT(shmptr, yptr);
		yptr++;
		CORE_GP_PUT_FLOAT(shmptr, &curpt->z);
            }
            *shmptr++ = GP1_SETCLIPPLANES | attr->curclipplanes;
            *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	    CORE_GP_PUT_INT(shmptr, &bitvector);
            if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
                while (cnt == attr->resetcnt)
                        ;
		if (wind->needlock)
	                pw_unlock(wind->pixwin);
                goto Restart2d;
            }

	    if (wind->needlock)
	            pw_unlock(wind->pixwin);

	} else { /* 3d polygons */

            /* if too many vertices ... drop on the floor for now*/
            if (n > 180){
		printf("Number of vertices  %d exceeded GP limit of 180\n",n);
	    	return;		
	    }
                
 
	    if (_core_current.polyintstyl == PLAIN) {
                gpshadestyle = GP1_SHADE_CONSTANT;
	    } else switch (_core_shading_parameters.shadestyle) {
                case WARNOCK:
                        gpshadestyle = GP1_SHADE_CONSTANT;
                        break;
                case GOURAUD:
                        gpshadestyle = GP1_SHADE_GOURAUD;
                        break;
            }
 
            if (gpshadestyle == GP1_SHADE_CONSTANT)
                nblocks = (7 + 511 + 6*n)/512;
            else
                nblocks = (7 + 511 + 8*n)/512;
        
            if ( (hue = _core_shading_parameters.hue) == 0) {
                hueshift = 9;
                hueoffset = 0;
            } else if (hue == 1) {
                hueshift = 7;
                hueoffset = 1<<16;
            } else {
                hueshift = 7;
                hueoffset = (hue -1) << 22;
            }
        
            prptr = gp1_d(wind->pixwin->pw_pixrect);
 
Restart3d:
	    xptr = ptrx;
	    yptr = ptry;
	    zptr = ptrz;
	    colorptr = &_core_vtxlist[0].dx;

            cnt = attr->resetcnt;
            if (wind->needlock && _core_gp1_pw_lock( wind->pixwin, (Rect *)&wind->rect, attr)) {
                _core_gp1_winupdate(wind, port, attr);
            }
 
            if (attr->needreset)
                _core_gp1_reset(wind, attr, port);
            if (attr->attrchg)
                _core_gp1_snd_attr(wind->pixwin, attr);
            if (attr->xfrmtype != XFORMTYPE_WLDSCR)
                _core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_WLDSCR);

            while ((offset = gp1_alloc(prptr->gp_shmem, nblocks, &bitvector,
                        prptr->minordev, prptr->ioctl_fd)) == 0)
                ;
 
            shmptr = &((short *)prptr->gp_shmem)[offset];
            *shmptr++ = GP1_USEFRAME | attr->statblkindx;
            *shmptr++ = GP1_XFPOLYGON_3D | gpshadestyle;
            *shmptr++ = 1;
            *shmptr++ = n;
 
            switch (gpshadestyle) {
                case GP1_SHADE_CONSTANT:
                    for (i=0; i<n; i++) {
			CORE_GP_PUT_FLOAT(shmptr, xptr);
			xptr++;
			CORE_GP_PUT_FLOAT(shmptr, yptr);
			yptr++;
			CORE_GP_PUT_FLOAT(shmptr, zptr);
			zptr++;
                }
                break;
                case GP1_SHADE_GOURAUD:
                for (i=0; i<n; i++) {
			CORE_GP_PUT_FLOAT(shmptr, xptr);
			xptr++;
			CORE_GP_PUT_FLOAT(shmptr, yptr);
			yptr++;
			CORE_GP_PUT_FLOAT(shmptr, zptr);
			zptr++;
                        intval = hueoffset + ((*colorptr)<<hueshift);
			CORE_GP_PUT_INT(shmptr, &intval);
			colorptr += 8;
                }
                break;
	    }

        *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(shmptr, &bitvector);
        if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
                while (cnt == attr->resetcnt)
                        ;
		if (wind->needlock)
	                pw_unlock(wind->pixwin); 
                goto Restart3d;
        }
 
	if (wind->needlock)
	        pw_unlock(wind->pixwin);

    }
}

_core_gp1_wld_polygon_to_ndc(wind, port, attr, n, curpt, maxret, ptrx, ptry, ptrz, vtxlist)
        struct windowstruct *wind;
        porttype *port;
        struct gp1_attr *attr;
        int n;
	int *maxret;
        pt_type *curpt;
        float *ptrx, *ptry, *ptrz;
	vtxtype *vtxlist;
{
        register short *shmptr;
        register int i, cnt, nblocks, offset;
	unsigned int bitvector;
        int maxvert, nbnds, nvert;
        struct gp1pr *prptr;
        float *xptr, *yptr, *zptr;
        int hue, hueshift, hueoffset;
        int *colorptr, *resptr;
	short *saveptr, *maxvertptr;
	short clipplanes;
	int intval;

    if (ptrz == 0) { /* 2d vectors */

        /* if too many vertices ... drop on the floor for now*/
        if (n > 180) {
		*maxret = -1;
		printf("Number of vertices %d exceeded GP limit of 180\n",n);
        	return;
	}

	maxvert =  *maxret;
	nblocks = ( 9 + 6*n + 6*maxvert )/512 + 1;
	if (nblocks > 8) {
		nblocks = 8;
		maxvert = (8 * 512 - 9 - 6*n)/6;
	}
 
        prptr = gp1_d(wind->pixwin->pw_pixrect);
 
Restart2:
        xptr = ptrx;
        yptr = ptry;
              
	resptr = (int *)vtxlist;
	cnt = attr->resetcnt;
        if (attr->needreset)
        	_core_gp1_reset(wind, attr, port);
        if (attr->xfrmtype != XFORMTYPE_WLDNDC)
        	_core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_WLDNDC);
	clipplanes = attr->curclipplanes & 0x3C;
 
        while ((offset = gp1_alloc(prptr->gp_shmem, nblocks, &bitvector,
                prptr->minordev, prptr->ioctl_fd)) == 0)
            ;
 
 
        saveptr = shmptr = &((short *)prptr->gp_shmem)[offset];
        *shmptr++ = GP1_USEFRAME | attr->statblkindx;
        *shmptr++ = GP1_SETCLIPPLANES | clipplanes;
        *shmptr++ = GP1_COREWLDPOLYNDC_3D | GP1_SHADE_CONSTANT;
        *shmptr++ = 1; /* number of bounds ... always 1 in suncore */
        *shmptr++ = n; /* number of vertices */
        for (i=0; i<n; i++) {
		CORE_GP_PUT_FLOAT(shmptr, xptr);
		xptr++;
		CORE_GP_PUT_FLOAT(shmptr, yptr);
		yptr++;
		CORE_GP_PUT_FLOAT(shmptr, &curpt->z);
        }
	maxvertptr = shmptr;
	*shmptr++ = maxvert*6 + 2;
        *shmptr++ = GP1_EOCL;
        if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
                while (cnt == attr->resetcnt)
                    ;
            goto Restart2;
        }

	if (_core_gp1_chkloc(maxvertptr, prptr->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		goto Restart2;
	}

	nbnds = *shmptr++;
	if ((nbnds < 0) || (nbnds > 1))	{	/* set error and  return */
		*maxret = -1;
		goto getout2;
	} else if (nbnds == 0) {
		*maxret = 0; /* entirely clipped away */
		goto getout2;
	}

	nvert = *shmptr++;
	for (i=0; i<nvert; i++) {
		CORE_GP_GET_INT(shmptr, resptr);
		resptr++;
		CORE_GP_GET_INT(shmptr, resptr);
		resptr++;
		CORE_GP_GET_INT(shmptr, resptr);
		resptr++;
		resptr += 5;
	}
	
	*maxret = nvert;
getout2:
	shmptr = saveptr; /* clean up and get out */
        *shmptr++ = GP1_USEFRAME | attr->statblkindx;
        *shmptr++ = GP1_SETCLIPPLANES | attr->curclipplanes;
	*shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	CORE_GP_PUT_INT(shmptr, &bitvector);
	if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		goto Restart2;
	}

    } else {	/* 3d polygons */

        /* if too many vertices ... drop on the floor for now*/
        if (n > 180) {
		*maxret = -1;
		printf("Number of vertices %d exceeded GP limit of 180\n",n);
                return;
	}

        maxvert =  *maxret;
        nblocks = ( 8 + 8*n + 8*maxvert )/512 + 1;
        if (nblocks > 8) {
                nblocks = 8;
                maxvert = (8 * 512 - 8 - 8*n)/8;
        }

        if ( (hue = _core_shading_parameters.hue) == 0) {
                hueshift = 9;
                hueoffset = 0;
        } else if (hue == 1) {
                hueshift = 7;
                hueoffset = 1<<16;
        } else {
                hueshift = 7;
                hueoffset = (hue -1) << 22;
        }

        prptr = gp1_d(wind->pixwin->pw_pixrect);

Restart3:
	    xptr = ptrx;
	    yptr = ptry;
	    zptr = ptrz;
	    colorptr = &_core_vtxlist[0].dx;
	    resptr = (int *)vtxlist;

            cnt = attr->resetcnt;
            if (attr->needreset)
                _core_gp1_reset(wind, attr, port);
            if (attr->xfrmtype != XFORMTYPE_WLDNDC)
                _core_gp1_snd_xfrm_attr(wind->pixwin, attr, XFORMTYPE_WLDNDC);

            while ((offset = gp1_alloc(prptr->gp_shmem, nblocks, &bitvector,
                        prptr->minordev, prptr->ioctl_fd)) == 0)
                ;
 
            saveptr = shmptr = &((short *)prptr->gp_shmem)[offset];
            *shmptr++ = GP1_USEFRAME | attr->statblkindx;

	   /* always do gouraud, in case user needs clipped intensities later */
            *shmptr++ = GP1_COREWLDPOLYNDC_3D | GP1_SHADE_GOURAUD;
            *shmptr++ = 1; /* number of bounds, always 1 in suncore */
            *shmptr++ = n; /* number of vertices coming in */
 
            for (i=0; i<n; i++) {
		    CORE_GP_PUT_FLOAT(shmptr, xptr);
		    xptr++;
		    CORE_GP_PUT_FLOAT(shmptr, yptr);
		    yptr++;
		    CORE_GP_PUT_FLOAT(shmptr, zptr);
		    zptr++;
                    intval = hueoffset + ((*colorptr)<<hueshift);
		    CORE_GP_PUT_INT(shmptr, &intval);
		    colorptr += 8;
            }

	    maxvertptr = shmptr;
	    *shmptr++ = maxvert * 8 + 2;
            *shmptr++ = GP1_EOCL ;
            if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
                while (cnt == attr->resetcnt)
                        ;
                goto Restart3;
            }

	    if (_core_gp1_chkloc(maxvertptr, prptr->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		goto Restart3;
	    }

            nbnds = *shmptr++;
            if ((nbnds < 0) || (nbnds > 1)) {       /* set error and  return */
                *maxret = -1;
                goto getout3;
            } else if (nbnds == 0) {
                *maxret = 0; /* entirely clipped away */
                goto getout3;
            }

            nvert = *shmptr++;
            for (i=0; i<nvert; i++) {
		CORE_GP_GET_INT(shmptr, resptr);
		resptr++;
		CORE_GP_GET_INT(shmptr, resptr);
		resptr++;
		CORE_GP_GET_INT(shmptr, resptr);
		resptr++;
		resptr++;
		CORE_GP_GET_INT(shmptr, resptr);
		*resptr = (*resptr - hueoffset)>>hueshift;
		resptr += 4;				/* skip dy,dz,dw */
	    }
	    *maxret = nvert;
getout3:
	    shmptr = saveptr;
	    *shmptr++ = GP1_EOCL | GP1_FREEBLKS;
	    CORE_GP_PUT_INT(shmptr, &bitvector);
	    if (gp1_post(prptr->gp_shmem, offset, prptr->ioctl_fd)) {
		while (cnt == attr->resetcnt)
			;
		goto Restart3;
	    }
    }
}
