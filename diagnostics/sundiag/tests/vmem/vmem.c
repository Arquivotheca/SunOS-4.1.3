static char     version[] = "Version 4.1";
static char     sccsid[] = "@(#)vmem.c 1.1 7/30/92 Copyright 1985 Sun Micro";
/*
 * vmem.c
 *
 * usage: vmem [d] [m=margin] [sd] [cg2] [cg4] [cg5]
 *	       [ibis] [taac] [ipcs=number_of_ipcs] [M=pattern]
 *             [R=reserve] [page]
 *	       [cerr=number_of_contiguous_errors]
 *	       [nerr=number_of_noncontiguous_errors]
 *
 * This program will allocate a chunk of virtual memory and then write
 * a pattern to each word.  The pattern is a modified address of each
 * location:  the pattern argument is logically or'ed with the MSB of
 * the virtual address.  Vmem can be run in 1 of 2 modes.  The default
 * is to write to all of memory and then to go back and read/compare
 * the pattern written.  Any errors found will be recorded in the
 * verr[] array and then displayed.  The 2nd mode is the "page" mode.
 * In the page mode, after writing to each page, the program will
 * read and compare the observed pattern with the expected pattern.
 * This program reserves ~2.5 megabytes (by default) for other processes
 * to use, but this can be changed by the "m= margin" option (in megabytes).
 *
 * compile:  cc vmem.c -g -I../../include ../../lib/libtest.a -lkvm -o vmem
 */

#include <sys/param.h>

#include <kvm.h>
#include <nlist.h>
#include <vm/anon.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "sdrtns.h"
#include "../../../lib/include/libonline.h" /* online library include file */
#include "vmem.h"
#include "vmem_msg.h"

extern int process_vmem_args();
extern int routine_usage();

struct verr	verr[MAXERRS];
extern int	errno;
int		numerrs = 0;		/* total # of noncontiguous errors */

/* variables initialized from command line arguments.  */
int             margin = 0;     /* memory reserved for OS to use */
int		reserve = 0;	/* mem reserved in addition to the default */
int             cg2 = FALSE;
int             cg4 = FALSE;
int             cg5 = FALSE;
int		gttest = FALSE;
int		cg12 = FALSE;
int             gp2 = FALSE;
int             ibis = FALSE;
int             taac = FALSE;
int		zebra = 0;
int             ipcs = 0;
int   		pattern = 0x1000000;
int		pagemode = FALSE;	/* flag to run test 1 page at a time */
int		sim_nerrs = 0;		/* # of noncontig errs to sim	   */
int		sim_cerrs = 0;		/* # of contig errs to simulate    */


main(argc, argv)
   int	argc;
   char	*argv[];
{
   int	exitcode = 0;
		    
   versionid = "1.1";
   test_init(argc, argv, process_vmem_args, routine_usage, test_usage_msg);
   device_name = "kmem";

   select_margin();
   exitcode = runtests();

   if (numerrs) 
      send_message(0, ERROR, test_fail_msg, numerrs);

   if (exitcode) 
	exit(exitcode);
   test_end();			/* Sundiag normal exit */
}


/*
 * runtests() determines how much virtual memory is available, minus any
 *   margin and reserve, allocates the amount to be tested, and then calls
 *   the routine to test the memory.
 */
runtests()
{
   char		*valloc();
   char 	*start;		/* base addr of where testing begins	*/
   int		memsize = 0;	/* size of memory reported by kernel.	*/
   int		testsize = 0;	/* size of memory - margin.		*/
   int		retcode = 0;
   struct	rlimit limit ;

   func_name = "runtests";
   TRACE_IN
   memsize = get_vmem_size();
   send_message(0, VERBOSE, found_msg, memsize, memsize, memsize/PAGESIZE);


   /* overwrite resource limit for BIG swap disk, Hai, 1/5/91 */
   getrlimit(RLIMIT_DATA,&limit) ;
   if ( limit.rlim_max < memsize ) {
      limit.rlim_max = RLIM_INFINITY ;
      limit.rlim_cur = RLIM_INFINITY ;
      if ( setrlimit(RLIMIT_DATA,&limit) == -1 )
	 send_message(1,ERROR, "fail set resource limit");
   }

   getrlimit(RLIMIT_DATA,&limit) ;	/* re-confirm limit */
   if ( memsize > limit.rlim_max ) {
       memsize = limit.rlim_max ;
       send_message(0, INFO,"Resize memsize to max allowed size(0x%x)",limit.rlim_max);
   }
   if ((testsize = memsize - margin - reserve) < 2*PAGESIZE) 
      send_message(1, ERROR, swap_err_msg);


   if ((start = valloc(testsize)) == NULL)  /* allocate the memory */
      send_message(1, ERROR, valloc_err_msg, testsize);

   send_message(0, VERBOSE, testing_msg,testsize, testsize, testsize/PAGESIZE);

   retcode = (pagemode) ? pagetest(start, testsize) : memtest(start, testsize);
   free(start);				/* release memory */
   TRACE_OUT
   return(retcode);
}

/*
 * memtest() will write to the entire range of virtual memory and then go
 *   back and read/compare the data with the expected pattern.  If the 
 *   "run_on_error" flag is not set, then the miscompared data will be
 *   displayed immediately.  If set, then the error will be recorded in
 *   the verr[] array and displayed after the entire range is read.
 *
 * arguments:  staddr.  The starting address of range to test.
 *             size.    The size of the the range to test.
 *             pass.    The pattern used to logically or'ed with the virtual
 *                      address.  The result is the data written to memory.
 *
 * returns 0 if no errors and 1 (MISCOMPARE) if found error.
 */
memtest(staddr, size)
   char		*staddr;	/* start address of memory to test.	*/
   int		size;		/* how much memory to test.		*/
{
   int          errors = 0;	/* # of miscompare errors.		*/
   int		vindex = 0;	/* index into verr[] error-info array.	*/
   int		recmp_errs = 0; /* errors found in recompare.		*/
   int		randnum = 0;	/* random number, for simulating errors */
   int		i;

   func_name = "memtest";
   TRACE_IN
   send_message(0, VERBOSE, memtest_msg, staddr, staddr+size);

   if (!sim_nerrs && sim_cerrs)
      sim_nerrs = 1;
   else if (sim_nerrs && !sim_cerrs) 
      sim_cerrs = 1;

   fill(staddr, size, pattern);

   for (i = 1; i <= sim_nerrs; i++) {
      randnum = ((rand() + 0x10) & 0xffff0) + (int)staddr;
      send_message(0, VERBOSE, sim_err_msg, randnum);
      fill((char *)randnum, sim_cerrs*sizeof(long), 0xff00ff00);
   }

   send_message(0, VERBOSE, sim_read_msg, staddr, staddr+size);

   if ((errors = check(staddr, size, pattern, 0))) {
      for (vindex = 0; vindex < errors; vindex++) 
	 printerrs(pattern, vindex, 0, 0);

      send_message(0, VERBOSE, recomp_test_msg, staddr, staddr+size);

      if ((recmp_errs = check(staddr, size, pattern, 0)) != errors) 
         send_message(0, INFO, recomp_err_msg, recmp_errs, errors);

      for (vindex = 0; vindex < recmp_errs; vindex++) 
	 printerrs(pattern, vindex, 0, 1);

      if ((numerrs = errors) >= MAXERRS)   
         send_message(0, ERROR, max_err_msg, numerrs);
      return(MISCOMPARE);  /* nonzero means found error */
   }
   TRACE_OUT
   return(0);
}

/*
 * pagetest() sets up the amount of memory to test and then calls the
 *   routines fill() and check() to do the actual testing and checking.
 *   Testvmem() checks the kernel for the amount of available memory
 *   and then subtracts the margin to reserve for the OS and other
 *   tests.  The remaining memory will then be allocated to be tested.
 *   The memory is written/read 1 page at a time.  Each page is mmap'ed
 *   to the temporary file /tmp/vmem.page and paged out to disk after
 *   the data is written.  The page is paged-in again before it is read.
 *
 * return value:   0 if no errors found.
 *                 1 (MISCOMPARE) if found error(s).
 */
pagetest(startaddr, size)
   char		*startaddr;	/* start address of memory to test.     */
   int		size;
{
   char			*calloc();
   int			pgerrs = 0;	/* # of noncontig errors in a page   */
   int			sim_conterrs = sim_cerrs;
   int			sim_nconterrs = sim_nerrs;
   int			verr = 0;       /* index to the verr[] array.	     */
   int			recmp_errs = 0; /* errors found in recompare.	     */
   int			pagefd;		/* file descriptor for PAGEFILE.     */
   int			i, j;
   char			*pagedata;
  
   func_name = "pagetest"; 
   TRACE_IN
   if ((pagefd = open(PAGEFILE,  O_RDWR|O_CREAT, 0666)) == -1) 
        send_message(1, ERROR, open_err_msg, PAGEFILE, errmsg(errno));

   pagedata = calloc(PAGESIZE, sizeof(char));
   write(pagefd, pagedata, PAGESIZE); /* put data in file before mmap() */
   free(pagedata);	/* release buffer */
   send_message(0, VERBOSE,test_msg, startaddr, startaddr+size, size/PAGESIZE);

   if (!sim_nconterrs && sim_conterrs)
      sim_nconterrs = 1;
   else if (sim_nconterrs && !sim_conterrs) {
      sim_conterrs = 1;
      sim_cerrs = 1;
   }
   if (sim_nerrs > 1)
      run_on_error = TRUE;
      
	 /* test the number of pages allocated, 1 page at a time */
   for (i = 0; i < size/PAGESIZE; i++, startaddr += PAGESIZE) {
      if ((mmap(startaddr, PAGESIZE, PROT_READ|PROT_WRITE,
                 MAP_FIXED|MAP_SHARED, pagefd, 0) == (caddr_t)-1) ) {
         send_message(1, ERROR, mmap_err_msg,startaddr,PAGEFILE,errmsg(errno));
      }
      fill((long *)startaddr, PAGESIZE, pattern);
      if (sim_nconterrs && sim_conterrs) {
	 send_message(0, VERBOSE, simulate_msg, sim_cerrs, startaddr);
	 fill((long *)startaddr, sim_cerrs*sizeof(long), 0xff00ff00);
	 sim_nconterrs--;
      }
      /* Flush the page out to disk before attempting to read.  The
       * MS_INVALIDATE flag will force paging-in upon reading. 
       */
      if (msync(startaddr, PAGESIZE, MS_INVALIDATE) == -1) 
         send_message(1, ERROR, msync_err_msg, errmsg(errno));

      if ((pgerrs = check(startaddr, PAGESIZE, pattern, verr))) {
	 for (j = verr; j < verr+pgerrs; j++) 
	    printerrs(pattern, j, i, 0);

         if ((recmp_errs=check(startaddr, PAGESIZE, pattern, verr)) != pgerrs) 
            send_message(0, INFO, recompare_msg, i, recmp_errs, pgerrs);
	 for (j = verr; j < verr+recmp_errs; j++) 
	    printerrs(pattern, j, i, 1);

         numerrs += pgerrs; /* incr numerrs after the recompare! */
	 send_message(0, LOGFILE, "\n");
         if (numerrs >= MAXERRS) {  /* stop if get 10 noncontiguous errors */
            send_message(0, ERROR, max_err_msg, pattern, numerrs);
	    return(MISCOMPARE);
         }
	 verr += pgerrs;
	 if (!run_on_error)
	    break;		/* if re flag not set, stop after 1st error */
      }
      if (debug && (i % 100) == 0) 
         send_message(0, DEBUG, page_done_msg, i, i, startaddr);
      if (munmap(startaddr, PAGESIZE) == -1)  /* unmap the page */
         send_message(1, ERROR, munmap_err_msg, startaddr, errmsg(errno));
   }
   if (numerrs) {
	if (!run_on_error) 
	   send_message(0, ERROR, test_err_msg);
	return(MISCOMPARE);
   }
   if (close(pagefd) == -1) 
         send_message(1, ERROR, close_err_msg, PAGEFILE, errmsg(errno));

   unlink(PAGEFILE);
   TRACE_OUT
   return(0);
}  /* end of pagetest() */

/*
 * fill()
 *
 *   This routine writes into memory (amount specified by size) starting
 *   at base.  The pattern is ipat 'or'ed with the virtual address
 *   of each location.  ipat is just the current pass count logically
 *   shifted left to the highest byte (i.e.,  << 24).  For example, address
 *   0x12340 will get the pattern 0x02012340 in pass #2.  Long words
 *   (4 bytes) are written.
 *
 * no return value.
 */
fill(base, size, ipat)
   long *base;
   int		 size;
   unsigned long ipat;		/* initial pattern, to be modified */
{
   int	nwords;

   func_name = "fill";
   TRACE_IN
   nwords = size / sizeof(long);
   while (nwords--)  {
	/* This is ambiguous C code and fail vmem in 5.0DR 
	 * *base++ = ipat | (unsigned long)base;
	 */
	*base = ipat | (unsigned long)base ;
	base++ ;
   }
   TRACE_OUT
}

/*
 * check()
 *
 *   reads from memory starting at base.  The expected pattern is
 *   ipat 'or'ed with the virtual address of each location.  ipat
 *   is just the current pass count logically shifted left to the
 *   highest byte.  Long words are read.
 *   If an error occurs during comparison, then we record the
 *   address of the initial error and continue to compare until
 *   a successful comparison is found, at which time we stop
 *   reading unless the run_on_error flag is set.  If it's not
 *   set, then we continue to check and fill in the verr[] array
 *   upon finding compare errors.  Each noncontiguous error is
 *   stored in an element of the verr[] array.
 *   The number of contiguous errors are recorded and also 
 *   the observed patterns of the 1st 30 errors are recorded if
 *   there are more than 30 errors.
 *   When we read, we want to read the data from memory only once
 *   and store the data in local memory (preferably a register). This
 *   is because in systems with cache, sometimes when there's a timing
 *   problem in the cache, the first time we read the data it may be
 *   incorrect, but the second time we read it, the data would be correct.
 *
 * return value:  0 if no error found.
 *                number_of _errors if found some errors.
 */
check(base, size, ipat, vindex)
   register long *base;
   int		 size;
   unsigned long ipat;
   int		 vindex;	/* index into verr[] array  */
{
   register unsigned long obs_pat = 0;	/* pattern read from memory */
   int	nwords;			/* size (in words) to check */
   int	errflag = 0, errs = 0, pagesize = 0;

   func_name = "check";
   TRACE_IN
   pagesize = getpagesize() / sizeof (long);
   nwords = size /sizeof(long);
   while (nwords--) {
       if ((numerrs+errs) >= MAXERRS)
	    break;  /* stop test if found 10 noncontiguous errors */

#ifdef	sun386
       obs_pat = (*base)-4;  /* read from memory and store in local variable */
#else	sun386
       obs_pat = *base;	     /* read from memory and store in local variable */
#endif	sun386

       if (obs_pat != (ipat | (unsigned long)base)) {
	    if (!errflag) { /* record initial err addr */
	       verr[vindex].base = (unsigned long *)base;
	       verr[vindex].conterrs = 0;  /* reset contigous error counter */
	    }
	    errflag = 1;	/* indicate an error occured */
	    if (verr[vindex].conterrs <= SHOW_ERRS) /* record up to 30 errs */
		verr[vindex].observe[verr[vindex].conterrs] = obs_pat;
	    verr[vindex].conterrs++;

#ifndef	NEW	/* pre- 4.1 only hack -JCH- 4/3/90 */
		/* ALERT!!!  This hack is used to cover up a kernel bug.  This 
	 	 * code should be removed as soon as the kernel is fixed. */
	    if (verr[vindex].conterrs ==  pagesize)
		errflag = 0;
		/* */
#endif

       } else if (errflag) {
	     /* comes here on 1st correct compare after a contigous number of
	      * incorect compares.  */
	    errs++;
	    vindex++;
	    errflag = 0;  /* reset to indicate end of contiguous errors */
	    if (!run_on_error)
	       break;
       }
       base++;		/* next word */
   }
	/* if we have only 1 set of contiguous errors and then correct
	 * compares, then errflag==0 and errs==1;  if have 1 large
	 * set of contiguous errors to the end of the range we're testing,
	 * then errflag==1 and errs==0 */
   if (errflag)
	errs++;
   TRACE_OUT
   return(errs);
}


/*
 * printerrs()
 *
 *  This routine accepts a pointer to the error information structure filled
 *  in during the comparison phase of test (check()).  The structure contains
 *  the address of the initial error, the number of contiguous errors starting
 *  at that address, and up to 30 observed patterns.  We display the
 *  virtual address, observed pattern, and expected pattern.  The expected
 *  pattern is calculated:  ipat 'or'ed with the virtual address of each
 *  location.  ipat is just the current pass count logically shifted left
 *  to the highest byte (<< 24).
 *  
 * no return value.
 */
printerrs(ipat, vindex, page, recompare)
    unsigned long	ipat;   /* the initial pattern used		  */
    int		vindex; 	/* index into verr[] that we're to print  */
    int		page;		/* which page did the error occur at    */
    int		recompare;      /* flag for displaying recompare header */
{
    unsigned long	i;
    
    func_name = "printerrs";
    TRACE_IN
    errheader(page, vindex, recompare);	/* print header */

    for (i = 0; i < SHOW_ERRS; i++) { 	/* display up to 30 data errors */
	send_message(0, LOGFILE, show_err_msg, verr[vindex].base,
                     verr[vindex].observe[i], ipat | (u_long)verr[vindex].base);
	verr[vindex].base++;   		/* next address */
    	if (i == (verr[vindex].conterrs - 1))
    		break;		/* stop printing if less than 30 errors */
    }
    if (verr[vindex].conterrs > SHOW_ERRS) 
        send_message(0, LOGFILE, contig_err_msg, SHOW_ERRS);
    TRACE_OUT
}

/*
 * errheader() prints a header saying that miscompare errors have
 *    been found.  errheader() should be called prior to printing
 *    each element of the verr[] array.
 */
errheader(page, vindex, recompare)
    int		page;		/* which page did the error occur at	*/
    int		vindex;		/* index into the verr[] array		*/
    int		recompare;	/* flag for displaying recompare header */
{
    char	pagestr[20];

    func_name = "errheader";
    TRACE_IN
    if (pagemode)
       sprintf(pagestr, "page %d", page);
    else
       strcpy(pagestr, "memory");
    if (recompare)
       send_message(0, LOGFILE, re_mis_msg, pagestr, 4*verr[vindex].conterrs);
    else
       send_message(0, LOGFILE, miscompare_msg, vindex+1, pagestr, 
                    4*verr[vindex].conterrs);
    send_message(0, LOGFILE, errheader_msg);
    TRACE_OUT
}

/*
 * size = get_vmem_size();
 *
 * int size: size of available virtual memory in bytes.
 *
 * get_vmem_size() reads the kernel to find the amount of virtual memory
 * available.
 *
 */
get_vmem_size()
{
  kvm_t	*mem;
  struct anoninfo ai;
  int	pageshift = 0;
  char	*vmunix, *getenv();

  func_name = "get_vmem_size";
  TRACE_IN
  check_superuser();
  vmunix = getenv("KERNELNAME");
  if ((mem = kvm_open(vmunix, NULL, NULL, O_RDONLY, NULL)) == NULL) 
        send_message(1, CONSOLE, kvm_err_msg, "open");
  if (kvm_nlist(mem, nl) == -1) 
        send_message(1, CONSOLE, kvm_err_msg, "nlist");
  kvm_read(mem, nl[NL_SANON].n_value, (char *)&ai, sizeof(struct anoninfo));
  for (pageshift = 1; pageshift < 32; pageshift++) 
        if ((getpagesize() >> pageshift) == 1) 
            break; 
  kvm_close(mem);
  TRACE_OUT
  return((ai.ani_max - ai.ani_resv) << pageshift); /* virtural memory size */
}

/*
 * clean_up() is needed to satisfy libtest.a.
 */
clean_up()
{
   unlink(PAGEFILE);  /* remove temp file used for paging memory */
   if (numerrs)    /* in case being killed during "run_on_error"(bug 1027673) */
      send_message(1, ERROR, test_fail_msg, numerrs);
}

process_vmem_args(argv, arrcount)
   char		*argv[];
   int		arrcount;
{
   if (strncmp(argv[arrcount], "cg2", 3) == 0) {
      cg2 = TRUE;
   } else if (strncmp(argv[arrcount], "cg4", 3) == 0) {
      cg4 = TRUE;
   } else if (strncmp(argv[arrcount], "cg5", 3) == 0) {
      cg5 = TRUE;
   } else if (strncmp(argv[arrcount], "cg12", 4) == 0) {
      cg12 = TRUE;
   } else if (strncmp(argv[arrcount], "gt", 2) == 0) {
      gttest = TRUE;
   } else if (strncmp(argv[arrcount], "gp2", 3) == 0) {
      gp2 = TRUE;
   } else if (strncmp(argv[arrcount], "ibis", 4) == 0) {
      ibis = TRUE;
   } else if (strncmp(argv[arrcount], "taac", 4) == 0) {
      taac = TRUE;
   } else if (strncmp(argv[arrcount], "zebra=", 6) == 0) {
      zebra = atoi(&argv[arrcount][6]);
   } else if (strncmp(argv[arrcount], "ipcs=", 5) == 0) {
      ipcs = atoi(&argv[arrcount][5]);
   } else if (strncmp(argv[arrcount], "U", 1) == 0) {
      test_description();
      exit(0);
   } else if (strncmp(argv[arrcount], "M=", 2) == 0) {
      pattern = atoi(&argv[arrcount][2]) << PAT_SHIFT;
   } else if (strncmp(argv[arrcount], "m=", 2) == 0) {
      margin = atoi(&argv[arrcount][2]) * 0x100000;
   } else if (strncmp(argv[arrcount], "R=", 2) == 0) {
      reserve = atoi(&argv[arrcount][2]) * 0x100000;
   } else if (strncmp(argv[arrcount], "page", 4) == 0) {
      pagemode = TRUE;
   } else if (strncmp(argv[arrcount], "cerrs=", 6) == 0) {
      sim_cerrs = atoi(&argv[arrcount][6]);
   } else if (strncmp(argv[arrcount], "nerrs=", 6) == 0) {
      sim_nerrs = atoi(&argv[arrcount][6]);
   } else {
      return(FALSE);
   }
   return(TRUE);
}

select_margin()
{
   func_name = "select_margin";
   TRACE_IN
   if (!margin) { /* Margin of memory to save for OS if user didn't select */
      if (gp2)
         margin += GP2_MARGIN;
      else if (cg5)
         margin += CG5_MARGIN;
      else if (cg2)
         margin += COLOR_MARGIN;
      else if (cg12)
	margin += CG12_MARGIN;
   
      if (ibis)
         margin += IBIS_MARGIN;
      else if (cg4)
         margin += COLOR_MARGIN;

      if (ipcs)
         margin += IPC_MARGIN * ipcs;
      if (taac)
         margin += TAAC_MARGIN;
      if (zebra)
         margin += ZEBRA_MARGIN * zebra;
      if (gttest)
	 margin += COLOR_MARGIN; /* gttest use 2M bytes, same as COLOR */
      margin += FIXED_MARGIN;
   }
   TRACE_OUT
}

routine_usage()
{
    send_message (0, CONSOLE, routine1_msg, test_name);
    send_message (0, CONSOLE, routine2_msg);
    send_message (0, CONSOLE, routine3_msg);
}

test_description()
{
    send_message (0, CONSOLE, describe1_msg);
    send_message (0, CONSOLE, describe2_msg);
    send_message (0, CONSOLE, describe3_msg);
    send_message (0, CONSOLE, describe4_msg);
    send_message (0, CONSOLE, describe5_msg);
    send_message (0, CONSOLE, describe6_msg);
    send_message (0, CONSOLE, describe7_msg);
}
