/*	@(#)cgiconstants.h 1.1 92/07/30 Copyr 1985-9 Sun Micro		*/

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
 
/*
 * CGI Constants  -- which must match those in "f77/cgidefs77.h"
 */

/* errors */
#define NO_ERROR   0
#define ENOTCGCL   1
#define ENOTCGOP   2
#define	ENOTVSOP   3
#define	ENOTVSAC   4
#define	ENOTOPOP   5
#define	EVSIDINV  10
#define	ENOWSTYP  11
#define	EMAXVSOP  12
#define	EVSNOTOP  13
#define	EVSISACT  14
#define	EVSNTACT  15
#define	EINQALTL  16
#define	EBADRCTD  20
#define	EBDVIEWP  21
#define	ECLIPTOL  22
#define	ECLIPTOS  23
#define	EVDCSDIL  24
#define	EBTBUNDL  30
#define	EBBDTBDI  31
#define	EBTUNDEF  32
#define	EBADLINX  33
#define	EBDWIDTH  34
#define	ECINDXLZ  35
#define	EBADCOLX  36
#define	EBADMRKX  37
#define	EBADSIZE  38
#define	EBADFABX  39
#define	EPATARTL  40
#define	EPATSZTS  41
#define	ESTYLLEZ  42
#define	ENOPATNX  43
#define	EPATITOL  44
#define	EBADTXTX  45
#define	EBDCHRIX  46
#define	ETXTFLIN  47
#define	ECEXFOOR  48
#define	ECHHTLEZ  49
#define	ECHRUPVZ  50
#define	ECOLRNGE  51
#define	ENMPTSTL  60
#define	EPLMTWPT  61
#define	EPLMTHPT  62
#define	EGPLISFL  63
#define	EARCPNCI  64
#define	EARCPNEL  65
#define	ECELLATS  66
#define	ECELLPOS  67
#define	ECELLTLS  68
#define	EVALOVWS  69
#define	EPXNOTCR  70
#define	EINDNOEX  80
#define	EINDINIT  81
#define	EINDALIN  82
#define	EINASAEX  83
#define	EINAIIMP  84
#define	EINNTASD  85
#define	EINTRNEX  86
#define	EINNECHO  87
#define	EINECHON  88
#define	EINEINCP  89
#define	EINERVWS  90
#define	EINETNSU  91
#define	EINENOTO  92
#define	EIAEVNEN  93
#define	EINEVNEN  94
#define	EBADDATA  95
#define	ESTRSIZE  96
#define	EINQOVFL  97
#define	EINNTRQE  98
#define	EINNTRSE  99
#define	EINNTQUE 100
#define	EMEMSPAC 110
#define	ENOTCSTD 111
#define	ENOTCCPW 112
#define	EFILACC  113
#define	ECGIWIN  114

/* devices */
#define BW1DD 	      1
#define BW2DD	      2
#define CG1DD 	      3
#define BWPIXWINDD    4
#define CGPIXWINDD    5
#define GP1DD 	      6
#define CG2DD 	      7
#define CG4DD 	      8
#define PIXWINDD      9
#define CG3DD 	     10
#define CG6DD 	     11

#define VWSURF_NEWFLG  1
#define _CGI_KEYBORDS  1
#define _CGI_LOCATORS  4
#define _CGI_STROKES   3
#define _CGI_VALUATRS  3
#define _CGI_CHOICES   3
#define _CGI_PICKS     3
#define MAXVWS 	       5
#define MAXTRIG        6
#define MAXASSOC       5
#define MAXEVENTS      1024

/* limits */
#define MAXAESSIZE	  10	/* maximum number of AES table entries */
#define MAXNUMPATS	  50	/* maximum number of pattern table entries */
#define MAXPATSIZE	 256	/* maximum pattern size */

#define MAXPTS		1024	/* maximum number of pts per polygon */
#define MAXCHAR		 256	/* maximum number of chars in a string */

#define OUTFUNS		  67	/* number of output functions */
#define INFUNS		  22	/* number of input functions */

/* attributes */

/* fonts */
#define ROMAN		0
#define GREEK		1
#define SCRIPT		2
#define OLDENGLISH	3
#define STICK		4
#define SYMBOLS		5

#define DEVNAMESIZE	20

#define NORMAL_VWSURF(dev,surf) (\
	dev.screenname[0] = '\0', \
	dev.windowname[0] = '\0', \
	dev.windowfd = 0,  \
	dev.retained = 0, \
	dev.dd = surf, \
	dev.cmapsize = 0, \
	dev.cmapname[0] ='\0', \
	dev.flags = 0, \
	dev.ptr= '\0')
