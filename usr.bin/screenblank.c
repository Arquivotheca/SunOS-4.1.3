/* @(#)screenblank.c	1.1 7/30/92 92/07/30 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#define USAGE "Usage: screenblank [-m] [-k] [-d interval] [-e interval] [-f frame_buffer]"
#define PROGNAME "screenblank: "
#define DEFAULT_FB "/dev/fb"
#define MAX_DEVICES 3		/* kbd and mouse and console */
#define KBD 0
#define MOUSE 1
#define CONSOLE 2
#define DEFAULT_INTV_WHEN_ENABLED_SEC 600
#define DEFAULT_INTV_WHEN_ENABLED_USEC 0
#define DEFAULT_INTV_WHEN_DISABLED_SEC 0
#define DEFAULT_INTV_WHEN_DISABLED_USEC 250000
#define FILENAME_LEN 256
#define MAXFB 5

#define op_on_fb()	{short x; \
			 for (x = 0; x < num_of_fb; x++)  { \
			 	ioctl(frame[x].fd,FBIOSVIDEO,&enabled); \
			 } \
			}

int	errno;
struct dev_info {
	int should_check;	/* whether or not to chk access time of dev */
	char dev_file[FILENAME_LEN];	/* dev file name */
	time_t last_atime;	/* last access time of the dev */
	time_t last_mtime;	/* last modified time of the dev */
};

static	void sigterm_catcher();
static	struct {
	int fd;
	char name[FILENAME_LEN]
}   frame[MAXFB];
static  int num_of_fb;

main(argc,argv)
int argc;
char *argv[];
{
	struct stat statb;
	struct dev_info devices[MAX_DEVICES];
	struct timeval intv_when_disabled;
	struct timeval intv_when_enabled;
	double	interval,convert();
	int enabled = 1; /* Can't be reg */
	char c;
	register int i;
	int k;
	int tty;
	int tab_size;
	int f_flag = 0;
	int accessed;
	int modified;

	intv_when_disabled.tv_sec = DEFAULT_INTV_WHEN_DISABLED_SEC;
	intv_when_disabled.tv_usec = DEFAULT_INTV_WHEN_DISABLED_USEC;
	intv_when_enabled.tv_sec = DEFAULT_INTV_WHEN_ENABLED_SEC;
	intv_when_enabled.tv_usec = DEFAULT_INTV_WHEN_ENABLED_USEC;
	argc--;
	argv++;

	/* default is to check both keyboard and mouse */	
	devices[KBD].should_check = 1;
	strcpy(devices[KBD].dev_file,"/dev/kbd");
	devices[KBD].last_atime = 0;
   	devices[MOUSE].should_check = 1;
    	strcpy(devices[MOUSE].dev_file,"/dev/mouse");
    	devices[MOUSE].last_atime = 0;
   	devices[CONSOLE].should_check = 1;
    	strcpy(devices[CONSOLE].dev_file,"/dev/console");
    	devices[CONSOLE].last_atime = 0;

	while (argc)  {
		switch ((*argv)[1])  {
		    case 'k':	
			if (devices[MOUSE].should_check == 0)  {
			    fprintf(stderr,"Setting both -m and -k options is disallowed because it disables unblanking the video.\n");
			    exit(1);
			}
		    	devices[KBD].should_check = 0;
		     	break;
		    case 'm':	
			if (devices[KBD].should_check == 0)  {
			    fprintf(stderr,"Setting both -m and -k options is disallowed because it disables unblanking the video.\n");
			    exit(1);
			}
			devices[MOUSE].should_check = 0;
		     	break;
		    case 'e':
		    	argv++;
		    	argc--;
			if (!*argv)
			    error(USAGE);
			else {	
		     	    interval = convert(*argv);
		     	    intv_when_disabled.tv_sec = (int) interval;
		     	    intv_when_disabled.tv_usec = (interval-intv_when_disabled.tv_sec) * 1000000;
#ifdef DEBUG
			    printf("disable interval = %f\n",interval);
			    printf("disable sec = %d, usec = %d\n",
			    intv_when_disabled.tv_sec,intv_when_disabled.tv_usec);
#endif
			}
			break;
		    case 'd':
		    	argv++;
		    	argc--;
			if (!*argv)
                            error(USAGE);
                        else {
		    	    interval = convert(*argv);
		     	    intv_when_enabled.tv_sec = (int) interval;
		     	    intv_when_enabled.tv_usec = (interval-intv_when_enabled.tv_sec) * 1000000;
#ifdef DEBUG
			    printf("enable interval = %f\n",interval);
			    printf("enable sec = %d, usec = %d\n",
			    intv_when_enabled.tv_sec,intv_when_enabled.tv_usec);
#endif
			}
		     	break;
		     case 'f':
		        (void)sprintf(frame[num_of_fb].name,"%s",argv[1]);
		     	argv++;
		     	argc--;
		     	f_flag = 1;
		     	num_of_fb++;
		     	if (num_of_fb >= MAXFB)  {
		     		printf("Too many frame buffer specified\n");
		     		exit(1);
		     	}
		     	break;
		     default:	
		     	error(USAGE);
		     	break;
		 }
		argv++;
		argc--;
	}
#ifndef DEBUG
	if (fork())  {
		exit(0);
	}
	tab_size = getdtablesize();
	for (k = 0; k < tab_size; k++)  {
		(void) close(k);
	}
	(void) open("/",O_RDONLY);
	(void) dup2(0,1);
	(void) dup2(0,2);
	tty = open("/dev/tty",O_RDWR);
	if (tty > 0)  {
		ioctl(tty,TIOCNOTTY,0);
		(void)close(tty);
	}
#endif DEBUG
	if (!f_flag)  {
		num_of_fb++;
		if ((frame[0].fd=open(DEFAULT_FB,O_RDWR,0)) < 0)  {
			perror(PROGNAME);
		     	exit(1);
		}
	}   else   {
		for (k = 0; k < num_of_fb; k++)  {
			if ((frame[k].fd =
				 open(frame[k].name,O_RDWR,0)) < 0)  {
		        	perror(frame[k].name);
		        	exit(1);
		        }
		}
	}
	
	signal(SIGTERM, sigterm_catcher);
	signal(SIGHUP, sigterm_catcher);

	/* Turn on initially */
	enabled = FBVIDEO_ON;
	op_on_fb();
	while (1)  {
		i = 0;
		accessed = 0;
		modified = 0;
		while (i < MAX_DEVICES) {
			if (devices[i].should_check) {
				if (stat(devices[i].dev_file, &statb) < 0)  {
					fprintf(stderr, "devstat: ");
					perror(devices[i].dev_file);
					exit(1);
				}
				if (statb.st_atime > devices[i].last_atime) {
					accessed = 1;
					devices[i].last_atime = statb.st_atime;
				}
				if (statb.st_mtime > devices[i].last_mtime) {
					modified = 1;
					devices[i].last_mtime = statb.st_mtime;
				}
			}
			i++;
		}
		if (enabled && !accessed && !modified)  {
			enabled = FBVIDEO_OFF;
			op_on_fb();
		} else if (!enabled && (accessed || modified))  {
			enabled = FBVIDEO_ON;
			op_on_fb();
		}
		if (enabled)	{
			if (select(0,0,0,0,&intv_when_enabled) < 0)  {
				perror("select: ");
				exit(1);
			}
		}   else   {
			if (select(0,0,0,0,&intv_when_disabled) < 0)  {
				perror("select: ");
				exit(1);
			}
		}
	}
}



error(str)
char *str;
{
	printf("%s\n",str);
	exit(1);
}

double
convert(str)
char  *str;
{
	int i = 0;
	char c;
	double num;
	int decimal;
	double neg_pow;
	double pow();
	
	num = 0.0;
	decimal = 0;
	neg_pow = 0.0;
	while (c = str[i++])   {
		if (c == '.')  {
			decimal = 1;
			neg_pow = -1.0;
		}   else   {
			if (c < '0' || c > '9')  {
				printf("Invalid floating point number\n");
				exit(1);
			}
			if (!decimal)  {
				num = num * 10 + (c - '0');
			}   else   {
				num = num + (double) (c-'0') * pow(10.0,neg_pow);
				neg_pow -= 1.0;
			}
		}
	}
	return(num);
}

static	void
sigterm_catcher()
{
	int enabled = FBVIDEO_ON;

	op_on_fb();
	exit(0);
}

