/*	@(#)prom_outpath.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

dnode_t	prom_nextnode();
int	prom_getprop();

static char stdoutpath[OBP_MAXDEVNAME];

char *
prom_stdoutpath()
{
	dnode_t rootid;

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((char *)0);

	default:
		if (stdoutpath[0] != (char)0)		/* Got it already? */
			return (stdoutpath);

		rootid = prom_nextnode((dnode_t)0);
		if (prom_getprop(rootid, OBP_STDOUTPATH, stdoutpath) != -1)  {
#ifdef	PROM_DEBUG_STDPATH
			prom_printf("%s: <%s> from root node %s property!\n",
			    "prom_stdoutpath", stdoutpath,  OBP_STDOUTPATH);
#endif
			return (stdoutpath);
		}

#ifdef	notdef
		/*
		 * XXX: Remove the following when all V3 machines are upgraded!
		 */

		if ((obp_romvec_version == OBP_V3_ROMVEC_VERSION) &&
		    (romp->op3_stdout_path != (char **)0))  {
			strncpy(stdoutpath, OBP_V3_STDOUTPATH,
			    sizeof (stdoutpath));
#ifdef	PROM_DEBUG_STDPATH
			prom_printf("XXX: %s: <%s> from Romvec stdoutpath!\n",
			    "prom_stdoutpath", stdoutpath);
#endif
			return (stdoutpath);
		}
#endif	notdef

		return ((char *)0);
	}
}
