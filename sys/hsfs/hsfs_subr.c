#ifndef lint 
#ident "@(#)hsfs_subr.c 1.1 92/07/30" 
#endif

/*
 * Miscellaneous support subroutines for High Sierra filesystem
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <hsfs/hsfs_spec.h>
#include <hsfs/hsfs_isospec.h>
#include <hsfs/hsfs_private.h>
#include <hsfs/hsfs_node.h>

#define	THE_EPOCH	1970
#define	END_OF_TIME	2099

static long hs_date_to_gmtime();

/*
 * hs_parse_dirdate
 *
 * Parse the short 'directory-format' date into a Unix timeval.
 * This is the date format used in Directory Entries.
 *
 * If the date is not representable, make something up.
 */
void
hs_parse_dirdate(flag, dp, tvp)
	enum hs_vol_type flag;
	u_char *dp;
	struct timeval *tvp;
{
	int year, month, day, hour, min, sec;

	year = HDE_DATE_YEAR(dp);
	month = HDE_DATE_MONTH(dp);
	day = HDE_DATE_DAY(dp);
	hour = HDE_DATE_HOUR(dp);
	min = HDE_DATE_MIN(dp);
	sec = HDE_DATE_SEC(dp);


	tvp->tv_usec = 0;
	if (year < THE_EPOCH) {
		tvp->tv_sec = 0;
	} else {
		tvp->tv_sec = hs_date_to_gmtime(year, month, day);
		if (tvp->tv_sec != -1) {
			tvp->tv_sec += ((hour * 60) + min) * 60 + sec;
		}
	}

	if (flag == HS_VOL_TYPE_ISO){
	/* XXX - not yet done the GMT adjustment */
	}

	return;

}

/*
 * hs_parse_longdate
 *
 * Parse the long 'user-oriented' date into a Unix timeval.
 * This is the date format used in the Volume Descriptor.
 *
 * If the date is not representable, make something up.
 */
void
hs_parse_longdate(flag, dp, tvp)
	enum hs_vol_type flag;
	u_char *dp;
	struct timeval *tvp;
{
	int year, month, day, hour, min, sec;

	year = HSV_DATE_YEAR(dp);
	month = HSV_DATE_MONTH(dp);
	day = HSV_DATE_DAY(dp);
	hour = HSV_DATE_HOUR(dp);
	min = HSV_DATE_MIN(dp);
	sec = HSV_DATE_SEC(dp);


	tvp->tv_usec = 0;
	if (year < THE_EPOCH) {
		tvp->tv_sec = 0;
	} else {
		tvp->tv_sec = hs_date_to_gmtime(year, month, day);
		if (tvp->tv_sec != -1) {
			tvp->tv_sec += ((hour * 60) + min) * 60 + sec;
			tvp->tv_usec = HSV_DATE_HSEC(dp) * 10000;
		}
	}

	if (flag == HS_VOL_TYPE_ISO){
	/* XXX - not yet done the GMT adjustment */
	}
}

/* cumulative number of seconds per month,  non-leap and leap-year versions */
static long cum_sec[] = {
	0x0, 0x28de80, 0x4dc880, 0x76a700, 0x9e3400, 0xc71280,
	0xee9f80, 0x1177e00, 0x1405c80, 0x167e980, 0x190c800, 0x1b85500
};
static long cum_sec_leap[] = {
	0x0, 0x28de80, 0x4f1a00, 0x77f880, 0x9f8580, 0xc86400,
	0xeff100, 0x118cf80, 0x141ae00, 0x1693b00, 0x1921980, 0x1b9a680
};
#define	SEC_PER_DAY	0x15180
#define	SEC_PER_YEAR	0x1e13380

/*
 * hs_date_to_gmtime
 *
 * Convert year(1970-2099)/month(1-12)/day(1-31) to seconds-since-1970/1/1.
 *
 * Returns -1 if the date is out of range.
 */
static long
hs_date_to_gmtime(year, mon, day)
	int year;
	int mon;
	int day;
{
	register long sum;
	register long *cp;
	register int y;

	if ((year < THE_EPOCH) || (year > END_OF_TIME) ||
	    (mon < 1) || (mon > 12) ||
	    (day < 1) || (day > 31))
		return (-1);

	/*
	 * Figure seconds until this year and correct for leap years.
	 * Note: 2000 is a leap year but not 2100.
	 */
	y = year - THE_EPOCH;
	sum = y * SEC_PER_YEAR;
	sum += ((y + 1) / 4) * SEC_PER_DAY;
	/*
	 * Point to the correct table for this year and
	 * add in seconds until this month.
	 */
	cp = ((y + 2) % 4) ? cum_sec : cum_sec_leap;
	sum += cp[mon - 1];
	/*
	 * Add in seconds until 0:00 of this day.
	 * (days-per-month validation is not done here)
	 */
	sum += (day - 1) * SEC_PER_DAY;
	return (sum);
}

/*
 * hs_readsector
 *
 * Read a sector into a buffer.
 * Return any error that occurred.
 */
int
hs_readsector(vp, secno, ptr)
	struct vnode *vp;
	u_int secno;
	u_char *ptr;
{
	struct iovec iov;
	struct uio iod;
	int error;

	iov.iov_base = (caddr_t) ptr;
	iov.iov_len = HS_SECTOR_SIZE;
	iod.uio_iov = &iov;
	iod.uio_iovcnt = 1;
	iod.uio_offset = HS_SECTOR_SIZE * secno;
	iod.uio_segflg = UIO_SYSSPACE;
	iod.uio_fmode = 0;
	iod.uio_resid = HS_SECTOR_SIZE;
	if ((error = VOP_RDWR(vp, &iod, UIO_READ, 0, u.u_cred)) ||
	    (iod.uio_resid != 0)) {
		if (!error)
			error = EFBIG;
	}
	return (error);
}
