#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)attr.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sunwindow/attr_impl.h>
#include <sunwindow/sun.h>
#include <sunwindow/sv_malloc.h>

/* Note that changes to the basic attribute-value traversal
 * should also be made in attr_portable.c for portable implmentations.
 */


/* attr_create creates an avlist from the VARARGS passed
 * on the stack.
 */
/*VARARGS2*/
Attr_avlist
attr_create(listhead, listlen, va_alist)
caddr_t	*listhead; 
int	 listlen; 
va_dcl
{
   Attr_avlist	avlist;
   va_list	valist;

   va_start(valist);
   avlist = attr_make(listhead, listlen, valist);
   va_end(valist);
   return avlist;
}


/* attr_make copies the attribute-value list pointed to by valist to
 * the storage pointed to by listhead, or some new storage if that is
 * null.  If listhead is not null, then the list must be less than or equal
 * to listlen ATTR_SIZE byte chunks; if not 0 is returned.
 * The count of the avlist is returned in *count_ptr, if count_ptr is
 * not NULL.
 */
Attr_avlist
attr_make_count(listhead, listlen, valist, count_ptr) 
Attr_avlist	listhead; 
int	 	listlen; 
va_list		valist;
int		*count_ptr;
{
   u_int 	count;
   
#ifdef NON_PORTABLE
   count = (u_int) (LINT_CAST(attr_count((Attr_avlist) valist)));
#else
   count = (u_int) (LINT_CAST(valist_count(valist)));
#endif
   /* if the user supplied storage space for attributes is not big enough
    * for the attributes in the attribute list, then exit!
    */
   if (listhead)  {
	if (count > listlen)  {
            printf("Number of attributes(%d) in the attr list exceeds\n",count);
	    printf("the maximum number(%d) specified.  Exit!\n",listlen);
	    exit(1);
	}
   }    else   {
      listhead = (Attr_avlist) (LINT_CAST(sv_malloc((ATTR_SIZE * count)+1)));
   }
#ifdef NON_PORTABLE
   (void) attr_copy_avlist(listhead, (Attr_avlist) (LINT_CAST(valist))); 
#else
   (void) attr_copy_valist(listhead, valist); 
#endif

   if (count_ptr)
      *count_ptr = count;
   return(listhead);
}


#define	avlist_get(avlist)	*(avlist)++

/* Copy count elements from list to dest.
 * Advance both list and dest to the next element after
 * the last one copied.
 */
#define	avlist_copy(avlist, dest, count)	\
    { \
        bcopy((char *) avlist, (char *) dest, (int)(count * ATTR_SIZE)); \
        avlist += count; \
        dest += count; \
    }


/* A macro to copy attribute values 
 * count is the number of caddr_t size chunks to copy.
 */
#define	avlist_copy_values(avlist, dest, count) \
    if (count == 1) \
        *dest++ = avlist_get(avlist); \
    else { \
	avlist_copy(avlist, dest, count); \
    }


/* attr_copy_avlist copies the attribute-value list from 
 * avlist to dest.  Recursive lists are collapsed into dest.
 */
 
Attr_avlist
attr_copy_avlist(dest, avlist) 
register Attr_avlist	dest;
register Attr_avlist	avlist;
{
   register Attr_attribute	attr;
   register u_int		cardinality;
   
   while (attr = (Attr_attribute) avlist_get(avlist)) {
       cardinality = ATTR_CARDINALITY(attr);
       switch(ATTR_LIST_TYPE(attr)) {
           case ATTR_NONE:	/* not a list */
               *dest++ = (caddr_t) attr;
               avlist_copy_values(avlist, dest, cardinality);
               break;
               
           case ATTR_NULL:	/* null terminated list */
               *dest++ = (caddr_t) attr;
               switch (ATTR_LIST_PTR_TYPE(attr)) {
                   case ATTR_LIST_IS_INLINE:
                       /* Note that this only checks the first four bytes
                        * for the null termination.
			* Copy each value element until we have copied the
			* null termination.
                        */
		       do {
		           avlist_copy_values(avlist, dest, cardinality);
		       } while (*(dest - 1));
                       break;
                       
                   case ATTR_LIST_IS_PTR:
                       *dest++ = avlist_get(avlist);
                       break;
               }
               break;
               
            case ATTR_COUNTED:		/* counted list */
               *dest++ = (caddr_t) attr;
               switch (ATTR_LIST_PTR_TYPE(attr)) {
                   case ATTR_LIST_IS_INLINE: {
			   register u_int	count;

			   *dest = avlist_get(avlist);	/* copy the count */
			    count = ((u_int) *dest++) * cardinality;
			    avlist_copy_values(avlist, dest, count);
			}
                        break;
                        
                   case ATTR_LIST_IS_PTR:
                       *dest++ = avlist_get(avlist);
                       break;
               }
               break;
               
            case ATTR_RECURSIVE:	/* recursive attribute-value list */
                if (cardinality != 0)	/* don't strip it */
                    *dest++ = (caddr_t) attr;
                    
               switch (ATTR_LIST_PTR_TYPE(attr)) {
                   case ATTR_LIST_IS_INLINE:
                       dest = attr_copy_avlist(dest, avlist);
                       if (cardinality != 0)	/* don't strip it */
                           dest++;	/* move past the null terminator */
                       avlist = attr_skip(attr, avlist);
                       break;
                       
                   case ATTR_LIST_IS_PTR:
                       if (cardinality != 0)	/* don't collapse inline */
                           *dest++ = avlist_get(avlist);
                       else {
			   Attr_avlist new_avlist = (Attr_avlist) 
			   	(LINT_CAST(avlist_get(avlist)));
			   if (new_avlist)
			       /* Copy the list inline -- don't 
				* move past the null termintor.
				* Here both the attribute and null
				* terminator will be stripped away.
				*/
			       dest = attr_copy_avlist(dest, new_avlist);
		       }
	               break;
	       }
	       break;
	 }
    }
    *dest = 0; 
    return(dest);
}


/* attr_count counts the number of slots in the av-list avlist.
 * Recursive lists are counted as being collapsed inline.
 */
int
attr_count(avlist)
Attr_avlist avlist; 
{
   /* count the null termination */
   return(attr_count_avlist(avlist, 0) + 1);
}

static char *attr_names[ATTR_PKG_LAST-ATTR_PKG_FIRST+1] = {
    "Generic",
    "Panel",
    "Scrollbar",
    "Cursor",
    "Menu",
    "Textsw",
    "Tool",
    "Seln_base",
    "Entity",
    "Image",
    "Win",
    "Frame",
    "Canvas",
    "Tty",
    "Icon",
    "NOP",
    "Pixwin",
    "Alert",
    "Help"
};


char *
attr_sprint(s, attr)
char *s;
register Attr_attribute attr;
{
    static char msgbuf[100];
    extern char	*sprintf();
    
    if (!s) s = msgbuf;
    if (((int) ATTR_PKG(attr)) >= ((int) ATTR_PKG_FIRST) && 
	((int) ATTR_PKG(attr)) <= ((int) ATTR_PKG_LAST)) {
	char *name = attr_names[((int) ATTR_PKG(attr)) - ((int)ATTR_PKG_FIRST)];

	if (!name) name = "Syspkg";
	(void) sprintf(s, "Attr pkg= %s, id= %d, cardinality= %d (0x%x)",
		name, ATTR_ORDINAL(attr), ATTR_CARDINALITY(attr), attr);
    } else {
	(void) sprintf(s, "Attr pkg= %s, id= %d, cardinality= %d (0x%x)",
	    ATTR_PKG(attr), ATTR_ORDINAL(attr), ATTR_CARDINALITY(attr), attr);
    }
    return(s);
}

void
attr_check_pkg(last_attr, attr)
    Attr_attribute	last_attr, attr;
{
    if (!ATTR_VALID_PKG_ID(attr)) {
	char	expand_attr[100];

	(void) fprintf(stderr,
		"Malformed or non-terminated attribute-value list.\n");
	(void) fprintf(stderr, "Last valid attribute was %s.\n",
		attr_sprint(expand_attr, last_attr));
	abort();
    }
}


int
attr_count_avlist(avlist, last_attr) 
register Attr_avlist	avlist; 
register Attr_attribute	last_attr;
{
   register Attr_attribute	attr;
   register u_int		count = 0;
   register u_int		num;
   register u_int		cardinality;
   
   while (attr = (Attr_attribute) *avlist++) {
       count++;			/* count the attribute */
       cardinality = ATTR_CARDINALITY(attr);
       attr_check_pkg(last_attr, attr);
       last_attr = attr;
       switch(ATTR_LIST_TYPE(attr)) {
           case ATTR_NONE:	/* not a list */
               count += cardinality;
               avlist += cardinality;
               break;
               
           case ATTR_NULL:	/* null terminated list */
               switch (ATTR_LIST_PTR_TYPE(attr)) {
                   case ATTR_LIST_IS_INLINE:
                       /* Note that this only checks the first four bytes
                        * for the null termination.
                        */
                       while (*avlist++)
                           count++;
                       count++;			/* count the null terminator */
                       break;
               
                   case ATTR_LIST_IS_PTR:
                       count++;
                       avlist++;
                       break;
               }
               break;
               
            case ATTR_COUNTED:		/* counted list */
                switch (ATTR_LIST_PTR_TYPE(attr)) {
                    case ATTR_LIST_IS_INLINE:
                       num = ((u_int)(*avlist)) * cardinality + 1;
                       count += num;
                       avlist += num;
                       break;
                    case ATTR_LIST_IS_PTR:
                        count++;
                        avlist++;
                        break;
                }
                break;      
               
            case ATTR_RECURSIVE:	/* recursive attribute-value list */
                if (cardinality == 0)	/* don't include the attribute */
                    count--;
                    
                switch (ATTR_LIST_PTR_TYPE(attr)) {
                    case ATTR_LIST_IS_INLINE:
                        count += attr_count_avlist(avlist, attr);
                        if (cardinality != 0)	/* count the null terminator */
                            count++;
                        avlist = attr_skip(attr, avlist);
                        break;
                       
                    case ATTR_LIST_IS_PTR:
                        if (cardinality != 0) {	/* don't collapse inline */
                            count++;
                            avlist++;
                        } else if (*avlist)
                            /* Here we count the elements of the
                             * recursive list as being inline.
                             * Don't count the null terminator.
                             */
	                    count += attr_count_avlist((Attr_avlist) (LINT_CAST
			    	(*avlist++)), attr);
	               else
	                   avlist++;
		       break;
                }
            break;
        }    
    }
    return count;
}


/* attr_skip_value returns a pointer to the attribute after 
 * the value pointed to by avlist.  attr should be the 
 * attribute which describes the value at avlist.
 */
Attr_avlist
attr_skip_value(attr, avlist)
register Attr_attribute	 attr;
register Attr_avlist	 avlist;
{ 
    switch (ATTR_LIST_TYPE(attr)) {
        case ATTR_NULL:
            if (ATTR_LIST_PTR_TYPE(attr) == ATTR_LIST_IS_PTR)
                avlist++;
            else
 	        while (*avlist++);
 	    break;
 	        
 	case ATTR_RECURSIVE:
            if (ATTR_LIST_PTR_TYPE(attr) == ATTR_LIST_IS_PTR)
	        avlist++;
            else
                while (attr = (Attr_attribute) *avlist++)
                    avlist = attr_skip_value(attr, avlist);
 	    break;
 	        
 	case ATTR_COUNTED:
            if (ATTR_LIST_PTR_TYPE(attr) == ATTR_LIST_IS_PTR)
                avlist++;
            else
 	        avlist += ((int) *avlist) * ATTR_CARDINALITY(attr) + 1;
 	    break;
 	        
 	case ATTR_NONE:
 	    avlist += ATTR_CARDINALITY(attr);
 	    break;
     }
     return avlist;
}
 	
/* attr_free frees the attribute-value list
 */
void
attr_free(avlist)
Attr_avlist avlist; 
{
   free((char *) avlist);
}
