/*	@(#)sundiag.h 1.1 92/07/30 Copyright Sun Micro"		*/

#include <stdio.h>
#include <sys/types.h>
#include <suntool/sunview.h>
#include <suntool/text.h>
#include <suntool/tty.h>
#include <suntool/canvas.h>
#include <suntool/panel.h>
#include <suntool/scrollbar.h>

/*************************** global defines *****************************/
/*
 * subwindow layout
 */
#define	STATUS_WIDTH	0.35	/* 35/100 of the frame width */
#define PERFMON_WIDTH	0.28	/* 28/100 of the frame width */

#define PERFMON_HEIGHT	0.54	/* 54/100 of the frame height */

/*
 * hardwired directories
 */
#define LOG_DIR		"/usr/adm/sundiaglog"
#define OPTION_DIR	"/usr/adm/sundiaglog/options"

#define	INFO_FILE	"sundiag.info"
#define ERROR_FILE	"sundiag.err"
#define CONF_FILE	"sundiag.conf"
#define OPT_FILE	".sundiag"    /* the default "startup" option file */
#define USER_FILE	".usertest"   /* user-defined tests description file */
#define VERSION_FILE	".version"    /* file contains the patched version # */
#define VERSION		"2.3.1"         /* sundiag version/label */

/*
 * font stuff
 */
#define STANDARD_FONT	"/usr/lib/fonts/fixedwidthfonts/screen.b.14"
#define BOLD_FONT	"/usr/lib/fonts/fixedwidthfonts/gallant.r.19"

/*
 * misc.
 */

#ifndef IDLE
#define	IDLE	0	/* no tests are running */
#endif

#define	GO	1	/* the enabled tests are running, no failure yet */
#define	KILL	2	/* in the process of killing the tests */ 
#define	SUSPEND	3	/* the tests are currently suspended */

#define	SEL_DEF	0	/* The "Default" test selection */
#define	SEL_NON	1	/* The "None" test selection */
#define	SEL_ALL	2	/* The "All" test selection */

#define	MAXTESTS	200		/* max. # of tests */

/************************* end of global defines **********************/

/************************* global variables ***************************/
/*
 * pixfonts		( in sundiag.c )
 */
extern struct pixfont	*sundiag_font;		/* current sundiag font */

/*
 * window handles	( in sundiag.c )
 */
extern Frame		sundiag_frame;
extern Panel		sundiag_control;
extern Tty		sundiag_console;
extern Canvas		sundiag_perfmon;
extern Panel		sundiag_status;
extern Scrollbar	status_bar, control_bar;

extern int		frame_width;		/* screen width */
extern int		frame_height;		/* screen height */

/*
 * SunView objects	( in panel.c )
 */
extern Menu             log_files_menu;

extern Panel_item	test_item;
extern Panel_item	reset_item;
extern Panel_item	log_files_item;
extern Panel_item	option_files_item;
extern Panel_item	options_item;
extern Panel_item	quit_item;
extern Panel_item	pscrn_item;
extern Panel_item	ttime_item;
extern Panel_item	eeprom_item;
extern Panel_item       cpu_item;

extern Panel_item	mode_item;
extern Panel_item	select_item;

/*
 * Option flags(switches)	( in panelprocs.c )
 */
extern int		core_file;	/* generate core files? */
extern int		single_pass;	/* single pass for all enabled tests? */
extern int		quick_test;	/* run tests with "q(uick)" option? */
extern int		verbose;	/* run tests in verbose mode? */
extern int		trace;		/* run tests in trace mode? */
extern int		auto_start;	/* start testing when booted? */
extern int		run_error;	/* enable run_on_error mode? */
extern int		max_errors;	/* max. # of errors allowed if */
					/* run_on_error was enabled */
extern int		max_tests;	/* max. # of tests in a group */
extern int		max_sys_pass;	/* the maximum passes to run */
extern int		log_period;	/* period(minutes) to log the status */
extern int		send_email;	/* flag for sending emails */
extern char		eaddress[];	/* to contain the email address */

extern int		intervention;
extern int		select_value;

/* 
 * eeprom defines
 */
extern char 	  hwupdate_file[];
extern char 	  bootdev_file[];
extern char 	  banner_file[];
extern char 	  diagpath_file[];
extern char 	  diagdev_file[];
extern int 	  console_file;
extern int 	  scrsize_file;
extern int 	  ttya_rtsdtr_file;
extern int 	  ttyb_rtsdtr_file;
extern int 	  keyclick_file;
extern int 	  watchdog_reboot_file;
extern int 	  default_boot_file;
extern int 	  custom_banner_file;
extern int 	  kbdtype_file;
extern int 	  memsize_file;
extern int 	  memtest_file;
extern int 	  columns_file;
extern int 	  rows_file;

/*
 * schedule defines
 */
extern int schedule_file;
extern char start_date_file[];
extern char start_time_file[];
extern char stop_date_file[];
extern char stop_time_file[];

/*
 * processors defines in mptest option menu
 */
extern int processors_mask;
extern int number_processors;

/*
 * processors defines in mptest system menu
 */
extern int number_system_processors;
extern int system_processors_mask;
extern int system_processor_selected[30];

/*
 * batch defines
 */
extern	int  batch_mode;		/* declared in batch.c */
/*
 * misc
 */
extern char		*cpuname;	/* in tests.c, contains the cpu type */
extern char		*version;	/* in sundiag.c, contains the version */
extern char		option_fname[];	/* in sundiag.c */
extern char		*info_file;	/* in sundiag.c, information filename */
extern char		*error_file;	/* in sundiag.c, error filename */
extern char		*conf_file;	/* in sundiag.c, configuration file */
extern FILE		*info_fp;	/* in sundiag.c, info_file's fp */
extern FILE		*error_fp;	/* in sundiag.c, error_file's fp */
extern struct group_info groups[];	/* in data.c */
extern struct test_info	tests_base[];	/* in data.c */
extern struct test_info	*tests[];	/* in test.c */
extern Frame		option_frame;	/* in test.c, to keep option popup */
extern int		color_monitor;	/* whether it's a color system */
extern int		exist_tests;	/* total # of existing tests */
extern int		tty_fd;		/* in sundiag.c(fd of the console) */
extern char		printer_name[];	/* in sundiag.c(name of the printer) */
extern int		tty_mode;	/* in sundiag.c, =1 if running TTY */
extern int		tty_ppid;	/* in tty.c */
extern int		running;	/* in panel_procs.c(testing status) */
extern int		fail_flag;	/* in status.c(any test failed?) */
extern int		ngroups;	/* total # of defined groups */
extern int		sundiag_done;	/* user has just quitted sundiag */
extern int		sys_error;	/* to keep total error count */
extern int		sys_pass;	/* to keep system pass count */
extern unsigned		elapse_count;	/* to count the elapsed senconds */
extern unsigned		last_elapse;	/* when elapse_count was updated */
extern int		pfd[];		/* file descriptors for message pipe */
extern int		pty_fd;		/* file descriptor for pseudo console */
extern int		selection_flag;	/* whether "test selection" inverted */

/*
 * ATS special flags.
 */
extern int		ats_nohost;	/* "standalone in ATS mode" flag */

/************************* end of global variables ***********************/
