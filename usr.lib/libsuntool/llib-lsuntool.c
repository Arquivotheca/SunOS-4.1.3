/*	@(#)llib-lsuntool.c 1.1 92/07/30 SMI	*/
/*LINTLIBRARY*/
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/ttychars.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/errno.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <strings.h>
#include <utmp.h>
#include <pwd.h>
#include <ctype.h>
#include <sgtty.h>
#include <varargs.h>
#include <sunwindow/sun.h>
#include <sunwindow/attr.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/defaults.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_enum.h>
#include <suntool/sunview.h>
#include <suntool/menu.h>
#include <suntool/emptysw.h>
#include <suntool/msgsw.h>
#include <suntool/tool_struct.h>
#undef RESET
#undef LF
#include <suntool/panel.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/wmgr.h>
#include <suntool/icon_load.h>
#include <suntool/fullscreen.h>
#include <suntool/ttysw.h>
#include <suntool/tty.h>
#include <suntool/canvas.h>
#undef WIN_ATTR
#undef WIN_ATTR_LIST
#include <suntool/wmgr_policy.h>
#undef ord
#include <suntool/selection_impl.h>
#undef RESET
#undef LF
#include <suntool/scrollbar_impl.h>
#include <suntool/panel_impl.h>
#undef set
#include <suntool/entity_view.h>
#include <suntool/primal.h>
#include <suntool/ev_impl.h>
#include <suntool/textsw_impl.h>
#include <suntool/finger_table.h>
#include <suntool/ps_impl.h>

#include <suntool/ttysw_impl.h>
#include <suntool/ttytlsw_impl.h>
#undef CTRL
#undef NUL
#include <suntool/ttyansi.h>
#include <suntool/charimage.h>
#include <suntool/charscreen.h>


Notify_value 	msgsw_event(msgsw,event,arg,type) 
		Msgsw *msgsw; Event *event; Notify_arg arg; 
		Notify_event_type type; 
		{ return msgsw_event(msgsw,event,arg,type); }

Notify_value msgsw_destroy(msgsw,status) 
		Msgsw *msgsw; Destroy_status status; 
		{ return msgsw_destroy(msgsw,status); }

Msgsw *msgsw_create(tool,name,width,height,string,font) 
		struct tool *tool; char *name; short width; short height; 
		char *string; struct pixfont *font; 
		{ return msgsw_create(tool,name,width,height,string,font); }

Msgsw *msgsw_init(windowfd,string,font) 
		int windowfd; char *string; struct pixfont *font; 
		{ return msgsw_init(windowfd,string,font); }

int msgsw_done(msgsw) 
		Msgsw *msgsw; 
		{ return msgsw_done(msgsw); }
	
int msgsw_display(msgsw) 
		Msgsw *msgsw; 
		{ return msgsw_display(msgsw); }

int msgsw_setstring(msgsw,string) 
		Msgsw *msgsw; char *string; 
		{ return msgsw_setstring(msgsw,string); }

struct toolsw *msgsw_createtoolsubwindow(tool,name,width,height,string,font) 
		struct tool *tool; char *name; short width; short height; 
		char *string; struct pixfont *font; 
		{ return msgsw_createtoolsubwindow(tool,name,width,height,string,font); }


Emptysw *esw_begin(tool,name,width,height,string,font) 
		struct tool *tool; char *name; short width; short height; 
		char *string; struct pixfont *font; 
		{ return esw_begin(tool,name,width,height,string,font); }
		
Emptysw *esw_init(windowfd) 
		int windowfd; 
		{ return esw_init(windowfd); }
		
int esw_done(esw) 
		Emptysw *esw; 
		{ return esw_done(esw); }
		
struct toolsw *esw_createtoolsubwindow(tool,name,width,height) 
		struct tool *tool; char *name; short width; short height; 
		{ return esw_createtoolsubwindow(tool,name,width,height); }
		
struct selection selnull;

int selection_set(sel,sel_write,sel_clear,windowfd) 
		struct selection *sel; int (*sel_write)(); int (*sel_clear)();
		 int windowfd; 
		 { return selection_set(sel,sel_write,sel_clear,windowfd); }
		 
int selection_get(sel_read,windowfd) 
		int (*sel_read)(); int windowfd; 
		{ return selection_get(sel_read,windowfd); }
		
int selection_clear(windowfd) 
		int windowfd; 
		{ return selection_clear(windowfd); }
		
char *selection_filename() 
		{ return selection_filename(); }
		
int tool_select(tool,waitprocessesdie) 
		struct tool *tool; int waitprocessesdie; 
		{ return tool_select(tool,waitprocessesdie); }
		
int tool_sigchld(tool) 
		struct tool *tool; 
		{ return tool_sigchld(tool); }
		
int tool_sigwinch(tool) 
		struct tool *tool; 
		{ return tool_sigwinch(tool); }
		
struct gfxsubwindow *gfxsw_init(windowfd,argv) 
		int windowfd; char **argv; 
		{ return gfxsw_init(windowfd,argv); }
		
int gfxsw_handlesigwinch(gfx) 
		struct gfxsubwindow *gfx; 
		{ return gfxsw_handlesigwinch(gfx); }
		
int gfxsw_done(gfx) 
		struct gfxsubwindow *gfx; 
		{ return gfxsw_done(gfx); }
		
struct toolsw *gfxsw_createtoolsubwindow(tool,name,width,height,argv) 
		struct tool *tool; char *name; short width; short height; 
		char **argv; 
		{ return gfxsw_createtoolsubwindow(tool,name,width,height,argv); }
		
int gfxsw_getretained(gfx) 
		struct gfxsubwindow *gfx; 
		{ return gfxsw_getretained(gfx); }
		
int gfxsw_interpretesigwinch(gfx) 
		struct gfxsubwindow *gfx; 
		{ return gfxsw_interpretesigwinch(gfx); }
		
int gfxsw_select(gfx,selected,ibits,obits,ebits,timer) 
		struct gfxsubwindow *gfx; int (*selected)(); int ibits; 
		int obits; int ebits; struct timeval *timer; 
		{ return gfxsw_select(gfx,selected,ibits,obits,ebits,timer); }
		
int gfxsw_selected(tool,ibits,obits,ebits,timer) 
		struct tool *tool; int *ibits; int *obits; int *ebits; 
		struct timeval **timer; 
		{ return gfxsw_selected(tool,ibits,obits,ebits,timer); }
		
int gfxsw_selectdone(gfx) 
		struct gfxsubwindow *gfx; 
		{ return gfxsw_selectdone(gfx); }
		
struct cursor blank_cursor;

int gfxsw_notusingmouse(gfx) 
		struct gfxsubwindow *gfx; 
		{ return gfxsw_notusingmouse(gfx); }

int gfxsw_setinputmask(gfx,im_set,im_flush,nextwindownumber,usems,usekbd) 
		struct gfxsubwindow *gfx; struct inputmask *im_set; 
		struct inputmask *im_flush; int nextwindownumber; int usems; 
		int usekbd; 
		{ return gfxsw_setinputmask(gfx,im_set,im_flush,
			nextwindownumber,usems,usekbd); }
			
int gfxsw_inputinterrupts(gfx,ie) 
		struct gfxsubwindow *gfx; struct inputevent *ie; 
		{ return gfxsw_inputinterrupts(gfx,ie); }
		
int initrandom(r) 
		int r; 
		{ return initrandom(r); }
		
int r(i,j) 
		int i; int j; 
		{ return r(i,j); }
		
int sqroot(a) 
		int a; 
		{ return sqroot(a); }
		
int tool_layoutsubwindows(tool) 
		struct tool *tool; 
		{ return tool_layoutsubwindows(tool); }
		
int tool_positionsw(tool,swprevious,width,height,rect) 
		struct tool *tool; struct toolsw *swprevious; int width; 
		int height; struct rect *rect; 
		{ return tool_positionsw(tool,swprevious,width,height,rect); }
		
short tool_stripeheight(tool) 
		struct tool *tool; 
		{ return tool_stripeheight(tool); }
		
short tool_borderwidth(tool) 
		struct tool *tool; 
		{ return tool_borderwidth(tool); }
		
short tool_subwindowspacing(tool) 
		struct tool *tool; 
		{ return tool_subwindowspacing(tool); }
		
int fullscreendebug;
struct fullscreen *fsglobal;
struct fullscreen *fullscreen_init(windowfd) 
		int windowfd; 
		{ return fullscreen_init(windowfd); }

int fullscreen_destroy(fs) 
		struct fullscreen *fs; 
		{ return fullscreen_destroy(fs); }
		
struct cursor menu_cursor;

struct menuitem *menu_display(menuptr,inputevent,iowindowfd) 
		struct menu **menuptr; Event *inputevent; int iowindowfd; 
		{ return menu_display(menuptr,inputevent,iowindowfd); }
		
struct pixrect *save_bits(pixwin,r) 
		struct pixwin *pixwin; struct rect *r; 
		{ return save_bits(pixwin,r); }
		
int restore_bits(pixwin,r,mpr) 
		struct pixwin *pixwin; struct rect *r; struct pixrect *mpr; 
		{ return restore_bits(pixwin,r,mpr); }
		
int menu_prompt(prompt,inputevent,iowindowfd) 
		struct prompt *prompt; struct inputevent *inputevent; 
		int iowindowfd; 
		{ return menu_prompt(prompt,inputevent,iowindowfd); }

struct pixrect *tool_bkgrd;

int tool_display(tool) 
		struct tool *tool; 
		{ return tool_display(tool); }
		
int tool_displayicon(tool) 
		struct tool *tool; 
		{ return tool_displayicon(tool); }
		
int tool_displaynamestripe(tool) 
		struct tool *tool; 
		{ return tool_displaynamestripe(tool); }
		
int tool_displaytoolonly(tool) 
		struct tool *tool; 
		{ return tool_displaytoolonly(tool); }
		
int _tool_displaydefaulticon(tool) 
		struct tool *tool; 
		{ return _tool_displaydefaulticon(tool); }
		
int formatstringtorect(pixwin,s,font,rect) 
		struct pixwin *pixwin; char *s; struct pixfont *font; 
		struct rect *rect; 
		{ return formatstringtorect(pixwin,s,font,rect); }
		
int draw_box(pixwin,op,r,w,color) 
		struct pixwin *pixwin; int op; struct rect *r; int w; 
		int color; 
		{ return draw_box(pixwin,op,r,w,color); }
		
struct toolsw *tool_find_sw_with_client(tool,client) 
		struct tool *tool; caddr_t client; 
		{ return tool_find_sw_with_client(tool,client); }
		
int tool_kbd_use(tool,client) 
		struct tool *tool; caddr_t client; 
		{ return tool_kbd_use(tool,client); }
		
int tool_kbd_done(tool,client) 
		struct tool *tool; caddr_t client; 
		{ return tool_kbd_done(tool,client); }
		
int tool_is_exposed(toolfd) 
		int toolfd; 
		{ return tool_is_exposed(toolfd); }
		
int rootfd_for_toolfd(toolfd) 
		int toolfd; 
		{ return rootfd_for_toolfd(toolfd); }
		
Notify_value tool_input(tool,event,arg,type) 
		Tool *tool; Event *event; Notify_arg arg; 
		Notify_event_type type; 
		{ return tool_input(tool,event,arg,type); }
		
struct cursor confirm_cursor;
struct cursor move_cursor;
struct cursor moveH_cursor;
struct cursor moveV_cursor;
struct cursor stretchMID_cursor;
struct cursor stretchE_cursor;
struct cursor stretchW_cursor;
struct cursor stretchN_cursor;
struct cursor stretchS_cursor;
struct cursor stretchNW_cursor;
struct cursor stretchNE_cursor;
struct cursor stretchSE_cursor;
struct cursor stretchSW_cursor;

Menu tool_walkmenu(tool) 
		Tool *tool; 
		{ return tool_walkmenu(tool); }
		
Menu_item tool_menu_zoom_state(mi,op) 
		Menu_item mi; 
		Menu_generate op; 
		{ return tool_menu_zoom_state(mi,op); }
		
Menu_item tool_menu_open_state(mi,op) 
		Menu_item mi; Menu_generate op; 
		{ return tool_menu_open_state(mi,op); }
		
void tool_menu_open(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_move(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_cmove(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_stretch(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_cstretch(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_zoom(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_fullscreen(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_expose(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_hide(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_redisplay(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void tool_menu_quit(menu,mi) 
		Menu menu; Menu_item mi; 
		{;}
		
void wmgr_fullscreen(tool,rootfd) 
		Tool *tool; int rootfd; 
		{;}
		
int WMGR_STATE_POS;
int WMGR_MOVE_POS;
int WMGR_STRETCH_POS;
int WMGR_TOP_POS;
int WMGR_BOTTOM_POS;
int WMGR_REFRESH_POS;
int WMGR_DESTROY_POS;
struct menu *wmgr_toolmenu;

void wmgr_open(toolfd,rootfd) 
		int toolfd; int rootfd; 
		{;}
		
void wmgr_close(toolfd,rootfd) 
		int toolfd; int rootfd; 
		{;}
		
void wmgr_move(toolfd) 
		int toolfd; 
		{;}
		
void wmgr_stretch(toolfd) 
		int toolfd; 
		{;}
		
void wmgr_top(toolfd,rootfd) 
		int toolfd; int rootfd; 
		{;}
		
void wmgr_bottom(toolfd,rootfd) 
		int toolfd; int rootfd; 
		{;}
		
int wmgr_handletoolmenuitem(menu,mi,toolfd,rootfd) 
		struct menu *menu; struct menuitem *mi; int toolfd; int rootfd;
		{ return wmgr_handletoolmenuitem(menu,mi,toolfd,rootfd); }
		
void wmgr_setupmenu(windowfd) 
		int windowfd; 
		{;}
		
int wmgr_forktool(programname,otherargs,rectnormal,recticon,iconic) 
		char *programname; char *otherargs; struct rect *rectnormal; 
		struct rect *recticon; int iconic; 
		{ return wmgr_forktool(programname,otherargs,rectnormal,
			recticon,iconic); }
			
int wmgr_confirm(windowfd,text) 
		int windowfd; char *text; 
		{ return wmgr_confirm(windowfd,text); }
		
int constructargs(args,programname,otherargs,maxargcount) 
		char **args; char *programname; char *otherargs; 
		int maxargcount; 
		{return constructargs(args,programname,otherargs,maxargcount);}

void wmgr_changerect(feedbackfd,windowfd,event,move,accelerated) 
		int feedbackfd; int windowfd; struct inputevent *event; 
		int move; int accelerated; 
		{;}
		
void wmgr_winandchildrenexposed(pixwin,rl) 
		struct pixwin *pixwin; struct rectlist *rl; 
		{;}
		
void wmgr_completechangerect(windowfd,rectnew,rectoriginal,parentprleft,
	parentprtop) 
		int windowfd; struct rect *rectnew; struct rect *rectoriginal;
		int parentprleft; int parentprtop; 
		{;}
		
void wmgr_refreshwindow(windowfd) 
		int windowfd; 
		{;}
		
void wmgr_providefeedback(feedbackfd,r,event,is_move,is_iconic,grasp,
	bounds_rect,adjust_position,swsp,obstacles) 
		int feedbackfd; Rect *r; Event *event; int is_move; 
		int is_iconic; WM_Direction grasp; Rect *bounds_rect; 
		int (*adjust_position)(); int swsp; Rectlist *obstacles; 
		{;}
		
int wmgr_get_placeholders(irect,orect) 
		struct rect *irect; struct rect *orect;
		{ return wmgr_get_placeholders(irect,orect); }
		
int wmgr_set_placeholders(irect,orect) 
		struct rect *irect; struct rect *orect; 
		{ return wmgr_set_placeholders(irect,orect); }
		
int wmgr_setrectalloc(rootfd,tool_left,tool_top,icon_left,icon_top) 
		int rootfd; short tool_left; short tool_top; short icon_left; 
		short icon_top; 
		{ return wmgr_setrectalloc(rootfd,tool_left,tool_top,icon_left,
			icon_top); }
		
int wmgr_getrectalloc(rootfd,tool_left,tool_top,icon_left,icon_top) 
		int rootfd; short *tool_left; short *tool_top; 
		short *icon_left; short *icon_top; 
		{ return wmgr_getrectalloc(rootfd,tool_left,tool_top,
			icon_left,icon_top); }
			
int wmgr_figureiconrect(rootfd,rect) 
		int rootfd; struct rect *rect; 
		{ return wmgr_figureiconrect(rootfd,rect); }
		
int wmgr_figuretoolrect(rootfd,rect) 
		int rootfd; struct rect *rect; 
		{ return wmgr_figuretoolrect(rootfd,rect); }
		
int wmgr_constrainrect(rconstrain,rbound,dx,dy) 
		struct rect *rconstrain; struct rect *rbound; int dx; int dy; 
		{ return wmgr_constrainrect(rconstrain,rbound,dx,dy); }
		
int fullscreen_not_draw_box(pixwin,r,w) 
		struct pixwin *pixwin; struct rect *r; int w; 
		{ return fullscreen_not_draw_box(pixwin,r,w); }
		
int wmgr_background_close(windowfd) 
		int windowfd; 
		{ return wmgr_background_close(windowfd); }
		
void wmgr_changestate(windowfd,rootfd,close) 
		int windowfd; int rootfd; int close; 
		{;}
		
int wmgr_iswindowopen(windowfd) 
		int windowfd; 
		{ return wmgr_iswindowopen(windowfd); }
		
void wmgr_changelevel(windowfd,parentfd,top) 
		int windowfd; int parentfd; int top; 
		{;}
		
void wmgr_changelevelonly(windowfd,parentfd,top) 
		int windowfd; int parentfd; int top; 
		{;}
		
void wmgr_full(tool,rootfd) 
		struct tool *tool; int rootfd; 
		{;}
		
int tool_moveboundary(tool,event) 
		Tool *tool; Event *event; 
		{ return tool_moveboundary(tool,event); }
		
void tool_expand_neighbors(tool,target_sw,old_rect) 
		Tool *tool; Toolsw *target_sw; Rect *old_rect;
		{;}
	
void tool_compute_constraint(tool,target_sw,rconstrain) 
		Tool *tool; Toolsw *target_sw; Rect *rconstrain; 
		{;}
		
struct tool *tool_create(name,flags,tool_rect,icon) 
		char *name; int flags; struct rect *tool_rect; 
		struct icon *icon; 
		{ return tool_create(name,flags,tool_rect,icon); }
		
struct toolsw *tool_createsubwindow(tool,name,width,height) 
		struct tool *tool; char *name; short width; short height; 
		{ return tool_createsubwindow(tool,name,width,height); }
		
int tool_destroy(tool) 
		struct tool *tool; 
		{ return tool_destroy(tool); }
		
int tool_destroysubwindow(tool,toolsw) 
		struct tool *tool; struct toolsw *toolsw; 
		{ return tool_destroysubwindow(tool,toolsw); }
		
void icon_display(icon,pixwin,x,y) 
		struct icon *icon; struct pixwin *pixwin; int x; int y; 
		{;}
		
int tool_find_attribute(avlist,attr,v) 
		char **avlist; int attr; char **v; 
		{ return tool_find_attribute(avlist,attr,v); }
		
char *tool_get_attribute(tool,attr) 
		struct tool *tool; int attr; 
		{ return tool_get_attribute(tool,attr); }
		
int tool_parse_all(argc_ptr,argv,avlist_ptr,tool_name) 
		int *argc_ptr; char **argv; char ***avlist_ptr; 
		char *tool_name; 
		{ return tool_parse_all(argc_ptr,argv,avlist_ptr,tool_name); }
		
int tool_parse_one(argc,argv,avlist_ptr,tool_name) 
		int argc; char **argv; char ***avlist_ptr; char *tool_name; 
		{ return tool_parse_one(argc,argv,avlist_ptr,tool_name); }
		
int tool_parse_font(argc,argv) 
		int argc; char **argv; 
		{ return tool_parse_font(argc,argv); }
		
int tool_defaultlines;
int tool_defaultcolumns;

int tool_headerheight(namestripe) 
		int namestripe; 
		{ return tool_headerheight(namestripe); }
		
int tool_heightfromlines(n,namestripe) 
		int n; int namestripe; 
		{ return tool_heightfromlines(n,namestripe); }
		
int tool_widthfromcolumns(x) 
		int x; 
		{ return tool_widthfromcolumns(x); }
		
int tool_linesfromheight(tool,y) 
		struct tool *tool; int y; 
		{ return tool_linesfromheight(tool,y); }
		
int tool_columnsfromwidth(tool,x) 
		struct tool *tool; int x; 
		{ return tool_columnsfromwidth(tool,x); }
		
int tool_free_attribute(attr,v) 
		int attr; char *v; 
		{ return tool_free_attribute(attr,v); }
		
struct cursor tool_cursor;

struct tool *tool_make(va_alist) 
		int va_alist; 
		{ return tool_make(va_alist); }
		
int tool_set_attributes(tool,args) 
		struct tool *tool; char *args; 
		{ return tool_set_attributes(tool,args); }
		
int tool_cmsname(tool,name) 
		struct tool *tool; 
		char *name; 
		{ return tool_cmsname(tool,name); }
		
int tool_setgroundcolor(tool,foreground,background,makedefault) 
		struct tool *tool; struct singlecolor *foreground; 
		struct singlecolor *background; int makedefault; 
		{ return tool_setgroundcolor(tool,foreground,
			background,makedefault); }
			
int tool_usage(tool_name) 
		char *tool_name; 
		{ return tool_usage(tool_name); }
		
int tool_install(tool) 
		Tool *tool; 
		{ return tool_install(tool); }
		
int tool_remove(tool) 
		Tool *tool; 
		{ return tool_remove(tool); }
		
Notify_value tool_sw_input(sw,fd) 
		struct toolsw *sw; int fd; 
		{ return tool_sw_input(sw,fd); }
		
struct toolsw *tool_sw_from_client(tool,client) 
		Tool *tool; Notify_client client; 
		{ return tool_sw_from_client(tool,client); }
		
int tool_done_with_no_confirm(tool) 
		struct tool *tool; 
		{ return tool_done_with_no_confirm(tool); }
		
int tool_done(tool) 
		struct tool *tool; 
		{ return tool_done(tool); }
		
void tool_veto_destroy(tool) 
		Tool *tool; 
		{;}
		
void tool_confirm_destroy(tool) 
		Tool *tool; 
		{;}
		
int tool_notify_count;

struct tool *tool_begin(args) 
		char *args; 
		{ return tool_begin(args); }
		
Notify_value tool_event(tool,event,arg,type) 
		Tool *tool; Event *event; Notify_arg arg; 
		Notify_event_type type; 
		{ return tool_event(tool,event,arg,type); }
		
/* VARARGS0 */		
Icon icon_create() 
		{ return icon_create(); }
		
int icon_destroy(icon_client) 
		Icon icon_client; 
		{ return icon_destroy(icon_client); }
		
/* VARARGS1 */ 
int icon_set(icon_client) 
		Icon icon_client; 
		{ return icon_set(icon_client); }
		
caddr_t icon_get(icon_client,attr) 
		Icon icon_client; Icon_attribute attr; 
		{ return icon_get(icon_client,attr); }
		
struct pixrect confirm_pr;

int cursor_confirm(fd) 
		int fd; 
		{ return cursor_confirm(fd); }
		
FILE *icon_open_header(from_file,error_msg,info) 
		char *from_file; char *error_msg; icon_header_handle info; 
		{ return icon_open_header(from_file,error_msg,info); }
		
int icon_read_pr(fd,header,pr) 
		FILE *fd; icon_header_handle header;struct pixrect *pr;
		{ return icon_read_pr(fd,header,pr); }
		
struct pixrect *icon_load_mpr(from_file,error_msg) 
		char *from_file; char *error_msg; 
		{ return icon_load_mpr(from_file,error_msg); }
		
int icon_init_from_pr(icon,pr) 
		struct icon *icon; struct pixrect *pr; 
		{ return icon_init_from_pr(icon,pr); }
		
int icon_load(icon,from_file,error_msg) 
		struct icon *icon; char *from_file; char *error_msg; 
		{ return icon_load(icon,from_file,error_msg); }
		
struct namelist *expand_name(name) 
		char *name; 
		{ return expand_name(name); }
		
int anyof(s1,s2) 
		char *s1; char *s2; 
		{ return anyof(s1,s2); }
		
struct namelist *make_0list() 
		{ return make_0list(); }
		
struct namelist *make_1list(str) 
		char *str; 
		{ return make_1list(str); }
		
struct namelist *makelist(len,str) 
		int len; char *str; 
		{ return makelist(len,str); }
		
void free_namelist(ptr) 
		struct namelist *ptr; 
		{;}
		
void expand_path(nm,buf) 
		char *nm; char *buf; 
		{;}

Scrollbar scrollbar_build(argv) 
		caddr_t argv; 
		{ return scrollbar_build(argv); }
		
/*VARARGS0*/		
Scrollbar scrollbar_create(va_alist) 
		int va_alist;
		{ return scrollbar_create(va_alist); }
		
int scrollbar_init(sb) 
		struct scrollbar *sb; 
		{ return scrollbar_init(sb); }

/*VARARGS1*/		
int scrollbar_set(sb,argv) 
		Scrollbar sb; caddr_t *argv; 
		{ return scrollbar_set(sb,argv); }
		
caddr_t scrollbar_get(sb,attr) 
		Scrollbar sb; 
		Scrollbar_attribute attr;
		{ return scrollbar_get(sb,attr); }
		
int scrollbar_destroy(sb) 
		Scrollbar sb; 
		{ return scrollbar_destroy(sb); }
		
void scrollbar_scroll_to(sb,new_view_start) 
		Scrollbar sb; long new_view_start; 
		{;}
		
int scrollbar_paint(sb) 
		Scrollbar sb; 
		{ return scrollbar_paint(sb); }
		
int scrollbar_paint_clear(sb) 
		Scrollbar sb; 
		{ return scrollbar_paint_clear(sb); }
		
int scrollbar_clear(sb) 
		Scrollbar sb; 
		{ return scrollbar_clear(sb); }
		
int scrollbar_paint_bubble(sb) 
		Scrollbar sb; 
		{ return scrollbar_paint_bubble(sb); }
		
int scrollbar_clear_bubble(sb) 
		Scrollbar sb; 
		{ return scrollbar_clear_bubble(sb); }
		
int scrollbar_repaint(sb,status) 
		Scrollbar sb; int status; 
		{ return scrollbar_repaint(sb,status); }
		
int scrollbar_paint_buttons(sb) 
		scrollbar_handle sb; 
		{ return scrollbar_paint_buttons(sb); }
		
int scrollbar_paint_all_clear(pixwin) 
		struct pixwin *pixwin; 
		{ return scrollbar_paint_all_clear(pixwin); }
		
int scrollbar_paint_all(pixwin) 
		struct pixwin *pixwin; 
		{ return scrollbar_paint_all(pixwin); }
		
/* VARARGS2 */
void seln_init_request(buffer,holder) 
		Seln_request *buffer; Seln_holder *holder; {;}

/* VARARGS3 */		
Seln_result seln_query(holder,reader,context) 
		Seln_holder *holder; Seln_result (*reader)(); char *context; 
		{ return seln_query(holder,reader,context); }
		
void seln_report_event(seln_client,event) 
		Seln_client seln_client; struct inputevent *event; 
		{;}
		
char *seln_create(function_proc,request_proc,client_data) 
		void (*function_proc)(); Seln_result (*request_proc)(); 
		char *client_data; 
		{ return seln_create(function_proc,request_proc,client_data); }
		
void seln_destroy(client) 
		char *client; 
		{;}
		
Seln_rank seln_acquire(seln_client,asked) 
		Seln_client seln_client; Seln_rank asked; 
		{ return seln_acquire(seln_client,asked); }

/*VARARGS1*/
Seln_request *seln_ask(holder)
		Seln_holder *holder;
		{ return seln_ask(holder); }

Seln_result seln_done(seln_client,rank) 
		Seln_client seln_client; Seln_rank rank; 
		{ return seln_done(seln_client,rank); }

void seln_dump_function_buffer(stream,ptr) 
		FILE *stream; Seln_function_buffer *ptr; 
		{;}
		
void seln_dump_function_key(stream,ptr)
		FILE *stream; Seln_function *ptr;
		{;}
		
void seln_dump_file_args(stream,ptr) 
		FILE *stream; Seln_file_info *ptr; 
		{;}
		
void seln_dump_holder(stream,ptr) 
		FILE *stream; Seln_holder *ptr; 
		{;}
		
void seln_dump_inform_args(stream,ptr) 
		FILE *stream; Seln_inform_args *ptr;
		{;}
		
void seln_dump_rank(stream,ptr) 
		FILE *stream; Seln_rank *ptr; 
		{;}
		
void seln_dump_response(stream,ptr) 
		FILE *stream; 
		Seln_response *ptr; 
		{;}

void seln_dump_result(stream,ptr) 
		FILE *stream; Seln_result *ptr; 
		{;}
		
Seln_result seln_dump_service(stream,holder,rank) 
		FILE  *stream; Seln_holder *holder; Seln_rank rank; 
		{ return seln_dump_service(stream,holder,rank); }
		
void seln_dump_state(stream,ptr) 
		FILE *stream; Seln_state *ptr; 
		{;}
		
Seln_response seln_figure_response(buffer,holder) 
		Seln_function_buffer *buffer; Seln_holder **holder; 
		{ return seln_figure_response(buffer,holder); }
		
Seln_result seln_hold_file(rank,path) 
		Seln_rank rank; char *path; 
		{ return seln_hold_file(rank,path); }
		
int seln_holder_same_process(holder) 
		Seln_holder *holder; 
		{ return seln_holder_same_process(holder); }
		
int seln_holder_same_client(holder,client_data) 
		Seln_holder *holder; char *client_data; 
		{ return seln_holder_same_client(holder,client_data); }
		
void seln_yield_all() 
		{;}
		
Seln_function_buffer seln_inform(seln_client,which,down) 
		Seln_client seln_client; Seln_function which; int down; 
		{ return seln_inform(seln_client,which,down); }
		
Seln_result seln_functions_state(result) 
		Seln_functions_state *result; 
		{ return seln_functions_state(result); }
		
int seln_get_function_state(func) 
		Seln_function func; 
		{ return seln_get_function_state(func); }
		
Seln_holder seln_inquire(which) 
		Seln_rank  which; 
		{ return seln_inquire(which); }
		
Seln_holders_all seln_inquire_all() 
		{ return seln_inquire_all(); }

/*VARARGS*/	
Seln_result seln_debug() 
		{ return seln_debug(); }
		
void seln_clear_functions() 
		{;}
		
Seln_result seln_stop(auth) 
		int auth; 
		{ return seln_stop(auth); }
		
Seln_result seln_request(holder,buffer) 
		Seln_holder *holder; Seln_request *buffer; 
		{ return seln_request(holder,buffer); }
		
void seln_use_timeout(secs) 
		int secs; 
		{;}
		
void seln_use_test_service() 
		{;}
		
int seln_secondary_made(buffer) 
		Seln_function_buffer *buffer; 
		{ return seln_secondary_made(buffer); }
		
int seln_secondary_exists(buffer) 
		Seln_function_buffer *buffer; 
		{ return seln_secondary_exists(buffer); }
		
int seln_same_holder(h1,h2) 
		Seln_holder *h1; Seln_holder *h2; 
		{ return seln_same_holder(h1,h2); }
		
Seln_function_buffer seln_null_function;
Seln_holder seln_null_holder;
Seln_request seln_null_request;
/* Menu routines */
struct pixrect menu_gray25_pr;
struct pixrect menu_gray50_pr;
struct pixrect menu_gray75_pr;
/* VARARGS3 */
char *	menu_show(menu,win,iep) char *menu; char *win; struct inputevent *iep;
			{ return menu_show(menu,win,iep); }
char *	menu_show_using_fd(menu,fd,iep) char *menu; int fd;
			struct inputevent *iep;
			{ return menu_show_using_fd(menu,fd,iep); }
/* VARARGS0 */
char *	menu_create(){ return menu_create(); }
/* VARARGS0 */
char *	menu_create_item()
			{ return menu_create_item(); }
/* VARARGS1 */
int	menu_set(m) char *m; { return menu_set(m); }
/* VARARGS1 */
char *	menu_get(m) char *m; { return menu_get(m); }
void	menu_destroy(m) char *m; {;}
void	menu_destroy_with_proc(m,destroy_proc) char *m; 
	void (*destroy_proc)();
			{;}
char *	menu_pullright_return_result(menu_item) char *menu_item;
			{ return menu_pullright_return_result(menu_item); }
char *	menu_return_value(menu,menu_item) char *menu; char *menu_item;
			{ return menu_return_value(menu,menu_item); }
char *	menu_return_item(menu,menu_item) char *menu; char *menu_item;
			{ return menu_return_item(menu,menu_item); }
char *	menu_return_no_value(menu,menu_item) char *menu; char *menu_item;
			{ return menu_return_no_value(menu,menu_item); }
char *	menu_return_no_item(menu,menu_item) char *menu; char *menu_item;
			{ return menu_return_no_item(menu,menu_item); }
/* VARARGS1 */
char *	menu_find(menu) char *menu; { return menu_find(menu); }

Menu	menu_create_customizable(entries, proc, first_attr)
			char *entries; void (*proc)(); caddr_t	*first_attr;
			{ return menu_create_customizable(entries, proc,
			first_attr);}
Menu_item	menu_customizable_entry(m, entry, client_data)
			Menu m; char *entry;
			{return menu_customizable_entry(m, entry, client_data);}

/* Panel public routines */
struct pixrect panel_cycle_pr;

int panel_accept_key(client_object,event) 
		Panel_item client_object; Event *event; 
		{ return panel_accept_key(client_object,event); }

int panel_accept_menu(client_object,event) 
		Panel_item client_object; Event *event; 
		{ return panel_accept_menu(client_object,event); }

int panel_accept_preview(client_object,event) 
		Panel_item client_object; Event *event; 
		{ return panel_accept_preview(client_object,event); }

Panel_item panel_advance_caret(client_panel) 
		Panel_item client_panel; 
		{ return panel_advance_caret(client_panel); }

Panel_item panel_backup_caret(client_panel) 
		Panel_item client_panel; 
		{ return panel_backup_caret(client_panel); }

Pixrect *panel_button_image(client_object,string,width,font) 
		Panel client_object; char *string; int width; Pixfont *font; 
		{ return panel_button_image(client_object,string,width,font); }

int panel_begin_preview(object,event)
		Panel_item object; Event *event;
		{return panel_begin_preview(object,event);}

int panel_cancel_preview(client_object,event) 
		Panel_item client_object; Event *event; 
		{ return panel_cancel_preview(client_object,event); }

/* VARARGS2 */ 
Panel_item panel_create_item(client_panel,create_proc) 
		Panel client_panel; Panel_item(*create_proc)(); 
		{ return panel_create_item(client_panel,create_proc); }

int panel_destroy_item(item)
		Panel_item item;
		{return panel_destroy_item(item);}

int panel_default_handle_event(client_object,event) 
		Panel_item client_object; Event *event; 
		{ return panel_default_handle_event(client_object,event); }

Event *panel_event(client_panel,event) 
		Panel client_panel; Event *event; 
		{ return panel_event(client_panel,event); }

/* VARARGS2 */
Panel_attribute_value panel_get(client_object,attr) 
		Panel client_object; Panel_attribute attr; 
		{ return panel_get(client_object,attr); }

int panel_paint(client_object,flag) 
		Panel client_object; Panel_setting flag; 
		{ return panel_paint(client_object,flag); }

/* VARARGS1 */
int panel_set(client_object) 
		Panel client_object; 
		{ return panel_set(client_object); }

Panel_setting panel_text_notify(item,event)
		Panel_item item; Event *event;
		{ return panel_text_notify(item,event); }

int panel_update_scrolling_size(panel)
		Panel panel;
		{ return panel_update_scrolling_size(panel); }

int panel_update_preview(client_object,event) 
		Panel_item client_object; Event *event; 
		{ return panel_update_preview(client_object,event); }

Event *panel_window_event(client_panel,event) 
		Panel client_panel; Event *event; 
		{ return panel_window_event(client_panel,event); }

Textsw_mark textsw_add_mark(abstract,position,flags) 
		Textsw abstract; Textsw_index position; unsigned flags; 
		{ return textsw_add_mark(abstract,position,flags); }

int textsw_append_file_name(abstract,name) 
		Textsw abstract; char *name; 
		{ return textsw_append_file_name(abstract,name); }

Textsw_index textsw_index_for_file_line(abstract,line) 
		Textsw abstract; int line; 
		{ return textsw_index_for_file_line(abstract,line); }

void textsw_scroll_lines(abstract,count) 
		Textsw abstract; int count; 
		{;}

int textsw_set_selection(abstract,first,last_plus_one,type) 
		Textsw abstract; Textsw_index first; Textsw_index last_plus_one; 
		unsigned type; 
		{ return textsw_set_selection(abstract,first,last_plus_one,type); }

Textsw_index textsw_delete(abstract,first,last_plus_one) 
		Textsw abstract; Textsw_index first; Textsw_index last_plus_one; 
		{ return textsw_delete(abstract,first,last_plus_one); }

Textsw_index textsw_erase(abstract,first,last_plus_one) 
		Textsw abstract; Textsw_index first; Textsw_index last_plus_one; 
		{ return textsw_erase(abstract,first,last_plus_one); }

void textsw_file_lines_visible(abstract,top,bottom) 
		Textsw abstract; int *top; int *bottom; 
		{;}

int textsw_find_bytes(abstract,first,last_plus_one,buf,buf_len,flags) 
		Textsw abstract; Textsw_index *first; Textsw_index *last_plus_one; 
		char *buf; unsigned buf_len; unsigned flags; 
		{ return textsw_find_bytes(abstract,first,last_plus_one,
			buf,buf_len,flags); }

Textsw_index textsw_find_mark(abstract,mark) 
		Textsw abstract; Textsw_mark mark; 
		{ return textsw_find_mark(abstract,mark); }

Textsw textsw_first(any) 
		Textsw any; 
		{ return textsw_first(any); }

Textsw_index textsw_edit(abstract,unit,count,direction) 
		Textsw abstract; unsigned unit; unsigned count; 
		unsigned direction; 
		{ return textsw_edit(abstract,unit,count,direction); }

Textsw_index textsw_insert(abstract,buf,buf_len) 
		Textsw abstract; char *buf; long buf_len; 
		{ return textsw_insert(abstract,buf,buf_len); }

Textsw textsw_next(previous) 
		Textsw previous; 
		{ return textsw_next(previous); }

void textsw_remove_mark(abstract,mark) 
		Textsw abstract; Textsw_mark mark; 
		{;}

Textsw_index textsw_replace_bytes(abstract,first,last_plus_one,buf,buf_len) 
		Textsw abstract; Textsw_index first; Textsw_index last_plus_one; 
		char *buf; long buf_len; 
		{ return textsw_replace_bytes(abstract,first,last_plus_one,buf,
			buf_len); }

int textsw_possibly_normalize(abstract,pos) 
		Textsw abstract; Textsw_index pos; 
		{ return textsw_possibly_normalize(abstract,pos); }

int textsw_normalize_view(abstract,pos) 
		Textsw abstract; Textsw_index pos;
		{ return textsw_normalize_view(abstract,pos); }

unsigned textsw_save(abstract,locx,locy) 
		Textsw abstract; int locx; int locy; 
		{ return textsw_save(abstract,locx,locy); }

int textsw_screen_line_count(abstract) 
		Textsw abstract; 
		{ return textsw_screen_line_count(abstract); }

unsigned textsw_store_file(abstract,filename,locx,locy) 
		Textsw abstract; char *filename; int locx; int locy; 
		{ return textsw_store_file(abstract,filename,locx,locy); }

void textsw_reset(abstract,locx,locy) 
		Textsw abstract; int locx; int locy; 
		{;}

/* Ttysw routines */
struct ttysw_createoptions {
    int		  becomeconsole; /* be the console */
    char	**argv;		 /* args to be used in exec */
    char	 *args[4];	 /* scratch array if need to build argv */
};

int ttysw_output(ttysw_client,addr,len0) 
		struct ttysubwindow *ttysw_client; char *addr; int len0; 
		{ return ttysw_output(ttysw_client,addr,len0); }

int ttysw_input(tty,buf,len)
		Tty tty; char *buf; int len;
		{ return ttysw_input(tty,buf,len); }

/* Window public routines */
struct inputevent *canvas_event(canvas,event) 
		Canvas canvas; struct inputevent *event; 
		{ return canvas_event(canvas,event); }
		
struct inputevent *canvas_window_event(canvas,event) 
		Canvas canvas; struct inputevent *event; 
		{ return canvas_window_event(canvas,event); }

/*struct pixwin *canvas_pixwin(canvas)
		Canvas canvas;
		{ return canvas_pixwin(canvas); }*/
		
void window_bell(window) 
		Window window; 
		{;}

/* VARARGS2 */
Window window_create(base_frame,create_proc) 
		Window base_frame; char *(*create_proc)(); 
		{ return window_create(base_frame,create_proc); }

int window_destroy(window) 
		Window window; 
		{ return window_destroy(window); }

int window_done(win) 
		Window win; 
		{ return window_done(win); }

/* VARARGS2 */
char *window_get(window,attr) 
		Window window; Window_attribute attr; 
		{ return window_get(window,attr); }

char *window_loop(frame) 
		Frame frame; 
		{ return window_loop(frame); }
		
void window_return(value) 
		char *value; 
		{;}

/*VARARGS1*/
int window_set(window) 
		Window window; 
		{ return window_set(window); }

int window_read_event(window,event)
		Window window; Event *event;
		{ return window_read_event(window,event); }

void window_refuse_kbd_focus(window)
		Window window;
		{;}

void window__release_event_lock(window)
		Window window;
		{;}

void window_main_loop(frame) 
		Window frame; 
		{;}
