
#ifndef lint
static char sccsid[] = "@(#)vidcal.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <fcntl.h>
#include <suntool/sunview.h>

static short vidcal_icon_image[256] =
{
#include "vidcal_icon.h"
};
mpr_static(vidcal_icon_pixrect, 64, 64, 1, vidcal_icon_image);

Window	calibrate_frame;
int	tv1_fd;


main(argc, argv)
    int argc;
    char **argv;
{
    if ((tv1_fd = open("/dev/tvone0", O_RDWR, 0)) == -1) {
	perror("Open failed on /dev/tvone0:");
	exit(1);
    }
    calibrate_frame = window_create(0, FRAME,
	FRAME_LABEL, "Video Calibration",
	FRAME_ICON, icon_create(ICON_IMAGE, &vidcal_icon_pixrect, 0),
	FRAME_ARGC_PTR_ARGV, &argc, argv,
	0);
    create_video_cal_panel(calibrate_frame);
    window_fit(calibrate_frame);
    window_main_loop(calibrate_frame);
}
