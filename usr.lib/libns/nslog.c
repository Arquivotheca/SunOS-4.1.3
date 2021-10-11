
/*	@(#)nslog.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:nslog.c	1.7" */
/*
 *	nslog.c contains the setlog function, which sets up logging
 */
#include <stdio.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>
#include "nslog.h"
#include "stdns.h"
#include "nsdb.h"

int	Loglevel;
FILE	*Logfd = stderr;
int	Logstamp;
char	Logbuf[BUFSIZ];
static	char	*typetostr();

static int findint();
static struct logtype {
	int	l_type;
	char	l_symb;
} Loglist[] = {
	{ L_DB,		'd'},	/* data base functions		*/
	{ L_MALLOC,	'm'},	/* malloc and free tracing	*/
	{ L_COMM,	'c'},	/* communications (nsports)	*/
	{ L_BLOCK,	'b'},	/* block making, (nsblock)	*/
	{ L_CONV,	'i'},	/* data conversion, (ind_data)	*/
	{ L_TRANS,	't'},	/* transactions, (nsfunc)	*/
	{ L_PIPE,	'p'},	/* pipe communications, (ptype)	*/
	{ L_STREAM,	's'},	/* stream comm., (stype)	*/
	{ L_ALL,	'a'},	/* log everything		*/
	{ L_OVER,	'o'},	/* overview of major events	*/
	{ L_REC,	'r'},	/* recovery mechanism		*/
	{ 0,		'\0'}	/* end with null entry		*/
};

static struct errtype {
	int	e_type;
	char	*e_name;
} Errlist[] = {
	{ R_NOERR,	"R_NOERR"  },	/* no error			*/
	{ R_FORMAT,	"R_FORMAT" },	/* format error 		*/
	{ R_NSFAIL,	"R_NSFAIL" },	/* name server failure 		*/
	{ R_NONAME,	"R_NONAME" },	/* name does not exist 		*/
	{ R_IMP,	"R_IMP"    },	/* type unimplemented (or bad)	*/
	{ R_PERM,	"R_PERM"   },	/* no permission for operation	*/
	{ R_DUP,	"R_DUP"    },	/* name not unique (for adv)	*/
	{ R_SYS,	"R_SYS"    },	/* syscall failed in ns		*/
	{ R_EPASS,	"R_EPASS"  },	/* error reading passwd file 	*/
	{ R_INVPW,	"R_INVPW"  },  	/* invalid password		*/
	{ R_NOPW,	"R_NOPW"   },	/* no password entry    	*/
	{ R_SETUP,	"R_SETUP"  },	/* error in ns_setup()		*/
	{ R_SEND,	"R_SEND"   },	/* error in ns_send()		*/
	{ R_RCV,	"R_RCV"    },	/* error in ns_rcv()		*/
	{ R_INREC,	"R_INREC"  },	/* in recovery, try later	*/
	{ R_FAIL,	"R_FAIL"   },	/* unknown failure		*/
	{ -1,		NULL       }	/* end of list must be NULL	*/
};
static struct errtype Fnlist[] = {
	{ NS_SNDBACK,	"NS_SNDBACK"	},
	{ NS_ADV,	"NS_ADV"	},
	{ NS_UNADV,	"NS_UNADV"	},
	{ NS_GET,	"NS_GET"	},
	{ NS_QUERY,	"NS_QUERY"	},
	{ NS_INIT,	"NS_INIT"	},
	{ NS_SENDPASS,	"NS_SENDPASS"	},
	{ NS_VERIFY,	"NS_VERIFY"	},
	{ NS_MODADV,	"NS_MODADV"	},
	{ NS_BYMACHINE, "NS_BYMACHINE"	},
	{ NS_IQUERY,	"NS_IQUERY"	},
	{ NS_IM_P,	"NS_IM_P"	},
	{ NS_IM_NP,	"NS_IM_NP"	},
	{ NS_FINDP,	"NS_FINDP"	},
	{ NS_REL,	"NS_REL"	},
	{ -1,		NULL	}
};

int
setlog(str)
char	*str;
{
	Loglevel = 0;	/* start fresh	*/
	if (str == NULL)
		return(Loglevel);
	while (*str != '\0')
		Loglevel |= findint(*str++);

	return(Loglevel);
}
static int
findint(ch)
char	ch;
{
	register struct logtype  *place;

	for (place=Loglist; place->l_type != 0; place++)
		if (place->l_symb == ch)
			return(place->l_type);

	return(0);
}
char	*
prtype(t)
{
	return(typetostr(t,Errlist));
}
char	*
fntype(t)
{
	return(typetostr(t,Fnlist));
}
static char *
typetostr(type,list)
int	type;
struct errtype	*list;
{
	int	i;
	static  char	*retval="UNKNOWN";
	for (i=0; list[i].e_type != -1; i++)
		if (list[i].e_type == type)
			return(list[i].e_name);
	return(retval);
}
