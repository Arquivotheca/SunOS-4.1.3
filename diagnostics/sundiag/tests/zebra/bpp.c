/* bpp.c 3/10/90 Copyright Sun Micro */

/*
 * ---------------------------------------------------------------------------
 * |                        Filename     : bpp.c                             |
 * |                        Originator   : Massoud Hammadi                   |
 * |                        Date         : March 10, 90                      |
 * ---------------------------------------------------------------------------   */

#ifndef lint
static  char sccsid[] = "@(#)bpp.c 1.1 7/30/92 Copyright 1986 Sun Microsystems,Inc.";
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/fcntl.h>
#include <sys/ioccom.h>
#include <sys/filio.h>
#include <sys/mman.h>
#include <signal.h>

#include "zebra.h"
#include "zebra_msg.h"
#include "sdrtns.h"
#include "../../../lib/include/libonline.h"        /* online library include */
#include "bpp_io.h"

#include <pixrect/pixrect.h>
#include <pixrect/pr_io.h>
#include <pixrect/memvar.h>
#include <rasterfile.h>

#define FALSE 0
#define TRUE  1

 
#define MODE         (0666|IPC_CREAT)
#define OMODE1       (O_WRONLY)                      /* write only mode */
#define OMODE2       (O_RDONLY)                      /* read only mode */
#define MEMSC        128000               /* memory set and clear buffer size */

static char dev_name[12];
static char open_bpp[12];            /* string containig arg to open bpp dev */
u_char    *bppw;                             /* pointers to ASCII characters */
u_long    do_sleep;                     /* time to sleep the printing process*/
int       bppflg, wflg,  devflg, fastflg, medflg, extflg;
int       start_flg = FALSE; 
int       fdp, errsave, pidp, pidmemp;     /* file descriptor for bpp device */
extern int errno;

struct interface{
       char *device;
       }testing[] = {
           "lpvi0",
           "lpvi1",
           "lpvi2",
           "bpp0",
           "bpp1",
           "bpp2"
           };


struct bpp_transfer_parms bpp_trans;              /* bpp transfer parameters */
struct bpp_pins bpp_pin;                          /* bpp output pins struct */
struct bpp_error_status bpp_errs;                 /* bpp error structure */
int starttest = 0;                                 /* flag to start the test */

void startmemfunct()      /* function to start the test using SIGUSR2 signal */
{
        start_flg = TRUE;
        signal(SIGUSR2, SIG_IGN);
}

void exit_proc() {
        send_message(0, VERBOSE, "Process %d is being terminated\n",
                                                          getpid());
        signal(SIGINT, SIG_IGN);
        exit(0);
}

extern int      process_zebra_args();
extern int      routine_usage();

main(argc, argv)
int  argc;
char *argv[];
{
     int i = 0;
     versionid = "1.2";                         /* SCCS version id */
     

     bppflg = wflg = fastflg = medflg= extflg = FALSE;
     devflg = start_flg = FALSE;
     fdp = 0;
     device_name = dev_name;
     strcpy(test_name, "zebra"); 
     test_init(argc, argv, process_zebra_args, routine_usage, testbp_usage_msg);

     send_message(0, VERBOSE, start_test_msg, test_name, device_name); 
     dev_test();
     send_message(0, VERBOSE, end_test_msg, test_name, device_name);

     clean_up();
     test_end();                                      /* sundiag normal exit */
}




process_zebra_args(argv, arrcount)
char *argv[];
int arrcount;
{
     int   i;
     

     if (strncmp(argv[arrcount], "D=", 2) == 0){
      for (i = 0; i < 6; ++i) {
       if (strcmp(testing[i].device, &argv[arrcount][7]) == 0){
         devflg = TRUE;
         strcpy(dev_name, &argv[arrcount][2]);    
       }
      }
      if (!devflg)    return(FALSE);
     }
     else if (strcmp(argv[arrcount], "W") == 0){
              wflg = TRUE; 
     }
     else if (strncmp(argv[arrcount], "M=", 2) == 0){
         if (strcmp(&argv[arrcount][2], "fast") == 0){
           fastflg = TRUE;
           do_sleep = 0;                             /* sleep for 0 second */
         }
         if (strcmp(&argv[arrcount][2], "medium") == 0){
           medflg = TRUE;
           do_sleep = (12 * 60);                     /* sleep for 6 minutes */  
         }
         if (strcmp(&argv[arrcount][2], "extended") == 0){
           extflg = TRUE;
           do_sleep = (30 * 60);                     /* sleep for 30 minutes */
         } 
     }else return(FALSE);

     return(TRUE);
}





routine_usage()
{
     (void) send_message(0, CONSOLE, routinebp_msg);
} 


routine_comm()
{
     (void) send_message(0, CONSOLE, testbp_usage_msg);
}



dev_test()
{
     union wait        status;
     u_char            pg_size, buff[64], mem_cl[MEMSC];
     u_long            bsize;
     int               BYTEWP;
     int               i, j, k, retioct, result, mask, n;

     char    open_argument[15];      /* string containing arg to open() */
     char    *open_arg;              /* pointer to string above */
     int     size;                   /* size of mapped mem in bytes */
     int     rone = 0xFF;            /* expected values in memory */
     int     rzero = 0x00;
     int     bit_shift;

     func_name = "dev_test";
     TRACE_IN
     bsize = 80*24;

     if (wflg && (fastflg || medflg || extflg)){

          /* open the bpp interface with write only permission */
          if ((fdp = open(dev_name, OMODE1)) == -1) {
              errsave = errno;
              send_message(OPEN_ERROR, ERROR, open_err_msg, dev_name);
          }
          send_message(0, VERBOSE, "opened %s fdp = %d\n", device_name, fdp);
	  bppw = (u_char *) malloc(bsize + 51);
          if (bppw != NULL) {
             send_message(0, VERBOSE, "%s:initializing text data",
                                                         dev_name);
             init_txt(bsize);
             send_message(0, VERBOSE,"Starting writing to %s\n",
                                                         dev_name);
             if ((BYTEWP = write(fdp,bppw,(bsize + 50))) != (bsize + 50)) {
	         printf("BYTEWP = %d\n", BYTEWP);
                 if(BYTEWP == -1 || BYTEWP < (bsize + 50)) {
                      errsave = errno;
                      free(bppw);
                      get_errslpv(fdp);
	              printf("BYTEWP = %d\n", BYTEWP);
                      send_message(WRITE_ERROR, FATAL, write_fail_msg,
                              errsave, device_name, bsize + 50, BYTEWP);
                  }
              } 
              send_message(0, VERBOSE,"RANDOM:Wrote %d bytes to %s\n",
                                                     BYTEWP, dev_name);
              sleep(5);
              init_txt_h(bsize);
              if ((BYTEWP = write(fdp,bppw,(bsize + 51))) != (bsize + 51)) {
                  if(BYTEWP == -1 || BYTEWP < (bsize + 51)) {
                       errsave = errno;
                       free(bppw);
                       get_errslpv(fdp);
                       send_message(WRITE_ERROR, FATAL, write_fail_msg,
                                errsave, device_name, bsize + 51, BYTEWP);
                   }
               }  
               send_message(0, VERBOSE,"FIXED:Wrote %d bytes to %s\n",
                                                     BYTEWP, dev_name);
               free(bppw);
	       close(fdp);
          }
          else {
	       send_message(-2, ERROR, "%s: Malloc couldn't allocate memory", 
                                                          device_name);
	  }
	  /* open the bpp interface with read only permission */
          if ((fdp = open(dev_name, OMODE2)) == -1) {
              errsave = errno;
	      send_message(0, VERBOSE, "Read PERM errsave = %d fdb = %d\n",
                                                             errsave, fdp);
              send_message(OPEN_ERROR, ERROR, open_err_msg, dev_name);
          }
          switch (pidmemp = fork()){
             case -1: errsave = errno; 
                      send_message(0, ERROR, "%s: main:fork %s", device_name,
                                  errmsg(errsave));
                      break;
             case 0: signal(SIGUSR2, startmemfunct);
                     signal(SIGINT, exit_proc);
                     send_message(0, VERBOSE,"%s:start DMA memory test\n",
                                              device_name);
                     while (start_flg == FALSE) {
                        if(ioctl(fdp, BPPIOC_GETPARMS, &bpp_trans) == -1) {
                            errsave = errno;
                            send_message(0, ERROR, ioctl_err_msg,
                                         device_name, errmsg(errsave));
                            exit(errsave);
                        }
                        bpp_trans.read_handshake = BPP_SET_MEM;
 
                        /* write ones to memory */
                        if(ioctl(fdp, BPPIOC_SETPARMS, &bpp_trans) == -1) {
                            errsave = errno;
                            send_message(0, ERROR, ioctl_err_msg,
                                         device_name, errmsg(errsave));
                            exit(errsave);
                        }
                        if (read(fdp,mem_cl,MEMSC) != MEMSC) 
                          errsave = errno;
                          
                        k = 0;
                        while ((mem_cl[k] == rone) && (k < MEMSC))
                          ++k;
                        if (k < MEMSC) {
                            send_message(0, ERROR, check_err_msg,
                                         rone, mem_cl[k], device_name);
                            exit(errsave);
                        }
                        bpp_trans.read_handshake = BPP_CLEAR_MEM;
          
                        /* write zeroes to memory */
                        if(ioctl(fdp, BPPIOC_SETPARMS, &bpp_trans) == -1) {
                            errsave = errno;
                            send_message(0, ERROR, ioctl_err_msg,
                                         device_name, errmsg(errsave));
                            exit(errsave);
                        }
                        if (read(fdp,mem_cl,MEMSC) != MEMSC)
                          errsave = errno;
                        k = 0;
                        while (mem_cl[k] == rzero && k < MEMSC)
                          ++k;
                        if (k < MEMSC) {
                            send_message(0, ERROR, check_err_msg,
                                         rzero, mem_cl[k], device_name);
                            exit(errsave);
                        }  
                        /* give up resources to other bpp devices */
                     }
                     send_message(0, VERBOSE,"%s:Finished DMA memory test\n",
                                                                device_name);
                     exit(0);
           
          }
	  sleep(do_sleep);
          signal(SIGUSR2, SIG_IGN);
          kill(pidmemp, SIGUSR2); 
          send_message(0, VERBOSE, "closing %s", dev_name); 
          if (close(fdp) == -1)
              send_message(0, VERBOSE, "error in close %d\n", errno);
          /* open the bpp interface with write only permission */
          sleep(10);
          if ((fdp = open(dev_name, OMODE1)) == -1) {
              errsave = errno;
              send_message(0, VERBOSE, "WRITE PERM errsave = %d fdp = %d\n",
                                                        errsave, fdp);     
              send_message(OPEN_ERROR, ERROR, open_err_msg, dev_name);     
          }
          switch (pidp = fork()){
             case -1: errsave = errno; 
                      send_message(0, ERROR, "%s: main:fork %s", device_name,
                                   errmsg(errsave));
                      kill(pidmemp, SIGUSR2);
                      break;
             case 0:  /* call exit routine to terminate the process */
                      signal(SIGINT, exit_proc);

                      /* initialize the bpp buffer */
                      sleep(do_sleep);
                      signal(SIGUSR2, SIG_IGN);

                      /* send termination signal to DMA mem set and clear */
/*
                      kill(pidmemp, SIGUSR2);
*/
                      bppw = (u_char *) malloc(bsize + 51);
                      if (bppw != NULL) {
                         send_message(0, VERBOSE, "%s:initializing text data",
                                               dev_name);
                         init_txt(bsize);
                         /* wait until DMA process is terminated */
                         sleep(5);
                         send_message(0, VERBOSE,"Starting writing to %s\n",
                                                                  dev_name);
                         if ((BYTEWP = write(fdp,bppw,(bsize + 50))) !=
                                                          (bsize + 50)) {
                            if(BYTEWP == -1 || BYTEWP < (bsize + 50)) {
                               errsave = errno;
                               free(bppw);
                               get_errslpv(fdp);
                               close(fdp);
                               send_message(0, FATAL, write_fail_msg,
                                        errsave, device_name, bsize + 50, BYTEWP);
                               exit(errsave);
                            }
                         } 

                        send_message(0, VERBOSE,"RANDOM:Wrote %d bytes to %s\n",
                                                            BYTEWP, dev_name);
                         sleep(5);
                         init_txt_h(bsize); 
                         if ((BYTEWP = write(fdp,bppw,(bsize + 51))) !=
                                                          (bsize + 51)) {
                            if(BYTEWP == -1 || BYTEWP < (bsize + 50)) {
                               errsave = errno;
                               free(bppw);
                               get_errslpv(fdp);
                               close(fdp);
                               send_message(0, FATAL, write_fail_msg,
                                        errsave, device_name, bsize + 51, BYTEWP);
                               exit(errsave);
                            }
                         }  
                         send_message(0, VERBOSE,"FIXED:Wrote %d bytes to %s\n",
                                                            BYTEWP, dev_name);
                        
                         free(bppw);
                      }
                      else
                        send_message(0, WARNING, "%s: Malloc couldn't allocate memory", device_name);
                      exit(0);
          }/* switch() */
     }else {
        routine_comm();  
        routine_usage(); 
     }
     while(wait(&status) != -1)
     ;
     send_message(0, VERBOSE, "status = %d\n", status.w_retcode);
     if ((status.w_retcode > 0) && (status.w_retcode <= 90) )
         send_message(WRITE_ERROR, ERROR, "EXITING:error=%d %s\n",
                                         errsave, errmsg(errsave));
     TRACE_OUT
}



/*---------------------------------------------------------------------------*/
              /* function to initialize text data for bpp */
/*---------------------------------------------------------------------------*/
int init_txt(bsize)
u_long bsize;
{
    u_long i, k, l ;
    u_char *bbuff, j;

    func_name = "init_txt";
    TRACE_IN

    j = '0';
    k = 2;

    bbuff = bppw;
    for (i = 0; i < bsize + 51; i++)
      *(bbuff+i) = 0;
    *(bbuff+0) = 0xA;            /* Cr */
    *(bbuff+1) = 0xD;            /* Lf */

    l = 1;
    for (i = 1; i <= bsize; i++) {
     if (l == 80) {               /* insert (<CR>, <LF> after 80th character */
      *(bbuff+k) = 0xA;           /* Line feed */
      k++;
      *(bbuff+k) = 0xD;
      k++;                        /* Carriage Return */
      l = 1;
     }
     else {
       *(bbuff+k) = j++;         /* Repeat printable ascii characters */
       k++;
       l++;
     }

     /* wrap around if exhusted printable ascii character set */
     if (j > '~')
        j = '0';
    }
    TRACE_OUT
} 



int init_txt_h(bsize)
u_long bsize;
{
    u_long i, k, l ;
    u_char *bbuff, j;

    func_name = "init_txt_h";
    TRACE_IN

    k = 2;

    bbuff = bppw;
    for (i = 0; i < bsize + 51; i++)
      *(bbuff+i) = 0;
    *(bbuff+0) = 0xA;            /* Cr */
    *(bbuff+1) = 0xD;            /* Lf */

    l = 1;
    for (i = 1; i <= bsize; i++) {
     if (l == 80) {               /* insert (<CR>, <LF> after 80th character */
      *(bbuff+k) = 0xA;           /* Line feed */
      k++;
      *(bbuff+k) = 0xD;
      k++;                        /* Carriage Return */
      l = 1;
     }
     else {
       *(bbuff+k) = 'H';         /* Repeat printable ascii characters */
       k++;
       l++;
     }
    }
    *(bbuff+k) = 0xC;		 /* New page */
    TRACE_OUT
}




/*---------------------------------------------------------------------------*/
   /*function to report bpp detailed info on the current error condition*/
/*---------------------------------------------------------------------------*/
get_errslpv(fdvp)
int fdvp;
{
    int retioct;

    func_name = "get_errslpv";
    TRACE_IN

      if (errsave == EBADF || errsave == EBUSY || errsave == EIO
          || errsave == EINVAL || errsave == ENXIO || errsave == ENODEV)
      {
       if(ioctl(fdvp, BPPIOC_GETERR, &bpp_errs) != -1)
       {
         send_message(0, ERROR, "%s: %s\n\n\
                      \t\t Time out Error:%x\n\
                      \t\t Bus Error: \t%x\n\
                      \t\t Pin Status: \t%x\n", device_name, errmsg(errsave),
                           bpp_errs.timeout_occurred, bpp_errs.bus_error,
                           bpp_errs.pin_status);
       }  
       else{ 
         send_message(0, ERROR, "%s\n", errmsg(errsave));
       }
      }
    TRACE_OUT
}



/*---------------------------------------------------------------------------*/
          /* function to reset all parameters and close bpp device */ 
/*---------------------------------------------------------------------------*/
clean_up()
{
  func_name = "clean_up";
  TRACE_IN

  /* start terminating all processes created by parent, this is necessary
   * when sundiag is terminating the test abnormally.
   */
  if (pidp > 0) {
    if (kill(pidp, SIGINT) == -1) {
       send_message(0, VERBOSE, "process %d is terminated: %s\n",
                     pidp, errmsg(errno));
    }
  }  
  if (pidmemp > 0) {
    if (kill(pidmemp, SIGINT) == -1) {
       send_message(0, VERBOSE, "process %d is terminated: %s\n",
                     pidmemp, errmsg(errno));
    }
  } 
  close(fdp);

  TRACE_OUT
}
