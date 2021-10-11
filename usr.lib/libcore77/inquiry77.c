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
static char sccsid[] = "@(#)inquiry77.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

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

int inqinvcompmatrix_(arrayptr)
    float *arrayptr;
{
    return(inqinvcompmatrix(arrayptr));
}
    
int inqinvcompmatrix(arrayptr)
float *arrayptr;
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

int inqndcspace2_(width, height)
float *width, *height;
	{
	return(inquire_ndc_space_2(width, height));
	}

int inqndcspace3_(width, height, depth)
float *width, *height, *depth;
	{
	return(inquire_ndc_space_3(width, height, depth));
	}

int inqopenretainseg_(segname)
int *segname;
	{
	return(inquire_open_retained_segment(segname));
	}

int inqopenretainseg(segname)
int *segname;
	{
	return(inquire_open_retained_segment(segname));
	}

int inqopentempseg_(open)
int *open;
	{
	return(inquire_open_temporary_segment(open));
	}

int inqprojection_(projection_type, dx_proj, dy_proj, dz_proj)
int *projection_type;
float *dx_proj, dy_proj, dz_proj;
	{
	return(inquire_projection(projection_type, dx_proj, dy_proj, dz_proj));
	}

int inqretainsegname_(listcnt, seglist, segcnt)
int *listcnt, seglist[], *segcnt;
	{
	return(inquire_retained_segment_names(*listcnt, seglist, segcnt));
	}

int inqretainsegname(listcnt, seglist, segcnt)
int *listcnt, seglist[], *segcnt;
	{
	return(inquire_retained_segment_names(*listcnt, seglist, segcnt));
	}

#define DEVNAMESIZE 20

struct vwsurf	{
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char **ptr;
		};

int inqretainsegsurf_(segname, arraycnt, surfaray, surfnum)
int *segname, *arraycnt, *surfnum;
struct vwsurf surfaray[];
	{
	return(inquire_retained_segment_surfaces(*segname, *arraycnt,
		surfaray, surfnum));
	}

int inqretainsegsurf(segname, arraycnt, surfaray, surfnum)
int *segname, *arraycnt, *surfnum;
struct vwsurf surfaray[];
	{
	return(inquire_retained_segment_surfaces(*segname, *arraycnt,
		surfaray, surfnum));
	}

int inqviewdepth_(front_distance, back_distance)
float *front_distance, *back_distance;
	{
	return(inquire_view_depth(front_distance, back_distance));
	}

int inqviewplanedist_(view_distance)
float *view_distance;
	{
	return(inquire_view_plane_distance(view_distance));
	}
	
int inqviewplanedist(view_distance)
float *view_distance;
	{
	return(inquire_view_plane_distance(view_distance));
	}

int inqviewplanenorm_(dx_norm, dy_norm, dz_norm)
float *dx_norm, *dy_norm, *dz_norm;
	{
	return(inquire_view_plane_normal(dx_norm, dy_norm, dz_norm));
	}

int inqviewplanenorm(dx_norm, dy_norm, dz_norm)
float *dx_norm, *dy_norm, *dz_norm;
	{
	return(inquire_view_plane_normal(dx_norm, dy_norm, dz_norm));
	}

int inqviewrefpoint_(x_ref, y_ref, z_ref)
float *x_ref, *y_ref, *z_ref;
	{
	return(inquire_view_reference_point(x_ref, y_ref, z_ref));
	}

int inqviewup2_(dx_up, dy_up)
float *dx_up, *dy_up;
	{
	return(inquire_view_up_2(dx_up, dy_up));
	}

int inqviewup3_(dx_up, dy_up, dz_up)
float *dx_up, *dy_up, *dz_up;
	{
	return(inquire_view_up_3(dx_up, dy_up, dz_up));
	}

int inqvwgcntrlparms_(windowclip,frontclip, backclip, type)
int *windowclip, *frontclip, *backclip, *type;
	{
	return(inquire_viewing_control_parameters(windowclip, frontclip,
		backclip, type));
	}

int inqvwgcntrlparms(windowclip,frontclip, backclip, type)
int *windowclip, *frontclip, *backclip, *type;
	{
	return(inquire_viewing_control_parameters(windowclip, frontclip,
		backclip, type));
	}

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

int inqviewingparams_(viewparm)
vwprmtype *viewparm;
	{
	return(inquire_viewing_parameters(viewparm));
	}

int inqviewingparams(viewparm)
vwprmtype *viewparm;
	{
	return(inquire_viewing_parameters(viewparm));
	}

int inqviewport2_(xmin, xmax, ymin, ymax)
float *xmin, *xmax, *ymin, *ymax;
	{
	return(inquire_viewport_2(xmin, xmax, ymin, ymax));
	}

int inqviewport3_(xmin, xmax, ymin, ymax, zmin, zmax)
float *xmin, *xmax, *ymin, *ymax, *zmin, *zmax;
	{
	return(inquire_viewport_3(xmin, xmax, ymin, ymax, zmin, zmax));
	}

int inqwindow_(umin, umax, vmin, vmax)
float *umin, *umax, *vmin, *vmax;
	{
	return(inquire_window(umin, umax, vmin, vmax));
	}

int inqworldmatrix2_(wmat)
float *wmat;
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

int inqworldmatrix3_(wmat)
float *wmat;
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
