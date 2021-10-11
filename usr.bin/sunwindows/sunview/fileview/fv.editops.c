#ifndef lint
static  char sccsid[] = "@(#)fv.editops.c 1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

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
extern time_t time();

fv_move_copy(dest_folder, copy)
	char *dest_folder;
	BOOLEAN copy;
{
	register char *b_p, *n_p;	/* Buffer pointer */
	char buf[MAXPATH];		/* Buffer */
	register FV_FILE **f_p, **l_p;	/* Files array pointers */


	/* Collect all selected files and call mv(1) or cp(1) */

	if (copy)
	{
		(void)strcpy(buf, "/bin/cp -r ");
		b_p = buf+11;
	}
	else
	{
		(void)strcpy(buf, "/bin/mv ");
		b_p = buf+8;
	}

	if (Fv_tselected)
	{
		fv_getpath(Fv_tselected, b_p);
		while (*b_p)
			b_p++;
		*b_p++ = ' ';
	}
	else
	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++)
		if ((*f_p)->selected)
		{
			/* Escape spaces to the shell */

			n_p = (*f_p)->name;
			while (*n_p)
			{
				if (*n_p==' ')
					*b_p++ = '\\';
				*b_p++ = *n_p++;
			}
			*b_p++ = ' ';
			if (b_p - buf >= MAXPATH)
				break;
		}
	*b_p = NULL;
	(void)strcat(b_p-1, dest_folder);
	(void)strcat(b_p-1, " 2>/tmp/.err");

	if (system(buf)) {
		fv_showerr();
		return(FAILURE);
	}

	if (!copy) {
		(void)strcpy(buf, dest_folder);
		dest_folder = buf;
		if (!Fv_tselected)
			fv_display_folder(FV_BUILD_FOLDER);
	}
	fv_putmsg(FALSE, Fv_message[MSUCCEED],
		(int)(copy ? Fv_message[MCOPY] : Fv_message[MMOVE]),
		(int)dest_folder);

	return(SUCCESS);
}


fv_duplicate()			/* Create '*.copy' in current directory */
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	char buf[MAXPATH];		/* Buffer */
	register int sfd, dfd;		/* Src & dest fds */
	register int size;
	struct stat fstatus;
	int copy_time;			/* Time copied */
	register int ncopies = 0;	/* Number of successful copies */


	copy_time = time((time_t *)0);
	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++)
		if ((*f_p)->selected && !my_access(f_p))
			if ((*f_p)->type == FV_IDOCUMENT ||
				 (*f_p)->type == FV_IAPPLICATION )
			{
				if ((sfd=open((*f_p)->name, O_RDONLY)) == -1)
				{
					fv_putmsg(TRUE, Fv_message[MEREAD],
						(int)(*f_p)->name, (int)sys_errlist[errno]);
					continue;
				}
				(void)sprintf(buf, "%s.%s", (*f_p)->name,
					Fv_message[MCOPY]);

				/* Preserve permissions */
				(void)fstat(sfd, &fstatus);

				if ((dfd=open(buf, O_CREAT | O_WRONLY, 
					fstatus.st_mode)) == -1)
				{
					fv_putmsg(TRUE, Fv_message[MECREAT],
						(int)buf, (int)sys_errlist[errno]);
					(void)close(sfd);
					return;
				}

				errno = 0;
				while ((size = read(sfd, buf, BUFSIZ)) && size != -1)
					if (write(dfd, buf, size)==-1)
						break;
				

				(void)close(sfd);
				(void)close(dfd);

				/* Complain if we stopped reading or 
				 * writing because of an error... */
				if (errno)
					fv_putmsg(TRUE, Fv_message[MECREAT],
						(int)buf, (int)sys_errlist[errno]);
				else
					ncopies++;
			}
			else
			{
				/* XXX Embedded spaces */
				(void)sprintf(buf, "/bin/cp -r %s %s.%s 2>/tmp/.err",
					(*f_p)->name, (*f_p)->name, Fv_message[MCOPY]);
				if (system(buf))
					fv_showerr();
				else
					ncopies++;
			}

	if (ncopies)
	{
		fv_display_folder(FV_BUILD_FOLDER);

		/* Select all newly created files */

		for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles, ncopies=0; 
			f_p < l_p; f_p++, ncopies++)
			if ((*f_p)->mtime >= copy_time)
			{
				(*f_p)->selected = TRUE;
				fv_reverse(ncopies);
			}
	}
}


fv_paste_seln(files)		/* Move/Copy from clipboard */
	char *files;
{
	char buffer[4096];
	char movee[256];
	register char *b_p, *s_p, *m_p, *l_p;
	register BOOLEAN null;
	BOOLEAN duplicate;

	fv_busy_cursor(TRUE);

	/* A dot dot in the filename means move it */
	/* Files on clipboard are delimited by spaces; 
	 * XXX Embedded spaces? */

	b_p = files;
	while (*b_p)
		if (*b_p == '.' && *(b_p+1) == '.')
			break;
		else
			b_p++;

	fv_putmsg(FALSE, "%sing file(s) from clipboard", *b_p ? "Mov" : "Copy",
		0);
	if (*b_p)
	{
		/* Move file(s) from clipboard to current directory */

		s_p = b_p = files;
		while (*s_p)
		{
			/* Get file */

			while (*b_p && *b_p != ' ')
				b_p++;
			null = *b_p == NULL;
			*b_p = NULL;

			/* Get destination (no path) */

			m_p = b_p-1;
			while (*m_p != '/')
				m_p--;
			m_p += 3;		/* Skip /.. */
			l_p = movee;

			/* Copy destination into buffer */

			while (*l_p++ = *m_p++)
				;

			/* XXX Ensure in correct folder */

			(void)sprintf(buffer, "/bin/mv %s %s 2>/tmp/.err",
				s_p, movee);
			if (system(buffer))
				fv_showerr();

			/* Position to next file (if any) */
			s_p = null ? b_p : ++b_p;
		}
	}
	else
	{
		/* Copy */

		duplicate = FALSE;

		/* Get last character of file name and determine if
		 * we're really trying to duplicate a file by copying in
		 * into the same folder. If so, append a ".copy".
		 */
		if (strchr(files, ' ') == NULL)
		{
			/* Do this only if one file is copied */
			/* XXX Multiple copies? */
			b_p = files+strlen(files)-1;
			while (*b_p && *b_p != '/')
				b_p--;
			if (*b_p)
				*b_p = NULL;

			if (strcmp(files, Fv_openfolder_path) == 0)
			{
				/* Duplicate */
				*b_p = '/';
				(void)sprintf(buffer, "/bin/cp -r %s %s.%s 2>/tmp/.err",
					files, files, Fv_message[MCOPY]);
				duplicate = TRUE;
			}
			*b_p = '/';
		}

		if (!duplicate)
			(void)sprintf(buffer, "/bin/cp -r %s %s 2>/tmp/.err",
				files, Fv_openfolder_path);

		if (system(buffer))
			fv_showerr();
	}

	fv_display_folder(FV_BUILD_FOLDER);
	fv_busy_cursor(FALSE);
}



char *
fv_getmyselns(cut)			/* Return selected files */
	BOOLEAN cut;			/* Prefix them with .. */
{
	char Sbuf[4096];		/* Selection buffer */
	register char *s_p, *b_p;	/* Buffer pointers */
	register FV_FILE **f_p, **l_p;	/* Files array pointers */


	if (cut && (access(".", W_OK) == -1))
	{
		fv_putmsg(TRUE, Fv_message[MEDELETE], Fv_current->name, 
			sys_errlist[errno]);
		return(NULL);
	}

	if (Fv_tselected)
	{
		/* Tree selection */

		(void)strcpy(Sbuf, Fv_path);
	}
	else
	{
		/* Folder selection */

		s_p = Sbuf;
		for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++)
			if ((*f_p)->selected)
			{
				b_p = Fv_openfolder_path;
				while (*b_p)
					*s_p++ = *b_p++;
				*s_p++ = '/';
				if (cut)
				{
					*s_p++ = '.';
					*s_p++ = '.';
				}
				b_p = (*f_p)->name;
				while (*b_p)
					*s_p++ = *b_p++;
				*s_p++ = ' ';

				if (s_p - Sbuf > 4064)
					break;
			}
		if (*(s_p-1) == ' ')	/* Strip space off end */
			s_p--;
		*s_p = NULL;
	}

	if (cut)
	{
		if (mark_for_delete())
		{
			Fv_current->mtime = time((time_t *)0);
			fv_display_folder(FV_SMALLER_FOLDER);
		}
		else
			return(NULL);
	}

	return(Sbuf);
}


fv_delete_selected()
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	int len;
	unsigned undelete_len;
	register char *u_p;
	char *realloc();

	if (mark_for_delete())
	{
		/* Remember marked files for undelete/confirm */

		if (Fv_undelete)
			fv_commit_delete(TRUE);

		undelete_len = 256;
		Fv_undelete = (char *)fv_malloc(undelete_len);
		if (Fv_undelete)
		{
		(void)strcpy(Fv_undelete, Fv_openfolder_path);
		u_p = Fv_undelete + strlen(Fv_openfolder_path);
		*u_p++ = '\t';
		for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++)
			if ((*f_p)->mtime == 1)
			{
				/* ..<name><tab><null> */

				len = strlen((*f_p)->name)+3;
				if (u_p+len - Fv_undelete > undelete_len)
				{
					undelete_len += 256;
					Fv_undelete = realloc(Fv_undelete,
						(unsigned)undelete_len);
				}
				*u_p++ = '.';
				*u_p++ = '.';
				(void)strcpy(u_p, (*f_p)->name);
				u_p += len-3;
				*u_p++ = '\t';
			}
		*u_p = NULL;
		}

		Fv_current->mtime = time((time_t *)0);
		fv_display_folder(FV_SMALLER_FOLDER);
	}
}


static
mark_for_delete()		/* Mark by setting time to 1 */
{
	register FV_FILE **f_p, **l_p;	/* Files array pointers */
	register FV_TNODE *t_p, *nt_p;	/* Tree nodes */
	char buf[256];
	register int removed = 0;	/* Number of marked files */
	int tree_removed = 0;

	for (f_p=Fv_file, l_p=Fv_file+Fv_nfiles; f_p != l_p; f_p++)
		if ((*f_p)->selected)
		{
			if (access((*f_p)->name, W_OK) == -1)
			{
				fv_putmsg(TRUE, Fv_message[MEDELETE],
					(int)(*f_p)->name, sys_errlist[errno]);
				break;
			}
				
			(void)sprintf(buf, "..%s", (*f_p)->name);
			if (rename((*f_p)->name, buf) == -1)
			{
				fv_putmsg(TRUE, Fv_message[MEDELFAILED],
					(int)buf, (int)sys_errlist[errno]);
				break;
			}
			else
			{
				(*f_p)->mtime = 1;
				removed++;

				/* Fix up tree */
				if ((*f_p)->type == FV_IFOLDER)
				{
					for (nt_p = NULL, t_p=Fv_current->child;
						t_p; nt_p = t_p, t_p = t_p->sibling)
						if (strcmp(t_p->name, (*f_p)->name)==0)
						{
							if (nt_p)
								nt_p->sibling = t_p->sibling;
							else
								Fv_current->child = t_p->sibling;
							free(t_p->name);
							free((char *)t_p);
							tree_removed++;
							break;
						}
				}
			}
		}

	/* Update tree, if visible */

	if (tree_removed && Fv_treeview)
		fv_drawtree(TRUE);

	return(removed);
}

fv_undelete()
{
	if (fv_commit_delete(FALSE) == SUCCESS)
		fv_display_folder(FV_BUILD_FOLDER);
	else
		fv_putmsg(TRUE, Fv_message[MEDELETE], 0, 0);
}

fv_commit_delete(delete)
	BOOLEAN delete;
{
	register char *u_p, *f_p;
	struct stat fstatus;
	char buf[256];

	if (!Fv_undelete)
		return(FAILURE);
	u_p = Fv_undelete;
	while (*u_p && *u_p != '\t')
		u_p++;
	if (*u_p != '\t')
		return(FAILURE);
	*u_p = NULL;

	if (chdir(Fv_undelete)==-1)
		return(FAILURE);

	f_p = u_p+1;
	while (*f_p)
	{
		u_p = f_p;
		while (*u_p && *u_p != '\t')
			u_p++;
		*u_p = NULL;
		if (delete)
		{
			if (stat(f_p, &fstatus) != -1)
				if ((fstatus.st_mode & S_IFMT) == S_IFDIR)
				{
					(void)sprintf(buf, "rm -rf %s2>/tmp/.err", f_p);
					if (system(buf))
						fv_showerr();
				}
				else
					(void)unlink(f_p);
		}
		else
		{
			if (rename(f_p, f_p+2) == -1)
				fv_putmsg(TRUE, Fv_message[MEUNDELFAILED],
					(int)f_p+2, (int)sys_errlist[errno]);
		} 
		f_p = u_p+1;
	}

	/* Update the modification time on the folder so it doesn't get
	 * redisplayed */
	if (delete && strcmp(Fv_undelete, Fv_openfolder_path) == 0)
		Fv_current->mtime = time(0);

	(void)chdir(Fv_openfolder_path);
	free(Fv_undelete);
	Fv_undelete = NULL;

	return(SUCCESS);
}

/* Does a copy of this file exist?  Quit prematurely if sorted by name. */

static
my_access(f_p)
	register FV_FILE **f_p;
{
	char buf[262];
	register int i;
	register FV_FILE **l_p;


	(void)sprintf(buf, "%s.copy", (*f_p)->name);

	if (Fv_sortby != FV_SORTALPHA)
		f_p = Fv_file;
	l_p = Fv_file+Fv_nfiles;

	while (f_p != l_p)
	{
		i = strcmp(buf, (*f_p)->name);
		if (i==0)
		{
			fv_putmsg(TRUE, Fv_message[MEEXIST], (int)buf, 0);
			return(TRUE);
		}
		if (i<0 && Fv_sortby == FV_SORTALPHA)
			return(FALSE);
		f_p++;
	}

	return(FALSE);
}
