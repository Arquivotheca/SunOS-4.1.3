
#ifndef lint
static char sccsid[] = "@(#)video_pr.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/tv1var.h>
#include <sun/tvio.h>
#include <suntool/sunview.h>

extern struct tv1_data *get_tv1_data();

#define isvideopr(pr) (get_tv1_data(pr))


vid_set_xy(pr, pos)
    struct pixrect *pr;
    struct pr_pos *pos;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    pr_rop(get_tv1_data(pr)->pr_video_enable,  0, 0, 1152, 900,
           PIX_CLR, NULL, 0, 0);
    if (ioctl(get_tv1_data(pr)->fd, TVIOSPOS, pos) == -1) {
        return -1;
    }
    return 0;
}

struct pr_pos *
vid_get_xy(pr)
    struct pixrect *pr;
{
    static struct pr_pos pos;
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGPOS, &pos) == -1) {
        return ((struct pr_pos *)0);
    }
    return &pos;
}


vid_set_luma_gain(pr, luma_gain)
    struct pixrect *pr;
    int luma_gain;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSLUMAGAIN, &luma_gain) == -1) {
	perror("TVIOSLUMAGAIN failed");
	return -1;
    }
    return(0);
}

int
vid_get_luma_gain(pr)
    struct pixrect *pr;
{
    int luma_gain;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSLUMAGAIN, &luma_gain) == -1) {
	perror("TVIOSLUMAGAIN failed");
	return -1;
    }
    return(luma_gain);
}

vid_set_chroma_gain(pr, chroma_gain)
    struct pixrect *pr;
    int chroma_gain;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSCHROMAGAIN, &chroma_gain) == -1) {
	perror("TVIOSCHROMAGAIN failed");
	return -1;
    }
    return 0;
}


vid_get_chroma_gain(pr)
    struct pixrect *pr;
{
    int chroma_gain;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGCHROMAGAIN, &chroma_gain) == -1) {
	perror("TVIOGCHROMAGAIN failed");
	return -1;
    }
    return (chroma_gain);
}

vid_set_red_gain(pr, gain)
    struct pixrect *pr;
    int gain;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSREDGAIN, &gain) == -1) {
	perror("TVIOSREDGAIN failed");
	return -1;
    }
    return 0;
}
vid_get_red_gain(pr)
    struct pixrect *pr;
{
    int gain;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGREDGAIN, &gain) == -1) {
	perror("TVIOGREDGAIN failed");
	return -1;
    }
    return (gain);
}

vid_set_red_black_level(pr, black_level)
    struct pixrect *pr;
    int black_level;
{
     if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSREDBLACK, &black_level) == -1) {
	perror("TVIOSREDBLACK failed");
	return -1;
    }
    return 0;
}
vid_get_red_black_level(pr)
    struct pixrect *pr;
{
    int black_level;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGREDBLACK, &black_level) == -1) {
	perror("TVIOGREDBLACK failed");
	return -1;
    }
    return (black_level);
}

vid_set_green_gain(pr, gain)
    struct pixrect *pr;
    int gain;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSGREENGAIN, &gain) == -1) {
	perror("TVIOSGREENGAIN failed");
	return -1;
    }
    return 0;
}
vid_get_green_gain(pr)
    struct pixrect *pr;
{
    int gain;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGGREENGAIN, &gain) == -1) {
	perror("TVIOGGREENGAIN failed");
	return -1;
    }
    return(gain);
}


vid_set_green_black_level(pr, black_level)
    struct pixrect *pr;
    int black_level;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSGREENBLACK, &black_level) == -1) {
	perror("TVIOSGREENBLACK failed");
	return -1;
    }
    return 0;
}
vid_get_green_black_level(pr)
    struct pixrect *pr;
{
    int black_level;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGGREENBLACK, &black_level) == -1) {
	perror("TVIOGGREENBLACK failed");
	return -1;
    }
    return (black_level);
}


vid_set_blue_gain(pr, gain)
    struct pixrect *pr;
    int gain;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSBLUEGAIN, &gain) == -1) {
	perror("TVIOSBLUEGAIN failed");
	return -1;
    }
    return 0;
}
vid_get_blue_gain(pr)
    struct pixrect *pr;
{
    int gain;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGBLUEGAIN, &gain) == -1) {
	perror("TVIOGBLUEGAIN failed");
	return -1;
    }
    return (gain);
}


vid_set_blue_black_level(pr, black_level)
    struct pixrect *pr;
    int black_level;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSBLUEBLACK, &black_level) == -1) {
	perror("TVIOSBLUEBLACK failed");
	return -1;
    }
    return 0;
}
vid_get_blue_black_level(pr)
    struct pixrect *pr;
{
    int black_level;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGBLUEBLACK, &black_level) == -1) {
	perror("TVIOGBLUEBLACK failed");
	return -1;
    }
    return (black_level);
}


vid_set_input_format(pr, video_format)
    struct pixrect *pr;
    int video_format;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSFORMAT, &video_format) == -1) {
	perror("TVIOSFORMAT failed");
	return -1;
    }
    
}

vid_get_input_format(pr)
    struct pixrect *pr;
{
    int video_format;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGFORMAT, &video_format) == -1) {
	perror("TVIOSFORMAT failed");
	return -1;
    }
    return(video_format);
    
}


vid_set_sync_format(pr, sync_format)
    struct pixrect *pr;
    int sync_format;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSSYNC, &sync_format) == -1) {
	perror("TVIOSSYNC failed");
	return -1;
    }
    
}

vid_get_sync_format(pr)
    struct pixrect *pr;
{
    int sync_format;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGSYNC, &sync_format) == -1) {
	perror("TVIOSFORMAT failed");
	return -1;
    }
    return(sync_format);
}


vid_set_zoom(pr, zoom)
    struct pixrect *pr;
    int zoom;
{
    static int zoom_convert[] = {
	0, 1, 2, 4, 6, 7
    };
    
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSCOMPRESS, &zoom_convert[zoom]) == -1) {
	perror("TVIOSCOMPRESS failed");
	return -1;
    }
    return 0;
}
vid_get_zoom(pr)
    struct pixrect *pr;
{
    int zoom;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGCOMPRESS, &zoom) == -1) {
	perror("TVIOGCOMPRESS failed");
	return -1;
    }
    switch (zoom) {
	case 0: return(0);
	case 1: return(1);
	case 2: return(2);
	case 4: return(4);
	case 6: return(6);
	case 7: return(7);
	default: return(-1);
    }
}


vid_set_video_out(pr, where)
    struct pixrect *pr;
    int where;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSOUT, &where) == -1) {
	perror("TVIOSOUT failed");
	return -1;
    }
    return 0;
}
vid_get_video_out(pr)
    struct pixrect *pr;
{
    int where;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGOUT, &where) == -1) {
	perror("TVIOSOUT failed");
	return -1;
    }
    return (where);
}


vid_set_live(pr, live)
    struct pixrect *pr;
    int live;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSLIVE, &live) == -1) {
	perror("TVIOSLIVE failed");
	return -1;
    }
    /* Now wait for digitize to complete and Sync up */
    if (!live) {
	usleep(120000);
	if (ioctl(get_tv1_data(pr)->fd, TVIOSYNC) == -1) {
	    perror("TVIOSYNC failed");
	}
    }
    return 0;
}
vid_get_live(pr, live)
    struct pixrect *pr;
    int live;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGLIVE, &live) == -1) {
	perror("TVIOGLIVE failed");
	return -1;
    }
    return (live);
}


vid_set_chroma_sep(pr, chroma_sep)
    struct pixrect *pr;
    int chroma_sep;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSCHROMASEP, &chroma_sep) == -1) {
	perror("TVIOSCHROMASEP failed");
	return -1;
    }
    return 0;
}
vid_get_chroma_sep(pr)
    struct pixrect *pr;
{
    int chroma_sep;

    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGCHROMASEP, &chroma_sep) == -1) {
	perror("TVIOGCHROMASEP failed");
	return -1;
    }
    return (chroma_sep);
}

vid_set_cal(pr, cal)
    struct pixrect *pr;
    union tv1_nvram *cal;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOSVIDEOCAL, cal) == -1) {
	perror("TVIOSVIDEOCAL failed");
	return -1;
    }
    return 0;
}

vid_get_cal(pr, cal)
    struct pixrect *pr;
    union tv1_nvram *cal;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIOGVIDEOCAL, cal) == -1) {
	perror("TVIOGVIDEOCAL failed");
	return -1;
    }
    return 0;
}

vid_store_cal(pr, cal)
    struct pixrect *pr;
    union tv1_nvram *cal;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIONVWRITE, cal) == -1) {
	perror("TVIONVWRITE failed");
	return -1;
    }
    return 0;
}

vid_recall_cal(pr, cal)
    struct pixrect *pr;
    union tv1_nvram *cal;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    if (ioctl(get_tv1_data(pr)->fd, TVIONVREAD, cal) == -1) {
	perror("TVIONVREAD failed");
	return -1;
    }
    return 0;
}

vid_clear_enable_plane(pr)
    struct pixrect *pr;
{
    if (!isvideopr(pr)) {
	return 0;
    }
    pr_rop(get_tv1_data(pr)->pr_video_enable,  0, 0, 1152, 900,
           PIX_CLR, NULL, 0, 0);
}

struct pixrect *
vid_get_video_pixrect(pr)
    struct pixrect *pr;
{
    if (!isvideopr(pr)) {
	return((struct pixrect *)0);
    } else {
	return (get_tv1_data(pr)->pr_video);
    }
}

vid_set_comp_out(pr, comp_type)
    struct pixrect *pr;
    int comp_type;
{
     if (ioctl(get_tv1_data(pr)->fd, TVIOSCOMPOUT, &comp_type) == -1) {
	perror("TVIOSCOMPTYPE failed");
    }
}
vid_get_comp_out(pr)
    struct pixrect *pr;
{
    int comp_type;

    if (ioctl(get_tv1_data(pr)->fd, TVIOGCOMPOUT, &comp_type) == -1) {
	perror("TVIOGCOMPTYPE failed");
    }
    return(comp_type);
}

vid_set_chroma_demod(pr, value)
    struct pixrect *pr;
    int value;
{
   if (ioctl(get_tv1_data(pr)->fd, TVIOSCHROMADEMOD, &value) == -1) {
	perror("TVIOSCHROMADEMOD failed");
    }
}
 
vid_get_chroma_demod(pr)
    struct pixrect *pr;
{
   int value;

   if (ioctl(get_tv1_data(pr)->fd, TVIOGCHROMADEMOD, &value) == -1) {
	perror("TVIOGCHROMADEMOD failed");
   }
   return(value);
} 

vid_set_win_defaults(pr, rect)
    struct pixrect *pr;
    Rect *rect;
{
    rect->r_top = 29; 
    rect->r_left = 0;
    rect->r_width = 640;
    rect->r_height = 486;
    vid_set_live(pr, 1);
    return(0);
}

vid_set_gen_lock(pr, genlock)
    struct pixrect *pr;
    int genlock;
{
    if (ioctl(get_tv1_data(pr)->fd, TVIOSGENLOCK, &genlock) == -1) {
	perror("TVIOSGENLOCK failed");
    }
}

vid_get_sync_absent(pr)
    struct pixrect *pr;
{
    int absent;

    if (ioctl(get_tv1_data(pr)->fd, TVIOGSYNCABSENT, &absent) == -1) {
	perror("TVIOGSYNCABSENT failed");
    }
    return(absent);
}

vid_get_burst_absent(pr)
    struct pixrect *pr;
{
    int absent;

    if (ioctl(get_tv1_data(pr)->fd, TVIOGBURSTABSENT, &absent) == -1) {
	perror("TVIOGBURSTABSENT failed");
    }
    return(absent);
}

vid_set_loopback_cal(pr)
    struct pixrect *pr;
{
    if (ioctl(get_tv1_data(pr)->fd, TVIOSLOOPBACKCAL) == -1) {
	perror("TVIOSLOOPBACKCAL failed");
    }
}
