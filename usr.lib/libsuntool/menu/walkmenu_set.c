#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)walkmenu_set.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WALKING MENU PACKAGE

	walkmenu_set.c, Sun Jun 30 15:38:39 1985

		Craig Taylor,
		Sun Microsystems
 */

/* ------------------------------------------------------------------------- */

#include <sys/types.h>
#include <stdio.h>
#include <varargs.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
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
Pkg_extern	struct menu_item *menu_create_item_avlist();
Pkg_extern	struct menu_ops_vector menu_ops;
Pkg_extern	struct menu_ops_vector menu_item_ops;

/* Pullright arrow pixrect.  This really is Pkg_private despite mpr_static. */
Private		short arrow_data[] = {
/*  0x0200, 0x0300, 0x7E80, 0x00C0, 0x7E80, 0x0300, 0x0200, 0x0000 (width=12) */
    0x0080, 0x00C0, 0x1FA0, 0x0030, 0x1FA0, 0x00C0, 0x0080, 0x0000
};
Private		mpr_static(menu_arrow_pr, 14, 8, 1, arrow_data);

Pkg_private	void menu_sets();
Pkg_private	void menu_item_sets();
Pkg_private	void menu_destroys();
Pkg_private	void menu_item_destroys();

/* 
 * Private
 */
Private		int extend_item_list();
Private		int remove_item();
Private		int replace_item();
Private		int insert_item();
Private		int lookup();

/* 
 * Private defs
 */
    /*  None */

/* ------------------------------------------------------------------------- */

Pkg_private void
menu_sets(m, attrs)
	register struct menu *m;
	register Menu_attribute *attrs;
{   
    for (; *attrs; attrs = menu_attr_next(attrs))
	switch (attrs[0]) {

	  case MENU_ACTION_IMAGE:
	  case MENU_ACTION_ITEM:
	  case MENU_GEN_PULLRIGHT_IMAGE:
	  case MENU_GEN_PULLRIGHT_ITEM:
	  case MENU_GEN_PROC_IMAGE:
	  case MENU_GEN_PROC_ITEM:
	  case MENU_IMAGE_ITEM:
	  case MENU_PULLRIGHT_IMAGE:
	  case MENU_PULLRIGHT_ITEM:
	  case MENU_STRING_ITEM:
	    if (m->nitems < m->max_nitems || extend_item_list(m)) {
		m->item_list[m->nitems++] = (struct menu_item *)LINT_CAST(
		    menu_create_item(LINT_CAST(MENU_RELEASE),
				     attrs[0], attrs[1], attrs[2], 0));
	    }
	    break;

	  case MENU_APPEND_ITEM:
	    if (m->nitems < m->max_nitems || extend_item_list(m))
		m->item_list[m->nitems++] = (struct menu_item *)attrs[1];
	    break;
	    
	  case MENU_BOXED:
	    (void) image_set(&m->default_image, IMAGE_BOXED, attrs[1], 0);
	    break;

	  case MENU_CENTER:
	    (void) image_set(&m->default_image, IMAGE_CENTER, attrs[1], 0);
	    break;
	    
	  case MENU_CLIENT_DATA:
	    m->client_data = (caddr_t)attrs[1];
	    break;
	    
	  case HELP_DATA:
	    m->help_data = (caddr_t)attrs[1];
	    break;
	    
	  case MENU_COLUMN_MAJOR:
	    m->column_major = (int)attrs[1];
	    break;
	    
	  case MENU_DEFAULT:
	    m->default_position = (int)attrs[1];
	    break;
	    
	  case MENU_DEFAULT_ITEM:
	    m->default_position = lookup(m->item_list, m->nitems,
					(struct menu_item *)LINT_CAST(attrs[1]));
	    break;
	    
	  case MENU_DEFAULT_IMAGE:
	    if (attrs[1]) m->default_image = *(struct image *)attrs[1];
	    break;
	    
	  case MENU_DEFAULT_SELECTION:
	    m->default_selection = (Menu_attribute)attrs[1];
	    break;
	    
	  case MENU_FONT:
	    (void) image_set(&m->default_image, IMAGE_FONT, attrs[1], 0);
	    break;

	  case MENU_GEN_PROC:
	    m->gen_proc = (struct menu *(*)())attrs[1];
	    break;

	  case MENU_HEIGHT:
	    m->height = (int)attrs[1];
	    break;
	    
	  case MENU_IMAGES:
	    {   
		char **a = (char **)&attrs[1];
		while (*a) {
		    if (m->nitems < m->max_nitems || extend_item_list(m)) {
			m->item_list[m->nitems] = (struct menu_item *)LINT_CAST(
				menu_create_item(LINT_CAST(MENU_RELEASE),
				    MENU_IMAGE_ITEM, *a++, m->nitems + 1, 0));
		    }
		    m->nitems++;
		}
	    }
	    break;
	    
	  case MENU_INITIAL_SELECTION:
	  /* case MENU_ACCELERATED_SELECTION: Obsolete */
	    m->initial_selection = (Menu_attribute)attrs[1];
	    break;
	    
	  case MENU_INITIAL_SELECTION_EXPANDED:
	    m->display_one_level = !(int)attrs[1];
	    break;
	    
	  case MENU_INITIAL_SELECTION_SELECTED:
	    m->stand_off = !(int)attrs[1];
	    break;
	    
	  case MENU_JUMP_AFTER_SELECTION:
	    m->jump_after_selection = (int)attrs[1];
	    break;

	  case MENU_JUMP_AFTER_NO_SELECTION:
	    m->jump_after_no_selection = (int)attrs[1];
	    break;

	  case MENU_INSERT:
	    if (++m->nitems < m->max_nitems || extend_item_list(m)) {
		if (!insert_item(m->item_list, m->nitems,
				 (int)attrs[1], (struct menu_item *)attrs[2]))
		    --m->nitems;
	    }
	    break;
	    
	  case MENU_INSERT_ITEM:
	    if (++m->nitems < m->max_nitems || extend_item_list(m)) {
		if (!insert_item(m->item_list, m->nitems,
			    lookup(m->item_list, m->nitems,
				   (struct menu_item *)LINT_CAST(attrs[1])),
			    (struct menu_item *)attrs[2]))
		    --m->nitems;
	    }
	    break;
	    
	  case MENU_ITEM:
	    if (m->nitems < m->max_nitems || extend_item_list(m)) {
		m->item_list[m->nitems] = menu_create_item_avlist(&attrs[1]);
	    }
	    (void)menu_set((caddr_t)m->item_list[m->nitems++], MENU_RELEASE, 0);
	    break;
	    
	  case MENU_LEFT_MARGIN:
	    (void) image_set(&m->default_image, IMAGE_LEFT_MARGIN, attrs[1], 0);
	    break;
	    
	  case MENU_MARGIN:
	    (void) image_set(&m->default_image, IMAGE_MARGIN, attrs[1], 0);
	    break;
	    
	  case MENU_NCOLS:
	    m->ncols = imax(0, (int)attrs[1]);
	    break;

	  case MENU_NROWS:
	    m->nrows = imax(0, (int)attrs[1]);
	    break;

	  case MENU_NOTIFY_PROC:
	    m->notify_proc = (caddr_t (*)())attrs[1];
	    if (!m->notify_proc) m->notify_proc = MENU_DEFAULT_NOTIFY_PROC;
	    break;
	    
	  case MENU_PULLRIGHT_DELTA:
	    m->pullright_delta = (int)attrs[1];
	    if (m->pullright_delta < 0)
		m->pullright_delta = MENU_DEFAULT_PULLRIGHT_DELTA;
	    break;
	    
	  case MENU_REMOVE:
	    if (remove_item(m->item_list, m->nitems, (int)attrs[1]))
		--m->nitems;
	    break;
	    
	  case MENU_REMOVE_ITEM:

	    if (remove_item(m->item_list, m->nitems, 
			    lookup(m->item_list, m->nitems,
				   (struct menu_item *)LINT_CAST(attrs[1]))))
	    --m->nitems;
	    break;
	    
	  case MENU_REPLACE:
	    (void) replace_item(m->item_list, m->nitems, (int)attrs[1],
			 (struct menu_item *)LINT_CAST(attrs[2]));
	    break;
	    
	  case MENU_REPLACE_ITEM:
	    (void) replace_item(m->item_list, m->nitems,
			 lookup(m->item_list, m->nitems,
				(struct menu_item *)LINT_CAST(attrs[1])),
			 (struct menu_item *)LINT_CAST(attrs[2]));
	    break;
	    
	  case MENU_RIGHT_MARGIN:
	    (void)image_set(&m->default_image, IMAGE_RIGHT_MARGIN, attrs[1], 0);
	    break;
	    
	  case MENU_SELECTED:
	    m->selected_position = (int)attrs[1];
	    break;
	    
	  case MENU_SELECTED_ITEM:
	    m->selected_position = lookup(m->item_list,m->nitems,
					(struct menu_item *)LINT_CAST(attrs[1]));
	    break;
	    
	  case MENU_SHADOW:
	    m->shadow_pr = (struct pixrect *)attrs[1];
	    break;
	    
	  case MENU_STAY_UP:
	    m->stay_up = (int)attrs[1];
	    break;
	    
	  case MENU_STRINGS:
	    {   
		char **a = (char **)&attrs[1];
		while (*a) {
		    if (m->nitems < m->max_nitems || extend_item_list(m)) {
			m->item_list[m->nitems] = (struct menu_item *)
			  LINT_CAST(menu_create_item(LINT_CAST(MENU_RELEASE),
				    MENU_STRING_ITEM, *a++, m->nitems + 1, 0));
		    }
		    m->nitems++;
		}
	    }
	    break;
	    
	  case MENU_TITLE_ITEM:
	  case MENU_TITLE_IMAGE:
	    if (m->nitems < m->max_nitems || extend_item_list(m))
		m->item_list[m->nitems++] = (struct menu_item *)LINT_CAST(
		    menu_create_item(LINT_CAST(((MENU_TITLE_ITEM == attrs[0])
				     ? MENU_STRING : MENU_IMAGE)), attrs[1], 
				     MENU_INVERT, TRUE, MENU_FEEDBACK, FALSE,
				     MENU_BOXED, TRUE, MENU_RELEASE,
				     MENU_NOTIFY_PROC, menu_return_no_value,
				     0));
	    break;

	  case MENU_VALID_RESULT:
	    m->valid_result = (int)attrs[1];
	    break;
	    
	  case MENU_WIDTH:
	    m->width = (int)attrs[1];
	    break;

	  case MENU_LINE_AFTER_ITEM:
	    switch ((int)attrs[1])  {
	       case MENU_HORIZONTAL_LINE: 
	           m->h_line = 1;
		   break;
	       case MENU_VERTICAL_LINE:
	           m->v_line = 1;
		   break;
	       default:
	           (void) fprintf(stderr,
		   	"Invalid argument for attribute MENU_LINE_AFTER_ITEM: %d\n",(int)attrs[1]);
	    }
	    break;
		   
	  case MENU_NOP:
	    break;
	
	  default:
	    if (ATTR_PKG_MENU == ATTR_PKG(attrs[0]))
		(void) fprintf(stderr,
			"menu_set:  Menu attribute not allowed.\n%s\n",
			attr_sprint((caddr_t)NULL, (u_int)attrs[0]));
		break;
	    
	}
}


Pkg_private void
menu_item_sets(mi, attrs)
	register struct menu_item *mi;
 	register Menu_attribute *attrs;
{   
    for (; *attrs; attrs = menu_attr_next(attrs))
	switch (attrs[0]) {

	  case MENU_ACTION: /* & case MENU_NOTIFY_PROC: */
	    mi->notify_proc = (caddr_t (*)())attrs[1];
	    break;

	  case MENU_ACTION_ITEM:
	    (void) image_set(mi->image, IMAGE_STRING, attrs[1], 0);
	    mi->notify_proc = (caddr_t (*)())attrs[2];
	    break;

	  case MENU_STRING_ITEM:
	    (void) image_set(mi->image, IMAGE_STRING, attrs[1], 0);
	    mi->value = (caddr_t)attrs[2];
	    break;

	  case MENU_ACTION_IMAGE:
	    if (IMAGE_P(attrs[1])) { /* Fixme:  Image pkg should do this? */
		image_destroy(mi->image); /* Fixme:  If attrs[] == NULL ?? */
		mi->image = (struct image *)attrs[1];
	    } else if (mi->image) {
		(void) image_set(mi->image, IMAGE_PIXRECT, attrs[1], 0);
	    }
	    mi->notify_proc = (caddr_t (*)())attrs[2];
	    break;

	  case MENU_IMAGE_ITEM:
	    (void) image_set(mi->image, IMAGE_PIXRECT, attrs[1], 0);
	    mi->value = (caddr_t)attrs[2];
	    break;

	  case MENU_BOXED:
	    (void) image_set(mi->image, IMAGE_BOXED, attrs[1], 0);
	    break;

	  case MENU_CENTER:
	    (void) image_set(mi->image, IMAGE_CENTER, attrs[1], 0);
	    break;
	    
	  case MENU_CLIENT_DATA:
	    mi->client_data = (caddr_t)attrs[1];
	    break;
	    
	  case HELP_DATA:
	    mi->help_data = (caddr_t)attrs[1];
	    break;
	    
	  case MENU_FEEDBACK:
	    mi->no_feedback = !(int)attrs[1];
	    break;
	    
	  case MENU_FONT:
	    (void) image_set(mi->image, IMAGE_FONT, attrs[1], 0);
	    break;

	  case MENU_GEN_PROC:
	    mi->gen_proc = (struct menu_item *(*)())attrs[1];
	    break;

	  case MENU_GEN_PROC_IMAGE:
	    if (IMAGE_P(attrs[1])) { /* Fixme:  Image pkg should do this? */
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else {
		(void) image_set(mi->image, IMAGE_PIXRECT, attrs[1], 0);
		(void) image_set(mi->image, IMAGE_RIGHT_PIXRECT,
				 &menu_arrow_pr, 0);
	    }
	    mi->gen_proc = (struct menu_item *(*)())attrs[2];
	    break;

	  case MENU_GEN_PROC_ITEM:
	    (void) image_set(mi->image, IMAGE_STRING, attrs[1], 0);
	    (void) image_set(mi->image, IMAGE_RIGHT_PIXRECT, &menu_arrow_pr, 0);
	    mi->gen_proc = (struct menu_item *(*)())attrs[2];
	    break;
	    
	  case MENU_GEN_PULLRIGHT:
	    mi->gen_pullright = (struct menu *(*)())attrs[1];
	    mi->pullright = mi->gen_pullright != NULL;
	    (void) image_set(mi->image, IMAGE_RIGHT_PIXRECT,
		      mi->pullright ? &menu_arrow_pr : NULL, 
		      0);
	    break;

	  case MENU_GEN_PULLRIGHT_IMAGE:
	    if (IMAGE_P(attrs[1])) { /* Fixme:  Image pkg should do this? */
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else {
		(void) image_set(mi->image, IMAGE_PIXRECT, attrs[1], 0);
		(void) image_set(mi->image, IMAGE_RIGHT_PIXRECT,
				 &menu_arrow_pr, 0);
	    }
	    mi->gen_pullright = (struct menu *(*)())attrs[2];
	    mi->pullright = mi->gen_pullright != NULL;
	    mi->value = 0;
	    break;

	  case MENU_GEN_PULLRIGHT_ITEM:
	    (void) image_set(mi->image, IMAGE_STRING, attrs[1], 0);
	    (void) image_set(mi->image, IMAGE_RIGHT_PIXRECT, &menu_arrow_pr, 0);
	    mi->gen_pullright = (struct menu *(*)())attrs[2];
	    mi->pullright = mi->gen_pullright != NULL;
	    mi->value = 0;
	    break;
	    
	  case MENU_IMAGE:
	    if (IMAGE_P(attrs[1])) { /* Fixme:  Image pkg should do this? */
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else {
		(void) image_set(mi->image, IMAGE_PIXRECT, attrs[1], 0);
	    }
	    break;

	  case MENU_INACTIVE:
	    mi->inactive = (int)attrs[1];
	    break;

	  case MENU_INVERT:
	    (void) image_set(mi->image, IMAGE_INVERT, attrs[1], 0);
	    break;
	    
	  case MENU_LEFT_MARGIN:
	    (void) image_set(mi->image, IMAGE_LEFT_MARGIN, attrs[1], 0);
	    break;
	    
	  case MENU_MARGIN:
	    (void) image_set(mi->image, IMAGE_MARGIN, attrs[1], 0);
	    break;
	    
	  case MENU_PULLRIGHT:
	    mi->value = (caddr_t)attrs[1];
	    mi->pullright = mi->value != NULL;
	    (void) image_set(mi->image, IMAGE_RIGHT_PIXRECT,
		      mi->pullright ? &menu_arrow_pr : NULL,
		      0);
	    break;

	  case MENU_PULLRIGHT_IMAGE:
	    if (IMAGE_P(attrs[1])) { /* Fixme:  Image pkg should do this? */
		image_destroy(mi->image);
		mi->image = (struct image *)attrs[1];
	    } else if (mi->image) {
		(void) image_set(mi->image, IMAGE_PIXRECT, attrs[1], 0);
		(void) image_set(mi->image, IMAGE_RIGHT_PIXRECT,
				 &menu_arrow_pr, 0);
	    }
	    mi->value = (caddr_t)attrs[2];
	    mi->pullright = mi->value != NULL;
	    break;

	  case MENU_PULLRIGHT_ITEM:
	    (void) image_set(mi->image, IMAGE_STRING, attrs[1], 0);
	    (void) image_set(mi->image, IMAGE_RIGHT_PIXRECT, &menu_arrow_pr, 0);
	    mi->value = (caddr_t)attrs[2];
	    mi->pullright = mi->value != NULL;
	    break;

	  case MENU_RELEASE:
	    mi->free_item = TRUE;
	    break;
	    
	  case MENU_RELEASE_IMAGE:
	    if (mi->image) {
		/* FIXME: Someday this should separate the 2 cases. */
		mi->image->free_string = TRUE;
		mi->image->free_pr = TRUE;
	    }
	    break;
	    
	  case MENU_RIGHT_MARGIN:
	    (void) image_set(mi->image, IMAGE_RIGHT_MARGIN, attrs[1], 0);
	    break;
	    
	  case MENU_SELECTED:
	    mi->selected = (int)attrs[1];
	    break;
	    
	  case MENU_STRING:
	    (void) image_set(mi->image, IMAGE_STRING, attrs[1], 0);
	    break;
	    
	  case MENU_VALUE:
	    mi->value = (caddr_t)attrs[1];
	    mi->pullright = FALSE;
	    (void) image_set(mi->image, IMAGE_RIGHT_PIXRECT, NULL, 0);
	    break;

	  case MENU_LINE_AFTER_ITEM:
	    switch ((int)attrs[1])  {
	       case MENU_HORIZONTAL_LINE: 
	           mi->h_line = 1;
		   break;
	       case MENU_VERTICAL_LINE:
	           mi->v_line = 1;
		   break;
	       default:
	           (void) fprintf(stderr,
		   	"Invalid argument for attribute MENU_LINE_AFTER_ITEM: %d\n",(int)attrs[1]);
	    }
	    break;
		   	    
	  case MENU_NOP:
	    break;
	
	  default:
	    if (ATTR_PKG_MENU == ATTR_PKG(attrs[0]))
		(void) fprintf(stderr,
			"menu_set(item):  Menu attribute not allowed.\n%s\n",
			attr_sprint((caddr_t)NULL, (u_int)attrs[0]));
		break;

	}
}


Pkg_private void
menu_destroys(m, destroy_proc)
	register struct menu *m;
	void (*destroy_proc)();
{   
    register struct menu_item *mi;
    
    if (!m) return;
    for (; m->nitems-- > 0;) {
	mi = m->item_list[m->nitems];
	if (mi->pullright && !mi->gen_pullright && mi->value)
	    menu_destroys((struct menu *)LINT_CAST(mi->value), destroy_proc);
	menu_item_destroys(mi, destroy_proc);
    }
    free((caddr_t)m->item_list);
    if (destroy_proc) destroy_proc(m, MENU_MENU);
    free((caddr_t)m);
}


Pkg_private void
menu_item_destroys(mi, destroy_proc)
	register struct menu_item *mi;
	void (*destroy_proc)();
{
    if (!mi || !mi->free_item) return;
    image_destroy(mi->image);
    if (destroy_proc) destroy_proc(mi, MENU_ITEM);
    free((caddr_t)mi);
}


Private int
extend_item_list(m)
	register struct menu *m;
{
    extern caddr_t realloc();

    m->max_nitems =  m->max_nitems + MENU_FILLER;
    m->item_list = (struct menu_item **)LINT_CAST(realloc(
      (caddr_t)m->item_list,
      (u_int)(m->max_nitems * sizeof(struct menu_item *))));
    if (!m->item_list) {
	(void) fprintf(stderr,
		       "menu_set: Malloc failed to allocate an item list.\n");
	perror("menu_set");
	m->max_nitems =  m->max_nitems - MENU_FILLER;
	return FALSE;
    }
    return TRUE;
}


Private int
remove_item(il, len, pos)
	register struct menu_item *il[];
{
    register int i;
    if (pos < 1 || pos > len) return FALSE;
    for (i = pos; i < len; i++) il[i - 1] = il[i];
    return TRUE;
}


Private int
replace_item(il, len, pos, mi)
	struct menu_item *il[], *mi;
{
    if (pos < 1 || pos > len) return FALSE;
    il[pos - 1] = mi;
    return TRUE;
}


Private int
insert_item(il, len, pos, mi)
	register struct menu_item *il[];
	struct menu_item *mi;
{
    register int i;
    if (pos < 0 || pos >= len) return FALSE;
    for (i = len - 1; i > pos; --i) il[i] = il[i - 1];
    il[i] = mi;
    return TRUE;
}


Private int
lookup(il, len, mi)
	register struct menu_item *il[];
	struct menu_item *mi;
{
    int i;
    
    for (i = 0; i < len; i++) if (il[i] == mi) return i + 1;
    return -1;
}
