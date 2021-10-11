#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)write_font.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <signal.h>
#include "other_hs.h"
#include "fontedit.h"
#include "externs.h"
#include <vfont.h>
#include <sys/file.h>

#ifdef i386
#define byteswap(x)	( (((x)>>8)&255) | (((x)&255)<<8) )
#endif


FILE	*fopen();

void
fted_save_current_font()
{
    register	FILE	*fp;		    /* file pointer      */
    
    (void) strcpy(fted_font_name, (char *)panel_get_value(fted_font_name_item));

    if (fted_not_vfont_format(fted_font_name)) {
	fted_message_user("File exists but is not in vfont format. Font not written.");
	return;
    }
    
    fp = fopen(fted_font_name,"w");
    if (fp == (FILE *) NULL) {
	fted_message_user("Open for the font file failed.");
	return;
    }
    if ((fted_char_max_width == 0) || (fted_char_max_height == 0)) {
	fted_message_user("Can't write font with a dimension of 0");
	fclose(fp);
	return;
    }
    fted_cur_font->pf_defaultsize.x = fted_char_max_width;
    fted_cur_font->pf_defaultsize.y = fted_char_max_height;
    
    if (fted_write_vfont(fp, fted_cur_font)) {
	fted_message_user("The font has been saved.");
    	fted_font_modified = FALSE;
    }
    else
        fted_message_user("The font was not written.");
	
    fclose(fp);
}

/* 
 * If the file exists and is not in vfont format, return an error.
 */

int
fted_not_vfont_format(file_name)
char	*file_name;
{   
    struct header	vfont_header;
    int			fd;
    
    
    fd = open(file_name, O_RDONLY, (00600 | 00060 | 00004));
    if (fd < 0 )
	return(FALSE);		/* Can't read file but may be able to
				 * write it.
			 	 * Or,  the file doesn't exist.
				 */

    (void) read(fd, &vfont_header, sizeof(struct header));
    close(fd);
#ifndef i386
    if (vfont_header.magic == VFONT_MAGIC) {
	return(FALSE);		/* file is in vfont format */
    }
    else {
	return(TRUE);		/* file is NOT in vfont format */
    }
#else
    if (vfont_header.magic == byteswap(VFONT_MAGIC)) {
	/* swap the header so you can read it. */
	vfont_header.magic == byteswap(vfont_header.magic);
	vfont_header.size = byteswap(vfont_header.size);
	vfont_header.maxx = byteswap(vfont_header.maxx);
	vfont_header.maxy = byteswap(vfont_header.maxy);
	vfont_header.xtend = byteswap(vfont_header.xtend);
	return(FALSE);		/* file is in vfont format */
    }
    else {
	return(TRUE);		/* file is NOT in vfont format */
    }
#endif
	
}



fted_write_vfont(fp, font)
FILE 	*fp;		/* the file pointer				    */
PIXFONT	*font;		/* the pixfont to be written in vfont format        */
{
    		struct header	vfont_header;	     /* header info	     */
    		struct dispatch	disp[NUM_DISPATCH];  /* vfont info records   */
    register	struct dispatch *d;		     /* pointer to one       */
    register 	struct pixchar	*pc;		     /* pointer to a pix char*/
    register 	unsigned char	*data;		     /* data to be written   */
    register    int i, j,k;
    register	int offset;
    register	int bytes_per_row, bytes_per_pixchar_row; /* number of bytes
    							     in a character */
    		int num_bytes;


    offset = 0;
    for (i = 0, pc = &(font->pf_char[0]), d = &(disp[0]);
    					i < NUM_DISPATCH; i++, pc++, d++) {
	if (pc->pc_pr != 0){
	    d->addr   = (unsigned short) offset;
	    num_bytes = ((pc->pc_pr->pr_size.x + 7) >> 3 ) * pc->pc_pr->pr_size.y;
	    d->nbytes = (short) num_bytes;
	    offset    += num_bytes;

	    d->left  = (char) -pc->pc_home.x;
    	    d->up    = (char) -pc->pc_home.y;
	    d->right = (char)  pc->pc_pr->pr_size.x + pc->pc_home.x;
	    d->down  = (char)  pc->pc_pr->pr_size.y + pc->pc_home.y;
	    d->width = (short) pc->pc_adv.x;
	}
	else {
	    d->addr = d->nbytes = d->left = d->right = d->up = d->down = d->width = 0;
	}
    }

    vfont_header.magic = (short) VFONT_MAGIC;
    vfont_header.size  = (unsigned short) offset;
    vfont_header.maxx  = (short) font->pf_defaultsize.x;
    vfont_header.maxy  = (short) font->pf_defaultsize.y;
    vfont_header.xtend = (short) 0;
 
#ifdef i386
    /* swap the header so you can read it. */
    vfont_header.magic = byteswap(vfont_header.magic);
    vfont_header.size = byteswap(vfont_header.size);
    vfont_header.maxx = byteswap(vfont_header.maxx);
    vfont_header.maxy = byteswap(vfont_header.maxy);
    vfont_header.xtend = byteswap(vfont_header.xtend);
#endif

    
    if (fwrite((char *) &(vfont_header), sizeof(struct header), 1, fp) != 1) {
	fprintf(stderr,"Write of header failed.\n");
	return(FALSE);
    }

#ifdef i386
    /* Convert the byte-swapped fields in the dispatch table */
    for (i = 0; i < NUM_DISPATCH; i++) {
	disp[i].addr   = byteswap(disp[i].addr);
	disp[i].nbytes = byteswap(disp[i].nbytes);
	disp[i].width  = byteswap(disp[i].width);
    }
#endif

    if (fwrite((char *) disp, sizeof(disp), 1, fp) != 1) {
	fprintf(stderr, "Dispatch write failed.\n");
	return(FALSE);
    }

#ifdef i386 
    /* Convert the byte-swapped fields in the dispatch table */
    for (i = 0; i < NUM_DISPATCH; i++) {
	disp[i].addr   = byteswap(disp[i].addr);
	disp[i].nbytes = byteswap(disp[i].nbytes);
	disp[i].width  = byteswap(disp[i].width);
    }       
#endif

    for (i = 0, pc = &(font->pf_char[0]); i < NUM_DISPATCH; i++, pc++) {
	if (pc->pc_pr != 0){
	    data = (unsigned char *)mpr_d(pc->pc_pr)->md_image;
	    bytes_per_row = ((pc->pc_pr->pr_size.x + 7) >> 3 );
	    bytes_per_pixchar_row = mpr_d(pc->pc_pr)->md_linebytes;
	    for (j = 0; j < pc->pc_pr->pr_size.y; j++) {
#ifdef i386
		for(k=0;k < bytes_per_row;k++)bitflip8(&(data[k]));
#endif
		if (fwrite(data, bytes_per_row, 1, fp) != 1)
		    fprintf(stderr, "Write for char %d, row %d failed.\n", i,j);
#ifdef i386
		/* flip the fonts back after they are written */
		for(k=0;k<bytes_per_row;k++)bitflip8(&(data[k]));
#endif
		data += bytes_per_pixchar_row;
	    }
	}
    }
    fflush(fp);
    return(TRUE);
}
