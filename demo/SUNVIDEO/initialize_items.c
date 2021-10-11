
#ifndef lint
static char sccsid[] = "@(#)initialize_items.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * This file contains a package used to save and load the state of SunView
 * panel items. Panel items need to be registered with the register_item
 * function. It requires a string and a type. I would have used the label
 * string instead of the name, but sometimes a label is not wanted. The
 * value strings are taken from the CHOICE_STRINGS of a choice item.
 *
 * This package will call the each item's notify procedure when a file is
 * loaded. It passes a null event, so any items that do event processing
 * should check for this case.
 *
 */

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <strings.h>
#include "init_item.h"

#include <stdio.h>
#include <errno.h>

#define MAX_ITEMS 256
typedef void (*PFV)();	/* pointer to funtion returning void */

Panel_item item_list[MAX_ITEMS];
int	   item_count;

struct init_item *
create_init_item(type, name)
    caddr_t type;
    char *name;
{
    struct init_item *new_init_item;

    new_init_item = (struct init_item *)malloc(sizeof(struct init_item));
    new_init_item->init_type = type;
    new_init_item->init_name = name;
    return(new_init_item);
}

register_item(item, name, type)
    Panel_item	item;
    char	*name;
    caddr_t	type;
{
    if (item_count >= MAX_ITEMS) {
	printf(stderr, "register_item: out of space on item list.\n");
	return(-1);
    }
    panel_set(item, PANEL_CLIENT_DATA, create_init_item(type, name), 0);
    item_list[item_count++] = item;
}


/* search the item list for an item matching the string string and
   initiailize it */
initialize_item(string)
    char *string;
{
    int i, value;
    caddr_t item_type;
    struct init_item *init_item;
    void (*notify_proc)();
    char name[128];
    char *value_string;

    strncpy(name, string, 127);
    if (!(value_string = index(name, '='))) {
	return;
    }
    *value_string = '\0';
    value_string++;  /* to point at value */

    for (i = 0; i < item_count; i++) {
	init_item = (struct init_item *)
	               panel_get(item_list[i], PANEL_CLIENT_DATA);
	if (strcmp(name, init_item->init_name) == 0) {
	    char *s;

	    notify_proc = (PFV)panel_get(item_list[i],
			PANEL_NOTIFY_PROC);
	    if (init_item->init_type == (caddr_t)PANEL_CHOICE) {
		value = 0;
		while(s = (char *)panel_get(item_list[i], PANEL_CHOICE_STRING,
		value)) {
		    if (strcmp(value_string, s) == 0) {
			panel_set(item_list[i], PANEL_VALUE, value, 0);
			(*notify_proc)(item_list[i], value, EVENT_NULL);
			return;
		    }
		    value++;
		}
		printf("Choice has no strings\n");
		return;
	    } else if (init_item->init_type == (caddr_t)PANEL_SLIDER) {
		value = atoi(value_string);
		if (value < (int)panel_get(item_list[i], PANEL_MIN_VALUE)){
		    value = (int)panel_get(item_list[i], PANEL_MIN_VALUE);
		}
		if (value > (int)panel_get(item_list[i], PANEL_MAX_VALUE)){
		    value = (int)panel_get(item_list[i], PANEL_MAX_VALUE);
		}
		panel_set(item_list[i], PANEL_VALUE, value, 0);
			(*notify_proc)(item_list[i], value, EVENT_NULL);
		return;
		
	    }
	}

    }
    printf("item  not found (%s)\n", name);
}

initialize_items_from_list(list)
    char *list[];
{
    int i = 0;
    while (list[i]) {
	initialize_item(list[i++]);
    }
}
initialize_items_from_file(filename)
    char *filename;
{
    FILE *fp;
    char linebuf[MAXLINE+1];
    static char *fgetline(); /* Forward ref */

    if ((fp = fopen(filename, "r")) == NULL) {
	return(errno);
    }
    while (fgetline(linebuf, MAXLINE, fp) != NULL) {
	initialize_item(linebuf);
    }
    fclose(fp);
}

/* fgetline: much like fgets, except newline is not copied to string */
static char *
fgetline(s, n, stream)
    char *s;
    int n;
    FILE *stream;
{
    char c;
    char *cs;
    
    cs = s;
    while (--n > 0 && ((c = getc(stream)) != EOF)) {
	if (c == '\n') {
	    break;
	} else {
	    *cs++ = c;
	}
    }
    *cs = '\0';
    return((c == EOF) ? (char *)NULL : s);
   
}

write_items_to_file(filename)
    char *filename;
{
    FILE *fp;
    int i, current_value;
    struct init_item *init_item;

    if ((fp = fopen(filename, "w")) == NULL) {
	return(errno);
    }

    for (i = 0; i < item_count; i++) {
	init_item = (struct init_item *)
	             panel_get(item_list[i], PANEL_CLIENT_DATA);
	current_value = (int)panel_get(item_list[i], PANEL_VALUE);
	if (init_item->init_type == (caddr_t)PANEL_CHOICE) {
	    fprintf(fp, "%s=%s\n", init_item->init_name,
			panel_get(item_list[i], PANEL_CHOICE_STRING,
			          current_value));
	} else if (init_item->init_type == (caddr_t)PANEL_SLIDER) {
	    fprintf(fp, "%s=%d\n", init_item->init_name, current_value);
	} else {
	    printf("Unsupported panel item type.\n");
	}
    }
    fclose(fp);
}
