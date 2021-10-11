#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)attr_cu.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <pixrect/pixrect_hs.h>
#include <sunwindow/attr.h>
#include <sunwindow/sun.h>

/* attr_decode_cu decodes the encoded character row & pixel 
 * value in encoded_value.
 */
static int
attr_decode_cu(encoded_value, char_part, pixel_part)
register u_int	 	encoded_value;
int			*char_part;
int			*pixel_part;
{
   short	char_bits, pixel_bits;

   char_bits = (short) ( (encoded_value >> 16) & 0x1FFF );
   /* sign extend if negative */
   if (char_bits & 0x1000)
      char_bits |= 0xE000;
   pixel_bits = ( (short) encoded_value) - ATTR_PIXEL_OFFSET;
   *char_part = (int) char_bits;
   *pixel_part = (int) pixel_bits;
}

/* attr_cu_to_y converts the encoded character row & pixel 
 * value in encoded_value to a pixel value.
 */
#undef	attr_cu_to_y
int
attr_cu_to_y(encoded_value, font, top_margin, row_gap)
u_int		encoded_value;
struct pixfont	*font;
int		top_margin;
int		row_gap;
{
   return attr_rc_unit_to_y(encoded_value, font->pf_defaultsize.y, top_margin, row_gap);
}

int
attr_rc_unit_to_y(encoded_value, row_height, top_margin, row_gap)
u_int		encoded_value;
int		row_height;
int		top_margin;
int		row_gap;
{
   int	char_part, pixel_part;
   int	length;
   
   if (!attr_is_cu(encoded_value))
       return (int) encoded_value;

   attr_decode_cu(encoded_value, &char_part, &pixel_part);
   length = pixel_part + char_part * (row_height + row_gap);
   
   switch (ATTR_CU_TYPE(encoded_value)) {
       case ATTR_CU_POSITION:
          return (top_margin + length);
          
       case ATTR_CU_LENGTH:
       default:
           return length;
   }
}

/* attr_cu_to_x converts the encoded character column & pixel 
 * value in encoded_value to a pixel value.
 */
#undef	attr_cu_to_x
int
attr_cu_to_x(encoded_value, font, left_margin)
u_int		encoded_value;
struct pixfont	*font;
int		left_margin;
{
   return attr_rc_unit_to_x(encoded_value, font->pf_defaultsize.x, 
			    left_margin, 0);
}


int
attr_rc_unit_to_x(encoded_value, col_width, left_margin, col_gap)
u_int		encoded_value;
int		col_width;
int		left_margin, col_gap;
{
   int	char_part, pixel_part;
   int	length;
   
   if (!attr_is_cu(encoded_value))
       return (int) encoded_value;
       
   attr_decode_cu(encoded_value, &char_part, &pixel_part);
   length = pixel_part + char_part * (col_width + col_gap);

   switch (ATTR_CU_TYPE(encoded_value)) {
       case ATTR_CU_POSITION:
          return (left_margin + length);
          
       case ATTR_CU_LENGTH:
       default:
           return length;
   }
}


/* attr_replace_cu_value converts the value for attr (pointed to by avlist)
 * from character units to pixel units.
 */
static Attr_avlist
attr_rc_unit_to_pixel(base_type, avlist, col_width, row_height, 
		      left_margin, top_margin, col_gap, row_gap)
Attr_base_type	base_type;
register Attr_avlist	avlist;
int			col_width, row_height;
int			left_margin, top_margin;
int			col_gap, row_gap;
{
    switch (base_type) {
        case ATTR_BASE_INDEX_X:	/* index, x-coordinate */
            avlist++;		/* skip the index */
            			/* and fall through ... */
        case ATTR_BASE_X:	/* single x-coordinate */
            *avlist = (caddr_t) 
		attr_rc_unit_to_x((u_int)*avlist, col_width, left_margin, col_gap);
            avlist++;
            break;
            
        case ATTR_BASE_INDEX_Y:	/* index, y-coordinate */
            avlist++;		/* skip the index */
                		/* and fall through ... */
        case ATTR_BASE_Y:	/* single y-coordinate */
            *avlist = (caddr_t) 
                attr_rc_unit_to_y((u_int)*avlist, row_height, top_margin, row_gap);
            avlist++;
            break;

        case ATTR_BASE_INDEX_XY:/* index, x-coordinate, y-coordinate */
            avlist++;		/* skip the index */
                		/* and fall through ... */
        case ATTR_BASE_XY:
            *avlist = (caddr_t) 
		attr_rc_unit_to_x((u_int)*avlist, col_width, left_margin, col_gap);
            avlist++;
            *avlist = (caddr_t) 
                attr_rc_unit_to_y((u_int)*avlist, row_height, top_margin, row_gap);
            avlist++;
            break;
        
        default:		/* some other base type */
            /* we should complain here */
            break;
    }
    return avlist;
}
        

/* attr_replace_cu replaces any character unit values in avlist with
 * the corresponding pixel unit.
 */
#undef	attr_replace_cu
void
attr_replace_cu(avlist, font, left_margin, top_margin, row_gap)
register Attr_avlist	avlist;
struct pixfont		*font;
int			 left_margin;
int			 top_margin;
int			 row_gap;
{
    attr_rc_units_to_pixels(avlist, font->pf_defaultsize.x,  
	font->pf_defaultsize.y, left_margin, top_margin, 0, row_gap);
}

void
attr_rc_units_to_pixels(avlist, col_width, row_height, left_margin, top_margin,
			col_gap, row_gap)
register Attr_avlist	 avlist;
int			 col_width, row_height;
int			 left_margin, top_margin;
int			 col_gap, row_gap;
{
    register Attr_attribute	attr;
    register Attr_base_type	base_type;
    register caddr_t		*value_list;
    register short		is_ptr;
    register int		count;
   
/*
 * The goal is to convert all CU arguments (e.g., ATTR_X, ATTR_Y).  ATTR_BASE_AV
 * needs to be recursively traversed to check for embedded CU types.  As such,
 * unless an attribute's base type a) can take a CU value or b) can take an embedded
 * list that might contain a CU value its value can be ignored (skipped).
 */

    while (attr = (Attr_attribute) *avlist++) {
        switch (base_type = ATTR_BASE_TYPE(attr)) {
            default:
                avlist = attr_skip(attr, avlist);
                break;
               
            case ATTR_BASE_AV:
                attr_rc_units_to_pixels(avlist, col_width, row_height,
                                      left_margin, top_margin, col_gap, row_gap);
                avlist = attr_skip(attr, avlist);
                break;
                        
            case ATTR_BASE_X:
            case ATTR_BASE_INDEX_X:
            case ATTR_BASE_Y:
            case ATTR_BASE_INDEX_Y:
            case ATTR_BASE_XY:
            case ATTR_BASE_INDEX_XY:
                is_ptr = ATTR_LIST_PTR_TYPE(attr) == ATTR_LIST_IS_PTR;
                switch (ATTR_LIST_TYPE(attr)) {
                    case ATTR_NONE:
                        avlist = attr_rc_unit_to_pixel(base_type, avlist,
                            col_width, row_height, left_margin, top_margin, 
			    col_gap, row_gap);
                        break;
                        
                    case ATTR_NULL:
                        if (is_ptr)
                            value_list = (caddr_t *) (LINT_CAST(*avlist));
                        else
                            value_list = avlist;
                        
                        while (*value_list)
                            value_list = attr_rc_unit_to_pixel(base_type, 
                                value_list, col_width, row_height, 
				left_margin, top_margin, col_gap, row_gap);
                                
                        if (is_ptr)
                            avlist++;
                        else
                            avlist = value_list + 1;
                        break;

                    case ATTR_COUNTED:
                        if (is_ptr)
                            value_list = (caddr_t *) (LINT_CAST(*avlist));
                        else
                            value_list = avlist;
                        
                        for (count = (int) *value_list++; count; count--)
                            value_list = attr_rc_unit_to_pixel(base_type, 
                                value_list, col_width, row_height, 
				left_margin, top_margin, col_gap, row_gap);
                                
                        if (is_ptr)
                            avlist++;
                        else
                            avlist = value_list;
                        break;
                        
                    case ATTR_RECURSIVE:
                        if (is_ptr)
                            attr_rc_units_to_pixels((Attr_avlist)
			        (LINT_CAST(*avlist++)), 
				col_width, row_height, left_margin, 
				top_margin, col_gap, row_gap);
                        else {
                            attr_rc_units_to_pixels(avlist, 
				col_width, row_height, left_margin, 
				top_margin, col_gap, row_gap);
                            avlist = attr_skip(attr, avlist);
                        }
                        break;
                }
                break;
        }
    }
}
