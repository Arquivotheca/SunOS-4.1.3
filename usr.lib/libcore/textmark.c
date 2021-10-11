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
static char sccsid[] = "@(#)textmark.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: text                                                       */
/*                                                                          */
/*     PURPOSE: THE CHARACTER STRING IS DRAWN IN THE WORLD COORDINATE       */
/*              SYSTEM.                                                     */
/*                                                                          */
/****************************************************************************/

static pt_type sp, up, path;
static ipt_type isp, iup, ipath;
static short charbuf[64];
static int stdmarkr = 43;

text(string) char *string;
   {
   char *funcname, *chptr, *sptr;
   viewsurf *surfp;
   int (*surfdd)();
   short ptype, errnum, strlength, numchars, n;
   ipt_type ip1, ip2, ip3;
   pt_type p1, p2;
   ddargtype ddstruct;

   funcname = "text";
   if (!_core_osexists)
	{
	_core_errhand(funcname, 21);
	return(1);
	}
   if(strchk(string) == 2) {
      errnum = 23;
      _core_errhand(funcname,errnum);
      return(errnum);
      }

   _core_critflag++;
   if (_core_xformvs)
	{
	_core_tranpt(&_core_cp.x, &p1.x);
	_core_pt3cnvrt(&p1.x, &_core_ndccp.x);
	}
   ip1 = _core_ndccp;		/* get current point in NDC space */

   /*---- View transform character orientation parameters --------*/
   /*  compute three scaled direction vectors in NDC space        */

   if (_core_upflag || _core_pathflag || _core_spaceflag) {
       p2 = _core_current.chrup;
       _core_scalept( _core_current.charsize.height, &p2);/* size up vector */
       _core_sizept( &p2.x, &up.x);			/* transform up vector */
       _core_pt3cnvrt( &up.x, &iup.x);
       _core_vwptxtscale( &iup);


       p2 = _core_current.chrpath;
       _core_scalept( _core_current.charsize.width, &p2);/* size path vector */
       _core_sizept( &p2.x, &path.x);		/* transform path vector */
       _core_pt3cnvrt( &path.x, &ipath.x);
       _core_vwptxtscale( &ipath);

       p2 = _core_current.chrpath;
       _core_scalept( _core_current.chrspace.x, &p2);	/* size space vector */
       _core_sizept( &p2.x, &sp.x);	/* transform space vector */
       _core_pt3cnvrt( &sp.x, &isp.x);
       _core_vwptxtscale( &isp);
       }
   _core_vwpscale( &ip1);		/* scale and translate to viewport */

   /*---------------- Write text stuff out to PDF --------------*/

   if (_core_openseg->type >= RETAIN) {
	 ptype = PDFPICKID;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.pickid);
	 n = 4;
	 (void)_core_pdfwrite(SHORT,&n);
	 if (_core_current.chqualty >= CHARACTER) {
	     inquire_text_extent_3( string, &p2.x, &p2.y, &p2.z);
	     p2.w = 1.0;
	     _core_sizept( &p2.x, &p1.x);
	     _core_pt3cnvrt( &p1.x, &ip2.x);	/* text extent NDC */
	     _core_vwptxtscale( &ip2);	/* scale and translate to viewport */
	     ip2.x += ip1.x;  ip2.y += ip1.y;  ip2.z += ip1.z;
	     }
	 else {			/* hard coded for current raster fonts */
	     numchars = length( string);
	     ip2.x = numchars * 288 + ip1.x; ip2.y = ip1.y; ip2.z = 0;
	     iup.y = 384; iup.x = 0;
	     }
	 ip3.w = 1;
	 ip3.x = ip1.x + iup.x;  ip3.y = ip1.y + iup.y;  ip3.z = ip1.z;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.x = ip2.x + iup.x;  ip3.y = ip2.y + iup.y;  ip3.z = ip2.z;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.x = ip2.x - iup.x;  ip3.y = ip2.y - iup.y;  ip3.z = ip2.z;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.x = ip1.x - iup.x;  ip3.y = ip1.y - iup.y;  ip3.z = ip1.z;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
      if (_core_ropflag) {			/* rasterop changed */
	 ptype = PDFROP; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.rasterop);
	 }
      if (_core_textcflag) {			/* color changed */
	 ptype = PDFTCOLOR; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.textindx);
	 }
      if (_core_penflag) {			/* pen properties changed */
	 ptype = PDFPEN; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.pen);
	 }
      if (_core_fntflag) {			/* font properties changed */
	 ptype = PDFFONT; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.font);
	 }
      if (_core_upflag) {			/* up direction changed */
	 ptype = PDFUP; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT*3,(short *)&iup);
	 }
      if (_core_pathflag) {			/* path direction changed */
	 ptype = PDFPATH; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT*3,(short *)&ipath);
	 }
      if (_core_spaceflag) {		/* inter-char spacing vector changed */
	 ptype = PDFSPACE; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT*3,(short *)&isp);
	 }
      if (_core_justflag) {			/* character justify changed */
	 ptype = PDFCHARJUST; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.chjust);
	 }
      if (_core_qualflag) {			/* character quality changed */
	 ptype = PDFCHARQUALITY; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.chqualty);
	 }
      if (_core_cpchang) {		/* cp changed since last put in pdf */
	 ptype = PDFMOVE; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT*3,(short *)&ip1);
	 }
      ptype = PDFTEXT;
      (void)_core_pdfwrite(SHORT,&ptype);
      strlength = length(string);	/* number of shorts */
      (void)_core_pdfwrite(SHORT,&strlength);
      sptr = string;  chptr = (char*)charbuf;
      while (*chptr++ = *sptr++);
      (void)_core_pdfwrite((strlength+1)>>1, charbuf); /* number of shorts required */
      }

      /*********************************************************************/
      /*   NOTE: STRING IS BEING PLACED IN PDF AS A WHOLE NO MATTER WHAT   */
      /*         THE CHARACTER QUALITY TO SAVE SPACE AND ACCESSES TO       */
      /*         FILE EVEN THOUGH CALCULATIONS FOR CHARACTER QUALITY TEXT  */
      /*         WILL HAVE TO BE PERFORMED AGAIN UPON READING IN STRING    */
      /*         FROM PDF DURING A REPAINT. (CONSIDERED LESS EXPENSIVE)    */
      /*********************************************************************/

/*   before sending to devices must use image transformation on primitives   */

   if(! _core_openseg->idenflag) {
      if(_core_openseg->type == XLATE2) {
	 ip1.x += _core_openseg->imxform[3][0];	/* PERFORM A TRANSLATION ONLY */
	 ip1.y += _core_openseg->imxform[3][1];
	 ip1.z += _core_openseg->imxform[3][2];
	 }
      else if(_core_openseg->type == XFORM2) {
	 _core_imxfrm3( _core_openseg, &ip1, &ip2); ip1 = ip2;/* image xform current point */
	 ip2 = iup;
	 _core_imszpt3( _core_openseg, &ip2, &iup);		/* image xform up vector     */
	 ip2 = isp;
	 _core_imszpt3( _core_openseg, &ip2, &isp);		/* image xform space vector  */
	 ip2 = ipath;
	 _core_imszpt3( _core_openseg, &ip2, &ipath);	/* image xform path vector   */
	 }
      }

/*--------- Write text on all selected surfaces ------------*/

   for (n = 0; n < _core_openseg->vsurfnum; n++) {
	 surfp = _core_openseg->vsurfptr[n];
	 ddstruct.instance = surfp->vsurf.instance;
	 surfdd = surfp->vsurf.dd;
	 if (_core_textcflag || _core_ropflag || _core_texatt){
	    ddstruct.opcode = SETTCOL;			/* color changed */
	    ddstruct.int1 = _core_current.textindx;
    	    ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    	    ddstruct.int3 = FALSE;
	    (*surfdd)(&ddstruct);
	    }
	 if(_core_fntflag || _core_texatt){	/* FONT PROPERTY CHANGED? */
	    ddstruct.opcode = SETFONT;
	    ddstruct.int1 = _core_current.font;
	    (*surfdd)(&ddstruct);
	    }
	 if(_core_penflag || _core_texatt){	/* PEN PROPERTY CHANGED? */
	    ddstruct.opcode = SETPEN;
	    ddstruct.int1 = _core_current.pen;
	    (*surfdd)(&ddstruct);
	    }
	 if(_core_upflag || _core_texatt) {	/* UP PROPERTY CHANGED?*/
	    ddstruct.opcode = SETUP;
	    ddstruct.ptr1 = (char*)(&iup);
	    (*surfdd)(&ddstruct);
	    }
	 if(_core_pathflag || _core_texatt){	/* PATH PROPERTY CHANGED?*/
	    ddstruct.opcode = SETPATH;
	    ddstruct.ptr1 = (char*)(&ipath);
	    (*surfdd)(&ddstruct);
	    }
	 if(_core_spaceflag || _core_texatt){	/* SPACING PROPERTY CHANGED?*/
	    ddstruct.opcode = SETSPACE;
	    ddstruct.ptr1 = (char*)(&isp);
	    (*surfdd)(&ddstruct);
	    }
	 if (_core_cpchang){		/* cp change since last sent to DD */
	    ddstruct.opcode = MOVE;
	    ddstruct.int1 = ip1.x;
	    ddstruct.int2 = ip1.y;
	    (*surfdd)(&ddstruct);
	    }
	 if (_core_openseg->segats.visbilty) {
	     if (_core_current.chqualty == STRING) {
		ddstruct.opcode = TEXT;
		ddstruct.ptr1 = string;
		ddstruct.ptr2 = (char*)(&_core_vwstate.viewport);
		ddstruct.int1 = _core_current.font;
		ddstruct.int2 = 0;		/* string posit from cp */
		ddstruct.logical = TRUE;
		(*surfdd)(&ddstruct);
		}
	     else  {			/* use vector font text */
		ddstruct.opcode = VTEXT;
		ddstruct.ptr1 = string;
		ddstruct.ptr2 = (char*)(&_core_vwstate.viewport);
		(*surfdd)(&ddstruct);
		}
	     }
      }

   _core_textcflag = FALSE;
   _core_fntflag   = FALSE;
   _core_penflag   = FALSE;
   _core_upflag    = FALSE;
   _core_pathflag  = FALSE;
   _core_spaceflag = FALSE;
   _core_justflag  = FALSE;
   _core_qualflag  = FALSE;
   _core_ropflag   = FALSE;
   _core_texatt   = FALSE;
   _core_cpchang   = FALSE;

   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_abs_2                                               */
/*                                                                          */
/*     PURPOSE: THE CP IS UPDATED TO (x,y) AND THEN THE SYSTEM-DEFINED      */
/*              MARKER 'n',CENTERED AT THE TRANSFORMED POSITION OF THE CP,  */
/*              IS CREATED.                                                 */
/*                                                                          */
/****************************************************************************/

static ipt_type mup = { 0,256,0,1};	/* fix this to use defaults MJS */
static ipt_type mpath = { 256,0,0,1};

marker_abs_2(mx,my) float mx,my;
{
   char *funcname, sstr[2];
   int cm;
   viewsurf *surfp;
   int (*surfdd)();
   short ptype, n;
   pt_type p1;
   ipt_type ip1, ip2, ip3;
   ddargtype ddstruct;

   funcname = "marker_abs_2";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   cm = _core_current.marker;
   if ( (cm > MAXASCII) || (cm < MINASCII)) {
      _core_errhand(funcname,25);  /* report and replace with a question mark */
      cm = QUESTION;
      }
  
   _core_critflag++;
   _core_cp.x = mx;  _core_cp.y = my;		/* update cp and _core_ndccp */
   _core_cp.z = 0.;  _core_cp.w = 1.0;
   _core_tranpt2( &_core_cp.x ,&p1.x);
   _core_pt2cnvrt( &p1.x, &ip1.x);
   _core_ndccp = ip1;
   _core_cpchang = TRUE;

   if(_core_wndwclip) {
      if( !_core_clippt2( ip1.x, ip1.y)){	/* if point outside window */
         if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	 return(0);
	 }
      }
   _core_vwpscale( &ip1);		/* scale and translate to viewport */

   if (_core_openseg->type >= RETAIN) {
	 ptype = PDFPICKID;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.pickid);
	 n = 4;
	 (void)_core_pdfwrite(SHORT,&n);

	 ip3.x = ip1.x - mpath.x; ip3.y = ip1.y + mup.y;  ip3.z = 0;  ip3.w = 1;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.x = ip1.x + mpath.x;  ip3.y = ip1.y + mup.y;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.x = ip1.x + mpath.x;  ip3.y = ip1.y - mup.y;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.x = ip1.x - mpath.x;  ip3.y = ip1.y - mup.y;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
      if (_core_ropflag) {			/* rasterop changed */
	 ptype = PDFROP; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.rasterop);
	 }
      if (_core_textcflag) {			/* color changed */
	 ptype = PDFTCOLOR;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.textindx);
	 }
      if (_core_cpchang) {			/* if cp changed */
	 ptype = PDFMOVE;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT*3,(short *)&ip1);
	 }
      ptype = PDFMARKER;
      (void)_core_pdfwrite(SHORT,&ptype);
      (void)_core_pdfwrite(FLOAT,(short *)&cm);
      }

   /* perform an image transformation on the point before sending to DD */

   if(! _core_openseg->idenflag) {
      if(_core_openseg->type == XLATE2) {	/* PERFORM TRANSLATION ONLY. */
	 ip1.x += _core_openseg->imxform[3][0];
	 ip1.y += _core_openseg->imxform[3][1];
	 }
      else if(_core_openseg->type == XFORM2) {	/* ROTATE,SCALE, TRANSLATE */
	 _core_imxfrm2( _core_openseg, &ip1, &ip2);
	 ip1 = ip2;
	 }
      }
   if(_core_outpclip) {
      if( !_core_oclippt2( ip1.x, ip1.y, &_core_vwstate.viewport)){
          if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	 return(0);				/* if pt outside viewport */
	 }
      }

/****************************************************************************/
/** from here on this function is essentially the same as TEXT except that***/
/** even if the DD supports text, it is only expected to generate ASCII   ***/
/** characters, not special characters. Others are done by software.      ***/
/** The DD is issued a MARK opcode rather than a TEXT so that it will use ***/
/** default character sizes and orientations and the cp is the center of  ***/
/** the character.                                                        ***/
/****************************************************************************/

   for (n = 0; n < _core_openseg->vsurfnum; n++) {
	 surfp = _core_openseg->vsurfptr[n];
	 ddstruct.instance = surfp->vsurf.instance;
	 surfdd = surfp->vsurf.dd;
	 if (_core_textcflag || _core_ropflag || _core_texatt){
	    ddstruct.opcode = SETTCOL;		/* color changed */
	    ddstruct.int1 = _core_current.textindx;
    	    ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    	    ddstruct.int3 = FALSE;
	    (*surfdd)(&ddstruct);
	    }
	 if(_core_cpchang){	  		/* current point changed */
	    ddstruct.opcode = MOVE;
	    ddstruct.int1 = ip1.x;
	    ddstruct.int2 = ip1.y;
	    (*surfdd)(&ddstruct);
	    }
	 sstr[0] = cm;  sstr[1] = '\0';	/* one char long string */

	 if (_core_openseg->segats.visbilty) {
	     ddstruct.opcode = MARKER;
	     ddstruct.ptr1 = sstr;
	     ddstruct.ptr2 = (char*)(&_core_vwstate.viewport);
	     (*surfdd)(&ddstruct);
	     }
      }

   _core_textcflag = FALSE;
   _core_ropflag = FALSE;
   _core_cpchang = FALSE;

   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_rel_2                                               */
/*                                                                          */
/*     PURPOSE: THE CP IS UPDATED TO A RELATIVE (x,y) AND THEN THE          */
/*              SYSTEM-DEFINED MARKER 'n',CENTERED AT THE TRANSFORMED       */
/*              POSITION OF THE CP, IS CREATED.                             */
/*                                                                          */
/****************************************************************************/

marker_rel_2(dx,dy) float dx,dy;
{
   char *funcname;

   funcname = "marker_rel_2";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }

   return( marker_abs_2(_core_cp.x + dx,_core_cp.y + dy));
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polymarker_abs_2                                           */
/*                                                                          */
/*     PURPOSE: DESCRIBE A SEQUENCE OF MARKERS IN WORLD COORDINATES.        */
/*                                                                          */
/****************************************************************************/

polymarker_abs_2(xcoord,ycoord,n) float xcoord[],ycoord[]; short n;
{
   char *funcname;
   int errnum;
   register int index;

   funcname = "polymarker_abs_2";
   if (! _core_osexists) {
      errnum = 21; _core_errhand(funcname,errnum);
      return(errnum);
      }
   if (n <= 0) {
      errnum = 22; _core_errhand(funcname,errnum);
      return(errnum);
      }

   for (index = 0; index < n; index++) {
      (void)marker_abs_2(xcoord[index],ycoord[index]);
      }
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polymarker_rel_2                                           */
/*                                                                          */
/*     PURPOSE: DESCRIBE A SEQUENCE OF MARKERS IN WORLD COORDINATES.        */
/*                                                                          */
/****************************************************************************/

polymarker_rel_2(xcoord,ycoord,n) float xcoord[],ycoord[]; int n;
   {
   char *funcname;
   int errnum;
   register int index;

   funcname = "polymarker_rel_2";
   if (! _core_osexists) {
      errnum = 21; _core_errhand(funcname,errnum);
      return(errnum);
      }
   if (n <= 0) {
      errnum = 22; _core_errhand(funcname,errnum);
      return(errnum);
      }

   for (index = 0; index < n; index++) {
       (void)marker_rel_2(xcoord[index],ycoord[index]);
       }

   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_abs_3                                               */
/*                                                                          */
/*     PURPOSE: THE CP IS UPDATED TO (x,y,z) AND THEN THE SYSTEM-DEFINED    */
/*              MARKER 'n',CENTERED AT THE TRANSFORMED POSITION OF THE CP,  */
/*              IS CREATED.                                                 */
/*                                                                          */
/****************************************************************************/

marker_abs_3(mx,my,mz) float mx,my,mz;
{
   char *funcname, sstr[2];
   int cm;
   viewsurf *surfp;
   int (*surfdd)();
   short ptype, n;
   pt_type p1;
   ipt_type ip1, ip2;
   ddargtype ddstruct;

   funcname = "marker_abs_3";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   cm = _core_current.marker;
   if (cm < 1) {
      _core_errhand(funcname,25); return(2);
      }
   if ((cm > stdmarkr) && (cm < MINASCII) || (cm > MAXASCII)) {
      cm = QUESTION;
      }

   _core_critflag++;
   _core_cp.x = mx;  _core_cp.y = my;  _core_cp.z = mz;  _core_cp.w = 1.0;
   _core_tranpt( &_core_cp.x ,&p1.x);
   _core_pt3cnvrt( &p1.x, &ip1.x);
   _core_ndccp = ip1;
   _core_cpchang = TRUE;

   if(_core_wclipplanes) {
      if( !_core_clippt3( ip1.x, ip1.y, 0)){	/* if point outside window */
          if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	 return(0);
	 }
      }
   _core_vwpscale( &ip1);		/* scale and translate to viewport */

   if (_core_openseg->type >= RETAIN) {
	 ptype = PDFPICKID;
	 (void)_core_pdfwrite(SHORT, (short *)&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.pickid);
	 n = 4;
	 (void)_core_pdfwrite(SHORT,&n);

	 ip2.x = ip1.x - mpath.x; ip2.y = ip1.y + mup.y;
	 ip2.z = ip1.z; ip2.w = ip1.w;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip2);
	 _core_update_segbndbox(_core_openseg, &ip2);
	 ip2.x = ip1.x + mpath.x;  ip2.y = ip1.y + mup.y;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip2);
	 _core_update_segbndbox(_core_openseg, &ip2);
	 ip2.x = ip1.x + mpath.x;  ip2.y = ip1.y - mup.y;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip2);
	 _core_update_segbndbox(_core_openseg, &ip2);
	 ip2.x = ip1.x - mpath.x;  ip2.y = ip1.y - mup.y;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip2);
	 _core_update_segbndbox(_core_openseg, &ip2);
      if (_core_ropflag) {			/* rop properties changed */
	 ptype = PDFROP;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.rasterop);
	 }
      if (_core_textcflag) {		/* color changed */
	 ptype = PDFTCOLOR;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.textindx);
	 }
      if (_core_cpchang ) {			/* current point changed */
	 ptype = PDFMOVE;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT*3,(short *)&ip1);
	 }
      ptype = PDFMARKER;
      (void)_core_pdfwrite(SHORT,&ptype);
      (void)_core_pdfwrite(FLOAT,(short *)&cm);
      }
   /* perform an image transformation on the point before sending to DD */

   if(! _core_openseg->idenflag) {
      if(_core_openseg->type == XLATE3) {   /* PERFORM A TRANSLATION ONLY. */
	  ip1.x += _core_openseg->imxform[3][0];
	  ip1.y += _core_openseg->imxform[3][1];
	  ip1.z += _core_openseg->imxform[3][2];
          }
      else if(_core_openseg->type == XFORM3) {	/* ROTATE,SCALE, TRANSLATE */
	  _core_imxfrm3( _core_openseg, &ip1, &ip2);
	  ip1 = ip2;
	  }
      }
   if(_core_outpclip) {			/* for clipping after imxforming */
      if( !_core_oclippt2( ip1.x, ip1.y, &_core_vwstate.viewport)){
          if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	 return(0);				/* if point outside window */
	 }
      }

/****************************************************************************/
/*  from here on this function is essentially the same as TEXT except that  */
/*  even if the DD supports text, it is only expected to generate ASCII     */
/*  characters, not special characters. Others are done by software.        */
/*  The DD is issued a MARK opcode rather than a TEXT so that it will use   */
/*  default character sizes and orientations and the cp is the center of    */
/*  the character.                                                          */
/****************************************************************************/

   for (n = 0; n < _core_openseg->vsurfnum; n++) {
	 surfp = _core_openseg->vsurfptr[n];
	 ddstruct.instance = surfp->vsurf.instance;
	 surfdd = surfp->vsurf.dd;
	 if (_core_textcflag || _core_ropflag || _core_texatt) {
	    ddstruct.opcode = SETTCOL;		/* color changed */
	    ddstruct.int1 = _core_current.textindx;
    	    ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    	    ddstruct.int3 = FALSE;
	    (*surfdd)(&ddstruct);
	    }
	 if(_core_cpchang) {			/* current point changed */
	    ddstruct.opcode = MOVE;
	    ddstruct.int1 = ip1.x;
	    ddstruct.int2 = ip1.y;
	    (*surfdd)(&ddstruct);
	    }
	 sstr[0] = cm;  sstr[1] = '\0';	/* one char long string */

	 if (_core_openseg->segats.visbilty) {
	     ddstruct.opcode = MARKER;
	     ddstruct.ptr1 = sstr;
	     ddstruct.ptr2 = (char*)(&_core_vwstate.viewport);
	     (*surfdd)(&ddstruct);
	     }
      }

   _core_textcflag = FALSE;
   _core_ropflag = FALSE;
   _core_cpchang = FALSE;

   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_rel_3                                               */
/*                                                                          */
/*     PURPOSE: THE CP IS UPDATED TO A RELATIVE (x,y,z) AND THEN THE        */
/*              SYSTEM-DEFINED MARKER 'n',CENTERED AT THE TRANSFORMED       */
/*              POSITION OF THE CP, IS CREATED.                             */
/*                                                                          */
/****************************************************************************/

marker_rel_3(dx,dy,dz) float dx,dy,dz;
{
   char *funcname;
   int errnum;
   int n;

   funcname = "marker_rel_3";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   n = _core_current.marker;
   if (n < 1) {
      errnum = 25;
      _core_errhand(funcname,errnum);
      return(2);
      }
   if ((n > stdmarkr) && (n < MINASCII)) {
      errnum = 25;
      _core_errhand(funcname,errnum);
      /*** do not return, merely report and replace with a question mark ***/
      n = QUESTION;
      }
   if (n > MAXASCII) {
      errnum = 25;
      _core_errhand(funcname,errnum);
      /*** do not return, merely report and replace with a question mark ***/
      n = QUESTION;
      }

   return( marker_abs_3(_core_cp.x + dx, _core_cp.y + dy, _core_cp.z + dz)  );
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polymarker_abs_3                                           */
/*                                                                          */
/*     PURPOSE: DESCRIBE A SEQUENCE OF MARKERS IN WORLD COORDINATES.        */
/*                                                                          */
/****************************************************************************/

polymarker_abs_3(xcoord,ycoord,zcoord,n)
    float xcoord[],ycoord[],zcoord[]; int n;
{
   char *funcname;
   int errnum;
   register int index;

   funcname = "polymarker_abs_3";
   if (! _core_osexists) {
      errnum = 21; _core_errhand(funcname,errnum);
      return(errnum);
      }
   if (n <= 0) {
      errnum = 22; _core_errhand(funcname,errnum);
      return(errnum);
      }

   for (index = 0; index < n; index++) {
      (void)marker_abs_3(xcoord[index],ycoord[index],zcoord[index]);
      }
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polymarker_rel_3                                           */
/*                                                                          */
/*     PURPOSE: DESCRIBE A SEQUENCE OF MARKERS IN WORLD COORDINATES.        */
/*                                                                          */
/****************************************************************************/

polymarker_rel_3(xcoord,ycoord,zcoord,n)
    float xcoord[],ycoord[],zcoord[]; int n;
{
   char *funcname;
   register int index;
   int errnum;

   funcname = "polymarker_rel_3";
   if (! _core_osexists) {
      errnum = 21; _core_errhand(funcname,errnum);
      return(errnum);
      }
   if (n <= 0) {
      errnum = 22; _core_errhand(funcname,errnum);
      return(errnum);
      }
   for (index = 0; index < n; index++) {
       (void)marker_rel_3(xcoord[index],ycoord[index],zcoord[index]);
       }
   return(0);
   }


/****************************************************************************/
/*     FUNCTION: length                                                     */
/*                                                                          */
/*     PURPOSE: COMPUTE THE LENGTH ( NUMBER OF CHARACTERS) OF A STRING.     */
/****************************************************************************/

static int length(string) char *string;
{
   register int counter;

   for(counter = 0;*string != '\0';string++,counter++) ;
   return(counter);
}

/****************************************************************************/
/*     FUNCTION: strchk                                                     */
/*     PURPOSE: CHECK A STRING FOR AN ILLEGAL CHARACTER.                    */
/****************************************************************************/

static int strchk(string) char *string;
{
   for( ;*string != '\0';string++) {
      if((*string < MINASCII) || (*string > MAXASCII)) {
	 *string = QUESTION;
	 return(2);
	 }
      }
   return(0);
}
