

/*
	get_view_surface  --  Determines from command-line arguments and
			      the environment a reasonable view surface
			      for a SunCore program to run on.
*/

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <stdio.h>
#include <sunwindow/window_hs.h>
#include <usercore.h>

int bw1dd();		/* All five device-independent/device-dependent   */
int bw2dd();		/* routines are referenced in this function.      */
int cg1dd();		/* This means the linker will pull in all of them */
int pixwindd();
int cgpixwindd();

static struct vwsurf nullvs = NULL_VWSURF;

static char *devchk;
static int devhaswindows;

int get_surface(vsptr, argv)
struct	vwsurf	*vsptr;
char	**argv;
{
	get_view_surface(vsptr, argv);
	vsptr->cmapsize = 256;
	vsptr->cmapname[0] = '\0';
}

int get_view_surface(vsptr, argv)
struct vwsurf *vsptr;
char **argv;
	{
	int devfnd, fd, chkdevhaswindows();
	char *wptr, dev[DEVNAMESIZE], *getenv();
	struct screen screen;
	struct fbtype fbtype;

	*vsptr = nullvs;
	devfnd = FALSE;
	if (argv)
		/*
		If command-line arguments are passed, process them using
		win_initscreenfromargv (see the Programmer's Reference Manual
		for the Sun Window System).  The only option used by
		get_view_surface is the -d option, allowing the user to
		specify the display device on which to run.
		*/
		{
		win_initscreenfromargv(&screen, argv);
		if (screen.scr_fbname[0] != '\0')
			{
			/* -d option was found */
			devfnd = TRUE;
			strncpy(dev, screen.scr_fbname, DEVNAMESIZE);
			/*
			Check to see if this device has a window system
			running on it.  If so devhaswindows will be TRUE
			following the call to win_enumall.  win_enumall is
			a function in libsunwindow.a.  It takes a function
			as its argument, and applies this function to every
			window being displayed on any screen by the window
			system.  To do this it opens each window and passes
			the windowfd to the function.  The enumeration
			continues until all windows have been tried or the
			function returns TRUE.
			*/
			devchk = dev;
			devhaswindows = FALSE;
			win_enumall(chkdevhaswindows);
			}
		}
	if (!devfnd)
		/* No -d option was specified */
		if (wptr = getenv("WINDOW_ME"))
			{
			/*
			Running in the window system.  Find the device from
			which this program was started.
			*/
			devhaswindows = TRUE;
			if ((fd = open(wptr, O_RDWR, 0)) < 0)
			    {
			    fprintf(stderr, "get_view_surface: Can't open %s\n",
				wptr);
			    return(1);
			    }
			win_screenget(fd, &screen);
			close(fd);
			strncpy(dev, screen.scr_fbname, DEVNAMESIZE);
			}
		else
			{
			/*
			Not running in the window system.  Assume device is
			/dev/fb.
			*/
			devhaswindows = FALSE;
			strncpy(dev, "/dev/fb", DEVNAMESIZE);
			}
	/* Now have device name.  Find device type. */
	if ((fd = open(dev, O_RDWR, 0)) < 0)
		{
		fprintf(stderr, "get_view_surface: Can't open %s\n", dev);
		return(1);
		}
	if (ioctl(fd, FBIOGTYPE, &fbtype) == -1)
		{
		fprintf(stderr, "get_view_surface: ioctl FBIOGTYPE failed on %s\n",
			dev);
		close(fd);
		return(1);
		}
	close(fd);
	/* Now have device type and know if window system is running on it. */
	if (devhaswindows)
		switch(fbtype.fb_type)
			{
		case FBTYPE_SUN1BW:
		case FBTYPE_SUN2BW:
			vsptr->dd = pixwindd;
			break;
		case FBTYPE_SUN1COLOR:
			vsptr->dd = cgpixwindd;
			break;
		default:
			fprintf(stderr, "get_view_surface: %s is unknown fbtype\n",
				dev);
			return(1);
			}
	else
		switch(fbtype.fb_type)
			{
		case FBTYPE_SUN1BW:
			vsptr->dd = bw1dd;
			break;
		case FBTYPE_SUN2BW:
			vsptr->dd = bw2dd;
			break;
		case FBTYPE_SUN1COLOR:
			vsptr->dd = cg1dd;
			break;
		default:
			fprintf(stderr, "get_view_surface: %s is unknown fbtype\n",
				dev);
			return(1);
			}
	/* Now SunCore device driver pointer is set up. */
	if (!devhaswindows || devfnd)
		/*
		If no window system on device or -d option was specified,
		tell SunCore which device.  Otherwise, let SunCore figure
		out the device itself from WINDOW_GFX so the default
		window will be used if desired.
		*/
		strncpy(vsptr->screenname, dev, DEVNAMESIZE);
	return(0);
	}

static int chkdevhaswindows(windowfd)
int windowfd;
	{
	struct screen windowscreen;

	win_screenget(windowfd, &windowscreen);
	if (strcmp(devchk, windowscreen.scr_fbname) == 0)
		{
		/*
		If this window is on the display device we are checking, set
		the flag TRUE.  Return TRUE to terminate the enumeration.
		*/
		devhaswindows = TRUE;
		return(TRUE);
		}
	return(FALSE);
	}
