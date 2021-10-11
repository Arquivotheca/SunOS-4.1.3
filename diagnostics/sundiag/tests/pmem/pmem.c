static char     sccsid[] = "@(#)pmem.c 1.1 7/30/92 Copyright 1985 Sun Micro";
/*
 * pmem:  physical memory test.  pmem opens up /dev/mem, mmap's each page
 *   to pmem's address space, and then attempts to read each page.  This
 *   test doesn't attempt to write to memory.
 *
 * compile:  cc -g pmem.c -o pmem -I../include ../lib/libtest.a -lkvm
 */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <nlist.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#ifdef   SVR4
#include <kvm.h>
#include <sys/fcntl.h>
#else
#include <kvm.h>
#include <mon/idprom.h>
#endif	 SVR4
#include "openprom.h"	/* sun4c */
#define  NO_CLEAN_UP
#include "sdrtns.h"		        /* sundiag standard include file */
#include "../../../lib/include/libonline.h"
#include "pmem.h"
#include "pmem_msg.h"

#define	CTOB(x)			(((u_long)(x)) * pagesize)

u_char		*pageaddr;
int		pagesize, fd, size = 0;

main(argc, argv)
    int             argc;
    char            *argv[];
{
    extern int      process_pmem_args(), routine_usage();
    extern u_char   *valloc();
    kvm_t           *kd;
    int             campus = FALSE, nlvar = 0, pmemsize = 0, physmem;
    char	    *vmunix, *getenv();

/* sun4c */
    struct sunromvec	*sunrom, romvec;
    struct memlist	**pm, *pmlist, pmem;
/* sun4c */
    
    versionid = "1.1";
    func_name = "main";
			/* begin Sundiag test */
    test_init(argc, argv, process_pmem_args, routine_usage, test_usage_msg);
    device_name = "mem";

    trace_before("getpagesize");
    pagesize = getpagesize();

#ifndef	SVR4
    if (sun_arch() != ARCH4)
        campus = TRUE;
#else	SVR4
    campus = FALSE;
#endif SVR4

    vmunix = getenv("KERNELNAME");
    trace_before("kvm_open");
    if ((kd = kvm_open(vmunix, NULL, NULL, O_RDONLY, NULL)) == NULL) 
        send_message(1, ERROR, kvmopen_err_msg, errmsg(errno));

    trace_before("kvm_nlinst");
    if (kvm_nlist(kd, nl) == -1) 
        send_message(1, ERROR, kvmnlist_err_msg, errmsg(errno));

    nlvar = campus ? NL_ROMP : NL_PHYSMEM;
#ifndef sun386
    if (nl[nlvar].n_type == 0) 
#else
    if (nl[nlvar].n_value == 0) 
#endif
	send_message(-BAD_NAMELIST, FATAL, namelist_err_msg);

    if (campus) {
	trace_before("kvm_read - sunrom");
        if (kvm_read(kd,(u_long)nl[nlvar].n_value,(char *)&sunrom,
            sizeof(struct sunromvec *)) != sizeof(struct sunromvec *)) {
            send_message(-BAD_PHYSMEM_VALUE, FATAL, read_err_msg, "sunrom",
		errmsg(errno));
        }
	trace_before("kvm_read - romvec");
        if (kvm_read(kd,(u_long)sunrom,(char *)&romvec,
                     sizeof(struct sunromvec)) != sizeof(struct sunromvec)) {
            send_message(-BAD_PHYSMEM_VALUE, FATAL, read_err_msg, "romvec",
		errmsg(errno));
        }
    } else {
	trace_before("kvm_read - physmem");
        if (kvm_read(kd, nl[nlvar].n_value, (char *) &physmem,
                     sizeof(physmem)) != sizeof(physmem)) {
	    send_message(-BAD_PHYSMEM_VALUE, FATAL, read_err_msg, "physmem",
		errmsg(errno));
        } else if (!physmem) 
	    send_message(-NO_PHYSMEM, FATAL, no_memory_msg);

        send_message(0, VERBOSE, test_msg, CTOB(physmem), 
		     ceil((double)(CTOB(physmem)/1048576.0)));
    }

    if (size && size != physmem) {
	send_message(0, VERBOSE, size_err_msg, CTOB(size)/1048576.0, 
                     CTOB(physmem) / 1048576.0);
	physmem = size;
    }

    trace_before("open /dev/mem");
    if ((fd = open("/dev/mem", O_RDONLY)) < 0)
        send_message(-NO_MEM_FILE, FATAL, open_err_msg, errmsg(errno));

    trace_before("valloc");
    if ((pageaddr = valloc(pagesize)) == (u_char *)0) 
	send_message(-BAD_VALLOC, FATAL, valloc_err_msg, errmsg(errno));

    if (campus) { 	/* get all the chunks of physical memory */

	if(romvec.v_romvec_version > 0){  /* Calvin uses Open Boot Prom V2.0 */
	   trace_before("kvm_read - physmemory");
           if (kvm_read(kd,nl[NL_PHYSMEMORY].n_value,(char *)&pmlist,
                sizeof(struct memlist *)) != sizeof(struct memlist *)) {
                send_message(FATAL, ERROR, memlist_err_msg, errmsg(errno));
           }			  /* get value of _physmemory for kernal */
	}
 	else {					 
           pm = romvec.v_physmemory;
           if (kvm_read(kd,(u_long)pm,(char *)&pmlist,
                sizeof(struct memlist *)) != sizeof(struct memlist *)) {
                send_message(FATAL, ERROR, memlist_err_msg, errmsg(errno));
           }
	}
        if (!pmlist) 
            send_message(-NO_PHYSMEM, FATAL, found_err_msg);

	trace_before("kvm_read loop");
        while (pmlist) {
            if (kvm_read(kd,(u_long)pmlist,(char *)&pmem,
                    sizeof(struct memlist)) != sizeof(struct memlist)) {
                send_message(FATAL, ERROR, pmlist_err_msg, errmsg(errno));
            }
            pmem_test(pmem.address,pmem.size);
            pmemsize += pmem.size;
            pmlist = pmem.next;
        }
        send_message(0, VERBOSE, memory_size_msg, pmemsize,pmemsize/1048576.0);
    } else {
        pmem_test(0, CTOB(physmem));
    }
    
    kvm_close(kd);
    sleep(10);			/* slow down the pmem test */
    test_end();			/* Sundiag normal exit */
}

pmem_test(addr, size)
    u_int	addr, size;
{
    register u_char	ch, *p;
    register u_int	s, offset;
    register int	k;

    func_name = "pmem_test"; TRACE_IN
    for (offset = addr, s = 0; s < size; offset += pagesize, s += pagesize) {
        send_message(0, DEBUG, read_mem_msg, offset);
	if (mmap((char *)pageaddr, pagesize, PROT_READ, MAP_FIXED | MAP_SHARED,
	    fd, (off_t)offset) == (caddr_t)-1) {
	        if (errno == ENXIO) 
                    break;	/* for Sun3x I/O cache only */
	        send_message(-BAD_MMAP, ERROR, mmap_err_msg, errmsg(errno),
                             offset/pagesize, offset/pagesize, pageaddr);
	}
        for (p = pageaddr, k = 0; k < pagesize; k++, p++) 
	    ch = *p;		/* doing read only */
    }
    send_message(0, VERBOSE, read_end_msg, size, size/pagesize, addr);
    TRACE_OUT
}

routine_usage()
{
    send_message(0, CONSOLE, routine_msg, test_name);
}

process_pmem_args(argv, arrcount)
	char 	*argv[];
	int	arrcount;
{
    if (!strncmp(argv[arrcount], "s=", 2)) {
        size = atoi(&argv[arrcount][2]) * 128;
	return(TRUE);
    } 

    return (FALSE);
}

