/*
 * %Z%%M% %I% %E% Copyr 1987 Sun Micro
 *
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "mktp.h"

distribution_info dinfo;/* distribution information		*/
volume_info vinfo;	/* volume info - not used until actually*/
			/* making the distribution		*/
toc_entry entries[NENTRIES];	/* toc entry table		*/
toc_entry *ep;		/* pointer to one above last toc entry	*/
