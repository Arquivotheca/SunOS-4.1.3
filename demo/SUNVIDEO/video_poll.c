
#ifndef lint
static char sccsid[] = "@(#)video_poll.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

#include <sys/types.h>
#include <sunwindow/notify.h>
#include <suntool/sunview.h>
#include <video.h>

static char label_string[81];

Notify_value
video_poll(client, which)
    Notify_client client;
    int which;
{
    static int prev_burst = -1;
    static int prev_sync = -1;
    static char label_string[81];
    int burst, sync, format;

    extern Window video_window, video_frame;
    
    format = (int)window_get(video_window, VIDEO_INPUT);
    /* RGB and YUV don't supply burst, so check format beforehand */
    if ((format == VIDEO_NTSC) || (format == VIDEO_YC)) {
	burst = (int)window_get(video_window, VIDEO_BURST_ABSENT);
    } else {
	burst = 0;
    }
    sync = (int)window_get(video_window, VIDEO_SYNC_ABSENT);
    format = (int)window_get(video_window, VIDEO_INPUT);
    if ( (burst != prev_burst) || (sync != prev_sync) ) {
	sprintf(label_string, "Video  %s  %s",
		(sync ? "Sync Absent" : "           "),
		(burst ? "Burst Absent" : "            "));
	window_set(video_frame, FRAME_LABEL, label_string, 0);
	prev_burst = burst;
	prev_sync = sync;
    }
    return(NOTIFY_DONE);
}
