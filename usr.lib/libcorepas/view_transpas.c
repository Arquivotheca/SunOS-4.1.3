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
static char sccsid[] = "@(#)view_transpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int set_back_plane_clipping();
int set_coordinate_system_type();
int set_front_plane_clipping();
int set_output_clipping();
int set_window_clipping();
int set_world_coordinate_matrix_2();
int set_world_coordinate_matrix_3();

int setbackclip(onoff)
int onoff;
	{
	return(set_back_plane_clipping(onoff));
	}

int setcoordsystype(type)
int type;
	{
	return(set_coordinate_system_type(type));
	}

int setfrontclip(onoff)
int onoff;
	{
	return(set_front_plane_clipping(onoff));
	}

int setoutputclip(onoff)
int onoff;
	{
	return(set_output_clipping(onoff));
	}

int setwindowclip(onoff)
int onoff;
	{
	return(set_window_clipping(onoff));
	}

int setworldmatrix2(f77array)
double *f77array;
	{
	int i;
	float dummy[9], *dptr, *fptr,dummy2[9];
	for (i = 0; i < 9; ++i) {
		dummy2[i] = f77array[i];
	}
	dptr = dummy;
	for (i = 0; i < 3; i++)
		{
		fptr = dummy2 + i;
		*dptr++ = *fptr;
		fptr += 3;
		*dptr++ = *fptr;
		fptr += 3;
		*dptr++ = *fptr;
		}
	return(set_world_coordinate_matrix_2(dummy));
	}

int setworldmatrix3(f77array)
double *f77array;
	{
	int i;
	float dummy[16], *dptr, *fptr,dummy2[16];
	for (i = 0; i < 16; ++i) {
		dummy2[i] = f77array[i];
	}
	dptr = dummy;
	for (i = 0; i < 4; i++)
		{
		fptr = dummy2 + i;
		*dptr++ = *fptr;
		fptr += 4;
		*dptr++ = *fptr;
		fptr += 4;
		*dptr++ = *fptr;
		fptr += 4;
		*dptr++ = *fptr;
		}
	return(set_world_coordinate_matrix_3(dummy));
	}
