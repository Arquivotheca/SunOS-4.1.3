#ifndef lint
static  char sccsid[] = "@(#)fv.font.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <ctype.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

static short Doc_image[] = {
#include "document.icon"
};
static short DocI_image[] = {
#include "documentI.icon"
};
static short Fldr_image[] = {
#include "folder.icon"
};
static short FldrE_image[] = {
#include "folderunexpl.icon"
};
static short FldrI_image[] = {
#include "folderI.icon"
};
static short Ofldr_image[] = {
#include "openfolder.icon"
};
static short OfldrI_image[] = {
#include "openfolderI.icon"
};
static short Prgm_image[] = {
#include "application.icon"
};
static short PrgmI_image[] = {
#include "applicationI.icon"
};
static short Sys_image[] = {
#include "system.icon"
};
static short SysI_image[] = {
#include "systemI.icon"
};
static short Unknown_image[] = {
#include "brokenlink.icon"
};
static short Lock_image[] = {
#include "lock.icon"
};

mpr_static_static(Doc, 64, GLYPH_WIDTH, 1, Doc_image);
mpr_static_static(DocI, 64, GLYPH_WIDTH, 1, DocI_image);
mpr_static_static(Fldr, 64, GLYPH_WIDTH, 1, Fldr_image);
mpr_static_static(FldrE, 64, GLYPH_WIDTH, 1, FldrE_image);
mpr_static_static(FldrI, 64, GLYPH_WIDTH, 1, FldrI_image);
mpr_static_static(Ofldr, 64, GLYPH_WIDTH, 1, Ofldr_image);
mpr_static_static(OfldrI, 64, GLYPH_WIDTH, 1, OfldrI_image);
mpr_static_static(Prgm, 64, GLYPH_WIDTH, 1, Prgm_image);
mpr_static_static(PrgmI, 64, GLYPH_WIDTH, 1, PrgmI_image);
mpr_static_static(Sys, 64, GLYPH_WIDTH, 1, Sys_image);
mpr_static_static(SysI, 64, GLYPH_WIDTH, 1, SysI_image);
mpr_static_static(Unknown, 64, GLYPH_WIDTH, 1, Unknown_image);
mpr_static_static(Lock, 64, GLYPH_WIDTH, 1, Lock_image);

static short Doc_list_image[] = {
#include "doc_list.icon"
};
static short Folder_list_image[] = {
#include "folder_list.icon"
};
static short App_list_image[] = {
#include "app_list.icon"
};
static short Sys_list_image[] = {
#include "sys_list.icon"
};
static short Brk_list_image[] = {
#include "broken_list.icon"
};
mpr_static_static(doc_list_icon, 16, 16, 1, Doc_list_image);
mpr_static_static(folder_list_icon, 16, 16, 1, Folder_list_image);
mpr_static_static(app_list_icon, 16, 16, 1, App_list_image);
mpr_static_static(sys_list_icon, 16, 16, 1, Sys_list_image);
mpr_static_static(brk_list_icon, 16, 16, 1, Brk_list_image);

fv_init_font()
{
	register int i;		/* Index */
	char s[2];

	Fv_font = (Pixfont *)window_get(Fv_frame, WIN_FONT);

	s[1] = NULL;
	for ( i = ' '; i <= '~'; i++ )
	{
		*s = i;
		Fv_fontwidth[i-' '] = Fv_font->pf_char[i].pc_adv.x;
	}
	Fv_fontsize.x = Fv_fontwidth['m'-' '];	/* Widest character */
	Fv_fontsize.y = Fv_font->pf_defaultsize.y;
	Fv_maxwidth = 255*Fv_fontsize.x;

	Fv_fixed = Fv_fontwidth['m'-' '] == Fv_fontwidth['i'-' '];

	Fv_icon[FV_IDOCUMENT] = &Doc;
	Fv_icon[FV_IAPPLICATION] = &Prgm;
	Fv_icon[FV_ISYSTEM] = &Sys;
	Fv_icon[FV_IFOLDER] = &Fldr;
	Fv_icon[FV_IOPENFOLDER] = &Ofldr;
	Fv_icon[FV_IUNEXPLFOLDER] = &FldrE;
	Fv_icon[FV_IBROKENLINK] = &Unknown;
	Fv_invert[FV_IDOCUMENT] = &DocI;
	Fv_invert[FV_IAPPLICATION] = &PrgmI;
	Fv_invert[FV_ISYSTEM] = &SysI;
	Fv_invert[FV_IFOLDER] = &FldrI;
	Fv_invert[FV_IOPENFOLDER] = &OfldrI;
	Fv_invert[FV_IUNEXPLFOLDER] = &FldrI;
	Fv_invert[FV_IBROKENLINK] = &DocI;
	Fv_lock = &Lock;

	Fv_list_icon[FV_IFOLDER] = &folder_list_icon;
	Fv_list_icon[FV_IDOCUMENT] = &doc_list_icon;
	Fv_list_icon[FV_IAPPLICATION] = &app_list_icon;
	Fv_list_icon[FV_ISYSTEM] = &sys_list_icon;
	Fv_list_icon[FV_IBROKENLINK] = &brk_list_icon;
}
