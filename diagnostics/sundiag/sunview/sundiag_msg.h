/****** @(#)sundiag_msg.h	1.1  7/30/92 Copyright Sun Micro **********/

/***********************************************************
	ERROR CODE defines for Sunview INFO message type.
************************************************************/
#define START_SUNDIAG_INFO 	1
#define QUIT_SUNDIAG_INFO	2
#define QUIT_ABNORM_INFO	3
#define START_ALL_INFO		4
#define STOP_ALL_INFO		5
#define RESET_PECOUNT_INFO	6
#define PERIOD_LOG_INFO		7
#define SCHEDULE_START_INFO	8
#define SCHEDULE_STOP_INFO	9
#define ENABLE_TEST_INFO	10
#define DISABLE_TEST_INFO	11
#define TEST_FAILED_INFO	12
#define FAILED_NET_INFO		13
#define LOADED_OPTION_FILE      14
#define BATCH_PERIOD_INFO       15

/************************************************************
	ERROR CODE defines for Sunview ERROR message type.
*************************************************************/
#define PID_VALID_ERROR		101
#define CORE_DUMP_ERROR		102
#define TERM_SIGNAL_ERROR	103
#define SEND_MESSAGE_ERROR	104
#define FORK_ERROR		105
#define RPCINIT_ERROR      	106
 

/***************************************************************
	externs of Sundiag info messages type for Sunview part.
****************************************************************/
extern char *start_sundiag_info; 
extern char *quit_sundiag_info;
extern char *quit_abnorm_info;
extern char *start_all_info;
extern char *stop_all_info;
extern char *reset_pecount_info;
extern char *period_log_info;
extern char *schedule_start_info;
extern char *schedule_stop_info;
extern char *enable_test_info;
extern char *disable_test_info;
extern char *test_failed_info;
extern char *loaded_option_file;
extern char *batch_period_info;


/***************************************************************
	externs of Sundiag error message type for Sunview part.
****************************************************************/
extern char *pid_valid_error;
extern char *core_dump_error;
extern char *term_signal_error;
extern char *send_message_error;
extern char *rpcinit_error;
extern char *fork_error;
