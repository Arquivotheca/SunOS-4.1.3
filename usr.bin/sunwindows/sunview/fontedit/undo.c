#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)undo.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "other_hs.h"
#include "fontedit.h"
#include "externs.h"
#include "button.h"
#include "slider.h"
#include "edit.h"

struct pixchar	*fted_copy_pix_char();
       char	*malloc();

static int	do_undo = TRUE;

fted_undo_push(pad)
register edit_info	*pad;
{
    register undo_node	*new_node;

    if (fted_undoing)
	return(TRUE);

    new_node = (undo_node *) malloc(sizeof(undo_node));
    if (new_node == NULL)
    	return(FALSE);

    new_node->pix_char_ptr = fted_copy_pix_char(&(pad->window), pad->pix_char_ptr, TRUE);
    new_node->window = pad->window;
    new_node->next = pad->undo_list;
    pad->undo_list = new_node;
    return(TRUE);
}

struct pixchar *fted_undo_pop(pad)
register edit_info	*pad;
{
    register undo_node	*popped_node;
    register struct pixchar *ret_pix_char;

    popped_node = pad->undo_list;
    if (popped_node != NULL) {
	pad->undo_list = popped_node->next;
	ret_pix_char = popped_node->pix_char_ptr;
	pad->window = popped_node->window;
	free(popped_node);
	return(ret_pix_char);
    }

    return((struct pixchar *)NULL);
}



fted_undo_free_list(pad)
register edit_info	*pad;
{
    register undo_node		*popped_node, *node_to_free;
    register struct pixchar 	*pix_char_ptr;

    popped_node = pad->undo_list;
    while ( popped_node != NULL) {
	pix_char_ptr = popped_node->pix_char_ptr;
	mem_destroy(pix_char_ptr->pc_pr);
    	free(pix_char_ptr);
	node_to_free = popped_node;
	popped_node = popped_node->next;
	free(node_to_free);
    }
}



fted_print_undo_list()
{
    register int 	i,j;
    register undo_node	*node;

    for (i = 0; i < NUM_EDIT_PADS; i++) {
	node = fted_edit_pad_info[i].undo_list;
	printf("\nFor pad %d\n", i);
	while ( node != NULL) {
	    printf("  window (%2d, %2d), adv (%2d)\n",
	    		node->window.r_width, node->window.r_height,
			node->pix_char_ptr->pc_adv.x);
	    node = node->next;
	}
    }
}
