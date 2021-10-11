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
static char sccsid[] = "@(#)xformattpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int set_ndc_space_2();
int set_ndc_space_3();
int set_projection();
int set_view_depth();
int set_view_plane_distance();
int set_view_plane_normal();
int set_view_reference_point();
int set_view_up_2();
int set_view_up_3();
int set_viewing_parameters();
int set_viewport_2();
int set_viewport_3();
int set_window();

int setndcspace2(width, height)
double width, height;
	{
	return(set_ndc_space_2(width, height));
	}

int setndcspace3(width, height, depth)
double width, height, depth;
	{
	return(set_ndc_space_3(width, height, depth));
	}

int setprojection(projtype, dx, dy, dz)
int projtype;
double dx, dy, dz;
	{
	return(set_projection(projtype, dx, dy, dz));
	}

int setviewdepth(near, far)
double near, far;
	{
	return(set_view_depth(near, far));
	}

int setviewplanedist(dist)
double dist;
	{
	return(set_view_plane_distance(dist));
	}

int setviewplanenorm(dx, dy, dz)
double dx, dy, dz;
	{
	return(set_view_plane_normal(dx, dy, dz));
	}

int setviewrefpoint(x, y, z)
double x, y, z;
	{
	return(set_view_reference_point(x, y, z));
	}

int setviewup2(dx, dy)
double dx, dy;
	{
	return(set_view_up_2(dx, dy));
	}

int setviewup3(dx, dy, dz)
double dx, dy, dz;
	{
	return(set_view_up_3(dx, dy, dz));
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

int setviewingparams(viewparm2)
p_vwprmtype *viewparm2;
	{
	vwprmtype *viewparm;
		viewparm->vwrefpt[1] = viewparm2->p_vwrefpt[1];
		viewparm->vwrefpt[2] = viewparm2->p_vwrefpt[2];
		viewparm->vwrefpt[3] = viewparm2->p_vwrefpt[3];
		viewparm->vwplnorm[1] = viewparm2->p_vwplnorm[1];
		viewparm->vwplnorm[2] = viewparm2->p_vwplnorm[2];
		viewparm->vwplnorm[3] = viewparm2->p_vwplnorm[3];
		viewparm->viewdis = viewparm2->p_viewdis;
		viewparm->frontdis = viewparm2->p_frontdis;
		viewparm->backdis = viewparm2->p_backdis;
		viewparm->projtype = viewparm2->p_projtype;
		viewparm->projdir[1] = viewparm2->p_projdir[1];
		viewparm->projdir[2] = viewparm2->p_projdir[2];
		viewparm->projdir[3] = viewparm2->p_projdir[3];
		viewparm->window.xmin = viewparm2->p_window.p_xmin;
		viewparm->window.xmax = viewparm2->p_window.p_xmax;
		viewparm->window.ymin = viewparm2->p_window.p_ymin;
		viewparm->window.ymax = viewparm2->p_window.p_ymax;
		viewparm->vwupdir[1] = viewparm2->p_vwupdir[1];
		viewparm->vwupdir[2] = viewparm2->p_vwupdir[2];
		viewparm->vwupdir[3] = viewparm2->p_vwupdir[3];
		viewparm->viewport.xmin = viewparm2->p_viewport.p_xmin;
		viewparm->viewport.xmax = viewparm2->p_viewport.p_xmin;
		viewparm->viewport.ymin = viewparm2->p_viewport.p_ymin;
		viewparm->viewport.ymax = viewparm2->p_viewport.p_ymax;
		viewparm->viewport.zmin = viewparm2->p_viewport.p_zmin;
		viewparm->viewport.zmax = viewparm2->p_viewport.p_zmax;
	return(set_viewing_parameters(viewparm));
	}

int setviewport2(xmin, xmax, ymin, ymax)
double xmin, xmax, ymin, ymax; 
	{
	return(set_viewport_2(xmin, xmax, ymin, ymax));
	}

int setviewport3(xmin, xmax, ymin, ymax, zmin, zmax)
double xmin, xmax, ymin, ymax, zmin, zmax;
	{
	return(set_viewport_3(xmin, xmax, ymin, ymax, zmin, zmax));
	}

int setwindow(umin, umax, vmin, vmax)
double umin, umax, vmin, vmax;
	{
	return(set_window(umin, umax, vmin, vmax));
	}
