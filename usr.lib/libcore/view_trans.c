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
static char sccsid[] = "@(#)view_trans.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_output_clipping                                        */
/*                                                                          */
/*     PURPOSE: ENABLE OR DISABLE CLIPPING AGAINST THE WINDOW IN THE VIEW   */
/*              PLANE AFTER IMAGE TRANSFORM STAGE.                          */
/*                                                                          */
/****************************************************************************/

set_output_clipping(onoff) int onoff;
{
    viewsurf *sp;
    ddargtype ddstruct;

   _core_outpclip = onoff;
    for (sp = &_core_surface[0]; sp < &_core_surface[MAXVSURF]; sp++) {
	if (!sp->vinit)
		continue;
	if (sp->hphardwr) {
		ddstruct.opcode = OUTPUTCLIP;
		ddstruct.instance = sp->vsurf.instance;
		ddstruct.int1 = onoff;
		(* sp->vsurf.dd)(&ddstruct);
	}
   }
   return(0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_window_clipping                                        */
/*                                                                          */
/*     PURPOSE: ENABLE OR DISABLE CLIPPING AGAINST THE WORLD COORDINATE     */
/*              VIEW WINDOW                                                 */
/*                                                                          */
/****************************************************************************/

set_window_clipping(onoff) int onoff;
{
   if (_core_osexists)
	{
	_core_errhand("set_window_clipping", 8);
	return(1);
	}
   _core_wndwclip = onoff;
   if (onoff)
	_core_wclipplanes |= 0xF;
   else
	_core_wclipplanes &= (~0xF);
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_front_plane_clipping                                   */
/*                                                                          */
/*     PURPOSE: ENABLE OR DISABLE CLIPPING AGAINST THE NEAR CLIPPING        */
/*              PLANE.                                                      */
/*                                                                          */
/****************************************************************************/

set_front_plane_clipping(onoff) int onoff;
{
   if (_core_osexists)
	{
	_core_errhand("set_front_plane_clipping", 8);
	return(1);
	}
   _core_frontclip = onoff;
   if (onoff)
	_core_wclipplanes |= 0x10;
   else
	_core_wclipplanes &= (~0x10);
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_back_plane_clipping                                    */
/*                                                                          */
/*     PURPOSE: ENABLE OR DISABLE CLIPPING AGAINST THE FAR CLIPPING         */
/*              PLANE.                                                      */
/*                                                                          */
/****************************************************************************/

set_back_plane_clipping(onoff) int onoff;
{
   if (_core_osexists)
	{
	_core_errhand("set_back_plane_clipping", 8);
	return(1);
	}
   _core_wndwclip = onoff;
   if (onoff)
	_core_wclipplanes |= 0x20;
   else
	_core_wclipplanes &= (~0x20);
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_coordinate_system_type                                 */
/*                                                                          */
/*     PURPOSE: THE PARAMETER 'type' SELECTS WHETHER THE WORLD COORDINATE   */
/*              SYSTEM IS RIGHT-HANDED OR LEFT-HANDED.                      */
/*                                                                          */
/****************************************************************************/

set_coordinate_system_type(type) int type;
{
   char *funcname;
   int errnum;

   funcname = "set_coordinate_system_type";

   if(_core_corsyset) {  /*** FUNCTION ALREADY BEEN CALLED? ***/
      errnum = 11;
      _core_errhand(funcname,errnum);
      return(1);
      }
   if(_core_vfinvokd || _core_prevseg)
      {/*** VIEW FUNCTION ALREADY CALLED OR SEGMENT PREVIOUSLY CREATED ? ***/
      errnum = 19;
      _core_errhand(funcname,errnum);
      return(2);
      }
   if(type != LEFT && type != RIGHT)   /*** PROPER TYPE ? ***/
      {
      errnum = 20;
      _core_errhand(funcname,errnum);
      return(3);
      }

   _core_coordsys = type;

   _core_corsyset = TRUE;
   return(0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_world_coordinate_matrix_2                              */
/*                                                                          */
/*     PURPOSE:  Specify the 'world' or 'modelling' transform   	    */
/*                                                                          */
/****************************************************************************/

set_world_coordinate_matrix_2( array) float *array;
{
    _core_identity(&_core_modxform[0][0]);
    _core_modxform[0][0] = *array++; _core_modxform[0][1] = *array++;
    array++;
    _core_modxform[1][0] = *array++; _core_modxform[1][1] = *array++;
    array++;
    _core_modxform[3][0] = *array++; _core_modxform[3][1] = *array++;
    if (_core_osexists)
	_core_validvt(FALSE);
    else
	_core_vtchang = TRUE;
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_world_coordinate_matrix_3                              */
/*                                                                          */
/*     PURPOSE:  Specify the 'world' or 'modelling' transform   	    */
/*                                                                          */
/****************************************************************************/

set_world_coordinate_matrix_3( array) float *array;
{
    _core_identity(&_core_modxform[0][0]);
    _core_modxform[0][0] = *array++; _core_modxform[0][1] = *array++;
    _core_modxform[0][2] = *array++; array++;
    _core_modxform[1][0] = *array++; _core_modxform[1][1] = *array++;
    _core_modxform[1][2] = *array++; array++;
    _core_modxform[2][0] = *array++; _core_modxform[2][1] = *array++;
    _core_modxform[2][2] = *array++; array++;
    _core_modxform[3][0] = *array++; _core_modxform[3][1] = *array++;
    _core_modxform[3][2] = *array++;
    if (_core_osexists)
	_core_validvt(FALSE);
    else
	_core_vtchang = TRUE;
}
