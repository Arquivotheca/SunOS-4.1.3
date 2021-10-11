/*      @(#)map.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

extern struct loc map[];
extern int mapentries;

save(y,x,str,field)
	int y, x;
        char *str, *field;
{
        /*
         * Save coordinates
         */
        map[mapentries].x = x;
        map[mapentries].y = y;
        (void) strcpy(map[mapentries].str,str);
        (void) strcpy(map[mapentries].field,field);
	mapentries++;
}

int
search(str)
	char *str;
{
	register i;
	
	for (i=0; i <= mapentries; i++) {
        	if ( !strcmp(map[i].field,str) )
        		break;
       	}
	return (i);
}
