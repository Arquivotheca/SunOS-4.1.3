static char sccsid[] = "@(#)probe_check.c	1.1  7/30/92   Sun Microsystems, Inc.";

#include <stdio.h>
#include <errno.h>
#ifdef SVR4
#include <string.h>
#else SVR4
#include <strings.h>
#endif SVR4
#include <kvm.h>
#include <nlist.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/conf.h>
#ifndef SVR4
#include <sys/dir.h>
#endif SVR4
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/vmmac.h>
#ifdef SVR4
#include <sys/cpu.h>
#include <sys/param.h>
#include <vm/anon.h>
#include <sys/dkio.h>
#ifndef SVR4
#include <sys/screg.h>
#include <sys/scsi.h>
#endif SVR4
#include <sys/sireg.h>
#else SVR4
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/param.h>
#include <machine/pte.h>
#include <vm/anon.h>
#include <sun/dkio.h>
#include <sundev/mbvar.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#include "openprom.h"
#include "prestoioctl.h"
#endif SVR4


#include "probe.h"
#include "../../lib/include/probe_sundiag.h"
#include "sdrtns.h"
#include "../../lib/include/libonline.h"


extern struct mb_device *mbdinit;         
static kvm_t    *mem;

extern char *get_cpuname();
extern int get_physmem(), get_vmem();
#ifdef sun3
extern int get_fpp(), get_fpu();
#endif
#ifdef sun4
extern int get_fpa();
#endif
extern void get_scsi_names(), get_devices();
extern void get_drivers();

struct  found_dev found[MAXDEVS];
int     devno = 0;
char    tmp_devdir[NAMELEN];
char    buff[200];
char    *tmpname="/tmp/sundiag.XXXXXX";
extern  char *mktemp();

void save_mem(), save_dev();
static void save_tape_dev(), save_disk_dev();
void check_dev_files();

#ifdef sun386
#include <signal.h>
#include <setjmp.h>
jmp_buf error_buf;
#endif

/*
 * check_dev_files(makedevs, fpa_status) - checks the device files,
 * or otherwise determines that the device is valid, and then saves the info
 * for all of the devices in mbdinit which are "alive" - note that "alive"
 * is not terribly meaningful for SCSI disks and tapes, but otherwise valid
 */
void
check_dev_files(makedevs, fpa_exist, dev_name)
    int makedevs;
    int fpa_exist;		/* LINT MESSAGE */
    char *dev_name;
{
  struct mb_devinfo *mdc;
  struct mb_device *md;
  struct mb_driver *mdr;
  int	 k, amt, cname, status, type, result, mem_fd, processors_mask;
  char	 *devicename;
  short  t_type;
  unsigned	arch_code;

  func_name = "check_dev_files"; 
  TRACE_IN
  /* create tmp device directory */
  buff[0] = '\0';
  mktemp(tmpname);
#ifndef SVR4
  sprintf(buff,"mkdir %s;cp /dev/MAKEDEV %s;", tmpname, tmpname);
#else SVR4
  sprintf(buff,"mkdir %s;", tmpname);
#endif SVR4
  system(buff);
  strcpy(tmp_devdir, tmpname); /* copy tmp filename into global variable */

  arch_code = sun_arch();
  if (arch_code != ARCH4)		/* if OpenBoot Prom type CPU's */
  {

    if (arch_code != ARCH4C) {		/* if MP machines */
        processors_mask = get_processors_mask();
        if ((k=get_number_processors(processors_mask)) > 1)
        save_dev(MP4, k, 0);
    }

    for (mdc = (struct mb_devinfo *)&mbdinfo[1], k = 0; k < numdev; k++, mdc++)
    {
	if ((dev_name !=(char *)NULL) && strcmp(mdc->info.devi_name, dev_name)) 
		continue;		/* look for the specific device */
	   
	if (!mdc->info.devi_driver)	/* this is the same as alive */
		continue;

	send_message (0, VERBOSE, "device %s unit %d", mdc->info.devi_name,
		mdc->info.devi_unit);

	devicename = mdc->info.devi_name;
	if (strncmp(devicename, "SUNW,", 5) == 0)
	  devicename += 5;		/* skip the "SUNW," */

	/* SPARCstation dumb color frame buffer */
        if (strcmp(devicename, CGTHREE) == 0) {
            if (check_cgthree_dev(makedevs, &type, mdc->info.devi_unit))
                save_dev(CGTHREE, mdc->info.devi_unit, type);
            continue;
        }
        /* prism type frame buffer */
        if (strcmp(devicename, CGFOUR) == 0) {
            if (check_cgfour_dev(makedevs, &type, mdc->info.devi_unit))
                save_dev(CGFOUR, mdc->info.devi_unit, type);
            continue;
        }
	/* lego frame buffer */
        if (strcmp(devicename, CGSIX) == 0) {
	    if (check_cgsix_dev(makedevs, &type, mdc->info.devi_unit))
		save_dev(CGSIX, mdc->info.devi_unit, type);
	    continue;
	}
        /* egret graghic accelerator */
        if (strcmp(devicename, CGTWELVE) == 0) {
            if (check_cgtwelve_dev(makedevs, &type, mdc->info.devi_unit))
                save_dev(CGTWELVE, mdc->info.devi_unit, type);
            continue;
        }
        /* hawk2 graghic tower */
        if (strcmp(devicename, GT) == 0) {
            if (check_cgtwelve_dev(makedevs, &type, mdc->info.devi_unit))
                save_dev(GT, mdc->info.devi_unit, type);
            continue;
        }
	/* ethernet */
        if (strcmp(devicename, IE) == 0) {
            if ((check_net(IE, mdc->info.devi_unit)) == NETUP)
                save_dev(IE, mdc->info.devi_unit, NETUP);
            continue;
        }
	/* lance */
        if ((strcmp(devicename, LE) == 0) &&
             (strcmp(devicename, "lebuffer") != 0) ) {
            if ((check_net(LE, mdc->info.devi_unit)) == NETUP)
                save_dev(LE, mdc->info.devi_unit, NETUP);
            continue;
        }
	/* scsi disk */
        if (strcmp(devicename, SD) == 0) {
            if ((check_disk_dev(makedevs, SD, mdc->info.devi_unit, &amt,&cname))
								== DISKOK)
                save_disk_dev(SD, mdc->info.devi_unit, amt, DISKOK,
				mdc->info.devi_parent->devi_name, 
				mdc->info.devi_parent->devi_unit, cname);
            continue;
        }
	/* floppy disk (Campus 2) */
        if (arch_code != ARCH4C && strcmp(devicename, FDTWO) == 0) {
            if ((result=check_disk_dev(makedevs, FD, mdc->info.devi_unit,
                        &amt,&cname)) != DISKPROB)
                save_disk_dev(FD, mdc->info.devi_unit, amt, result,
                                "None", -1, cname);
            continue;
        }
	/* floppy disk */
        if (strcmp(devicename, FD) == 0) {
            if ((result=check_disk_dev(makedevs, FD, mdc->info.devi_unit,
			&amt,&cname)) != DISKPROB)
                save_disk_dev(FD, mdc->info.devi_unit, amt, result,
				"None", -1, cname);
            continue;
        }
	/* CD ROM */
        if (strcmp(devicename, CDROM) == 0) {
            if ((result=check_cdrom_dev(makedevs, CDROM, mdc->info.devi_unit,
				&amt, &cname)) != DISKPROB)
                save_disk_dev(CDROM, mdc->info.devi_unit, amt, result,
			mdc->info.devi_parent->devi_name,
			mdc->info.devi_parent->devi_unit, cname);
            continue;
        }
	/* scsi tape */
        if (strcmp(devicename, ST) == 0) {
            if ((status = check_st_dev(makedevs, mdc->info.devi_unit, &t_type)))
                save_tape_dev(ST, mdc->info.devi_unit, status,
				mdc->info.devi_parent->devi_name,
				mdc->info.devi_parent->devi_unit, t_type);
            continue;
        }
	/* parallel port */
        if (strcmp(devicename, PP) == 0) {
            if (check_printer_dev(makedevs, mdc->info.devi_unit))
                save_dev(PP, mdc->info.devi_unit, 0);
            continue;
        }
        /* audio port */
        if (strcmp(devicename, AUDIO) == 0) {
            if (check_audio_dev(0, mdc->info.devi_unit, &t_type))
                save_dev(AUDIO, mdc->info.devi_unit, t_type);
            continue;
        }
        /* dbri port */
        if (strncmp(devicename, DBRI, 4) == 0) {
            if (check_audbri_dev(0, mdc->info.devi_unit, &t_type))
                save_dev(AUDIO, mdc->info.devi_unit, t_type);
            continue;
        }
	/* serial port */
        if (strcmp(devicename, ZS) == 0) {
            if (check_sp_dev(makedevs, mdc->info.devi_unit))
                save_dev(ZS, mdc->info.devi_unit, 0);
            continue;
        }
        if (strcmp(devicename, ZEBRA1) == 0) {
		save_dev(ZEBRA1, mdc->info.devi_unit, 0);
		continue;
	}
        /* zebra2 lpvi (serial video port -- for image printing) */ 
        if (strcmp(devicename, ZEBRA2) == 0) {
                save_dev(ZEBRA2, mdc->info.devi_unit, 0); 
                continue; 
        }
	/* snapshot vfc (video frame capture -- taking a frame from a camera)*/
        if (strcmp(devicename, VFC) == 0) {
                save_dev(VFC, mdc->info.devi_unit, 0);
                continue;
        }
	/* sun2/3/4 black and white monocrome */
        if (strcmp(devicename, BWTWO) == 0) {
            if (check_bwtwo_dev(makedevs, &type, mdc->info.devi_unit))
                save_dev(BWTWO, mdc->info.devi_unit, type);
            continue;
        }
	/* fddi device */
        if (strcmp(devicename, FDDI) == 0) {
            if ((check_net(FDDI, mdc->info.devi_unit)) == NETUP)
                save_dev(FDDI, mdc->info.devi_unit, NETUP);
            continue;
        }
	/* token ring */
        if (strcmp(devicename, TRNET) == 0) {
            if ((check_net(TRNET, mdc->info.devi_unit)) == NETUP)
                save_dev(TRNET, mdc->info.devi_unit, NETUP);
            continue;
        }
	/* OMNI (Interface's Network Coprocessor) ethernet */
        if (strcmp(devicename, OMNI_NET) == 0) {
            if ((check_net(OMNI_NET, mdc->info.devi_unit)) == NETUP)
                save_dev(OMNI_NET, mdc->info.devi_unit, NETUP);
            continue;
        }
	/* SPIF serial parallel Interface port */
        if (strcmp(devicename, SPIF) == 0) {
                save_dev(SPIF, mdc->info.devi_unit, 1);
            if (check_spif_dev(makedevs, mdc->info.devi_unit))
            continue;
        }
        /* hss high speed port (VME bus) */
        if (strcmp(devicename, HSS) == 0) {
            if (check_hss_dev(makedevs, mdc->info.devi_unit))
                save_dev(HSS, mdc->info.devi_unit, 0);
            continue;
        }
	/* SBus high speed port */
        if (strcmp(devicename, HSI) == 0) {
            if (check_hss_dev(makedevs, mdc->info.devi_unit))
                save_dev(HSI, mdc->info.devi_unit, 0);
            continue;
        }
        if (strcmp(devicename, XD) == 0) {
            if ((check_disk_dev(makedevs, XD,mdc->info.devi_unit, &amt, &cname))                                                                == DISKOK)
                save_disk_dev(XD, mdc->info.devi_unit, amt, DISKOK,
                        mdc->info.devi_parent->devi_name,
                        mdc->info.devi_parent->devi_unit, cname);
            continue;
        }
        if (strcmp(devicename, XT) == 0) {
            if ((status = check_mt_dev(makedevs, XT, mdc->info.devi_unit,
                    &t_type)) != 0)
                save_tape_dev(XT, mdc->info.devi_unit, status,
                        mdc->info.devi_parent->devi_name,
                        mdc->info.devi_parent->devi_unit, t_type);
            continue;
        }
        if (strcmp(devicename, XY) == 0) {
            if ((check_disk_dev(makedevs, XY,mdc->info.devi_unit, &amt, &cname))                                                                == DISKOK)
                save_disk_dev(XY, mdc->info.devi_unit, amt, DISKOK,
                        mdc->info.devi_parent->devi_name,
                        mdc->info.devi_parent->devi_unit, cname);
            continue;
        }
        if (strcmp(devicename, IP) == 0) {
            if ((check_disk_dev(makedevs, IP,mdc->info.devi_unit, &amt, &cname))                                                                == DISKOK)
                save_disk_dev(IP, mdc->info.devi_unit, amt, DISKOK,
                        mdc->info.devi_parent->devi_name,
                        mdc->info.devi_parent->devi_unit, cname);
            continue;
        }
        if (strcmp(devicename, ID) == 0) {
            if ((check_disk_dev(makedevs, ID,mdc->info.devi_unit, &amt, &cname))                                                                == DISKOK)
                save_disk_dev(ID, mdc->info.devi_unit, amt, DISKOK,
                        mdc->info.devi_parent->devi_name,
                        mdc->info.devi_parent->devi_unit, cname);
            continue;
        }
        if (strcmp(devicename, MCP) == 0) {
            if ((status = check_mcp_dev(makedevs,mdc->info.devi_unit)) != 0)
                save_dev(MCP, mdc->info.devi_unit, status);
            continue;
        }
        if (strcmp(devicename, MT) == 0) {
            if ((status = check_mt_dev(makedevs, MT, mdc->info.devi_unit,
                    &t_type)) != 0)
                save_tape_dev(MT, mdc->info.devi_unit, status,
                        mdc->info.devi_parent->devi_name,
                        mdc->info.devi_parent->devi_unit, t_type);
            continue;
        }
        if (strcmp(devicename, MTI) == 0) {
            if (check_mti_dev(makedevs, mdc->info.devi_unit))
                save_dev(MTI, mdc->info.devi_unit, 0);
            continue;
        }
        if (strcmp(devicename, TAAC) == 0) {
            if (check_taac_dev(makedevs, &type, mdc->info.devi_unit))
                save_dev(TAAC, mdc->info.devi_unit, 0);
            continue;
        }
        /* Sbus prestoserve */
        if (strcmp(devicename, PR) == 0) {
            if (check_presto_dev(makedevs))
                save_mem(PR, get_presto_mem());
            continue;
        }
    }
  }
  else				/* if not OpenBoot Prom type CPU's */
  {
    for (md=mbdinit; mdr=md->md_driver; md++) {
        if ((dev_name != (char *)NULL) && strcmp(mdr->mdr_dname, dev_name))
            continue;			/* look for the specific device */
        if (!md->md_alive)
            continue;
	if (mdr->mdr_dname == NULL)
	    continue;			/* just in case */

	send_message(0, VERBOSE, "device %s%d attached", mdr->mdr_dname,
		md->md_unit);

        if (strcmp(mdr->mdr_dname, BWTWO) == 0) {
            if (check_bwtwo_dev(makedevs, &type, md->md_unit))
                save_dev(BWTWO, md->md_unit, type);
            continue;
        }

        if (strcmp(mdr->mdr_dname, CGONE) == 0) {
            if (check_cgone_dev(makedevs, &type, md->md_unit))
                save_dev(CGONE, md->md_unit, type);
            continue;
        }
		/* cg2, cg3, cg5 */
        if (strcmp(mdr->mdr_dname, CGTWO) == 0) {
            if (check_cgtwo_dev(makedevs, &type, md->md_unit))
                save_dev(CGTWO, md->md_unit, type);
            continue;
        }
		/* sun-386i frame buffer */
        if (strcmp(mdr->mdr_dname, CGTHREE) == 0) {
            if (check_cgthree_dev(makedevs, &type, md->md_unit))
                save_dev(CGTHREE, md->md_unit, type);
            continue;
        }
		/* prism type frame buffer */
        if (strcmp(mdr->mdr_dname, CGFOUR) == 0) {
            if (check_cgfour_dev(makedevs, &type, md->md_unit))
                save_dev(CGFOUR, md->md_unit, type);
            continue;
        }
		/* sun-386i RR frame buffer */
        if (strcmp(mdr->mdr_dname, CGFIVE) == 0)
        {
            if (check_cgfive_dev(makedevs, &type, md->md_unit))
                save_dev(CGFIVE, md->md_unit, type);
            continue;
        }
		/* lego frame buffer */
	if (strcmp(mdr->mdr_dname, CGSIX) == 0) {
	    if (check_cgsix_dev(makedevs, &type, md->md_unit))
		save_dev(CGSIX, md->md_unit, type);
	    continue;
	}
		/* ibis 24 bit frame buffer */
        if (strcmp(mdr->mdr_dname, CGEIGHT) == 0) {
            if (check_cgeight_dev(makedevs, &type, md->md_unit))
                save_dev(CGEIGHT, md->md_unit, type);
            continue;
        }
		/* crane frame buffer */
	if (strcmp(mdr->mdr_dname, CGNINE) == 0) {
	    if (check_cgnine_dev(makedevs, &type, md->md_unit))
		save_dev(CGNINE, md->md_unit, type);
	    continue;
	}
        	/* egret graghic accelerator */
        if (strcmp(mdr->mdr_dname, CGTWELVE) == 0) {
            if (check_cgtwelve_dev(makedevs, &type, md->md_unit))
                save_dev(CGTWELVE, md->md_unit, type);
            continue;
        }
                /* hawk2 graghic tower */
        if (strcmp(mdr->mdr_dname, GT) == 0) {
            if (check_cgtwelve_dev(makedevs, &type, md->md_unit))
                save_dev(GT, md->md_unit, type);
            continue;
        }
		/* flamingo */
	if (strcmp(mdr->mdr_dname, TVONE) == 0) {
	    if (check_tvone_dev(makedevs, &type, md->md_unit))
		save_dev(TVONE, md->md_unit, type);
	    continue;
	}
        if (strcmp(mdr->mdr_dname, DCP) == 0) {
            if (check_dcp_dev(makedevs, md->md_unit))
	        save_dev(DCP, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, DES) == 0) {
            if (check_des_dev(makedevs))
                save_dev(DES, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, FDDI) == 0) {
	    if ((check_net(FDDI, md->md_unit)) == NETUP)
	        save_dev(FDDI, md->md_unit, NETUP);
            continue;
        }
        if (strcmp(mdr->mdr_dname, TRNET) == 0) {
	    if ((check_net(TRNET, md->md_unit)) == NETUP)
	        save_dev(TRNET, md->md_unit, NETUP);
            continue;
        }
        if (strcmp(mdr->mdr_dname, OMNI_NET) == 0) {
	    if ((check_net(OMNI_NET, md->md_unit)) == NETUP)
	        save_dev(OMNI_NET, md->md_unit, NETUP);
            continue;
        }
        if (strcmp(mdr->mdr_dname, GPONE) == 0) {
            if ((type = check_gpone_dev(makedevs)) != 0)
                save_dev(GPONE, md->md_unit, type);
            continue;
        }
        if (strcmp(mdr->mdr_dname, HSS) == 0) {
            if (check_hss_dev(makedevs, md->md_unit))
                save_dev(HSS, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, HSI) == 0) {
            if (check_hss_dev(makedevs, md->md_unit))
                save_dev(HSI, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, IE) == 0) {
	    if ((check_net(IE, md->md_unit)) == NETUP)
	        save_dev(IE, md->md_unit, NETUP);
            continue;
        }
        if (strcmp(mdr->mdr_dname, IP) == 0) {
            if ((check_disk_dev(makedevs, IP, md->md_unit, &amt, &cname))
								== DISKOK)
                save_disk_dev(IP, md->md_unit, amt, DISKOK, mdr->mdr_cname,
                    md->md_ctlr, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, ID) == 0) {
            if ((check_disk_dev(makedevs, ID, md->md_unit, &amt, &cname))
								== DISKOK)
                save_disk_dev(ID, md->md_unit, amt, DISKOK, mdr->mdr_cname,
                    md->md_ctlr, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, LE) == 0) {
	    if ((check_net(LE, md->md_unit)) == NETUP)
	        save_dev(LE, md->md_unit, NETUP);
            continue;
        }
        if (strcmp(mdr->mdr_dname, MCP) == 0) {
            if ((status = check_mcp_dev(makedevs, md->md_unit)) != 0)
	        save_dev(MCP, md->md_unit, status);
            continue;
        }
        if (strcmp(mdr->mdr_dname, MT) == 0) {
            if ((status = check_mt_dev(makedevs, MT, md->md_unit, 
		    &t_type)) != 0)
                save_tape_dev(MT, md->md_unit, status, mdr->mdr_cname,
                    md->md_ctlr, t_type);
            continue;
        }
        if (strcmp(mdr->mdr_dname, MTI) == 0) {
            if (check_mti_dev(makedevs, md->md_unit))
	        save_dev(MTI, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, IPC) == 0) {
            if (check_pc_dev(makedevs, md->md_unit))
	        if (check_net(ME, 0))
		    save_dev(IPC, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, SD) == 0) {
            if ((check_disk_dev(makedevs, SD, md->md_unit, &amt, &cname))
								== DISKOK)
                save_disk_dev(SD, md->md_unit, amt, DISKOK, mdr->mdr_cname,
                    md->md_ctlr, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, SF) == 0) {
            if ((result=check_disk_dev(makedevs, SF, md->md_unit,
				&amt, &cname)) != DISKPROB)
                save_disk_dev(SF, md->md_unit, amt, result, mdr->mdr_cname,
                    md->md_ctlr, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, FD) == 0) {
            if ((result=check_disk_dev(makedevs, FD, md->md_unit,
				&amt, &cname)) != DISKPROB)
                save_disk_dev(FD, md->md_unit, amt, result, "None", -1, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, CDROM) == 0) {
            if ((result=check_cdrom_dev(makedevs, CDROM, md->md_unit,
				&amt, &cname)) != DISKPROB)
                save_disk_dev(CDROM, md->md_unit, amt, result,
			mdr->mdr_cname, md->md_ctlr, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, ST) == 0) {
            if ((status = check_st_dev(makedevs, md->md_unit, &t_type)) != 0)
                save_tape_dev(ST, md->md_unit, status, mdr->mdr_cname,
                    md->md_ctlr, t_type);
            continue;
        }
        if (strcmp(mdr->mdr_dname, TAAC) == 0) {
            if ((type = check_taac_dev(makedevs, &type, md->md_unit)) != 0)
                save_dev(TAAC, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, PP) == 0) {
            if (check_printer_dev(makedevs, md->md_unit))
                save_dev(PP, md->md_unit, 0);
            continue;
        }
        if (strcmp(mdr->mdr_dname, PR) == 0) {
            if (check_presto_dev(makedevs)) {
                save_mem(PR, get_presto_mem());
	    }
            continue;
        }
        if (strcmp(mdr->mdr_dname, XD) == 0) {
            if ((check_disk_dev(makedevs, XD, md->md_unit, &amt, &cname))
								== DISKOK)
                save_disk_dev(XD, md->md_unit, amt, DISKOK, mdr->mdr_cname,
                    md->md_ctlr, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, XT) == 0) {
            if ((status = check_mt_dev(makedevs, XT, md->md_unit, 
		    &t_type)) != 0)
                save_tape_dev(XT, md->md_unit, status, mdr->mdr_cname,
                    md->md_ctlr, t_type);
            continue;
        }
        if (strcmp(mdr->mdr_dname, XY) == 0) {
            if ((check_disk_dev(makedevs, XY, md->md_unit, &amt, &cname))
								== DISKOK)
                save_disk_dev(XY, md->md_unit, amt, DISKOK, mdr->mdr_cname,
                    md->md_ctlr, cname);
            continue;
        }
        if (strcmp(mdr->mdr_dname, ZS) == 0) {
            if (check_sp_dev(makedevs, md->md_unit))
                save_dev(ZS, md->md_unit, 0);
            continue;
        }
    }    
  }
  buff[0] = '\0';
  sprintf(buff,"rm -rf %s", tmpname);
  system(buff);
  TRACE_OUT
}


#ifndef SVR4
/* lifted from pstest.c */
int
get_presto_mem()
{
	int	  prfd;
	int	  res;
        struct presto_status prstatus;

char *open1_err_msg = "Failed to open prestoserve board: %s\n\
Possible causes:\n\
   1) Prestoserve hardware not installed.\n\
   2) Board not fully seated.\n\
   3) Prestoserve software not installed.\n\
   4) Not in superuser mode.";

char *prgetstatus_err_msg = "Checking prstatus failed: %s";
char *state_err_msg = "Prestoserve is in the ERROR state.\n\
Possible causes:\n\
    1) Errors have occured on a disk drive.";

        func_name = "get_presto_mem";
        TRACE_IN
 

        if ((prfd = open(PRDEV, O_RDWR)) == -1) {
                send_message (0, WARNING, open1_err_msg, errmsg(errno));
		TRACE_OUT
		return(0);
        } /* end of if */

        /* Check state of the board. */
        if ((res = ioctl(prfd, PRGETSTATUS, &prstatus)) == -1) {
                send_message(0, WARNING,
                        prgetstatus_err_msg, errmsg(errno));
		TRACE_OUT
		return(0);
	} /* end of if */

        /* Check for error state. */
        if (prstatus.pr_state == PRERROR) {
                send_message(0, WARNING, state_err_msg);
		TRACE_OUT
		return(0);
        } /* end of if */
 
        (void) send_message(0, VERBOSE, "prestoserve memory size= %d KB",
                prstatus.pr_maxsize/1024);

	TRACE_OUT
	return (prstatus.pr_maxsize/1024);
}
#endif SVR4

int
get_mp_number()
{
    int i, bit, mask, memfd;
 
    func_name = "get_mp_number";
    TRACE_IN
 
    memfd = open("/dev/null", 0);
/***********
    ioctl(memfd, MIOCGPAM, &mask);
***********/
    mask = 0x0F;
    close (memfd);
 
    i = 0;
    for (bit = 1; bit; bit <<= 1) {
        if (mask & bit) {
            i++;
        }
    }
 
    TRACE_OUT
    return(i);
 
}
/*
 * save_mem(name, amt) - saves device name, sets unit to 0, sets utype 
 * to MEM_DEV, saves amt of memory in mem_info struct, and increments devno
 */
void
save_mem(name, amt)
    char *name;
    int amt;
{
    func_name = "save_mem"; 
    TRACE_IN
    found[devno].device_name = name;
    found[devno].unit = 0;
    found[devno].u_tag.utype = MEM_DEV;
    found[devno].u_tag.uval.meminfo.amt = amt;
    devno++;
    TRACE_OUT
}

/*
 * save_dev(name, unit, status) - saves device name and unit, sets utype 
 * to GENERAL_DEV, saves status in dev_info struct, and increments devno
 */
void
save_dev(name, unit, status)
    char *name;
    int unit;
    int status;
{
    func_name = "save_dev";
    TRACE_IN
    if (strcmp(name, "spif") == 0)
      found[devno].device_name = "stc";
    else if (strncmp(name, DBRI, 4) == 0)
      found[devno].device_name = "dbri";
    else 
      found[devno].device_name = name;
    found[devno].unit = unit;
    found[devno].u_tag.utype = GENERAL_DEV;
    found[devno].u_tag.uval.devinfo.status = status;
    devno++;
    TRACE_OUT
}

/*
 * save_tape_dev(name, unit, status, ctlr, ctlr_num, t_type) - saves 
 * device name, unit, status, controller, controller number, and tape type,
 * sets utype to TAPE_DEV, saves device status in tape_info struct, and 
 * increments devno
 */
static void
save_tape_dev(name, unit, status, ctlr, ctlr_num, t_type)
    char *name;
    int unit;
    int status;
    char *ctlr;
    int ctlr_num;
    short t_type;
{
    func_name = "save_tape_dev";
    TRACE_IN
    found[devno].device_name = name;
    found[devno].unit = unit;
    found[devno].u_tag.utype = TAPE_DEV;
    found[devno].u_tag.uval.tapeinfo.status = status;
    found[devno].u_tag.uval.tapeinfo.ctlr = ctlr;
    found[devno].u_tag.uval.tapeinfo.ctlr_num = ctlr_num;
    found[devno].u_tag.uval.tapeinfo.t_type = t_type;
    devno++;
    TRACE_OUT
}

/*
 * save_disk_dev(name, unit, status, ctlr, ctlr_num) - saves device name, 
 * unit, status, controller, and controller number, sets utype to DISK_DEV, 
 * saves device status in disk_info struct, and increments devno
 */
static void
save_disk_dev(name, unit, amt, status, ctlr, ctlr_num, cname)
    char *name;
    int unit;
    int amt;
    int status;
    char *ctlr;
    int ctlr_num;
    int cname;
{
    func_name = "save_disk_dev";
    TRACE_IN
    found[devno].device_name = name;
    found[devno].unit = unit;
    found[devno].u_tag.utype = DISK_DEV;
    found[devno].u_tag.uval.diskinfo.amt = amt;
    found[devno].u_tag.uval.diskinfo.status = status;
    found[devno].u_tag.uval.diskinfo.ctlr = ctlr;
    found[devno].u_tag.uval.diskinfo.ctlr_num = ctlr_num;
    found[devno].u_tag.uval.diskinfo.ctlr_type = cname;
    devno++;
    TRACE_OUT
}

