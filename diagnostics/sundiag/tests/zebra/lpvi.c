/* lpvi.c 3/10/90 Copyright Sun Micro */

/*
 * ---------------------------------------------------------------------------
 * |                        Filename     : lpvi.c                            |
 * |                        Originator   : Massoud Hammadi                   |
 * |                        Date         : March 10, 90                      |
 * ---------------------------------------------------------------------------
 */
                                                        
#ifndef lint
static  char sccsid[] = "@(#)lpvi.c 1.1 7/30/92 Copyright 1986 Sun Microsystems,Inc.";
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
#include "lpviio.h"
/*
#include <unbdev/lpvixeng.h>
*/
#include <pixrect/pixrect.h>
#include <pixrect/pr_io.h>
#include <pixrect/memvar.h>
#include <rasterfile.h>

#define FALSE 0
#define TRUE  1

 
#define MODE         (0666|IPC_CREAT)
#define OMODE1       (O_WRONLY)                      /* write only mode */
#define OMODE2       (O_RDWR)                        /* read and write mode */
#define TOTAL_PASS   20                              /* number of test loops */
#define X_MARGIN_BYTES_300  1500                     /* left margin default */
#define Y_MARGIN_SCANS_300  1500                     /* top margin default */
#define X_MARGIN_BYTES_400  2000                     /* left margin default */
#define Y_MARGIN_SCANS_400  2000                     /* top margin default */
#define DEF_IMG_BYTES_SCAN      307         /* letter */
#define DEF_IMG_SCANS_PAGE      3264        /* letter */
#define DEF_RESLN       300

static char dev_name[12];
static char img_name[20];
static char open_bpp[12];           /* string containig arg to open bpp dev */
long      strtol();
u_char    *vdbuff[3];                        /* pointers to bitmap patters */
u_long    tmar, lmar, res, do_sleep;         /* top and left margines */
int       lpvflg, wflg,  devflg, imgflg, fastflg, medflg, extflg;
int       errsave = 0;
int       start_flg = FALSE; 
FILE      *imgfd;
int       fdv;                              /* file descriptor for SBus slot */
int       pidlpv[3], pidv;
extern int errno;

struct page{                             /* paper size structure for 300 res */
       char *p_size;
       u_long x_scan;
       u_long y_scan;
       }p300[] = {
           "B4", 2944, 4208,
           "A4", 2393, 3416,
           "B5", 2056, 2944,
           "LET", 2464, 3208,
           "LEG13", 2464, 3808,
           "LEG14", 2464, 4112
           };

struct page p400[] = {                  /* paper size structure for 400 res */
           "B4", 3928, 5608,
           "A4", 3184, 4552,
           "B5", 2744, 3928,
           "LET", 3280, 4280,
           "LEG13", 3280, 5080,
           "LEG14", 3280, 5480
           };

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

extern void printer_display1(), printer_display2(), printer_display3();
struct funcptr{
      void (*func)();
      }disfunc[] = {
         printer_display1,
         printer_display2,
         printer_display3
         };                     

struct lpvi_page pg_dim;                   /* lpvi page dimension structure */
struct lpvi_inq lp_inf;                           /* lpvi inquery structure */
struct lpvi_err lp_errs;                          /* lpvi error structure */
u_long image_height, image_width;    /* width and height of the bitmap image */
int starttest = 0;                                 /* flag to start the test */
static int flag1, flag2;
/*void startfunct()*/      /* function to start the test using SIGUSR1 signal */
/*
{
        starttest = 1;
        signal(SIGUSR1, SIG_IGN);
        signal(SIGUSR2, SIG_IGN);
}
*/
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
     

     lpvflg = wflg = fastflg = medflg= extflg = FALSE;
     devflg = imgflg = start_flg = FALSE;
     tmar = lmar = res = fdv = 0;
     device_name = dev_name;
     strcpy(test_name, "zebra"); 
     test_init(argc, argv, process_zebra_args, routine_usage, test_usage_msg);

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
     else if (strncmp(argv[arrcount], "I=", 2) == 0){
         if (strcmp(&argv[arrcount][2], "default") == 0){
           imgflg = TRUE; 
         }
         else strcpy(img_name, &argv[arrcount][2]);
     }
     else if (strncmp(argv[arrcount], "M=", 2) == 0){
         if (strcmp(&argv[arrcount][2], "fast") == 0){
           fastflg = TRUE;
           do_sleep = 0;                            /* sleep for 0 second */
         }
         if (strcmp(&argv[arrcount][2], "medium") == 0){
           medflg = TRUE;
           do_sleep = (12 * 60);                     /* sleep for 6 minutes */  
         }
         if (strcmp(&argv[arrcount][2], "extended") == 0){
           extflg = TRUE;
           do_sleep = (30 * 60);                     /* sleep for 30 minutes */
         } 
     }
     else if (strncmp(argv[arrcount], "R=", 2) == 0){
         res = atoi(&argv[arrcount][2]);
     }else return(FALSE);

     return(TRUE);
}





routine_usage()
{
     (void) send_message(0, CONSOLE, routine_msg);
} 


routine_comm()
{
     (void) send_message(0, CONSOLE, test_usage_msg);
}



dev_test()
{
     union wait        status;
     Pixrect           *pr;
     struct rasterfile rh;
     colormap_t        colormap;
     u_char            pg_size, buff[64], mem_cl[256];
     u_long            x, y, W, H, vsize, bsize, BYTEW[3], BYTEWP, BYTERP;
     u_long            Printer_X, temp, temp2, pe;
     char              *shmat(), *ccnt, lbuff0[2], lbuff1[2],lbuff2[2];
     int               i, j, k, retioct, result, mask, n;

     char    open_argument[15];      /* string containing arg to open() */
     char    *open_arg;              /* pointer to string above */
     int     size;                   /* size of mapped mem in bytes */
     int     rone = 0xFF;            /* expected values in memory */
     int     rzero = 0x00;
     int     bit_shift;

     func_name = "dev_test";
     TRACE_IN
     bsize = 80*12;

     if (wflg && (res != 0) && (fastflg || medflg || extflg)) {
       /* open the video laser printer interface */
       if((fdv = open(dev_name, OMODE2)) == -1) {
         errsave = errno;
         get_errslpv(fdv);
         send_message(OPEN_ERROR, ERROR, open_err_msg, dev_name);
       }
       send_message(0, VERBOSE, "opened %s fdv = %d\n", dev_name, fdv);

       /* get lpv paper size */
       if((retioct = ioctl(fdv, LPVIIOC_INQ, &lp_inf)) == -1) {
         errsave = errno;
         get_errslpv(fdv);
         send_message(IOCTL_ERROR, ERROR, ioctl_err_msg, dev_name);
       } 
       else{
         pg_size = lp_inf.papersize;
         switch(pg_size){
             case 0x00: errsave = errno;
                        send_message(NOTRAY, ERROR, paper_err_msg,
                                     device_name, errmsg(errsave));
                    break;
              case 0x01:
                    get_pg_size("B4");
                    break;
              case 0x02:
                    get_pg_size("A4");
                    break;
              case 0x03:
                    get_pg_size("LET");
                    break;
              case 0x05:
                    get_pg_size("B5");
                    break;
              case 0x06:
                    get_pg_size("LEG14");
                    break;
              case 0x07: 
                    get_pg_size("LEG13");
                    break;
         } /* switch */
       }/* else */
       
/*
       switch(pidv = fork()){
              case -1: errsave = errno;
                       send_message(0, ERROR, "%s: main:fork %s",
                                    device_name, errmsg(errsave));
                       break;
 
              case 0:  signal(SIGUSR2, startmemfunct);
                       signal(SIGINT, exit_proc);
                       send_message(0, VERBOSE,"%s:start DMA memory test\n",
                                              device_name);
                       while (start_flg == FALSE) {
                          if(ioctl(fdv, BPPIOC_GETPARMS, &bpp_trans) == -1) {
                              errsave = errno;  
                              send_message(0, ERROR, ioctl_err_msg,
                                           open_bpp, errmsg(errsave));
                              exit(errsave);
                          }
                          bpp_trans.read_handshake = BPP_SET_MEM;
 
*/
                          /* write ones to memory */
/*
                          if(ioctl(fdp, BPPIOC_SETPARMS, &bpp_trans) == -1) {
                              errsave = errno;
                              send_message(0, ERROR, ioctl_err_msg,
                                           open_bpp, errmsg(errsave));
                              exit(errsave);
                          }
                          if (read(fdp,mem_cl,256) != 256)
                             errsave = errno;
                          k = 0;
                          while ((mem_cl[k] == rone) && (k < 256))
                            ++k;
                          if (k < 256) {
                             send_message(0, ERROR, check_err_msg,
                                          rone, mem_cl[k], open_bpp);
                             exit(errsave);
                          }
                          bpp_trans.read_handshake = BPP_CLEAR_MEM;
 
*/
                          /* write zeroes to memory */
/*
                          if(ioctl(fdp, BPPIOC_SETPARMS, &bpp_trans) == -1) {
                              errsave = errno;
                              send_message(0, ERROR, ioctl_err_msg,
                                           open_bpp, errmsg(errsave));
                              exit(errsave);
                          }
                          if (read(fdp,mem_cl,256) != 256)
                            errsave = errno;
                          k = 0;
                          while (mem_cl[k] == rzero && k < 256)
                            ++k;
                          if (k < 256) {
                             send_message(0, ERROR, check_err_msg,
                                          rzero, mem_cl[k], open_bpp);
                             exit(errsave);
                          }
*/
                          /* give up resources to other lpvi devices */
/*
                          sleep(30);
                       }
                       send_message(0, VERBOSE,"%s:Finished DMA memory test\n",
                                                                device_name);
                       exit(0);
       }
*/
       if (imgflg) {
          /* set page dimentions in lpv driver structure */
          tmar = (res == 400) ? Y_MARGIN_SCANS_400 :Y_MARGIN_SCANS_300;
          lmar = (res == 400) ? X_MARGIN_BYTES_400 :X_MARGIN_BYTES_300;
          /* set height of page in pixels */
          pg_dim.page_length = (image_height - tmar);
          /* set width of page in pixels */
          pg_dim.page_width = (image_width - lmar);
          /* set width of image in bytes */
          pg_dim.bitmap_width = ((image_width - lmar)+7)/8;
          /* set top margin */
          pg_dim.top_margin = tmar;
          /* set left margin */
          pg_dim.left_margin = lmar;
          /* set resolution */
          pg_dim.resolution = res;
          
          if((retioct = ioctl(fdv, LPVIIOC_SETPAGE, &pg_dim)) == -1) {
            errsave = errno;
            get_errslpv(fdv);
            send_message(IOCTL_ERROR, ERROR, ioctl_err_msg, dev_name);
          }

          /* size of the image in byte */
          Printer_X = ((image_width - lmar)+7)/8;
          vsize = (((image_width - lmar)+7)/8) * (image_height - tmar);

          W = (image_width - lmar);
          H = (image_height - tmar);

          for (i = 1; i < 3; i++) {
            switch(pidlpv[i] = fork()){
              case -1: errsave = errno;
                       send_message(0, ERROR, "%s: main:fork %s", device_name,
                                    errmsg(errsave));
/*
                       kill(pidv, SIGUSR2);
*/
                       break;
              case 0: /* signal(SIGUSR1, startfunct); */
                      /* call exit routine to terminate the process */
                      signal(SIGINT, exit_proc);
/*
                      while (!starttest)
                         sleep(1);
                      kill(pidv, SIGUSR2);    
*/
                      /* allocate memory to bitmap buffers */
                      vdbuff[i] = (u_char *) malloc(vsize);
                      sleep(2);   /* give chance to malloc to set up memory */
                      if (vdbuff[i] != NULL) {
                         /* initialize bitmap buffers */
                         send_message(0, VERBOSE, "\n%s: Initializing.\n",
                                                              device_name);
                         for (k=0; k < vsize; k++) 
                            *(vdbuff[i]+k) = 0;

                         /* set the bitmap patterns */
                         disfunc[i].func(Printer_X, vsize); 

                         sleep(2);  /* wait until DMA process is terminated */
                         if ((BYTEW[i] = write(fdv,vdbuff[i],vsize)) !=  vsize){
                            if (i == 1)
                               errsave = errno;
                            send_message(0, VERBOSE, "%s: bytes written %d \n",
                                                           dev_name, BYTEW[i]);
                            if(BYTEW[i] == -1 || BYTEW[i] < vsize) {
                                free(vdbuff[i]);
                                get_errslpv(fdv);
                                exit(errsave);
                            } 
                         } 
                         else {
                            free(vdbuff[i]);
                            /* sleep depending on selected mode */ 
                            if (!errsave) 
                                sleep(do_sleep); 
                         }
                      }
                      else 
                        send_message(0, WARNING, "%s: Malloc couldn't allocate memory", device_name);
                      exit(0);
            }/* switch() */
          }

          signal(SIGUSR2, SIG_IGN);
/*
          signal(SIGUSR1, SIG_IGN);
          kill(pidlpv[1], SIGUSR1);
          kill(pidlpv[2], SIGUSR1);
*/
       }else {
/*
          kill(pidv, SIGUSR2);	
*/
          /* select rasterfile to which comply with resolution */
	  if (strcmp(img_name, "57fonts") == 0) {
	     if (res == 300)
	        strcpy(img_name, "57fonts.300");
	     else
                strcpy(img_name, "57fonts.400"); 
          }
          /* read rasterfile header */
          if ((imgfd = fopen(img_name, "r")) == NULL)
             send_message(OPEN_ERROR, ERROR, open_err_msg, img_name);
          if (result = pr_load_header(imgfd, &rh)) 
             send_message(GEN_ERROR, ERROR,
                          "zrf: error reading rf header  %x for %s\n",
                           result, device_name);

          /* read colormap and image */
          if (result = pr_load_colormap(imgfd, &rh, &colormap) ||
                  !(pr = pr_load_image(imgfd, &rh, &colormap))) 
             send_message(GEN_ERROR, ERROR,
                          "load: error loading image file  %x for %s\n",
                           result, device_name);

          /* initialize page dimentions for lpv driver structure */
          send_message(0, VERBOSE, "%s: Reading Raster file header\n",
                                                        dev_name); 
          pg_dim.top_margin = 1;
          pg_dim.left_margin = 0;
          tmar = 1;
          lmar = 0;

          /* set page dim from rasterfile header */
	  if ((rh.ras_width > image_width) && (rh.ras_height > image_height))
             send_message(-IOCTL_ERROR, ERROR,"%s: %s raster file too big\n",
                                       device_name, img_name);
          pg_dim.page_length = (rh.ras_height - tmar);   
          pg_dim.page_width = (rh.ras_width -lmar); 
          pg_dim.bitmap_width = ((rh.ras_width - lmar) + 7)/8;
          pg_dim.resolution = res;

          send_message(0, VERBOSE, "%s: top marginw = %d Left Margin = %d\n",
                              dev_name, pg_dim.top_margin, pg_dim.left_margin);

          /* set dimensions for printed page in driver */
          if ((result = ioctl(fdv, LPVIIOC_SETPAGE, &pg_dim)) == -1) {
             errsave = errno;
             get_errslpv(fdv);
             send_message(IOCTL_ERROR, ERROR,ioctl_err_msg,
                           device_name, img_name);
          }
   
          n = pg_dim.bitmap_width*pg_dim.page_length;
          send_message(0, VERBOSE, "%s: image_size = %d img_name = %s\n",
                                    dev_name, n, img_name); 

retry:    if ((result = write(fdv,
             ((struct mpr_data *)pr->pr_data)->md_image, n)) != n) {
                errsave = errno;
                get_errslpv(fdv);
                if (errsave == 0)
                   goto retry;
                else
                   send_message(WRITE_ERROR, ERROR, "EXITING:error=%d %s\n",
                                                  errsave, errmsg(errsave)); 
          }
          /* sleep depending on selected mode */   
          if (!errsave)    
             sleep(do_sleep);
        } 
    }
    while(wait(&status) != -1)
    ;
    send_message(0, VERBOSE, "status = %d\n", status.w_retcode);
    if ((status.w_retcode > 0) && (status.w_retcode <= 90))
       send_message(WRITE_ERROR, ERROR, "EXITING:error=%d %s\n",
                    status.w_retcode, errmsg(status.w_retcode));
    TRACE_OUT
}





/*---------------------------------------------------------------------------*/
                   /* function to get lpv page size */
/*---------------------------------------------------------------------------*/
get_pg_size(psize)
char *psize;
{
   struct page *keypage;
   int done;
   
   func_name = "get_pg_size";
   TRACE_IN 
   done = FALSE; 
   if (res == 300)
     keypage = &p300[0];
   else
     keypage = &p400[0];
   while(!done)
   {
     if (strcmp(keypage->p_size, psize) == 0)
     {
       image_width = keypage->x_scan;   
       image_height = keypage->y_scan;
       done = TRUE;
     }
     else
       ++keypage;
   }
   TRACE_OUT
}




/*---------------------------------------------------------------------------*/
/*function to report lpv and bpp detailed info on the current error condition*/
/*---------------------------------------------------------------------------*/
get_errslpv(fdvp)
int fdvp;
{
    int retioct;

    func_name = "get_errslpv";
    TRACE_IN
    if (errsave == EIO) { 
       if((retioct = ioctl(fdvp, LPVIIOC_INQ, &lp_inf)) != -1)
       {
          send_message(0, FATAL, "%s:\n\n\
          \t-> Paper size: %x, Counters: %d, Print Engine: %s\n",
                       device_name, lp_inf.papersize,
                       lp_inf.counters, lp_inf.engine);
       }
       /* output laser printer error code and error messages */ 
       if((retioct = ioctl(fdvp, LPVIIOC_GETERR, &lp_errs)) != -1)
       { 
          switch (lp_errs.err_code) {
              case 0x01:
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, Error in Main Motor\n",
                                   device_name, lp_errs.err_code);
                     break;
              case 0x02: 
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, ROS out of order\n",
                                device_name, lp_errs.err_code);  
                     break;
              case 0x03:
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, FUSER out of order\n",
                                  device_name, lp_errs.err_code);   
                     break;
              case 0x04:
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, XERO fail happened\n",
                                  device_name, lp_errs.err_code);   
                     break;
              case 0x05: 
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, Interlock open\n",
                                  device_name, lp_errs.err_code);  
                     break; 
              case 0x06:  
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, No Tray Installed\n",
                                  device_name, lp_errs.err_code);   
                     break; 
              case 0x07: 
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, No paper exists in selected tray\n",
                                  device_name, lp_errs.err_code);   
                     break;
              case 0x08:
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, Exit JAM\n",
                                  device_name, lp_errs.err_code);   
                     break;
              case 0x09: 
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, Misfeed JAM\n",
                                  device_name, lp_errs.err_code);  
                     break; 
              case 0x0a:  
                     send_message(0, WARNING, "%s:\n\n\
                    \t-> Error Code: %x, Drum cartridge is nearly exhausted\n",
                                  device_name, lp_errs.err_code);   
                     break; 
              case 0x0b: 
                     send_message(0, WARNING, "%s:\n\n\
                     \t-> Error Code: %x, Deve module is nearly exhausted\n",
                                  device_name, lp_errs.err_code);   
                     break;
              case 0x0c:
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, No Drum cartridge\n",
                                  device_name, lp_errs.err_code);   
                     break;
              case 0x0d: 
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, No Deve cartridge\n",
                                  device_name, lp_errs.err_code);   
                     break;  
              case 0x0e:   
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, Drum cartridge exhausted\n",
                                  device_name, lp_errs.err_code);    
                     break;  
              case 0x0f:   
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, Deve cartridge exhausted\n",
                                  device_name, lp_errs.err_code);    
                     break; 
              case 0x10:  
                     send_message(0, WARNING, "%s:\n\n\
                     \t-> Error Code: %x, Printer is warmming-up\n",
                                  device_name, lp_errs.err_code);
                     break;
              case 0x11:   
                     send_message(0, FATAL, "%s:\n\n\
                     \t-> Error Code: %x, Timeout: no response from printer\n",
                                  device_name, lp_errs.err_code);
                     break;
          }
          switch (lp_errs.err_code) {
             case 0x10:
/*
                  while (ioctl(fdvp, LPVIIOC_TESTIO) == EIO)
                     get_errslpv(fdvp);
*/
                  send_message(0, WARNING, "%s: Wait for printer ready signal, and restart the test\n", dev_name);
                  break; 
             case 0x01: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
             case 0x0c: case 0x0d: case 0x0e: case 0x0f: case 0x011:
                  send_message(0, FATAL, "%s: EIO Error\n", dev_name);
                  break;
          }
       }
    } 
    else 
        send_message(0, ERROR, "%s %d\n", errmsg(errsave), errsave);  

    TRACE_OUT
}



/*---------------------------------------------------------------------------*/
                   /* function to print Horizontal Lines */
/*---------------------------------------------------------------------------*/
void
printer_display1 (Xbyte, msize)
u_long  Xbyte, msize;
{
     register        i, j, k;
     u_char          *memory, *fbaddr, pattern;

     func_name = "printer_display1";
     TRACE_IN
     fbaddr = vdbuff[0];
     pattern = 0xFF;
     memory = fbaddr;
 
     for (i = 0; i < (msize - (40*Xbyte)); i += (20*Xbyte)) {
         for (k = 0; k < Xbyte; k++) {
           *(memory + k) = pattern;
         }
         memory += Xbyte;
         if (i < (msize - (40*Xbyte)))
          memory += (20*Xbyte);
     }
     TRACE_OUT
}




/*---------------------------------------------------------------------------*/
                  /* function to print Vertical Lines */
/*---------------------------------------------------------------------------*/
void
printer_display2 (Xbyte, msize)
u_long  Xbyte, msize;
{
     register        i, j, k;
     int             dx; 
     u_char          *memory, *fbaddr, pattern;
 
 
     func_name = "printer_display2";
     TRACE_IN
     
     dx = (res == 400) ? 53 :41;
     fbaddr = vdbuff[1];
     pattern = 0x80;
     memory = fbaddr;
 
    for (i = 0; i < (msize - Xbyte); i += Xbyte) {
      for (j = 0; j < dx; j++) {
          *(memory + (3*j)) = pattern;
      }
      if (i < (msize - 2*Xbyte))
         memory += Xbyte;
    }
    TRACE_OUT
}




/*---------------------------------------------------------------------------*/
                  /* function to print Grid pattern */
/*---------------------------------------------------------------------------*/
void
printer_display3 (Xbyte, msize)
u_long  Xbyte, msize;
{
    register        i, j, k, l;
    int             Printer_X, dx, offset;
    u_char          *memory, *fbaddr, pattern;
 
 
    func_name = "printer_display3";
    TRACE_IN
    dx = (res == 400) ? 4 : 3;
 
    fbaddr = vdbuff[2];
    pattern = 0x80;
    memory = fbaddr;
    i = offset = 0;
    while (memory < (fbaddr + msize - (120*Xbyte))) {
      if (!((memory-fbaddr)%120)) {
        for (l = 0; l < 120; l++) {
          for (j = 0; j < dx; j++) {
            for (k = 0; k < 20; k++) {
              *(memory + (2*j*20) + k + offset) = pattern;
            }
          }
          memory += Xbyte;
        }
        if (i == 0) {
          i++;
          offset = 20;
        }
        else {
          i = 0;
          offset = 0;
        }
      }
    }   
    TRACE_OUT
}




/*---------------------------------------------------------------------------*/
          /* function to reset all parameters and lpvi device */ 
/*---------------------------------------------------------------------------*/
clean_up()
{
  func_name = "clean_up";
  TRACE_IN
  
  /* start terminating all processes created by parent, this is necessary
   * when sundiag is terminating the test abnormally.
   */
  if (pidv > 0) {
    if (kill(pidv, SIGINT) == -1) {
       send_message(0, VERBOSE, "process %d is terminated: %s\n",
                     pidv, errmsg(errno));
    }
  }
  if (pidlpv[1] > 0) {
    if (kill(pidlpv[1], SIGINT) == -1) {
       send_message(0, VERBOSE, "process %d is terminated: %s\n",
                     pidlpv[1], errmsg(errno));
    }
  } 
  if (pidlpv[2] > 0) {
    if (kill(pidlpv[2], SIGINT) == -1) {  
       send_message(0, VERBOSE, "process %d is terminated: %s\n", 
                     pidlpv[2], errmsg(errno)); 
    }
  }
  if (!imgflg)
    close(imgfd);
  close(fdv);
  TRACE_OUT
}
