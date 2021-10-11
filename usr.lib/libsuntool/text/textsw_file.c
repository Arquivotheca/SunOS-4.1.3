#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)textsw_file.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * File load/save/store utilities for text subwindows.
 */ 
#include <suntool/primal.h>

#include <suntool/textsw_impl.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/param.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <errno.h>
#include <suntool/expand_name.h>

extern char	*strcat();
extern char	*strncat();
extern char	*getwd();
extern int	 errno, sys_nerr;
extern char	*sys_errlist[];

static int		textsw_menu_style_already_gotten;
static Textsw_enum 	textsw_menu_style;

static int		textsw_change_directory();
pkg_private void	textsw_display(), textsw_display_view_margins();
pkg_private void	textsw_input_before();
pkg_private void	textsw_init_undo(), textsw_replace_esh();
pkg_private Es_index	textsw_input_after();
extern Es_status	es_copy();
extern Textsw_enum	textsw_get_menu_style_internal();

unsigned tmtn_counter;
static
textsw_make_temp_name(in_here)
	char	*in_here;
/* *in_here must be at least MAXNAMLEN characters long */
{
	/* BUG ALERT!  Should be able to specify directory other than
	 *   /tmp.  However, if that directory is not /tmp it should
	 *   be a local (not NFS) directory and will make finding files
	 *   that need to be replayed after crash harder - assuming we
	 *   ever implement replay.
	 */
	in_here[0] = '\0';
	(void) sprintf(in_here, "%s/Text%d.%d",
		       "/tmp", getpid(), tmtn_counter++);
}

#define ES_BACKUP_FAILED	ES_CLIENT_STATUS(0)
#define ES_BACKUP_OUT_OF_SPACE	ES_CLIENT_STATUS(1)
#define ES_CANNOT_GET_NAME	ES_CLIENT_STATUS(2)
#define ES_CANNOT_OPEN_OUTPUT	ES_CLIENT_STATUS(3)
#define ES_CANNOT_OVERWRITE	ES_CLIENT_STATUS(4)
#define ES_DID_NOT_CHECKPOINT	ES_CLIENT_STATUS(5)
#define ES_PIECE_FAIL		ES_CLIENT_STATUS(6)
#define ES_SHORT_READ		ES_CLIENT_STATUS(7)
#define ES_UNKNOWN_ERROR	ES_CLIENT_STATUS(8)
#define ES_USE_SAVE		ES_CLIENT_STATUS(9)

#define TXTSW_LRSS_MASK		0x00000007

pkg_private Es_handle
textsw_create_ps(folio, original, scratch, status)
	Textsw_folio		  folio;
	register Es_handle	  original, scratch;
	Es_status		 *status;
{
        register Es_handle       result;

	result =
	    folio->es_create(folio->client_data, original, scratch);
	if (result) {
	    *status = ES_SUCCESS;
	} else {
	    es_destroy(original);
	    es_destroy(scratch);
	    *status = ES_PIECE_FAIL;
	}
	return(result);
}

pkg_private Es_handle
textsw_create_file_ps(folio, name, scratch_name, status)
	Textsw_folio		  folio;
	char			 *name, *scratch_name;
	Es_status		 *status;
/* *scratch_name must be at least MAXNAMLEN characters long, and is modified */
{
	extern Es_handle	  es_file_create();
	register Es_handle	  original_esh, scratch_esh, piece_esh;

	original_esh = es_file_create(name, 0, status);
	if (!original_esh)
	    return(ES_NULL);
	textsw_make_temp_name(scratch_name);
	scratch_esh = es_file_create(scratch_name,
				     ES_OPT_APPEND|ES_OPT_OVERWRITE,
				     status);
	if (!scratch_esh) {
	    es_destroy(original_esh);
	    return(ES_NULL);
	}
	(void) es_set(scratch_esh, ES_FILE_MODE, 0600, 0);
	piece_esh = textsw_create_ps(folio, original_esh, scratch_esh, status);
	return(piece_esh);
}

#define	TXTSW_LFI_CLEAR_SELECTIONS	0x1

pkg_private Es_status
textsw_load_file_internal(
    textsw, name, scratch_name, piece_esh, start_at, flags)
	register Textsw_folio	 textsw;
	char			*name, *scratch_name;
	Es_handle		*piece_esh;
	Es_index		 start_at;
	int			 flags;
/* *scratch_name must be at least MAXNAMLEN characters long, and is modified */
{
	Es_status		status;

	*piece_esh = textsw_create_file_ps(textsw, name,
					   scratch_name, &status);
	if (status == ES_SUCCESS) {
	    if (flags & TXTSW_LFI_CLEAR_SELECTIONS) {
		Textsw	abstract = VIEW_REP_TO_ABS(textsw->first_view);

		textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				     EV_SEL_PRIMARY);
		textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				     EV_SEL_SECONDARY);
	    }
	    textsw_replace_esh(textsw, *piece_esh);
	    if (start_at != ES_CANNOT_SET) {
		(void) ev_set(textsw->views->first_view,
			      EV_FOR_ALL_VIEWS,
			      EV_DISPLAY_LEVEL, EV_DISPLAY_NONE,
			      EV_DISPLAY_START, start_at,
			      EV_DISPLAY_LEVEL, EV_DISPLAY,
			      0);
		textsw_notify(textsw->first_view,
			      TEXTSW_ACTION_LOADED_FILE, name, 0);
		textsw_update_scrollbars(textsw, TEXTSW_VIEW_NULL);	      
	    }
	    
	}
	return(status);
}

pkg_private void
textsw_destroy_esh(folio, esh)
	register Textsw_folio	folio;
	register Es_handle	esh;
{
	Es_handle		original_esh, scratch_esh;
	
	if (folio->views->esh == esh)
	    folio->views->esh = ES_NULL;
	if (original_esh = (Es_handle)LINT_CAST(es_get(esh, ES_PS_ORIGINAL))) {
	    textsw_give_shelf_to_svc(folio);
	    scratch_esh = (Es_handle)LINT_CAST(es_get(esh, ES_PS_SCRATCH));
	    es_destroy(original_esh);
	    if (scratch_esh) es_destroy(scratch_esh);
	}
	es_destroy(esh);
}

pkg_private void
textsw_replace_esh(textsw, new_esh)
	register Textsw_folio	textsw;
	Es_handle		new_esh;
/* Caller is repsonsible for actually repainting the views. */
{
	Es_handle		save_esh = textsw->views->esh;
	int			undo_count = textsw->undo_count;

	(void) ev_set(textsw->views->first_view,
		      EV_DISPLAY_LEVEL, EV_DISPLAY_NONE,
		      EV_CHAIN_ESH, new_esh,
		      0);
	textsw->state &= ~TXTSW_EDITED;
	textsw_destroy_esh(textsw, save_esh);
	/* Following two calls are inefficient textsw_re-init_undo. */
	textsw_init_undo(textsw, 0);
	textsw_init_undo(textsw, undo_count);
	textsw->state &= ~TXTSW_READ_ONLY_ESH;
	if (textsw->notify_level & TEXTSW_NOTIFY_SCROLL) {
	    register Textsw_view	view;
	    FORALL_TEXT_VIEWS(textsw, view) {
		textsw_display_view_margins(view, RECT_NULL);
	    }
	}
}

pkg_private Es_handle
textsw_create_mem_ps(folio, original)
	Textsw_folio		 folio;
	register Es_handle	 original;
{
	Es_handle		 es_mem_create();
	register Es_handle	 scratch;
	Es_status		 status;
	Es_handle		 ps_esh = ES_NULL;

	if (original == ES_NULL)
	    goto Return;
	scratch = es_mem_create(folio->es_mem_maximum, "");
	if (scratch == ES_NULL) {
	    es_destroy(original);
	    goto Return;
	}
	ps_esh = textsw_create_ps(folio, original, scratch, &status);
Return:
	return(ps_esh);
}

/* Returns 0 iff load succeeded (can do cd instead of load). */
pkg_private int
textsw_load_selection(folio, locx, locy, no_cd)
	register Textsw_folio	folio;
	register int		locx, locy;
	int			no_cd;
{
	char			filename[MAXNAMLEN];
	register int		result;

	if (textsw_get_selection_as_filename(
			folio, filename, sizeof(filename), locx, locy)) {
	    return(-10);
	}
	result = no_cd ? -2
		 : textsw_change_directory(folio, filename, TRUE, locx, locy);
	if (result == -2) {
	    result = textsw_load_file(VIEW_REP_TO_ABS(folio->first_view),
				      filename, TRUE, locx, locy);
	    if (result == 0) {
		(void) textsw_set_insert(folio, 0L);
	    }
	}
	return(result);
}

pkg_private char *
textsw_full_pathname(name)
    register char	*name;
{
    char		 pathname[MAXPATHLEN];
    register char	*full_pathname;
    
    if (name == 0)
        return(name);
    if (*name == '/') {
        if ((full_pathname = malloc((unsigned)(1+strlen(name)))) == 0)
            return (0);
        (void) strcpy(full_pathname, name);
        return(full_pathname);
    }
    if (getwd(pathname) == 0)
        return (0);
    if ((full_pathname =
        malloc((unsigned)(2+strlen(pathname)+strlen(name)))) == 0)
        return(0);
    (void) strcpy(full_pathname, pathname);
    (void) strcat(full_pathname, "/");
    (void) strcat(full_pathname, name);
    return(full_pathname);
}

/* ARGSUSED */
pkg_private int
textsw_format_load_error(msg, status, filename, scratch_name)
	char		*msg;
	Es_status	 status;
	char		*filename;
	char		*scratch_name;	/* Currently unused */
{
	char		*full_pathname;

	switch (status) {
	  case ES_PIECE_FAIL:
	    (void) sprintf(msg, "Cannot create piece stream.");
	    break;
	  case ES_SUCCESS:
	    /* Caller is being lazy! */
	    break;
	  default:
	    full_pathname = textsw_full_pathname(filename);  
	    (void) sprintf(msg, "Cannot load; ");
	    es_file_append_error(msg, "file", status);
	    es_file_append_error(msg, full_pathname, status);
	    free(full_pathname);
	    break;
	}
}

pkg_private int
textsw_format_load_error_quietly(msg, status, filename, scratch_name)
	char		*msg;
	Es_status	 status;
	char		*filename;
	char		*scratch_name;	/* Currently unused */
{
	char		*full_pathname;

	switch (status) {
	  case ES_PIECE_FAIL:
	    (void) sprintf(msg,
		"INTERNAL ERROR: Cannot create piece stream.");
	    break;
	  case ES_SUCCESS:
	    /* Caller is being lazy! */
	    break;
	  default:
	    full_pathname = textsw_full_pathname(filename); 
	    (void) sprintf(msg, "Unable to load file:");
	    es_file_append_error(msg, full_pathname, status);
	    free(full_pathname);
	    break;
	}
}

/* Returns 0 iff load succeeded. */
extern int
textsw_load_file(abstract, filename, reset_views, locx, locy)
	Textsw		 abstract;
	char		*filename;
	int		 reset_views;
	int		 locx, locy;
{
	char		 msg_buf[MAXNAMLEN+100];
	char		 alert_msg_buf[MAXNAMLEN+100];
	char		 scratch_name[MAXNAMLEN];
	int		 result;
	Es_status	 status;
	Es_handle	 new_esh;
	Es_index	 start_at;
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);
	Event		 event;
	
	start_at = (reset_views) ? 0 : ES_CANNOT_SET;
	status = textsw_load_file_internal(
			textsw, filename, scratch_name, &new_esh, start_at,
			TXTSW_LFI_CLEAR_SELECTIONS);
	if (status == ES_SUCCESS) {
	    if (start_at == ES_CANNOT_SET)
		textsw_notify((Textsw_view)textsw,	/* Cast for lint */
			      TEXTSW_ACTION_LOADED_FILE, filename, 0);
	} else {
	        textsw_format_load_error_quietly(
		    alert_msg_buf, status, filename, scratch_name);
	        result = alert_prompt(
		    (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
		    &event,
	            ALERT_BUTTON_YES,	"Continue",
		    ALERT_TRIGGER,	ACTION_STOP, /* yes or no */
		    ALERT_MESSAGE_STRINGS,
		        alert_msg_buf,
		        0,
	            0);
	}
	return(status);
}

/* Returns 0 iff load succeeded. */
extern int
textsw_load_file_quietly(abstract, filename, feedback, reset_views, locx, locy)
	Textsw		 abstract;
	char		*filename, *feedback;
	int		 reset_views;
	int		 locx, locy;
{
	char		 msg_buf[MAXNAMLEN+100];
	char		 scratch_name[MAXNAMLEN];
	Es_status	 status;
	Es_handle	 new_esh;
	Es_index	 start_at;
	Textsw_view	 view = VIEW_ABS_TO_REP(abstract);
	Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);
	
	start_at = (reset_views) ? 0 : ES_CANNOT_SET;
	status = textsw_load_file_internal(
			textsw, filename, scratch_name, &new_esh, start_at,
			TXTSW_LFI_CLEAR_SELECTIONS);
	if (status == ES_SUCCESS) {
	    if (start_at == ES_CANNOT_SET)
		textsw_notify((Textsw_view)textsw,	/* Cast for lint */
			      TEXTSW_ACTION_LOADED_FILE, filename, 0);
	} else {
	    textsw_format_load_error_quietly(
		feedback, status, filename, scratch_name);
	}
	return(status);
}

#define RELOAD		1
#define NO_RELOAD	0
static Es_status
textsw_save_store_common(folio, output_name, reload)
	register Textsw_folio	 folio;
	char			*output_name;
	int			 reload;
{
	char			 scratch_name[MAXNAMLEN];
	Es_handle		 new_esh;
	register Es_handle	 output;
	Es_status		 result;
	Es_index		 length;
        struct statfs fs;
	struct stat   stat_buf;
        char *full_path;
        register char *char_ind;
        extern char *rindex();

	length = es_get_length(folio->views->esh);

        if (stat(output_name, &stat_buf) < 0 && errno != ENOENT) 
              return (ES_UNKNOWN_ERROR);

        if (errno == ENOENT) {
           stat_buf.st_size = 0;
           if ((full_path = textsw_full_pathname (output_name)) != 0)
              if ((char_ind = rindex (full_path, '/')) != NULL)
                 {
                   *char_ind = '\0';
                   if (statfs(full_path, &fs) < 0)
                      return (ES_UNKNOWN_ERROR);
                 }
        }
        else if (statfs(output_name, &fs) < 0)
                return (ES_UNKNOWN_ERROR);
  
        /* subtract file on disk length from file in memory length = number
           of bytes needed on disk to write entire file successfully */
        if (length - stat_buf.st_size > fs.f_bavail * DEV_BSIZE) 
            return (ES_SHORT_WRITE);
        
	output = es_file_create(output_name, ES_OPT_APPEND, &result);
	if (!output)
	    /* BUG ALERT!  For now, don't look at result. */
	    return(ES_CANNOT_OPEN_OUTPUT);
	result = es_copy(folio->views->esh, output, TRUE);
	if (result == ES_SUCCESS) {
	    es_destroy(output);
	    if (folio->checkpoint_name) {
		if (unlink(folio->checkpoint_name) == -1) {
		    perror("removing checkpoint file:");
		}
		free(folio->checkpoint_name);
		folio->checkpoint_name = NULL;
	    }
	    if (reload) {
		result = textsw_load_file_internal(
		    folio, output_name, scratch_name, &new_esh,
		    ES_CANNOT_SET, 0);
		if ((result == ES_SUCCESS) &&
		    (length != es_get_length(new_esh))) {
		    /* Added a newline - repaint to fix line tables */
		    textsw_display((Textsw)folio->first_view);
		}
	    }
	}
	return(result);
}

extern Es_status
textsw_process_save_error(abstract, error_buf, status, locx, locy)
	Textsw			 abstract;
	char			*error_buf;
	Es_status	 	 status;
	int			 locx, locy;
{
	char			*msg1, *msg2;
	char			 msg[200];
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	Event			 event;

	switch (status) {
	  case ES_BACKUP_FAILED:
	    msg[0] = '\0';
	    msg1 = "Unable to Save Current File. ";
	    msg2 = "Cannot back-up file:";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
	  case ES_BACKUP_OUT_OF_SPACE:
	    msg[0] = '\0';
	    msg1 = "Unable to Save Current File. ";
	    msg2 = "No space for back-up file:";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
	  case ES_CANNOT_OPEN_OUTPUT:
	    msg[0] = '\0';
	    msg1 = "Unable to Save Current File. ";
	    msg2 = "Cannot re-write file:";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
	  case ES_CANNOT_GET_NAME:
	    msg[0] = '\0';
	    msg1 = "Unable to Save Current File. ";
	    msg2 = "INTERNAL ERROR: Forgot the name of the file.";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
	  case ES_UNKNOWN_ERROR:	/* Fall through */
	  default:
	    goto InternalError;
	}
InternalError:
	msg[0] = '\0';
	msg1 = "Unable to Save Current File. ";
	msg2 = "An INTERNAL ERROR has occurred.";
	strcat(msg, msg1);
	strcat(msg, msg2);
PostError:
	(void) alert_prompt(
	        (Frame)window_get(WINDOW_FROM_VIEW(abstract), WIN_OWNER),
	        &event,
	        ALERT_BUTTON_YES,	"Continue",
	        ALERT_TRIGGER,		ACTION_STOP,
	        ALERT_MESSAGE_STRINGS,
		    msg1,
		    msg2,
		    error_buf,
		    0,
                0);
	return(ES_UNKNOWN_ERROR);
}

/* ARGSUSED */
static Es_status
textsw_save_internal(folio, error_buf, locx, locy)
	register Textsw_folio	 folio;
	char			*error_buf;
	int			 locx, locy;	/* Currently unused */
{
	extern Es_handle	 es_file_make_backup();
	char			 original_name[MAXNAMLEN], *name;
	register char		*msg;
	Es_handle		 backup, original = ES_NULL;
	int			 status;
	Es_status		 es_status;

	/* Save overwrites the contents of the original stream, which makes
	 * the call to textsw_give_shelf_to_svc in textsw_destroy_esh
	 * (reached via textsw_save_store_common calling textsw_replace_esh)
	 * perform bad operations on the pieces that are the shelf.
	 * To get around this problem, we first save the shelf explicitly.
	 */
	textsw_give_shelf_to_svc(folio);
	if (textsw_file_name(folio, &name) != 0)
	    return(ES_CANNOT_GET_NAME);
	(void) strcpy(original_name, name);
	original = (Es_handle)LINT_CAST(es_get(folio->views->esh,
						ES_PS_ORIGINAL));
	if (!original) {
	    msg = "es_ps_original";
	    goto Return_Error_Status;
	}
	if ((backup = es_file_make_backup(original, "%s%%", &es_status))
	    == ES_NULL) {
	    return(((es_status == ES_CHECK_ERRNO) && (errno == ENOSPC))
		   ? ES_BACKUP_OUT_OF_SPACE
		   : ES_BACKUP_FAILED);
	}
	(void) es_set(folio->views->esh,
	       ES_STATUS_PTR, &es_status,
	       ES_PS_ORIGINAL, backup,
	       0);
	if (es_status != ES_SUCCESS) {
		int	result;
		Event	alert_event;

		result = alert_prompt(
		    (Frame)window_get(
			WINDOW_FROM_VIEW(
			    VIEW_FROM_FOLIO_OR_VIEW(folio)),	
			WIN_OWNER),
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
		        "Unable to Save Current File.",
		        "Was the file edited with another editor?.",
		        0,
		    ALERT_BUTTON_YES,	"Continue",
		    ALERT_TRIGGER,	ACTION_STOP,
		    0);
		if (result == ALERT_FAILED) {
		    (void) es_destroy(backup);
		    status = (int)es_status;
		    msg = "ps_replace_original";
	            goto Return_Error_Status;
		}
		goto Dont_Return_Error_Status;
	}
	switch (status =
		textsw_save_store_common(folio, original_name, RELOAD)) {
	  case ES_SUCCESS:
	    (void) es_destroy(original);
	    textsw_notify(folio->first_view,
			  TEXTSW_ACTION_LOADED_FILE, original_name, 0);
	    return(ES_SUCCESS);
	  case ES_CANNOT_OPEN_OUTPUT:
	    if (errno == EACCES)
		goto Return_Error;
	    msg = "es_file_create";
	    goto Return_Error_Status;
	  default:
	    msg = "textsw_save_store_common";
	    break;
	}
Return_Error_Status:
	(void) sprintf(error_buf, "  %s; status = 0x%x", msg, status);
Dont_Return_Error_Status:
	status = ES_UNKNOWN_ERROR;
Return_Error:
	if (original)
	    (void) es_set(folio->views->esh,
			  ES_STATUS_PTR, &es_status,
			  ES_PS_ORIGINAL, original,
			  0);
	return(status);
}

extern unsigned
textsw_save(abstract, locx, locy)
	Textsw		abstract;
	int		locx, locy;
{
	char		error_buf[MAXNAMLEN+100];
	Es_status	status;
	Textsw_view	view = VIEW_ABS_TO_REP(abstract);

	error_buf[0] = '\0';
	status = textsw_save_internal(FOLIO_FOR_VIEW(view), error_buf,
				      locx, locy);
	if (status != ES_SUCCESS)
	    status = textsw_process_save_error(
				abstract, error_buf, status, locx, locy);
	return((unsigned)status);
}

static Es_status
textsw_get_from_fd(view, fd, print_error_msg)
	register Textsw_view	view;
	int			fd;
	int			print_error_msg;
{
	Textsw_folio		folio = FOLIO_FOR_VIEW(view);
	int			record;
	Es_index		old_insert_pos, old_length;
	register long		count;
	char			buf[2096];
	Es_status		result = ES_SUCCESS;
	int			 status;

	textsw_flush_caches(view, TFC_PD_SEL);		/* Changes length! */
	textsw_input_before(view, &old_insert_pos, &old_length);
	for (;;) {
	    count = read(fd, buf, sizeof(buf)-1);
	    if (count == 0)
		break;
	    if (count < 0) {
		return(ES_UNKNOWN_ERROR);
	    }
	    buf[count] = '\0';
	    status = ev_input_partial(FOLIO_FOR_VIEW(view)->views, buf, count);

	    if (status) {
	    	if (print_error_msg)
	    	    (void) textsw_esh_failed_msg(view, "Insertion failed - ");
		result = (Es_status)LINT_CAST(es_get(folio->views->esh,
						     ES_STATUS));
	        break;
	    }
	}
	record = (TXTSW_DO_AGAIN(folio) &&
		  ((folio->func_state & TXTSW_FUNC_AGAIN) == 0) );
	(void) textsw_input_after(view, old_insert_pos, old_length, record);
	return(result);
}

pkg_private int
textsw_cd(textsw, locx, locy)
	Textsw_folio	 textsw;
	int		 locx, locy;
{
	char		 buf[MAXNAMLEN];

	if (0 == textsw_get_selection_as_filename(
			textsw, buf, sizeof(buf), locx, locy)) {
	    (void) textsw_change_directory(textsw, buf, FALSE, locx, locy);
	}
	return;
}

pkg_private Textsw_status
textsw_get_from_file(view, filename, print_error_msg)
	Textsw_view	 view;
	char		*filename;
	int		 print_error_msg;
{
	Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	int		 fd;
	Es_status	 status;
	Textsw_status	 result = TEXTSW_STATUS_CANNOT_INSERT_FROM_FILE;
	int		 got_from_file = 0;
	char		 buf[MAXNAMLEN];
	pkg_private int	 textsw_is_out_of_memory();
	
	
	if (!TXTSW_IS_READ_ONLY(folio) && (strlen(filename) > 0)) {
	    strcpy(buf, filename);
	    if (textsw_expand_filename(folio, buf, sizeof(buf), -1, -1)
	        == 0) {
	        if ((fd = open(buf, 0)) >= 0) {
	            textsw_take_down_caret(folio);
	            textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
	            status = textsw_get_from_fd(view, fd, print_error_msg);
	            textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
	            textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	            (void) close(fd);
	            if (status == ES_SUCCESS)
	            	result = TEXTSW_STATUS_OKAY;
	            else if (textsw_is_out_of_memory(folio, status))
	            	result = TEXTSW_STATUS_OUT_OF_MEMORY;
	            textsw_invert_caret(folio); 
	        } 
	        
	    }
	}
	return(result);
}

	
pkg_private int
textsw_file_stuff(view, locx, locy)
	Textsw_view	 view;
	int		 locx, locy;
{
	Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	int		 fd;
	char		 buf[MAXNAMLEN], msg[MAXNAMLEN+100], *sys_msg;
	char		 alert_msg1[MAXNAMLEN+100];
	char		*alert_msg2;
	Es_status	 status;
	int		 cannot_open = 0;
	Event		 event;

	if (0 == textsw_get_selection_as_filename(
			folio, buf, sizeof(buf), locx, locy)) {
	    if ((fd = open(buf, 0)) < 0) {
		cannot_open = (fd == -1);
		goto InternalError;
	    };
	    errno = 0;
	    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
	    status = textsw_get_from_fd(view, fd, TRUE);
	    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				(caddr_t)TEXTSW_INFINITY-1);
	    textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	    (void) close(fd);
	    if (status != ES_SUCCESS && status != ES_SHORT_WRITE)
		goto InternalError;
	}
	return;
InternalError:
	if (cannot_open) {
	   char		*full_pathname;
	   
	   full_pathname = textsw_full_pathname(buf);
	   (void) sprintf(msg, "'%s': ", full_pathname);
	   (void) sprintf(alert_msg1, "'%s'", full_pathname);
	   alert_msg2 = "  ";
	   free(full_pathname);
	} else {
	   (void) sprintf(msg, "%s", "Unable to Include File.  An INTERNAL ERROR has occurred.: ");
	   (void) sprintf(alert_msg1, "%s", "Unable to Include File.");
	   alert_msg2 = "An INTERNAL ERROR has occurred.";
	}
	sys_msg = (errno > 0 && errno < sys_nerr) ? sys_errlist[errno] : NULL;
	(void) alert_prompt(
	        (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
	        &event,
	        ALERT_BUTTON_YES,	"Continue",
	        ALERT_TRIGGER,		ACTION_STOP,
	        ALERT_MESSAGE_STRINGS,
	            (strlen(sys_msg)) ? sys_msg : alert_msg1,
	            (strlen(sys_msg)) ? alert_msg1 : alert_msg2,
	            (strlen(sys_msg)) ? alert_msg2 : 0,
	            0,
	        0);
}

pkg_private int
textsw_file_stuff_quietly(view, file, msg, locx, locy)
	Textsw_view	 view;
	char		 *file, *msg;
	int		 locx, locy;
{
	Textsw_folio	 folio = FOLIO_FOR_VIEW(view);
	int		 fd;
	char		 *sys_msg;
	Es_status	 status;
	int		 cannot_open = 0;

	if ((fd = open(file, 0)) < 0) {
		cannot_open = (fd == -1);
		goto InternalError;
	};
	errno = 0;
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
					(caddr_t)TEXTSW_INFINITY-1);
	status = textsw_get_from_fd(view, fd, TRUE);
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				(caddr_t)TEXTSW_INFINITY-1);
	textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	(void) close(fd);
	if (status != ES_SUCCESS && status != ES_SHORT_WRITE)
		goto InternalError;
	return;
InternalError:
	if (cannot_open) {
/*   	char		*full_pathname;
	   
	   full_pathname = textsw_full_pathname(file); 
	   (void) sprintf(msg, "'%s': ", full_pathname);
	   free(full_pathname);
*/
	} else 
	   (void) sprintf(
	        msg, "%s", "Paste from file failed due to ERROR: ");
	sys_msg = (errno > 0 && errno < sys_nerr) ? sys_errlist[errno] : NULL;
	if (sys_msg)
	    (void) strcat(msg, sys_msg);
}


pkg_private Es_status
textsw_store_init(textsw, filename)
	Textsw_folio	 textsw;
	char		*filename;
{
	struct stat	 stat_buf;

	if (stat(filename, &stat_buf) == 0) {
	    Es_handle	 original = (Es_handle)LINT_CAST(
	    			  es_get(textsw->views->esh, ES_PS_ORIGINAL));
	    if AN_ERROR(original == ES_NULL) {
		return(ES_CANNOT_GET_NAME);
	    }
	    switch ((Es_enum)es_get(original, ES_TYPE)) {
	      case ES_TYPE_FILE:
		if (es_file_copy_status(original, filename) != 0)
		    return(ES_USE_SAVE);
		/* else fall through */
	      default:
		if ((stat_buf.st_size > 0) &&
		    (textsw->state & TXTSW_CONFIRM_OVERWRITE))
		    return(ES_CANNOT_OVERWRITE);
		break;
	    }
	} else if (errno != ENOENT) {
	    return(ES_CANNOT_OPEN_OUTPUT);
	}
	return(ES_SUCCESS);
}

/* ARGSUSED */
pkg_private Es_status
textsw_process_store_error(textsw, filename, status, locx, locy)
	Textsw_folio	 textsw;
	char		*filename;	/* Currently unused */
	Es_status	 status;
	int		 locx, locy;
{
	char		*msg1, *msg2;
	char		 msg[200];
	Event		 event;
	int		 result;

	switch (status) {
	  case ES_SUCCESS:
	    LINT_IGNORE(ASSUME(0));
	    return(ES_UNKNOWN_ERROR);
	  case ES_CANNOT_GET_NAME:
	    msg[0] = '\0';
	    msg1 = "Unable to Store as New File. ";
	    msg2 = "INTERNAL ERROR: Forgot the name of the file.";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
	  case ES_CANNOT_OPEN_OUTPUT:
	    msg[0] = '\0';
	    msg1 = "Unable to Store as New File. ";
	    msg2 = "Problems accessing specified file.";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
	  case ES_CANNOT_OVERWRITE:
		result = alert_prompt(
	            (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
		    &event,
	            ALERT_BUTTON_YES,	"Confirm",
		    ALERT_BUTTON_NO,	"Cancel",
	            ALERT_MESSAGE_STRINGS,
		        "Please confirm Store as New File:",
			filename,
			"  ",
			"That file exists and has data in it.",
		        0,
	            0);
	        if (result == ALERT_FAILED) {
	            return(ES_UNKNOWN_ERROR);
	        } else {
		    return((result == ALERT_YES) ?
			ES_SUCCESS : ES_UNKNOWN_ERROR);
	        }
	  case ES_FLUSH_FAILED:
	  case ES_FSYNC_FAILED:
	  case ES_SHORT_WRITE:
	    msg[0] = '\0';
	    msg1 = "Unable to Store as New File. ";
	    msg2 = "File system full.";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
	  case ES_USE_SAVE:
	    msg[0] = '\0';
	    msg1 = "Unable to Store as New File. ";
	    msg2 = "Use Save Current File instead.";
	    strcat(msg, msg1);
	    strcat(msg, msg2);
	    goto PostError;
    	  case ES_UNKNOWN_ERROR:	/* Fall through */
	  default:
	    goto InternalError;
	}
InternalError:
	msg[0] = '\0';
	msg1 = "Unable to Store as New File. ";
	msg2 = "An INTERNAL ERROR has occurred.";
	strcat(msg, msg1);
	strcat(msg, msg2);
PostError:
	(void) alert_prompt(
	        (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
	        &event,
	        ALERT_BUTTON_YES,	"Continue",
		ALERT_TRIGGER,		ACTION_STOP,
	        ALERT_MESSAGE_STRINGS,
		    msg1,
		    msg2,
		    0,
	        0);
	return(ES_UNKNOWN_ERROR);
}

extern unsigned
textsw_store_file(abstract, filename, locx, locy)
	Textsw			 abstract;
	char			*filename;
	int			 locx, locy;
{
	register Es_status	 status;
	Textsw_view		 view = VIEW_ABS_TO_REP(abstract);
	register Textsw_folio	 textsw = FOLIO_FOR_VIEW(view);

	status = textsw_store_init(textsw, filename);
	switch (status) {
	  case ES_SUCCESS:
	    break;
	  case ES_USE_SAVE:
	    if (textsw->state & TXTSW_STORE_SELF_IS_SAVE) {
		return(textsw_save(abstract, locx, locy));
	    }
	    /* Fall through */
	  default:
	    status = textsw_process_store_error(
				textsw, filename, status, locx, locy);
	    break;
	}
	if (status == ES_SUCCESS) {
	    status = textsw_save_store_common(textsw, filename,
	    			(textsw->state & TXTSW_STORE_CHANGES_FILE)
	    			? TRUE : FALSE);
	    if (status == ES_SUCCESS) {
		if (textsw->state & TXTSW_STORE_CHANGES_FILE)
		    textsw_notify(textsw->first_view,
		    		  TEXTSW_ACTION_LOADED_FILE, filename, 0);
	    } else {
		status = textsw_process_store_error(
				textsw, filename, status, locx, locy);
	    }
	}
	return((unsigned)status);
}

pkg_private Es_status
textsw_store_to_selection(textsw, locx, locy)
	Textsw_folio		textsw;
	int			locx, locy;
{
	char			filename[MAXNAMLEN];

	if (textsw_get_selection_as_filename(
		textsw, filename, sizeof(filename), locx, locy))
	    return(ES_CANNOT_GET_NAME);
	return ((Es_status)textsw_store_file(VIEW_REP_TO_ABS(textsw->first_view),
				  filename, locx, locy));
}

/* ARGSUSED */
extern void
textsw_reset_2(abstract, locx, locy, preserve_memory, cmd_is_undo_all_edit)
	Textsw		 abstract;
	int		 locx, locy;	/* Currently ignored */
	int		 preserve_memory;
	int		 cmd_is_undo_all_edit; /* This is for doing an "Undo All edit" */
					       /* in a cmdtool */
{
	static Es_status textsw_checkpoint_internal();
	Es_handle	 piece_esh, old_original_esh, new_original_esh;
	char		*name, save_name[MAXNAMLEN], scratch_name[MAXNAMLEN], *temp_name;
	int		 status;
	register Textsw_folio
			 folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
	register int	 old_count = folio->again_count;
	int		 old_memory_length = 0;
	int		 wrap_around_size = (int)es_get(
				    folio->views->esh, ES_PS_SCRATCH_MAX_LEN);

	if (preserve_memory) {
	    /* Get info about original esh before possibly invalidating it. */
	    old_original_esh = (Es_handle)LINT_CAST(es_get(
				    folio->views->esh, ES_PS_ORIGINAL));
	    if ((Es_enum)es_get(old_original_esh, ES_TYPE) ==
		ES_TYPE_MEMORY) {
		old_memory_length = es_get_length(old_original_esh);
	    }
	}
	if (textsw_has_been_modified(abstract) &&
	    (status = textsw_file_name(folio, &name)) == 0) {
	    /* Have edited a file, so reset to the file, not memory. */
	    /* First take a checkpoint, so recovery is possible. */
	    if (folio->checkpoint_frequency > 0) {
		if (textsw_checkpoint_internal(folio) == ES_SUCCESS) {
		    folio->checkpoint_number++;
		}
	    }
	    
	    /* This is for cmdsw log file */
	    /* When empty document load up the original empty tmp file */
	    /* instead of the one we did a store in */
	    temp_name = cmd_is_undo_all_edit ? NULL : 
	                 (char *)window_get(abstract, TEXTSW_TEMP_FILENAME);
	    
	    if (temp_name)
	        (void) strcpy(save_name, temp_name);
	    else   
	        (void) strcpy(save_name, name);
	        
	    status = textsw_load_file_internal(folio, save_name, scratch_name,
			&piece_esh, 0L, TXTSW_LFI_CLEAR_SELECTIONS);
			/*
			 * It would be nice to preserve the old positioning of
			 *   the views, but all of the material in a view may
			 *   be either new or significantly rearranged.
			 * One possiblity is to get the piece_stream to find
			 *   the nearest original stream position and use that
			 *   if we add yet another hack into ps_impl.c!
			 */
	    if (status == ES_SUCCESS)
		goto Return;
	    /* BUG ALERT: a few diagnostics might be appreciated. */
	}
	if (old_memory_length > 0) {
	    /* We are resetting from memory to memory, and the old
	     * memory had preloaded contents that we should preserve.
	     */
	    new_original_esh =
		es_mem_create(old_memory_length+1, "");
	    if (new_original_esh) {
		es_copy(old_original_esh, new_original_esh, FALSE);
	    }
	} else {
	    new_original_esh = es_mem_create(0, "");
	}
	piece_esh = textsw_create_mem_ps(folio, new_original_esh);
	if (piece_esh != ES_NULL) {
	    textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				 EV_SEL_PRIMARY);
	    textsw_set_selection(abstract, ES_INFINITY, ES_INFINITY,
				 EV_SEL_SECONDARY);
	    textsw_replace_esh(folio, piece_esh);
	    (void) ev_set(folio->views->first_view,
			  EV_FOR_ALL_VIEWS,
			  EV_DISPLAY_LEVEL, EV_DISPLAY_NONE,
			  EV_DISPLAY_START, 0,
			  EV_DISPLAY_LEVEL, EV_DISPLAY,
			  0);
			  
	    textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	    
	    textsw_notify(folio->first_view, TEXTSW_ACTION_USING_MEMORY, 0);
	}
Return:
	(void) textsw_set_insert(folio, 0L);
	textsw_init_again(folio, 0);
	textsw_init_again(folio, old_count);  /* restore number of again level */
	(void) es_set(folio->views->esh,
	       ES_PS_SCRATCH_MAX_LEN, wrap_around_size,
	       0);
}

extern void
textsw_reset(abstract, locx, locy)
	Textsw		 abstract;
	int		 locx, locy;
{
	textsw_reset_2(abstract, locx, locy, FALSE, FALSE);
}

pkg_private int
textsw_filename_is_all_blanks(filename)
	char		*filename;
{
	int		i = 0;
	
	while ((filename[i] == ' ') || (filename[i] == '\t')|| (filename[i] == '\n'))
		i++;
	return(filename[i] == '\0');	
}
	
/*** check filename for blanks and control chars ***/
pkg_private int
textsw_filename_contains_blanks(filename)
char		*filename;
{
	int		i = 0;
	
	while (filename[i] == ' ')      /* skip leading blanks */
	    i++;

	while (filename[i] != '\0') {
	    if (filename[i] < ' ' && filename[i] != '\n')
		return(TRUE);
	    else if (filename[i] == ' ' || filename[i] == '\n')
	        break;
	    i++;
	}

	while (filename[i] == ' ')      /* skip trailing blanks */
	    i++;

	if (filename[i] == '\0' || filename[i] == '\n')
	    return(FALSE);

	return(TRUE);	
}

/* Returns 0 iff a selection exists and it is matched by exactly one name. */
/* ARGSUSED */
/*** this function is not being referenced by anything in text directory ***/
pkg_private int
textsw_expand_filename_quietly(textsw, buf, err_buf, sizeof_buf, locx, locy)
	Textsw_folio	 textsw;
	char		*buf, *err_buf;
	int		 sizeof_buf;	/* BUG ALERT!  Being ignored. */
	int		 locx, locy;
{
	struct namelist	*nl;
	char		*msg, *msg_extra = 0;

	nl = expand_name(buf);
	if ((buf[0] == '\0') || (nl == NONAMES)) {
	    msg = "Unrecognized file name.  Unable to match specified pattern.";
	    msg_extra = buf;
	    goto PostError;
	}
	if (textsw_filename_is_all_blanks(buf)) {
	   msg = "Unrecognized file name.  Filename contains only blank or tab characters.";
	   goto PostError;
	}
	/*** check if filename contains blanks or control chars ***/
	if (textsw_filename_contains_blanks(buf)) {
	   msg = "Irregular file name.  Filename contains blanks or control characters.";
	   goto PostError;
	}
	/*
	 * Below here we have dynamically allocated memory pointed at by nl
	 *   that we must make sure gets deallocated.
	 */
	if (nl->count == 0) {
	    msg = "Unrecognized file name.  No files match specified pattern.";  
	    msg_extra = buf;
	} else if (nl->count > 1) {
	    msg = "Unrecognized file name.  Too many files match specified pattern"; 
	    msg_extra = buf;
	    goto PostError;
	} else
	    (void) strcpy(buf, nl->names[0]);
	free_namelist(nl);
	if (msg_extra != 0)
	    goto PostError;
	return(0);
PostError:
	strcat(err_buf, msg);/* strcat(err_buf, msg_extra); */
	return(1);
}
/* Returns 0 iff a selection exists and it is matched by exactly one name. */

pkg_private int
textsw_expand_filename(textsw, buf, sizeof_buf, locx, locy)
	Textsw_folio	 textsw;
	char		*buf;
	int		 sizeof_buf;	/* BUG ALERT!  Being ignored. */
	int		 locx, locy;
{
	struct namelist	*nl;
	char		*msg_extra = 0;
	int		result;
	Event		event;

	nl = expand_name(buf);
	if ((buf[0] == '\0') || (nl == NONAMES)) {
	    msg_extra = buf;
	    result = alert_prompt(
		    (Frame)window_get(
			VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
		    &event,
	            ALERT_BUTTON_YES,	"Continue",
		    ALERT_TRIGGER,	ACTION_STOP,
		    ALERT_MESSAGE_STRINGS,
			"Unrecognized file name.",
		        "Unable to expand specified pattern:",
		        buf,
		        0,
	            0);
	    if (result == ALERT_FAILED) {
	            goto PostError;
	    } else {
		    return (1);
	    }
	}
	if (textsw_filename_is_all_blanks(buf)) {
		result = alert_prompt(
		    (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw),
			WIN_OWNER),
		    &event,
	            ALERT_BUTTON_YES,	"Continue",
		    ALERT_TRIGGER,	ACTION_STOP,
		    ALERT_MESSAGE_STRINGS,
			"Unrecognized file name.",
			"File name contains only blank or tab characters.",
			"Please use a valid file name.",
		        0,
	            0);
	        if (result == ALERT_FAILED) {
	            goto PostError;
	        } else {
		    return (1);
	        }
	}
	/*** check if filename contains blanks or control chars ***/
	else if (textsw_filename_contains_blanks(buf)) {
		result = alert_prompt(
		    (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw),
			WIN_OWNER),
		    &event,
	            ALERT_BUTTON_YES,	"Confirm",
		    ALERT_BUTTON_NO,	"Cancel", 
		    ALERT_MESSAGE_STRINGS,
			"Irregular file name.",
			"File name contains blanks or control characters.",
			"Please confirm or cancel file name.",
		        0,
	            0);
	        if (result == ALERT_FAILED) {
	            goto PostError;
	        } else {
		    return ((result == ALERT_YES) ?
			ES_SUCCESS : ES_UNKNOWN_ERROR);
	        }
	}
	/*
	 * Below here we have dynamically allocated memory pointed at by nl
	 *   that we must make sure gets deallocated.
	 */
	if (nl->count == 0) {
	    msg_extra = buf;
	    result = alert_prompt(
		    (Frame)window_get(
			VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
		    &event,
	            ALERT_BUTTON_YES,	"Continue",
		    ALERT_TRIGGER,	ACTION_STOP,
		    ALERT_MESSAGE_STRINGS,
			"Unrecognized file name.",
			"No files match specified pattern:",
		        buf,
		        0,
	            0);
	    if (result == ALERT_FAILED) {
	            goto PostError;
	    } else {
		    return (1);
	    }
	} else if (nl->count > 1) {
	    msg_extra = buf;
	    result = alert_prompt(
		    (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
		    &event,
	            ALERT_BUTTON_YES,	"Continue",
		    ALERT_TRIGGER,	ACTION_STOP,
		    ALERT_MESSAGE_STRINGS,
			"Unrecognized file name.",
			"Too many files match specified pattern:",
		        buf,
		        0,
	            0);
	    if (result == ALERT_FAILED) {
	            goto PostError;
	    } else {
		    return (1);
	    }
	} else
	    (void) strcpy(buf, nl->names[0]);
	free_namelist(nl);
	if (msg_extra != 0)
	    goto PostError;
	return(0);
PostError:
	return(1);
}

/* Returns 0 iff there is a selection, and it is matched by exactly one name. */
pkg_private int
textsw_get_selection_as_filename(textsw, buf, sizeof_buf, locx, locy)
	Textsw_folio		 textsw;
	char			*buf;
	int			 sizeof_buf, locx, locy;
{
	Event			event;

	if (!textsw_get_selection_as_string(textsw, EV_SEL_PRIMARY,
					    buf, sizeof_buf)) {
	    goto PostError;
	}
	return(textsw_expand_filename(textsw, buf, sizeof_buf, locx, locy));
PostError:
	(void) alert_prompt(
		(Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
		&event,
		ALERT_MESSAGE_STRINGS,
		    "Please select a filename and choose this menu option again.",
		    0,
	        ALERT_BUTTON_YES,	"Continue",
		ALERT_TRIGGER,		ACTION_STOP,
	        0);
	return(1);
}

pkg_private int
textsw_possibly_edited_now_notify(textsw)
	Textsw_folio	textsw;
{
	char		*name;

	if (textsw_has_been_modified(VIEW_REP_TO_ABS(textsw->first_view))) {
	/* 
	 * WARNING: client may textsw_reset() during notification, hence
	 * edit flag's state is only known BEFORE the notification and must be
	 * set before the notification. 
	 */
	 
	    textsw->state |= TXTSW_EDITED;
	    
	    if (textsw_file_name(textsw, &name) == 0) 
		textsw_notify(textsw->first_view,
			      TEXTSW_ACTION_EDITED_FILE, name, 0);
	    else
	        textsw_notify(textsw->first_view,
			      TEXTSW_ACTION_EDITED_MEMORY, 0);
	    
	}
}

extern int
textsw_has_been_modified(abstract)
	Textsw		abstract;
{
	Textsw_folio	folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));

	if (folio->state & TXTSW_INITIALIZED) {
	    return((int)es_get(folio->views->esh, ES_HAS_EDITS));
	}
	return(0);
}

pkg_private int
textsw_file_name(textsw, name)
	Textsw_folio	  textsw;
	char		**name;
/* Returns 0 iff editing a file and name could be successfully acquired. */
{
	Es_handle	  original;

	original = (Es_handle)LINT_CAST(
		   es_get(textsw->views->esh, ES_PS_ORIGINAL));
	if (original == 0)
	    return(1);
	if ((Es_enum)es_get(original, ES_TYPE) != ES_TYPE_FILE)
	    return(2);
	if ((*name = (char *)es_get(original, ES_NAME)) == NULL)
	    return(3);
	if (name[0] == '\0')
	    return(4);
	return(0);
}

extern int
textsw_append_file_name(abstract, name)
	Textsw		 abstract;
	char		*name;
/* Returns 0 iff editing a file and name could be successfully acquired. */
{
	Textsw_folio	 textsw = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
	char		*internal_name;
	int		 result;

	result = textsw_file_name(textsw, &internal_name);
	if (result == 0)
	    (void) strcat(name, internal_name);
	return(result);
}


/*	===================================================================
 *
 *	Misc. file system manipulation utilities
 *
 *	===================================================================
 */

/* Returns 0 iff change directory succeeded. */
static int
textsw_change_directory(textsw, filename, might_not_be_dir, locx, locy)
	Textsw_folio	 textsw;
	char		*filename;
	int		 might_not_be_dir;
	int		 locx, locy;
{
	char		*sys_msg;
	char		*full_pathname;
	char		 alert_msg[MAXNAMLEN+100];
	struct stat	 stat_buf;
	int		 result = 0;
	Event		 event;

	errno = 0;
	if (stat(filename, &stat_buf) < 0) {
	    result = -1;
	    goto Error;
	}
	if ((stat_buf.st_mode&S_IFMT) != S_IFDIR) {
	    if (might_not_be_dir)
		return(-2);
	}
	if (chdir(filename) < 0) {
	    result = errno;
	    goto Error;
	}
	textsw_notify((Textsw_view)textsw,	/* Cast is for lint */
		      TEXTSW_ACTION_CHANGED_DIRECTORY, filename, 0);
	return(result);

Error:
	full_pathname = textsw_full_pathname(filename);
	(void) sprintf(alert_msg, "Unable to %s:",
		(might_not_be_dir ? "access file" : "cd to directory"));
	sys_msg = (errno > 0 && errno < sys_nerr) ? sys_errlist[errno] : NULL;
	(void) alert_prompt(
	        (Frame)window_get(VIEW_FROM_FOLIO_OR_VIEW(textsw), WIN_OWNER),
	        &event,
	        ALERT_BUTTON_YES,	"Continue",
		ALERT_TRIGGER,		ACTION_STOP,
	        ALERT_MESSAGE_STRINGS,
	            alert_msg,
		    full_pathname,
		    sys_msg,
	            0,
	        0);
	free(full_pathname);
	return(result);
}

/* Returns 0 iff change directory succeeded. */
pkg_private int
textsw_change_directory_quietly(textsw, filename, err_msgs, might_not_be_dir, locx, locy)
	Textsw_folio	 textsw;
	char		*filename, *err_msgs;
	int		 might_not_be_dir;
	int		 locx, locy;
{
	char		*sys_msg;
	char		*full_pathname;
	struct stat	 stat_buf;
	int		 result = 0;

	errno = 0;
	if (stat(filename, &stat_buf) < 0) {
	    result = -1;
	    goto Error;
	}
	if ((stat_buf.st_mode&S_IFMT) != S_IFDIR) {
	    if (might_not_be_dir)
		return(-2);
	}
	if (chdir(filename) < 0) {
	    result = errno;
	    goto Error;
	}
	textsw_notify((Textsw_view)textsw,	/* Cast is for lint */
		      TEXTSW_ACTION_CHANGED_DIRECTORY, filename, 0);
	return(result);

Error:
	full_pathname = textsw_full_pathname(filename);
	(void) sprintf(err_msgs, "Cannot %s '%s': ",
		       (might_not_be_dir ? "access file" : "cd to directory"),
		       full_pathname);

	(void) sprintf(err_msgs, "Cannot %s",
		       (might_not_be_dir ? "access file: " :
					   "cd to directory: "));
	free(full_pathname);
	sys_msg = (errno > 0 && errno < sys_nerr) ? sys_errlist[errno] : NULL;
	if (sys_msg)
	    strcat(err_msgs, sys_msg);
	return(result);
}

pkg_private Es_status
textsw_checkpoint_internal(folio)
	Textsw_folio	folio;
{
	Es_handle		 cp_file;
	Es_status		 result;
	
	if (!folio->checkpoint_name) {
	    char	*name;
	    if (textsw_file_name(folio, &name) != 0)
		return(ES_CANNOT_GET_NAME);
	    if ((folio->checkpoint_name = (char *)malloc(MAXNAMLEN)) == 0)
		return(ES_CANNOT_GET_NAME);
	    (void) sprintf(folio->checkpoint_name, "%s%%%%", name);
	}

	cp_file = es_file_create(folio->checkpoint_name,
				 ES_OPT_APPEND, &result);
	if (!cp_file) {
	    /* BUG ALERT!  For now, don't look at result. */
	    return(ES_CANNOT_OPEN_OUTPUT);
	}
	result = es_copy(folio->views->esh, cp_file, TRUE);
	es_destroy(cp_file);

	return(result);
}


/*
 *  If the number of edits since the last checkpoint has exceeded the
 *  checkpoint frequency, take a checkpoint.
 *  Return ES_SUCCESS if we do the checkpoint.
 */
pkg_private Es_status
textsw_checkpoint(folio)
	Textsw_folio	folio;
{
	int		edit_number = (int)LINT_CAST(
					ev_get((Ev_handle)folio->views,
						  EV_CHAIN_EDIT_NUMBER));
	Es_status	result = ES_DID_NOT_CHECKPOINT;

	if (folio->state & TXTSW_IN_NOTIFY_PROC ||
	    folio->checkpoint_frequency <= 0)
	    return(result);
	if ((edit_number / folio->checkpoint_frequency)
	                > folio->checkpoint_number) {
	    /* do the checkpoint */
	    result = textsw_checkpoint_internal(folio);
	    if (result == ES_SUCCESS) {
	        folio->checkpoint_number++;
	    }
	}
	return(result);
}

pkg_private void
textsw_empty_document(abstract, ie)
    Textsw	abstract;
    Event	*ie;
{
    register Textsw_view	view = VIEW_ABS_TO_REP(abstract);
    register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);
    int				has_been_modified = 
    				  textsw_has_been_modified(abstract);
    int				number_of_resets = 0;
    int				is_cmdtool = 
    				  (textsw->state & TXTSW_NO_RESET_TO_SCRATCH);
    int				result;
    Event			event;

    if (!textsw_menu_style_already_gotten) {
	textsw_menu_style = textsw_get_menu_style_internal(textsw);
	textsw_menu_style_already_gotten = 1;
    }

    if (textsw_menu_style == TEXTSW_STYLE_ORGANIZED) {
        if (has_been_modified) {
	        result = alert_prompt(
		    (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
	            &event,
	            ALERT_MESSAGE_STRINGS,
		        "The text has been edited.",
		        "Empty Document will discard these edits. Please confirm.",
	                0,
	            ALERT_BUTTON_YES,	"Confirm, discard edits",
		    ALERT_BUTTON_NO,	"Cancel",
	            0);
	        if (result != ALERT_YES)
		    return;
	        textsw_reset(abstract, ie->ie_locx, ie->ie_locy);
	        number_of_resets++; 
        }
    } else { 
        if (textsw_has_been_modified(abstract)) {
	        result = alert_prompt(
	            (Frame)window_get(
			WINDOW_FROM_VIEW(view), WIN_OWNER),
	            &event,
	            ALERT_MESSAGE_STRINGS,
		        "The text has been edited.",
	                "Reset will discard these edits. Please confirm.",
	                0,
	            ALERT_BUTTON_YES,	"Confirm, discard edits",
		    ALERT_BUTTON_NO,	"Cancel",
	            0);
	        if (result == ALERT_FAILED)
	            return;
	        if (result == ALERT_NO)
		    return;
        }
    }
    if (is_cmdtool) {
        if ((has_been_modified) && (number_of_resets == 0))
            textsw_reset(abstract, ie->ie_locx, ie->ie_locy);        
    } else {
        textsw_reset(abstract, ie->ie_locx, ie->ie_locy);
    }
}

pkg_private void
textsw_post_need_selection(abstract, ie)
    Textsw	abstract;
    Event	*ie;
{
    register Textsw_view	view = VIEW_ABS_TO_REP(abstract);
    Event	event;
    int		result;

    result = alert_prompt(
            (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
            &event,
	    ALERT_MESSAGE_STRINGS,
                "Please select a file name and choose this option again.",
                0,
            ALERT_BUTTON_YES,	"Continue",
            ALERT_TRIGGER,		ACTION_STOP,
            0);
    if (result == ALERT_FAILED)
    	    return;
    if (result == ALERT_NO)
           return;
}

pkg_private void
textsw_load_file_as_menu(abstract, ie)
    Textsw	abstract;
    Event	*ie;
{
    register Textsw_view	view = VIEW_ABS_TO_REP(abstract);
    register Textsw_folio	textsw = FOLIO_FOR_VIEW(view);
    Event			event;
    int				result, no_cd;
    int                         is_cmdtool =
                                 (textsw->state & TXTSW_NO_RESET_TO_SCRATCH);
    char                        msg[MAXNAMLEN+100];

   
    if 	(textsw->state& TXTSW_NO_LOAD) {
	(void) alert_prompt(
                (Frame)window_get (VIEW_FROM_FOLIO_OR_VIEW(textsw),
                       WIN_OWNER),
		       &event,
	               ALERT_BUTTON_YES, "Continue",
		       ALERT_TRIGGER, ACTION_STOP,
		       ALERT_MESSAGE_STRINGS, "Load File Operation Not Allowed",
		       0, 0); 
	return;
    }
    if (textsw_has_been_modified(abstract)) {
	result = alert_prompt(
	    (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER),
	    &event,
	    ALERT_MESSAGE_STRINGS,
	        "The text has been edited.",
	        "Load File will discard these edits. Please confirm.",
	        0,
	    ALERT_BUTTON_YES,	"Confirm, discard edits",
	    ALERT_BUTTON_NO,	"Cancel",
	    0);
        if (result == ALERT_FAILED)
	    return;
        if (result == ALERT_NO)
	    return;
    }
    no_cd = ((textsw->state & TXTSW_NO_CD) ||
	 ((textsw->state & TXTSW_LOAD_CAN_CD) == 0));
    if (textsw_load_selection(textsw,
	ie->ie_locx, ie->ie_locy, no_cd) == 0) {
    }
}

/* return 0 if textsw was readonly,
	  1 if should allow insert, accelerator not translated
	  2 shouldn't insert, accelerator translated
*/
pkg_private int
textsw_handle_esc_accelerators(abstract, ie)
    Textsw	abstract;
    Event	*ie;
{
    register Textsw_view   view = VIEW_ABS_TO_REP(abstract);
    register Textsw_folio  textsw = FOLIO_FOR_VIEW(view);
    char		   dummy[1024];
    int			   length = es_get_length(textsw->views->esh);

    if (event_shift_is_down(ie)) { /* select & include current line */
        unsigned	span_to_beginning_flags =
				(EI_SPAN_LINE | EI_SPAN_LEFT_ONLY);
	unsigned	span_line_flags =
				(EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY);
	Ev_chain	chain = textsw->views;
	Es_index	first = 0;
	Es_index	last_plus_one, insert_pos;

	textsw_flush_caches(view, TFC_STD);
	insert_pos = ev_get_insert(chain);
	if (insert_pos != 0) {
	    /* first go left from insertion pt */
	    (void)ev_span(chain, insert_pos,
		&first, &last_plus_one, span_to_beginning_flags);
	}
	/* then go right to end of line from line beginning */
	(void)ev_span(chain, first,
	    &first, &last_plus_one, span_line_flags);
	/* check for no selection because at beginning of new line */
	if ((first == last_plus_one) && (insert_pos != 0)) {
	    insert_pos--;
	    first = 0;
	    last_plus_one = 0;
	    if (insert_pos != 0) {
	        /* first go left from insertion pt */
	        (void)ev_span(chain, insert_pos,
		    &first, &last_plus_one, span_to_beginning_flags);
	    }
	    /* then go right to end of line from line beginning */
	    (void)ev_span(chain, first,
		&first, &last_plus_one, span_line_flags);
	}
	textsw_set_insert(textsw, last_plus_one);
	textsw_set_selection(abstract, 
	    first, last_plus_one, (EV_SEL_PRIMARY|EV_SEL_PD_PRIMARY));
	(void)textsw_file_stuff(view, first, last_plus_one);

    } else if (0 == textsw_file_name(textsw, dummy)) {
	if (TXTSW_IS_READ_ONLY(textsw))
	    return (0);
	return (1);

    } else { /* select and load-file current line */
        unsigned	span_to_beginning_flags =
				(EI_SPAN_LINE | EI_SPAN_LEFT_ONLY);
	unsigned	span_line_flags =
				(EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY);
	unsigned	span_document_flag = EI_SPAN_DOCUMENT;
	Ev_chain	chain = textsw->views;
	Es_index	first = 0, first_in_doc = 0;
	Es_index	last_in_doc_plus_one, last_plus_one, insert_pos;
        Event           event;
        int             result;
        char            filename[MAXNAMLEN];

	textsw_flush_caches(view, TFC_STD);
	insert_pos = ev_get_insert(chain);
	if (insert_pos != 0) {
	    /* first go left from insertion pt */
	    (void)ev_span(chain, insert_pos,
		&first, &last_plus_one, span_to_beginning_flags);
	}
	/* then go right to end of line from line beginning */
	(void)ev_span(chain, first,
	    &first, &last_plus_one, span_line_flags);
	if ((first == last_plus_one) && (insert_pos != 0)) {
	    insert_pos--;
	    first = 0;
	    last_plus_one = 0;
	    if (insert_pos != 0) {
	        /* first go left from insertion pt */
	        (void)ev_span(chain, insert_pos,
		    &first, &last_plus_one, span_to_beginning_flags);
	    }
	    /* then go right to end of line from line beginning */
	    (void)ev_span(chain, first,
		&first, &last_plus_one, span_line_flags);
	}
	textsw_set_insert(textsw, last_plus_one);
	textsw_set_selection(abstract, first, last_plus_one, EV_SEL_PRIMARY);
        /* span document */
	(void)ev_span(chain, first,
		&first_in_doc, &last_in_doc_plus_one, span_document_flag);
        /* if document size is not equal to selection */
        /* put up alert...may have inadvertently hit esc */
        /* with potential to lose edits */
        if ((last_plus_one - first) != last_in_doc_plus_one) {
           /* if name could be a directory */
           if (!(int)(textsw->state & TXTSW_NO_CD)) {
	      result = textsw_get_selection_as_filename(textsw, filename,
                           sizeof(filename), ie->ie_locx, ie->ie_locy);  
              /* if cancel -- mistakenly hit ESCAPE accelerator */
              if (result != 0) return 2;

              /* if directory, change to it */
              result = textsw_change_directory(textsw, filename, TRUE,
                                  ie->ie_locx, ie->ie_locy);
              /* if file or directory does not exist */
              if (result == -1) return 2;
            }
            /* if file exists and not a directory */
            if (result == -2) {
	       result = alert_prompt(
                  (Frame)window_get(WINDOW_FROM_VIEW(view), WIN_OWNER), &event,
                  ALERT_MESSAGE_STRINGS,
	          "The text has been edited.",
	          "Load File will discard these edits. Please confirm.",
	          0,
	          ALERT_BUTTON_YES,	"Confirm, discard edits",
	          ALERT_BUTTON_NO,	"Cancel",
	          0);
               if (result == ALERT_FAILED) return 2;
               if (result == ALERT_NO) return 2;
            }  
        }
	if (textsw_load_selection(textsw, ie->ie_locx,
		ie->ie_locy,
		(int)(textsw->state & TXTSW_NO_CD)) == 0) {
	    textsw->state &= ~TXTSW_READ_ONLY_ESH;
        }
    }
    return (2);
}

#if defined(DEBUG) && !defined(lint)
static char           *header = "fd      dev: #, type    inode\n";
static void
debug_dump_fds(stream)
	FILE                 *stream;
{
	register int          fd;
	struct stat           s;

	if (stream == 0)
	    stream = stderr;
	fprintf(stream, header);
	for (fd = 0; fd < 32; fd++) {
            if (fstat(fd, &s) != -1) {
        	fprintf(stream, "%2d\t%6d\t%4d\t%5d\n",
        		fd, s.st_dev, s.st_rdev, s.st_ino);
            }
	}
}
#endif

