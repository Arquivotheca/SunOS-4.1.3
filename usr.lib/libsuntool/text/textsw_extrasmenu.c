#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_extrasmenu.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
**	Allow a user definable "Extras" menu in the textsw.  The search
**	path to find the name of the file to initially look in is:
**	/Text/Extras_menu_filename (defaults), EXTRASMENU (environment var),
**	default ("/usr/lib/.text_extras_menu")
**
**	Much of this code was borrowed from the suntools dynamic rootmenu code.
*/
#include	<suntool/primal.h>
#include	<suntool/textsw_impl.h>
#include	<errno.h>
#include	<ctype.h>
#include	<suntool/walkmenu.h>
#include	<sunwindow/defaults.h>
#include	<suntool/wmgr.h>
#include	<suntool/icon.h>
#include	<suntool/icon_load.h>
#include	<sys/stat.h>
#include	<strings.h>
#include	<sunwindow/string_utils.h>
#include	<sunwindow/sun.h>
#include	"sunwindow/sv_malloc.h"

#define	ERROR	-1

#define	MAX_FILES	40
#define MAXPATHLEN	1024
#define	EXTRASMENU	"/usr/lib/.text_extras_menu"
#define	MAXSTRLEN	256
#define	MAXARGS		20

struct stat_rec
{
    char             *name;	   /* Dynamically allocated menu file name */
    time_t            mftime;      /* Modified file time */
};

static char		*textsw_savestr(), *textsw_save2str();
extern char		*strcpy(), *index(), *getenv();
void			expand_path();
struct stat_rec		Extras_stat_array[MAX_FILES];
int			Textsw_nextfile;
static char		*Textsw_extras_menu_file;

Menu_item		textsw_handle_extras_menuitem();
static int		get_new_menu();
static int		extras_menufile_changed();
static int		free_stat_array();
static int		walk_menufile();
static int		walk_getmenu();
pkg_private char	**string_to_argv();
static int		free_argv();
static Textsw		textsw; /* This should be a MENU_CLIENT_DATA in
				 * walk_getmenu() when the pullright is 
				 * created; however, it is already used.
				 * When I have more time, I will come back and
				 * fix this.
				 */
static Menu		Current_menu;
static int		Nargs;

int
make_new_menu(textsw, menu) 
    Textsw		textsw;
    Menu		menu;
{
/*  
 *  Create new menu if there isn't one already, or
 *  if the current menu is not created with the textsw.
 *  This should be fixed, so the extras menu will only be created
 *  once for each textsw.
 */
    return ((menu == NULL) || 
            (textsw != (Textsw)LINT_CAST(menu_get(menu, MENU_CLIENT_DATA)))); 
}
    
Menu
textsw_build_extras_menu(item, operation)
	Menu_item	item;
	Menu_generate	operation;
{
    textsw = (Textsw)LINT_CAST(menu_get(item, MENU_CLIENT_DATA));
    
	switch(operation)
	{
		case MENU_DISPLAY:
			if(make_new_menu(textsw, Current_menu) || 
			   extras_menufile_changed()) {
			    get_new_menu(textsw, item);
			}	
			break;

		case MENU_DISPLAY_DONE:
			break;

		case MENU_NOTIFY:
		        
			if(make_new_menu(textsw, Current_menu) || 
			   extras_menufile_changed()) {			    
			    get_new_menu(textsw, item);
			}	
			break;

		case MENU_NOTIFY_DONE:
			break;

		default: 
			break; 
	}
	return(Current_menu); 
}

static int
get_new_menu(textsw, item)
    Textsw		textsw;
	Menu_item	item;
{
	Menu	new_menu;
	
				
	free_stat_array();
	
	new_menu = menu_create(
		MENU_LEFT_MARGIN,	6,
		MENU_CLIENT_DATA,	textsw,
		HELP_DATA,		"textsw:extrasmenu",
		0);

	if(walk_menufile(Textsw_extras_menu_file, new_menu) < 0)
		menu_destroy(new_menu);
	else
	{
		menu_destroy(Current_menu);
		Current_menu = new_menu;
	}
}

/*
**	Check to see if there is a valid extrasmenu file.  If the file
**	exists then turn on the Extras item.  If the file does not 
**	exist and there isn't an Extras menu to display then gray out
**	the Extras item.  Otherwise leave it alone.
*/
Menu_item
textsw_toggle_extras_item(extras_item, operation)
	Menu_item	extras_item;
	Menu_generate	operation;
{
	char		full_file[MAXPATHLEN];
	struct stat	statb;

	switch(operation)
	{
		case MENU_DISPLAY:
			if(Textsw_extras_menu_file == NULL)
			{
			    Textsw_extras_menu_file = defaults_get_string(
			      "/Text/Extras_menu_filename", "", (int *)NULL);

			    if((*Textsw_extras_menu_file == '\0')
			      && ((Textsw_extras_menu_file = getenv("EXTRASMENU")) == NULL))
				Textsw_extras_menu_file = EXTRASMENU;
			}

			expand_path(Textsw_extras_menu_file, full_file);

			if(stat(full_file, &statb) < 0)
			{
				if(!Current_menu)
					menu_set(extras_item, MENU_INACTIVE, TRUE, 0);
			}
			else
				menu_set(extras_item, MENU_INACTIVE, FALSE, 0);
			break;

		case MENU_DISPLAY_DONE:
		case MENU_NOTIFY:
		case MENU_NOTIFY_DONE:
			break;

		default: 
			break; 
	}

	return(extras_item);
}

static int
extras_menufile_changed()
{
	int		i;
	struct stat	statb;

	if(Textsw_nextfile == 0) return(TRUE);

	/* Whenever existing menu going up, stat menu files */
	for(i = 0;i < Textsw_nextfile;i++)
	{
		if(stat(Extras_stat_array[i].name, &statb) < 0)
		{
			if(errno == ENOENT)
				return(TRUE);
			(void)fprintf(stderr, "textsw: ");
			perror(Extras_stat_array[i].name);
			return(ERROR);
		}

		if(statb.st_mtime > Extras_stat_array[i].mftime)
			return(TRUE);
	}

	return(FALSE);
}

static int
free_stat_array()
{   
	int i = 0;
    
	while(i < Textsw_nextfile)
	{
		free(Extras_stat_array[i].name);	/* file name */
		Extras_stat_array[i].name = NULL;
		Extras_stat_array[i].mftime = 0;	/* mod time */
		i++;
	}
	Textsw_nextfile = 0;
}

static int
walk_menufile(file, menu)
	char 		*file;
	Menu 		menu;
{   
	FILE		*mfd;
	int		lineno = 1; /* Needed for recursion */
	char		full_file[MAXPATHLEN];
	struct stat 	statb;

    expand_path(file, full_file);
    if((mfd = fopen(full_file, "r")) == NULL) {
	    (void)fprintf(stderr, "textsw: can't open extras menu file %s\n", full_file);
	    return(ERROR);
    }
    if(Textsw_nextfile >= MAX_FILES - 1) {
	    (void)fprintf(stderr, "textsw: max number of menu files is %ld\n", MAX_FILES);
	    (void)fclose(mfd);
	    return(ERROR);
    }
    if(stat(full_file, &statb) < 0) {
	    (void)fprintf(stderr, "textsw: ");
    	perror(full_file);
	    (void)fclose(mfd);
	    return(ERROR);
    }
    
    Extras_stat_array[Textsw_nextfile].mftime = statb.st_mtime;
    Extras_stat_array[Textsw_nextfile].name = textsw_savestr(full_file);
    Textsw_nextfile++;

    if(walk_getmenu(menu, full_file, mfd, &lineno) < 0) {
	    free(Extras_stat_array[--Textsw_nextfile].name);
	    (void)fclose(mfd);
	    return(ERROR);
    }
    else {
	    (void)fclose(mfd);
	    return(TRUE);
    }
}

#ifndef IL_ERRORMSG_SIZE
#define IL_ERRORMSG_SIZE	256
#endif

static int
walk_getmenu(m, file, mfd, lineno)
	Menu 		m;
	char 		*file;
	FILE 		*mfd;
	int 		*lineno;
{   
   	char 		line[256], tag[32], prog[256], args[256];
    	register char 	*p;
    	Menu 		nm;
    	Menu_item 	mi = (Menu_item) 0;
    	char 		*nqformat, *qformat, *iformat, *format;
    	char 		err[IL_ERRORMSG_SIZE], icon_file[MAXPATHLEN];
    	struct pixrect *mpr;

    nqformat = "%[^ \t\n]%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";
    qformat = "\"%[^\"]\"%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";
    iformat = "\<%[^\>]\>%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";

    for(; fgets(line, sizeof(line), mfd); (*lineno)++)  {

        if (line[0] == '#') {
            if (line[1] == '?') {
                for (p = line + 2; isspace(*p); p++)
                    ; 
                    
                if (*p != '\0' && sscanf(p, "%[^\n]\n", args) > 0)
                    menu_set((mi ? mi : m), HELP_DATA, textsw_savestr(args), 0); 
            }
            continue;
        }

	for(p = line; isspace(*p); p++)
	    ;

	if(*p == '\0') continue;

	args[0] = '\0';
	format =  *p == '"' ? qformat : *p == '<' ? iformat : nqformat;
	
	if(sscanf(p, format, tag, prog, args) < 2) {
	    (void)fprintf(stderr, "textsw: format error in %s: line %d\n",
		    file, *lineno);
	    return(ERROR);
	}

	if(strcmp(prog, "END") == 0) return(TRUE);

	if(format == iformat) {
	    expand_path(tag, icon_file);
	    if((mpr = icon_load_mpr(icon_file, err)) == NULL) {
		(void)fprintf(stderr, "textsw: icon file format error: %s:\n", err);
		return(ERROR);
	    }
	} else mpr = NULL;

	if(strcmp(prog, "MENU") == 0)	{
	    nm = menu_create(
		MENU_LEFT_MARGIN,	6,
		MENU_NOTIFY_PROC, menu_return_item,
		HELP_DATA, "textsw:extrasmenu",
		0);

	    if(args[0] == '\0')	 {
		    if(walk_getmenu(nm, file, mfd, lineno) < 0) {
		        menu_destroy(nm);
		        return(ERROR);
		    }
	    }
	    else {
	        if(walk_menufile(args, nm) < 0) {
		        menu_destroy(nm);
		        return(ERROR);
		    }
	    }
	    if(mpr)
		    mi = menu_create_item(MENU_IMAGE, mpr,
				      MENU_PULLRIGHT, nm,
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	    else
		    mi = menu_create_item(MENU_STRING, textsw_savestr(tag), 
				      MENU_PULLRIGHT, nm,
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
            (void)menu_set(mi, HELP_DATA, menu_get(nm, HELP_DATA), 0);
	} else {
	    if(mpr)
		    mi = menu_create_item(MENU_IMAGE, mpr,
				      MENU_CLIENT_DATA,
				        textsw_save2str(prog, args),
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      MENU_ACTION_PROC,	textsw_handle_extras_menuitem,
				      0);
	    else
		    mi = menu_create_item(MENU_STRING, textsw_savestr(tag),
				      MENU_CLIENT_DATA,
				        textsw_save2str(prog, args),
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      MENU_ACTION_PROC,	textsw_handle_extras_menuitem,
				      0);
	}
	(void)menu_set(m, MENU_APPEND_ITEM, mi, 0);
    }
    return(TRUE);
}

/*ARGSUSED*/
Menu_item
textsw_handle_extras_menuitem(menu, item)
	Menu		menu;
	Menu_item	item;
{
	char			*prog, *args;
	char			command_line[MAXPATHLEN];
	char			**filter_argv;
	pkg_private char	**string_to_argv();
	register Textsw_view  	view = VIEW_ABS_TO_REP(textsw);
	register Textsw_folio	folio = FOLIO_FOR_VIEW(view);
	int			again_state;

	prog = (char *)menu_get(item, MENU_CLIENT_DATA);
	args = index(prog, '\0') + 1;

	sprintf(command_line, "%s %s", prog, args);
	filter_argv = string_to_argv(command_line);
	
	textsw_flush_caches(view, TFC_STD);
	folio->func_state |= TXTSW_FUNC_FILTER;
	again_state = folio->func_state & TXTSW_FUNC_AGAIN;
	textsw_record_extras(folio, command_line);
	folio->func_state |= TXTSW_FUNC_AGAIN;
	
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			       (caddr_t)TEXTSW_INFINITY-1);

	if ( textsw_call_filter(view, filter_argv) == 1)
                fprintf(stderr, "Cannot locate filter '%s'.\n", prog);
	
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			       (caddr_t)TEXTSW_INFINITY-1);
			       
	folio->func_state &= ~TXTSW_FUNC_FILTER;
	if (again_state == 0)
	    folio->func_state &= ~TXTSW_FUNC_AGAIN;
	free_argv(filter_argv);
	return(item);
}

/*
**	string_to_argv - This function takes a char * that contains
**			a program and it's arguments and returns
**			a char ** argument vector for use with execvp
**
**			For example "fmt -65" is turned into
**			argv[0] = "fmt"
**			argv[1] = "-65"
**			argv[2] = NULL
*/
pkg_private char **
string_to_argv(command_line)
    char	*command_line;
{
    int		i, pos = 0;
    char	**new_argv;
    char	*arg_array[MAXARGS];
    char	scratch[MAXSTRLEN];
    int		use_shell = any_shell_meta(command_line);

    Nargs = 0;

    if (use_shell) {
        /* put in their favorite shell and pass cmd as single string */
        char *shell;
	extern char *getenv();

	if ((shell = getenv("SHELL")) == NULL)
	    shell = "/bin/sh";
	new_argv = (char **)sv_malloc((unsigned) 4  * sizeof (char *));
	new_argv[0] = shell;
	new_argv[1] = "-c";
	new_argv[2] = strdup(command_line);
	new_argv[3] = '\0';
    } else {
	/* Split command_line into it's individual arguments */
	while(string_get_token(command_line, &pos, scratch, white_space) != NULL)
	    arg_array[Nargs++] = strdup(scratch);

	/*
         * Allocate a new array of appropriate size (Nargs+1 for NULL string)
         * This is so the caller will know where the array ends
         */
        new_argv = (char **)sv_malloc((unsigned) (Nargs + 1) *
				     sizeof (char *));

	/*
	 * And WHATEVER you do, DON't forget to actually add that pesky zero.
	 * This is Mick's first SunView bug-fix, and will probably be famous
	 * someday for that reason.
	 * Autographed copies available on request.
	 */
	new_argv[Nargs] = (char *) NULL;

        /* Copy the strings from arg_array into it */
        for (i = 0; i < Nargs; i++)
    	    new_argv[i] = arg_array[i];
    }
    return(new_argv);
}

static int
free_argv(argv)
	char	**argv;
{
	while(Nargs > 0)
		free(argv[--Nargs]);
	free(argv);
}

static char *
textsw_savestr(s)
	register char *s;
{
	register char *p;

	p = sv_malloc((unsigned)(strlen(s) + 1));
	(void)strcpy(p, s);
	return(p);
}

static char *
textsw_save2str(s, t)
	register char *s, *t;
{
	register char *p;

	p = sv_malloc((unsigned)(strlen(s) + strlen(t) + 1 + 1));
	(void)strcpy(p, s);
	(void)strcpy(index(p, '\0') + 1, t);
	return(p);
}

/*
 * Are there any shell meta-characters in string s?
 */
static
any_shell_meta(s)
	register char *s;
{

	while (*s) {
		if (index("~{[*?$`'\"\\", *s))
			return (1);
		s++;
	}
	return (0);
}
