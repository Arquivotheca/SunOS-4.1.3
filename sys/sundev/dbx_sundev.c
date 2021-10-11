#ifndef lint
static	char sccsid[] = "@(#)dbx_sundev.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/time.h>

#include <sundev/mbvar.h>
#include <sundev/vuid_event.h>
#include <sundev/dmctl.h>

#include "ar.h"
#if NAR > 0
#include <sundev/arreg.h>
#endif NAR > 0

#include "bwtwo.h"
#if NBWTWO > 0
#include <sundev/bw2reg.h>
#endif NBWTWO > 0

#include "des.h"
#if NDES > 0
#include <sundev/desreg.h>
#endif NDES > 0

#include "fpa.h"
#if NFPA > 0
#include <sundev/fpareg.h>
#endif NFPA >0

#include "kb.h"
#if NKB > 0
#include <sundev/kbd.h>
#include <sundev/kbdreg.h>
#include <sundev/kbio.h>
#endif NKB > 0

#include "ms.h"
#if NMS > 0
#include <sundev/msreg.h>
#endif NMS > 0

#include "mti.h"
#if NMTI > 0
#include <sundev/mtireg.h>
#endif NMTI > 0

#include "sky.h"
#if NSKY > 0
#include <sundev/skyreg.h>
#endif NSKY > 0

#include "sc.h"
#include "si.h"
#if NSI > 0 || NSC > 0
#include <sun/dkio.h>
#include <sundev/sireg.h>	/* XXX - scsi.h shouldn't require sireg.h */
#include <sundev/screg.h>	/* XXX - scsi.h shouldn't require screg.h */
#include <sundev/scsi.h>
#endif NSC > 0 || NSC > 0

#include "st.h"
#if NST > 0
#include <sundev/streg.h>
#endif NST > 0

#include "mt.h"
#if NMT > 0
#include <sundev/tmreg.h>
#endif NMT > 0

#include "tod.h"
#if NTOD > 0
#include <sundev/todreg.h>
#endif NTOD > 0

#include "vp.h"
#if NVP > 0
#include <sundev/vpreg.h>
#endif NVP > 0

#include "vpc.h"
#if NVPC > 0
#include <sundev/vpcreg.h>
#endif NVPC > 0

#include "xt.h"
#if NXT > 0
#include <sundev/xycreg.h>
#include <sundev/xtreg.h>
#endif NXT > 0

#include "xd.h"
#if NXD > 0
#include <sun/dkio.h>
#include <sundev/xdvar.h>
#include <sundev/xderr.h>
#endif NXD > 0

#include "xy.h"
#if NXY > 0
#include <sun/dkio.h>
#include <sundev/xyvar.h>
#include <sundev/xyerr.h>
#endif NXY > 0

#include "zs.h"
#if NZS > 0
#include <sundev/zscom.h>
#include <sundev/zsreg.h>
#include <sundev/zsvar.h>
#endif NZS > 0
