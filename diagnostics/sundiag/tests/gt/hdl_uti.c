
#ifndef lint
static  char sccsid[] = "@(#)hdl_uti.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sbusdev/gtreg.h>
#include <varargs.h>
#include <pixrect/pixrect_hs.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gtmcb.h>
#include <hk_public.h>
#include <hk_comm.h>
#include <gttest.h>
#include <errmsg.h>

#undef DEBUG

/* Some predefined constants */
#define DISPLAY_X	0		/* display x start */
#define DISPLAY_Y	0		/* display y start */
#define DISPLAY_WIDTH	1024		/* display width */
#define DISPLAY_HEIGHT	1024		/* display height */
#define MAX_VM_SIZE     0x13b000        /* Max size for DL + CTX + Stack */
#define SCRATCH_BUFFER_SIZE	256	/* Size of scratch buffer */
#define BG_R            0.0            /* Default background color */
#define BG_G            0.0
#define BG_B            0.0
#define DEF_CLUT        	HK_DEFAULT_CLUT  /* Default CLUT */
#define FE_DELAY		12000    /* wait for FE 30 secs */

extern
int		sys_nerr;

extern
int		errno;

extern
char		*sys_errlist[];

int	fastclear_set = HK_FCS_NONE;
 
/* Static variables */
 
static Pixrect *pr;			/* Pixrect pointer to be used throughout this program */
static wid;				/* Unique WID for HDL files */
static unsigned *base_vm;               /* Pointer to VM */
static unsigned *vm;                    /* Pointer to next available VM */
static unsigned vm_size;                /* Size of virtual memory (in bytes) */
static int verbose = FALSE;             /* Indicates verbose messages */
static int finished = 0;		/* Display list completion */
static int hawk_opened = 0;		/* TRUE if hawk device already opened */
 
/* Pointers to Hawk flag words */
static unsigned *vcom_host_s;           /* Flag: Host to FE (signal handler) */
static unsigned *vcom_host_u;           /* Flag: Host to FE (user) */
static unsigned *vcom_hawk_u;           /* Flag: FE to host (user) */
 
/* Pointer to communication message control block */
static Hkvc_umcb *user_mcb_ptr;          /* user */
 
/* User context and display list data */
static Hk_context *dl_ctx; /* Ptr to test display list ctx */
static unsigned *fcbg; /* Pointer to fastclear color data in the
			    display list to fast clear the screen */
static unsigned *imgbuf; /* Pointer to the image buffer in the
			    display list to fast clear the screen */
static unsigned *ovlbuf; /* Pointer to the overlay buffer in the
			    display list to fast clear the screen */
static unsigned *fcs_fcbg; /* Pointer to the fast clear set # in the 
				display list to fast clear the screen */
static unsigned *fcs_wlut; /* Pointer to the fast clear set # in the
				display list to fast clear the screen */
static unsigned *wid_entry; /* Pointer to the wid entry in the display
				list to fast clear the screen */
static Hk_window_boundary *window_boundary;    /* Window boundary */
static unsigned *clearscreen_dl;	/* clear screen display list ptr */

/* error message */
static
char errtxt[256];
 
/* status at the time of interrupt */
static
unsigned	status;

/* dpc at the time of interrupt */
static
unsigned	dpc;

/* error code at the time of interrupt */
static
int		error = 0;

static
int		screen_x = DISPLAY_X;
static
int		screen_y = DISPLAY_Y;
static
int		screen_width = DISPLAY_WIDTH;
static
int		screen_height = DISPLAY_HEIGHT;

static
int		delay;

static
int		fbmode = -1;

static
unsigned int	fb_regs[32];

static
int		fb_saved=0;

static
int		dev_fd = -1;


/**********************************************************************/
static void
user_signal_handler()
/**********************************************************************/
/* Interupt handler for display list. Handles only TRAP 0 for the
   time being. */

{

    char *hk_error_string();
    char *diag_escape_error_string();

    int	save_cmd; 
    unsigned inst_err = 0, exp_inst, *iptr;

    func_name = "user_signal_handler";
    TRACE_IN

    status = user_mcb_ptr->status;
    dpc = user_mcb_ptr->dpc;
    iptr = (unsigned *) dpc;
    error = user_mcb_ptr->errorcode;

#ifdef DEBUG
    printf("*** BEFORE error ***\n");
    printf("user mcb: gt_stopped=0x%x status=0x%x, dpc=0x%x, errorcode=0x%x, command=0x%x\n", user_mcb_ptr->gt_stopped, status, dpc, error, user_mcb_ptr->command);
    printf("trap_instruction = 0x%x host instruction = 0x%x gt_flags=0x%x instruction_count=0x%x\n", user_mcb_ptr->trap_instruction, *iptr, user_mcb_ptr->gt_flags, user_mcb_ptr->instruction_count);
#endif DEBUG

    if (status & HKUVS_ERROR_CONDITION) {
	fb_send_message(SKIP_ERROR, ERROR, DLXERR_ERROR_CONDITION, error, (error < 0) ? diag_escape_error_string(error) : hk_error_string(error));
	inst_err = 1;

#ifdef DEBUG
    printf("*** AFTER error ***\n");
    printf("user mcb: gt_stopped=0x%x status=0x%x, dpc=0x%x, errorcode=0x%x, command=0x%x\n", user_mcb_ptr->gt_stopped, status, dpc, error, user_mcb_ptr->command);
    printf("trap_instruction = 0x%x host instruction = 0x%x gt_flags=0x%x instruction_count=0x%x\n", user_mcb_ptr->trap_instruction, *iptr, user_mcb_ptr->gt_flags, user_mcb_ptr->instruction_count);
#endif DEBUG

	/* verify dpc is within valid context range */
	if ((iptr >= base_vm) & (iptr <= (base_vm + (vm_size/sizeof(unsigned))))) {
		exp_inst = *iptr;
		fb_send_message(SKIP_ERROR, ERROR, DLXERR_ERR_INSTRUCTION,
			 dpc, exp_inst, user_mcb_ptr->trap_instruction);
	} else 
		fb_send_message(SKIP_ERROR, ERROR, DLXERR_DPC_OUT_OF_RANGE, dpc);

    } else if (status & HKUVS_INSTRUCTION_TRAP) {
        if (user_mcb_ptr->trap_instruction != (HK_OP_TRAP << HK_OP_POS)) {
	    fb_send_message(SKIP_ERROR, ERROR, DLXERR_UNKNOWN_TRAP_INSTRUCTION, user_mcb_ptr->trap_instruction, (HK_OP_TRAP << HK_OP_POS));
	}
    } else {
	fb_send_message(SKIP_ERROR, ERROR, DLXERR_NOT_TRAP_INSTRUCTION, error, (error < 0) ? diag_escape_error_string(error) : hk_error_string(error));
    }

    /* Flush everything to allow access to context and frame buffer */
    save_cmd = user_mcb_ptr->command;

    user_mcb_ptr->command = HKUVC_FLUSH_RENDERING_PIPE
			    | HKUVC_FLUSH_FULL_CONTEXT
			    | HKUVC_PAUSE_WITHOUT_INTERRUPT;
    *vcom_host_s = 1;       /* Tell Front End to do it */

#ifdef DEBUG
    printf("*** FLUSH in user_signal_handler() ***\n");
    printf("user mcb: gt_stopped=0x%x status=0x%x, dpc=0x%x, errorcode=0x%x, command=0x%x\n", user_mcb_ptr->gt_stopped, status, dpc, error, user_mcb_ptr->command);
    printf("trap_instruction = 0x%x gt_flags=0x%x instruction_count=0x%x\n", user_mcb_ptr->trap_instruction, user_mcb_ptr->gt_flags, user_mcb_ptr->instruction_count);
#endif DEBUG

    delay = FE_DELAY;
    while (*vcom_host_s && (delay-- >= 0)) usleep(10);   /* Wait for completion */
    if (delay < 0) {
	dump_status();
	fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
    }

    user_mcb_ptr->command = save_cmd;

    if (inst_err) {
	/* stop if FE detects an error condition */
	dump_status();
        fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_USER_TEST_ABORTED);
    } else
    	finished = 1; /* Tell the main loop that the display list
		     has been traversed */

    TRACE_OUT

} /* End of user_signal_handler */


/**********************************************************************/
open_hawk()
/**********************************************************************/
/* Initialize the Hawk Subsystem, returns 1 on success and 0 on
   failure. */

{
 
    extern Pixrect *open_pr_device();

    int i;
    int op;
    unsigned *build_clearscreen_ctx();
    unsigned *ptr;
    struct gt_connect conn;
    unsigned *dummy_vm;


    func_name = "open_hawk";
    TRACE_IN

/* Open Hawk (get the kernel connections) */

    if (hawk_opened) {
	TRACE_OUT
	return 1;
    }

    /* clear WID plane */
    pr = open_pr_device();
    if (pr == NULL) {
	TRACE_OUT
	return 0;
    }
    (void)pr_clear_all(pr);
    wid = wid_alloc(pr, FB_WID_DBL_24);
    pr_set_planes(pr, PIXPG_WID, PIX_ALL_PLANES);
    op = PIX_SRC | PIX_COLOR(wid);
    op = pr_clear(pr, op);
    close_pr_device();

    if (hk_open()) {
	fb_send_message(SKIP_ERROR, ERROR, DLXERR_HK_OPEN_FAILED);
	TRACE_OUT
	return 0;
    }

/* vm_size must include everything we put in the memory */
    vm_size = MAX_VM_SIZE;
    vm = (unsigned *) hk_mmap((char *) 0, vm_size);
    if (vm == NULL) {
	TRACE_OUT
        return 0;
    }
    base_vm = vm;


/* Build user MCB */
    user_mcb_ptr = (Hkvc_umcb *) vm;
    vm += sizeof(Hkvc_umcb)/sizeof(unsigned);
    ptr = (unsigned *) user_mcb_ptr;
    for (i = 0; i < (sizeof(Hkvc_umcb) / sizeof(unsigned)); i++)
        *ptr++ = 0;                     /* Clear out all words */

/* Set up window boundaries for all display lists */
    window_boundary = (Hk_window_boundary *) vm;
    vm += sizeof(Hk_window_boundary)/sizeof(unsigned);
/* Initialize the window_boundary */
    window_boundary->xleft = screen_x;
    window_boundary->ytop = screen_y;
    window_boundary->width = screen_width;
    window_boundary->height = screen_height;

    /* reserve static space for fast clear */
    clearscreen_dl = vm;
    vm = build_clearscreen_ctx();

    /* reserve static space for common context */
    dl_ctx = (Hk_context *) vm;
    /* intialise the ctx */
    (void)init_dl_ctx();

    /* reserve static space for test hdl files to be loaded */
    vm += sizeof(Hk_context)/sizeof(unsigned);
    /* vm will not be changed any more from now on */

/* Build a dummy dl which does nothing */
    dummy_vm = vm;	/* use dummy vm and keep the original vm */

    dl_ctx->risc_regs[HK_RISC_SP] = (int) (base_vm + (vm_size/sizeof(unsigned)));
    dl_ctx->dpc = (unsigned) dummy_vm;

    dl_ctx->window_bg_color.r = BG_R;
    dl_ctx->window_bg_color.g = BG_G;
    dl_ctx->window_bg_color.b = BG_B;
    dl_ctx->s.current_wid = wid;
    /*
    dl_ctx->s.wid_clip_mask = 0x000;
    dl_ctx->s.wid_write_mask = 0x3ff;
    */

    /*
    if (screen_height > screen_width) {
        dl_ctx->s.view.vt[HKM_1_1] = (float) screen_width / (float) screen_height;
    } else {
        dl_ctx->s.view.vt[HKM_0_0] = (float) screen_height / (float) screen_width;
    }
    dl_ctx->fast_clear_set = fastclear_set;
    */
    dl_ctx->s.z_buffer_update = HK_Z_UPDATE_ALL;
    dl_ctx->draw_buffer = HK_BUFFER_A;
 
    /*
    *dummy_vm++ = ((HK_OP_UPDATE_LUT << HK_OP_POS) |
		(HK_UPDATE_FCBG << HK_SO_POS) |
		(HK_AM_IMMEDIATE << HK_AM_POS));
    *dummy_vm++ = fastclear_set;
    *dummy_vm++ = ((int)(BG_B * 255.0) << 16) | ((int)(BG_G * 255.0) << 8) | (int)(BG_R * 255.0);

    *dummy_vm++ = (HK_OP_WINDOW_CLEAR << HK_OP_POS);
 
    *dummy_vm++ = ((HK_OP_UPDATE_LUT << HK_OP_POS) | (HK_UPDATE_WLUT << HK_SO_POS) | (HK_AM_IMMEDIATE << HK_AM_POS));
    *dummy_vm++ = wid;
    *dummy_vm++ = 0x3ff;
    *dummy_vm++ = HK_BUFFER_A;
    *dummy_vm++ = HK_BUFFER_A;
    *dummy_vm++ = HK_24BIT;
    *dummy_vm++ = DEF_CLUT;
    *dummy_vm++ = fastclear_set;
    */

    *dummy_vm++ = (HK_OP_TRAP << HK_OP_POS);
    

    user_mcb_ptr->magic = HK_UMCB_MAGIC;
    user_mcb_ptr->version = HK_UMCB_VERSION;
    user_mcb_ptr->context = (unsigned *) dl_ctx;
    user_mcb_ptr->command = HKUVC_LOAD_CONTEXT; /* Don't pause == go */

/* Set up a signal handler (must come before hk_connect) */
    (void)signal(SIGXCPU, user_signal_handler);
 
/* Get the MCB and context connected to Hawk */
    conn.gt_pmsgblk = user_mcb_ptr;
    if (hk_connect(&conn) < 0) {
	fb_send_message(SKIP_ERROR, ERROR, DLXERR_CONNECT);
	TRACE_OUT
	return 0;;
    }

    vcom_host_u = conn.gt_puvcr;
    vcom_host_s = conn.gt_psvcr;

#ifdef DEBUG [
	printf("**** open_hawk() Testing hawk ready ****\n");
#endif DEBUG ]

    delay = FE_DELAY;
    /* Make sure Hawk is ready */
    while (*vcom_host_u && (delay-- >= 0)) usleep(10);
    if (delay < 0) {
	dump_status();
	fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
    }

    /* Make Hawk travese dummy display list once */
    *vcom_host_u = 1;

#ifdef DEBUG [
	printf("**** open_hawk() Traverse dummy display list ****\n");
#endif DEBUG ]

    delay = FE_DELAY;
    while (!finished && (delay-- >= 0)) usleep(10);     /* Wait for interrupt */
    if (delay < 0) {
	dump_status();
	fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
    }

    finished = 0;
    hawk_opened = 1;		/* Hawk is now ready for use */

    TRACE_OUT
    return 1;
}


/**********************************************************************/
unsigned *
build_clearscreen_ctx()
/**********************************************************************/

{
 
    unsigned *cs_dl;

    func_name = "build_clearscreen_ctx";
    TRACE_IN

    cs_dl = clearscreen_dl;

/* Prepare to clear the full screen */

    *cs_dl++ = ((HK_OP_UPDATE_LUT << HK_OP_POS) |
		(HK_UPDATE_FCBG << HK_SO_POS) |
		(HK_AM_IMMEDIATE << HK_AM_POS));
    fcs_fcbg = cs_dl++;
    fcbg     = cs_dl++;

    *cs_dl++ = (HK_OP_WINDOW_CLEAR << HK_OP_POS);

    *cs_dl++ = ((HK_OP_UPDATE_LUT << HK_OP_POS) |
		(HK_UPDATE_WLUT << HK_SO_POS) |
		(HK_AM_IMMEDIATE << HK_AM_POS));
    wid_entry = cs_dl++;                  /* Entry */
    *cs_dl++ = 0x1f;                       /* Mask */
    imgbuf   = cs_dl++;			   /* Image buffer */
    ovlbuf   = cs_dl++;			   /* Overlay buffer */
    *cs_dl++ = HK_24BIT;                   /* Plane_group */
    *cs_dl++ = DEF_CLUT;                   /* CLUT */
    fcs_wlut = cs_dl++;

    *cs_dl++ = (HK_OP_TRAP << HK_OP_POS);

    TRACE_OUT

    return cs_dl;


}


/**********************************************************************/
char *
exec_dl(dl_file)
/**********************************************************************/
char *dl_file;
{
    char *exec_dl_op();
    int wait;
    char *res;


    func_name = "exec_dl";
    TRACE_IN

    wait = 1;
    res = exec_dl_op(dl_file, wait);

    TRACE_OUT

    return res;
}

/**********************************************************************/
char *
exec_dl_nowait(dl_file)
/**********************************************************************/
char *dl_file;
{
    char *exec_dl_op();
    int wait;
    char *res;


    func_name = "exec_dl_nowait";
    TRACE_IN

    wait = 0;
    res = exec_dl_op(dl_file, wait);

    TRACE_OUT

    return res;
}


/**********************************************************************/
char *
exec_dl_op(dl_file, wait)
/**********************************************************************/
char *dl_file;
int wait;
/* loads display list into the vm and starts the display list */

{

    char *hk_error_string();
    char *diag_escape_error_string();

    FILE *fd = (FILE *)0;
    unsigned	*dl_vm;	/* Pointer to available VM for test display list */
    int	dl_size;/* Size in bytes of the display list */


    func_name = "exec_dl_op";
    TRACE_IN

    error = 0;

    /* intialise the ctx */
    (void)init_dl_ctx();

    dl_vm = vm;

    dl_ctx->dpc = (unsigned) dl_vm;

/* Allocate and load a display list */
 
    if ((fd = fopen(dl_file, "r")) == NULL) {
        sprintf(errtxt, DLXERR_OPEN_HDL_FILE, dl_file);
        goto err_return;
    }
 
    dl_size = hk_read_hdl_header(fd);
    if (dl_size < 0) {
        sprintf(errtxt, DLXERR_READ_HDL_FILE, dl_file);
        goto err_return;
    } else if (dl_size > (((int)base_vm + (int)vm_size) - ((int)dl_vm)) - (SCRATCH_BUFFER_SIZE+1)*sizeof(unsigned)) {
	sprintf(errtxt, DLXERR_HDL_FILE_TOO_BIG, dl_size, (((int)base_vm + (int)vm_size) - ((int)dl_vm)) - (SCRATCH_BUFFER_SIZE+1)*sizeof(unsigned));
	goto err_return;
    }

    if (hk_load_hdl_file(fd, dl_size, dl_vm, dl_vm)) {
        sprintf(errtxt, DLXERR_READ_HDL_FILE, dl_file);
        goto err_return;
    }
    (void)fclose(fd);
    (void)unlink(dl_file);
    fd = (FILE *)0;

    dl_vm += dl_size/sizeof(unsigned);

    /* set scratch buffer */
    dl_ctx->scratch_buffer = (Hk_scratch_buffer *)dl_vm;
    *dl_vm++ = SCRATCH_BUFFER_SIZE;
    dl_vm += SCRATCH_BUFFER_SIZE+1;
    dl_ctx->risc_regs[HK_RISC_SP] = (int)(base_vm + (vm_size/sizeof(unsigned)));
    /* Pass the current WID to the display list for double buffering */
    dl_ctx->risc_regs[HK_RISC_R2] = wid;
    dl_ctx->risc_regs[HK_RISC_R3] = fastclear_set;
    dl_ctx->s.current_wid = wid;
    dl_ctx->s.wid_clip_mask = 0x000;
    dl_ctx->s.wid_write_mask = 0x3ff;
    dl_ctx->fast_clear_set = fastclear_set;

    /* if -1, wait for other argumens to be passed into VM */
    if (wait != -1) {
	(void)start_dl(dl_ctx, wait);
    }

    finished = 0;
    if (error) {
	sprintf(errtxt, DLXERR_HDL_EXEC, error, (error < 0) ? diag_escape_error_string(error) : hk_error_string(error));
	TRACE_OUT
	return errtxt;
    } else {
	TRACE_OUT
	return (char *)0;
    }

    err_return:
	if (fd) {
	    (void)fclose(fd);
	}
	(void)unlink(dl_file);
        finished = 0;
	TRACE_OUT
        return errtxt;

}

/**********************************************************************/
start_dl(ctx, wait)
/**********************************************************************/
Hk_context *ctx;
int wait;

{

    func_name = "start_dl";
    TRACE_IN

    user_mcb_ptr->errorcode = 0;
    user_mcb_ptr->context = (unsigned *) ctx;
    user_mcb_ptr->command = HKUVC_LOAD_CONTEXT; /* Don't pause == go */

#ifdef DEBUG [
	printf("**** start_dl() Make sure Hawk is ready ****\n");
#endif DEBUG ]

    delay = FE_DELAY;
    /* Make sure Hawk is ready */
    while (*vcom_host_s && (delay-- >= 0)) usleep(10);    /* Wait for completion */
    if (delay < 0) {
	dump_status();
	fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
    }

#ifdef DEBUG [
	printf("**** start_dl() Make sure Hawk is stopped ****\n");
#endif DEBUG ]

    delay = FE_DELAY;
    while (!user_mcb_ptr->gt_stopped && (delay-- >= 0)) usleep(10); /* Make sure Hawk is stopped */
    if (delay < 0) {
	dump_status();
	fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
    }

    finished = 0;

    *vcom_host_s = 1;

#ifdef DEBUG
    printf("*** LOAD in start_dl() ***\n");
    printf("user mcb: gt_stopped=0x%x status=0x%x, dpc=0x%x, errorcode=0x%x, command=0x%x\n", user_mcb_ptr->gt_stopped, status, dpc, error, user_mcb_ptr->command);
    printf("trap_instruction = 0x%x gt_flags=0x%x instruction_count=0x%x\n", user_mcb_ptr->trap_instruction, user_mcb_ptr->gt_flags, user_mcb_ptr->instruction_count);
#endif DEBUG

    if (wait) {
	delay = FE_DELAY*10;
	if (!finished) {	
		/* Wait for interrupt */
		while (!finished && (delay-- >= 0)) usleep(10);  
		if (delay < 0) {
	    		dump_status();
	    		fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
		}
	}
    }

    TRACE_OUT
}

/**********************************************************************/
stop_dl()
/**********************************************************************/
{
    int save_cmd;


    func_name = "stop_dl";
    TRACE_IN

#ifdef DEBUG [
	printf("**** stop_dl() Make sure Hawk is ready ****\n");
#endif DEBUG ]

    delay = FE_DELAY;
    while (*vcom_host_s && (delay-- >= 0)) usleep(10);    /* Make sure Hawk is ready */
    if (delay < 0) {
	dump_status();
	fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
    }

    save_cmd = user_mcb_ptr->command;

    user_mcb_ptr->command = HKUVC_FLUSH_RENDERING_PIPE
			    | HKUVC_FLUSH_FULL_CONTEXT
			    | HKUVC_PAUSE_WITHOUT_INTERRUPT;
    *vcom_host_s = 1;       /* Tell Front End to do it */
    delay = FE_DELAY;

#ifdef DEBUG
    printf("*** FLUSH in stop_dl() ***\n");
    printf("user mcb: gt_stopped=0x%x status=0x%x, dpc=0x%x, errorcode=0x%x, command=0x%x\n", user_mcb_ptr->gt_stopped, status, dpc, error, user_mcb_ptr->command);
    printf("trap_instruction = 0x%x gt_flags=0x%x instruction_count=0x%x\n", user_mcb_ptr->trap_instruction, user_mcb_ptr->gt_flags, user_mcb_ptr->instruction_count);
#endif DEBUG

    while (*vcom_host_s && (delay-- >= 0)) usleep(10);    /* Wait for completion */
    if (delay < 0) {
	dump_status();
	fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FE_TIMEOUT);
    }

    user_mcb_ptr->command = save_cmd;

    TRACE_OUT
}

/**********************************************************************/
clear_all()
/**********************************************************************/
{

    func_name = "clear_all";
    TRACE_IN

    /*
    int op;

    wid = wid_alloc(pr, FB_WID_DBL_24);
    pr_set_planes(pr, PIXPG_WID, PIX_ALL_PLANES);
    op = PIX_SRC | PIX_COLOR(wid);
    op = pr_clear(pr, op);
    */

    (void)fast_clear_all(wid);

    TRACE_OUT

}
/**********************************************************************/
fast_clear_all(wid_index)
/**********************************************************************/
int wid_index;
{
    func_name = "fast_clear_all";
    TRACE_IN

    (void)fast_clear(fastclear_set, HK_BUFFER_B, HK_BUFFER_B, BG_R, BG_G, BG_B, wid_index);
    (void)fast_clear(fastclear_set, HK_BUFFER_A, HK_BUFFER_A, BG_R, BG_G, BG_B, wid_index);

    TRACE_OUT
}

/**********************************************************************/
fast_clear(fcs, disp, clr, bg_r, bg_g, bg_b, wid_index)
/**********************************************************************/
int fcs;	/* Fast clear set */
int disp;	/* Display buffer */
int clr;	/* Clear buffer */
float bg_r, bg_g, bg_b; /* clear to bg color */
int wid_index;
{


    func_name = "fast_clear";
    TRACE_IN


    dl_ctx->dpc = (unsigned) clearscreen_dl;

    dl_ctx->window_bg_color.r = bg_r;
    dl_ctx->window_bg_color.g = bg_g;
    dl_ctx->window_bg_color.b = bg_b;
    dl_ctx->s.z_buffer_update = HK_Z_UPDATE_ALL;
    dl_ctx->fast_clear_set = *fcs_fcbg = *fcs_wlut = fcs;
    *wid_entry = wid_index;
    *imgbuf = *ovlbuf = disp;
    dl_ctx->draw_buffer = clr;

    *fcbg = ((int)(bg_b * 255.0) << 16) | ((int)(bg_g * 255.0) << 8) |
	    (int)(bg_r * 255.0);

    /* fast clear the screen */
    (void)start_dl(dl_ctx, 1);

    TRACE_OUT

}

/**********************************************************************/
close_hawk()
/**********************************************************************/

{

    func_name = "close_hawk";
    TRACE_IN

    if(hawk_opened) {
	stop_dl();
	if (hk_disconnect()) {
	    fb_send_message(SKIP_ERROR, WARNING, DLXERR_HK_DISCONNECT_FAILED);
	}
	if (hk_munmap(base_vm, vm_size)) {
	    fb_send_message(SKIP_ERROR, WARNING, DLXERR_HK_MUNMAP_FAILED);
	}
    }
    hk_close(1);
    hawk_opened = 0;

    TRACE_OUT

}

/**********************************************************************/
init_dl_ctx()
/**********************************************************************/
{

    func_name = "init_dl_ctx";
    TRACE_IN

/* Initialize the context area */
    (void)hk_init_default_context(dl_ctx);
/* Initialize the window boundaries */
    dl_ctx->ptr_window_boundary = window_boundary;

    TRACE_OUT

}
    
/*********************************************************************/
char *
chk_prb()
/*********************************************************************/
{
    int *rb;
    int miss_line;
    int miss_tri;
    int miss_outside;

    func_name = "chk_prb";
    TRACE_IN

    rb = (int *)vm;
    rb++;
    rb++;

    miss_line = 50 - *rb;
    rb++;
    miss_tri = 50 - *rb;
    rb++;
    miss_outside = *rb;

    if (miss_line || miss_tri || miss_outside) {
	sprintf(errtxt, DLXERR_PICK_MISSES, miss_line, miss_tri, miss_outside);
	TRACE_OUT
	return errtxt;
    }

    TRACE_OUT
    return (char *) 0;

}

/*********************************************************************/
set_fb_stereo()
/*********************************************************************/
{
    int fd;
    int mode;

    func_name = "set_fb_stereo";
    TRACE_IN

    fd = open(device_name, O_RDWR);
    if (fd < 0) {
        dump_status();
        (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_OPEN);
    }

    /* save the original fb mode */
    if (ioctl(fd, FB_GETMONITOR, &fbmode) < 0) {
        dump_status();
        (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FB_GETMONITOR, sys_errlist[errno]);
    }

    /* Have to set this first in order to be able to set fb mode */
    if (ioctl(fd, FB_SETDIAGMODE) < 0) {
        dump_status();
        (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FB_SETDIAGMODE, sys_errlist[errno]);
    }

    /* set fb to stereo mode */
    mode = GT_MONITOR_TYPE_STEREO;
    if (ioctl(fd, FB_SETMONITOR, &mode) < 0) {
        dump_status();
        (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FB_SETMONITOR, mode, sys_errlist[errno]);
    }

    dev_fd = fd;

    TRACE_OUT
}

/*********************************************************************/
save_fb_regs()
/*********************************************************************/
{
    char *errmsg;
    unsigned int *reg;

    func_name = "save_fb_regs";
    TRACE_IN

    xtract_hdl(SAVE_FB_MODE);
    errmsg = exec_dl(getfname(SAVE_FB_MODE, HDL_CHK));
    if (errmsg) {
	(void)fb_send_message(SKIP_ERROR, WARNING, errmsg);
    }

    /* save the FB regs */
    reg = vm;
    reg++;
    reg++;
    bcopy(reg, fb_regs, sizeof(fb_regs));

    /* turn the flag on */
    fb_saved = 1;

    TRACE_OUT
}

/*********************************************************************/
restore_fb()
/*********************************************************************/
{
    char *errmsg;
    int fd;
    unsigned int *reg;

    func_name = "restore_fb";
    TRACE_IN

    if (!fb_saved) {
	TRACE_OUT
	return;
    }


    if (dev_fd < 0) {
	fd = open(device_name, O_RDWR);
	if (fd < 0) {
	    dump_status();
	    (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_OPEN);
	}
	/* Have to set this first in order to be able to set fb mode */
	if (ioctl(fd, FB_SETDIAGMODE) < 0) {
	    dump_status();
	    (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FB_SETDIAGMODE, sys_errlist[errno]);
	}
    } else {
	fd = dev_fd;
    }

    if (ioctl(fd, FB_SETMONITOR, &fbmode) < 0) {
        dump_status();
        (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_FB_SETMONITOR, fbmode, sys_errlist[errno]);
    }
    (void)close(fd);
    dev_fd = -1;

    (void)stop_dl();

    /* load the hdl file (DIAG_ESCAPE), but don't start it (-1) */
    xtract_hdl(RESTORE_FB_MODE);
    errmsg = exec_dl_op(getfname(RESTORE_FB_MODE, HDL_CHK), -1);
    if (errmsg) {
	(void)fb_send_message(SKIP_ERROR, WARNING, errmsg);
    }

    /* copy register values to VM */
    reg = vm;
    reg++;
    reg++;
    bcopy(fb_regs, reg, sizeof(fb_regs));

    /* start restoring the FB registers */
    (void)start_dl(dl_ctx, 1);

    finished = 0;
    if (error) {
	sprintf(errtxt, DLXERR_HDL_EXEC, error, (error < 0) ? diag_escape_error_string(error) : hk_error_string(error));
	TRACE_OUT
	(void)fb_send_message(SKIP_ERROR, WARNING, errtxt);
	return;
    }

    fb_saved = 0;
    TRACE_OUT
}

/*********************************************************************/
char *
diag_escape_error_string(err)
/*********************************************************************/
int err;

{
    static char *diag_esc_errlist[] = {
					"",
					MEMORY_ERROR,
					MEMORY_ERROR,
					HARDWARE_TIMEOUT,
					CHECKSUMS_DONT_MATCH,
					(char *)0,
				      };
    int list_size;

    /* strip minus sign */
    err = -err;

    /* count the messages in the list */
    for (list_size = 0 ; diag_esc_errlist[list_size] ; list_size++);

    if (err > list_size) {
	return UNKNOWN_ERROR;
    }

    return diag_esc_errlist[err];
}

/*********************************************************************/
ctx_set(va_alist)
/*********************************************************************/
va_dcl

{
    Ctx_attr attr;
    va_list	ap;

    func_name = "ctx_set";
    TRACE_IN

    va_start(ap);

    while (attr = va_arg(ap, Ctx_attr)) {
	switch ((Ctx_attr)attr) {

	    case	CTX_ATTR_END:
	    break;

	    default:
		va_end(pa);
		fb_send_message(SKIP_ERROR, ERROR, DLXERR_UNKNOWN_CTX_ATTRIBUTE);
		return -1;
	}
    }
    va_end(pa);
    TRACE_OUT
    return 0;
}
