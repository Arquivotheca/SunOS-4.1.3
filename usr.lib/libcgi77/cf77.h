/*	@(#)cf77.h 1.1 92/07/30 Copyr 1985-9 Sun Micro	*/

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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


#define	ALLOC_COORLIST(l,x,y,n)	_cgi_f77_alloc_coorlist(l,x,y,n)
#define	PASS_STRING(d,s,n,m)	_cgi_f77_pass_string(d,s,n,m)
#define	FREE_INREP(s)		_cgi_f77_free_coorlist((s)->points)
#define	FREE_COORLIST(l)	_cgi_f77_free_coorlist(l)

#define	ASSIGN_COOR(cp,xv,yv)	((cp)->x = (Cint)(xv), (cp)->y = (Cint)(yv))
#define	ASSIGN_XY(xv,yv,cp)	((xv) = (cp)->x, (yv) = (cp)->y)
#define	ASSIGN_INREP(s,x,y,xlst,ylst,n,v,c,str,len,sgid,pkid,pt,list,cbuf,cbuflen,pk) \
    ((s)->xypt = (pt), \
    ASSIGN_COOR((pt), (x), (y)), \
    (s)->val = (Cfloat) (v), \
    (s)->choice = (Cint) (c), \
    (s)->string = (cbuf), \
    PASS_STRING((cbuf), (str), (len), (cbuflen)), \
    (s)->pick = (pk), \
    (pk)->segid = (sgid), \
    (pk)->pickid = (pkid), \
    (s)->points = (list),  \
    ALLOC_COORLIST((list), (xlst), (ylst), (n)))

#define	RETURN_INREP(d,x,y,xl,yl,n,v,c,s,sl,sg,pk,ptr) \
		_cgi_f77_return_inrep(d,x,y,xl,yl,n,v,c,s,sl,sg,pk,ptr)
#define	RETURN_STRING(d,s,n)	_cgi_f77_return_string(d,s,n)
#define	RETURN_1ARRAY(d,s,n)	_cgi_intncpy(d,s,n)
#define	RETURN_XYLIST(x,y,n,c)	_cgi_f77_return_xylist(x,y,n,c)

#define	CONDITIONAL_FREE(p)	_cgi_conditional_free(p)
#define	COPY_UC_TO_I(d,s,n)	_cgi_copy_uc_to_i(d,s,n)
#define	COPY_I_TO_UC(d,s,n)	_cgi_copy_i_to_uc(d,s,n)

/* To flag wrapper libarary internal errors with legal CGI error code */
#define	EF77INTERNAL	EMEMSPAC
#ifndef	NULL
#	define	NULL	0
#endif

/*
 * Types of functions private to the wrapper library.
 */
Cerror  _cgi_f77_alloc_coorlist ();
Cerror  _cgi_f77_assign_coorlist ();
Cerror  _cgi_f77_free_coorlist ();
Cerror  _cgi_f77_return_xylist ();
char *	_cgi_f77_pass_string ();
char *	_cgi_f77_return_string ();
Cerror	_cgi_f77_return_inrep ();
