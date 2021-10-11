#ifndef lint
static	char sccsid[] = "@(#)sundiag.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "sundiag.h"
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sun/fbio.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vfork.h>
#include "../../lib/include/libonline.h"
#include "testnames.h"
#include "sundiag_msg.h"

/*
 * frame's origin
 */
#define SUNDIAG_X		0	/* origin(fixed) */
#define SUNDIAG_Y		80

#define	SUNDIAG_PID		"/tmp/sundiag.pid"
#define MAXHOSTNAMELEN		64
/*
 * icon definitions
 */
static short    icon_image[] = {
#include "sundiag.icon"
};
DEFINE_ICON_FROM_IMAGE(sundiag_icon, icon_image);

Icon	passed_icon=(Icon)&sundiag_icon;

static short    failed_image[] = {
#include "failed.icon"
};
DEFINE_ICON_FROM_IMAGE(inverted_icon, failed_image);

Icon	failed_icon=(Icon)&inverted_icon;

/*
 * window object handles
 */
Frame           sundiag_frame;
Panel		sundiag_control;
Tty		sundiag_console=(Tty)0;
Canvas		sundiag_perfmon;
Panel		sundiag_status;
Scrollbar	status_bar, control_bar;

/*
 * pixfonts
 */
struct pixfont  *sundiag_font;

/*
 * miscellaneous globals
 */
int	tty_mode=FALSE;		/* will be set to 1 if running in TTY mode */
int	pfd[2];			/* file descriptors for error message pipe */
int	pty_fd;			/* pseudo TTY for console in TTY mode */
int	sundiag_done=0;		/* quiting flag */
char	option_fname[82];
char	printer_name[52];	/* to keep the printer name(for screen dump) */
int	tty_fd;			/* the file descriptor of the terminal device */
int	frame_width;
int	frame_height;
char	*info_file;		/* sundiag information file name */
char	*error_file;		/* sundiag error file name */
char	*conf_file;		/* sundiag configuration file name */
FILE	*info_fp;		/* info_file's file pointer */
FILE	*error_fp;		/* error_file's file pointer */
int	ats_nohost=FALSE;	/* "standalone in ATS mode" flag */
char	*remotehost=NULL;	/* name of remote machine to test */
char	*version=VERSION;	/* version number of the Sundiag */
int	config_file_only=FALSE;	/* create the sundiag.conf file only */	

extern	char *ttyname();	/* to get the device name of console */
extern	FILE *fopen();
extern	char *malloc();
extern	char *getenv();
extern	char *mktemp();
extern	char *sprintf();
extern	char *strcpy();
extern	char *strncpy();
extern	char *strtok();
extern	char *strrchr();
extern	char *fgets();
extern	time_t	time();
extern	Notify_value	foreground_proc();
extern	Notify_value	handle_input();
extern	Notify_value	message_input();
extern	Notify_value	console_input();
extern	void		sd_send_mail();
extern	FILE *conf_open(), *batch_fp;		/* declared in batch.c */
extern	int  batch_ready;			/* declared in batch.c */

/*
 * static variables
 */

static	int	tmp_argc=2;		/* to change the default font */
static	char	*tmp_argv[]={"-Wt", NULL, NULL};
static	int	become_console=0;
			/* flag for making console window UNIX console or not */
static	int	makedevice=0;		/* flag for making device file or not */
static	char	*hostname=NULL;		/* ATS hostname */
static	int	default_x, default_y;
#define	USEC_INC	0		/* update once every second. */
#define	SEC_INC		1
static	struct itimerval notify_timeout = { {SEC_INC,USEC_INC},
					    {SEC_INC,USEC_INC} };
static	int	client_object;
static	int	*client_handle = &client_object;
static	int	verbose=TRUE;		/* default to verbose */
static	int	probe_flag=TRUE;	/* default to probing the kernel */
static	int	auto_quit=FALSE;	/* defualt to not auto-quitting */
static	int	not_done=1;		/* probeing's done flag */
static	int	null_fd;		/* fd of /dev/null */
static	int	no_perfmon=FALSE;
/* perfmon won't work with special kernel because rpc.rstatd is hardwired
   to "/vmunix" */

static	char *ptynames[] =
{
    "/dev/ptyrf", "/dev/ptyre", "/dev/ptyqf", "/dev/ptyqe",
    "/dev/ptypf", "/dev/ptype", "/dev/ptyp7", "/dev/ptyp6"
};
#define NPTYS (sizeof(ptynames)/sizeof(*ptynames))

static	char *ttynames[] =
{
    "/dev/ttyrf", "/dev/ttyre", "/dev/ttyqf", "/dev/ttyqe",
    "/dev/ttypf", "/dev/ttype", "/dev/ttyp7", "/dev/ttyp6"
};

/*
 * forward declarations
 */
extern	Notify_value	kill_proc();
static	Notify_value	wait_probe();
static	int		check_exe();
static	void		check_arg();
static	Notify_value	stop_notifier_proc();
static	Notify_value	notify_proc();
static	int		check_sundiag();

typedef void	(*void_func)();		/* used to shut up lint */


/******************************************************************************
 * Main program starts here.						      *
 ******************************************************************************/
main(argc, argv)
int argc;
char *argv[];
{
  char  *tmpname="/tmp/sundiag.XXXXXX";		/* temporary ".suntools" file */
  char	cmd[122], *tmp, *tmp1;
  int	i, cgfour=0, remote=FALSE;
  FILE	*fp;
  struct fbtype	fb_type;
  struct fbgattr fb_gattr;
  extern char	*optarg;
  struct timeval c_tv;

	version_id = 0;
        test_id = get_test_id(argv[0], testname_list); 
	option_fname[0]=NULL;

	if (getuid())
	{
	  (void)fprintf(stderr,
			"Sundiag: must be a super user to run Sundiag.\n");
	  exit(1);
	}

	/* check command line for remote execution */
	for (i=0; i<argc; i++)
	  if (strcmp(argv[i], "-h") == 0) {
	    remote = TRUE;
	    break;
	  }
		/* if remote, don't check whether sundiag is already
		   running */
	if (!remote)
	  if (check_sundiag()) {
	    (void)fprintf(stderr,
			  "Sundiag: Sundiag is already running.\n");
	    exit(1);
	  }

	if (getenv("WINDOW_PARENT") == NULL)
	{
	  tmp = ttyname(0);		/* get the input device name */
	  if (tmp != NULL && strcmp(tmp+5, "console") == 0)
	  {
	    tmp = getenv("TERM");
	    if (tmp != NULL && strcmp(tmp, "sun") != 0)
	      tty_mode = TRUE;		/* should be run in TTY mode */

	    (void)check_arg(argc, argv);	/* forced tty mode? */

	    if (!tty_mode)
	    {
	      /* sundiag was invoked from console mode */
	      if ((i=open("/dev/fb", O_RDWR)) != -1)
	      {
	        (void)ioctl(i, FBIOGTYPE, &fb_type);

	        if (fb_type.fb_type == FBTYPE_SUN2BW)
	          if (ioctl(i, FBIOGATTR, &fb_gattr) == 0)
	            if (fb_gattr.real_type == FBTYPE_SUN4COLOR)
		      cgfour = 1;
		      /* this is cgfour0 */
	        (void)close(i);
	      }

	      (void)mktemp(tmpname);	/* make temporary file's name */

	      if (verbose)
	        (void)sprintf(cmd,"echo shelltool -Wp 200 200 csh -c \\\"%s -C",
								argv[0]);
	      else
		(void)sprintf(cmd,"echo \"%s -C", argv[0]);

	      for (i=1; i != argc; ++i)/* append the rest of the command line */
	        (void)sprintf(cmd, "%s %s", cmd, argv[i]);

	      if (verbose)
	        (void)sprintf(cmd, "%s\\\" > %s", cmd, tmpname);
	      else
		(void)sprintf(cmd, "%s\" > %s", cmd, tmpname);

	      (void)system(cmd);

	      if (cgfour)
	        execlp("suntools","suntools","-8bit_color_only","-s",tmpname,
								NULL);
	      else
	        execlp("suntools", "suntools", "-s", tmpname, NULL);

	      perror("execlp");
	      exit(1);	/* failed to bring up suntools automatically */
	    }
	  }
	  else
	  {
	    tty_mode = TRUE;		/* should be run in TTY mode */
	    (void)check_arg(argc, argv);/* options for TTY mode all checked */
	  }
	}

	if ((tmp=strrchr(argv[0], '/')) != NULL)
	/* running from other directories */
	{
	  *tmp = '\0';		/* NULL-terminated */
	  (void)chdir(argv[0]);	/* change current working directory */
	}
	else if (check_exe(argv[0]) == -1)	/* from command path */
	{
	  if ((tmp1=getenv("PATH")) != NULL)
	    if ((tmp=malloc((unsigned)strlen(tmp1)+2)) != NULL)
	    {
	      (void)strcpy(tmp, tmp1);
	      tmp1=tmp;		/* so we can free it later */
	      if (strtok(tmp, ": ,\n") != NULL)
	      {
	        do
	        {
		  (void)sprintf(cmd, "%s/%s", tmp, argv[0]);
		  if (check_exe(cmd) == 0)	/* find it */
		  {
		    (void)chdir(tmp);	/* change current working directory */
		    break;
		  }
	        } while ((tmp=strtok((char *)NULL, ": ,\n")) != NULL);
	      }
	      free(tmp1);
	    }
	}

	init_env();		/* initialize the global Sundiag environment */

	if (!tty_mode)
	{
   	  sundiag_frame = window_create((Frame)NULL, FRAME,
      	    FRAME_LABEL,		NULL,
      	    FRAME_ICON,			&sundiag_icon,
      	    WIN_X,			SUNDIAG_X,
      	    WIN_Y,			SUNDIAG_Y,
	    WIN_HEIGHT,			0,
	    WIN_WIDTH,			0,
      	    WIN_ERROR_MSG,    "sundiag: cannot create base sundiag frame",
	    FRAME_ARGS,			tmp_argc, tmp_argv,  /* defult font */
	    FRAME_ARGC_PTR_ARGV,	&argc, argv,	/* user's options */
      	    0);

	  (void)check_arg(argc, argv);
	}

	init_misc();		/* initialize misc. data */

	init_rpc();		/* start rpc server */

	if (probe_flag)
	  conf_probe();		/* probe the hardware configuration */
	else
	{
	  cpuname = "Unknown";	/* cpuname is always needed */
	  build_user_tests();	/* go checking for user-defined tests */
	}

	  if (config_file_only)
	  	exit(1);

	if (!ats_nohost)
	  init_client(hostname);

	if (option_fname[0] != NULL)
	  if (!load_option(option_fname, 0))
	    option_fname[0] = NULL;

	(void)notify_set_itimer_func((Notify_client)client_handle,
		notify_proc, ITIMER_REAL, &notify_timeout ,
		(struct itimerval *)0);
	(void)notify_set_signal_func((Notify_client)client_handle,
		kill_proc, SIGINT, NOTIFY_ASYNC);

	if (!tty_mode)
	{
	  	
	  if ((int)window_get(sundiag_frame, WIN_WIDTH) == 0)
	  /* if not set by command line */
	  {
	    (void)window_set(sundiag_frame,
	    WIN_HEIGHT,			frame_height-SUNDIAG_Y,
	    WIN_WIDTH,			frame_width-SUNDIAG_X,
	    0);
	  }

	  /* If FRAME_LABEL wasn't set by command line argument, set it */
	  if ((char *)window_get(sundiag_frame, FRAME_LABEL) == NULL)
	  {
	    if (remotehost == NULL || tty_mode)
	      (void)sprintf(cmd, " *** Sundiag %s ***  CPU: %s",
				version, cpuname);
	    else
	      (void)sprintf(cmd,
		" *** Sundiag %s ***  Remote Host: %s    CPU: %s",
				version, remotehost, cpuname);

   	    (void)window_set(sundiag_frame, FRAME_LABEL, cmd, 0);
	  }

		/* create pid file only if not in remote host mode */
	  if (remotehost == NULL) {
	    fp = fopen(SUNDIAG_PID, "w");	/* write out the PID */
	    (void)fprintf(fp, "%d\nSunView\n", getpid());
	    (void)fclose(fp);
	  }

	  create_status(sundiag_frame);
	  create_perfmon(sundiag_frame);
	  create_control(sundiag_frame);
	  create_console(sundiag_frame);

	  (void)notify_set_signal_func((Notify_client)client_handle,
		kill_proc, SIGHUP, NOTIFY_ASYNC);
	  (void)notify_interpose_destroy_func(sundiag_frame,stop_notifier_proc);

	  init_control(sundiag_control);
	  if (!no_perfmon) perfmon_main();/* initialize the perfmon subwindow */
	  test_items();     /* display test-related items on control window */
	  init_status();		/* create status message item */	
	}
	else
	{
	  switch ((i=fork()))
	  {
	    case -1:
		perror("fork background");
		break;
	    case 0:
		break;
	    default:
		execlp("./sundiagup", "./sundiagup", "sd", (char *)0);
		perror("execlp sundiagup");
		(void)kill(i, SIGKILL);
		exit(1);
	  }
	  tty_ppid = getppid();

		/* create pid file only if not in remote host mode */
	  if (remotehost == NULL) {
	    fp = fopen(SUNDIAG_PID, "w");	/* write out the PID */
	    (void)fprintf(fp, "%d\nTTY\n", getpid());
	    (void)fclose(fp);
	  }

	  (void)notify_set_signal_func((Notify_client)client_handle,
		kill_proc, SIGTERM, NOTIFY_ASYNC);
	  (void)notify_set_signal_func((Notify_client)client_handle,
		foreground_proc, SIGUSR1, NOTIFY_ASYNC);

	  test_items();			/* display the control screen */
	  init_ttys();			/* initialize the TTY screens */
	  print_status();		/* display the status screen */

	  (void)signal(SIGHUP, (void_func)kill_proc);	/* catch SIGHUP */
	  (void)notify_set_input_func((Notify_client)client_handle,
		handle_input, 0);	/* call "handle_input" when needed */
	  (void)notify_set_input_func((Notify_client)client_handle,
		message_input, pfd[0]);	/* call "message_input" when needed */
	  if (become_console)
	    (void)notify_set_input_func((Notify_client)client_handle,
		console_input, pty_fd);	/* call "console_input" when needed */
	}

	create_config_dir();			/* create the config dir if it does not exist */

	if (auto_start) start_proc();		/* automatically start testing */
	if (batch_mode) batch();	/* start batch testing */

	c_tv.tv_usec = 0;
	c_tv.tv_sec = 30;
	notify_set_signal_check(c_tv);
	/* -JCH- Utilize notifier hack to avoid stopped clock bug */

	if (!tty_mode)
	  window_main_loop(sundiag_frame);	/* start SunView */
	else
	  while (TRUE)
	    (void)notify_start();	/* start notifier */
}

static create_status(sundiag_frame)
Frame	sundiag_frame;
{
	status_bar = scrollbar_create(0);

	sundiag_status = window_create(sundiag_frame, PANEL,
	    WIN_VERTICAL_SCROLLBAR,	status_bar,
	    WIN_WIDTH,		(int)(frame_width*STATUS_WIDTH),
	    WIN_HEIGHT,		(int)(frame_height*PERFMON_HEIGHT),
	    WIN_IGNORE_KBD_EVENT,	WIN_NO_EVENTS,
	    0);
}

/******************************************************************************
 * check_arg(), checks the command line arguments.			      *
 * Input: argc, argv.							      *
 * Output: exit if errors were encountered.				      *
 ******************************************************************************/
static	void	check_arg(argc, argv)
int	argc;
char	*argv[];
{
  int	i;
  char	c, *tmp;

	/* check command line options */
	i = 0;				/* initialize the error flag */
	while ((c=getopt(argc, argv, "Cmtpqvwo:b:h:a:k:")) != EOF)
	{
	  switch(c)
	  {
	    case 'a':
		hostname = optarg;
		break;
	    case 'C':
		become_console = 1;
		break;
	    case 'h':			/* remote host */
		remotehost = optarg;
		break;
	    case 'm':
		makedevice = 1;
		break;
	    case 'o':
		(void)strcpy(option_fname, optarg);
		break;
	    case 't':
		tty_mode = TRUE;	/* forced into TTY mode */
		break;
	    case 'p':
		probe_flag = FALSE;	/* don't probe the kernel */
		break;
	    case 'q':
		auto_quit = TRUE;	/* auto quit when stopped */
		break;
	    case 'v':
		verbose = FALSE;
		/* don't print any message before sundiag screen is up */
		break;
	    case 'k':
		tmp = malloc((unsigned)strlen(optarg)+20);
		sprintf(tmp, "KERNELNAME=%s", optarg);
		putenv(tmp);
		if (strcmp(optarg, "/vmunix") != 0) no_perfmon = TRUE;
		break;
	    case '?':			/* illegal command line argument */
		i = 1;
		break;
	    case 'b':
		if ( (batch_fp = conf_open(optarg)) == (FILE *)0 )
			i = 1;
		else
			batch_mode = TRUE;	/* batch testing mode */
		break;
	    case 'w':
		config_file_only = TRUE;       /* write sundiag.conf file */
		break;
	  }
	}

	if (i != 0)
	{
	    (void)fprintf(stderr,
		"Usage: sundiag [-Cmpqtvw][-a|-h hostname][-o option_file][-b batch_file][-k kernel_name]\n");
	    exit(1);
	}
	else if (hostname == NULL)
	    ats_nohost = TRUE;
}

/******************************************************************************
 * check_exe(), checks whether the file is executable or not.		      *
 * Input: filename, path of the file to be check.			      *
 * Output; 0, if yes; -1, otherweise.					      *
 ******************************************************************************/
static	int check_exe(filename)
char	*filename;
{
    struct stat f_stat;

    if ((stat(filename, &f_stat) == -1) || (f_stat.st_size == 0))
        return(-1);

    if ((f_stat.st_mode & S_IFMT) != S_IFREG ||
        (f_stat.st_mode & S_IEXEC) != S_IEXEC)
        return(-1);

    return(0);
}

/******************************************************************************
 * Creates and initializes the console subwindow.			      *
 ******************************************************************************/
static create_console(sundiag_frame)
Frame	sundiag_frame;
{
	sundiag_console = window_create(sundiag_frame, TTY,
	    WIN_BELOW,		sundiag_status,
	    WIN_WIDTH,		(int)(frame_width*STATUS_WIDTH+
					frame_width*PERFMON_WIDTH)+5,
	    0);

	tty_fd = null_fd;

	if (become_console)
	  (void)window_set(sundiag_console, TTY_CONSOLE, TRUE, 0);
}

static create_perfmon(sundiag_frame)
Frame	sundiag_frame;
{
	sundiag_perfmon = window_create(sundiag_frame, CANVAS,
	    WIN_WIDTH,		(int)(frame_width*PERFMON_WIDTH),
	    WIN_HEIGHT,		(int)(frame_height*PERFMON_HEIGHT),
	    CANVAS_FAST_MONO,	TRUE,
	    0);
}

static create_control(sundiag_frame)
Frame	sundiag_frame;
{
	control_bar = scrollbar_create(
	    SCROLL_PLACEMENT,	SCROLL_EAST,
	    0);

	sundiag_control = window_create(sundiag_frame, PANEL,
	    WIN_VERTICAL_SCROLLBAR,	control_bar,
	    0);
}

/******************************************************************************
 * Initializes misc. variables and data structures.			      *
 ******************************************************************************/
static init_misc()
{
  char	*path, *tmp;
  char  *file_suffix = "";
  int	len, i, rty_fd, dummy=0;
  FILE	*version_fp;
  char	version_tmp[50];
  static char	version_buf[22];
  int memfd;

    /* get number of processors and processsors mask */
    memfd = open("/dev/null", 0);
    processors_mask = get_processors_mask(memfd, processors_mask);
    system_processors_mask = processors_mask;
    number_processors = get_number_processors(processors_mask);
    number_system_processors = number_processors;
    close(memfd);

	if ((null_fd=open("/dev/null", O_WRONLY)) < 0)
	{
	  perror("/dev/null");
	  exit(1);
	}
	tty_fd = null_fd;

	if (!tty_mode)
	{
	  if (verbose)
	    tty_fd = 2;		/* stderr untill console window is up */

	  sundiag_font = pf_default();	/* keep a copy of the font */
	  if (default_x != 0)		/* the default font was opened */
	  {
	    if (sundiag_font->pf_defaultsize.x != default_x)
	      frame_width=(frame_width*sundiag_font->pf_defaultsize.x)/
								default_x;
	    if (sundiag_font->pf_defaultsize.y != default_y)
	      frame_height = (frame_height*sundiag_font->pf_defaultsize.y)/
								default_y;
	  }
	}
	else				/* open the message pipe */
	{
	  if (pipe(pfd) == -1)
	  {
	    perror("pipe");
	    exit(1);
	  }

	  if (become_console)
	  {
	    for( i = 0; i < NPTYS; i++)		/* find an open pty */
		if ((pty_fd = open(ptynames[i], O_RDWR)) >= 0)
		  break;
	    if (i == NPTYS)
	    {
		perror("init_misc: can't get a pty");
		exit(1);
	    }
	    if ((rty_fd = open(ttynames[i], O_RDWR)) < 0)  /* open the slave */
	    {
		perror("init_misc: can't open slave tty"); 
		exit(1);
	    }

	    if (ioctl(rty_fd, TIOCCONS, &dummy) != 0) /* redirect the console */
	    {
		perror("init_misc: can't redirect the console");
		exit(1);
	    }
	  }

	  init_tty_windows();
	}

	path = malloc((unsigned)(strlen(LOG_DIR)+strlen(INFO_FILE)+
				strlen(ERROR_FILE)+ strlen(CONF_FILE)+2));

	if (access(LOG_DIR, F_OK) != 0)
	  if (mkdir(LOG_DIR, 0755) != 0)
	  {
	    (void)sprintf(path, "mkdir %s", LOG_DIR);
	    perror(path);
	    exit(1);
	  }
	if (remotehost != NULL) {
		file_suffix = malloc((unsigned)strlen(remotehost)+2);
		(void)sprintf(file_suffix, ".%s", remotehost);
	}
	(void)sprintf(path, "%s/%s%s", LOG_DIR, INFO_FILE, file_suffix);
	if ((info_fp=fopen(path, "a")) == NULL)
	{
	  perror("Open sundiag.info");
	  exit(1);
	}

	info_file = malloc((unsigned)strlen(path)+2);
	(void)strcpy(info_file, path);	/* keep a copy of it */

	(void)sprintf(path, "%s/%s%s", LOG_DIR, ERROR_FILE, file_suffix);
	if ((error_fp=fopen(path, "a")) == NULL)
	{
	  perror("Open sundiag.err");
	  exit(1);
	}

	error_file = malloc((unsigned)strlen(path)+2);
	(void)strcpy(error_file, path);	/* keep a copy of it */

	(void)sprintf(path, "%s/%s%s", LOG_DIR, CONF_FILE, file_suffix);
	conf_file = malloc((unsigned)strlen(path)+2);
	(void)strcpy(conf_file, path);

	free(path);

	if ((tmp=getenv("PRINTER")) != NULL)
	{
	  (void)strncpy(printer_name, tmp, 50);
	  printer_name[50] = '\0';	/* to be safe */
	}
	else
	  (void)strcpy(printer_name, "lp");	/* default printer name */

	if ((version_fp=fopen(VERSION_FILE, "r")) != NULL)
	{
	  version_buf[0] = '\0';
	  version = fgets(version_buf, 20, version_fp);
	  len = strlen(version) - 1;
	  if (len >= 0 && *(version+len) == '\n')
	    *(version+len) = '\0';
	  fclose(version_fp);
	}

        strncpy(version_buf, version, 4);
	subtest_id = get_version_id(version_buf);
	sprintf(version_tmp, start_sundiag_info, version);
	logmsg(version_tmp, 1, START_SUNDIAG_INFO);
						/* *** Start sundiag *** */
}

/******************************************************************************
 * initialize rpc server and fork probing routine.			      *
 ******************************************************************************/
static	int	probe_object;
static	int	*probe_handle = &probe_object;

static	conf_probe()
{
  int	childpid;
  char	myhostname[MAXHOSTNAMELEN];

	childpid = vfork();	/* fork the probing routine */

	if (childpid == 0)	/* in child process */
	{
	  if (remotehost == NULL) {
	    if (makedevice)
              execlp("./probe", "./probe", "-m", 0);
		/* start the probing routine, make the device file if missing */
	    else
              execlp("./probe", "./probe", 0);	/* start the probing routine */
	  } else {
	    if ((gethostname(myhostname, MAXHOSTNAMELEN - 1)) == -1) {
		(void)fprintf(stderr, "Can't get my hostname\n");
		_exit(1);
	    }
	    if (makedevice)
	      execlp("remote", "remote", remotehost, "probe", "-m", myhostname,
		     0);
	    else
	      execlp("remote", "remote", remotehost, "probe", "-h", myhostname,
		     0);
	  }

	  perror("execlp(probe)");
	  _exit(-1);
    	}

	(void)notify_set_wait3_func((Notify_client)probe_handle,
						wait_probe, childpid);

	if (verbose)
	  (void)fprintf(stderr,
		"\nSundiag: Starting probing routine, please wait...\n");

	while (not_done)	/* wait until done, no time out yet ! */
	  (void)notify_dispatch();
}

/******************************************************************************
 * Initializes the global Sundiag environment.				      *
 ******************************************************************************/
static	init_env()
{
  int	tmpfd;
  struct fbtype	fb_type;

	frame_width = 1152;
	frame_height = 900;		/* default dimensions */
	tmp_argv[1] = STANDARD_FONT;	/* default font */
	
	if (!tty_mode)
	{
	  if ((tmpfd=open("/dev/fb", O_RDWR)) != -1)
	  {
	    (void)ioctl(tmpfd, FBIOGTYPE, &fb_type);
	    frame_width = fb_type.fb_width;
	    frame_height = fb_type.fb_height;

	    if (frame_width > 1500)
	      tmp_argv[1] = BOLD_FONT;	/* must be on hi-res monitor */

	    (void)close(tmpfd);
	  }

	  sundiag_font = pf_open(tmp_argv[1]);
	  if (sundiag_font == NULL)
	  {
	    default_x = 0;		/* use it as a flag */
	    default_y = 0;
	  }
	  else
	  {
	    default_x = sundiag_font->pf_defaultsize.x;
	    default_y = sundiag_font->pf_defaultsize.y;
	    /* get the size of the default font */
	    (void)pf_close(sundiag_font);
	  }
	}

	if (option_fname[0] == NULL)	/* if no specified option file yet */
	{
	  /* check if .sundiag exists */
	  (void)sprintf(option_fname, "%s/%s", OPTION_DIR, OPT_FILE);
	  if (access(option_fname, R_OK) == 0)
	    (void)strcpy(option_fname, OPT_FILE);
	  else	
	    option_fname[0]=NULL;
	}
}

/******************************************************************************
 * wait_probe() is the notify procedure when probing routine exits.	      *
 ******************************************************************************/
/*ARGSUSED*/
static	Notify_value wait_probe(me, pid, status, rusage)
int *me;
int pid;
union wait *status;
struct rusage *rusage;
{
  if (exist_tests == 0)
  {
    (void)fprintf(stderr, "Sundiag: failed probing system configuration!\n");

    if (remotehost == NULL)
      (void)unlink(SUNDIAG_PID);
    exit(1);
  }

  not_done = 0;

  return(NOTIFY_DONE);
}

/******************************************************************************
 * sundiag_exit() is to be called to exit Sundiag abnormally.		      *
 ******************************************************************************/
/*ARGSUSED*/
sundiag_exit(code)
int	code;
{
	  (void)notify_stop();
	  sundiag_done = 1;
	  logmsg(quit_abnorm_info, 1, QUIT_ABNORM_INFO);

	  if (running == GO || running == SUSPEND || running == KILL)
	  {
	    kill_all_tests();
	    log_status();
	  }

	  write_log("\n\n", info_fp);		/* delimiter */
	  write_log("\n\n", error_fp);		/* delimiter */

	  if (tty_mode)
	  {
	    if (tty_ppid != 0)
	      (void)kill(tty_ppid, SIGTERM);
	      restore_term_tty_state();         /* clean up curses remnates */
	  }

    	  if (remotehost == NULL)
	    (void)unlink(SUNDIAG_PID);
	  exit(code);
}

/******************************************************************************
 * The SIGHUP and SIGINT signal handlers(set by notify_set_signal_func()).    *
 ******************************************************************************/
Notify_value	kill_proc()
{
	(void)notify_stop();
	sundiag_done = 1;
	logmsg(quit_sundiag_info, 1, QUIT_SUNDIAG_INFO);

	if (running == GO || running == SUSPEND || running == KILL)
	{
	  kill_all_tests();
	  log_status();
	}

	write_log("\n\n", info_fp);	/* delimiter */
	write_log("\n\n", error_fp);	/* delimiter */

	if (tty_mode)
	{
	  term_tty();		/* clean up curses routines */
	  if (tty_ppid != 0)
	    (void)kill(tty_ppid, SIGTERM);
	}

    	if (remotehost == NULL)
	  (void)unlink(SUNDIAG_PID);
	exit(0);
}

/******************************************************************************
 * The "notify_interpose_destroy_func()" procedure for the Sundiag frame.     *
 * Its purpose is to clean up the mess before exits.			      *
 ******************************************************************************/
static Notify_value	stop_notifier_proc(frame, status)
Frame		frame;
Destroy_status	status;
{
	if (status != DESTROY_CHECKING)
	{
	  (void)notify_stop();
	  sundiag_done = 1;
	  logmsg(quit_sundiag_info, 1, QUIT_SUNDIAG_INFO);

	  if (running == GO || running == SUSPEND || running == KILL)
	  {
	    kill_all_tests();
	    log_status();
	  }

	  write_log("\n\n", info_fp);		/* delimiter */
	  write_log("\n\n", error_fp);		/* delimiter */
	}

    	if (remotehost == NULL)
	  (void)unlink(SUNDIAG_PID);
	return(notify_next_destroy_func(frame, status));
}

/******************************************************************************
 * notify_proc(), the periodic timer function(called once every second) to    *
 * handle backgroud tasks.						      *
 ******************************************************************************/
static	Notify_value	notify_proc()
{
  long		current_time;
  static long	previous_time=0;
  static int	rpc_check=0;
  static int	heart_count=0;
  static int	elapse_secs=0;
  static unsigned  last_log=0;

	current_time = (long)time((time_t *)0);  /* get current time of the day */
	if ( batch_mode ) {
		if ( previous_time == 0 ) /* must be the initial pass */
			elapse_secs = 1;
		else 			    /* get the elapse seconds */
			elapse_secs = current_time - previous_time;
		previous_time = current_time; /* save the current time */
	}
	if (sundiag_done)
		return(NOTIFY_DONE);	/* quiting... */

	if (running == GO)
	{
	  elapse_count += current_time - last_elapse; /* increment the elapsed seconds */
	  last_elapse = current_time;
	  if (log_period != 0)	/* if periodical status log required */
	  {
	    if (last_log > elapse_count) last_log = elapse_count;
	    if ((elapse_count - last_log) >= (log_period*60))  /* time to log */
	    {
	      last_log += (log_period*60);
	      logmsg(period_log_info, 0, PERIOD_LOG_INFO);
	      log_status();
	      if (send_email==2 || send_email==3)
		sd_send_mail();
	    }
	  }
	  else
	  {
		last_log = elapse_count-1;
	  }

	  (void)time_display();	/* display elapse time on status sunwindow */
	  (void)invoke_tests();	/* periodically start enabled tests */
	}
	else if (running == KILL) {
		(void)check_term();/* check whether all tests are terminated */
		if (auto_quit && running == IDLE)    /* all tests are killed */
		  kill_proc();			     /* quit from here */
		if (batch_mode && running == IDLE)
		{
			if (batch_ready) {	/* time to load next batch entry */
				heart_count = 0;
				batch_ready = FALSE;
				batch();
			}
		}
	}

	if ( schedule_file == TRUE )
		sched_proc(elapse_count, current_time);
	if (!tty_mode && !no_perfmon)
	  perfmon_update();

	if ( (ats_nohost == 0) || batch_mode )
		heart_count += elapse_secs;
	if (!ats_nohost)
	{
	  if (++rpc_check == 10)
	  {
	    rpc_check = 0;
	    amp_elapsed_time();	/* check the message queue for un-ack'ed msg */
	  }
	  if (heart_count >= 60)
	  {
	    heart_count = 0;
	    send_pong();	/* send SUNDIAG_ATS_PONG to ATS */
	  }
	}
	if (batch_mode)
	{
	  if (heart_count >= 60)
	  {
	    batch_pong(heart_count/60);	/* batch timer */
	    heart_count = 0;
	  }
	}

	return(NOTIFY_DONE);
}

/******************************************************************************
 * set_input_notify, to be called to register input-pending event.	      *
 ******************************************************************************/
set_input_notify()
{
	  (void)notify_set_input_func((Notify_client)client_handle,
		handle_input, 0);	/* call "handle_input" when needed */
}

/******************************************************************************
 * unset_input_notify, to be called to unregister input-pending event.	      *
 ******************************************************************************/
unset_input_notify()
{
	(void)notify_set_input_func((Notify_client)client_handle,
		NOTIFY_FUNC_NULL, 0);
}

/******************************************************************************
 * check_sundiag(), check whether sundiag is already running.		      *
 * return: TRUE, if there is already a copy of Sundiag running.		      *
 *	   FALSE, if no Sundiag or only copies of remote Sundiag running.     * 
 ******************************************************************************/
static int check_sundiag()
{
  int	pid;
  FILE	*fp;
  char	tmp[22];

    fp = fopen(SUNDIAG_PID, "r");

    if (fp == (FILE *)0) return(FALSE);
    else
    {
	pid = atoi(fgets(tmp, 20, fp));
	(void)fclose(fp);

	if (kill(pid, 0) == 0) return(TRUE);	/* if it is a valid pid */

	(void)unlink(SUNDIAG_PID);		/* remove it for safty sake */
	return(FALSE);
    }
}

/******************************************************************************
 * sd_send_mail() sends the INFO. Sundiag message file to the specified user( *
 * defaulted to be "root"). 						      *
 ******************************************************************************/
void	sd_send_mail()
{
  char	buff[162];
  int	pid;

  (void)sprintf(buff,"tail -60 /usr/adm/sundiaglog/sundiag.info | /usr/ucb/mail -s \"Sundiag status\" %s", eaddress);

  if ((pid=vfork()) == 0)		/* child */
  {
    (void)execl("/bin/csh", "csh", "-c", buff, (char *)0);
    _exit(127);
  }

  if (pid != -1)			/* succeeded */
    (void)notify_set_wait3_func((Notify_client)client_handle,
			notify_default_wait3, pid);
}
