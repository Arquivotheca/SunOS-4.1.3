#ifndef lint

#endif

/*
 *	Copyright (c) 1985, 1986, 1987, 1988 by Sun Microsystems Inc.
 */

/* 
 *	perfmeter [-v name] [-s sample] [-m minute] [-h hour]
 *		[ -M value minmax maxmax ] [ host ]
 *
 *		displays meter for value 'name' sampling
 *		every 'sample' seconds.  The minute hand is an
 *		average over 'minute' seconds, the hour hand over
 *		'hour' seconds
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/dk.h>
#include <sys/vmmeter.h>
#include <sys/wait.h>
#include <suntool/sunview.h>
#include <sunwindow/rect.h>
#include <sunwindow/pixwin.h>
#include <suntool/wmgr.h>
#include "meter.h"

#define	MAXINT		0x7fffffff
#define	TRIES		8	/* number of bad rstat's before giving up */
#define TOP_KEY         KEY_LEFT(5)
#define OPEN_KEY        KEY_LEFT(7)
#define max_of(x,y)	MAX((x),(y))
#define min_of(x,y)	MIN((x),(y))

struct timeval TIMEOUT = {20,0};

struct	meter meters_init[] = {
	/* name		maxmax	minmax	curmax	scale	lastval */
	{ "cpu",	100,	100,	100,	1,	-1 },
#define	CPU	0
	{ "pkts",	MAXINT,	32,	32,	1,	-1 },
#define	PKTS	1
	{ "page",	MAXINT,	16,	16,	1,	-1 },
#define	PAGE	2
	{ "swap",	MAXINT,	4,	4,	1,	-1 },
#define	SWAP	3
	{ "intr",	MAXINT,	100,	100,	1,	-1 },
#define	INTR	4
	{ "disk",	MAXINT,	40,	40,	1,	-1 },
#define	DISK	5
	{ "cntxt",	MAXINT,	64,	64,	1,	-1 },
#define	CNTXT	6
	{ "load",	MAXINT,	4,	4,	FSCALE,	-1 },
#define	LOAD	7
	{ "colls",	MAXINT,	4,	4,	FSCALE,	-1 },
#define	COLL	8
	{ "errs",	MAXINT,	4,	4,	FSCALE,	-1 },
#define	ERR	9
};
#define	MAXMETERS	sizeof (meters_init) / sizeof (meters_init[0])

struct	meter *meters;	/* [MAXMETERS] */

int	*getdata_swtch();      /* version 3 (RSTATVERS_SWTCH) of RSTATPROG  */
int	*getdata_var();        /* version 4 (RSTATVERS_VAR) */
int	killkids();
int	meter_selected();
char 	*calloc();
static caddr_t	do_menu();
void	wmgr_changerect();
static	Notify_value	meter_event();
static	Notify_value	meter_itimer_expired();

caddr_t window_get();
Frame	frame;
struct	pixfont *pfont;
struct	pixrect *ic_mpr;
Icon 	metericon;		/* Defaults to all zeros */
Pixwin	*pw;
static	Menu	menu;	/* walking menus */
static	Rect	ic_gfxrect;
struct pixrect	*ic_mpr;

int	(*old_selected)();
int	wantredisplay;		/* flag set by interrupt handler */
int	*save;			/* saved values for redisplay; dynamically
				 * allocated to be [MAXMETERS][MAXSAVE] */
int	saveptr;		/* where I am in save+(visible*MAXMETERS) */
int	dead;			/* is remote machine dead? */
int 	sick;			/* is remote machine sick? (sending bogus data) */
int	visible;		/* which quantity is visible*/
int	length = sizeof (meters_init) / sizeof (meters_init[0]);
int	remote;			/* is meter remote? */
int	rootfd;			/* file descriptor for the root window */
int	width;			/* current width of graph area */
int	height;			/* current height of graph area */
char	*hostname;		/* name of host being metered */
int	sampletime;		/* sample seconds */
int	minutehandintv;		/* average this second interval */
int	hourhandintv;		/* long average over this seconds */
int	shortexp, longexp;
int	designee;
int	meter_client;

static int oldsocket;
#define VER_NONE -1
static int vers = VER_NONE;

#ifdef STANDALONE
main(argc, argv)
#else
perfmeter_main(argc, argv)
#endif STANDALONE
	int argc;
	char **argv;
{
	register int	i, j;
	register struct meter *mp;
	char 		*cmdname;
	char 		**av;
        int             pid;

	/* Dynamically allocate save */
	save = (int *)(calloc(1, sizeof (int)*MAXMETERS*MAXSAVE));
	/* Dynamically allocate and initialize meters */
	meters = (struct meter *)(calloc(1, sizeof (meters_init)));
	for (i = 0; i < MAXMETERS; i++) meters[i] = meters_init[i];
	sampletime = 2;
	minutehandintv = 2;
	hourhandintv = 20;
	oldsocket = -1;
	
	/*
	 * Create tool window and customize
	 */
	cmdname = argv[0];
	argc--;   argv++;		/*	skip over program name	*/
	frame = window_create(0, FRAME,
	    FRAME_SHOW_LABEL,		FALSE,
	    WIN_WIDTH,			ICONWIDTH,
	    WIN_HEIGHT,			6, /* not really, check later */
	    WIN_ERROR_MSG,              "Unable to create frame\n", 
	    FRAME_ARGC_PTR_ARGV,	&argc, argv,
	    WIN_CONSUME_KBD_EVENT,	WIN_ASCII_EVENTS,
	    0);
	    
	if (frame == (Frame)NULL) {
	    exit(1);
	}
	
        (void) notify_interpose_event_func((Notify_client)frame,
                        meter_event, NOTIFY_SAFE);

	while (argc > 0) {
		if (argv[0][0] == '-') {
			if (argv[0][2] != '\0')
				goto toolarg;
			switch (argv[0][1]) {
			case 's':
				if (argc < 2)
					usage(cmdname);
				sampletime = atoi(argv[1]);
				break;
			case 'h':
				if (argc < 2)
					usage(cmdname);
				hourhandintv = atoi(argv[1]);
				break;
			case 'm':
				if (argc < 2)
					usage(cmdname);
				minutehandintv = atoi(argv[1]);
				break;
					
			case 'v':
				if (argc < 2)
					usage(cmdname);
				for (i = 0, mp = meters; i < length; i++, mp++)
					if (strcmp(argv[1], mp->m_name) == 0)
						break;
				if (i >= length)
					usage(cmdname);
				visible = i;
				break;
			case 'M':
				if (argc < 4)
					usage(cmdname);
				for (i = 0, mp = meters; i < length; i++, mp++)
					if (strcmp(argv[1], mp->m_name) == 0)
						break;
				if (i >= length)
					usage(cmdname);
				visible = i;
				mp->m_curmax = mp->m_minmax = atoi(argv[2]);
				mp->m_maxmax = atoi(argv[3]);
				argc -= 2;
				argv += 2;
				break;
			default:
			toolarg:
				/*
				 * Pick up generic tool arguments.
				 */
/*				if ((i = 
					tool_parse_one(cmdname, argc, argv))
						 == -1) {
					(void)tool_usage(cmdname);
					exit(1);
				} else if (i == 0)
					usage(cmdname);
*/
				argc -= i;
				argv += i;
				continue;
			}
			argc--;
			argv++;
		} else {
			if (hostname != NULL)
				usage(cmdname);
			hostname = argv[0];
			remote = 1;
		}
		argc--;
		argv++;
	}

	if (sampletime <= 0 || hourhandintv < 0 || minutehandintv < 0)
		usage(cmdname);
	shortexp = (1 - ((double)sampletime/max_of(hourhandintv, sampletime))) *
	    FSCALE;
	longexp = (1 - ((double)sampletime/max_of(minutehandintv, sampletime))) *
	    FSCALE;

	/*
	 * Set up meter icon.
	 */
	ic_gfxrect.r_width = ICONWIDTH;
	ic_gfxrect.r_height = remote ? RICONHEIGHT : ICONHEIGHT;
	ic_mpr = mem_create(ICONWIDTH, remote ? RICONHEIGHT : ICONHEIGHT, 1);
	metericon = icon_create(
			ICON_WIDTH, 	ICONWIDTH,
			ICON_HEIGHT,	remote ? RICONHEIGHT : ICONHEIGHT,
			ICON_IMAGE_RECT,&ic_gfxrect,
			0);	
        if ((int)window_get(frame, WIN_HEIGHT) == 6) { 
            window_set(frame, 
        	FRAME_ICON,	metericon,
        	WIN_HEIGHT,	remote ? RICONHEIGHT : ICONHEIGHT,
        	0);
        } else 
            window_set(frame, 
        	FRAME_ICON,	metericon,
        	0);
        icon_destroy(metericon);
        metericon = window_get(frame, FRAME_ICON);
        (void)icon_set(metericon, ICON_IMAGE, ic_mpr, 0);
	/*
	 * Open font for icon label.
	 */
	pfont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
	if (pfont == NULL) {
		(void)fprintf(stderr, "%s: can't find screen.r.7\n", cmdname);
		exit(1);
	}

	pw = (Pixwin *) window_get(frame, WIN_PIXWIN);

	/*
	 * Setup timer.
	 */
	if (setup() < 0) {
		dead = 1;
		sick = 0;
		keeptrying();
	}

	/* 
	 * Get first set of data, then initialize arrays.
	 */
	updatedata();
	for (i = 0; i < length; i++)
		for (j = 1; j < MAXSAVE; j++)
			save_access(i, j) = -1;

	/*
	 * Initialize menu.
	 */
	av = (char **)(
		malloc((unsigned)(3*length*sizeof(char*) + sizeof(char *))));
	for (i = 0; i < 3*length; i+=3) {
		av[i] = (char *)MENU_STRING_ITEM;
		av[i+1] = meters[i/3].m_name;
		av[i+2] = (char *)(i/3);
	}
	av[i] = 0;
	menu = menu_create(ATTR_LIST, av,
				MENU_ACTION_PROC, do_menu,
			    	0);
	(void)menu_set(menu, MENU_INSERT, 0,
		menu_create_item(MENU_PULLRIGHT_ITEM, "frame", 
			window_get(frame, WIN_MENU),  0), 
		0);
	window_set(frame, WIN_MENU, menu, 0);

        pid = getpid();
        (void)setpgrp(pid, pid);
	(void)signal(SIGTERM, killkids);
	(void)signal(SIGINT, killkids);

	/* Set itimer event handler */
	(void) meter_itimer_expired((Notify_client) &meter_client,
		ITIMER_REAL);

	window_main_loop(frame);
	while (wantredisplay) {
		wantredisplay = 0;
		meter_paint();
		notify_start();
	}
	/*
	 * Cleanup
	 */
	(void)pf_close(pfont);
	(void)pr_destroy(ic_mpr);
	killkids();
	exit(0);
}

usage(name)
	char *name;
{
	register int i;
	
	(void)fprintf(stderr, "Usage: %s [-m minutehandintv] [-h hourhandintv] \
[-s sampletime] [-v value] [-M value minmax maxmax] [hostname]\n", name);
	(void)fprintf(stderr, "value is one of: ");
	for (i = 0; i < length; i++)
		(void)fprintf(stderr, " %s", meters[i].m_name);
	(void)fprintf(stderr,"\n");
	exit(1);
}

/*
 * New selection handler for perfmeter tool window.
 */
/*ARGSUSED*/
static
Notify_value
meter_event(object, event, arg, type)
	Window		object;
	Event			*event;
	Notify_arg		arg;
	Notify_event_type	type;
{
	int		n;
	Notify_value	value;

	value = notify_next_event_func((Notify_client) (object),
			(Notify_event)(event), arg, type);

	switch (event->ie_code) {

	case WIN_RESIZE: {
		Rect	rect;
		
		(void)win_getsize(window_get(object, WIN_FD), &rect);
		width = rect.r_width - 2*BORDER;
		height = rect.r_height - 2*BORDER - NAMEHEIGHT - 1;
		if (remote)
			height -= NAMEHEIGHT;
		break;
	}

/*
	case MENU_BUT: {
		int		n;

		n = (int)menu_show(window_get(object, WIN_MENU),
					object, event, 0);
		if (menu_get(window_get(object, WIN_MENU), 
					MENU_VALID_RESULT)) {
			visible = n;
			meter_paint();
		}
		event->ie_code = 0;
		break;
	}
*/

	case WIN_REPAINT:
		meter_paint();
		break;
	case '1': 
		sampletime = 1;
		break;
	case '2': 
		sampletime = 2;
		break;
	case '3': 
		sampletime = 3;
		break;
	case '4': 
		sampletime = 4;
		break;
	case '5': 
		sampletime = 5;
		break;
	case '6': 
		sampletime = 6;
		break;
	case '7': 
		sampletime = 7;
		break;
	case '8': 
		sampletime = 8;
		break;
	case '9': 
		sampletime = 9;
		break;
	case 'h':	/* hour hand */
		if (hourhandintv > 0)
			hourhandintv--;
		break;
	case 'H':	/* hour hand */
		hourhandintv++;
		break;
	case 'm':	/* minute hand */
		if (minutehandintv > 0)
			minutehandintv--;
		break;
	case 'M':	/* minute hand */
		minutehandintv++;
		break;
	}
	shortexp = (1 - ((double)sampletime/
	    max_of(hourhandintv, sampletime))) * FSCALE;
	longexp = (1 - ((double)sampletime/
	    max_of(minutehandintv, sampletime))) * FSCALE;
	return(value);
}

/*
 * SIGCHLD signal catcher.
 * Harvest any child (from keeptrying).
 * If can now contact host, request redisplay.
 */
ondeath()
{
	union wait status;

	while (wait3(&status, WNOHANG, (struct rusage *)0) > 0)
		;
	if (setup() < 0)
		keeptrying();
	else {
		dead = 0;
		/*
		 * Can't do redisplay from interrupt level
		 * so set flag and do a tool_done.
		 */
		wantredisplay = 1;
		notify_stop();
	}
}

/*
 * Convert raw data into properly scaled and averaged data
 * and save it for later redisplay.
 */
updatedata()
{
	register int i, *dp, old, tmp;
	register struct meter *mp;
	
	if (dead)
		return;
	if (vers == RSTATVERS_VAR)
		dp = getdata_var();
	else
		dp = getdata_swtch();
	if (dp == NULL) {
		dead = 1;
		sick = 0;
		meter_paint();
		keeptrying();
		return;
	}

	/* 
	 * Don't have to worry about save[old] being -1 the
         * very first time thru, because we are called
	 * before save is initialized to -1.
	 */
	old = saveptr;
	if (++saveptr == MAXSAVE)
		saveptr = 0;
	for (i = 0, mp = meters; i < length; i++, mp++, dp++) {
		if (*dp < 0)	/* should print out warning if this happens */
			*dp = 0;
		tmp = (longexp * save_access(i, old) +
		    (*dp * FSCALE)/mp->m_scale * (FSCALE - longexp)) >> FSHIFT;
		if (tmp < 0)	/* check for wraparound */
			tmp = mp->m_curmax * FSCALE;
		save_access(i, saveptr) = tmp;
		tmp = (shortexp * mp->m_longave +
		    (*dp * FSCALE/mp->m_scale) * (FSCALE - shortexp)) >> FSHIFT;
		if (tmp < 0)	/* check for wraparound */
			tmp = mp->m_curmax * FSCALE;
		mp->m_longave = tmp;
	}
}

getminutedata()
{
	
	return (save_access(visible, saveptr));
}

gethourdata()
{
	
	return (meters[visible].m_longave);
}

/*
 * Initialize the connection to the metered host.
 */

static	CLIENT *client, *oldclient;


static int
setup()
{
	struct timeval timeout;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct sockaddr_in serveradr;
	int snum;

	snum = RPC_ANYSOCK;
	bzero((char *)&serveradr, sizeof (serveradr));
	if (hostname) {
		if ((hp = gethostbyname(hostname)) == NULL) {
			(void)fprintf(stderr,
			    "Sorry, host %s not in hosts database\n",
			     hostname);
			exit(1);
		}
		bcopy(hp->h_addr, (char *)&serveradr.sin_addr, hp->h_length);
	}
	else {
		if (hp = gethostbyname("localhost"))
			bcopy(hp->h_addr, (char *)&serveradr.sin_addr,
			    hp->h_length);
		else
			serveradr.sin_addr.s_addr = inet_addr("127.0.0.1");
	}
	serveradr.sin_family = AF_INET;
	serveradr.sin_port = 0;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if ((vers == VER_NONE) || (vers == RSTATVERS_VAR)) {
		if ((client = clntudp_bufcreate(&serveradr, RSTATPROG,
				RSTATVERS_VAR, timeout, &snum,
				sizeof(struct rpc_msg), UDPMSGSIZE)) == NULL)
			return (-1);
	
		clnt_stat = clnt_call(client, NULLPROC, xdr_void, 0,xdr_void,
					0, TIMEOUT);
		if (clnt_stat == RPC_SUCCESS) {
			vers = RSTATVERS_VAR;
		} else if (clnt_stat != RPC_PROGVERSMISMATCH) {
			clnt_destroy(client);
			return(-1);
		} else {		/* version mismatch */
			clnt_destroy(client);
			vers = RSTATVERS_SWTCH; 
		}
	}
	if ((vers == VER_NONE) || (vers == RSTATVERS_SWTCH)) {
		snum = RPC_ANYSOCK;
		if ((client = clntudp_bufcreate(&serveradr, RSTATPROG,
			RSTATVERS_SWTCH, timeout, &snum,
			sizeof(struct rpc_msg), sizeof(struct rpc_msg) +  
			sizeof(struct statsswtch)))== NULL) 
			return (-1);
		clnt_stat = clnt_call(client, NULLPROC, xdr_void, 0,
				      xdr_void, 0, TIMEOUT);
		if (clnt_stat == RPC_SUCCESS) {
			vers = RSTATVERS_SWTCH;
		} else {
			clnt_destroy(client);
			return (-1);
		}
	}
	if (oldsocket >= 0) 
		(void)close(oldsocket);
	oldsocket = snum;
	if (oldclient) 
		clnt_destroy(oldclient);
	oldclient = client;
	return (0);
}

/*
 * Fork a separate process to keep trying to contact the host
 * so that the main process can continue to service window
 * requests (repaint, move, stretch, etc.).
 */

keeptrying()
{	
	int pid;
	
	if ((int)signal(SIGCHLD, ondeath) == -1)
		perror("signal");
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		for (;;) {
			sleep(1);
			if (setup() < 0)
				continue;
			exit(0);
		}
	}
}

/*
 * Kill any background processes we may've started.
 */
killkids()
{

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGCHLD, SIG_IGN);
	(void)killpg(getpgrp(0), SIGINT);	/* get rid of forked processes */
	exit(0);
}

/*
 * Get the metered data from the host via RPC
 * and process it to compute the actual values
 * (rates) that perfmeter wants to see.
 */


/* static data used only by getdata_swtch() and getdata_var() */
static	statsswtch statswtch;
static	statsvar stats_var;
static	int *oldtime;
static	int total;			/* Default to zero */
static	int toterr;			/* Default to zero */
static	int totcoll;			/* Default to zero */
static	struct timeval tm, oldtm;
static	int *xfer1;
static	int badcnt;			/* Default to zero */
static	int ans[MAXMETERS];
static	int oldi, olds, oldp, oldsp;
static	int getdata_init_done;		/* Default to zero */
static int cpustates;
static int dk_ndrive;

int *
getdata_swtch()
{
	register int i, t;
	register int msecs;
	int maxtfer;
	enum clnt_stat clnt_stat;
	int intrs, swtchs, pag, spag;
	int sum, ppersec;

	clnt_stat = clnt_call(client, RSTATPROC_STATS, xdr_void, 0,
		xdr_statsswtch, &statswtch, TIMEOUT);
	if (clnt_stat == RPC_TIMEDOUT)
		return (NULL);		
	if (clnt_stat != RPC_SUCCESS) {
		if (!sick) {
			clnt_perror(client, "cpugetdata");
			sick = 1;
		}
       		meter_paint();
		return (NULL);		
	}
	if (oldtime == (int *)NULL) { /* allocate memory for structures */
		cpustates = 4;		/* compatibility with version 3*/
		dk_ndrive = 4;
		oldtime = (int *) malloc(cpustates * sizeof(int));
		xfer1 = (int *) malloc(dk_ndrive * sizeof(int));
		if ( (oldtime == NULL) || (xfer1 == NULL)) {
			fprintf(stderr,"failed in malloc()\n");
			exit(1);
		}
	}
	for (i = 0; i < cpustates; i++) {
		t = statswtch.cp_time[i];
		statswtch.cp_time[i] -= oldtime[i];
		oldtime[i] = t;
	}
	t = 0;
	for (i = 0; i < cpustates -1; i++)	/* assume IDLE is last */
		t += statswtch.cp_time[i];
	if (statswtch.cp_time[cpustates -1] + t <= 0) {
		t++;
		badcnt++;
		if (badcnt >= TRIES) {
			sick = 1;
            		meter_paint();
		}
	} else if (sick) {
		badcnt = sick = 0;
        	meter_paint();
	} else {
		badcnt = 0;
	}
	ans[CPU] = (100*t) / (statswtch.cp_time[cpustates - 1] + t);

	(void)gettimeofday(&tm, (struct timezone *)0);
	msecs = (1000 * (tm.tv_sec - oldtm.tv_sec) +
			(tm.tv_usec - oldtm.tv_usec)/1000);

	msecs++;	/* round up 1ms to avoid msecs == 0 */

	sum = statswtch.if_ipackets + statswtch.if_opackets;
	ppersec = 1000*(sum - total) / msecs;
	total = sum;
	ans[PKTS] = ppersec;

	ans[COLL] = FSCALE*(statswtch.if_collisions - totcoll)*1000 / msecs;
	totcoll = statswtch.if_collisions;
	ans[ERR] = FSCALE*(statswtch.if_ierrors - toterr)*1000 / msecs;
	toterr = statswtch.if_ierrors;

	if (!getdata_init_done) {
		pag = 0;
		spag = 0;
		intrs = 0;
		swtchs = 0;
		getdata_init_done = 1;
	} else {
		pag = statswtch.v_pgpgout + statswtch.v_pgpgin - oldp;
		pag = 1000*pag / msecs;
		spag = statswtch.v_pswpout + statswtch.v_pswpin - oldsp;
		spag = 1000*spag / msecs;
		intrs = statswtch.v_intr - oldi;
		intrs = 1000*intrs / msecs;
		swtchs = statswtch.v_swtch - olds;
		swtchs = 1000*swtchs / msecs;
	}
	oldp = statswtch.v_pgpgin + statswtch.v_pgpgout;
	oldsp = statswtch.v_pswpin + statswtch.v_pswpout;
	oldi = statswtch.v_intr;
	olds = statswtch.v_swtch;
	ans[PAGE] = pag;
	ans[SWAP] = spag;
	ans[INTR] = intrs;
	ans[CNTXT] = swtchs;
	ans[LOAD] = statswtch.avenrun[0];
	for (i = 0; i < dk_ndrive; i++) {
		t = statswtch.dk_xfer[i];
		statswtch.dk_xfer[i] -= xfer1[i];
		xfer1[i] = t;
	}

/*	Bug# 1028570 fix, old code commented out
 *
 *	maxtfer = statswtch.dk_xfer[0];
 *	for (i = 1; i < dk_ndrive; i++)
 *		if (statswtch.dk_xfer[i] > maxtfer)
 *			maxtfer = statswtch.dk_xfer[i];
 */
	for (i = 0, maxtfer = 0; i < dk_ndrive; i++)
		maxtfer += statswtch.dk_xfer[i];

	maxtfer = (1000*maxtfer) / msecs;
	ans[DISK] = maxtfer;
	oldtm = tm;
	return (ans);
}

int *
getdata_var()
{
	register int i, t;
	register int msecs;
	int maxtfer;
	enum clnt_stat clnt_stat;
	int intrs, swtchs, pag, spag;
	int sum, ppersec;

	if (oldtime == (int *) NULL) {
		stats_var.dk_xfer.dk_xfer_val = (int *) NULL;
		stats_var.cp_time.cp_time_val = (int *) NULL;
	}
	clnt_stat = clnt_call(client, RSTATPROC_STATS, xdr_void, 0,
			xdr_statsvar, &stats_var, TIMEOUT);
	if (clnt_stat == RPC_TIMEDOUT)
		return (NULL);		
	if (clnt_stat != RPC_SUCCESS) {
		if (!sick) {
			clnt_perror(client, "cpugetdata");
			sick = 1;
		}
       		meter_paint();
		return (NULL);		
	}
	if ( oldtime == (int * ) NULL) { /* allocate memory for structures */
		cpustates = stats_var.cp_time.cp_time_len;
		dk_ndrive = stats_var.dk_xfer.dk_xfer_len;
		oldtime = (int *) malloc(cpustates * sizeof(int));
		xfer1 = (int *) malloc(dk_ndrive * sizeof(int));
		if ( (oldtime == NULL) || (xfer1 == NULL)) {
			fprintf(stderr,"failed in malloc()\n");
			exit(1);
		}
	}

	for (i = 0; i < cpustates; i++) {
		t = stats_var.cp_time.cp_time_val[i];
		stats_var.cp_time.cp_time_val[i] -= oldtime[i];
		oldtime[i] = t;
	}
	t = 0;
	for (i = 0; i < cpustates - 1 ; i++)	/* assume IDLE is last */
		t += stats_var.cp_time.cp_time_val[i];
	if (stats_var.cp_time.cp_time_val[cpustates - 1 ] + t <= 0) {
		t++;
		badcnt++;
		if (badcnt >= TRIES) {
			sick = 1;
            		meter_paint();
		}
	} else if (sick) {
		badcnt = sick = 0;
        	meter_paint();
	} else {
        	badcnt = 0;
	}
	ans[CPU] = (100*t) / 
			(stats_var.cp_time.cp_time_val[cpustates - 1 ] + t);

	(void)gettimeofday(&tm, (struct timezone *)0);
	msecs = (1000*(tm.tv_sec - oldtm.tv_sec) +
			(tm.tv_usec - oldtm.tv_usec)/1000);

	msecs++;	/* round up 1ms to avoid msecs == 0 */

	/*
	 * (Bug# 1013912 fix) in the rare occurence that msecs = 0,
	 * reset to msecs = 1 to ensure no "divide by zero" 
	 * errors later.
	 */
	if (msecs == 0) msecs = 1;

	sum = stats_var.if_ipackets + stats_var.if_opackets;
	ppersec = 1000*(sum - total) / msecs;
	total = sum;
	ans[PKTS] = ppersec;

	ans[COLL] = FSCALE*(stats_var.if_collisions - totcoll)*1000 / msecs;
	totcoll = stats_var.if_collisions;
	ans[ERR] = FSCALE*(stats_var.if_ierrors - toterr)*1000 / msecs;
	toterr = stats_var.if_ierrors;

	if (!getdata_init_done) {
		pag = 0;
		spag = 0;
		intrs = 0;
		swtchs = 0;
		getdata_init_done = 1;
	} else {
		pag = stats_var.v_pgpgout + stats_var.v_pgpgin - oldp;
		pag = 1000*pag / msecs;
		spag = stats_var.v_pswpout + stats_var.v_pswpin - oldsp;
		spag = 1000*spag / msecs;
		intrs = stats_var.v_intr - oldi;
		intrs = 1000*intrs / msecs;
		swtchs = stats_var.v_swtch - olds;
		swtchs = 1000*swtchs / msecs;
	}
	oldp = stats_var.v_pgpgin + stats_var.v_pgpgout;
	oldsp = stats_var.v_pswpin + stats_var.v_pswpout;
	oldi = stats_var.v_intr;
	olds = stats_var.v_swtch;
	ans[PAGE] = pag;
	ans[SWAP] = spag;
	ans[INTR] = intrs;
	ans[CNTXT] = swtchs;
	ans[LOAD] = stats_var.avenrun[0];
	for (i = 0; i < dk_ndrive; i++) {
		t = stats_var.dk_xfer.dk_xfer_val[i];
		stats_var.dk_xfer.dk_xfer_val[i] -= xfer1[i];
		xfer1[i] = t;
	}

/*	Bug# 1028570 fix, old code commented out
 *
 *	maxtfer = stats_var.dk_xfer.dk_xfer_val[0];
 *	for (i = 1; i < dk_ndrive; i++)
 *		if (stats_var.dk_xfer.dk_xfer_val[i] > maxtfer)
 *			maxtfer = stats_var.dk_xfer.dk_xfer_val[i];
 */

	for (i = 0, maxtfer = stats_var.dk_xfer.dk_xfer_val[0]; i < dk_ndrive; i++)
 		maxtfer += stats_var.dk_xfer.dk_xfer_val[i];

	maxtfer = (1000*maxtfer) / msecs;
	ans[DISK] = maxtfer;
	oldtm = tm;
	return (ans);
}

static caddr_t
do_menu(menu, mi)
	Menu		menu;
	Menu_item	mi;
{
	int	n;

	n = (int)menu_get(mi, MENU_VALUE);

	/*  BUG: WOrkaround ; There should be another way of doing this */
	if (strcmp("frame", (char *)menu_get(mi, MENU_STRING)))
		visible = n;
	meter_paint();
	return(mi);
}

static
Notify_value
meter_itimer_expired(meter, which)
	Notify_client   meter;
	int          which;
{
	struct itimerval	itimer;

	updatedata();
	itimer = NOTIFY_NO_ITIMER;
	itimer.it_value.tv_sec = sampletime;
	meter_update();
        (void) notify_set_itimer_func((Notify_client)(&meter_client),
		meter_itimer_expired, ITIMER_REAL, &itimer,
		(struct itimerval *)0);
}
