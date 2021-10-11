#ifndef lint
static	char sccsid[] = "@(#)dbx_vm.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/as.h>
#include <vm/mp.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/rm.h>
#include <vm/seg.h>
#include <vm/seg_dev.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/swap.h>
#include <vm/vpage.h>
