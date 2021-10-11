/*	@(#)cgiminicon.h 1.1 92/07/30 Copyr 1985-9 Sun Micro		*/

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

/* input device counts */
#define OFF1	 0
#define OFF2	 1
#define OFF3	 4
#define OFF4	 8
#define OFF5	12
#define OFF6	16
#define MAXCOLOR 255


/*  CGI VDC-to-Device Coordinate Transformation macros */

/* Determine if device scaling is needed, or only offsetting */
#define X_DEVSCALE_NEEDED(vws_ptr)					\
	(vws_ptr->xform.scale.x.num != (vws_ptr->xform.scale.x.den<<1))
#define Y_DEVSCALE_NEEDED(vws_ptr)					\
	(vws_ptr->xform.scale.y.num != (vws_ptr->xform.scale.y.den<<1))

/*
 * Set the identity scaling rational fraction, which includes
 * a prescale by 2 in the numerator to give us one bit whose value
 * is 1/2 for rounding.
 */
#define	IDENTITY_SCALE(fract)						\
	{								\
	    (fract).num = 2;						\
	    (fract).den = 1;						\
	}

/* Return a floating-point scale factor */
#define	DC_TO_VDC_SCALE_FACTOR(vws_ptr)					\
	((float) (vws_ptr)->xform.scale.x.den				\
	/ (float) (vws_ptr)->xform.scale.x.num)

#define	VDC_TO_DC_SCALE_FACTOR(vws_ptr)					\
	((float) (vws_ptr)->xform.scale.x.num				\
	/ (float) (vws_ptr)->xform.scale.x.den)

/* Device scale, as well as offset: bump is one of ++, --, (nothing) */

#define DEV_XFORM_X(vdc_coord_ptr,dev_coord_ptr,bump,vws_ptr)		\
	{								\
	register int _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = ((((vdc_coord_ptr)bump->x)			\
		    * vws_ptr->xform.scale.x.num)			\
		    / vws_ptr->xform.scale.x.den);			\
	if (_cgi_tmp_qxy > 0)						\
	    _cgi_tmp_qxy++;						\
	(dev_coord_ptr)bump->x = (_cgi_tmp_qxy >> 1)			\
		+ vws_ptr->xform.off.x - vws_ptr->conv.clip.r_left;	\
	}

#define DEV_XFORM_Y(vdc_coord_ptr,dev_coord_ptr,bump,vws_ptr)		\
	{								\
	register int _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = ((((vdc_coord_ptr)bump->y)			\
		    * vws_ptr->xform.scale.y.num)			\
		    / vws_ptr->xform.scale.y.den);			\
	if (_cgi_tmp_qxy > 0)						\
	    _cgi_tmp_qxy++;						\
	(dev_coord_ptr)bump->y = (_cgi_tmp_qxy >> 1)			\
		+ vws_ptr->xform.off.y - vws_ptr->conv.clip.r_top;	\
	}

/* Device offset only (no scaling): bump is one of ++, --, (nothing) */
#define DEV_OFF_X(vdc_coord_ptr,dev_coord_ptr,bump)			\
(   (dev_coord_ptr)bump->x =						\
 	((vdc_coord_ptr)bump->x) + _cgi_vws->xform.off.x 		\
	- _cgi_vws->conv.clip.r_left)
#define DEV_OFF_Y(vdc_coord_ptr,dev_coord_ptr,bump)			\
(   (dev_coord_ptr)bump->y =						\
	((vdc_coord_ptr)bump->y) + _cgi_vws->xform.off.y		\
	- _cgi_vws->conv.clip.r_top)


/* Note: _cgi_devscale takes the names of variables to load.
 * If it were a procedure, it would need the addresses of variables to load.
 * This macro is overkill, but will allow use of floats, shorts, or ints.
 * Use "unique" names so dbx doesn't confuse macro's vars with others.
 *
 * The conversion is: round((vdc_coord * xscale.num) / xscale.den)
 * Xscale.num was multiplied by 2 when is was stored, to save us a shift
 * here, thus the number generated after the multiply and divide has
 * 1 bit of fraction.  Because the division truncates toward 0, while the
 * shift truncates toward lower values, the rounding need only add 1/2
 * to positive numbers.  This really works!  Try a few examples on paper
 * first if you're tempted to change this code.
 */
#define _cgi_devscale(_x_vdc_coord, _y_vdc_coord, _x_dev_coord, _y_dev_coord) \
	{								\
	register View_surface	*_cgi_rvws = _cgi_vws;			\
	register int _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = (_x_vdc_coord);					\
	if(X_DEVSCALE_NEEDED(_cgi_rvws))				\
	    {								\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy * _cgi_rvws->xform.scale.x.num)\
			    / _cgi_rvws->xform.scale.x.den);		\
	    if (_cgi_tmp_qxy > 0)					\
		_cgi_tmp_qxy++;						\
	    _cgi_tmp_qxy >>= 1;						\
	    }								\
	(_x_dev_coord) = _cgi_tmp_qxy + _cgi_rvws->xform.off.x - \
					_cgi_rvws->conv.clip.r_left;		\
									\
	_cgi_tmp_qxy = (_y_vdc_coord);					\
	if(Y_DEVSCALE_NEEDED(_cgi_rvws))				\
	    {								\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy * _cgi_rvws->xform.scale.y.num)\
			    / _cgi_rvws->xform.scale.y.den);		\
	    if (_cgi_tmp_qxy > 0)					\
		_cgi_tmp_qxy++;						\
	    _cgi_tmp_qxy >>= 1;						\
	    }								\
	(_y_dev_coord) = _cgi_tmp_qxy + _cgi_rvws->xform.off.y -		\
					_cgi_rvws->conv.clip.r_top;		\
	}

#define _cgi_devscale_clip(_x_vdc_coord, _y_vdc_coord, _x_dev_coord, _y_dev_coord) \
	{								\
	register View_surface	*_cgi_rvws = _cgi_vws;			\
	register int _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = (_x_vdc_coord);					\
	if(X_DEVSCALE_NEEDED(_cgi_rvws))				\
	    {								\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy * _cgi_rvws->xform.scale.x.num)\
			    / _cgi_rvws->xform.scale.x.den);		\
	    if (_cgi_tmp_qxy > 0)					\
		_cgi_tmp_qxy++;						\
	    _cgi_tmp_qxy >>= 1;						\
	    }								\
	(_x_dev_coord) = _cgi_tmp_qxy + _cgi_rvws->xform.off.x; \
									\
	_cgi_tmp_qxy = (_y_vdc_coord);					\
	if(Y_DEVSCALE_NEEDED(_cgi_rvws))				\
	    {								\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy * _cgi_rvws->xform.scale.y.num)\
			    / _cgi_rvws->xform.scale.y.den);		\
	    if (_cgi_tmp_qxy > 0)					\
		_cgi_tmp_qxy++;						\
	    _cgi_tmp_qxy >>= 1;						\
	    }								\
	(_y_dev_coord) = _cgi_tmp_qxy + _cgi_rvws->xform.off.y; \
	}
#define _cgi_dev_scale_only(_x_vdc, _y_vdc, _x_dev, _y_dev)		\
	{								\
	register View_surface	*_cgi_rvws = _cgi_vws;			\
	register int _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = (_x_vdc);					\
	if(X_DEVSCALE_NEEDED(_cgi_rvws))				\
	    {								\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy * _cgi_rvws->xform.scale.x.num)\
			    / _cgi_rvws->xform.scale.x.den);		\
	    if (_cgi_tmp_qxy > 0)					\
		_cgi_tmp_qxy++;						\
	    _cgi_tmp_qxy >>= 1;						\
	    }								\
	(_x_dev) = _cgi_tmp_qxy;					\
									\
	_cgi_tmp_qxy = (_y_vdc);					\
	if(Y_DEVSCALE_NEEDED(_cgi_rvws))				\
	    {								\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy * _cgi_rvws->xform.scale.y.num)\
			    / _cgi_rvws->xform.scale.y.den);		\
	    if (_cgi_tmp_qxy > 0)					\
		_cgi_tmp_qxy++;						\
	    _cgi_tmp_qxy >>= 1;						\
	    }								\
	(_y_dev) = _cgi_tmp_qxy;					\
	}

/*
 * Devscalen does rounding correctly for negative lengths, even though
 * length should always be positive.  This is intended as "defensive
 * programming", and is cheap since lengths are evaluated very seldom
 * compared to coordinate conversions.
 */

#define _cgi_devscalen(_cgi_vdc_coord, _cgi_dev_coord)			\
	{								\
	register int _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = (_cgi_vdc_coord);				\
	if (X_DEVSCALE_NEEDED(_cgi_vws))				\
	    {								\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy				\
			    * _cgi_vws->xform.scale.x.num)		\
			    / _cgi_vws->xform.scale.x.den);		\
	    if (_cgi_tmp_qxy > 0)					\
		_cgi_tmp_qxy++;						\
	    _cgi_tmp_qxy >>= 1;						\
	    }								\
	(_cgi_dev_coord) = _cgi_tmp_qxy;				\
	}

#define _cgi_rev_devscalen(_cgi_vdc_coord, _cgi_dev_coord)		\
	{								\
	register int _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = (_cgi_dev_coord);				\
	if (X_DEVSCALE_NEEDED(_cgi_vws))				\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy				\
			    * _cgi_vws->xform.scale.x.den)		\
			    / (_cgi_vws->xform.scale.x.num)>>1);	\
	(_cgi_vdc_coord) = _cgi_tmp_qxy;				\
	}

#define _cgi_f_devscalen(_cgi_vdc_coord, _cgi_dev_coord)		\
	{								\
	register float _cgi_tmp_qxy;					\
	_cgi_tmp_qxy = (_cgi_vdc_coord);				\
	if (X_DEVSCALE_NEEDED(_cgi_vws))				\
	    _cgi_tmp_qxy = ((_cgi_tmp_qxy				\
		    * (float)(_cgi_vws->xform.scale.x.num>>1))		\
		    / (float)_cgi_vws->xform.scale.x.den);		\
	(_cgi_dev_coord) = _cgi_tmp_qxy;				\
	}

/* Note: _cgi_rev_devscale takes the names of variables to load.
 * If it were a procedure, it would need the addresses of variables to load.
 * This macro is overkill, but will allow use of floats, shorts, or ints.
 * Use "unique" names so dbx doesn't confuse macro's vars with others.
 * Reverse scaling of pixel coordinates generates the VDC coordinate for
 * the pixel's center.
 */

#define _cgi_rev_devscale(_x_vdc, _y_vdc, _x_dev, _y_dev)		\
	{								\
		register int _cgi_tmp_qxy;				\
		_cgi_tmp_qxy = (_x_dev);				\
		_cgi_tmp_qxy -= _cgi_vws->xform.off.x;			\
		if (X_DEVSCALE_NEEDED(_cgi_vws))			\
		    _cgi_tmp_qxy = ((_cgi_tmp_qxy			\
				* _cgi_vws->xform.scale.x.den)		\
				/ (_cgi_vws->xform.scale.x.num>>1));	\
		(_x_vdc) = _cgi_tmp_qxy;				\
									\
		_cgi_tmp_qxy = (_y_dev);				\
		_cgi_tmp_qxy -= _cgi_vws->xform.off.y;			\
		if (Y_DEVSCALE_NEEDED(_cgi_vws))			\
		    _cgi_tmp_qxy = ((_cgi_tmp_qxy			\
				* _cgi_vws->xform.scale.y.den)		\
				/ (_cgi_vws->xform.scale.y.num>>1));	\
		(_y_vdc) = _cgi_tmp_qxy;				\
	}

#define _cgi_rev_dev_scale_only(_x_vdc, _y_vdc, _x_dev, _y_dev)		\
	{								\
		register int _cgi_tmp_qxy;				\
		_cgi_tmp_qxy = (_x_dev);				\
		if (X_DEVSCALE_NEEDED(_cgi_vws))			\
		    _cgi_tmp_qxy = ((_cgi_tmp_qxy			\
				* _cgi_vws->xform.scale.x.den)		\
				/ (_cgi_vws->xform.scale.x.num>>1));	\
		(_x_vdc) = _cgi_tmp_qxy;				\
									\
		_cgi_tmp_qxy = (_y_dev);				\
		if (Y_DEVSCALE_NEEDED(_cgi_vws))			\
		    _cgi_tmp_qxy = ((_cgi_tmp_qxy			\
				* _cgi_vws->xform.scale.y.den)		\
				/ (_cgi_vws->xform.scale.y.num>>1));	\
		(_y_vdc) = _cgi_tmp_qxy;				\
	}
