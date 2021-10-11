/*	@(#)llib-lsunwindow.c 1.1 92/07/30 SMI	*/
/*LINTLIBRARY*/

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sundev/kbd.h>
#include <varargs.h>
#include <stdio.h>
#include <sunwindow/attr.h>
#include <sunwindow/defaults.h>
#include <sunwindow/filter.h>
#include <sunwindow/io_stream.h>
#include <sunwindow/notify.h>
#include <sunwindow/sun.h>
#include <sunwindow/string_utils.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_lock.h>
#include <sunwindow/win_enum.h>

Bool	defaults_get_boolean(path_name,default_bool,status) 
			char *path_name; Bool default_bool; int *status;
			{ return defaults_get_boolean(path_name,default_bool,
				status); }
int 	defaults_get_character(path_name,default_character,status) 
			char *path_name; char default_character; int *status;
			{ return defaults_get_character(path_name,
				default_character,status); }
int 	defaults_get_enum(path_name,pairs,status) 
			char *path_name; struct _default_pairs *pairs; 
			int *status;
			{ return defaults_get_enum(path_name,pairs,status); }
void 	defaults_set_character(path_name,value,status) 
			char *path_name; char value; int *status; {;}
void 	defaults_set_enumeration(path_name,value,status) 
			char *path_name; char *value; int *status; {;}
void 	defaults_set_integer(path_name,value,status) 
			char *path_name; int value; int *status; {;}
void 	defaults_set_string(path_name,value,status) 
			char *path_name; char *value; int *status; {;}
void 	defaults_special_mode() {;}
int 	defaults_get_integer(path_name,default_integer,status) 
			char *path_name; int default_integer; int *status;
			{ return defaults_get_integer(path_name,default_integer,				status); }
int 	defaults_get_integer_check(path_name,default_int,minimum,maximum,status)			char *path_name; int default_int; int minimum; 
			int maximum; int *status;
			{ return defaults_get_integer_check(path_name,
				default_int,minimum,maximum,status); }
char *	defaults_get_string(path_name,default_string,status) 
			char *path_name; char *default_string; int *status;
			{ return defaults_get_string(path_name,default_string,
				status); }

struct itimerval	NOTIFY_NO_ITIMER;
struct itimerval	NOTIFY_POLLING_ITIMER;

Notify_error	notify_errno;

Notify_error	notify_die(status) Destroy_status status;
			{ return (NOTIFY_OK); }
Notify_error	notify_do_dispatch() { return (NOTIFY_OK); }
Notify_func	notify_get_destroy_func(client) Notify_client client;
			{ return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_event_func(client, when) Notify_client client;
			Notify_event_type when; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_exception_func(client, fd) Notify_client client;
			int fd; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_input_func(client, fd) Notify_client client;
			int fd; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_itimer_func(client, which) Notify_client client;
			int which; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_output_func(client, fd) Notify_client client;
			int fd; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_prioritizer_func(client) Notify_client client;
			{ return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_signal_func(client, signal, mode)
			Notify_client client; int signal;
			Notify_signal_mode mode; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_get_wait3_func(client, pid) Notify_client client;
			int pid; { return (NOTIFY_FUNC_NULL); }
Notify_error	notify_start() { return (NOTIFY_OK); }
Notify_error	notify_stop() { return (NOTIFY_OK); }
void		notify_set_signal_check(tv) struct timeval tv; { return; }
struct timeval	notify_get_signal_check() { struct timeval tv; return (tv); }
int		notify_get_signal_code() { return (0); }
struct sigcontext *	notify_get_signal_context()
			{ return ((struct sigcontext *)0); }
Notify_error	notify_no_dispatch() { return (NOTIFY_OK); }
Notify_error	notify_post_destroy(client, status, when) Notify_client client;
			Destroy_status status; Notify_event_type when;
			{ return (NOTIFY_OK); }
Notify_error	notify_post_event(client, event, when_hint)
			Notify_client client; Notify_event event;
			Notify_event_type when_hint; { return (NOTIFY_OK); }
Notify_error	notify_post_event_and_arg(client, event, when_hint, arg,
			copy_func, release_func) Notify_client client;
			Notify_event event; Notify_event_type when_hint;
			Notify_arg arg; Notify_copy copy_func;
			Notify_release release_func; { return (NOTIFY_OK); }
Notify_error	notify_remove(client) Notify_client client;
			{ return (NOTIFY_OK); }
Notify_func	notify_set_destroy_func(client, func) Notify_client client;
			Notify_func func; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_event_func(client, func, when)
			Notify_client client; Notify_func func;
			Notify_event_type when; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_exception_func(client, func, fd)
			Notify_client client; Notify_func func;
			int fd; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_input_func(client, func, fd) Notify_client client;
			Notify_func func; int fd; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_itimer_func(client, func, which, value, ovalue)
			Notify_client client; Notify_func func; int which;
			struct itimerval *value, *ovalue;
			{ return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_output_func(client, func, fd) Notify_client client;
			Notify_func func; int fd; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_prioritizer_func(client, func)
			Notify_client client; Notify_func func;
			{ return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_signal_func(client, func, sig, mode)
			Notify_client client; Notify_func func; int sig;
			Notify_signal_mode mode; { return (NOTIFY_FUNC_NULL); }
Notify_func	notify_set_wait3_func(client, func, pid) Notify_client client;
			Notify_func func; int pid;
			{ return (NOTIFY_FUNC_NULL); }
Notify_error	notify_dispatch() { return (NOTIFY_OK); }
Notify_error	notify_itimer_value(client, which, value) Notify_client client;
			int which; struct itimerval *value;
			{ return (NOTIFY_OK); }
Notify_error	notify_veto_destroy(client) Notify_client client;
			{ return (NOTIFY_OK); }
Notify_value	notify_default_wait3(client, pid, status, rusage)
			Notify_client client; int pid; union wait *status;
			struct rusage *rusage; { return (NOTIFY_DONE); }
void		notify_flush_pending(client) Notify_client client; { return; }
Notify_error	notify_client(client) Notify_client client;
			{ return (NOTIFY_OK); }
Notify_error	notify_input(client, fd) Notify_client client; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_output(client, fd) Notify_client client; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_exception(client, fd) Notify_client client; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_itimer(client, which) Notify_client client; int which;
			{ return (NOTIFY_OK); }
Notify_error	notify_event(client, event, arg) Notify_client client;
			Notify_event event; Notify_arg arg;
			{ return (NOTIFY_OK); }
Notify_error	notify_signal(client, sig) Notify_client client; int sig;
			{ return (NOTIFY_OK); }
Notify_error	notify_destroy(client, status) Notify_client client;
			Destroy_status status;
			{ return (NOTIFY_OK); }
Notify_error	notify_wait3(client) Notify_client client;
			{ return (NOTIFY_OK); }
Notify_func	notify_get_scheduler_func() { return (NOTIFY_FUNC_NULL); }

Notify_func	notify_set_scheduler_func(scheduler_func)
			Notify_func scheduler_func;
			{ return (NOTIFY_FUNC_NULL); }
Notify_error	notify_interpose_destroy_func(client, func)
			Notify_client client; Notify_func func;
			{ return (NOTIFY_OK); }
Notify_error	notify_interpose_event_func(client, func, when)
			Notify_client client; Notify_func func;
			Notify_event_type when; { return (NOTIFY_OK); }
Notify_error	notify_interpose_exception_func(client, func, fd)
			Notify_client client; Notify_func func; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_interpose_input_func(client, func, fd)
			Notify_client client; Notify_func func; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_interpose_itimer_func(client, func, which)
			Notify_client client; Notify_func func; int which;
			{ return (NOTIFY_OK); }
Notify_error	notify_interpose_output_func(client, func, fd)
			Notify_client client; Notify_func func; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_interpose_signal_func(client, func, signal, mode)
			Notify_client client; Notify_func func; int signal;
			Notify_signal_mode mode; { return (NOTIFY_OK); }
Notify_error	notify_interpose_wait3_func(client, func, pid)
			Notify_client client; Notify_func func; int pid;
			{ return (NOTIFY_OK); }
Notify_value	notify_next_destroy_func(client, status)
			Notify_client client; Destroy_status status;
			{ return (NOTIFY_DONE); }
Notify_value	notify_next_event_func(client, event, arg, when)
			Notify_client client; Notify_event event;
			Notify_arg arg; Notify_event_type when;
			{ return (NOTIFY_DONE); }
Notify_value	notify_next_exception_func(client, fd)
			Notify_client client; int fd; { return (NOTIFY_DONE); }
Notify_value	notify_next_input_func(client, fd)
			Notify_client client; int fd; { return (NOTIFY_DONE); }
Notify_value	notify_next_itimer_func(client, which)
			Notify_client client; int which;
			{ return (NOTIFY_DONE); }
Notify_value	notify_next_output_func(client, fd) Notify_client client;
			int fd; { return (NOTIFY_DONE); }
Notify_value	notify_next_signal_func(client, signal, mode)
			Notify_client client; int signal;
			Notify_signal_mode mode; { return (NOTIFY_DONE); }
Notify_value	notify_next_wait3_func(client, pid, status, rusage)
			Notify_client client; int pid;
			union wait *status; struct rusage *rusage;
			{ return (NOTIFY_DONE); }
Notify_error	notify_remove_destroy_func(client, func) Notify_client client;
			Notify_func func; { return (NOTIFY_OK); }
Notify_error	notify_remove_event_func(client, func, when)
			Notify_client client; Notify_func func;
			Notify_event_type when; { return (NOTIFY_OK); }
Notify_error	notify_remove_exception_func(client, func, fd)
			Notify_client client; Notify_func func; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_remove_input_func(client, func, fd)
			Notify_client client; Notify_func func; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_remove_itimer_func(client, func, which)
			Notify_client client; Notify_func func; int which;
			{ return (NOTIFY_OK); }
Notify_error	notify_remove_output_func(client, func, fd)
			Notify_client client; Notify_func func; int fd;
			{ return (NOTIFY_OK); }
Notify_error	notify_remove_signal_func(client, func, signal, mode)
			Notify_client client; Notify_func func; int signal;
			Notify_signal_mode mode; { return (NOTIFY_OK); }
Notify_error	notify_remove_wait3_func(client, func, pid)
			Notify_client client; Notify_func func; int pid;
			{ return (NOTIFY_OK); }
Notify_value	notify_nop() { return (NOTIFY_DONE); }
void		notify_dump(client, type, file) 
			Notify_client client;
			Notify_dump_type type; FILE *file; {;}
void		notify_perror(str) char *str; {;}

	/* VARARGS2 */ 
char **	attr_create(listhead,listlen) char **listhead; int listlen;
			{ return attr_create(listhead,listlen); }
char **	attr_make_count(listhead,listlen,valist,count_ptr) char **listhead; 
			int listlen; va_list valist; int *count_ptr;
			{ return attr_make_count(listhead,listlen,valist,
				count_ptr); }
char **	attr_copy_avlist(dest,avlist) char **dest; char **avlist;
			{ return attr_copy_avlist(dest,avlist); }
int 	attr_count(avlist) char **avlist;
			{ return attr_count(avlist); }
char *	attr_sprint(s,attr) char *s; unsigned int attr;
			{ return attr_sprint(s,attr); }
void 	attr_check_pkg(last_attr,attr) unsigned int last_attr; 
			unsigned int attr; {;}
int	attr_count_avlist(avlist,last_attr) char **avlist; 
			unsigned int last_attr;
			{ return attr_count_avlist(avlist,last_attr); }
char **	attr_skip_value(attr,avlist) unsigned int attr; char **avlist;
			{ return attr_skip_value(attr,avlist); }
void 	attr_free(avlist) char **avlist; {;}
int 	attr_copy(source,dest) unsigned int **source; unsigned int **dest;
			{ return attr_copy(source,dest); }

int 	attr_cu_to_y(encoded_value,font,top_margin,row_gap) 
			unsigned int encoded_value; struct pixfont *font; 
			int top_margin; int row_gap;
			{ return attr_cu_to_y(encoded_value,font,top_margin,
				row_gap); }
int 	attr_rc_unit_to_y(encoded_value,row_height,top_margin,row_gap) 
			unsigned int encoded_value; int row_height; 
			int top_margin; int row_gap;
			{ return attr_rc_unit_to_y(encoded_value,row_height,
				top_margin,row_gap); }
int 	attr_cu_to_x(encoded_value,font,left_margin) unsigned int encoded_value;			struct pixfont *font; int left_margin;
			{ return attr_cu_to_x(encoded_value,font,left_margin); }
int 	attr_rc_unit_to_x(encoded_value,col_width,left_margin,col_gap) 
			unsigned int encoded_value; int col_width; 
			int left_margin; int col_gap;
			{ return attr_rc_unit_to_x(encoded_value,col_width,
				left_margin,col_gap); }
void 	attr_replace_cu(avlist,font,left_margin,top_margin,row_gap) 
			char **avlist; struct pixfont *font; int left_margin; 
			int top_margin; int row_gap; {;}
void 	attr_rc_units_to_pixels(avlist,col_width,row_height,left_margin,
			top_margin,col_gap,row_gap) char **avlist; 
			int col_width; int row_height; int left_margin; 
			int top_margin; int col_gap; int row_gap; {;}

/* VARARGS0 */
char **	attr_create_list()
			{ return attr_create_list(); }
char **	attr_find(attrs,attr) char **attrs; unsigned int attr;
			{ return attr_find(attrs,attr); }
int 	cmdline_scrunch(argcptr,argv) int *argcptr; char **argv;
			{ return cmdline_scrunch(argcptr,argv); }
/* VARARGS0 */
char *	cursor_create()
			{ return cursor_create(); }
void 	cursor_destroy(cursor_client) Cursor cursor_client; {;}
char *	cursor_get(cursor_client,which_attr) Cursor cursor_client; 
			Cursor_attribute which_attr;
			{ return cursor_get(cursor_client,which_attr); }
/* VARARGS1 */
int 	cursor_set(cursor_client) Cursor cursor_client;
			{ return cursor_set(cursor_client); }
char *	cursor_copy(cursor_client) Cursor cursor_client;
			{ return cursor_copy(cursor_client); }
char *	getlogindir()
			{ return getlogindir(); }
int 	pixwindebug;
struct pixwin *	pw_open(windowfd) int windowfd;
			{ return pw_open(windowfd); }

int	pw_rop(dpw, dx, dy, w, h, op, sp, sx, sy)
			struct pixwin *dpw;
			struct pixrect *sp;
			int dx, dy, w, h, op, sx, sy;
			{ return 0; }
int	pw_write(dpw, dx, dy, w, h, op, spr, sx, sy)
			struct pixwin *dpw;
			struct pixrect *spr;
			int dx, dy, w, h, op, sx, sy;
			{ return 0; }
int	pw_writebackground(dpw, dx, dy, w, h, op)
			struct pixwin *dpw;
			int dx, dy, w, h, op;
			{ return 0; }
int	pw_read(dpr, dx, dy, w, h, op, spw, sx, sy) 
			struct pixwin *spw;
			struct pixrect *dpr;
			int dx, dy, w, h, op, sx, sy;
			{ return 0; }
int	pw_copy(dpw, dx, dy, w, h, op, spw, sx, sy) 
			struct pixwin *dpw, *spw;
			int dx, dy, w, h, op, sx, sy;
			{ return 0; }
int	pw_batchrop(dpw, x, y, op, sbp, n)       
        		struct pixwin *dpw;
        		int x, y, op, n;
        		struct pr_prpos *sbp;
			{ return 0; }
int	pw_stencil(dpw, x, y, w, h, op, stpr, stx, sty, spr, sy, sx)
        		struct pixwin *dpw;
			struct pixrect *stpr, *spr;
        		int x, y, w, h, op, stx, sty, sx, sy;			
			{ return 0; }
int	pw_destroy(pw)                           
			struct pixwin *pw;
			{ return 0; }
int	pw_get(pw, x, y)                         
			struct pixwin *pw;
        		int x, y;
			{ return 0; }

int	pw_put(pw, x, y, val)                    
			struct pixwin *pw;
        		int x, y, val;
			{ return 0; }
int	pw_vector(pw, x0, y0, x1, y1, op, val)   
			struct pixwin *pw;
        		int x0, y0, x1, y1, op, val;
			{ return 0; }
struct pixwin *pw_region(pw, x, y, w, h)                
			struct pixwin *pw;
        		int x, y, w, h;
			{ return (struct pixwin *) 0; }
int	pw_putattributes(pw, planes)             
			struct pixwin *pw;
			int *planes;
			{ return 0; }
int	pw_getattributes(pw, planes)             
			struct pixwin *pw;
			int *planes;
			{ return 0; }
int	pw_putcolormap(pw, index, count, red, green, blue)          
			struct pixwin *pw;
        		int index, count;
        		unsigned char red[], green[], blue[];
			{ return 0; }
int	pw_getcolormap(pw, index, count, red, green, blue)          
			struct pixwin *pw;
        		int index, count;
        		unsigned char red[], green[], blue[];
			{ return 0; }
int	pw_lock(pixwin,rect)                     
			struct pixwin *pixwin;
			struct rect *rect;
			{ return 0; }
int	pw_unlock(pixwin)                        
			struct pixwin *pixwin;
			{ return 0; }
int	pw_reset(pixwin)                         
			struct pixwin *pixwin;
			{ return 0; }
int	pw_getclipping(pixwin)                   
			struct pixwin *pixwin;
			{ return 0; }

int 	pw_close(pw) struct pixwin *pw;
			{ return pw_close(pw); }
int 	pw_exposed(pw) struct pixwin *pw;
			{ return pw_exposed(pw); }
int 	pw_damaged(pw) struct pixwin *pw;
			{ return pw_damaged(pw); }
int 	pw_donedamaged(pw) struct pixwin *pw;
			{ return pw_donedamaged(pw); }
int 	win_getdamagedid(windowfd) int windowfd;
			{ return win_getdamagedid(windowfd); }
int 	win_getscreenposition(windowfd,x,y) int windowfd; int *x; int *y;
			{ return win_getscreenposition(windowfd,x,y); }
int 	pw_repairretained(pw) struct pixwin *pw;
			{ return pw_repairretained(pw); }
int 	pw_repair(pw,r) struct pixwin *pw; struct rect *r;
			{ return pw_repair(pw,r); }
void 	pw_batch(pw,type) struct pixwin *pw; enum pw_batch_type type; {;}
int 	pw_repair_batch(pw) struct pixwin *pw;
			{ return pw_repair_batch(pw); }
int 	pw_update_batch(pw,r) struct pixwin *pw; struct rect *r;
			{ return pw_update_batch(pw,r); }
int 	pw_reset_batch(pw) struct pixwin *pw;
			{ return pw_reset_batch(pw); }
void	pw_set_xy_offset(pw,x_offset,y_offset) struct pixwin *pw; 
			int x_offset; int y_offset; {;}
void 	pw_set_x_offset(pw,x_offset) struct pixwin *pw; int x_offset; {;}
void 	pw_set_y_offset(pw,y_offset) struct pixwin *pw; int y_offset; {;}
int	pw_get_x_offset(pw) struct pixwin *pw;
			{ return pw_get_x_offset(pw); }
int 	pw_get_y_offset(pw) struct pixwin *pw;
			{ return pw_get_y_offset(pw); }
int 	pw_setcmsname(pw,cmsname) struct pixwin *pw; char *cmsname;
			{ return pw_setcmsname(pw,cmsname); }
int 	pw_getcmsname(pw,cmsname) struct pixwin *pw; char *cmsname;
			{ return pw_getcmsname(pw,cmsname); }
int 	pw_preparesurface(pw,rect) struct pixwin *pw; struct rect *rect;
			{ return pw_preparesurface(pw,rect); }
int 	pw_blackonwhite(pw,first,last) struct pixwin *pw; int first; int last;
			{ return pw_blackonwhite(pw,first,last); }
int 	pw_whiteonblack(pw,first,last) struct pixwin *pw; int first; int last;
			{ return pw_whiteonblack(pw,first,last); }
int 	pw_reversevideo(pw,first,last) struct pixwin *pw; int first; int last;
			{ return pw_reversevideo(pw,first,last); }
int 	pw_setdefaultcms(cms,map) struct colormapseg *cms; struct cms_map *map;
			{ return pw_setdefaultcms(cms,map); }
int 	pw_getdefaultcms(cms,map) struct colormapseg *cms; struct cms_map *map;
			{ return pw_getdefaultcms(cms,map); }
struct pixwin *	pw_open_monochrome(windowfd) int windowfd;
			{ return pw_open_monochrome(windowfd); }
void 	pw_use_fast_monochrome(pw) struct pixwin *pw; {;}
void 	pw_set_plane_group_preference(plane_group) int plane_group; {;}
int 	pw_get_plane_group_preference()
			{ return pw_get_plane_group_preference(); }
int 	pw_getcmsdata(pw,cms,planes) struct pixwin *pw; 
			struct colormapseg *cms; int *planes;
			{ return pw_getcmsdata(pw,cms,planes); }
int 	win_get_plane_group(windowfd,pr) int windowfd; struct pixrect *pr;
			{ return win_get_plane_group(windowfd,pr); }
void 	win_set_plane_group(windowfd,new_plane_group) int windowfd; 
			int new_plane_group; {;}
int 	pw_setgrnd(pw,first,last,frred,frgreen,frblue,bkred,bkgreen,bkblue) 
			struct pixwin *pw; int first; int last; 
			unsigned char frred; unsigned char frgreen; 
			unsigned char frblue; unsigned char bkred; 
			unsigned char bkgreen; unsigned char bkblue;
			{ return pw_setgrnd(pw,first,last,frred,frgreen,frblue,
				bkred,bkgreen,bkblue); }
int 	fullscreen_pw_vector(pw,x0,y0,x1,y1,op,cms_index) struct pixwin *pw; 
			int x0; int y0; int x1; int y1; int op; int cms_index;
			{ return fullscreen_pw_vector(pw,x0,y0,x1,y1,op,
				cms_index); }
int 	fullscreen_pw_write(pw,xw,yw,width,height,op,pr,xr,yr) 
			struct pixwin *pw; int xw; int yw; int width; 
			int height; int op; struct pixrect *pr; int xr; int yr;
			{ return fullscreen_pw_write(pw,xw,yw,width,height,
				op,pr,xr,yr); }
void 	fullscreen_pw_copy(pw,xw,yw,width,height,op,pw_src,xr,yr) 
			struct pixwin *pw; int xw; int yw; int width; 
			int height; int op; struct pixwin *pw_src; int xr; 
			int yr; {;}
struct pw_pixel_cache *pw_save_pixels(pw,r) struct pixwin *pw; struct rect *r;
			{ return pw_save_pixels(pw,r); }
void 	pw_restore_pixels(pw,pc) struct pixwin *pw; struct pw_pixel_cache *pc; 
			{;}
int 	pwo_get(pw,x0,y0) struct pixwin *pw; int x0; int y0;
			{ return pwo_get(pw,x0,y0); }
int 	pw_line(pw,x0,y0,x1,y1,brush,tex,op) struct pixwin *pw; int x0; 
			int y0; int x1; int y1; struct pr_brush *brush; 
			Pr_texture *tex; int op;
			{ return pw_line(pw,x0,y0,x1,y1,brush,tex,op); }
int 	pw_polygon_2(pw,dx,dy,nbds,npts,vlist,op,spr,sx,sy) struct pixwin *pw; 
			int dx; int dy; int nbds; int *npts; 
			struct pr_pos *vlist; int op; struct pixrect *spr; 
			int sx; int sy;
			{ return pw_polygon_2(pw,dx,dy,nbds,npts,vlist,
				op,spr,sx,sy); }
int 	pw_polyline(pw,dx,dy,npts,ptlist,mvlist,brush,tex,op) 
			struct pixwin *pw; int dx; int dy; int npts; 
			struct pr_pos *ptlist; unsigned char *mvlist; 
			struct pr_brush *brush; struct pr_texture *tex; int op;
			{ return pw_polyline(pw,dx,dy,npts,ptlist,mvlist,
				brush,tex,op); }
int    pw_polypoint(pw, dx, dy, npts, ptlist, op)
			int dx; int dy; int npts; int op;
			Pixwin *pw; struct pr_pos *ptlist;
			{return pw_polypoint(pw, dx, dy, npts, ptlist, op);}

int 	pw_dontclipflag;
int 	pw_replrop(pw,xw,yw,width,height,op,pr,xr,yr) 
			struct pixwin *pw; int xw; int yw; int width; 
			int height; int op; struct pixrect *pr; int xr; int yr;
			{ return pw_replrop(pw,xw,yw,width,height,op,pr,xr,yr); }
int 	pw_cyclecolormap(pw,cycles,begin,length) 
			struct pixwin *pw; int cycles; int begin; int length;
			{ return pw_cyclecolormap(pw,cycles,begin,length); }
int 	pw_fork()
			{ return pw_fork(); }
int 	pw_set_shared_locking(on) int on;
			{ return pw_set_shared_locking(on); }
struct pixfont *pf_sys;
int 	pw_char(pw,xw,yw,op,pixfont,c) 
			struct pixwin *pw; int xw; int yw; int op; 
			struct pixfont *pixfont; char c;
			{ return pw_char(pw,xw,yw,op,pixfont,c); }
int 	pw_ttext(pw,xbasew,ybasew,op,pixfont,s) 
			struct pixwin *pw; int xbasew; int ybasew; 
			int op; struct pixfont *pixfont; char *s;
			{ return pw_ttext(pw,xbasew,ybasew,op,pixfont,s); }
int 	pw_text(pw,xbasew,ybasew,op,pixfont,s) 
			struct pixwin *pw; int xbasew; int ybasew; int op; 
			struct pixfont *pixfont; char *s;
			{ return pw_text(pw,xbasew,ybasew,op,pixfont,s); }
struct pixfont *pw_pfsysopen()
			{ return pw_pfsysopen(); }
int 	pw_pfsysclose()
			{ return pw_pfsysclose(); }
int 	pw_traprop(pw,dx,dy,trap,op,spr,sx,sy) 
			struct pixwin *pw; int dx; int dy; struct pr_trap trap;
			int op; struct pixrect *spr; int sx; int sy;
			{ return pw_traprop(pw,dx,dy,trap,op,spr,sx,sy); }
struct rect rect_null;
int	rect_intersection(r1,r2,r) 
			struct rect *r1; struct rect *r2; struct rect *r;
			{ return rect_intersection(r1,r2,r); }
unsigned int rect_clipvector(r,x1arg,y1arg,x2arg,y2arg) 
			struct rect *r; int *x1arg; int *y1arg; int *x2arg; 
			int *y2arg;
			{ return rect_clipvector(r,x1arg,y1arg,x2arg,y2arg); }
unsigned int rect_order(r1,r2,sortorder) 
			struct rect *r1; struct rect *r2; int sortorder;
			{ return rect_order(r1,r2,sortorder); }
struct rect rect_bounding(r1,r2) struct rect *r1; struct rect *r2;
			{ return rect_bounding(r1,r2); }
int 	rect_distance(rect,x,y,x_used,y_used) 
			struct rect *rect; int x; int y; 
			int *x_used; int *y_used;
			{ return rect_distance(rect,x,y,x_used,y_used); }
struct rectlist rl_null;
unsigned int	rl_includespoint(rl,x,y) struct rectlist *rl; short x; short y;
			{ return rl_includespoint(rl,x,y); }
int 	rl_intersection(rl1,rl2,rl) struct rectlist *rl1; struct rectlist *rl2;
			struct rectlist *rl;
			{ return rl_intersection(rl1,rl2,rl); }
int 	rl_sort(rl1,rl,sortorder) struct rectlist *rl1; struct rectlist *rl; 
			int sortorder;
			{ return rl_sort(rl1,rl,sortorder); }
int 	rl_union(rl1,rl2,rl) struct rectlist *rl1; struct rectlist *rl2; 
			struct rectlist *rl;
			{ return rl_union(rl1,rl2,rl); }
int 	rl_difference(rl1,rl2,rl) struct rectlist *rl1; struct rectlist *rl2; 
			struct rectlist *rl;
			{ return rl_difference(rl1,rl2,rl); }
unsigned int rl_empty(rl) struct rectlist *rl;
			{ return rl_empty(rl); }
unsigned int rl_equal(rl1,rl2) struct rectlist *rl1; struct rectlist *rl2;
			{ return rl_equal(rl1,rl2); }
unsigned int rl_equalrect(r,rl) struct rect *r; struct rectlist *rl;
			{ return rl_equalrect(r,rl); }
unsigned int rl_boundintersectsrect(r,rl) struct rect *r; struct rectlist *rl;
			{ return rl_boundintersectsrect(r,rl); }
unsigned int rl_rectintersects(r,rl) struct rect *r; struct rectlist *rl;
			{ return rl_rectintersects(r,rl); }
int 	rl_rectintersection(r,rl1,rl) struct rect *r; struct rectlist *rl1; 
			struct rectlist *rl;
			{ return rl_rectintersection(r,rl1,rl); }
int	rl_rectunion(r,rl1,rl) struct rect *r; struct rectlist *rl1; 
			struct rectlist *rl;
			{ return rl_rectunion(r,rl1,rl); }
int 	rl_rectdifference(r,rl1,rl) struct rect *r; struct rectlist *rl1; 
			struct rectlist *rl;
			{ return rl_rectdifference(r,rl1,rl); }
int 	rl_initwithrect(r,rl) struct rect *r; struct rectlist *rl;
			{ return rl_initwithrect(r,rl); }
int 	rl_copy(rl1,rl) struct rectlist *rl1; struct rectlist *rl;
			{ return rl_copy(rl1,rl); }
int 	rl_free(rl) struct rectlist *rl;
			{ return rl_free(rl); }
int 	rl_coalesce(rl) struct rectlist *rl;
			{ return rl_coalesce(rl); }
int 	rl_normalize(rl) struct rectlist *rl;
			{ return rl_normalize(rl); }
int 	rl_print(rl,tag) struct rectlist *rl; char *tag;
			{ return rl_print(rl,tag); }
int 	setenv(name,value) char *name; char *value;
			{ return setenv(name,value); }
int 	unsetenv(name) char *name;
			{ return unsetenv(name); }
int 	win_remove_input_device(windowfd,name) int windowfd; char *name;
			{ return win_remove_input_device(windowfd,name); }
int 	win_bell(windowfd,tv,pw) 
			int windowfd; struct timeval tv; struct pixwin *pw;
			{ return win_bell(windowfd,tv,pw); }
int 	blocking_wait(wait_tv) struct timeval wait_tv;
			{ return blocking_wait(wait_tv); }
int 	win_isblanket(windowfd) int windowfd;
			{ return win_isblanket(windowfd); }
int 	win_insertblanket(windowfd,parentfd) int windowfd; int parentfd;
			{ return win_insertblanket(windowfd,parentfd); }
int 	win_removeblanket(windowfd) int windowfd;
			{ return win_removeblanket(windowfd); }
int 	win_register(client,pw,event_func,destroy_func,flags) 
			char *client; struct pixwin *pw; 
			enum notify_value (*event_func)(); 
			enum notify_value (*destroy_func)(); unsigned int flags;
			{ return win_register(client,pw,event_func,
				destroy_func,flags); }
int 	win_unregister(client) char *client;
			{ return win_unregister(client); }
int 	win_set_flags(client,flags) char *client; unsigned int flags;
			{ return win_set_flags(client,flags); }
unsigned int 	win_get_flags(client) char *client;
			{ return win_get_flags(client); }
enum notify_error	win_post_id(client,id,when) char *client; short id; 
			enum notify_event_type when;
			{ return win_post_id(client,id,when); }
enum notify_error	win_post_id_and_arg(client,id,when,arg,
				copy_func,release_func) 
			char *client; short id; enum notify_event_type when; 
			char *arg; char *(*copy_func)(); void (*release_func)();
			{ return win_post_id_and_arg(client,id,when,arg,
				copy_func,release_func); }
enum notify_error	win_post_event(client,event,when) char *client; 
			struct inputevent *event; enum notify_event_type when;
			{ return win_post_event(client,event,when); }
enum notify_error	win_post_event_arg(client,event,when,arg,
				copy_func,release_func) 
			char *client; struct inputevent *event; 
			enum notify_event_type when; char *arg; 
			char *(*copy_func)(); void (*release_func)();
			{ return win_post_event_arg(client,event,when,arg,
				copy_func,release_func); }
char *	win_copy_event(client,arg,event_ptr) 
			char *client; char *arg; struct inputevent **event_ptr;
			{ return win_copy_event(client,arg,event_ptr); }
void 	win_free_event(client,arg,event) 
			char *client; char *arg; struct inputevent *event; {;}
int 	win_get_fd(client) char *client;
			{ return win_get_fd(client); }
struct pixwin *	win_get_pixwin(client) char *client;
			{ return win_get_pixwin(client); }
int 	pw_set_region_rect(pw,r,use_same_pr) 
			struct pixwin *pw; struct rect *r; int use_same_pr;
			{ return pw_set_region_rect(pw,r,use_same_pr); }
int 	pw_get_region_rect(pw,r) struct pixwin *pw; struct rect *r;
			{ return pw_get_region_rect(pw,r); }
int 	pw_restrict_clipping(pw,rl) struct pixwin *pw; struct rectlist *rl;
			{ return pw_restrict_clipping(pw,rl); }
int 	pw_reduce_clipping(pw,rl) struct pixwin *pw; struct rectlist *rl;
			{ return pw_reduce_clipping(pw,rl); }
int 	pw_setup_clippers(pw) struct pixwin *pw;
			{ return pw_setup_clippers(pw); }
int 	win_getscreenrect(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_getscreenrect(windowfd,rect); }
int 	pw_set_retain(pw,width,height) struct pixwin *pw; int width; int height;
			{ return pw_set_retain(pw,width,height); }
int 	win_setcms(windowfd,cms,cmap) int windowfd; 
			struct colormapseg *cms; struct cms_map *cmap;
			{ return win_setcms(windowfd,cms,cmap); }
int 	win_getcms(windowfd,cms,cmap) 
			int windowfd; struct colormapseg *cms; 
			struct cms_map *cmap;
			{ return win_getcms(windowfd,cms,cmap); }
int 	win_clearcms(windowfd) int windowfd;
			{ return win_clearcms(windowfd); }
int 	win_setmouseposition(windowfd,x,y) int windowfd; short x; short y;
			{ return win_setmouseposition(windowfd,x,y); }
int 	win_setcursor(windowfd,cursor) int windowfd; Cursor cursor;
			{ return win_setcursor(windowfd,cursor); }
int 	win_getcursor(windowfd,cursor) int windowfd; Cursor cursor;
			{ return win_getcursor(windowfd,cursor); }
int 	win_findintersect(windowfd,x,y) int windowfd; short x; short y;
			{ return win_findintersect(windowfd,x,y); }
int 	win_enumall(func) int (*func)();
			{ return win_enumall(func); }
int 	win_enumscreen(screen,func) struct screen *screen; int (*func)();
			{ return win_enumscreen(screen,func); }
enum win_enumerator_result win_enumerate_children(window,proc,args) int window; enum win_enumerator_result (*proc)(); char *args;
			{ return win_enumerate_children(window,proc,args); }
enum win_enumerator_result win_enumerate_subtree(window,proc,args) 
			int window; enum win_enumerator_result (*proc)(); 
			char *args;
			{ return win_enumerate_subtree(window,proc,args); }
int 	win_get_tree_layer(parent,size,buffer) 
			int parent; unsigned int size; char *buffer;
			{ return win_get_tree_layer(parent,size,buffer); }
int 	win_enum_many;
int 	win_enum_input_device(windowfd,func,data) 
			int windowfd; int (*func)(); char *data;
			{ return win_enum_input_device(windowfd,func,data); }
int 	we_setparentwindow(windevname) char *windevname;
			{ return we_setparentwindow(windevname); }
int 	we_getparentwindow(windevname) char *windevname;
			{ return we_getparentwindow(windevname); }
int 	we_setinitdata(initialrect,initialsavedrect,iconic) 
			struct rect *initialrect; 
			struct rect *initialsavedrect; int iconic;
			{ return we_setinitdata(initialrect,initialsavedrect,
				iconic); }
int 	we_getinitdata(initialrect,initialsavedrect,iconic) 
			struct rect *initialrect; 
			struct rect *initialsavedrect; int *iconic;
			{ return we_getinitdata(initialrect,initialsavedrect,
				iconic); }
int 	we_clearinitdata()
			{ return we_clearinitdata(); }
int 	we_setgfxwindow(windevname) char *windevname;
			{ return we_setgfxwindow(windevname); }
int 	we_getgfxwindow(windevname) char *windevname;
			{ return we_getgfxwindow(windevname); }
int 	we_setmywindow(windevname) char *windevname;
			{ return we_setmywindow(windevname); }
int 	win_getrect_from_source(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_getrect_from_source(windowfd,rect); }
int 	win_setrect_at_source(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_setrect_at_source(windowfd,rect); }
int 	win_getrect(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_getrect(windowfd,rect); }
int 	win_setrect(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_setrect(windowfd,rect); }
int 	win_setsavedrect(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_setsavedrect(windowfd,rect); }
int 	win_getsavedrect(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_getsavedrect(windowfd,rect); }
int 	win_getsize(windowfd,rect) int windowfd; struct rect *rect;
			{ return win_getsize(windowfd,rect); }
short 	win_getheight(windowfd) int windowfd;
			{ return win_getheight(windowfd); }
short 	win_getwidth(windowfd) int windowfd;
			{ return win_getwidth(windowfd); }
int 	win_screenget(windowfd,screen) int windowfd; struct screen *screen;
			{ return win_screenget(windowfd,screen); }
int 	win_lockdatadebug;
int	win_grabiodebug;
int 	win_lockdata(windowfd) int windowfd;
			{ return win_lockdata(windowfd); }
int 	win_unlockdata(windowfd) int windowfd;
			{ return win_unlockdata(windowfd); }
int 	win_computeclipping(windowfd) int windowfd;
			{ return win_computeclipping(windowfd); }
int 	win_partialrepair(windowfd,rectok) int windowfd; struct rect *rectok;
			{ return win_partialrepair(windowfd,rectok); }
int 	win_grabio(windowfd) int windowfd;
			{ return win_grabio(windowfd); }
int 	win_releaseio(windowfd) int windowfd;
			{ return win_releaseio(windowfd); }
short 	win_getfocusevent(windowfd) int windowfd;
			{ return win_getfocusevent(windowfd); }
int 	win_setfocusevent(windowfd,eventcode) int windowfd; short eventcode;
			{ return win_setfocusevent(windowfd,eventcode); }
int 	win_getinputmask(windowfd,im,nextwindownumber) 
			int windowfd; struct inputmask *im; 
			int *nextwindownumber;
			{ return win_getinputmask(windowfd,im,nextwindownumber); }
int 	win_setinputmask(windowfd,im_set,im_flush,nextwindownumber) 
			int windowfd; struct inputmask *im_set; 
			struct inputmask *im_flush; int nextwindownumber;
			{ return win_setinputmask(windowfd,im_set,im_flush,
				nextwindownumber); }
int 	input_imnull(im) struct inputmask *im;
			{ return input_imnull(im); }
int 	input_imall(im) struct inputmask *im;
			{ return input_imall(im); }
int 	input_readevent(windowfd,event) int windowfd; struct inputevent *event;
			{ return input_readevent(windowfd,event); }
int 	win_get_kbd_mask(windowfd,im) int windowfd; struct inputmask *im;
			{ return win_get_kbd_mask(windowfd,im); }
int 	win_set_kbd_mask(windowfd,im) int windowfd; struct inputmask *im;
			{ return win_set_kbd_mask(windowfd,im); }
int 	win_get_pick_mask(windowfd,im) int windowfd; struct inputmask *im;
			{ return win_get_pick_mask(windowfd,im); }
int 	win_set_pick_mask(windowfd,im) int windowfd; struct inputmask *im;
			{ return win_set_pick_mask(windowfd,im); }
int 	win_get_designee(windowfd,nextwindownumber) 
			int windowfd; int *nextwindownumber;
			{ return win_get_designee(windowfd,nextwindownumber); }
int 	win_set_designee(windowfd,nextwindownumber) 
			int windowfd; int nextwindownumber;
			{ return win_set_designee(windowfd,nextwindownumber); }
int 	win_get_focus_event(windowfd,fe,shifts) 
			int windowfd; struct firm_event *fe; int *shifts;
			{ return win_get_focus_event(windowfd,fe,shifts); }
int 	win_set_focus_event(windowfd,fe,shifts) 
			int windowfd; struct firm_event *fe; int shifts;
			{ return win_set_focus_event(windowfd,fe,shifts); }
int 	win_get_swallow_event(windowfd,fe,shifts)
			int windowfd; struct firm_event *fe; int *shifts;
			{ return win_get_swallow_event(windowfd,fe,shifts); }
int 	win_set_swallow_event(windowfd,fe,shifts) 
			int windowfd; struct firm_event *fe; int shifts;
			{ return win_set_swallow_event(windowfd,fe,shifts); }
int 	win_get_event_timeout(windowfd,tv) int windowfd; struct timeval *tv;
			{ return win_get_event_timeout(windowfd,tv); }
int 	win_set_event_timeout(windowfd,tv) int windowfd; struct timeval *tv;
			{ return win_set_event_timeout(windowfd,tv); }
int 	win_get_vuid_value(windowfd,id) int windowfd; unsigned short id;
			{ return win_get_vuid_value(windowfd,id); }
int 	win_refuse_kbd_focus(windowfd) int windowfd;
			{ return win_refuse_kbd_focus(windowfd); }
int 	win_release_event_lock(windowfd) int windowfd;
			{ return win_release_event_lock(windowfd); }
int 	win_set_kbd_focus(windowfd,number) int windowfd; int number;
			{ return win_set_kbd_focus(windowfd,number); }
int 	win_get_kbd_focus(windowfd) int windowfd;
			{ return win_get_kbd_focus(windowfd); }
int 	win_get_button_order(windowfd) int windowfd;
			{ return win_get_button_order(windowfd); }
int 	win_set_button_order(windowfd,number) int windowfd; int number;
			{ return win_set_button_order(windowfd,number); }
int 	win_get_scaling(windowfd,buffer) int windowfd; Ws_scale_list *buffer;
			{ return win_get_scaling(windowfd,buffer); }
int 	win_set_scaling(windowfd,buffer) int windowfd; Ws_scale_list *buffer;
			{ return win_set_scaling(windowfd,buffer); }
int 	win_is_input_device(windowfd,name) int windowfd; char *name;
			{ return win_is_input_device(windowfd,name); }
int 	win_getuserflags(windowfd) int windowfd;
			{ return win_getuserflags(windowfd); }
int 	win_setuserflags(windowfd,flags) int windowfd; int flags;
			{ return win_setuserflags(windowfd,flags); }
int 	win_getowner(windowfd) int windowfd;
			{ return win_getowner(windowfd); }
int 	win_setowner(windowfd,pid) int windowfd; int pid;
			{ return win_setowner(windowfd,pid); }
int 	(*win_error)();
int 	werror(errnum,winopnum) int errnum; int winopnum;
			{ return werror(errnum,winopnum); }
int 	(*win_errorhandler(win_errornew))() int (*win_errornew)();
			{ return win_errorhandler(win_errornew); }
int 	win_errordefault(errnum,winopnum) int errnum; int winopnum;
			{ return win_errordefault(errnum,winopnum); }
int 	win_setuserflag(windowfd,flag,value) 
			int windowfd; int flag; unsigned int value;
			{ return win_setuserflag(windowfd,flag,value); }
int 	winscreen_print;
int 	win_screennew(screen) struct screen *screen;
			{ return win_screennew(screen); }
int 	win_screendestroy(windowfd) int windowfd;
			{ return win_screendestroy(windowfd); }
int 	win_setscreenpositions(windowfd,neighbors) int windowfd; int *neighbors;
			{ return win_setscreenpositions(windowfd,neighbors); }
int 	win_getscreenpositions(windowfd,neighbors) int windowfd; int *neighbors;
			{ return win_getscreenpositions(windowfd,neighbors); }
int 	win_setkbd(windowfd,screen) int windowfd; struct screen *screen;
			{ return win_setkbd(windowfd,screen); }
int 	win_setms(windowfd,screen) int windowfd; struct screen *screen;
			{ return win_setms(windowfd,screen); }
int 	win_initscreenfromargv(screen,argv) struct screen *screen; char **argv;
			{ return win_initscreenfromargv(screen,argv); }
int 	win_getsinglecolor(argsptr,scolor) 
			char ***argsptr; struct singlecolor *scolor;
			{ return win_getsinglecolor(argsptr,scolor); }
int 	win_screenadj(base,neighbors,updateneighbors) 
			struct screen *base; struct screen *neighbors; 
			int updateneighbors;
			{ return win_screenadj(base,neighbors,updateneighbors); }
int 	win_set_input_device(windowfd,inputfd,name) 
			int windowfd; int inputfd; char *name;
			{ return win_set_input_device(windowfd,inputfd,name); }
int 	win_getlink(windowfd,linkname) int windowfd; int linkname;
			{ return win_getlink(windowfd,linkname); }
int 	win_setlink(windowfd,linkname,number) 
			int windowfd; int linkname; int number;
			{ return win_setlink(windowfd,linkname,number); }
int 	win_insert(windowfd) int windowfd;
			{ return win_insert(windowfd); }
int 	win_remove(windowfd) int windowfd;
			{ return win_remove(windowfd); }
int 	win_nextfree(windowfd) int windowfd;
			{ return win_nextfree(windowfd); }
int 	win_numbertoname(number,name) int number; char *name;
			{ return win_numbertoname(number,name); }
int 	win_nametonumber(name) char *name;
			{ return win_nametonumber(name); }
int 	win_fdtoname(windowfd,name) int windowfd; char *name;
			{ return win_fdtoname(windowfd,name); }
int 	win_fdtonumber(windowfd) int windowfd;
			{ return win_fdtonumber(windowfd); }
int 	win_getnewwindow()
			{ return win_getnewwindow(); }
int 	we_getmywindow(windevname) char *windevname;
			{ return we_getmywindow(windevname); }


