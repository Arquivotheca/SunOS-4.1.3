#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)icon_load.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright 1984-1989 Sun Microsystems Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <suntool/icon.h>
#include <suntool/icon_load.h>

#define NULL_PIXRECT	((struct pixrect *)0)
#define NULL_PIXFONT	((struct pixfont *)0)
extern char *sprintf();
extern char *malloc();

FILE *
icon_open_header(from_file, error_msg, info)
	char				*from_file, *error_msg;
	register icon_header_handle	 info;
/* See comments in icon_load.h */
{
#define INVALID	-1
	register int	 c;
	char		 c_temp;
	register FILE	*result;

	if (from_file == "" ||
	    (result = fopen(from_file, "r")) == NULL) {
	    (void)sprintf(error_msg, "Cannot open file %s.\n", from_file);
	    goto ErrorReturn;
	}
	info->depth = INVALID;
	info->height = INVALID;
	info->format_version = INVALID;
	info->valid_bits_per_item = INVALID;
	info->width = INVALID;
	info->last_param_pos = INVALID;
	/*
	 * Parse the file header
	 */
	do {
	    if ((c = fscanf(result, "%*[^DFHVW*]")) == EOF)
		break;
	    switch (c = getc(result)) {
	      case 'D':	if (info->depth == INVALID) {
			    c = fscanf(result, "epth=%d", &info->depth);
			    if (c == 0) c = 1;
			    else info->last_param_pos = ftell(result);
			}
			break;
	      case 'H':	if (info->height == INVALID) {
			    c = fscanf(result, "eight=%d", &info->height);
			    if (c == 0) c = 1;
			    else info->last_param_pos = ftell(result);
			}
			break;
	      case 'F':	if (info->format_version == INVALID) {
			    c = fscanf(result, "ormat_version=%d",
					&info->format_version);
			    if (c == 0) c = 1;
			    else info->last_param_pos = ftell(result);
			}
			break;
	      case 'V':	if (info->valid_bits_per_item == INVALID) {
			    c = fscanf(result, "alid_bits_per_item=%d",
					&info->valid_bits_per_item);
			    if (c == 0) c = 1;
			    else info->last_param_pos = ftell(result);
			}
			break;
	      case 'W':	if (info->width == INVALID) {
			    c = fscanf(result, "idth=%d", &info->width);
			    if (c == 0) c = 1;
			    else info->last_param_pos = ftell(result);
			}
			break;
	      case '*':	if (info->format_version == 1) {
			    c = fscanf(result, "%c", &c_temp);
			    if (c_temp == '/') c = 0;  /* Force exit */
			    else (void)ungetc(c_temp, result);
			}
			break;
	      default:
			c = EOF; /* force error */
			break;
	    }
	} while (c != 0 && c != EOF);
	if (c == EOF || info->format_version != 1) {
	    (void)sprintf(error_msg, "%s has invalid header format.\n", from_file);
	    goto ErrorReturn;
	}
	if (info->depth == INVALID)
	    info->depth = 1;
	if (info->height == INVALID)
	    info->height = 64;
	if (info->valid_bits_per_item == INVALID)
	    info->valid_bits_per_item = 16;
	if (info->width == INVALID)
	    info->width = 64;
	if (info->depth != 1) {
	    (void)sprintf(error_msg, "Cannot handle Depth of %d.\n", info->depth);
	    goto ErrorReturn;
	}
	if (info->valid_bits_per_item != 16 &&
	    info->valid_bits_per_item != 32) {
	    (void)sprintf(error_msg, "Cannot handle Valid_bits_per_item of %d.\n",
		info->valid_bits_per_item);
	    goto ErrorReturn;
	}
	return(result);

ErrorReturn:
	if (result)
	    (void)fclose(result);
	return(NULL);
#undef INVALID
}

int
icon_read_pr(fd, header, pr)
	FILE *fd;
	register icon_header_handle header;
	struct pixrect *pr;
{
	register struct mpr_data *mprd;
	int linebytes, linepad;
	register int count;
	short *image;

	mprd = (struct mpr_data *)(LINT_CAST(pr->pr_data));

	if ((header->valid_bits_per_item != 16 &&
		header->valid_bits_per_item != 32) ||
		MP_NOTMPR(pr) ||
		mprd->md_flags & MP_REVERSEVIDEO ||
		mprd->md_offset.x != 0 ||
		mprd->md_offset.y != 0)
		return -1;

	/*
	 * Icon data in files follows the static pixrect (16 bit) padding 
	 * rule while memory pixrect may have 32 bit padding.
	 */
	linebytes = mpr_linebytes(header->width, header->depth);
	linepad = mprd->md_linebytes - linebytes;

	count = linebytes * header->height;

	/*
	 * If memory pixrect is padded more than icon data, allocate
	 * a scratch buffer for the data; otherwise read it directly 
	 * into the pixrect image.
	 */
	if (linepad) { 
		image = (short *) malloc((unsigned) count);
		if (image == 0)
			return -1;
	}
	else
		image = mprd->md_image;

	/*
	 * Read the icon image into the pixrect or scratch buffer.
	 */
	{
		register short *p = image;

		while (count > 0) {
			register int c;
			long value;

			c = fscanf(fd, " 0x%lx,", &value);
			if (c == 0 || c == EOF)
				break;

			if (header->valid_bits_per_item > 16) {
#ifdef i386
				bitflip32(&value);
#endif
				* (long *) p = value;
				p += 2;
				count -= 4;
			}
			else {
#ifdef i386
				bitflip16(&value);
#endif
				*p++ = value;
				count -= 2;
			}
		}
	}

	/*
	 * Copy image from scratch buffer to pixrect.
	 */
	if (linepad) {
		register char *in = (char *) image;
		register char *out = (char *) mprd->md_image;
		register int h = header->height;

		while (--h >= 0) {
			bcopy(in, out, linebytes);
			in += linebytes;
			out += linebytes + linepad;
		}

		(void) free((char *) image);
	}
		
	return count > 0 ? -1 : 0;
}

struct pixrect *
icon_load_mpr(from_file, error_msg)
	char		*from_file, *error_msg;
/* See comments in icon_load.h */
{
	register FILE			*fd;
	icon_header_object		 header;
	register struct pixrect		*result;

	fd = icon_open_header(from_file, error_msg, &header);
	if (fd == NULL)
	    return(NULL_PIXRECT);
	/*
	 * Allocate the pixrect and read the actual bits making up the icon.
	 */
	result = mem_create(header.width, header.height, header.depth);
	if (result == NULL_PIXRECT) {
	    (void)sprintf(error_msg, "Cannot create memory pixrect %dx%dx%d.\n",
		header.width, header.height, header.depth);
	}
	else if (icon_read_pr(fd, &header, result) != 0) {
	    (void) pr_destroy(result);
	    result = NULL_PIXRECT;
	    (void) sprintf(error_msg,
		"Error reading icon data.\n");
	}
	(void)fclose(fd);
	return(result);
}

int
icon_init_from_pr(icon, pr)
	register struct icon	*icon;
	register struct pixrect	*pr;
/* See comments in icon_load.h */
{
	icon->ic_mpr = pr;
	/*
	 * Set the icon's size and graphics area to match its pixrect's extent.
	 */ 
	icon->ic_gfxrect.r_top = icon->ic_gfxrect.r_left = 0;
	icon->ic_gfxrect.r_width = icon->ic_width = pr->pr_size.x;
	icon->ic_gfxrect.r_height = icon->ic_height = pr->pr_size.y;
	/*
	 * By default, the icon has no text or associated area and font.
	 */
	icon->ic_textrect = rect_null;
	icon->ic_text = NULL;
	icon->ic_font = NULL_PIXFONT;
	/*
	 * Also, by default, the icon has no background pattern.
	 */
	icon->ic_background = NULL_PIXRECT;
	icon->ic_flags = 0;
}

int
icon_load(icon, from_file, error_msg)
	register struct icon	*icon;
	char			*from_file, *error_msg;
/* See comments in icon_load.h */
{
	register struct pixrect	*pr;

	pr = icon_load_mpr(from_file, error_msg);
	if (pr == NULL_PIXRECT)
	    return(1);
	(void)icon_init_from_pr(icon, pr);
#ifdef i386
	pr_flip(pr);
#endif
	return(0);
}

