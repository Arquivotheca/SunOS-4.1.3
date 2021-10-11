#ifndef lint
static	char sccsid[] = "@(#)vfctest.c 1.1 92/07/30 Copyright Sun Micro";
#endif

#include <sys/types.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include "vfctest_defs.h"
#include "vfc_defs.h"
#include "vfc_ioctls.h"
#include "sdrtns.h"	/* sdrtns.h should always be included */
#include "../../../lib/include/libonline.h"


#define	DEVICE_NAME	"/dev/vfc0"
#define	TOTAL_PASS	1     /* number of test loops */
#define ERROR_LIMIT	100   /* max num of errors allowed if run_on_error */

int probe_vfcdev(), process_vfc_args(), vfctest_usage();

/* VFC-specific globals */
u_int *vfc_port1, *vfc_port2;
int vfcfd;			/* file-desc for use during test */
int vidchk;			/* flag to see if user wants a video chk */
int memchk;			/* flag to see if user wants a mem chk */
int enggflag;
int errors = 0;
struct memstrct {
  int pixloc;
  u_int wpatt;
  u_int rpatt;
} ;
struct memstrct *memerr;
static int errnum = 0;
static char mesgstr[120];


main(argc, argv)
     int	argc;
     char	*argv[];
{
  int fd;

  versionid = "1.1";

  device_name = DEVICE_NAME;

  vidchk = TRUE;
  memchk = TRUE;
  enggflag = FALSE;
  /* Sundiag start up call */
  test_init(argc, argv, process_vfc_args, vfctest_usage, (char *)NULL);
  if (vidchk == FALSE)
    send_message(SKIP_ERROR, VERBOSE, "Don't Report on Video Signals");
  if (memchk == FALSE)
    send_message(SKIP_ERROR, VERBOSE, "Don't test the Field Memory");
  if (enggflag == TRUE)
    send_message(SKIP_ERROR, VERBOSE, "Engg. Testing/Debug");

  if (!exec_by_sundiag)		/* command-line execution in progress */
    probe_vfcdev();
  
  run_vfctests(single_pass ? 1 : TOTAL_PASS);	/* start vfc tests */

  if (errors > 0)
    exit(1);
  else
    {
      clean_up();
      test_end();
    }
}

/**
 * Process_vfc_args() processes vfctest specific args.
 * Check if the user wants to check the video signals on each port.
 */
process_vfc_args(argv, argindex)
char *argv[];
int  argindex;
{
  int match = FALSE;

   if (strncmp(argv[argindex], "novideo", 7) == 0) {
     vidchk = FALSE;
     match = TRUE;
   }
   if (strncmp(argv[argindex], "nomem", 5) == 0) {
     memchk = FALSE;
     match = TRUE;
   }
  if (strncmp(argv[argindex], "vfceng", 6) == 0) {
    enggflag = TRUE;
    match = TRUE;
  }
  if (strncmp(argv[argindex], "vfc0", 4) == 0) {
	strcpy(device_name, "/dev/vfc0");
	match = TRUE;
  }
	
  if (strncmp(argv[argindex], "vfc1", 4) == 0) {
	strcpy(device_name, "/dev/vfc1");
	match = TRUE;
	}
	
  return (match);
}

vfctest_usage()
{
  send_message(SKIP_ERROR, CONSOLE, "vfctest [novideo] [nomem]: \n\
 	novideo = Do not test video section\n\
	nomem = Do not test memory section\n");
}



/*  Since vfc has a loadable-only driver, the open call should
 *  failed if the board is not installed in the system.
 */

probe_vfcdev()
{
  int rcode, fd;

  rcode = 0;
  fd = -1;
  /* First check if the driver has been installed */
  if (access(device_name, 0) != 0)
    rcode = DEV_NOT_ACCESSIBLE;
  else
    if ((fd = open(device_name, O_RDWR)) == -1) 
      rcode = DEV_OPEN_ERR;

  if (rcode)
    errorlog(rcode, "Probe Failed \n");

  if (fd > 0)			/* valid fd */
    close(fd);

  return (0);
  
}


run_vfctests(total_pass)
int total_pass;
{
  int pass = 0;
  caddr_t base;
  int subcmd;

  if ((vfcfd = open(device_name, O_RDWR)) == -1)
    errorlog(DEV_OPEN_ERR, "Open Failed \n");

  base = mmap(0, 0x8000, PROT_READ | PROT_WRITE, MAP_SHARED, vfcfd, 0x0);

  if (base < 0)
    errorlog(DEV_MMAP_ERR, "Mmap Failure \n");

  /* Obtain pointers to the 2 FRAM banks */
  vfc_port1 = (u_int *)((caddr_t) base + 0x5000);
  vfc_port2 = (u_int *)((caddr_t) base + 0x7000);

  memerr = (struct memstrct *)malloc(sizeof(struct memstrct) * 3000);
  while (++pass <= total_pass) {
    if (memchk == TRUE) {
      subcmd = DIAGMODE;
      ioctl(vfcfd, VFCSCTRL, &subcmd); /* put the device in diag-mode */
      send_message(SKIP_ERROR, VERBOSE, "Memory Test");
      walking_test();	/* walking 1s and 0s test */
      unique_test();	/* unique test  */
      subcmd = NORMMODE;
      ioctl(vfcfd, VFCSCTRL, &subcmd); /* put the board back in Normal mode */
    }
    if (vidchk == TRUE)
      video_report();

    if (vidchk || memchk)
      send_message(SKIP_ERROR, VERBOSE, "vfctest: Pass = %d, Errors = %d",
		   pass, errors);
  }

  if (!exec_by_sundiag)
    printf("vfctest: Errors found = %d \n", errors);
  free((struct memstrct *)memerr);
  close(vfcfd);
}

	/* Report on the Video signals at each port */

video_report()
{
  int subcmd, rcode;
  int vidport;

  for (vidport = 1; vidport <= 3; ++vidport) {
    subcmd = STD_NTSC;
    rcode = ioctl(vfcfd, VFCSVID, &subcmd);
    if (rcode) {
      errorlog(SKIP_ERROR, "Failed during NTSC initialisation");
      return(0);			/* no use going further */
    }
    subcmd = vidport;
    if (rcode = ioctl(vfcfd, VFCPORTCHG, &subcmd)) {
      errorlog(SKIP_ERROR, "Failed to change port");
      continue;
    }
    subcmd = 0;
    rcode = ioctl(vfcfd, VFCGVID, &subcmd);
    
    if (rcode == -1) {
      errorlog(SKIP_ERROR, "Failed to get video status");
      continue;
    }
    
    if (debug) {
      sprintf(mesgstr,"Video Status from driver = %x \n", subcmd);
      send_message(0, DEBUG, mesgstr);
    }

    send_message(SKIP_ERROR, VERBOSE, "Report on Port %d", vidport);
    switch(subcmd) {
    case NO_LOCK:
      send_message(SKIP_ERROR, VERBOSE, "\tNo Video Signal Detected");
      break;

    case NTSC_COLOR:
      /* We detected NTSC Hlock, so let's try Capture Command */
      send_message(SKIP_ERROR, VERBOSE,
		   "\tNTSC Color signal detected");
      subcmd = CAPTRCMD;
      if (ioctl(vfcfd, VFCSCTRL, &subcmd))
	errorlog(SKIP_ERROR,
		 "NTSC Horz Freq. detected, but Capture Command failed");
      else
	send_message(SKIP_ERROR, VERBOSE, "Capture Command ok");
      break;

    case NTSC_NOCOLOR:
      /* We detected NTSC Hlock, so let's try Capture Command */
      send_message(SKIP_ERROR, VERBOSE,
		   "\tNTSC signal detected, but NO color signal confirmation");
      subcmd = CAPTRCMD;
      if (ioctl(vfcfd, VFCSCTRL, &subcmd))
	errorlog(SKIP_ERROR,
		 "NTSC Horz Freq. detected, but Capture Command failed");
      else
	send_message(SKIP_ERROR, VERBOSE, "Capture Command ok");
      break;
    case PAL_NOCOLOR:

      /* We assume that 50 Hz pulses is a PAL signal. Since we had
       * initialised for NTSC, we should re-initialise for PAL and
       * check for PAL color detected. After that we will try a
       * Capture.
       */
      subcmd = STD_PAL;
      if (ioctl(vfcfd, VFCSVID, &subcmd)) {
	errorlog(SKIP_ERROR, "Failed during PAL initialisation");
	break;
      }
      subcmd = 0;
      if (ioctl(vfcfd, VFCGVID, &subcmd)) {
	errorlog(SKIP_ERROR,
		 "Failed to get Video Status after PAL initialisation");
	break;
      }

      if (debug) {
	sprintf(mesgstr,"Video Status from driver(after PAL init.) = %x \n", subcmd);
	send_message(0, DEBUG, mesgstr);
      }

      if (subcmd == PAL_COLOR)
	send_message(SKIP_ERROR, VERBOSE, "\tPAL Color Signal Detected");
      else
	send_message(SKIP_ERROR, VERBOSE,
 	            "\tPAL signal detected, but NO color signal confirmation");

      subcmd = CAPTRCMD;
      if (ioctl(vfcfd, VFCSCTRL, &subcmd))
	errorlog(SKIP_ERROR,
		 "PAL Horz. Freq. detected, but Capture Command failed");
      else
	send_message(SKIP_ERROR, VERBOSE, "Capture Command ok");
      break;
    default:
      errorlog(-VFC_FATAL, "Unknown code returned from analyse_vidstatus\n");
    }
  }

  return (0);
}


	/* The following routine zaptopmem() takes care of
	 * ensuring the top 248 pixels are off-limits to testing.
	 */	   

zaptopmem (port, op)
     u_int *port;
     int   op;
{
  int i;
  u_int data;

  if (op == WRITE) {
    for (i = 1; i <= GARBAGE_PIXELS; ++i)
	*port = 0xbad00000;
  }
  else {
    for (i = 1; i <= GARBAGE_PIXELS; ++i)
	data = *port;
  }

  return (0);
}


walking_test()
{

  int loop, field;
  u_int *taddr;
  int retstat;

  for (field = 1; field <= 2; ++field) {
    if (field == 1)
      taddr = vfc_port1;
    else
      taddr = vfc_port2;

    for (loop = 1; loop <= 2; ++loop) {

      walking_write(taddr, loop); /* write walking pattern */

      /* Now perform the reading ... */
      if (enggflag)
	{
	  /* walking_read(taddr, loop, field, 3); */
	}
      else
	{
	  if (walking_read(taddr, loop, field,1) > 0)	/* first loop */
	    {
	      errnum = 0;
	      retstat = walking_read(taddr, loop, field, 2);
	      if (retstat == 1)
		{
		  errnum = 0;
		  while ((memerr[errnum].pixloc > 0) && (errnum < 3000))
		    {
		      sprintf(mesgstr,
			      "Walking Mem.: (Transient) Data Exp = 0x%x, Obs = 0x%x, Field %d, Pixel# %d",
			      memerr[errnum].wpatt, memerr[errnum].rpatt,
			      field, memerr[errnum].pixloc);
		      errorlog(SKIP_ERROR, mesgstr);
		      errnum++;
		    }
		}
	      if (retstat == 2)
		{
		  errnum = 0;
		  while ((memerr[errnum].pixloc > 0) && (errnum < 3000))
		    {
		      sprintf(mesgstr,
			      "Walking Mem.: Data Exp = 0x%x, Obs = 0x%x, Field %d, Pixel# %d",
			      memerr[errnum].wpatt, memerr[errnum].rpatt,
			      field, memerr[errnum].pixloc);
		      errorlog(SKIP_ERROR, mesgstr);
		      errnum++;
		    }
		  if (retstat == 0)
		    {		/* check for bad cells in same location */
		      walking_write(taddr, loop);
		      if ((walking_read(taddr, loop, field, 2)) != 1) /* errors */
			{
			  errnum = 0;
			  while ((memerr[errnum].pixloc > 0) && (errnum < 3000))
			    {
			      sprintf(mesgstr,
				      "Walking Mem.: (Cell Chk.)Data Exp = 0x%x, Obs = 0x%x, Field %d, Pixel# %d",
				      memerr[errnum].wpatt, memerr[errnum].rpatt,
				      field, memerr[errnum].pixloc);
			      errorlog(SKIP_ERROR, mesgstr);
			      errnum++;
			    }
			}
		    }		/* if retstat == 0 */
		}
	    }
	}
    }				/* for loop= */
  }				/* for field=  */
  return(0);
}


walking_write(taddr, loop)
     u_int *taddr;
     int   loop;
{
  int i, subcmd;
  u_int data, rdata, wdata;

  subcmd = MEMPRST;
  ioctl(vfcfd, VFCSCTRL, &subcmd);
  zaptopmem (taddr, WRITE);	/* throwaway top 248 pixels */
  data = 0x00010000;	/* initial data */
  for (i = 0; i < FIELD_SIZE; ++i) {
    if (loop == 1)
      wdata = data & FRAM_MASK; /* walking 1s pattern */
    else
      wdata = ~data & FRAM_MASK; /* walking 0s pattern */
    *taddr = wdata;		/* write to the FRAM */
    if ((data & FRAM_MASK) == 0x80000000)
      data = 0x00010000;
    else
      data <<= 1;
  }
  
  return(0);
}


walking_read(taddr, loop, field, flag)
     int flag, loop, field;
     u_int *taddr;
{
  int i;
  u_int data, rdata, wdata;
  int subcmd;
  int retcode, mismatchflag;
  

  subcmd = MEMPRST;
  ioctl(vfcfd, VFCSCTRL, &subcmd); /* reset memptr before reading */

  data = 0x00010000;
  errnum = 0;
  if (flag == 2)
    {
      retcode = 1;
      mismatchflag = 0;
    }
  zaptopmem (taddr, READ);	/* throwaway top 248 pixels */
  for (i = 0; i < FIELD_SIZE; ++i)
    {
      rdata = *taddr;
      rdata &= FRAM_MASK;	/* mask off the read data */
      if (loop == 1)
	wdata = data & FRAM_MASK;
      else
	wdata = ~data & FRAM_MASK;
      if (wdata != rdata)
	{
	  if (flag == 1)
	    {			/* entry into memerr structre */
	      if (errnum >= 3000)
		  {
		errorlog(SKIP_ERROR, "memerr structure out of memory");
		goto walkend;
		}
	      else
		{
		  memerr[errnum].pixloc = i;
		  memerr[errnum].rpatt = rdata;
		  memerr[errnum].wpatt = wdata;
		  errnum++;
		}
	    }
	  if (flag == 2)
	    {			/* check against the log */
	      if ((errnum >= 3000) || (memerr[errnum].pixloc < 0))
		;
	      else
		{
		  if ((memerr[errnum].pixloc == i) &&
		      (memerr[errnum].rpatt == rdata))
		    retcode = 0;
		  else
		    {
		      mismatchflag = 1;
		    }
		  errnum++;
		}
	    }
	  if (flag == 3) /* Engg debug flag */
	    {
	      sprintf(mesgstr,
	      "Walking Mem: Data Exp. = 0x%x - Data Obs. = 0x%x, Field %d, Pixel# %d",
		     wdata, rdata, field, i);
	      errorlog(SKIP_ERROR, mesgstr);
	    }
	}
      if ((data & FRAM_MASK) == 0x80000000)
	data = 0x00010000;
      else
	data <<= 1;
    }

 walkend:
  if (flag == 1)
    {
      retcode = errnum;
      if (errnum < 3000)
	memerr[errnum].pixloc = -1;
      else
	memerr[2999].pixloc = -1;
    }
  if ((flag == 2) && mismatchflag)
    retcode = 2;
  return(retcode);
  
}


unique_test()
{

  int incr, field, data;
  int rdata, lcnt;
  int subcmd, retstat;
  u_int *taddr;


  incr = 0x10 << 16;		/* incr. data: it is left-shifted
				 * to align with the upper bytes */

  subcmd = MEMPRST;
  ioctl(vfcfd, VFCSCTRL, &subcmd);
  zaptopmem (vfc_port1, WRITE);	/* increment pointers for port1 and port2 */
  zaptopmem (vfc_port2, WRITE);

  data = 0;			/* initial data */
  for (lcnt = 0; lcnt < 0x2000; ++lcnt) {
    *vfc_port1 = data & FRAM_MASK;
    data += incr;
    *vfc_port2 = data & FRAM_MASK;
    data += incr;
  }

  if (enggflag)
    uniq_read(3);
  else
    {
      if (uniq_read(1) > 0)	/* first loop */
	{
	  errnum = 0;
	  retstat = uniq_read(2);
	  if (retstat == 1)
	    {
	      errnum = 0;
	      while ((memerr[errnum].pixloc > 0) && (errnum < 3000))
		{
		  sprintf(mesgstr,
			  "Uniq Mem: (Transient) Data Exp = 0x%x, Obs = 0x%x, Pixel# %d",
			  memerr[errnum].wpatt, memerr[errnum].rpatt, memerr[errnum].pixloc);
		  errorlog(SKIP_ERROR, mesgstr);
		}
	    }
	  if (retstat == 2)
	    {
	      errnum = 0;
	      while ((memerr[errnum].pixloc > 0) && (errnum < 3000))
		{
		  sprintf(mesgstr,
			  "Uniq Mem: Data Exp = 0x%x, Obs = 0x%x, Pixel# %d",
			  memerr[errnum].wpatt, memerr[errnum].rpatt, memerr[errnum].pixloc);
		  errorlog(SKIP_ERROR, mesgstr);
		}
	    }
	}
    }

  return(0);

}


uniq_read(flag)
     int flag;
{
  int incr, field, data;
  int rdata, lcnt;
  int subcmd;
  u_int *taddr;
  int retcode, mismatchflag = 0;

  subcmd = MEMPRST;
  ioctl(vfcfd, VFCSCTRL, &subcmd); /* reset memptr */
  errnum = 0;
  incr = 0x10 << 16;
  if (flag == 2)
    retcode = 1;
  for (field = 1; field <= 2; field++) {
    if (field == 1) {
      data = 0;
      taddr = vfc_port1;
    }
    else {
      data = incr;
      taddr = vfc_port2;
    }

    zaptopmem (taddr, READ);
    for (lcnt = 0; lcnt < 0x1000; ++lcnt) {
      rdata = *taddr;
      rdata &= FRAM_MASK;
      if ((data & FRAM_MASK) != rdata)
	{
	  if (flag == 1)
	    {
	      if (errnum >= 3000)
		  {
		errorlog(SKIP_ERROR, "memerr structure out of memory");
		goto uniqend;
		}
	      else
		{
		  memerr[errnum].pixloc = lcnt;
		  memerr[errnum].rpatt = rdata;
		  memerr[errnum].wpatt = data & FRAM_MASK;
		  errnum++;
		}
	    }
	  if (flag == 2)
	    {
	      if ((errnum >= 3000) || (memerr[errnum].pixloc < 0))
		;
	      else
		{
		  if ((memerr[errnum].pixloc == lcnt) &&
		      (memerr[errnum].rpatt == rdata))
		    retcode = 0;
		  else
		    {
		      mismatchflag = 1;
		    }
		  errnum++;
		}
	    }
	  if (flag == 3)
	    {
	      sprintf(mesgstr,
		      "Uniq. Mem.: Data Exp. = 0x%x, Obs. = 0x%x, Field %d, Pixel# %d",
		      data & FRAM_MASK, rdata, field, lcnt);
	      errorlog(SKIP_ERROR, mesgstr);
	    }
	}
      data += 2*incr;
    }
  }

 uniqend:
  if (flag == 1)
    {
      retcode = errnum;
      if (errnum < 3000)
	memerr[errnum].pixloc = -1;
      else
	memerr[2999].pixloc = -1;
    }
  if ((flag == 2) && mismatchflag)
    retcode = 2;

  return(retcode);
}

/* Central error-handler */
errorlog(errcode, errmsg)
     int errcode;
     char *errmsg;
{

  send_message(errcode, ERROR, errmsg);

  /*  If the error code is non-zero, then the send_message
   *  will not return back to us. All non-fatal errors are
   *  logged as 0 error code. We count those errors.
   */
     
  ++errors;			/* increment error count */
  if (errors >= ERROR_LIMIT)
    send_message(TOO_MANY_ERRS, ERROR, "Too many errors \n");
  
  return(0);
}


/*
 * clean_up(), contains necessary code to clean up resources before exiting.
 * Note: this function is always required in order to link with sdrtns.c
 * successfully.
 */
clean_up()
{
}

