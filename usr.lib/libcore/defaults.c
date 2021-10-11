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
/*	defaults.c	1.1	92/07/30	*/
#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_primitive_attributes                                   */
/*                                                                          */
/*     PURPOSE: SET THE DEFAULT PRIMITIVE AND SEGMENT ATTRIBUTE VALUES      */
/*              TO THOSE VALUES CONTAINED IN THE SPECIFIED STRUCTURES.      */
/*                                                                          */
/****************************************************************************/

set_primitive_attributes(defprim,segatval)
    primattr *defprim; segattr *segatval;
{
   char *funcname;
   int errnum,i;

   funcname = "set_primitive_attributes";
   if(dfaltset)   /*** FUNCTION BEEN CALLED ALREADY? ***/
      {
      errnum = 11;
      errhand(funcname,errnum);
      return(1);
      }
   if(prevseg)   /*** SEGMENT BEEN PREVIOUSLY OPENED? ***/
      {
      errnum = 12;
      errhand(funcname,errnum);
      return(2);
      }
   /**************************************************/
   /*** CHECK FOR VALIDITY OF PRIMITIVE ATTRIBUTE  ***/
   /*** PARAMETERS PASSED.                         ***/
   /**************************************************/

   if(fldefck(defprim->colorindx,minimum.colorindx,
   maximum.colorindx,funcname) == 3) return(3);
   if(fldefck(defprim->linwidth,minimum.linwidth,maximum.linwidth,
   funcname) == 3) return(3);
   if(indefck(defprim->linestyl,minimum.linestyl,maximum.linestyl,
   funcname) == 3) return(3);
   if(indefck(defprim->font,minimum.font,maximum.font,funcname) == 3) return(3);
   if(fldefck(defprim->charsize.width,minimum.charsize.width,
   maximum.charsize.width,funcname) == 3) return(3);
   if(fldefck(defprim->charsize.height,minimum.charsize.height,
   maximum.charsize.height,funcname) == 3) return(3);

   if(fldefck(defprim->chrspace.x,minimum.chrspace.x,maximum.chrspace.x,
   funcname) == 3) return(3);
   if(fldefck(defprim->chrup.x,minimum.chrup.x,maximum.chrup.x,
   funcname) == 3) return(3);
   if(fldefck(defprim->chrup.y,minimum.chrup.y,maximum.chrup.y,
   funcname) == 3) return(3);
   if(fldefck(defprim->chrup.z,minimum.chrup.z,maximum.chrup.z,
   funcname) == 3) return(3);
   if(fldefck(defprim->chrpath.x,minimum.chrpath.x,maximum.chrpath.x,
   funcname) == 3) return(3);
   if(fldefck(defprim->chrpath.y,minimum.chrpath.y,maximum.chrpath.y,
   funcname) == 3) return(3);
   if(fldefck(defprim->chrpath.z,minimum.chrpath.z,maximum.chrpath.z,
   funcname) == 3) return(3);

   if(indefck(defprim->chqualty,minimum.chqualty,maximum.chqualty,
   funcname) == 3) return(3);
   if(indefck(defprim->pickid,minimum.pickid,maximum.pickid,
   funcname) == 3) return(3);
   if(indefck(defprim->marker,minimum.marker,maximum.marker,
   funcname) == 3) return(3);

   /**************************************************/
   /*** CHECK FOR VALIDITY OF SEGMENT ATTRIBUTE    ***/
   /*** PARAMETERS PASSED.                         ***/
   /**************************************************/

   if(sattrck(segatval->visbilty,funcname) == 3) return(3);
   if(sattrck(segatval->detectbl,funcname) == 3) return(3);
   if(sattrck(segatval->highlght,funcname) == 3) return(3);

   /**************************************************/
   /*** DEFAULT PRIMITIVE ATTRIBUTE TRANSFERS      ***/
   /**************************************************/

   defaultt.colorindx = defprim->colorindx;
   defaultt.linwidth = defprim->linwidth;
   defaultt.linestyl = defprim->linestyl;
   defaultt.font = defprim->font;
   defaultt.charsize = defprim->charsize;
   defaultt.chrspace = defprim->chrspace;
   defaultt.chrup = defprim->chrup;
   defaultt.chrpath = defprim->chrpath;
   defaultt.chqualty = defprim->chqualty;
   defaultt.pickid = defprim->pickid;
   defaultt.marker = defprim->marker;

   /******************************************/
   /*** SEGMENT ATTRIBUTE VALUES TRANSFERS ***/
   /******************************************/

   defsegat.visbilty = segatval->visbilty;
   defsegat.detectbl = segatval->detectbl;
   defsegat.highlght = segatval->highlght;
   defsegat.scale[0] = segatval->scale[0];
   defsegat.scale[1] = segatval->scale[1];
   defsegat.scale[2] = segatval->scale[2];
   defsegat.translat[0] = segatval->translat[0];
   defsegat.translat[1] = segatval->translat[1];
   defsegat.translat[2] = segatval->translat[2];
   defsegat.rotate[0] = segatval->rotate[0];
   defsegat.rotate[1] = segatval->rotate[1];
   defsegat.rotate[2] = segatval->rotate[2];

   dfaltset = TRUE;
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_primitive_attributes                               */
/*                                                                          */
/*     PURPOSE: INQUIRE DEFAULT PRIMITIVE AND SEGMENT ATTRIBUTES.           */
/*                                                                          */
/****************************************************************************/

inquire_primitive_attributes(defprim,segatval)
    primattr *defprim; segattr *segatval;
{
   register int i;

   /**************************************************/
   /*** DEFAULT PRIMITIVE ATTRIBUTE TRANSFERS      ***/
   /**************************************************/

   defprim->colorindx = defaultt.colorindx;
   defprim->linwidth = defaultt.linwidth;
   defprim->linestyl = defaultt.linestyl;
   defprim->font = defaultt.font;
   defprim->charsize = defaultt.charsize;
   defprim->chrspace = defaultt.chrspace;
   defprim->chrup = defaultt.chrup;
   defprim->chrpath = defaultt.chrpath;
   defprim->chqualty = defaultt.chqualty;
   defprim->pickid = defaultt.pickid;
   defprim->marker = defaultt.marker;

   /******************************************/
   /*** SEGMENT ATTRIBUTE VALUES TRANSFERS ***/
   /******************************************/

   segatval->visbilty = defsegat.visbilty;
   segatval->detectbl = defsegat.detectbl;
   segatval->highlght = defsegat.highlght;
   segatval->scale[0] = defsegat.scale[0];
   segatval->scale[1] = defsegat.scale[1];
   segatval->scale[2] = defsegat.scale[2];
   segatval->translat[0] = defsegat.translat[0];
   segatval->translat[1] = defsegat.translat[1];
   segatval->translat[2] = defsegat.translat[2];
   segatval->rotate[0] = defsegat.rotate[0];
   segatval->rotate[1] = defsegat.rotate[1];
   segatval->rotate[2] = defsegat.rotate[2];

   return(0);
   }
