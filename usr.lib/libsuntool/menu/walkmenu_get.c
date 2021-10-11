#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)walkmenu_get.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WALKING MENU PACKAGE

	walkmenu_get.c, Sun Jun 30 15:38:39 1985

		Craig Taylor,
		Sun Microsystems
 */

/* ------------------------------------------------------------------------- */

#include <sys/types.h>
#include <stdio.h>
#include <varargs.h>

#include <sunwindow/sun.h> /* for LINT_CAST */
#include <suntool/walkmenu_impl.h>

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */
    /* None */

/* 
 * Package private
 */
Pkg_private	caddr_t	menu_gets();
Pkg_private	caddr_t menu_item_gets();

/* 
 * Private
 */
    /* None */

/* 
 * Private defs
 */
    /* None */

/* ------------------------------------------------------------------------- */

#define	CADDR_CAST(arg) ((caddr_t)LINT_CAST((arg)))

Pkg_private caddr_t
menu_gets(m, attr, value)
	struct menu *m;
	Menu_attribute attr;
	caddr_t value;
{   
    caddr_t v = NULL;
    
    switch (attr) {

      case MENU_BOXED:
	 v = (caddr_t)m->default_image.boxed;
	break;
	    
      case MENU_CENTER:
	v = (caddr_t)m->default_image.center;
	break;
	    
      case MENU_CLIENT_DATA:
	v = (caddr_t)m->client_data;
	break;

      case HELP_DATA:
	v = (caddr_t)m->help_data;
	break;
	    
      case MENU_COLUMN_MAJOR:
	v = (caddr_t)m->column_major;
	break;
	    
      case MENU_DEFAULT:
	v = (caddr_t)m->default_position;
	break;
	    
      case MENU_DEFAULT_ITEM:
	if (range(m->default_position, 1, m->nitems))
	    v = (caddr_t)m->item_list[m->default_position - 1];
	break;
	    
      case MENU_DEFAULT_IMAGE:
	v = (caddr_t)&m->default_image;
	break;
	    
      case MENU_DEFAULT_SELECTION:
	v = (caddr_t)m->default_selection;
	break;
	    
      case MENU_FIRST_EVENT:
	v = m->dynamic_info ? (caddr_t)m->dynamic_info->first_iep : NULL;
	break;
	    
      case MENU_FONT:
	v = image_get(&m->default_image, &m->default_image, IMAGE_FONT);
	break;
	    
      case MENU_GEN_PROC:
	v = CADDR_CAST(m->gen_proc);
	break;
	    
      case MENU_HEIGHT:
	v = (caddr_t)m->height;
	break;
	    
      case MENU_INITIAL_SELECTION:
      /* case MENU_ACCELERATED_SELECTION: Obsolete */
	v = (caddr_t)m->initial_selection;
	break;
	    
      case MENU_INITIAL_SELECTION_EXPANDED:
	v = (caddr_t)!m->display_one_level;
	break;
	    
      case MENU_INITIAL_SELECTION_SELECTED:
	v = (caddr_t)!m->stand_off;
	break;
	    
      case MENU_JUMP_AFTER_SELECTION:
	v = (caddr_t)m->jump_after_selection;
	break;

      case MENU_JUMP_AFTER_NO_SELECTION:
	v = (caddr_t)m->jump_after_no_selection;
	break;

      case MENU_LAST_EVENT:
	v = m->dynamic_info ? (caddr_t)m->dynamic_info->last_iep : NULL;
	break;
	    
      case MENU_LEFT_MARGIN:
	v = (caddr_t)m->default_image.left_margin;
	break;
	    
/*    case /--/MENU_LIKE: Copy menu and items into current menu
	break;
 */	    
      case MENU_MARGIN:
	v = (caddr_t)m->default_image.margin;
	break;
	    
      case MENU_NOTIFY_PROC:
	v = CADDR_CAST(m->notify_proc);
	break;
	    
      case MENU_NTH_ITEM:
      	if ((int)value > 0 && (int)value <= m->nitems)
	    v = (caddr_t)m->item_list[(int)value - 1];
	break;
      
      case MENU_NCOLS:
	/* FIXME: to return correct size m->ncols (e.g., ifdef notdef below) */
	v = (caddr_t)m->ncols;
#ifdef	notdef
	    int cols, rows;
	    menu_compute_size(m, &cols, &rows);
	    v = (caddr_t)cols;
#endif	notdef
	break;

      case MENU_NITEMS:
	v = (caddr_t)m->nitems;
	break;

      case MENU_NROWS:
	/* FIXME: to return correct size m->nrows (e.g., ifdef notdef below) */
	v = (caddr_t)m->nrows;
#ifdef	notdef
	    int cols, rows;
	    menu_compute_size(m, &cols, &rows);
	    v = (caddr_t)rows;
#endif	notdef
	break;

      case MENU_PARENT:
        v = (caddr_t)m->parent;
	break;

      case MENU_PULLRIGHT_DELTA:
        v = (caddr_t)m->pullright_delta;
	break;

      case MENU_RIGHT_MARGIN:
	v = (caddr_t)m->default_image.right_margin;
	break;
	    
      case MENU_SELECTED:
	v = (caddr_t)m->selected_position;
	break;
	    
      case MENU_SELECTED_ITEM:
	if (range(m->selected_position, 1, m->nitems))
	    v = (caddr_t)m->item_list[m->selected_position - 1];
	break;
	    
      case MENU_SHADOW:
	v = (caddr_t)m->shadow_pr;
	break;
	    
      case MENU_STAY_UP:
	v = (caddr_t)m->stay_up;
	break;
	    
      case MENU_TYPE:
	v = (caddr_t)MENU_MENU;
	break;
      
      case MENU_VALID_RESULT:
	v = (caddr_t)m->valid_result;
	break;
      
      case MENU_WIDTH:
	v = (caddr_t)m->width;
	break;
	    
      case MENU_NOP:
	break;
	
      default:
	if (ATTR_PKG_MENU == ATTR_PKG(attr))
	    (void) fprintf(stderr,
	"menu_get: 0x%x not recognized as a menu attribute.\n", attr);
	break;
	    
    }
    return v;
}


/* ARGSUSED */
Pkg_private caddr_t
menu_item_gets(mi, attr, value)
	struct menu_item *mi;
	Menu_attribute attr;
	caddr_t value;
{   
    caddr_t v = NULL;
    
    switch (attr) {

      case MENU_ACTION: /* & case MENU_NOTIFY_PROC: */
	v = CADDR_CAST(mi->notify_proc);
	break;
	    
      case MENU_BOXED:
	if (mi->image) v = (caddr_t)mi->image->boxed;
	break;
	    
      case MENU_CENTER:
	if (mi->image) v = (caddr_t)mi->image->center;
	break;
	    
      case MENU_CLIENT_DATA:
	v = (caddr_t)mi->client_data;
	break;

      case HELP_DATA:
	v = (caddr_t)mi->help_data;
	break;
	    
      case MENU_FEEDBACK:
	v = (caddr_t)!mi->no_feedback;
	break;
      
      case MENU_FONT:
	v = image_get(mi->image, (struct image *)NULL, IMAGE_FONT);
	break;
	    
      case MENU_GEN_PROC:
	v = CADDR_CAST(mi->gen_proc);
	break;
	    
      case MENU_GEN_PULLRIGHT:
	v = CADDR_CAST(mi->gen_pullright);
	break;
	    
      case MENU_IMAGE: /* Questionable use of the word image */
	if (mi->image) v = (caddr_t)mi->image->pr;
	break;

      case MENU_INACTIVE:
	v = (caddr_t)mi->inactive;
	break;
	    
      case MENU_INVERT:
	if (mi->image) v = (caddr_t)mi->image->invert;
	break;
	    
      case MENU_LEFT_MARGIN:
	if (mi->image) v = (caddr_t)mi->image->left_margin;
	break;
	    
      case MENU_MARGIN:
	if (mi->image) v = (caddr_t)mi->image->margin;
	break;
	    
      case MENU_PARENT:
        v = (caddr_t)mi->parent;
	break;

      case MENU_PULLRIGHT:
	v = (caddr_t)(mi->pullright ? mi->value : 0);
	break;
	    
      case MENU_RELEASE:
	v = (caddr_t)mi->free_item;
	break;
	    
      case MENU_RELEASE_IMAGE:
	if (mi->image)
	    v = (caddr_t)(mi->image->free_string || mi->image->free_pr);
	break;
	    
      case MENU_RIGHT_MARGIN:
	if (mi->image) v = (caddr_t)mi->image->right_margin;
	break;
	    
      case MENU_SELECTED:
	v = (caddr_t)mi->selected;
	break;
	    
      case MENU_STRING:
	if (mi->image) v = mi->image->string;
	break;

      case MENU_TYPE:
	v = (caddr_t)MENU_ITEM;
	break;
      
      case MENU_VALUE: /* FIXME: If mi parent is invalid this will die */
	if (mi->pullright && mi->parent && mi->parent->dynamic_info)
	    v = (caddr_t)menu_pullright_return_result((Menu_item)mi);
	else
	    v = (caddr_t)mi->value;
	break;
	    
      case MENU_NOP:
	break;
	
      default:
	if (ATTR_PKG_MENU == ATTR_PKG(attr))
	    (void) fprintf(stderr,
	"menu_set: 0x%x not recognized as a menu attribute.\n", attr);
	break;
	    
    }
    return v;   
}

/*  FIXME: to get screen size */
#ifdef not_implemented
/* ARGSUSED */
Private
menu_compute_size(m, colp, rowp)
	struct menu *m;
{
    register int col = *colp, row = *rowp;
    if (!(cols && rows)) return;
    if (cols) {    /*  case: cols=n, rows=to fit */
	    if (!cols) cols = m->nitems;
	    menurect.r_width = (m->ncols * max_width) +
		2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
	    if (menurect.r_width > m_fs->fs_screenrect.r_width) {
		cols = (m_fs->fs_screenrect.r_width
			- 2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0))
		       / max_width;
	    }
	    rows = (m->nitems - 1) / cols + 1;
	} else { /* case: rows=n, cols=to fit */
	    menurect.r_height = (m->nrows * max_height) +
		2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0);
	    if (menurect.r_height > m_fs->fs_screenrect.r_height) {
		rows = (m_fs->fs_screenrect.r_height
			- 2*MENU_BORDER + (m->shadow_pr ? MENU_SHADOW : 0))
		       / max_height;
	    }
	    cols = (m->nitems - 1) / rows + 1;
	}
    *colp = col, *rowp = row;
}
#endif
