#ifndef lint
static	char sccsid[] = "@(#)mkioconf.c 1.1 92/07/30 SMI"; /* from UCB 2.8 83/06/11 */
#endif

#include <stdio.h>
#include "../../sys/sun/autoconf.h"
#include "y.tab.h"
#include "config.h"

/*
 * build the ioconf.c file
 */
char	*qu();
char	*intv();

#ifdef MACHINE_VAX
vax_ioconf()
{
	register struct device *dp, *mp, *np;
	register int uba_n, slave;
	register struct vlst *vp;
	FILE *fp;

	fp = fopen(path("ioconf.c"), "w");
	if (fp == 0) {
		perror(path("ioconf.c"));
		exit(1);
	}
	fprintf(fp, "#include <machine/pte.h>\n");
	fprintf(fp, "#include <sys/param.h>\n");
	fprintf(fp, "#include <sys/buf.h>\n");
	fprintf(fp, "#include <sys/map.h>\n");
	fprintf(fp, "#include <sys/vm.h>\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include <vaxmba/mbavar.h>\n");
	fprintf(fp, "#include <vaxuba/ubavar.h>\n\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define C (caddr_t)\n\n");
	fprintf(fp, "\n");
	fprintf(fp, "#ifndef lint\n");
	fprintf(fp, "#define INTR_HAND(func)\t\textern func();\n");
	fprintf(fp, "#else lint\n");
	fprintf(fp, "#define INTR_HAND(func)\t\tfunc() { ; }\n");
	fprintf(fp, "#endif lint\n");
	fprintf(fp, "\n\n");

	/*
	 * First print the mba initialization structures
	 */
	if (seen_mba) {
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			    !eq(mp->d_name, "mba"))
				continue;
			fprintf(fp, "extern struct mba_driver %sdriver;\n",
			    dp->d_name);
		}
		fprintf(fp, "\nstruct mba_device mbdinit[] = {\n");
		fprintf(fp, "\t/* Device,  Unit, Mba, Drive, Dk */\n");
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (dp->d_unit == QUES || mp == 0 ||
			    mp == TO_NEXUS || !eq(mp->d_name, "mba"))
				continue;
			if (dp->d_addr != UNKNOWN) {
				printf("can't specify csr address on mba for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_vec != 0) {
				printf("can't specify vector for %s%d on mba\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("drive not specified for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_slave != UNKNOWN) {
				printf("can't specify slave number for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			fprintf(fp, "\t{ &%sdriver, %d,   %s,",
				dp->d_name, dp->d_unit, qu(mp->d_unit));
			fprintf(fp, "  %s,  %d },\n",
				qu(dp->d_drive), dp->d_dk);
		}
		fprintf(fp, "\t0\n};\n\n");
		/*
		 * Print the mbsinit structure
		 * Driver Controller Unit Slave
		 */
		fprintf(fp, "struct mba_slave mbsinit [] = {\n");
		fprintf(fp, "\t/* Driver,  Ctlr, Unit, Slave */\n");
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			/*
			 * All slaves are connected to something which
			 * is connected to the massbus.
			 */
			if ((mp = dp->d_conn) == 0 || mp == TO_NEXUS)
				continue;
			np = mp->d_conn;
			if (np == 0 || np == TO_NEXUS ||
			    !eq(np->d_name, "mba"))
				continue;
			fprintf(fp, "\t{ &%sdriver, %s",
			    mp->d_name, qu(mp->d_unit));
			fprintf(fp, ",  %2d,    %s },\n",
			    dp->d_unit, qu(dp->d_slave));
		}
		fprintf(fp, "\t0\n};\n\n");
	}
	/*
	 * Now generate interrupt vectors for the unibus
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_vec != 0) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			    !eq(mp->d_name, "uba"))
				continue;
			fprintf(fp,
			    "extern struct uba_driver %sdriver;\n",
			    dp->d_name);
			vp = dp->d_vec;
			for (vp = dp->d_vec; vp; vp= vp->v_next)
				fprintf(fp, "INTR_HAND(X%s%d)\n",
				    vp->v_id, dp->d_unit);
			fprintf(fp, "int\t (*%sint%d[])() = { ", dp->d_name,
			    dp->d_unit);
			for (vp = dp->d_vec; ;) {
				fprintf(fp, "X%s%d", vp->v_id, dp->d_unit);
				if (vp->v_vec != 0)
					fprintf(stderr,
					    "vector number for %s ignored\n",
					    vp->v_id);
				vp = vp->v_next;
				if (vp == 0)
					break;
				fprintf(fp, ", ");
			}
			fprintf(fp, ", 0 } ;\n");
		}
	}
	fprintf(fp, "\nstruct uba_ctlr ubminit[] = {\n");
	fprintf(fp, "/*\t driver,\tctlr,\tubanum,\talive,\tintr,\taddr */\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_type != CONTROLLER || mp == TO_NEXUS || mp == 0 ||
		    !eq(mp->d_name, "uba"))
			continue;
		if (dp->d_vec == 0) {
			printf("must specify vector for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_addr == UNKNOWN) {
			printf("must specify csr address for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
			printf("drives need their own entries; dont ");
			printf("specify drive or slave for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_flags) {
			printf("controllers (e.g. %s%d) ",
			    dp->d_name, dp->d_unit);
			printf("don't have flags, only devices do\n");
			continue;
		}
		fprintf(fp,
		    "\t{ &%sdriver,\t%d,\t%s,\t0,\t%sint%d, C 0%o },\n",
		    dp->d_name, dp->d_unit, qu(mp->d_unit),
		    dp->d_name, dp->d_unit, dp->d_addr);
	}
	fprintf(fp, "\t0\n};\n");
/* unibus devices */
	fprintf(fp, "\nstruct uba_device ubdinit[] = {\n");
	fprintf(fp,
"\t/* driver,  unit, ctlr,  ubanum, slave,   intr,    addr,    dk, flags*/\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_unit == QUES || dp->d_type != DEVICE || mp == 0 ||
		    mp == TO_NEXUS || mp->d_type == MASTER ||
		    eq(mp->d_name, "mba"))
			continue;
		np = mp->d_conn;
		if (np != 0 && np != TO_NEXUS && eq(np->d_name, "mba"))
			continue;
		np = 0;
		if (eq(mp->d_name, "uba")) {
			if (dp->d_vec == 0) {
				printf("must specify vector for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr == UNKNOWN) {
				printf("must specify csr for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
				printf("drives/slaves can be specified ");
				printf("only for controllers, ");
				printf("not for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			uba_n = mp->d_unit;
			slave = QUES;
		} else {
			if ((np = mp->d_conn) == 0) {
				printf("%s%d isn't connected to anything ",
				    mp->d_name, mp->d_unit);
				printf(", so %s%d is unattached\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			uba_n = np->d_unit;
			if (dp->d_drive == UNKNOWN) {
				printf("must specify ``drive number'' ");
				printf("for %s%d\n", dp->d_name, dp->d_unit);
				continue;
			}
			/* NOTE THAT ON THE UNIBUS ``drive'' IS STORED IN */
			/* ``SLAVE'' AND WE DON'T WANT A SLAVE SPECIFIED */
			if (dp->d_slave != UNKNOWN) {
				printf("slave numbers should be given only ");
				printf("for massbus tapes, not for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_vec != 0) {
				printf("interrupt vectors should not be ");
				printf("given for drive %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr != UNKNOWN) {
				printf("csr addresses should be given only ");
				printf("on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = dp->d_drive;
		}
		fprintf(fp, "\t{ &%sdriver,  %2d,   %s,",
		    eq(mp->d_name, "uba") ? dp->d_name : mp->d_name, dp->d_unit,
		    eq(mp->d_name, "uba") ? " -1" : qu(mp->d_unit));
		fprintf(fp, "  %s,    %2d,   %s, C 0%-6o,  %d,  0x%x },\n",
		    qu(uba_n), slave, intv(dp), dp->d_addr, dp->d_dk,
		    dp->d_flags);
	}
	fprintf(fp, "\t0\n};\n");
	pseudo_inits(fp);
	(void) fclose(fp);
}
#endif MACHINE_VAX

#if defined(MACHINE_SUN2) || defined(MACHINE_SUN3) || defined(MACHINE_SUN3X) || defined(MACHINE_SUN4) || defined(MACHINE_SUN4C) || defined(MACHINE_SUN4M)

#include <machine/scb.h>

check_vector(vec)
	register struct vlst *vec;
{

	if (vec->v_vec == 0)
		fprintf(stderr, "vector number for %s not given\n", vec->v_id);
	else if (vec->v_vec < VEC_MIN || vec->v_vec > VEC_MAX)
		fprintf(stderr,
			"vector number %d for %s is not between %d and %d\n",
			vec->v_vec, vec->v_id, VEC_MIN, VEC_MAX);
}

/*
 * More than generous..
 */
#define	NUNIQUE	512
sun_ioconf()
{
	register struct device *dp, *mp;
	register struct device *prev;
	register i;
	auto struct device *unique[NUNIQUE];

	register int slave;
	register struct vlst *vp;
	FILE *fp;
	int k = 0;
	int avcnt = 0;
	register int vpos;
	extern char *vec_intrnames[];
	extern int vec_intrcnt;
	static char *extmb = "extern struct mb_driver %sdriver;\n";
	static char *mbdfmt =
"{ &%sdriver,\t%d,  %s,   %2d,     C 0x%08x, %d,   %d, 0x%x, %s, 0x%x },\n";

	fp = fopen(path("ioconf.c"), "w");
	if (fp == 0) {
		perror(path("ioconf.c"));
		exit(1);
	}
	if (devinfo) {		/* devinfo support */
		fprintf(fp, "#include <sun/openprom.h>\n");
		fprintf(fp, "#define	NULL	0\n");
		fprintf(fp, "\n");
	}
	if (mainbus) {
/* old prom & MB support */
		fprintf(fp, "#include <sys/param.h>\n");
		fprintf(fp, "#include <sys/buf.h>\n");
		fprintf(fp, "#include <sys/map.h>\n");
		fprintf(fp, "#include <sys/vm.h>\n");
		fprintf(fp, "\n");
		fprintf(fp, "#include <sundev/mbvar.h>\n");
		fprintf(fp, "\n");
		fprintf(fp, "#define C (caddr_t)\n");
		fprintf(fp, "\n");
		fprintf(fp, "#define INTR_HAND(func, var, val)\t");
		fprintf(fp, 	"int var = val;\textern func();\n");
		fprintf(fp, "\n\n");
	}

	/*
	 * Now generate interrupt vectors for the Mainbus
	 *
	 * Emit this stuff only if "mainbus" is turned on.
	 *
	 * What to avoid:
	 *	- avoid (for cleanliness) printing 'extern mb_driver XXdriver'
	 *	  twice
	 *	- avoid printing INTR_HAND() and struct vec XXint0[] arrays
	 *	  twice.
	 * For cleanliness, we'll do this in two steps-
	 *	- first print out the 'extern struct mb_driver XXdriver'
	 *	  declarations
	 *	- then go through again, producing INTR_HAND struct vec
	 *	  declarations unique for each controller,
	 */

	bzero ((caddr_t) unique, sizeof (struct device *) * NUNIQUE);
	for (dp = dtab; dp != NULL; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp == TO_NEXUS || mp == 0)
			continue;
		if (mp->d_conn != TO_NEXUS && mp->d_type != SCSIBUS)
			continue;
		if (devinfo && (mp->d_type == SCSIBUS))
			continue;
		if (!mainbus) {
			if (mp->d_type == SCSIBUS)
				continue; /* %%% ??? */
			printf("Warning: %s wants an mb_driver structure\n",
				dp->d_name);
			printf("but %s machines don't use them!\n", machinename);
			continue;
		}

		/*
		 * mb_driver declarations need only be
		 * unique with respect to the driver name.
		 */

		for (i = 0; unique[i] != NULL && i < NUNIQUE; i++) {
			if (eq (dp->d_name, unique[i]->d_name)) {
				break;
			}
		}
		if (i >= NUNIQUE) {
			printf("Too many controllers\n");
			exit(2);
		}
		if (unique[i] == NULL) {
			unique[i] = dp;
			fprintf(fp, extmb, dp->d_name);
		}
	}

	bzero ((caddr_t) unique, sizeof (struct device *) * NUNIQUE);
	for (dp = dtab; dp != NULL; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp == TO_NEXUS || mp == 0)
			continue;
		if (mp->d_conn != TO_NEXUS || dp->d_vec == 0)
			continue;
		if (!mainbus)
			continue;
		/*
		 * Declaration of INTR_HAND macros and struct vec
		 * declarations need be unique with respect to controller
		 * name and unit number.
		 */
		for (i = 0; unique[i] != NULL && i < NUNIQUE; i++) {
			if (eq (dp->d_name, unique[i]->d_name) &&
			    dp->d_unit == unique[i]->d_unit) {
				break;
			}
		}
		if (i >= NUNIQUE) {
			printf("Too many controllers\n");
			exit(2);
		}
		if (unique[i])
			continue;
		unique[i] = dp;
		if (dp->d_pri == 0) {
			printf("no priority specified for %s%d\n",
			    dp->d_name, dp->d_unit);
		}
		for (vp = dp->d_vec; vp; vp = vp->v_next) {
			if ((machine == MACHINE_SUN4) || (machine == MACHINE_SUN4M)) {
				fprintf(fp, "INTR_HAND(%s, V%s%d, %d)\n",
				    vp->v_id, vp->v_id, dp->d_unit,
				    dp->d_unit);
			} else {
				fprintf(fp, "INTR_HAND(X%s%d, V%s%d, %d)\n", 
				    vp->v_id, dp->d_unit,
				    vp->v_id, dp->d_unit, dp->d_unit);
			}
		}
		fprintf(fp, "struct vec %s[] = { ", intv(dp));
		for (vp = dp->d_vec; vp != 0; vp = vp->v_next) {
			if ((machine == MACHINE_SUN4) || (machine == MACHINE_SUN4M)) {
				fprintf(fp, "{ %s, %d, &V%s%d }, ",
				    vp->v_id, vp->v_vec, vp->v_id, dp->d_unit);
			} else {
				fprintf(fp, "{ X%s%d, %d, &V%s%d }, ",
				    vp->v_id, dp->d_unit, vp->v_vec,
				    vp->v_id, dp->d_unit);
			}
			check_vector(vp);
		}
		fprintf(fp, "0 };\n");
	}

	/*
	 * Now spew forth the mb_ctlr structures
	 */
	if (mainbus) {
		fprintf(fp, "\nstruct mb_ctlr mbcinit[] = {\n");
		fprintf(fp,
"/* driver,\tctlr,\talive,\taddress,\tintpri,\t intr,\tspace */\n");
	};
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_type != CONTROLLER || mp == TO_NEXUS || mp == 0 ||
		    mp->d_conn != TO_NEXUS)
			continue;
		if (!mainbus) {
			printf("Warning: %s%d wants an mb_ctlr structure but ",
			    dp->d_name, dp->d_unit);
			printf("%s doesn't have mb_ctlr structures.\n", machinename);
			continue;
		}

		if (machine == MACHINE_SUN4M) {
			if (dp->d_bus != SP_VME16D16 &&
			    dp->d_bus != SP_VME24D16 &&
			    dp->d_bus != SP_VME32D16 &&
			    dp->d_bus != SP_VME16D32 &&
			    dp->d_bus != SP_VME24D32 &&
			    dp->d_bus != SP_VME32D32 &&
			    dp->d_bus != SP_IPI) {
				continue;
			}
		}
		if (dp->d_addr == UNKNOWN) {
			printf("must specify csr address for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
			printf("drives need their own entries; ");
			printf("don't specify drive or slave for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_flags) {
			printf("controllers (e.g. %s%d) don't have flags, ",
			    dp->d_name, dp->d_unit);
			printf("only devices do\n");
			continue;
		}
		if (dp->d_pri > 0 && dp->d_vec == 0 && (
		    dp->d_bus == SP_VME16D16 ||
		    dp->d_bus == SP_VME24D16 ||
		    dp->d_bus == SP_VME32D16 ||
		    dp->d_bus == SP_VME16D32 ||
		    dp->d_bus == SP_VME24D32 ||
		    dp->d_bus == SP_VME32D32)) {
			printf("Warning: should use vectored interrupts for ");
			printf("vme device %s%d\n", dp->d_name, dp->d_unit);
		}
		fprintf(fp,
		"{ &%sdriver,\t%d,\t0,\tC 0x%08x,\t%d,\t%s, 0x%x },\n",
		    dp->d_name, dp->d_unit, dp->d_addr, dp->d_pri,
		    intv(dp), (MAKE_MACH(dp->d_mach) | dp->d_bus));
	}
	if (mainbus) {
		fprintf(fp, "\t0\n};\n");
	}

	/*
	 * Now we go for the mb_device stuff
	 */
	if (mainbus) {
		fprintf(fp, "\nstruct mb_device mbdinit[] = {\n");
		fprintf(fp,
"/* driver,\tunit, ctlr, slave, address,      pri, dk, flags, intr, space */\n");
	}
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_unit == QUES || dp->d_type != DEVICE || mp == 0 ||
		    mp == TO_NEXUS || mp->d_type == MASTER)
			continue;
		if (mp->d_type == SCSIBUS) {
			if (mainbus && !devinfo)
				fprintf(fp,mbdfmt,dp->d_name,dp->d_unit,
					" -1",dp->d_slave,0,0,dp->d_dk,0,"0",0);
			continue;
		}
		if (!mainbus) {
			printf(
			"Warning: %s%d wants an mb_device structure but ",
			dp->d_name, dp->d_unit);
			printf("%s doesn't have mb_device structures.\n", machinename);
			continue;
		}
                if (machine == MACHINE_SUN4M) {
                        /*
                         * We assume that if the device is not in VME space,
			 * that it will use an SBus-style driver, in which case
			 * it doesn't need a mbcinit[] or mbdinit[] entry..
                         */
                        if (dp->d_bus != SP_VME16D16 &&
                            dp->d_bus != SP_VME24D16 &&
                            dp->d_bus != SP_VME32D16 &&
                            dp->d_bus != SP_VME16D32 &&
                            dp->d_bus != SP_VME24D32 &&
                            dp->d_bus != SP_VME32D32 &&
                            dp->d_bus != SP_IPI &&
			    dp->d_bus != 0x0)
                                continue;
                }
		if (mp->d_conn == TO_NEXUS) {
			if (dp->d_addr == UNKNOWN) {
				printf("must specify csr for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
				printf("drives/slaves can be specified only ");
				printf("for controllers, not for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_pri > 0 && dp->d_vec == 0 && (
			    dp->d_bus == SP_VME16D16 ||
			    dp->d_bus == SP_VME24D16 ||
			    dp->d_bus == SP_VME32D16 ||
			    dp->d_bus == SP_VME16D32 ||
			    dp->d_bus == SP_VME24D32 ||
			    dp->d_bus == SP_VME32D32)) {
				printf("Warning: should use vectored ");
				printf("interrupts for vme device %s%d\n",
				    dp->d_name, dp->d_unit);
			}
			slave = QUES;
		} else {
			if (mp->d_conn == 0) {
				printf("%s%d isn't connected to anything, ",
				    mp->d_name, mp->d_unit);
				printf("so %s%d is unattached\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("must specify ``drive number'' for %s%d\n",
				   dp->d_name, dp->d_unit);
				continue;
			}
			/* NOTE THAT ON THE UNIBUS ``drive'' IS STORED IN */
			/* ``SLAVE'' AND WE DON'T WANT A SLAVE SPECIFIED */
			if (dp->d_slave != UNKNOWN) {
				printf("slave numbers should be given only ");
				printf("for massbus tapes, not for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_pri != 0) {
				printf("interrupt priority should not be ");
				printf("given for drive %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr != UNKNOWN) {
				printf("csr addresses should be given only");
				printf(" on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = dp->d_drive;
		}
		fprintf(fp,mbdfmt,(mp->d_conn == TO_NEXUS) ? dp->d_name :
			mp->d_name, dp->d_unit, (mp->d_conn == TO_NEXUS) ?
			" -1" : qu(mp->d_unit),	slave,
			(dp->d_addr == UNKNOWN) ? 0 : dp->d_addr,
			dp->d_pri,dp->d_dk, dp->d_flags, intv(dp),
			(MAKE_MACH(dp->d_mach) | dp->d_bus));

		/*
		 * make a list of autovectored names for vmstat
		 * by adding to vectored list (some vectored guys may
		 * be configured as autovectors (e.g., si)).
		 */
		if ((dp->d_pri != 0) && (dp->d_vec == 0)) {
			char name[100];

			sprintf(name, "%s%d", dp->d_name, dp->d_unit);

			/*
			 * if duplicates, there is no easy way to
			 * find out which really interrupts without asking
			 * the driver. Here, we punt and mark the
			 * device as xx? (e.g., zs?)
			 */
			for (k = 1; k <= avcnt; k++) {
				vpos = vec_intrcnt + k - 1;
				if (strncmp(name, vec_intrnames[vpos],
				  strlen(dp->d_name)) == 0) {
					vec_intrnames[vpos][strlen(dp->d_name)] =
					  '?';
					break;
				}
			}
			if ((avcnt == 0) || (k > avcnt)) {
				vpos = vec_intrcnt + avcnt;
				vec_intrnames[vpos] =
				  (char *)malloc(strlen(name) + 1);
				strcpy(vec_intrnames[vpos], name);
				avcnt++;
			}
		}
	}
	if (mainbus && !devinfo) {
		/*
		 * Okay, we have to find all SCSI host adapters and
		 * give them a mb_device entry (so that slave && attach
		 * are called by autoconfig code).
		 */
		bzero ((caddr_t) unique, sizeof (struct device *) * NUNIQUE);
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			if (dp == 0 || dp == TO_NEXUS || dp->d_type != SCSIBUS)
				continue;
			mp = dp->d_conn;
			if(mp == 0 || mp == TO_NEXUS) {
				/*
				 * 'cannot happen'
				 */
				continue;
			}
			for (i = 0; unique[i] && i < 64; i++)
				if(unique[i] == mp)
					break;
			if (i >= 64) {
				printf("Too many SCSI host adapters\n");
				exit(2);
			} else if (unique[i]) {
				continue;
			}
			unique[i] = mp;
			fprintf(fp,mbdfmt,mp->d_name, mp->d_unit,
				qu(mp->d_unit), -1,
				(mp->d_addr == UNKNOWN) ? 0 : mp->d_addr,
				mp->d_pri, mp->d_dk, mp->d_flags, intv(mp),
				(MAKE_MACH(mp->d_mach) | mp->d_bus));
		}
	}
	if (mainbus)
		fprintf(fp, "\t0\n};\n");

/*
 * We really only need the av_nametab[] for non-VME devices.
 * SBus-style drivers maintain this information in a different
 * way.
 */
 	if (mainbus && !devinfo) {
		fprintf(fp, "\nchar *av_nametab[] = {\n");
		for (k = 0; vec_intrnames[k]; k++)
			fprintf(fp, "\t\"%s\",\n", vec_intrnames[k]);
		fprintf(fp, "\t\"\"\n");  /* trick to get end of last string */
		fprintf(fp, "};\n");
		fprintf(fp, "char **av_end = &av_nametab[%d];\n", k);
	}

	/*
	 * We want to produce a dev_ops chain for DEVINFO, but the
	 * original implementer declared this too hard due to, and
	 * I quote, "the limitations of initializing a chain in C".
	 * So, we set up a null terminated table, and the autoconf
	 * routines will patch 'em together at runtime.
	 */
	if (devinfo) {
		for (dp = dtab; dp != 0; dp = dp->d_next)
			if (dp->d_type == DEVICE_DRIVER)
				fprintf(fp,"extern struct dev_ops %s_ops;\n",
					dp->d_name);
		fprintf(fp,"\nstruct dev_opslist av_opstab[] = {\n");
		for (dp = dtab; dp != 0; dp = dp->d_next)
			if (dp->d_type == DEVICE_DRIVER)
				fprintf(fp,"\t{ (struct dev_opslist *)NULL, &%s_ops },\n", dp->d_name);
		fprintf(fp,"\t{ (struct dev_opslist *)NULL, (struct dev_ops *)NULL },\n};\n");
	}
	pseudo_inits(fp);
	scsi_inits(fp);
	(void) fclose(fp);
}
#endif defined(MACHINE_SUN2) || defined(MACHINE_SUN3) || defined(MACHINE_SUN3X) || defined(MACHINE_SUN4) || defined(MACHINE_SUN4C) || defined(MACHINE_SUN4M)

pseudo_inits(fp)
	FILE *fp;
{
	register struct device *dp;
	int count;

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_type != PSEUDO_DEVICE || dp->d_init == 0)
			continue;
		fprintf(fp, "extern %s();\n", dp->d_init);
	}
	fprintf(fp, "\nstruct pseudo_init {\n");
	fprintf(fp, "\tint\tps_count;\n\tint\t(*ps_func)();\n");
	fprintf(fp, "} pseudo_inits[] = {\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_type != PSEUDO_DEVICE || dp->d_init == 0)
			continue;
		count = dp->d_slave;
		if (count <= 0)
			count = 1;
		fprintf(fp, "\t%d,\t%s,\n", count, dp->d_init);
	}
	fprintf(fp, "\t0,\t0,\n};\n");
}

char *
intv(dev)
	register struct device *dev;
{
	static char buf[20];

	if (dev->d_vec == 0)
		return ("     0");
/*
	if (machine == MACHINE_SUN4) {
		(void)sprintf(buf, "%sint", dev->d_name);
	} else {
		(void)sprintf(buf, "%sint%d", dev->d_name, dev->d_unit);
	}
*/
	(void)sprintf(buf, "%sint%d", dev->d_name, dev->d_unit);
	return (buf);
}

char *
qu(num)
{

	if (num == QUES)
		return ("'?'");
	if (num == UNKNOWN)
		return (" -1");
	(void)sprintf(errbuf, "%3d", num);
	return (errbuf);
}

scsi_inits(fp)
	FILE *fp;
{
	extern char *tomacro();
	register struct device *dp, *mp, *mmp;
	auto struct device *unique[64];
	int found, i;
	static char *fmt =
"{ &%s%s,\t&%s%s, \"%s\", \"%s\",\t%d,  %d,  %d,   %3d,   %d },\n";

/* %%% can we handle both devinfo AND mainbus at the same time? */

	found = 0;
	bzero((char *)unique,64 * sizeof (struct device *));

	/*
	 * This 'unique' array serves two purposes:
	 *	For DEVINFO machines it just
	 *	stores up unique target driver names
	 *	so that it doesn't emit more than one
	 *	dev_ops declaration. As a side effect,
	 *	it sees whether or not there are *any*
	 *	scsi busses attached.
	 *
	 *	For mainbus machines, it finds all SCSIBUS
	 *	declarations and saves pointers to them.
	 *	This is because multiple 'scsibusN' tokens
	 *	get entered multiple times. Later, when
	 *	we are emitting the conf table, we can
	 *	then find all the unique controller names
	 *	that map to the same scsi bus.
	 */

	/*
	 * A special note for Sun-4M and other archetectures that
	 * support VME, Sbus, devinfo, openprom, and mainbus, all at
	 * the same time: there seems to be no way to support both
	 * devinfo and mainbus scsi drivers in the same system, so
	 * if we support devinfo and mainbus, then all scsi drivers
	 * must be devinfo, and NOT mainbus.
	 *
	 * If you don't like it, then YOU can fix the scsi drivers.
	 *
	 * Make my day, prove me wrong. :-) :-)
	 */
	if (devinfo) {
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS || mp->d_type != SCSIBUS)
				continue;
			if (!found++)
				fprintf(fp,"\n");
			for (i = 0; unique[i] != 0 && i < 64; i++)
				if (eq(dp->d_name,unique[i]->d_name))
					break;
			if (i == 64) {
				printf("Too many different SCSI drivers\n");
				exit(2);
			}
			if (unique[i])
				continue;
			unique[i] = dp;
			fprintf(fp,"extern struct dev_ops %s_ops;\n",
				dp->d_name);
		}
	} else if (mainbus) {
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			if (dp->d_type != SCSIBUS)
				continue;
			unique[found++] = dp;
			if (found >= 64) {
				printf("Too many SCSIBUS declarations\n");
				exit(2);
			}
		}
	}

	if (!found)
		return;

	fprintf(fp,"\n#include <scsi/conf/autoconf.h>\n");
	fprintf(fp,"#include <scsi/conf/device.h>\n\n");

	fprintf(fp,"\nstruct scsi_conf scsi_conf[] = {\n");
	fprintf(fp,"/*\n");
	fprintf(fp,
" *   H/A         Target   Target H/A  H/A           Unix\n");
	fprintf(fp,
" *  driver       driver    name  name unit tgt lun  unit  busid\n");
	fprintf(fp,
" ****************************************************************/\n");

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (!mp || mp == TO_NEXUS || mp->d_type != SCSIBUS) {
			continue;
		}
		mmp = mp->d_conn;
		if (!mmp) {
			printf("Cannot find device to connect scsibus%d to\n",
				mp->d_unit);
			continue;
		}

		if (devinfo) {
			fprintf(fp, fmt, mmp->d_name, "_ops", dp->d_name,
				"_ops", dp->d_name, mmp->d_name, mp->d_unit,
				dp->d_slave>>3, dp->d_slave&0x7,
				dp->d_unit, mp->d_unit);
		} else if (mainbus) {
			for (i = 0; i < found; i++) {
				mp = unique[i];
				if (mp->d_unit != dp->d_conn->d_unit)
					continue;
				mmp = mp->d_conn;
				fprintf(fp, fmt, mmp->d_name, "driver",
					dp->d_name, "driver", dp->d_name,
					mmp->d_name, mmp->d_unit,
					dp->d_slave>>3, dp->d_slave&0x7,
					dp->d_unit,mp->d_unit);
			}
		}
	}
	fprintf(fp,"{ 0 }\n};\n");
}
