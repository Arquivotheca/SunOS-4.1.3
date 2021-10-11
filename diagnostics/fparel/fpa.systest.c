static char sccsid[] = "@(#)fpa.systest.c 1.1 7/30/92 Copyright Sun Microsystems"; 

/*
 * ****************************************************************************
 * Program Name    : fparel 
 * Usage           : fparel
 * Option(s)       : -pn -v 
 * Source File     : fpa.systest.c
 * Original Engr   : Nancy Chow
 * Date            : 10/12/88
 * Function        : This file contains the main module for the fparel test.
 *		   : The fparel is an online reliability test for the fpa-3x.
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

/* ***************
 * Include Files
 * **************/

#include <sys/types.h>
#include "fpa3x.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sundev/fpareg.h>

/* ***********
 * Externals
 * **********/

extern int ierr_test();
extern int imask_test();
extern int ldptr_test();
extern int map_ram_sel();
extern int ustore_ram_sel();
extern int reg_uh_ram_sel();

extern int sim_ins_test();
extern int reg_ram();
extern int shadow_ram();
extern int pointer_test();
extern int ptr_incdec_test();
extern int lock_test();
extern int nack2_test();
extern int wlwf_test();
extern int test_mode_reg();
extern int test_wstatus_reg();
extern int fpa_wd();
extern int w_op_test();
extern int status_sel();
extern int w_jump_cond_test();
extern int timing();

/* ***************
 * Local Globals
 * **************/

int lin_pack_test();

struct test_struct {
    int  (*test)();
    char *test_nam;
    int  pre_act;			/* pre-test action */
    int  post_act;			/* post-test action */
};

struct test_struct test_info[] = {

    {  ierr_test       , "Ierr"			  , NONE,    NONE},
    {  imask_test      , "Imask"		  , NONE,    NONE},
    {  ldptr_test      , "Load Pointer"		  , NONE,    NONE},
    {  map_ram_sel     , "Mapping Ram"		  , LOAD_ON, LOAD_OFF},
    {  ustore_ram_sel  , "Micro Store Ram"	  , LOAD_ON, LOAD_OFF},
    {  reg_uh_ram_sel  , "Register Ram Upper Half", NONE,    NONE},
    {  sim_ins_test    , "Simple Instruction"     , NONE,    NONE},
    {  reg_ram         , "Register Ram Lower Half", NONE,    NONE},
    {  shadow_ram      , "Shadow Ram"             , NONE,    NONE},
    {  pointer_test    , "Pointer"		  , NONE,    NONE},
    {  ptr_incdec_test , "Pointer Incr. Decr."    , NONE,    NONE},
    {  lock_test       , "Lock"			  , SIG_SET, SIG_RSET},
    {  nack2_test      , "Nack"		  	  , SEG_SET, SEG_RSET},
    {  wlwf_test       , "F+"			  , NONE,    NONE},
    {  test_mode_reg   , "Mode Register"	  , NONE,    NONE},
    {  test_wstatus_reg, "Wstatus Register"	  , NONE,    NONE},
    {  fpa_wd	       , "Weitek/TI Data Path"	  , NONE,    NONE},
    {  w_op_test       , "Weitek/TI Operation"	  , NONE,    NONE},
    {  status_sel      , "Weitek/TI Status"	  , NONE,    NONE},
    {  w_jump_cond_test, "Jump Conditions"	  , NONE,    NONE},
    {  timing	       , "Timing"		  , NONE,    NONE},
    {  lin_pack_test   , "Linpack"		  , NONE,    NONE}
};

main(argc, argv)
int  argc;
char *argv[];
{

int	test_cnt;			/* current test count */
int	value;				
int     exec_val;
char    mesg[180];			/* mesg str */
u_long	pass_count;			/* current pass count */
u_long  contexts;               	/* to get context in case of failure */
u_long	val_rotate;

    init_fparel();			/* init data structs */
    if (!parse_args(argc, argv))	/* if invalid parameters */
	exit(1);			/* fail test */

    if (verbose)
	printf("FPA System (Reliability) Test : ");

    if (!(winitfp_())) {		/* init fpa */
	if (verbose)
	    printf("Could not open FPA.\n");
	exit(0);
    }

    close_new_context();		/* close context of previous open */

    for (pass_count=1; no_times != 0; no_times--, pass_count++) {
	contexts = 0;				/* init context number */
	value = open_new_context();		/* open for new context */
        if (!value) {
	    val_rotate = *(u_long *)REG_STATE & CXT_MASK;
	    contexts = (1 << val_rotate);
	    if (verbose)
	    	printf("All Tests - Context Number = %d\n", val_rotate);

	    for (test_cnt=0; test_cnt < MAX_TEST_CNT; test_cnt++) {
		exec_val = PASS;
		if (test_info[test_cnt].pre_act != NONE)
		    exec_val = exec_act(test_info[test_cnt].pre_act);	/* pre act */

		if (exec_val == PASS) {
	    	    if (verbose)
		    	printf("        %s Test.\n",test_info[test_cnt].test_nam);

		    value = test_info[test_cnt].test();		/* test */

		    if (test_info[test_cnt].post_act != NONE)	/* post act */
		    	exec_act(test_info[test_cnt].post_act);
		}

	    	if (value) {			/* test error? */

						/* close current context */
		    fail_close(test_info[test_cnt].test_nam, contexts);	
		    exit(1);			/* exit test */
	    	}
	    }
	    close_new_context();		/* release current context */

						/* verify other contexts */
	    if (value = other_contexts(&contexts)) {	
                if (value != RAM_LH_FAIL) { 	/* test ok. something wrong */
                                      		/*   with opening fpa */
                    fpa_open_fail(value);
                    restore_signals();
                    exit(0);
                }
                else {
						/* lower ram failed */
                    fail_close(test_info[RAM_LH_ERR].test_nam, contexts); 
                    exit(1);
                }
            }
	}
	else {
	    fpa_open_fail(value);
	    restore_signals();
	    exit(0);
	}
	
	if (verbose)
	    printf("PASS COUNT = %d\n",pass_count);
    }

    make_diaglog_msg(contexts, PASS, 0, mesg);
    restore_signals();
    exit(0);
}

parse_args(argc, argv)
int  argc;
char *argv[];
{

int i;

    for (i = 1; i < argc ; i++) {	/* for each arg entered */
	if (*argv[i] == '-') {		/* option selected? */
	    switch(argv[i][1]) {
		case 'v':		/* verbose mode? */
		    verbose = TRUE;
		    break;
		case 'p':		/* pass count? */
		    if (!(no_times = atoi(&argv[i][2]))) no_times = 0x7fffffff;
		    break;
		default:
		    printf("Unknown option %c\n", argv[i][1]);
		    return(FAIL);
		    break;
	    }
	}
	else {
	    printf("Usage: %s [-v] [-p<pass_count>]\n", *argv);
	    return(FAIL);
	}
    }
    return(PASS);
}

/* *************
 * init_fparel
 * *************/

/* Initialize fparel test state */

init_fparel()
{
    sig_err_flag = FALSE;	/* reset flag for fpa error handler */
    seg_sig_flag = FALSE;	/* reset flag for seg violation handler */
    verbose 	 = FALSE;	/* no verbose message display */
    no_times     = 1;		/* test pass count */
}

/* **********
 * exec_act
 * **********/

/* Execute a test preparation or cleanup action */

exec_act(act)
int act;			/* action to execute */
{
char null_str[2];
int  ret_val;

    ret_val = PASS;
    null_str[0] = '\0';

    switch(act) {
	case LOAD_ON :		/* set Load Enable */
	    if (ioctl(dev_no,FPA_LOAD_ON,(char *)null_str) < 0)
		ret_val = FAIL;
	    break;
	case LOAD_OFF:		/* reset Load Enable */
	    ioctl(dev_no,FPA_LOAD_OFF,(char *)null_str);
	    break;
	case SEG_SET:		/* expect segmentation violation */
	    seg_sig_flag = TRUE;
	    break;
	case SEG_RSET:		/* don't expect seg violation */
	    seg_sig_flag = FALSE;
	    break;
	case SIG_SET:		/* expect buserror */
	    sig_err_flag = TRUE;
	    break;
	case SIG_RSET:		/* don't expect buserror */
	    sig_err_flag = FALSE;
    }
    return(ret_val);
}

lin_pack_test()
{
	if ( slinpack_test() )
		return -1;
	if ( dlinpack_test() )
		return -1;
}
