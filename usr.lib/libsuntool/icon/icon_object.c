#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)icon_object.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                            icon_object.c                                  */
/*                   Copyright (c) 1985 by Sun Microsystems, Inc.            */
/*****************************************************************************/

#include <stdio.h>
#include <varargs.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/icon.h>

static int icon_init_attrs(), icon_set_attrs();

/*****************************************************************************/
/* icon_create                                                               */
/*****************************************************************************/

/* VARARGS0 */
Icon
icon_create(va_alist)
va_dcl
{
   Attr_avlist	 	 avlist[ATTR_STANDARD_SIZE];
   va_list 		 valist;
   register struct icon  *icon;
   char *calloc();
   extern PIXFONT	 *pw_pfsysopen();

   va_start(valist);
   (void)attr_make((char **)avlist, ATTR_STANDARD_SIZE, valist);
   va_end(valist);

   if (!(icon = (struct icon *) (LINT_CAST(calloc (1, sizeof(struct icon))))))
      return NULL;

   icon_init_attrs(icon);

   (void)icon_set_attrs(icon, avlist);

   if (!icon->ic_font)
   	icon->ic_font = pw_pfsysopen();

   return (Icon) icon;
}

static
icon_init_attrs(icon)
register icon_handle icon;
{
   icon->ic_width            = 64;
   icon->ic_height           = 64;
   icon->ic_gfxrect.r_width  = 64;
   icon->ic_gfxrect.r_height = 64;
}

/*****************************************************************************/
/* icon_destroy                                                              */
/*****************************************************************************/

icon_destroy(icon_client)
Icon icon_client;
{
   icon_handle icon;
   
   icon = (icon_handle)(LINT_CAST(icon_client));
   free((char *)(LINT_CAST(icon)));
}

/*****************************************************************************/
/* icon_set                                                                  */
/*****************************************************************************/

/*VARARGS1*/
int
icon_set(icon_client, va_alist)
Icon icon_client;
va_dcl
{
   Attr_avlist	 avlist[ATTR_STANDARD_SIZE];
   va_list	 valist;
   icon_handle	 icon;

   icon = (icon_handle)(LINT_CAST(icon_client));
   va_start(valist);
   (void)attr_make((char **)avlist, ATTR_STANDARD_SIZE, valist);
   va_end(valist);
   return icon_set_attrs(icon, avlist);
}

/*VARARGS1*/
static int
icon_set_attrs(icon, avlist)
register icon_handle icon;
register Attr_avlist avlist;
{
   register Icon_attribute 	attr;
   Rect				*r;

   while (attr = (Icon_attribute) *avlist++) {
      switch (attr) {

	 case ICON_X:
	    avlist++;
	    break;

	 case ICON_Y:
	    avlist++;
	    break;

	 case ICON_WIDTH:
	    icon->ic_width = (short) *avlist++;
	    break;

	 case ICON_HEIGHT:
	    icon->ic_height = (short) (LINT_CAST(*avlist++));
	    break;

         case ICON_IMAGE:
	    icon->ic_mpr = (struct pixrect *) (LINT_CAST(*avlist++));
	    break;

	 case ICON_IMAGE_RECT:
	    r = (Rect *) (LINT_CAST(*avlist++));
	    if (r)
		icon->ic_gfxrect = *r;
	    break;

	 case ICON_LABEL_RECT:
	    r = (Rect *) (LINT_CAST(*avlist++));
	    if (r)
		icon->ic_textrect = *r;
	    break;

	 case ICON_LABEL:
	    icon->ic_text = (char *) *avlist++;
	    break;

	 case ICON_FONT:
	    icon->ic_font = (struct pixfont *) (LINT_CAST(*avlist++));
	    break;

	 default:
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }
   return 1;
}

/*****************************************************************************/
/* icon_get                                                                  */
/*****************************************************************************/

caddr_t
icon_get(icon_client, attr)
register Icon	icon_client;
Icon_attribute		attr;
{
   icon_handle	icon;
   
   icon = (icon_handle)(LINT_CAST(icon_client));
   switch (attr) {

      case ICON_WIDTH:
	 return (caddr_t) icon->ic_width;

      case ICON_HEIGHT:
	 return (caddr_t) icon->ic_height;

      case ICON_IMAGE:
	 return (caddr_t) icon->ic_mpr;

      case ICON_IMAGE_RECT:
	 return (caddr_t) &(icon->ic_gfxrect);

      case ICON_LABEL_RECT:
	 return (caddr_t) &(icon->ic_textrect);

      case ICON_LABEL:
	 return (caddr_t) icon->ic_text;

      case ICON_FONT:
	 return (caddr_t) icon->ic_font;

      case ICON_X:
      case ICON_Y:
      default:
	 return (caddr_t) NULL;
   }
}
