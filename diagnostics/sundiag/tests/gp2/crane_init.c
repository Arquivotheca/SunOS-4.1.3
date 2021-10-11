#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <suntool/sunview.h>
#include <sun/fbio.h>
#include "gp2test.h"
#define FALSE 0
#define TRUE  1

#ifndef PIXPG_24BIT_COLOR
#define PIXPG_24BIT_COLOR 5
#endif

extern Pixrect *screen;
extern int CgTWO, winfd;

Crane_Init()

{
	pr_set_plane_group ( screen, PIXPG_OVERLAY );
	pr_rop ( screen, 0, 0, screen->pr_width, screen->pr_height,
		PIX_SRC, 0, 0, 0 );

	pr_set_plane_group ( screen, PIXPG_OVERLAY_ENABLE );
	pr_rop ( screen, 0, 0, screen->pr_width, screen->pr_height,
		PIX_SRC, 0, 0, 0 );

	pr_set_plane_group ( screen, PIXPG_24BIT_COLOR);
	pr_rop ( screen, 0, 0, screen->pr_width, screen->pr_height,
		PIX_SRC, 0, 0, 0 );

}

getfb()

{
	int fd;
	struct fbgattr fb_gattr;
	struct fbtype fb_type;

/* Search for gp2 */

	fd = open ("/dev/fb", O_RDWR );
	ioctl ( fd, FBIOGTYPE, &fb_type );
	if ( fb_type.fb_type != FBTYPE_SUN2GP ) {
		close ( fd );
		fd = open (GP_DEV, O_RDWR );
	}

	ioctl ( fd, FBIOGATTR, &fb_gattr );
/*	if ( fb_gattr.fbtype.fb_type != FBTYPE_SUN2COLOR ) CgTWO = FALSE; */
	if ( fb_gattr.sattr.dev_specific[0] != FBTYPE_SUN2COLOR ) CgTWO = FALSE;
	else CgTWO = TRUE;
/*	CgTWO = TRUE; */

	close ( fd );
}

check_input()

{
        Event   event;
        int arg;

        arg = 0;
        ioctl(winfd, FIONREAD, &arg);
        if (arg != 0) {
                input_readevent(winfd, &event);
                if (event_id(&event) == 0x03)           /* CTRL-C */
                        finish();
        }
        return(0);
}


