/*	@(#)prom_support.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

/*
 * Kernel only promlib functions.  Don't even think about it in other SA's.
 */

extern char *prom_bootpath();
extern char *strncpy();

struct dev_info *
prom_get_boot_devi()
{
	extern struct dev_info *path_to_devi();

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((struct dev_info *)0);

	default:
		return (path_to_devi(prom_bootpath()));
	}
}

int
prom_get_boot_dev_name(p, bufsize)
	char *p;
	int bufsize;
{
	struct dev_info *dip, *path_to_devi();

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		if (bufsize < 3)
			return (-1);
		*p++ = (char)((OBP_V0_BOOTPARAM)->bp_dev[0]);
		*p++ = (char)((OBP_V0_BOOTPARAM)->bp_dev[1]);
		*p   = (char)0;
#ifdef	DPRINTF
		DPRINTF("prom_get_boot_dev_name: <%s>\n", p-2);
#endif	DPRINTF
		return (0);

	default:
		dip = path_to_devi(prom_bootpath());
		if (dip == (struct dev_info *)0)
			return (-1);
		if (bufsize < 3)
			return (-1);
		*(p + bufsize - 1) = (char)0;
		strncpy(p, dip->devi_name, bufsize - 1);
#ifdef	DPRINTF
		DPRINTF("prom_get_boot_dev_name: <%s>\n", p);
#endif	DPRINTF
		return (0);
	}
}

int
prom_get_boot_dev_unit()
{
	struct dev_info *dip;

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
#ifdef	DPRINTF
		DPRINTF("prom_get_boot_dev_unit: unit %d\n",
		    (OBP_V0_BOOTPARAM)->bp_unit);
#endif	DPRINTF
		return ((OBP_V0_BOOTPARAM)->bp_unit);

	default:
		dip = path_to_devi(prom_bootpath());
		if (dip == (struct dev_info *)0)
			return (0);		/* XXX */
#ifdef	DPRINTF
		DPRINTF("prom_get_boot_dev_unit: unit %d\n", dip->devi_unit);
#endif	DPRINTF
		return (dip->devi_unit);
	}
}

int
prom_get_boot_dev_part()
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((OBP_V0_BOOTPARAM)->bp_part);

	default:
		return (get_part_from_path(prom_bootpath()));
	}
}

/*
 *	This just returns a pointer to the option's part of the
 *	last part of the string.  Useful for determining which is
 *	the boot partition, tape file or channel of the DUART.
 */
char *
prom_get_path_option(path)
	char *path;
{
	char *p, *s, *strrchr();

	s = strrchr(path, '/');
	if (s == (char *)0)
		return ((char *)0);
	p = strrchr(s, ':');
	if (p == (char *)0)
		return ((char *)0);
#ifdef	DPRINTF
	DPRINTF("prom_get_path_option: path: <%s> option: <%s>\n", path, p+1);
#endif	DPRINTF
	return (p+1);
}

prom_get_stdin_dev_name(path, len)
	char *path;
	int   len;
{
	struct dev_info *devi;
	char *s, *prom_stdinpath();

	/*
	 * Try the property interface first...
	 * This case handles V3 > (and perhaps later V2?)
	 */

	if ((s = prom_stdinpath()) != (char *)0)  {

#ifdef	DPRINTF
		DPRINTF("stdinpath: <%s>\n", s);
#endif	DPRINTF

		if ((devi = path_to_devi(s)) != 0)  {
			if (len < 3)
				return (-1);
			*(path + len -1) = (char)0;
			strncpy(path, devi->devi_name, len - 1);
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_dev_name: <%s>\n",
			    devi->devi_name);
#endif	DPRINTF
			return (0);
		}
	}

	switch  (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		switch (OBP_V0_INSOURCE)  {
		case INKEYB:
		case INUARTA:
		case INUARTB:
		case INUARTC:
		case INUARTD:
			if (len < 3)
				return (-1);
			strncpy(path, "zs", len);
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_dev_name: <%s>\n", "zs");
#endif	DPRINTF
			return (0);
		}

	}
	return (-1);
}

prom_get_stdout_dev_name(path, len)
	char *path;
	int   len;
{
	struct dev_info *devi;
	char *s, *prom_stdoutpath();

	/*
	 * Try the property interface first...
	 */

	if ((s = prom_stdoutpath()) != (char *)0)  {
		if ((devi = path_to_devi(s)) != 0)  {
			if (len < 3)
				return (-1);
			*(path + len -1) = (char)0;
			strncpy(path, devi->devi_name, len - 1);
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_dev_name: <%s>\n",
			    devi->devi_name);
#endif	DPRINTF
			return (0);
		}
	}

	switch  (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		switch (OBP_V0_OUTSINK)  {
		case OUTUARTA:
		case OUTUARTB:
		case OUTUARTC:
		case OUTUARTD:
			if (len < 3)
				return (-1);
			strncpy(path, "zs", len);
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_dev_name: <%s>\n", "zs");
#endif	DPRINTF
			return (0);
		case OUTSCREEN:
		default:
			return (-1);
		}
	}
	return (-1);
}

/*
 * From zs_async.c which is zs code common to all architectures with zs...
 *
 *	 *
 *	 * The following value mapping lines assume that insource
 *	 * and outsink map as "screen, a, b, c, d, ...", and that
 *	 * zs minors are "a, b, kbd, mouse, c, d, ...".
 *	 *
 *	ri = *romp->v_insource;
 *	ro = *romp->v_outsink;
 *	un = zs->zs_unit;
 *	un = (un<2) ? un+1 : (un<4) ? 0 : un-1;
 */

int
prom_get_stdin_unit()
{
	struct dev_info *devi;
	char *s, *prom_stdinpath();

	if ((s = prom_stdinpath()) != (char *)0)  {
		if ((devi = path_to_devi(s)) != 0)  {
#ifdef	DPRINTF
		    DPRINTF("prom_get_stdin_unit: unit <%d>, reg <%x.%x.%x>\n",
			devi->devi_unit,
			devi->devi_nodeid,
			devi->devi_reg->reg_bustype,
			devi->devi_reg->reg_addr, devi->devi_reg->reg_size);
#endif	DPRINTF
			return (devi->devi_unit);
		}
	}

	switch (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
#ifdef	DPRINTF
		DPRINTF("prom_get_stdin_unit: insource = %d\n",
		    OBP_V0_INSOURCE);
#endif	DPRINTF
		switch (OBP_V0_INSOURCE)  {
		case INKEYB:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_unit: unit <1>\n");
#endif	DPRINTF
			return (1);
		case INUARTA:
		case INUARTB:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_unit: unit <0>\n");
#endif	DPRINTF
			return (0);
		case INUARTC:
		case INUARTD:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_unit: unit <2>\n");
#endif	DPRINTF
			return (2);
		default:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_unit: unit <unknown>!\n");
#endif	DPRINTF
			return (-1);
		}

	}
	return (-1);
}

int
prom_get_stdout_unit()
{
	struct dev_info *devi;
	char *s, *prom_stdoutpath();


	if ((s = prom_stdoutpath()) != (char *)0)  {
		if ((devi = path_to_devi(s)) != 0)  {
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_unit: unit <%d>\n",
				devi->devi_unit);
#endif	DPRINTF
		return (devi->devi_unit);
		}
	}

	switch (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		switch (OBP_V0_INSOURCE)  {
		case OUTUARTA:
		case OUTUARTB:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_unit: unit <0>\n");
#endif	DPRINTF
			return (0);
		case OUTUARTC:
		case OUTUARTD:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_unit: unit <2>\n");
#endif	DPRINTF
			return (2);
		case OUTSCREEN:
		default:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_unit: unit <unknown>!\n");
#endif	DPRINTF
			return (-1);
		}
	}
	return (-1);
}

char *
prom_get_stdin_subunit()
{
	char *p, *prom_stdinpath();

	if ((p = prom_stdinpath()) != (char *)0)  {
		p = prom_get_path_option(p);
#ifdef	DPRINTF
		DPRINTF("prom_get_stdin_subunit: <%s>\n", p ? p : "unknown!");
#endif	DPRINTF
		return (p);
	}

	switch (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		switch (OBP_V0_INSOURCE)  {
		case INKEYB:
		case INUARTA:
		case INUARTC:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_subunit: <a>\n");
#endif	DPRINTF
			return ("a");		/* Assumes zs style option */
		case INUARTB:
		case INUARTD:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_subunit: <b>\n");
#endif	DPRINTF
			return ("b");		/* Assumes zs style option */
		default:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdin_subunit: <unknown>!\n");
#endif	DPRINTF
			return ((char *)0);
		}

	}
	return ((char *)0);
}

char *
prom_get_stdout_subunit()
{
	char *p;

	if ((p = prom_stdoutpath()) != (char *)0)  {
		p = prom_get_path_option(p);
#ifdef	DPRINTF
		DPRINTF("prom_get_stdout_subunit: <%s>\n", p ? p : "unknown!");
#endif	DPRINTF
		return (p);
	}

	switch (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		switch (OBP_V0_INSOURCE)  {
		case OUTUARTA:
		case OUTUARTC:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_subunit: <a>\n");
#endif	DPRINTF
			return ("a");		/* Assumes zs style option */
		case OUTUARTB:
		case OUTUARTD:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_subunit: <b>\n");
#endif	DPRINTF
			return ("b");		/* Assumes zs style option */
		default:
#ifdef	DPRINTF
			DPRINTF("prom_get_stdout_subunit: <unknown!>\n");
#endif	DPRINTF
			return ((char *)0);
		}
	}
	return ((char *)0);
}

int
prom_stdin_stdout_equivalence()
{
	char *s, *p, *prom_stdinpath(), *prom_stdoutpath();

	s = prom_stdinpath();
	p = prom_stdoutpath();

	if ((s != (char *)0) && (p != (char *)0))  {
#ifdef	DPRINTF
		DPRINTF("prom_stdin_stdout_equivalence <%s>\n",
		    (strcmp(s, p) == 0 ? "true": "false"));
#endif	DPRINTF
		return (strcmp(s, p) == 0? 1:0);
	}

	switch (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		if ((OBP_V0_OUTSINK != OUTSCREEN) &&
		    (OBP_V0_INSOURCE == OBP_V0_OUTSINK))
			return (1);
	}
	return (0);
}
