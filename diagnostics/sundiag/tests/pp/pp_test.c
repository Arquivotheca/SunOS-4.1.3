#ifndef lint
static  char sccsid[] = "@(#)pp_test.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include "pp_test_msg.h" 
#include "pp_test.h" 
#include "ppreg.h"
#include "sdrtns.h"  /* sdrtns.h should always be included */
#include "../../../lib/include/libonline.h"	/* online library include */
#include <sys/fcntl.h>
#include <sys/ioctl.h>

/************************************************************************
*                                                                       *
*  main() - Entry point of pp_test.c.                                   *
*                                                                       *
*       Imports:  <none>                                                *
*       Exports:                                                        *
*                                                                       *
************************************************************************/

char   msgbuf [MESSAGE_SIZE];
static int      dummy(){return FALSE;}

main(argc, argv)
int	argc;
char	*argv[];
{
    int	fd, pass = 1;
    
    versionid = "1.1";
    					/* begin Sundiag test */
    test_init(argc, argv, dummy, dummy, (char *)NULL); 
    device_name = DEVICE_NAME;

    if ((fd = open("/dev/ppdiag0",O_RDWR | O_NDELAY)) == -1)  
        send_message(-OPEN_ERROR, ERROR, no_open_pp);
 
    while (pass <= Default_pass) 
	pp_ctrl_test(fd, pass++);	/* run tests */
 
    close (fd);
    test_end();				/* Sundiag normal exit */
}

/************************************************************************
*                                                                       *
*  pp_ctrl_test() - Tests Paradise Systems PPC1 Parallel Port control   *
*       and status lines. ** EXTERNAL Loopback Connector REQUIRED. **   *
*                                                                       *
*            This is an EXTERNAL Loopback test that verifies that the   *
*       controller's status and control lines are functional.  The test *
*       does require that a Parallel Port External Loopback connector   *
*       be attacted. (Now in intervention mode if run ON-LINE.)	        *
*                                                                       *
*       Imports: <none>                                                 *
*                                                                       *
************************************************************************/
int	pp_ctrl_test(prt_fd, pass)	
    int  prt_fd, pass;
{
    unsigned char   obs, save_ctrl;

    send_message (0, VERBOSE, "Test = %s, Device = %s, Pass = %d.\n",
                      test_name, device_name, pass);

    if (ioctl(prt_fd, PPIOCGETC, &save_ctrl) == -1)   /* save Control reg */
        send_message(NO_WRITE, ERROR, no_save_ctl_reg);

    control_reg_test(prt_fd);
    external_test(prt_fd);

    if (ioctl(prt_fd, PPIOCSETC, &save_ctrl) == -1)   /* restore Control reg */
        send_message(NO_WRITE, ERROR, no_rest_ctl_reg);
}

/************************************************************************
*                                                                       *
*  control_reg_test()    Verify ability to read and write from Parallel *
*       Port's Control register.  Note that the upper 3 bits (7-5) of   *
*       the PPC1 are unused and are always set to 1.  Therefore, this   *
*       test ignores testing of those bits.                             *
*                                                                       *
*       Imports:                                                        *
*          int file_descrip:  parallel port device file descriptor.     *
*       Exports:                                                        *
*                                                                       *
************************************************************************/
control_reg_test(file_descrip)
    int  file_descrip;
{
    unsigned char  pattern, obs;

    for (pattern = 0;pattern <= 0x1f; pattern++) {
        if (ioctl(file_descrip, PPIOCSETC, &pattern) == -1) 
            send_message(NO_WRITE, ERROR, no_wr_ctl_reg);

    	if (ioctl(file_descrip, PPIOCGETC, &obs) == -1) 
            send_message(NO_READ, ERROR, no_rd_ctl_reg);

	obs &= 0x1f;
    	if (pattern != obs)
            send_message(TEST_ERR, ERROR, test_err_msg);
   }
}

/************************************************************************
*                                                                       *
*  external_test()    Verify Parallel Port's (PC1) ability to read and  *
*       write control information to and from the chip.  This is an     *
*       external test, as it writes data to the control lines and       *
*       expects to receive the data on the status lines.  This tests    *
*       verifies not only the Control and Status registers, but also    *
*       the external circuitry required to handshake with an external   *
*       peripheral.                                                     *
*                                                                       *
*           Parallel Port External Loopback Connector REQUIRED !!       *
*                                                                       *
*       Imports:                                                        *
*          int file_descrip:  parallel port device file descriptor.     *
*       Exports:                                                        *
*                                                                       *
************************************************************************/
external_test(file_descrip)
    int  file_descrip;
{
    unsigned char  pattern, obs;
    int     Data_Bit;

    for (Data_Bit = 0; Data_Bit <= 1; Data_Bit++) {
        for (pattern = 0; pattern <= 0x0f; pattern++) {
            if (ioctl(file_descrip, PPIOCSETC, &pattern) == -1) 
                send_message(NO_WRITE, ERROR, no_wr_ctl_reg);

    	    if (ioctl(file_descrip, PPIOCGETS, &obs) == -1) 
                send_message(NO_READ, ERROR, no_rd_ctl_reg);

	    flip_stat_data (&obs);

    	    if (pattern != obs)
                send_message(TEST_ERR, ERROR, test_err_msg);
       }
   }
}

/************************************************************************
*                                                                       *
*  flip_stat_data() - line up data bits in Status Reg to correspond to  *
*       bits in Control Reg.                                            *
*                                                                       *
************************************************************************/
flip_stat_data (data)
   unsigned char  *data;
{
   unsigned char   temp;

   *data = (~(*data >> 3) & 0x0f);
   temp = *data & ININ_mask;
   if (temp)
       *data &= ININ_zero;
   else 
       *data |= ININ_one;
}

clean_up()
{
}

