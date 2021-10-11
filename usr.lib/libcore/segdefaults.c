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
static char sccsid[] = "@(#)segdefaults.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/*--------------------------------------------------------------------*/
set_visibility(visibility)
int visibility;
   {
   if(sattrck(visibility,"set_visibility") == 3) return(3);
   _core_defsegat.visbilty = visibility;
   return(0);
   }

/*--------------------------------------------------------------------*/
set_highlighting(highlighting)
int highlighting;
   {
   if(sattrck(highlighting,"set_highlighting") == 3) return(3);
   _core_defsegat.highlght = highlighting;
   return(0);
   }

/*--------------------------------------------------------------------*/
set_detectability(detectability)
int detectability;
   {
   if (detectability < 0)
	{
	_core_errhand("set_detectability",13);
	return(3);
	}
   _core_defsegat.detectbl = detectability;
   return(0);
   }

/*--------------------------------------------------------------------*/
static identityimxform()
{
	_core_defsegat.scale[0] = 1.0;
	_core_defsegat.scale[1] = 1.0;
	_core_defsegat.scale[2] = 1.0;
	_core_defsegat.rotate[0] = 0.0;
	_core_defsegat.rotate[1] = 0.0;
	_core_defsegat.rotate[2] = 0.0;
	_core_defsegat.translat[0] = 0.0;
	_core_defsegat.translat[1] = 0.0;
	_core_defsegat.translat[2] = 0.0;
	}
/*--------------------------------------------------------------------*/
set_image_translate_2(tx, ty)
float tx, ty;
	{
	identityimxform();
	_core_defsegat.translat[0] = tx * (float) MAX_NDC_COORD;
	_core_defsegat.translat[1] = ty * (float) MAX_NDC_COORD;
	return(0);
	}

/*--------------------------------------------------------------------*/
set_image_translate_3(tx, ty, tz)
float tx, ty, tz;
	{
	identityimxform();
	_core_defsegat.translat[0] = tx * (float) MAX_NDC_COORD;
	_core_defsegat.translat[1] = ty * (float) MAX_NDC_COORD;
	_core_defsegat.translat[2] = tz * (float) MAX_NDC_COORD;
	return(0);
	}

/*--------------------------------------------------------------------*/
set_image_transformation_2(sx, sy, a, tx, ty)
float sx, sy, a, tx, ty;
	{
	identityimxform();
	_core_defsegat.scale[0] = sx;
	_core_defsegat.scale[1] = sy;
	_core_defsegat.rotate[2] = a;
	_core_defsegat.translat[0] = tx * (float) MAX_NDC_COORD;
	_core_defsegat.translat[1] = ty * (float) MAX_NDC_COORD;
	return(0);
	}

/*--------------------------------------------------------------------*/
set_image_transformation_3(sx, sy, sz, ax, ay, az, tx, ty, tz)
float sx, sy, sz, ax, ay, az, tx, ty, tz;
	{
	_core_defsegat.scale[0] = sx;
	_core_defsegat.scale[1] = sy;
	_core_defsegat.scale[2] = sz;
	_core_defsegat.rotate[0] = ax;
	_core_defsegat.rotate[1] = ay;
	_core_defsegat.rotate[2] = az;
	_core_defsegat.translat[0] = tx * (float) MAX_NDC_COORD;
	_core_defsegat.translat[1] = ty * (float) MAX_NDC_COORD;
	_core_defsegat.translat[2] = tz * (float) MAX_NDC_COORD;
	return(0);
	}

/*--------------------------------------------------------------------*/
inquire_visibility(visibility)
int *visibility;
   {
   *visibility = _core_defsegat.visbilty;
   }

/*--------------------------------------------------------------------*/
inquire_highlighting(highlighting)
int *highlighting;
   {
   *highlighting = _core_defsegat.highlght;
   }

/*--------------------------------------------------------------------*/
inquire_detectability(detectability)
int *detectability;
   {
   *detectability = _core_defsegat.detectbl;
   }

/*--------------------------------------------------------------------*/
inquire_image_translate_2(tx, ty)
float *tx, *ty;
	{
	*tx = _core_defsegat.translat[0]/(float) MAX_NDC_COORD;
	*ty = _core_defsegat.translat[1]/(float) MAX_NDC_COORD;
	}

/*--------------------------------------------------------------------*/
inquire_image_translate_3(tx, ty, tz)
float *tx, *ty, *tz;
	{
	*tx = _core_defsegat.translat[0]/(float) MAX_NDC_COORD;
	*ty = _core_defsegat.translat[1]/(float) MAX_NDC_COORD;
	*tz = _core_defsegat.translat[2]/(float) MAX_NDC_COORD;
	}

/*--------------------------------------------------------------------*/
inquire_image_transformation_2(sx, sy, a, tx, ty)
float *sx, *sy, *a, *tx, *ty;
	{
	*sx = _core_defsegat.scale[0];
	*sy = _core_defsegat.scale[1];
	*a = _core_defsegat.rotate[2];
	*tx = _core_defsegat.translat[0]/(float) MAX_NDC_COORD;
	*ty = _core_defsegat.translat[1]/(float) MAX_NDC_COORD;
	}

/*--------------------------------------------------------------------*/
inquire_image_transformation_3(sx, sy, sz, ax, ay, az, tx, ty, tz)
float *sx, *sy, *sz, *ax, *ay, *az, *tx, *ty, *tz;
	{
	*sx = _core_defsegat.scale[0];
	*sy = _core_defsegat.scale[1];
	*sz = _core_defsegat.scale[2];
	*ax = _core_defsegat.rotate[0];
	*ay = _core_defsegat.rotate[1];
	*az = _core_defsegat.rotate[2];
	*tx = _core_defsegat.translat[0]/(float) MAX_NDC_COORD;
	*ty = _core_defsegat.translat[1]/(float) MAX_NDC_COORD;
	*tz = _core_defsegat.translat[2]/(float) MAX_NDC_COORD;
	}


/****************************************************************************/
/*     FUNCTION: sattrck                                                    */
/*                                                                          */
/*     PURPOSE: CHECK THE ATTRIBUTE VALUE PASSED TO SEE IF IT EQUALS EITHER */
/*              TRUE OR FALSE.                                              */
/****************************************************************************/

static int sattrck(value,funcname) int value; char *funcname;
{
   int errnum;

   if((value != TRUE) && (value != FALSE)) {
      errnum = 13;
      _core_errhand(funcname,errnum);
      return(3);
      }
   return(0);
}
