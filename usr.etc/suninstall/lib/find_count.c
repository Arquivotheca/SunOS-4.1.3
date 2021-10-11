/*      @(#)find_count.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

int
find_count(str)
        char *str;
{
        char *ptr;
        register i;

        for(ptr=str,i=0;*ptr != '\0' && *ptr != '\n' && *ptr != ' ';*ptr++) {
                if ( *ptr == '/' ) i++;
        }
        return(i);
}
