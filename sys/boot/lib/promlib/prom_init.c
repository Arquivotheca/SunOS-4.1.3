/*	@(#)prom_init.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

#ifdef	DPRINTF
int	promlib_debug = 2;	/* 0 = quiet, 1 = VPRINTF only, 2 = debug */
#endif	DPRINTF

int	obp_romvec_version = -1;  /* -1 rsrvd for non-obp sunromvec */

/*
 * XXX: This is a hack to allow boot without /boot present
 * Remove when debugging is completed.
 */
#ifdef	notdef
static char *defaultpath =
	"/iommu@f,e0000000/sbus@f,e0001000/lebuffer@f,40000/le@f,60000";
static char fakepath[OBP_BOOTBUFSIZE] = {(char)0};
#endif	notdef

extern void prom_enter_mon();

void
prom_init(pgmname)
	char *pgmname;
{
	if (romp->op_magic != OBP_MAGIC)  {
		prom_printf("PROM Magic Number");
		prom_enter_mon();
	}

	obp_romvec_version = romp->op_romvec_version;

#ifdef	notdef
	/* XXX: DEBUG/BRINGUP MODE ONLY */
	prom_printf("%s: libprom.a: Romvec Version %d\n", pgmname,
	    obp_romvec_version);

	if (obp_romvec_version > 0)  {
		if ((OBP_V2_BOOTPATH == (char *)0) ||
		    (strcmp(OBP_V2_BOOTPATH, "") == 0))  {
			extern int boothowto;

			if (strlen(fakepath) == 0)
				(void)strcpy(fakepath, defaultpath);
			if (OBP_V2_BOOTPATH == (char *)0)  {
#ifdef	DPRINTF
				DPRINTF("prom_init: op_bootpath is null!\n");
#endif	DPRINTF
				OBP_V2_BOOTPATH = fakepath;
			} else
				(void)strcpy(OBP_V2_BOOTPATH, fakepath);
			prom_printf("prom_init: setting bootpath to <%s>!\n",
				OBP_V2_BOOTPATH);
#ifdef	notdef
			/* Enable this to force a boot -a */
			prom_printf("setting boothowto to 0x103 - ask mode!\n");
			boothowto = 0x103;
#endif	notdef
		}

#ifdef	VPRINTF
		VPRINTF("libprom.a: bootpath <%s>\n",
			OBP_V2_BOOTPATH ? OBP_V2_BOOTPATH : "UNKNOWN!");
#endif	VPRINTF
	}
#endif	notdef
}
