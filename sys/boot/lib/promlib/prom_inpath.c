/*	@(#)prom_inpath.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

dnode_t	prom_nextnode();
int	prom_getprop();

static char stdinpath[OBP_MAXDEVNAME];

char *
prom_stdinpath()
{
	dnode_t rootid;

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((char *)0);

	default:
		if (stdinpath[0] != (char)0)		/* Got it already? */
			return (stdinpath);

		rootid = prom_nextnode((dnode_t)0);
		if (prom_getprop(rootid, OBP_STDINPATH, stdinpath) != -1)  {
#ifdef	PROM_DEBUG_STDPATH
			prom_printf("%s: <%s> from root node %s property!\n",
			    "prom_stdinpath", stdinpath,  OBP_STDINPATH);
#endif
			return (stdinpath);
		}

#ifdef	notdef
		/*
		 * XXX: Remove me when all V3 machines are upgraded!
		 * XXX: V3 was to have romvec entries for stdinpath/stdoutpath
		 * XXX: We changed this to root node properties!
		 */

		if ((obp_romvec_version == OBP_V3_ROMVEC_VERSION) &&
		    (romp->op3_stdin_path != (char **)0))  {
			strncpy(stdinpath, OBP_V3_STDINPATH,
			sizeof (stdinpath));
#ifdef	PROM_DEBUG_STDPATH
			prom_printf("XXX: %s: <%s> from Romvec stdinpath!\n",
			    "prom_stdinpath", stdinpath);
#endif
			return (stdinpath);
		}
#endif	notdef

		return ((char *)0);
	}
}
