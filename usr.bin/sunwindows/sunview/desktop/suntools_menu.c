#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)suntools_menu.c 1.1 92/07/30";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * Suntools: Handle creation of walking menus
 */

#include <suntool/tool_hs.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <sys/stat.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/wmgr.h>
#include <suntool/icon_load.h>
#include <suntool/walkmenu.h>
#include <suntool/help.h>

#define Pkg_private 
#define Private static

Pkg_private int walk_getrootmenu(), walk_handlerootmenuitem();
Pkg_private int wmgr_menufile_changes();
Pkg_private char *wmgr_savestr(), *wmgr_save2str();

Pkg_private int _error();
#define	NO_PERROR	((char *) 0)

extern char	     *strcpy(),
		     *index();
void		     expand_path();
int		     wmgr_forktool();


Pkg_private Menu wmgr_rootmenu;

struct stat_rec {
    char             *name;	   /* Dynamically allocated menu file name */
    time_t            mftime;      /* Modified file time */
};

#define	MAX_FILES	40
#define MAXPATHLEN	1024
#define MAXARGLEN	3000

Pkg_private struct stat_rec wmgr_stat_array[];
Pkg_private int            wmgr_nextfile;


/* ======================== Walking menu support ============================ */


/* ARGSUSED */
Pkg_private int
walk_getrootmenu(file)
	char *file;
{ 
    Menu m;
    
    if (wmgr_menufile_changes() == 0) return 1;
    wmgr_free_changes_array();
    m = menu_create(MENU_NOTIFY_PROC, menu_return_item,
		    HELP_DATA, "sunview:rootmenu",
		    0);
    if (walk_menufile(file, m) < 0) {
	menu_destroy(m);
    } else {
	menu_destroy(wmgr_rootmenu);
	wmgr_rootmenu = m;
    }
    return wmgr_rootmenu ? 1 : -1;
}

Private
walk_menufile(file, menu)
	char *file;
	Menu menu;
{   
    FILE *mfd;
    int	lineno = 1; /* Needed for recursion */
    struct stat statb;
    char full_file[MAXPATHLEN];

    expand_path(file, full_file);
    if ((mfd = fopen(full_file, "r")) == NULL) {
	return _error("cannot open menu file %s", full_file);
    }
    if (wmgr_nextfile >= MAX_FILES - 1) {
	(void)fclose(mfd);
	return _error(NO_PERROR, "max number of menu files is %d", MAX_FILES);
    }
    if (stat(full_file, &statb) < 0) {
	(void)fclose(mfd);
	return _error("rootmenu %s stat error", full_file);
    }
    
    wmgr_stat_array[wmgr_nextfile].mftime = statb.st_mtime;
    wmgr_stat_array[wmgr_nextfile].name = wmgr_savestr(full_file);
    wmgr_nextfile++;

    if (walk_getmenu(menu, full_file, mfd, &lineno) < 0) {
	free(wmgr_stat_array[--wmgr_nextfile].name);
	(void)fclose(mfd);
	return -1;
    } else {
	(void)fclose(mfd);
	return 1;
    }
}

Private
walk_getmenu(m, file, mfd, lineno)
	Menu m;
	char *file;
	FILE *mfd;
	int *lineno;
{   
    char line[256], tag[256], prog[256], args[256];
    register char *p;
    Menu nm;
    Menu_item mi = NULL;
    char *nqformat, *qformat, *iformat, *format;
    char err[IL_ERRORMSG_SIZE], icon_file[MAXPATHLEN];
    struct pixrect *mpr;

    nqformat = "%[^ \t\n]%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";
    qformat = "\"%[^\"]\"%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";
    iformat = "\<%[^\>]\>%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";

    for (; fgets(line, sizeof(line), mfd); (*lineno)++) {

	if (line[0] == '#') {
	    if (line[1] == '?') {
		char help[256];

	        for (p = line + 2; isspace(*p); p++)
	            ;
		
		if (*p != '\0' && sscanf(p, "%[^\n]\n", help) == 1)
		    menu_set((mi != NULL ? mi : m), HELP_DATA, help, 0);
	    }
	    continue;
	}

	for (p = line; isspace(*p); p++)
	    ;

	if (*p == '\0') continue;

	args[0] = '\0';
	format =  *p == '"' ? qformat : *p == '<' ? iformat : nqformat;
	if (sscanf(p, format, tag, prog, args) < 2) {
	    return _error(NO_PERROR, "format error in %s: line %d",
		    file, *lineno);
	}

	if (strcmp(prog, "END") == 0) return 1;

	if (format == iformat) {
	    expand_path(tag, icon_file);
	    if ((mpr = icon_load_mpr(icon_file, err)) == NULL) {
		return _error(NO_PERROR, "icon file %s format error: %s",
			icon_file, err);
	    }
	} else mpr = NULL;

	if (strcmp(prog, "MENU") == 0) {
	    nm = menu_create(MENU_NOTIFY_PROC, menu_return_item,
			     HELP_DATA, "sunview:rootmenu",
			     0);
	    if (args[0] == '\0') {
		if (walk_getmenu(nm, file, mfd, lineno) < 0) {
		    menu_destroy(nm);
		    return -1;
		}
	    } else {
		if (walk_menufile(args, nm) < 0) {
		    menu_destroy(nm);
		    return -1;
		}
	    }
	    if (mpr)
		mi = menu_create_item(MENU_IMAGE, mpr,
				      MENU_PULLRIGHT, nm,
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	    else
		mi = menu_create_item(MENU_STRING, wmgr_savestr(tag), 
				      MENU_PULLRIGHT, nm,
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	} else {
	    if (mpr)
		mi = menu_create_item(MENU_IMAGE, mpr,
				      MENU_CLIENT_DATA,
				        wmgr_save2str(prog, args),
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	    else
		mi = menu_create_item(MENU_STRING, wmgr_savestr(tag),
				      MENU_CLIENT_DATA,
				        wmgr_save2str(prog, args),
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	}
	(void)menu_set(m, MENU_APPEND_ITEM, mi, 0);
    }
    return 1;
}

/* ARGSUSED */
Pkg_private
walk_handlerootmenuitem(menu, mi, rootfd)
	Menu menu;
	Menu_item mi;
	int	rootfd;	/* Ignored */
{
	int	returncode = 0;
	int 	i = 0;
	struct	rect recticon, rectnormal;
	char	*prog, *args;
	char full_prog[MAXPATHLEN], full_args[MAXARGLEN]; 
	char *tmp_args, path_args[MAXARGLEN], *p;

	/*
	 * Get next default tool positions
	 */
	rect_construct(&recticon, WMGR_SETPOS, WMGR_SETPOS,
	    WMGR_SETPOS, WMGR_SETPOS);
	rectnormal = recticon;
	prog = (char *)menu_get(mi, MENU_CLIENT_DATA);
	args = index(prog, '\0') + 1;
	if (strcmp(prog, "EXIT") == 0) {
	    int result;
	    Event alert_event;
	    int real_result = 0;

	    result = alert_prompt(
		    (Frame)0,
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
		        "Do you really want to exit SunView?",
		        0,
		    ALERT_BUTTON_YES,	"Confirm",
		    ALERT_BUTTON_NO,	"Cancel",
		    0);
	    switch (result) {
	 	    case ALERT_YES: 
		        returncode = -1;
		        break;
		    case ALERT_NO:
		        returncode = 0;
		        break;
		    default: /* alert_failed */
		        returncode = 0;
		    break;
	    }
	} else if (strcmp(prog, "REFRESH") == 0) {
		wmgr_refreshwindow(rootfd);
	} else {
		suntools_mark_close_on_exec();
                expand_path(prog, full_prog);
                full_args[0] = '\0';
                tmp_args = args;
                while((p = index(tmp_args, '~')) != NULL) {
                	while((*tmp_args != *p) && (i < MAXARGLEN)) {
                		full_args[i++] = *(tmp_args++);
                	}
                	expand_path(tmp_args, path_args);
                	tmp_args = path_args;
                }
		if ( i > 0 )
                	while((i < MAXARGLEN) && ((full_args[i] = *tmp_args) != '\0')) {
				i++;
				tmp_args++;
			}
		else
			strcpy(full_args, args);
		(void)wmgr_forktool(full_prog, full_args, &rectnormal, &recticon, 0);
	}
	return (returncode);
}
