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
static char sccsid[] = "@(#)inquirypas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int inquire_inverse_composite_matrix();
int inquire_ndc_space_2();
int inquire_ndc_space_3();
int inquire_open_retained_segment();
int inquire_open_temporary_segment();
int inquire_projection();
int inquire_retained_segment_names();
int inquire_retained_segment_surfaces();
int inquire_view_depth();
int inquire_view_plane_distance();
int inquire_view_plane_normal();
int inquire_view_reference_point();
int inquire_view_up_2();
int inquire_view_up_3();
int inquire_viewing_control_parameters();
int inquire_viewing_parameters();
int inquire_viewport_2();
int inquire_viewport_3();
int inquire_window();
int inquire_world_coordinate_matrix_2();
int inquire_world_coordinate_matrix_3();

int inqinvcompmatrix(arrayptr)
double arrayptr[];
	{
	int i, f;
	float dummy[16], *dptr;

	f = inquire_inverse_composite_matrix(dummy);
	for (i = 0; i < 4; i++)
		{
		dptr = dummy + i;
		*arrayptr++ = *dptr;
		dptr += 4;
		*arrayptr++ = *dptr;
		dptr += 4;
		*arrayptr++ = *dptr;
		dptr += 4;
		*arrayptr++ = *dptr;
		}
	return(f);
	}

int inqndcspace2(width, height)
double *width, *height;
	{
	float pwidth, pheight;
	int f;
	f=inquire_ndc_space_2(&pwidth, &pheight);
 	*width = pwidth; 
	*height = pheight;
	return(f);
	}

int inqndcspace3(width, height, depth)
double *width, *height, *depth;
	{
	float pwidth, pheight, pdepth;
	int f;
	f=inquire_ndc_space_3(&pwidth, &pheight, &pdepth);
 	*width = pwidth; 
	*height = pheight;
	*depth = pdepth;
	return(f);
	}

int inqopenretainseg(segname)
int *segname;
	{
	return(inquire_open_retained_segment(segname));
	}

int inqopentempseg(open)
int *open;
	{
	return(inquire_open_temporary_segment(open));
	}

int inqprojection(projection_type, dx_proj, dy_proj, dz_proj)
int *projection_type;
double *dx_proj, *dy_proj, *dz_proj;
	{
	float tx,ty,tz;
	int f;
	f=inquire_projection(projection_type, &tx, &ty, &tz);
	*dx_proj=tx;
	*dy_proj=ty;
	*dz_proj=tz;
	return(f);
	}

int inqretainsegname(listcnt, seglist, segcnt)
int listcnt, seglist[], *segcnt;
	{
	return(inquire_retained_segment_names(listcnt, seglist, segcnt));
	}

int inqretainsegsurf(segname, arraycnt, surfaray, surfnum)
int segname, arraycnt, *surfnum;
struct vwsurf surfaray[];
	{
	return(inquire_retained_segment_surfaces(segname, arraycnt,
		surfaray, surfnum));
	}

int inqviewdepth(front_distance, back_distance)
double *front_distance, *back_distance;
	{
	float tx,ty;
	int f;
	f=inquire_view_depth(&tx, &ty);
	*front_distance=tx;
	*back_distance=ty;
	return(f);
	}
	
int inqviewplanedist(view_distance)
double *view_distance;
	{
	float tx;
	int f;
	f=inquire_view_plane_distance(&tx);
	*view_distance=tx;
	return(f);
	}

int inqviewplanenorm(dx_norm, dy_norm, dz_norm)
double *dx_norm, *dy_norm, *dz_norm;
	{
	float tx,ty,tz;
	int f;
	f=inquire_view_plane_normal(&tx, &ty, &tz);
	*dx_norm=tx;
	*dy_norm=ty;
	*dz_norm=tz;
	return(f);
	}

int inqviewrefpoint(x_ref, y_ref, z_ref)
double *x_ref, *y_ref, *z_ref;
	{
	float tx,ty,tz;
	int f;
	f=inquire_view_reference_point(&tx, &ty, &tz);
	*x_ref=tx;
	*y_ref=ty;
	*z_ref=tz;
	return(f);
	}

int inqviewup2(dx_up, dy_up)
double *dx_up, *dy_up;
	{
	float tx,ty;
	int f;
	f=inquire_view_up_2(&tx, &ty);
	*dx_up=tx;
	*dy_up=ty;
	return(f);
	}

int inqviewup3(dx_up, dy_up, dz_up)
double *dx_up, *dy_up, *dz_up;
	{
	float tx,ty,tz;
	int f;
	f=inquire_view_up_3(&tx, &ty, &tz);
	*dx_up=tx;
	*dy_up=ty;
	*dz_up=tz;
	return(f);
	}

int inqvwgcntrlparms(windowclip,frontclip, backclip, type)
int *windowclip, *frontclip, *backclip, *type;
	{
	return(inquire_viewing_control_parameters(windowclip, frontclip,
		backclip, type));
	}

typedef struct {
	double p_xmin, p_xmax, p_ymin, p_ymax; } p_windtype;
typedef struct {
	double p_xmin, p_xmax, p_ymin, p_ymax, p_zmin, p_zmax; } p_porttype;

typedef struct {
      double p_vwrefpt[3];
      double p_vwplnorm[3];
      double p_viewdis;
      double p_frontdis;
      double p_backdis;
      int p_projtype;
      double p_projdir[3];
      p_windtype p_window;
      double p_vwupdir[3];
      p_porttype p_viewport;
      } p_vwprmtype;

typedef struct {
	float xmin, xmax, ymin, ymax; } windtype;
typedef struct {
	float xmin,xmax,ymin,ymax,zmin,zmax; } porttype;

typedef struct {
      float vwrefpt[3];
      float vwplnorm[3];
      float viewdis;
      float frontdis;
      float backdis;
      int projtype;
      float projdir[3];
      windtype window;
      float vwupdir[3];
      porttype viewport;
      } vwprmtype;

int inqviewingparams(viewparm2)
p_vwprmtype *viewparm2;
	{
	vwprmtype viewparm;
		int f;
		f=inquire_viewing_parameters(&viewparm);
		viewparm2->p_vwrefpt[1] = viewparm.vwrefpt[1];
		viewparm2->p_vwrefpt[2] = viewparm.vwrefpt[2];
		viewparm2->p_vwrefpt[3] = viewparm.vwrefpt[3];
		viewparm2->p_vwplnorm[1] = viewparm.vwplnorm[1];
		viewparm2->p_vwplnorm[2] = viewparm.vwplnorm[2];
		viewparm2->p_vwplnorm[3] = viewparm.vwplnorm[3];
		viewparm2->p_viewdis = viewparm.viewdis;
		viewparm2->p_frontdis = viewparm.frontdis;
		viewparm2->p_backdis = viewparm.backdis;
		viewparm2->p_projtype = viewparm.projtype;
		viewparm2->p_projdir[1] = viewparm.projdir[1];
		viewparm2->p_projdir[2] = viewparm.projdir[2];
		viewparm2->p_projdir[3] = viewparm.projdir[3];
		viewparm2->p_window.p_xmin = viewparm.window.xmin;
		viewparm2->p_window.p_xmax = viewparm.window.xmax;
		viewparm2->p_window.p_ymin = viewparm.window.ymin;
		viewparm2->p_window.p_ymax = viewparm.window.ymax;
		viewparm2->p_vwupdir[1] = viewparm.vwupdir[1];
		viewparm2->p_vwupdir[2] = viewparm.vwupdir[2];
		viewparm2->p_vwupdir[3] = viewparm.vwupdir[3];
		viewparm2->p_viewport.p_xmin = viewparm.viewport.xmin;
		viewparm2->p_viewport.p_xmin = viewparm.viewport.xmax;
		viewparm2->p_viewport.p_ymin = viewparm.viewport.ymin;
		viewparm2->p_viewport.p_ymax = viewparm.viewport.ymax;
		viewparm2->p_viewport.p_zmin = viewparm.viewport.zmin;
		viewparm2->p_viewport.p_zmax = viewparm.viewport.zmax;
		return(f);
	}

int inqviewport2(xmin, xmax, ymin, ymax)
double *xmin, *xmax, *ymin, *ymax;
	{
	float xminf, xmaxf, yminf, ymaxf;
	int f;
	f=inquire_viewport_2(&xminf, &xmaxf, &yminf, &ymaxf);
	*xmin = xminf;
	*xmax = xmaxf;
	*ymin = yminf;
	*ymax = ymaxf;
	return(f);
	}

int inqviewport3(xmin, xmax, ymin, ymax, zmin, zmax)
double *xmin, *xmax, *ymin, *ymax, *zmin, *zmax;
	{
	float xminf, xmaxf, yminf, ymaxf, zminf, zmaxf;
	int f;
	f=inquire_viewport_3(&xminf, &xmaxf, &yminf, &ymaxf, &zminf, &zmaxf);
	*xmin = xminf;
	*xmax = xmaxf;
	*ymin = yminf;
	*ymax = ymaxf;
	*zmin = zminf;
	*zmax = zmaxf;
	return(f);
	}

int inqwindow(umin, umax, vmin, vmax)
double *umin, *umax, *vmin, *vmax;
	{
	float uminf, umaxf, vminf, vmaxf;
	int f;
	f=inquire_window(&uminf, &umaxf, &vminf, &vmaxf);
	*umin = uminf;
	*umax = umaxf;
	*vmin = vminf;
	*vmax = vmaxf;
	return(f);
	}

int inqworldmatrix2(wmat)
double *wmat;
	{
	int i, f;
	float dummy[9], *dptr;

	f = inquire_world_coordinate_matrix_2(dummy);
	for (i = 0; i < 3; i++)
		{
		dptr = dummy + i;
		*wmat++ = *dptr;
		dptr += 3;
		*wmat++ = *dptr;
		dptr += 3;
		*wmat++ = *dptr;
		}
	return(f);
	}

int inqworldmatrix3(wmat)
double *wmat;
	{
	int i, f;
	float dummy[16], *dptr;

	f = inquire_world_coordinate_matrix_3(dummy);
	for (i = 0; i < 4; i++)
		{
		dptr = dummy + i;
		*wmat++ = *dptr;
		dptr += 4;
		*wmat++ = *dptr;
		dptr += 4;
		*wmat++ = *dptr;
		dptr += 4;
		*wmat++ = *dptr;
		}
	return(f);
	}

int pasloc(surfname)
int (*surfname)();
	{
	return((int)surfname);
	}
