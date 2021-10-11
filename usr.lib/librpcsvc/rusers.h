/*	@(#)rusers.h 1.1 92/07/30 SMI */

/* 
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef _rpcsvc_rusers_h
#define _rpcsvc_rusers_h

#define RUSERSPROC_NUM 1
#define RUSERSPROC_NAMES 2
#define RUSERSPROC_ALLNAMES 3
#define RUSERSPROG 100002
#define RUSERSVERS_ORIG 1
#define RUSERSVERS_IDLE 2
#define RUSERSVERS 2

#define MAXUSERS 100

struct utmparr {
	struct utmp **uta_arr;
	int uta_cnt;
};

struct utmpidle {
	struct utmp ui_utmp;
	unsigned ui_idle;
};

struct utmpidlearr {
	struct utmpidle **uia_arr;
	int uia_cnt;
};

int xdr_utmparr();
int xdr_utmpidlearr();

#endif /*!_rpcsvc_rusers_h*/
