#ifndef lint
static  char sccsid[] = "@(#)probe_nlist.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <kvm.h>
#include <nlist.h>
#include <signal.h>

#ifdef SVR4
#include <dirent.h>
#include <string.h>
#else SVR4
#include <strings.h>
#endif SVR4

#include <fcntl.h>
#include <sys/types.h>
#include <sys/conf.h>
#ifndef SVR4
#include <mon/openprom.h>
#include <sys/dir.h>
#include <sys/param.h>
#endif SVR4
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/vmmac.h>
#ifdef SVR4
#include <sys/cpu.h>
#include <sys/param.h>
#else SVR4
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/param.h>
#include <machine/pte.h>
#endif SVR4

#include <vm/anon.h>
#ifdef SVR4
#include <sys/dkio.h>
#include <sys/sireg.h>
#else SVR4
#include <sun/dkio.h>
#include <sundev/mbvar.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#endif SVR4
#include "probe.h"
#include "../../lib/include/probe_sundiag.h"
#include "sdrtns.h"
#include "../../lib/include/libonline.h"

#define NL_PHYSMEM      0
#define NL_ROMP         1
#define NL_PHYSMEMORY   2
#define NL_ANONINFO     3
#define NL_MBDINIT      4
#define NL_SCSINTYPE    5
#define NL_SCSIUNITSUBR 6
#define FPUEXISTS       7       
#define NL_TOPDEVINFO   8       
#define NL_LAST         9

#ifndef sun386
struct nlist nl[] = {
#ifdef SVR4
        { "cputype", 0, 0, 0, 0, 0 },
        {"physmem", 0, 0, 0, 0, 0 },
        {"anoninfo", 0, 0, 0, 0, 0 },
        { "mbdinit", 0, 0, 0, 0, 0 },
        { "scsi_ntype", 0, 0, 0, 0, 0 },
        { "scsi_unit_subr", 0, 0, 0, 0, 0 },
        {"fpu_exists", 0, 0, 0, 0, 0 },
        {"top_devinfo",0, 0, 0, 0, 0 },
        { "" },
#else SVR4
        {"_physmem", 0, 0, 0, 0 },
        { "_romp", 0, 0, 0, 0 },
        { "_physmemory", 0, 0, 0, 0 },
        {"_anoninfo", 0, 0, 0, 0 },
        { "_mbdinit", 0, 0, 0, 0 },
        { "_scsi_ntype", 0, 0, 0, 0 },
        { "_scsi_unit_subr", 0, 0, 0, 0 },
#ifdef sun3
        {"_fppstate", 0, 0, 0, 0 },
        {"_fpa_exist", 0, 0, 0, 0 },
        { "" },
#endif
#ifdef sun4
        {"_fpu_exists", 0, 0, 0, 0 },
        {"_top_devinfo",0, 0, 0, 0 },
        { "" },
#endif
#endif SVR4
};
 
#else   sun386
#define NL_CPU          0
#define NL_PHYSMEM      1
#define NL_SANON        2
#define NL_MBDINIT      3
#define NL_SCSINTYPE    4
#define NL_SCSIUNITSUBR 5
struct nlist nl[] = {
        { "cpu", 0, 0, 0, 0 },
        { "physmem", 0, 0, 0, 0 },
        { "anoninfo", 0, 0, 0, 0 },
        { "mbdinit", 0, 0, 0, 0 },
        { "scsi_ntype", 0, 0, 0, 0 },
        { "scsi_unit_subr", 0, 0, 0, 0 },
        { "" },
};
#include <setjmp.h>
jmp_buf error_buf;
#endif  sun386

struct sunromvec  *sunrom, romvec;
struct memlist    **pm, *pmlist, pmem;
extern int devno;
static int cpu_type;
extern struct found_dev found[MAXDEVS];

/* for SPARCstation 1 only */
int    numdev=0;
struct mb_devinfo mbdinfo[MAXDEVS];

extern kvm_t *kvm_open();
extern char *malloc();
extern char *calloc();

extern long lseek();

extern void save_mem(), save_dev();
extern void check_dev_files();
void get_c_devices();

#define MAX_SCSI_DEV_NAME_LENGTH 3
 
struct mb_device *mbdinit;
static struct mb_driver *scdriver;
static int scsi_ntype;
static struct scsi_unit_subr *scsi_unit_subr=(struct scsi_unit_subr *)0;
static kvm_t	*mem;
static char *get_cpuname();
static int get_physmem(), get_vmem();
static int openboot_prom=FALSE, nlvar=0;

#ifdef SVR4
static int get_fpu();
static long _fsr_temp;
#else SVR4
/* #ifdef sun3 */
static int get_fpp();
/* #endif */
/* #ifdef sun4 */
static long	fsr_temp;
static int get_fpa();
/* #endif */
static void get_scsi_names(), get_devices();
static void get_drivers();
#endif SVR4

/*********************************************************************
        This sun_probe routines can probe any device as specified by
        user, and return TRUE/FALSE to indicate device availability.
        This routine also provide for some information about the
        probed device, and error message if failed.
*********************************************************************/
int
sun_probe(dev_name, f_dev, makedevs)
char *dev_name;                 /* device to probe */
struct f_devs *f_dev;
int  makedevs;
{
    char *vmunix, *getenv();
    char *cpuname;
    int physmem, vmem;
    int fpp, fpa, fpu;
    char error_msg[BUFSIZ];
 
#ifdef sun386
    int bus(), segv();
#endif
 
    func_name = "sun_probe";
    TRACE_IN
    check_superuser();
 
/* begin probe */
 
    vmunix = getenv("KERNELNAME");
    if ((mem = kvm_open(vmunix, NULL, NULL, O_RDONLY, NULL)) == NULL) {
        send_message(1, FATAL, "kvm_open: %s, kvm_open failed", errmsg(errno)); 
    }  
    if (kvm_nlist(mem, nl) == -1) {
        send_message(1, FATAL,"kvm_nlist: %s, kvm_nlist failed", errmsg(errno));
    }  
#ifndef SVR4
    if (sun_arch() != ARCH4)
      openboot_prom = TRUE;

    if (!check_nlist()) {
        send_message(1, FATAL, "no namelist");
    }  
#endif SVR4
 
#ifdef SVR4
	get_c_devices(mem);
	cpuname = get_cpuname(mem);
	physmem = get_physmem(mem);
	vmem = get_vmem(mem);
	fpu = get_fpu(mem);
#else SVR4
	cpuname = get_cpuname(mem);
	physmem = get_physmem(mem);
	vmem = get_vmem(mem);
    if (sun_arch() != ARCH4)
        get_c_devices(mem);
    else {
      get_scsi_names(mem);
      get_devices(mem);
      get_drivers(mem);
    }
    fpu = get_fpu(mem);
#endif SVR4
 
    (void)kvm_close(mem);
 
/* end of probe */
 
    f_dev->cputype = cpu_type;
    f_dev->cpuname = cpuname;
    save_mem(MEM, physmem);
    save_mem(KMEM, vmem);
    if (fpu) save_dev(FPUU, 0, fpu);
 
#ifndef	SVR4
    check_dev_files(makedevs, fpa, dev_name);
#endif	SVR4
    f_dev->num = devno;
    f_dev->found_dev = &found[0];
    TRACE_OUT
    return (devno);

}                   
 
 
static int
check_nlist(error_msg)
char *error_msg;
{                
  unsigned arch_code;

    func_name = "check_nlist";
    TRACE_IN
    arch_code = sun_arch();
    if(arch_code == ARCH4) {
        if (nl[NL_MBDINIT].n_type == 0 ) {
		TRACE_OUT
                return(0); /* FATAL */
        }
    }
    TRACE_OUT
    return(1);
}

/*
 * from autoconf.c:
 * We set the cpu type and associated variables.  Should there get to
 * be too many variables, they should be collected together in a
 * structure and indexed by cpu type.
 */
static char *
get_cpuname(mem)
    int mem;
{
    static char *cpuname;

#ifdef SVR4
  	struct mb_devinfo *mdc;
	mdc = mbdinfo;
#endif SVR4

    func_name = "get_cpuname";
    TRACE_IN
    if ((cpuname = malloc(MAXNAMLEN)) == NULL) {
        send_message(1, FATAL, "malloc cpuname: %s", errmsg(errno)); 
    }
    (void) strcpy(cpuname, "Unknown machine type");   /* default CPU type */
#ifndef SVR4
    switch (cpu_type = sun_cpu()) {
#ifdef sun3
    case CPU_SUN3X_470:
        (void) strcpy(cpuname, "Sun-3X/460,3X/470,3X/480");
	/* they can be 460, 470, or 480 */
        break;
    case CPU_SUN3X_80:
        (void) strcpy(cpuname, "Sun-3X/80");
        break;
    case CPU_SUN3_160:
        (void) strcpy(cpuname, "Sun-3/75, 3/160, 3/180");
        break;
    case CPU_SUN3_50:
        (void) strcpy(cpuname, "Sun-3/50");
        break;
    case CPU_SUN3_260:
        (void) strcpy(cpuname, "Sun-3/260, 3/280");
        break;
    case CPU_SUN3_110:
        (void) strcpy(cpuname, "Sun-3/110");
        break;
    case CPU_SUN3_60:
        (void) strcpy(cpuname, "Sun-3/60");
        break;
    case CPU_SUN3_E:
        (void) strcpy(cpuname, "Sun-3/E");
        break;
    default:
        (void) strcpy(cpuname, "Sun-3, unknown type");
        break;
#endif

#ifdef sun4
    case CPU_SUN4_890:
	(void) strcpy(cpuname, "Sun-4/890");
	break;
    case CPU_SUN4_460:
        (void) strcpy(cpuname, "Sun-4/470, 4/490");
        break;
    case CPU_SUN4_330:
        (void) strcpy(cpuname, "Sun-4/330, 4/370, 4/390");
        break;
    case CPU_SUN4_260:
        (void) strcpy(cpuname, "Sun-4/260, 4/280");
        break;
    case CPU_SUN4_110:
        (void) strcpy(cpuname, "Sun-4/110");
        break;
    case CPU_SUN4C_60:
	(void) strcpy(cpuname, "SPARCstation 1");
	break;
    case CPU_SUN4C_40:
	(void) strcpy(cpuname, "Sun-4/40");
	break;
    case CPU_SUN4C_65:
	(void) strcpy(cpuname, "SPARCstation 1+");
	break;
    case CPU_SUN4C_20:
	(void) strcpy(cpuname, "Sun-4/20");
	break;
    case CPU_SUN4C_75:
	(void) strcpy(cpuname, "Sun-4/75");
	break;
    case CPU_SUN4C_30:
	(void) strcpy(cpuname, "Sun-4/30");
	break;
    case CPU_SUN4C_50:
	(void) strcpy(cpuname, "Sun-4/50");
	break;
    case GALAXY:
        (void) strcpy(cpuname, "SPARCsystem 600 MP");
        break;
    case C2:
        (void) strcpy(cpuname, "SPARCstation 10");
        break;
    default:
	switch (sun_arch())
	{
	  case ARCH4:
	    (void)strcpy(cpuname, "Sun-4, unknown type");
	    break;
	  case ARCH4M:
	    (void)strcpy(cpuname, "Sun-4m, unknown type");
	    break;
	  default:
            (void)strcpy(cpuname, "Sun-4c, unknown type");
	}
#endif
#ifdef  sun386
    case CPU_SUN386_150:
        (void) strcpy(cpuname, "Sun-386i");
        break;
    case 32:	/* do this properly with define ASAP */ 
        (void) strcpy(cpuname, "Sun-486i");
        break;
    default:
	(void) strcpy(cpuname, "Sun-x86i, unknown type");
        break;
#endif
    } 
#else SVR4
	strcpy(cpuname, mdc->info.devi_name);
#endif SVR4
    send_message (0, VERBOSE, "cpuname = %s", cpuname);
#ifndef SVR4
    send_message (0, VERBOSE, "cputype = %x", cpu_type);
#endif SVR4
    TRACE_OUT
    return(cpuname);
}

/*
 * from machdep.c:
 * memory size in pages, patch if you want less
 * If physmem is patched to be non-zero, use it instead of
 * the monitor value unless physmem is larger than the total
 * amount of memory on hand.
 * Adjust physmem down for the pages stolen by the monitor.
 * Comment: how can a user find out how many pages the monitor has?
 */
static int
get_physmem(mem)
    int mem;
{
    int physmem = 0;

    func_name = "get_physmem";
    TRACE_IN
    nlvar = openboot_prom ? NL_ROMP : NL_PHYSMEM;
    if (openboot_prom) {
      if (kvm_read(mem, (u_long)nl[nlvar].n_value, (char *)&sunrom,
        sizeof(struct sunromvec *)) != sizeof(struct sunromvec *))
        send_message(1, FATAL, "sunromvec Failed: %s", errmsg(errno));

      if (kvm_read(mem, (u_long)sunrom, (char *)&romvec,
        sizeof(struct sunromvec)) != sizeof(struct sunromvec)) {
        send_message(1, FATAL, "romvec Failed: %s", errmsg(errno));
      }  

      if (romvec.op_romvec_version > 0) /* Calvin uses OpenBootProm V2.0 */
      {  
        kvm_read(mem, (u_long)nl[NL_PHYSMEMORY].n_value, (char *)&pmlist,
          sizeof(struct memlist *));
      }  
      else {
        pm = romvec.v_physmemory;
        kvm_read(mem, (u_long)pm, (char *)&pmlist, sizeof(struct memlist *));
      }  
 
      while (pmlist) {    
        kvm_read(mem,(u_long)pmlist,(char *)&pmem, sizeof(struct memlist));
        physmem += pmem.size;
        pmlist = pmem.next;
      }
      physmem = physmem / 1024; /* size in KB */
      send_message (0, VERBOSE, "physmem = %dK", physmem);

  }  
  else {
    kvm_read(mem, nl[NL_PHYSMEM].n_value, (char *)&physmem, sizeof(physmem));
    physmem = (getpagesize() * physmem) / 1024; /* size in KB */
    send_message (0, VERBOSE, "physmem = %dK", physmem);
  }
    TRACE_OUT
    return(physmem);
}

#ifdef sun386

#define ctok(x) ((ctob(x)) >> 10)
static int
get_vmem(mem)
    int mem;
{
    struct anoninfo ai;
    int vmem;
 
    func_name = "get_vmem";
    TRACE_IN
    kvm_read(mem, nl[NL_SANON].n_value, (char *)&ai, sizeof(struct
anoninfo));
    vmem = ctok(ai.ani_max);
    send_message (0, VERBOSE, "vmem = %dK", vmem);
    TRACE_OUT
    return(vmem);
}

#else	sun386

static int
get_vmem(mem)
    int mem;
{
    struct anoninfo ai;
    int vmem;

    func_name = "get_vmem";
    TRACE_IN
    kvm_read(mem, nl[NL_ANONINFO].n_value, (char *)&ai,sizeof(struct anoninfo));
    vmem = (ai.ani_max*getpagesize()) >> 10;	/* in the unit of K bytes */
    send_message (0, VERBOSE, "vmem = %dK", vmem);
    TRACE_OUT
    return(vmem);
}

#endif	sun386

#ifndef SVR4
/*
 * from mark opperman's probe program:
 */
static void
get_scsi_names(mem)
    int mem;
{
    char *devname;
    int i;

    func_name = "get_scsi_names";
    TRACE_IN
    if (nl[NL_SCSINTYPE].n_value) {
        kvm_read(mem, nl[NL_SCSINTYPE].n_value, (char *)&scsi_ntype, 
	    sizeof(scsi_ntype));
        scsi_unit_subr = (struct scsi_unit_subr *) 
	    calloc((unsigned) scsi_ntype, 
	    (unsigned) sizeof(struct scsi_unit_subr));
        if (scsi_unit_subr == NULL) {
            send_message(1, FATAL,"calloc scsi_unit_subr table: %s", 
		errmsg(errno)); 
        }
        kvm_read(mem, nl[NL_SCSIUNITSUBR].n_value, (char *)scsi_unit_subr,
            (u_int) (scsi_ntype * sizeof(struct scsi_unit_subr)));
	send_message (0, DEBUG, "scsi_ntype = %d", scsi_ntype);
        for (i=0; i < scsi_ntype; i++) {
            devname = malloc((unsigned)MAX_SCSI_DEV_NAME_LENGTH);
            if (devname == NULL) {
                send_message(1, FATAL, "malloc devname: %s", errmsg(errno)); 
	    }
            kvm_read(mem, (u_long) scsi_unit_subr[i].ss_devname, devname,
                MAX_SCSI_DEV_NAME_LENGTH);
            scsi_unit_subr[i].ss_devname = devname;
	    send_message (0, DEBUG, "scsi_unit_subr[%d].ss_devname = %s",
			i, devname);
        }
    }    
    TRACE_OUT
}

/*
 * from mark opperman's probe and Guy Harris's devs programs:
 */
static void
get_devices(mem)
    int mem;
{
    struct mb_device mb_device;
    u_long dev_addr;
    int ndev;

    func_name = "get_devices";
    TRACE_IN
    for (dev_addr = nl[NL_MBDINIT].n_value, ndev=0; ;ndev++) {
        kvm_read(mem, dev_addr, (caddr_t) &mb_device, sizeof(struct mb_device));
        if (mb_device.md_driver == NULL)
            break;
        dev_addr += sizeof(struct mb_device);
    }
    mbdinit = (struct mb_device *) calloc((unsigned) ndev+1, 
	(unsigned) sizeof(struct mb_device));
    if (mbdinit == NULL) {
        send_message(1, FATAL, "calloc mbdinit table: %s", errmsg(errno)); 
    }
    kvm_read(mem, nl[NL_MBDINIT].n_value, (caddr_t) mbdinit,
        (u_int) (ndev * sizeof(struct mb_device)));
    mbdinit[ndev].md_driver = (struct mb_driver *) NULL;
    send_message (0, VERBOSE, "ndev = %d", ndev);
    TRACE_OUT
}
 
/*
 * from mark opperman's probe and Guy Harris's devs programs:
 */
static void
get_drivers(mem)
    int mem;
{
    struct mb_device *md;
    struct mb_driver *mdr;
    u_long drv_addr;
    char name[16];
 
    func_name = "get_drivers";
    TRACE_IN
    for (md=mbdinit; md->md_driver != NULL; md++ ) {
        mdr = (struct mb_driver *) malloc((unsigned) sizeof(struct mb_driver));
	if (mdr == NULL) {
            send_message(1, FATAL, "malloc mdr: %s", errmsg(errno)); 
	}
        drv_addr = (unsigned long) md->md_driver;
        kvm_read(mem, drv_addr, (caddr_t) mdr, sizeof(struct mb_driver));
        md->md_driver = mdr;
        if (mdr->mdr_dname != NULL) {
            kvm_read(mem, (u_long) mdr->mdr_dname, (char *) name, sizeof(name));
            if (((strcmp(name, SD) == 0) || (strcmp(name, SF) == 0) ||
                (strcmp(name, ST) == 0) || (strcmp(name, CDROM) == 0))
		&& scsi_unit_subr)
                mdr->mdr_dname =
                    scsi_unit_subr[TYPE(md->md_flags)].ss_devname;
            else {
                mdr->mdr_dname = malloc((unsigned) strlen(name)+1);
		if (mdr->mdr_dname == NULL) {
            	    send_message(1,FATAL,"malloc mdr_dname: %s", errmsg(errno));
		}
                (void) strcpy(mdr->mdr_dname, name);
            }
        }
        if (mdr->mdr_cname != NULL) {
            kvm_read(mem, (u_long) mdr->mdr_cname, (char *) name, sizeof(name));
	    mdr->mdr_cname = malloc((unsigned) strlen(name)+1);
	    if (mdr->mdr_cname == NULL) {
            	send_message(1, FATAL, "malloc mdr_cname: %s", errmsg(errno));
	    }
            (void) strcpy(mdr->mdr_cname, name);
        }
    }
    TRACE_OUT
}
 
#ifdef sun3 
/*
 * from Locore.c:
 * 0 = no 81, 1 = 81 enable, -1 = 81 disabled
 */
static int
get_fpp(mem)
    int mem;
{
    short fppstate;
 
    func_name = "get_fpp";
    TRACE_IN
    kvm_read(mem, nl[FPPSTATE].n_value, &fppstate, sizeof(fppstate));
    if (fppstate == -1)
	send_message(0, INFO, "mc68881 disabled");
    TRACE_OUT
    return((int)fppstate);
}

/*
 * from fpa.c:
 * Fpa_exist == 0 if FPA does not exist.  Fpa_exist == 1 if there is a
 * functioning FPA attached.  Fpa_exist == -1 if the FPA is disabled
 * due to FPA hardware problems.
 */
static int
get_fpa(mem)
    int mem;
{
    short fpa_exist;
 
    func_name = "get_fpa";
    TRACE_IN
    kvm_read(mem, nl[FPA_EXIST].n_value, &fpa_exist, sizeof(fpa_exist));
    send_message (0, DEBUG, "fpa_exist = %d", fpa_exist);
    if (fpa_exist == -1) 
        send_message(0, INFO, "FPA disabled");
    TRACE_OUT
    return((int)fpa_exist);
}

#endif

#endif SVR4
 
#ifdef sun4
/*
 * from Locore.c: flag for fpu existence
 *
 * get_fpu() returns:	 0 if fpu does not exist.
 *			-1 if fpu is disabled.
 *			 1 if functioning fpu exists.
 *			 2 if functioning fpu2 exists.
 *		
 * Currently (4/89), the Sunray/Stingray use the Cypress IU; their fsr
 * identify the rev number of the fpc and not the type of fpu.  The Sunrise/
 * Cobra machines use either the fpu or fpu2, as identified in their fsr.
 */
static int
get_fpu(mem)
    int mem;
{
    int			fpu_exists;
    int			fpu_type;	/* fpu or fpu2 */
    unsigned long	cputype;	/* Sunray/Stingray don't have fpu2 */
 
    func_name = "get_fpu";
    TRACE_IN
    kvm_read(mem, nl[FPUEXISTS].n_value, (char *)&fpu_exists,
						sizeof(fpu_exists));
    switch(fpu_exists) {
    case 1:
	cputype = sun_cpu();
	if ((cputype == CPU_SUN4_260) || (cputype == CPU_SUN4_110) ||
		(cputype == CPU_SUN4_890)) {
	    fpu_type = ((get_fsr() >> 17) & 0x3);  /* check for fpu or fpu2 */
	    if (fpu_type == FPU2_EXIST)
		fpu_exists = FPU2_EXIST;
	}
	break;
    case 0:
	break;
    case -1:
	send_message(0, INFO, "fpu disabled");
	break;
    }
    TRACE_OUT
    return(fpu_exists);
}

/*
 * get_fsr():  returns the contents of the fsr.
 */
get_fsr()
{
    asm("set	_fsr_temp, %l0"); /* set the address of the result holder */
    asm("st	%fsr, [%l0]");	  /* store the contents of FSR in fsr_temp */
#ifdef SVR4
    return(_fsr_temp);	  	  /* return the fsr to caller */
#else SVR4
    return(fsr_temp);	  	  /* return the fsr to caller */
#endif SVR4
}

/*
 * Function to get all other hardware devices exist on the system.
 * The dev_info structure in this routine is specific to Campus1 machine
 * only.
 */
void
get_c_devices(mem)
int	mem;
{
	struct dev_info *top;

    	func_name = "get_c_devices";
#ifndef SVR4
  	TRACE_IN
#endif SVR4
	kvm_read(mem,nl[NL_TOPDEVINFO].n_value,(char *)&top,
					sizeof(struct dev_info *));
	if (top) walk_devs(top,0,mem);
#ifndef SVR4
	TRACE_OUT
#endif SVR4
}


/*
 * Function recursively gets all devices from the dev_info tree and
 * puts into the mbdinfo array.
 */
walk_devs(dp,parent,mem)
struct dev_info *dp, *parent;
int	mem;
{

	func_name = "walk_devs";
#ifndef SVR4
	TRACE_IN
#endif SVR4
	kvm_read(mem,(u_long)dp,(char *)&mbdinfo[numdev].info,
					sizeof(struct dev_info));
	mbdinfo[numdev].info.devi_parent = parent;
	dp = &mbdinfo[numdev].info;
	get_devname(mbdinfo[numdev].name,dp->devi_name,mem);
	dp->devi_name = mbdinfo[numdev].name;
	numdev++;
	if (dp->devi_slaves)			/* next sibbling */
		walk_devs(dp->devi_slaves,dp,mem);
	if (dp->devi_next)			/* next child */
		walk_devs(dp->devi_next,parent,mem);
#ifndef SVR4
	TRACE_OUT
#endif SVR4
}


/*
 * Function gets the device name and stored in string buf.
 */
get_devname(buf,dname,mem)
char *buf, *dname;
int  mem;
{
	int k;

  	func_name = "get_devname";
#ifndef SVR4
	TRACE_IN
#endif SVR4
	if (dname)
	{
		for (k = 0; k < NAMELEN; k++)
		{
			kvm_read(mem,(u_long)dname++,buf,1);
			if (!*buf) break;
			buf++;
		}
	}
	*buf = '\0';
#ifndef SVR4
	TRACE_OUT
#endif SVR4
}

#endif  sun4

#ifdef sun386
weitek_probe()
{
        asm("movl    0xffc0c400, %eax");
}
 
i387_probe()
{
        asm("finit");
}
 
#define VGA_BASE                0xA00000    /* board base addr */
#define VGA_REG_OFFSET          0x1c0000
 
int sunvga_probe()
{
    u_short *vga_capture_reg_base;
    register bit_bucket;

    func_name = "sunvga_probe";
    TRACE_IN
    vga_capture_reg_base = (u_short *) ((u_char *)VGA_BASE +
VGA_REG_OFFSET);

    *vga_capture_reg_base = 0x08;  /* benign allows cpu access to mem */
    bit_bucket = *vga_capture_reg_base;
    TRACE_OUT
}

bus()
{
        longjmp(error_buf, 1);
}
segv()
{
        longjmp(error_buf, 2);
}

#endif

