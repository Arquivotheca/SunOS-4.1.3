#ifndef lint
static	char sccsid[] = "@(#)acutab.c 1.1 92/07/30 SMI"; /* from UCB 4.7 6/25/83 */
#endif

#include "tip.h"

extern int df02_dialer(), df03_dialer(), df_disconnect(), df_abort(),
	   biz31f_dialer(), biz31_disconnect(), biz31_abort(),
	   biz31w_dialer(),
	   biz22f_dialer(), biz22_disconnect(), biz22_abort(),
	   biz22w_dialer(),
	   hayes_dialer(), hayes_disconnect(), hayes_abort(),
	   ven_dialer(), ven_disconnect(), ven_abort(),
	   v3451_dialer(), v3451_disconnect(), v3451_abort(),
	   v831_dialer(), v831_disconnect(), v831_abort(),
	   dn_dialer(), dn_disconnect(), dn_abort();

acu_t acutable[] = {
#if BIZ1031
	"biz31f", biz31f_dialer, biz31_disconnect,	biz31_abort,
	"biz31w", biz31w_dialer, biz31_disconnect,	biz31_abort,
#endif
#if BIZ1022
	"biz22f", biz22f_dialer, biz22_disconnect,	biz22_abort,
	"biz22w", biz22w_dialer, biz22_disconnect,	biz22_abort,
#endif
#if DF02
	"df02",	df02_dialer,	df_disconnect,		df_abort,
#endif
#if DF03
	"df03",	df03_dialer,	df_disconnect,		df_abort,
#endif
#if DN11
	"dn11",	dn_dialer,	dn_disconnect,		dn_abort,
#endif
#ifdef VENTEL
	"ventel",ven_dialer,	ven_disconnect,		ven_abort,
#endif
#ifdef V3451
#ifndef V831
	"vadic",v3451_dialer,	v3451_disconnect,	v3451_abort,
#endif
	"v3451",v3451_dialer,	v3451_disconnect,	v3451_abort,
#endif
#ifdef V831
#ifndef V3451
	"vadic",v831_dialer,	v831_disconnect,	v831_abort,
#endif
	"v831",v831_dialer,	v831_disconnect,	v831_abort,
#endif
#ifdef HAYES
	"hayes",hayes_dialer,	hayes_disconnect,	hayes_abort,
	"at",	hayes_dialer,	hayes_disconnect,	hayes_abort,
#endif
	0,	0,		0,			0
};
