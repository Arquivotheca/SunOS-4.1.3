
# ifndef lint [
static	char sccsid[] = "@(#)spiftest.c 1.1 7/30/92 Copyright 1992 Sun Microsystems,Inc.";
#endif ]

/*
 * spiftest.c -
 *
 */

#include 	<stdio.h>
#include 	<strings.h>
#include 	<ctype.h>
#include 	<sys/errno.h>
#include 	<sys/types.h>
#include	<sys/termios.h>
#include        <sys/fcntl.h>
#include	<sys/ttold.h>
#include 	<sys/file.h>
#include 	<sys/wait.h>
#include        <sys/mman.h>
#include 	<signal.h>
#include 	"sdrtns.h"	/* should always be included */
#include "../../../lib/include/libonline.h"    /* online library include */

/* 
 * SPC/S device driver include files - if SPC/S is installed,
 * they will be in /usr/sys/sbusdev, but since spiftest will be
 * released from stomper, or another server, they may not be there.
 */
#include	"stcio.h"
#include	"stcreg.h"

#include "spiftest.h"
#include "spif_dev.h"
#include "spif_msg.h"

extern int process_spif_args();
extern int routine_usage();
extern int errno;

/* SPC/S test related */
static	void	wait_pid_update();
void    enable_softcar();
void	probe_spif();
void	int_loopback();
void	data_loopback();
void	modem_loopback();
void	sigint_handler();
void	print_pp_file();
void	print_sp_file();
void 	echo_tty();
void	get_modem_bits();
void	data_timeout();
void    rw_loop();
void	flush_io();
int	get_brd_num();

#define DTR_MASK	\
(TIOCM_LE|TIOCM_RTS|TIOCM_ST|TIOCM_SR|TIOCM_CTS|TIOCM_CD|TIOCM_RI|TIOCM_DSR)

main(argc, argv)
int	argc;
char	*argv[];
{

    int i, fd;

    versionid = "%I";	/* SCCS version id */

    /* Initialize tests arguments */

    signal(SIGINT, sigint_handler);

    /* Default values */
    baud = 9600; csize = 8; sbit = 1; parity = 0; flow = 1; data = 0xaaaaaaaa;
    strcpy(parity_str, "none");
    strcpy(flow_str, "rtscts");

    for (i = 0; i < MAX_BOARDS; i++) { 
	spif[i] = OFF;
    }

    /* parallel = FALSE; */
    strcpy(test_name, "spiftest");

    /* termios values are not actually set with TCSETS yet */
    for (i = 0; i < TOTAL_IO_PORTS; i++) {
        sp_ports[i].selected = OFF;
	sp_ports[i].fd = 0;
	sp_ports[i].termios.c_iflag = BRKINT;
	sp_ports[i].termios.c_oflag = 0;
	sp_ports[i].termios.c_lflag = 0;
	sp_ports[i].termios.c_cflag = CLOCAL|CREAD|HUPCL;
    }

    /* Always start with test_init to enter sundiag environment */
    test_init(argc, argv, process_spif_args, routine_usage, test_usage_msg);
    dev_test();
    clean_up();

    /* Always end with test_end to close up sundiag environment */
    test_end();
} /* main */


/*
 * Process_spif_args -
 *
 * Processes test-specific command line aguments.
 * For termios related arguments (baud rate, character size, etc.),
 * the argument is verified to be valid only.  No actual assigment
 * to the serial port termios is done.  It is deferred until dev_test
 * to take default values into account.
 */
process_spif_args(argv, index)
char *argv[];
int  index;
{
    int match = FALSE;
    u_int i, brd, tmask, smask;

    tmask = smask = 0;

    if (!strncmp(argv[index],"D=",2)) {
	/* Get device name from sundiag - 
	 * Can be any installed board, board in slot 1-3, or
	 * or specific serial port.  If individual device is
	 * specified, seach for serial port device first, 
	 * then parallel port.
	 */
	if (!strncmp(&argv[index][2], "any",3)) {
	    probe_spif();
	    strcpy(dev_name, "any SPC/S board");
	    match = TRUE;
	} else if (!strncmp(&argv[index][2], "sb1",3)) {
	    for (i=0; i < MAX_TERMS; i++)
		sp_ports[i].selected = ON;
            spif[0] = ON;
	    strcpy(dev_name, spif_dev_name[0]);
	    match = TRUE;
	} else if (!strncmp(&argv[index][2], "sb2",3)) {
	    for (i=MAX_TERMS; i < (2*MAX_TERMS); i++)
		sp_ports[i].selected = ON;
            spif[1] = ON;
	    strcpy(dev_name, spif_dev_name[1]);
	    match = TRUE;
	} else if (!strncmp(&argv[index][2], "sb3",3)) {
	    for (i=2*MAX_TERMS; i < (3*MAX_TERMS); i++)
		sp_ports[i].selected = ON;
            spif[2] = ON;
	    strcpy(dev_name, spif_dev_name[2]);
	    match = TRUE;
	} else if (!strncmp(&argv[index][2], "sb4",3)) {
	    for (i=3*MAX_TERMS; i < TOTAL_IO_PORTS; i++)
		sp_ports[i].selected = ON;
            spif[3] = ON;
	    strcpy(dev_name, spif_dev_name[3]);
	    match = TRUE;
        } else {
	    for (i=0; (i < TOTAL_IO_PORTS) && (!match); i++) {
		if (!strcmp(sp_dev_name[i], &argv[index][2])) {
		    brd = get_brd_num(i);
		    spif[brd] = ON;
		    strcpy(dev_name, sp_dev_name[i]);
		    sp_ports[i].selected = ON;
		    match = TRUE;
		}
	    }
        }

	/* search for parallel device */
	if (!match) {
	    for (i=0; i < MAX_BOARDS; i++) {
		if (!strcmp(pp_dev_name[i], &argv[index][2])) {
		    strcpy(dev_name, &argv[index][2]);
		    match = TRUE;
		    /* parallel = TRUE; */
		    spif[i] = ON;
		}
	    }
	}

	/* still no device selected */
	if (!match) return(FALSE);

    } else if (!strncmp(argv[index],"q",1)) {
        /* Enable internal test if quick test is selected */
	testid[INTERNAL] = ON;

    } else if (!strncmp(argv[index],"T=",2)) {
	/* Get test mask from sundiag, a five-bit binary value
	 * to specify a combination of test selected.
	 *  if bit 0 = ON, internal test is selected
	 *         1 = ON, printer test is selected
	 *         2 = ON, spif 96-pin loopback
	 *         3 = ON, DB-25 loopback
	 *         4 = ON, echo tty test
	 */
	tmask = atoi(&argv[index][2]);

	if ((tmask < 1) || (tmask > 31))
	    return(FALSE);

	for (i = 0; i < MAX_TESTS; i++)
	     testid[i] = ON;	/* turn on all tests initially */
         
        for (i = 0; i < MAX_TESTS; i++) {
	     smask = 1 << i;	/* create select mask */
	     if (!(tmask & smask))
		 testid[i] = OFF;
	}

    } else if (!strncmp(argv[index],"B=",2)) {

	baud = atoi(&argv[index][2]);

	switch(baud) {

	case 110 :
	case 300 :
	case 600 :
	case 1200 :
	case 2400 :
	case 4800 :
	case 9600 :
	case 19200 :
	case 38400 :
		break;
	default :
		return(FALSE);

	}

    } else if (!strncmp(argv[index],"C=",2)) {

	csize = atoi(&argv[index][2]);

	switch (csize) {

	case 5 :
	case 6 :
	case 7 :
	case 8 :
		break;
			
	default :
		return(FALSE);
	}

    } else if (!strncmp(argv[index],"S=",2)) {

	sbit = atoi(&argv[index][2]);

	if ((sbit != 2) && (sbit != 1))
	    return(FALSE);

    } else if (!strncmp(argv[index],"P=",2)) {

	strcpy(parity_str,&argv[index][2]);

	if (!strcmp(&argv[index][2], "none"))
	    parity = 0;
	if (!strcmp(&argv[index][2], "odd"))
	    parity = 1;
	if (!strcmp(&argv[index][2], "even"))
	    parity = 2;

        switch (parity) {

	case 0 :
	case 1 :
	case 2 :
	    break;

        default :
		return(FALSE);

	}
        
    } else if (!strncmp(argv[index],"F=",2)) {

	strcpy(flow_str,&argv[index][2]);

	if (!strcmp(&argv[index][2], "xonoff"))
	    flow = 0;
	if (!strcmp(&argv[index][2], "rtscts"))
	    flow = 1;
	if (!strcmp(&argv[index][2], "both"))
	    flow = 2;

        switch (flow) {

        case 0 :
	case 1 :
        case 2 :
	    break;

        default :
		return(FALSE);

	}

    } else if (!strncmp(argv[index],"I=",2)) {

	 switch(argv[index][2]) {

	 case '5' : 
	     data = 0x55555555;
	     break;

	 case 'a' : 
	     data = 0xaaaaaaaa;
	     break;

	 case 'r' : 
	    data = random();
	    break;

	 default :
	    return(FALSE);

	 }

    } else return(FALSE);

    return(TRUE);
} /* process_spif_args */


/*
 * routine_usage() explains the meaning of each test-specific command 
 * argument.
 */
routine_usage()
{
    send_message(0, CONSOLE, routine_msg, test_name);
}


/*
 * The probing function should check that the specified device is available
 * to be tested(optional if run by Sundiag). Usually, this involves opening
 * the device file, and using an appropriate ioctl to check the status of the
 * device, and then closing the device file.
 */
void
probe_spif()
{

   struct stcregs_t *sr;
   int i, j, fd, offset, flag = FALSE;

   func_name = "probe_spif";
   TRACE_IN

   for (i = 0; i < MAX_BOARDS; i++) {

       send_message(0, TRACE, "%s: ", spif_dev_name[i]);
       if ((fd=open(spif_dev_name[i],O_RDWR)) != FAIL) {
		flag++;
		spif[i] = ON;
		for (j = i*MAX_TERMS; j < (i+1)*MAX_TERMS; j++)
		     sp_ports[j].selected = ON;

	        send_message(0, TRACE, "Board %d installed\n", i);
	        close(fd);
       }
   }

   if (!flag) {
       send_message(0, ERROR, probe_err_msg);
       exit(PROBE_ERROR);
   }

   TRACE_OUT
} /* probe_spif */


/*
 * set_termios -
 * 
 * Assign termios value for each serial port selected.
 */

void
set_termios(port)
int	port;

{
    int i, fd;

    func_name = "set_termios";
    TRACE_IN

    switch(baud) {

	case 110 :
		sp_ports[port].termios.c_cflag |= B110;
		break;
			
	case 300 :
		sp_ports[port].termios.c_cflag |= B300;
		break;
			
	case 600 :
		sp_ports[port].termios.c_cflag |= B600;
		break;
			
	case 1200 :
		sp_ports[port].termios.c_cflag |= B1200;
		break;
			
	case 2400 :
		sp_ports[port].termios.c_cflag |= B2400;
		break;
			
	case 4800 :
		sp_ports[port].termios.c_cflag |= B4800;
		break;
		
	case 9600 :
		sp_ports[port].termios.c_cflag |= B9600;
		break;
		
	case 19200 :
		sp_ports[port].termios.c_cflag |= B19200;
		break;
		
	case 38400 :
		sp_ports[port].termios.c_cflag |= B38400;
		break;
		
	default : 
		send_message(UNEXP_ERROR, ERROR, routine_msg);

    }

    switch (csize) {

	case 5 :
		sp_ports[port].termios.c_cflag |= CS5;
		break;
		
	case 6 :
		sp_ports[port].termios.c_cflag |= CS6;
		break;
		
	case 7 :
		sp_ports[port].termios.c_cflag |= CS7;
		break;
			
	case 8 :
		sp_ports[port].termios.c_cflag |= CS8;
		break;
			
	default :
		send_message(UNEXP_ERROR, ERROR, routine_msg);
    }

    if (sbit == 2)
	    sp_ports[port].termios.c_cflag |= CSTOPB;

    switch (parity) {

	case 0 :
		break;

	case 1 :
		sp_ports[port].termios.c_cflag |= PARENB | PARODD;
		break;

	case 2 :
		sp_ports[port].termios.c_cflag |= PARENB;
		break;

        default :
		send_message(UNEXP_ERROR, ERROR, routine_msg);

    }

    switch (flow) {

        case 0 :
		sp_ports[port].termios.c_iflag |= IXON | IXOFF;
		break;
   
	case 1 :
		if ((testid[SP_96]) || (testid[DB_25]))
		    sp_ports[port].termios.c_cflag |= CRTSCTS;
		break;

        case 2 :
		sp_ports[port].termios.c_iflag |= IXON | IXOFF;
		if ((testid[SP_96]) || (testid[DB_25]))
		    sp_ports[port].termios.c_cflag |= CRTSCTS;
		break;

        default :
		send_message(UNEXP_ERROR, ERROR, routine_msg);

    }

	sp_ports[port].termios.c_cc[VMIN] = 1;
	sp_ports[port].termios.c_cc[VTIME] = 0;

    if ((ioctl(sp_ports[port].fd, TCSETS,&sp_ports[port].termios)) == FAIL) {
	send_message(0, ERROR, tcsets_err, sp_dev_name[port]);
        exit(errno);
    }

    send_message(0, TRACE, "%s: ", sp_dev_name[port]);
    send_message(0, TRACE, "iflag=0x%x oflag=0x%x lflag=0x%x cflag=0x%x\n", sp_ports[port].termios.c_iflag, sp_ports[port].termios.c_oflag, sp_ports[port].termios.c_lflag, sp_ports[port].termios.c_cflag);

    TRACE_OUT
} /* set_termios */
	

/*
 * The actual test.
 * Return True if the test passed, otherwise FALSE.
 *
 */

int
dev_test()
{

    int i;

    func_name = "dev_test";
    TRACE_IN

    strcpy(device_name, dev_name);
    send_message(0, VERBOSE, start_test_msg, test_name);

    for (i = 0; i < MAX_BOARDS; i++)
	 send_message(0, TRACE, "Board[%d] = %d\n", i, spif[i]);

    for (i = 0; i < MAX_TESTS; i++)
	 send_message(0, TRACE, "Testid[%d] = %d\n", i, testid[i]);

    if (!testid[PRINT]) {
	send_message(0, VERBOSE, parms_msg, baud, csize, sbit, parity_str, flow_str);

	if (!testid[ECHO_TTY]) {
	   switch (csize) {
	      case 5: data &= 0x1f1f1f1f; break;
	      case 6: data &= 0x3f3f3f3f; break;
	      case 7: data &= 0x7f7f7f7f; break;
	   }
	   send_message(0, VERBOSE, data_msg, data);
        }
    }

    /* Execute this test first, always */
    if (testid[INTERNAL]) {
	send_message(0, VERBOSE, internal_test);
	current_test = INTERNAL;
	int_loopback();
        sleep(10);	/* ample time for internal test to complete */
    }

    if (testid[SP_96]) {
	/* 
	 * Verify that sundiag, in intervention mode, prints message 
	 * to advice user to plug the SPC/S 96-pin loopback plug into 
	 * the board.
	 */

	send_message(0, VERBOSE, lp96_msg);
	current_test = SP_96;
        data_loopback();
	modem_loopback();
    } 

    if (testid[DB_25]) {
	/* 
	 * Verify that sundiag, in intervention mode, prints message 
	 * to advice user to plug the DB-25 loopback plug into the 
	 * patch panel.
	 */

	send_message(0, VERBOSE, lp25_msg);
	current_test = DB_25;
        data_loopback();
	modem_loopback();
    } 
    
    if (testid[ECHO_TTY]) {

	send_message(0, VERBOSE, echotty_msg);
	current_test = ECHO_TTY;
	echo_tty();
    } 
    
    if (testid[PRINT]) {

        send_message(0, VERBOSE, printer_msg);
	current_test = PRINT;
	print_pp_file();
    }

    send_message(0, VERBOSE, end_test_msg, test_name);

    TRACE_OUT

} /* dev_test */


/*
 * int_loopback -
 *
 * Enable local loopback mode in the CD180 so that receive line is 
 * looped to transmit line internally.  Also, read and write to data
 * line within the PPC2 using ioctl calls provided by the
 * SPC/S device driver.
 *
 * Enable if quick test is selected or if user specifies explicitly.
 */

void
int_loopback()
{

    struct stcregs_t *sr;
    struct ppcregs_t *pr;

    int brd;
    int bfd, dfd, offset, bytes;
    u_char b_exp, b_obs;

    func_name = "int_loopback";
    TRACE_IN

    for (brd = 0; brd < MAX_BOARDS; brd++) {

	if (spif[brd]) {

	    /*
	     * Open the control device for installed SPC/S board
	     */
	    if ((bfd=open(spif_dev_name[brd],O_RDWR|O_NDELAY)) < 0) {
		send_message(0, ERROR, open_err_msg, spif_dev_name[brd]);
                exit(errno);
            }

	    strcpy(device_name, spif_dev_name[brd]);
    	    send_message(0,VERBOSE, testing_msg, device_name);

	    for (dev_num = (brd*MAX_TERMS); dev_num < (brd+1)*MAX_TERMS ; dev_num++) {


		if ((dfd=open(sp_dev_name[dev_num],O_RDWR|O_NDELAY)) < 0) {
		    send_message(0, ERROR, open_err_msg, sp_dev_name[dev_num]);
		    exit(errno);
		}

		if (ioctl(dfd, TIOCEXCL, 0) == FAIL) {
		    send_message(0, ERROR, device_open, sp_dev_name[dev_num]);
                    exit(errno);
                }

		sp_ports[dev_num].fd = dfd;
		enable_softcar(dev_num);
		set_termios(dev_num);

		/*
		 * Enable local loopback. Write 0x10 to CD180 COR2.
		 * register.  Verify that CCR is 0, then update it
		 * to inform CD180 that COR2 has changed. The channel
		 * number is local to each chip.
		 */
		offset = (caddr_t)&(sr->cor2) - (caddr_t)sr;
		sd.line_no = dev_num % MAX_TERMS;
		sd.op = STC_REGIOW | STC_SETCAR;
		sd.reg_offset = offset;
		sd.reg_data = LLOOP;
		if (ioctl(bfd,STC_DCONTROL,&sd) < 0) {
		    send_message(0, ERROR, stc_regiow_cor2, 
				sp_dev_name[dev_num]);
                    exit(errno);
                }

    send_message(0, TRACE, "%s: COR2 = 0x%x\n", sp_dev_name[dev_num], sd.reg_data);

		/* Make sure CCR is 0 per CD180 specification */
		offset = (caddr_t)&(sr->ccr) - (caddr_t)sr;
		sd.line_no = dev_num % MAX_TERMS;
		sd.op = STC_REGIOR | STC_SETCAR;
		sd.reg_offset = offset;
		if (ioctl(bfd,STC_DCONTROL,&sd) < 0) {
		    send_message(0, ERROR, stc_regior_ccr, 
				sp_dev_name[dev_num]);
                    exit(errno);
                }

		/* Write to CCR to inform COR2 has changed, per
		 * CD180 specification 
		 */
		offset = (caddr_t)&(sr->ccr) - (caddr_t)sr;
		sd.line_no = dev_num % MAX_TERMS;
		sd.op = STC_REGIOW | STC_SETCAR;
		sd.reg_offset = offset;
		sd.reg_data = COR2_CHNG;
		if (ioctl(bfd,STC_DCONTROL,&sd) < 0) {
		    send_message(0, ERROR, stc_regiow_ccr,
				sp_dev_name[dev_num]);
                    exit(errno);
                }

    send_message(0, TRACE, "%s: CCR = 0x%x\n", sp_dev_name[dev_num], sd.reg_data);

		rw_loop();
		close(dfd);
            }


	    /*
	     * Write to ppc2 data register, and then read and
	     * and compare.  There is only one parallel port per board.
	     */
	    b_exp = 0xaa;

	    for (bytes = 0; bytes < MAX_BYTES; bytes++) {

		    sd.line_no = 0;
		    sd.op = STC_PPCREGW;
		    sd.reg_offset = (caddr_t)&(pr->pdata) - (caddr_t)pr;
		    sd.reg_data = b_exp;
		    if (ioctl(bfd,STC_DCONTROL,&sd)<0) {
			send_message(0, ERROR, stc_ppcregw, 
				pp_dev_name[brd]);
                        exit(errno);
                    }

		    sd.line_no = 0;
		    sd.op = STC_PPCREGR;
		    sd.reg_offset = (caddr_t)&(pr->pdata) - (caddr_t)pr;
		    if (ioctl(bfd,STC_DCONTROL,&sd) < 0) {
			send_message(0, ERROR, stc_ppcregr, pp_dev_name[brd]);
                        exit(errno);
                    }

                    b_obs = sd.reg_data;
    send_message(0, TRACE, "%s: exp=0x%x obs=0x%x\n", pp_dev_name[brd], b_exp, b_obs);

		    if (b_obs != b_exp) {
			send_message(0, ERROR, compare_err_msg, b_exp, b_obs);
			send_message(0, ERROR, internal_test_err, pp_dev_name[brd]);
                        exit(CMP_ERROR);
		    }
	    }

	    close(bfd);
        }
    }
  
    TRACE_OUT
} /* int_loopback */


/*
 * data_loopback -
 *
 * Send data and read back from same port with the help of 96-pin
 * and 25-pin loopback plug.  Data is sent to selected port only.
 */

void
data_loopback()
{
    int i, fd;

    func_name = "data_loopback";
    TRACE_IN

    send_message(0, TRACE, dataloop_test);

    for (dev_num = 0; dev_num < TOTAL_IO_PORTS; dev_num++) {
        if (sp_ports[dev_num].selected) {

	    strcpy(device_name, sp_dev_name[dev_num]);
    	    send_message(0,VERBOSE, testing_msg, device_name);

	    if ((fd=open(sp_dev_name[dev_num],O_RDWR|O_NDELAY)) < 0) {
		send_message(0, ERROR, open_err_msg, sp_dev_name[dev_num]);
	        exit(errno);
	    }

	    if (ioctl(fd, TIOCEXCL, 0) == FAIL) {
		send_message(0, ERROR, device_open, sp_dev_name[dev_num]);
                exit(errno);
            }

	    sp_ports[dev_num].fd = fd;
	    enable_softcar(dev_num);
	    set_termios(dev_num);

	    rw_loop();

	    close(fd);
        }
    }
    
    TRACE_OUT

} /* data_loopback */


/*
 * modem_loopback -
 *
 * Since the modem lines DTR is loopback to CD and DSR (in DB-25 
 * plug), and RTS to CTS, we use ioctl TIOCMSET to set DTR and RTS.  
 * Use TIOCMBIC to clear DTR and RTS, then verify that they are 
 * cleared.
 * 
 * For 96-pin loopback, DSRx lines are connected to parallel port
 * lines, so we have to set and clear the Pdx lines.
 */

void
modem_loopback()
{
    struct ppcregs_t *pr;
    int bfd, dfd;
    int modem = 0, exp = 0, obs = 0, tmp = 0;
    int offset, brd;
    u_char pattern = 0, dsr_obs = 0;

    func_name = "modem_loopback";
    TRACE_IN

    /* 
     * Perform parallel port loopback test -
     * If 96-pin loopback, enable DSRx by writing to
     * parallel port data register.  Find out which
     * board is enable to open control line for
     * that board.
     */

    if (testid[SP_96]) {

	send_message(0, TRACE, pploop_test);

	for (brd = 0; brd < MAX_BOARDS; brd++) {
	    if (spif[brd]) {

		if ((bfd=open(spif_dev_name[brd],O_RDWR|O_NDELAY)) == FAIL) {
		    send_message(0, ERROR, open_err_msg, spif_dev_name[brd]);
                    exit(errno);
                }

		pattern = 0xff;
		offset = (caddr_t)&(pr->pdata) - (caddr_t)pr;
		sd.line_no = 0;
		sd.op = STC_PPCREGW;
		sd.reg_offset = offset;
		sd.reg_data = pattern;
		if (ioctl(bfd,STC_DCONTROL,&sd) == FAIL) {
		    send_message(0, ERROR, stc_ppcregw, pp_dev_name[brd]);
                    exit(errno);
                }

	        sd.line_no = 0;
		sd.op = STC_PPCREGR;
		sd.reg_offset = (caddr_t)&(pr->pdata) - (caddr_t)pr;
		if (ioctl(bfd,STC_DCONTROL,&sd) < 0) {
		    send_message(0, ERROR, stc_ppcregr, pp_dev_name[brd]);
                    exit(errno);
                }

		send_message(0, TRACE, "STC_PPCREGW(DATA): offset 0x%x, data 0x%x\n", sd.reg_offset, sd.reg_data);

		/* Get current modem state of each enable port */

		dsr_obs = 0;
	        for (dev_num = (brd*MAX_TERMS); dev_num < (brd+1)*MAX_TERMS ; dev_num++) {

		    if (sp_ports[dev_num].selected) {

			if ((dfd = open(sp_dev_name[dev_num], O_RDWR|O_NDELAY)) < 0) {
			    send_message(0, ERROR, open_err_msg,
				sp_dev_name[dev_num]);
                            exit(errno);
			}

			if (ioctl(dfd, TIOCEXCL, 0) == FAIL) {
			    send_message(0, ERROR, device_open, 
					sp_dev_name[dev_num]);
	                    exit(errno);
			}

 			sp_ports[dev_num].fd = dfd;
			enable_softcar(dev_num);

			if ((ioctl(dfd,TIOCMGET,&obs)) == FAIL) {
			    send_message(0, ERROR, tiocmget_err, sp_dev_name[dev_num]);
                            exit(errno);
                        }

			send_message(0, TRACE, "obs = 0x%x\n", obs);

			if (!(obs & TIOCM_DSR)) {
			    send_message(0, ERROR, dsr_clear_err, sp_dev_name[dev_num]);
                            exit(CMP_ERROR);
                        }

		    }
		}

		/* Now clear the parallel port register */
		pattern = 0x0;
		offset = (caddr_t)&(pr->pdata) - (caddr_t)pr;
		sd.line_no = 0;
		sd.op = STC_PPCREGW;
		sd.reg_offset = offset;
		sd.reg_data = pattern;
		if (ioctl(bfd,STC_DCONTROL,&sd) == FAIL) {
		    send_message(0, ERROR, stc_ppcregw, pp_dev_name[brd]);
                    exit(errno);
                }

	        sd.line_no = 0;
		sd.op = STC_PPCREGR;
		sd.reg_offset = (caddr_t)&(pr->pdata) - (caddr_t)pr;
		if (ioctl(bfd,STC_DCONTROL,&sd) < 0) {
		    send_message(0, ERROR, stc_ppcregr, pp_dev_name[brd]);
                    exit(errno);
                }
		send_message(0, TRACE, "STC_PPCREGW(DATA): offset 0x%x, data 0x%x\n", sd.reg_offset, sd.reg_data);

		/* Verify that DSRx also are cleared */
	        for (dev_num = (brd*MAX_TERMS); dev_num < (brd+1)*MAX_TERMS ; dev_num++) {
		    if (sp_ports[dev_num].selected) {
			if ((ioctl(sp_ports[dev_num].fd,TIOCMGET,&obs)) == FAIL) {
			    send_message(0, ERROR, tiocmget_err, sp_dev_name[dev_num]);
                            exit(errno);
                        }

			send_message(0, TRACE, "obs = 0x%x\n", obs);

			if (obs & TIOCM_DSR) {
			    send_message(0, ERROR, dsr_set_err, sp_dev_name[dev_num]);
                            exit(errno);
                        }

			close(sp_ports[dev_num].fd);
		    }
		}
		close(bfd);
	    }
	}
    }

    /* Begin modem loopback test on modem signals */

    send_message(0, TRACE, modemloop_test);

    for (dev_num = 0; dev_num < TOTAL_IO_PORTS; dev_num++) {
	if (sp_ports[dev_num].selected) {

	    if ((dfd = open(sp_dev_name[dev_num], O_RDWR|O_NDELAY)) < 0) {
		send_message(0, ERROR, open_err_msg, sp_dev_name[dev_num]);
                exit(errno);
	    }

	    if (ioctl(dfd, TIOCEXCL, 0) == FAIL) {
		send_message(0, ERROR, device_open, sp_dev_name[dev_num]);
                exit(errno);
	    }

	    sp_ports[dev_num].fd = dfd;
	    enable_softcar(dev_num);

	    if ((ioctl(dfd,TIOCMGET,&tmp)) == FAIL) {
		send_message(0, ERROR, tiocmget_err, sp_dev_name[dev_num]);
                exit(errno);
            }

	    /* Clear modem states, leave line enable */
	    tmp = TIOCM_LE;
	    if ((ioctl(dfd,TIOCMSET,&tmp)) == FAIL) {
		send_message(0, ERROR, tiocmset_err, sp_dev_name[dev_num]);
                exit(errno);
            }

	    if ((ioctl(dfd,TIOCMGET,&tmp)) == FAIL) {
		send_message(0, ERROR, tiocmget_err, sp_dev_name[dev_num]);
                exit(errno);
            }

	    /* set DTR and RTS */
	    modem = TIOCM_DTR|TIOCM_RTS|TIOCM_LE;
	    if ((ioctl(dfd,TIOCMSET,&modem)) == FAIL) {
		send_message(0, ERROR, tiocmset_err, sp_dev_name[dev_num]);
                exit(errno);
            }

	    if ((ioctl(dfd,TIOCMGET,&obs)) == FAIL) {
		send_message(0, ERROR, tiocmget_err, sp_dev_name[dev_num]);
                exit(errno);
            }

	    /* Masked out DTR line since spif driver v1.2 returned its
	       value and older driver doesn't */
	    obs &= DTR_MASK;

	    /* now compare */
	    if (testid[SP_96]) {
		exp = TIOCM_LE | TIOCM_CTS | TIOCM_CAR;
            } else
		exp = TIOCM_LE | TIOCM_CTS | TIOCM_CAR | TIOCM_DSR;

	    if (exp != obs) {
		send_message(0, ERROR, "Expected (0x%x): ", exp);
		get_modem_bits(exp);
		send_message(0, ERROR, "Observed (0x%x): ", obs);
		get_modem_bits(obs);
		send_message(0, ERROR, modemloop_test_err, sp_dev_name[dev_num]);
                exit(CMP_ERROR);
            }

	    /* Now clear DTR and RTS */
	    exp = TIOCM_LE;

	    modem = TIOCM_DTR|TIOCM_RTS;
	    if ((ioctl(dfd,TIOCMBIC,&modem)) == FAIL) {
		send_message(0, ERROR, tiocmbic_err, sp_dev_name[dev_num]);
                exit(errno);
            }

	    if ((ioctl(dfd,TIOCMGET,&obs)) == FAIL) {
		send_message(0, ERROR, tiocmget_err, sp_dev_name[dev_num]);
                exit(errno);
            }

	    if (exp != obs) {
		send_message(0, ERROR, "Expected (0x%x): ", exp);
		get_modem_bits(exp);
		send_message(0, ERROR, "Observed (0x%x): ", obs);
		get_modem_bits(obs);
		send_message(0, ERROR, modemloop_test_err, sp_dev_name[dev_num]);
                exit(CMP_ERROR);
            }

	    close(dfd);
        }
    }

    TRACE_OUT
} /* modem_loopback */


/* 
 * echo_tty -
 * 
 * Continually read and write to device selected (with a tty connected) 
 * until ETX character is received or when the timeout (two minutes)
 * is reached.  Note that VMIN is set to one character and VTIME is 0.
 * No flow control is necessary for this test.
 */
void
echo_tty()
{
    int fd;
    int done = 0, rl = 0, wl = 0;
    u_char ch;

    func_name = "echo_tty";
    TRACE_IN

    alarm(120);
    signal(SIGALRM, data_timeout);

    for (dev_num = 0; dev_num < TOTAL_IO_PORTS; dev_num++) {

	if (sp_ports[dev_num].selected) {

	    strcpy(device_name, sp_dev_name[dev_num]);
    	    send_message(0,VERBOSE, testing_msg, device_name);

	    if ((fd = open(sp_dev_name[dev_num], O_RDWR)) == FAIL) {
		send_message(0, ERROR, open_err_msg, sp_dev_name[dev_num]);
                exit(errno);
	    }

	    if (ioctl(fd, TIOCEXCL, 0) == FAIL) {
		send_message(0, ERROR, device_open, sp_dev_name[dev_num]);
                exit(errno);
	    }

	    sp_ports[dev_num].fd = fd;
	    enable_softcar(dev_num);
	    set_termios(dev_num);

	    /* Open the serial port and continually reading from it 
	     * Echo mode will echo the character to TTY terminal.
	     */
            while (!done) {
		if ((rl = read(fd, &ch, 1)) == FAIL) {
		    send_message(0, ERROR, read_err_msg, sp_dev_name[dev_num]);
                    exit(errno);
                }

		if (ch == ETX) {
		    send_message(0, VERBOSE, end_echotty);
                    done++;
                } else {
		    if ((wl = write(fd, &ch, rl)) == FAIL) {
			send_message(0, ERROR, write_err_msg, sp_dev_name[dev_num]);
                        exit(errno);
                    }

			send_message(0, TRACE, "ch = 0x%x [%c]\n",ch&0x0ff,ch); }
	    }

	    alarm(0);

	    flush_io(dev_num);
	    close(sp_ports[dev_num].fd);
        }
    }

    TRACE_OUT
} /* echo_tty */


/* 
 * print_pp_file -
 *
 * Check the printer state and print out appropriate messages.
 * Flush the printer.
 *
 * Print a ascii file to the parallel port.  Do it the simpliest way 
 * without having to count the characters or do many writes.  Write
 * new line at beginning and new page at the end of print.  No need
 * to set termios for the printer.
 *
 */

void
print_pp_file()
{


    int fd;
    int lc, cc, wl;
    int brd;
    u_char u_ascii[49];
    u_char l_ascii[49];
    u_char np = '\f';

    func_name = "print_pp_file";
    TRACE_IN

    strcpy(u_ascii, "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNO\n\r");
    strcpy(l_ascii, "PQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\n\r");

    for (brd = 0; brd < MAX_BOARDS; brd++) {

	if (spif[brd] == ON) {

	    strcpy(device_name, pp_dev_name[brd]);
    	    send_message(0,VERBOSE, testing_msg, device_name);

	    if ((fd = open(pp_dev_name[brd], O_RDWR)) < 0) {
		send_message(0, ERROR, open_err_msg, pp_dev_name[brd]);
                exit(errno);
            }

	    if (ioctl(fd, TIOCEXCL, 0) == FAIL) {
		send_message(0, ERROR, device_open, pp_dev_name[brd]);
                exit(errno);
            }

	    /* Get status of the printer (actual status, not what driver think it is)
             * for now terminate when error occurs.
             */

	    if (ioctl(fd, STC_GPPC, &pd) == FAIL) {
		send_message(0, ERROR, stc_gppc, pp_dev_name[brd]);
                exit(errno);
            }

            if (!(pd.state & (PP_SELECT<<PP_SHIFT))) {
		send_message(0, ERROR, offline_err, pp_dev_name[brd]);
                exit(errno);
            }

            if (pd.state & (PP_PAPER_OUT<<PP_SHIFT)) {
		send_message(0, ERROR, paper_out_err, pp_dev_name[brd]);
                exit(errno);
            }

            if (pd.state & (PP_BUSY<<PP_SHIFT)) {
		send_message(0, ERROR, busy_err, pp_dev_name[brd]);
                exit(errno);
            }

            if (pd.state & (PP_ERROR<<PP_SHIFT)) {
		send_message(0, ERROR, pp_err, pp_dev_name[brd]);
                exit(errno);
            }

	    /* If the printer is ready, start sending data, check 
	     * for end of line and end of page.
	     */
	    for (lc = 0; lc < PSIZE; lc++) {
		if ((wl = write(fd, u_ascii, strlen(u_ascii))) != strlen(u_ascii)) {
		    send_message(0, ERROR, write_err_msg, pp_dev_name[brd]);
                    exit(errno);
                }

		if ((wl = write(fd, l_ascii, strlen(l_ascii))) != strlen(l_ascii)) {
		    send_message(0, ERROR, write_err_msg, pp_dev_name[brd]);
                    exit(errno);
                }

		send_message(0, TRACE, "PRINT, %s: upper ascii=%s, len = %d\n", pp_dev_name[brd], u_ascii, strlen(u_ascii));
		send_message(0, TRACE, "PRINT, %s: lower ascii=%s, len = %d\n", pp_dev_name[brd], l_ascii, strlen(l_ascii));
	    }

	    if ((wl = write(fd, &np, 1)) == FAIL) {
		send_message(0, ERROR, write_err_msg, pp_dev_name[brd]);
                exit(errno);
	    }
	    close(fd);
	    flush_io(brd|PP_LINE);
        }
    }

    TRACE_OUT
} /* print_pp_file */


static void
wait_pid_update(pid)
int	pid;
{
    int	i;

    for (i = 0; i < TOTAL_IO_PORTS; i++) {
	if (rw_child_pid[i].rpid == pid) {
	    rw_child_pid[i].rstatus = PASS;
	} else if (rw_child_pid[i].wpid == pid) {
	    rw_child_pid[i].wstatus = PASS;
	}
    }
}


void
rw_loop()
{
    int fd;
    int i, bytes, rc = 0, wc = 0, acc = 0;
    u_long *exp, *obs;
    char tina[4];

    func_name = "rw_loop";
    TRACE_IN

    alarm(TIMEOUT);
    signal(SIGALRM, data_timeout);

    if (!testid[INTERNAL])
       send_message(0, VERBOSE, test_name, dataloop_test);

    /* Now perform local loopback */
    fd = sp_ports[dev_num].fd;
    exp = (u_long *) malloc(sizeof(long));
    obs = (u_long *) malloc(sizeof(long));
    *exp = data;
    *obs = 0;
    send_message(0, TRACE, "INITIALLY, exp = 0x%x obs = 0x%x\n", *exp, *obs);

    for (bytes = 0; bytes < MAX_RUNS; bytes++) {

	if ((wc = write(fd, exp, sizeof(long))) < 0) {
	    send_message(0, ERROR, write_err_msg, sp_dev_name[dev_num]);
            exit(errno);
        }

	acc = 0;
	for (i = 0; (i < RETRIES) && (rc != sizeof(long)); i++) {
	    alarm(TIMEOUT);
	    if ((acc = read(fd, &tina[rc], sizeof(long)-rc)) < 0) {
		send_message(0, ERROR, read_err_msg, sp_dev_name[dev_num]);
                exit(errno);
	    } else
		rc += acc;
	}

	bcopy(tina, obs, 4);

	/* Adjust expected value depening on character size */
	switch (csize) {
	   case 5: *exp &= 0x1f1f1f1f; break;
	   case 6: *exp &= 0x3f3f3f3f; break;
	   case 7: *exp &= 0x7f7f7f7f; break;
	}

	send_message(0, TRACE, "%s: exp=0x%x obs=0x%x\n", sp_dev_name[dev_num], *exp, *obs);

	if (wc != rc) {
	    send_message(0, ERROR, mismatch_err_msg, wc, rc);
	    if (testid[INTERNAL]) {
		send_message(0, ERROR, internal_test_err, sp_dev_name[dev_num]);
                exit(CMP_ERROR);
            } else {
		send_message(0, ERROR, data_test_err, sp_dev_name[dev_num]);
                exit(CMP_ERROR);
            }
	}

	if (*exp != *obs) {
	    send_message(0, ERROR, compare_err_msg, *exp, *obs);
	    send_message(0, ERROR, internal_test_err, sp_dev_name[dev_num]);
            exit(CMP_ERROR);
        }

    }

    alarm(0);
    flush_io(dev_num);

    TRACE_OUT 
} /* rw_loop */


/*
 * get_brd_num -
 *
 * Get the board number from the serial port device number.
 * This is necessary for ioctl that requires to open the board
 * device.
 */
 int
 get_brd_num(port)
 int port;
 {
    return(port/MAX_TERMS);
 }


/* 
 * enable_softcar -
 *
 * Enable soft carrier detect so if driver sees CD
 * in transition, won't close port.
 */
void 
enable_softcar(port)
int port;
{

    func_name = "enable_softcar";
    TRACE_IN

    if (ioctl(sp_ports[port].fd, STC_GDEFAULTS, &sd) == FAIL) {
	send_message(0, ERROR, stc_gdefaults, sp_dev_name[port]);
	exit(errno);
    }

    send_message(0, TRACE, "before, sd.flags on %s = 0x%x\n", sp_dev_name[port],sd.flags);

    sd.flags |= SOFT_CARR;
    if (ioctl(sp_ports[port].fd, STC_SDEFAULTS, &sd) == FAIL) {
	send_message(0, ERROR, stc_sdefaults, sp_dev_name[port]);    
	exit(errno);
    }

    send_message(0, TRACE, "after, sd.flags on %s = 0x%x\n", sp_dev_name[port],sd.flags);

    TRACE_OUT
}


/* 
 * flush_io -
 *
 * Flush the serial/parallel port.  Have to use SPC/S driver specific
 * ioctl for this instead of TCFLSH because there is a bug in the 
 * sunos 4.1 (so I was told).  To flush the printer port, the line
 * number used is the printer port number ored with 64.  For serial
 * port, make sure the the port number is relative to each SPC/S  
 * board (0-7).
 */
void
flush_io(port)
int port;
{

    int fd, brd;

    func_name = "flush_io";
    TRACE_IN

    if (port < PP_LINE) {
	brd = get_brd_num(port);
    } else {
	brd = port ^ PP_LINE;
    }

    if ((fd=open(spif_dev_name[brd],O_RDWR|O_NDELAY)) != FAIL) {
	if (port < PP_LINE)	/* serial port */
	    sd.line_no = port % MAX_TERMS;
        else 			/* parallel printer port */
	    sd.line_no = port;
	sd.op = STC_CFLUSH;

	if (ioctl(fd, STC_DCONTROL, &sd) == FAIL) {
	    send_message(0, ERROR, stc_dcontrol, dev_name);
            exit(errno);
	} else 
	    send_message(0, TRACE, "Ioctl STC_CFLUSH on %s done\n", dev_name);
    } else {
	send_message(0, ERROR, open_err_msg, dev_name);
        exit(errno);
    }

    TRACE_OUT
}


void
sigint_handler()
{
    int	pgrpid;

    send_message(0, ERROR, "Interrupted!\n");
    if (kill(0,SIGTERM) != 0) {
	send_message(0, ERROR, "Can't kill all processes\n");
        exit(errno);
    }
}


void
get_modem_bits(bits)
 int bits;
{
	char msignal[256];

	if (bits & TIOCM_LE)
		strcpy(msignal, "TIOCM_LE ");
	if (bits & TIOCM_DTR)
		strcat(msignal, "TIOCM_DTR ");
	if (bits & TIOCM_RTS)
		strcat(msignal, "TIOCM_RTS ");
	if (bits & TIOCM_ST)
		strcat(msignal, "TIOCM_ST ");
	if (bits & TIOCM_SR)
		strcat(msignal, "TIOCM_SR ");
	if (bits & TIOCM_CTS)
		strcat(msignal, "TIOCM_CTS ");
	if (bits & TIOCM_CAR)
		strcat(msignal, "TIOCM_CAR ");
	if (bits & TIOCM_RI)
		strcat(msignal, "TIOCM_RI ");
	if (bits & TIOCM_DSR)
		strcat(msignal, "TIOCM_DSR ");
	send_message(0, ERROR, msignal);
}


static void
data_timeout()
{ 
	if (current_test == DB_25 || current_test == SP_96)
	   send_message(0, ERROR, lp_err_msg, sp_dev_name[dev_num]);
	if (current_test == ECHO_TTY)
	   send_message(0, ERROR, tty_err_msg, sp_dev_name[dev_num]);
	if (current_test == INTERNAL)
	   send_message(0, ERROR, timeout_err, sp_dev_name[dev_num]);
        exit(TIMEOUT_ERROR);
}


/*
 * clean_up(), contains necessary code to clean up resources before exiting.
 * Note: this function is always required in order to link with sdrtns.c
 * successfully.
 */
clean_up()
{
  int i;

  func_name = "clean_up";
  TRACE_IN

  TRACE_OUT
  return(0);
}
