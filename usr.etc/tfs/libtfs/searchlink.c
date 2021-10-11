#ifndef lint
static char sccsid[] = "@(#)searchlink.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * Library routines to read/write .tfs_info files, which contain
 * searchlinks, backlinks, and whiteout information.  These routines
 * are called by the tfsd and by preserve, which creates rev-trees
 * behind the tfsd's back, and so needs to look at .tfs_info files
 * directly.
 */

#include <nse/stdio.h>
#include <strings.h>
#include <errno.h>
#include <sys/stat.h>
#include <nse/param.h>
#include <nse/util.h>
#include <nse/searchlink.h>
#include <nse/file_err.h>

#define STRIP_EOLN(string)	string[strlen(string) - 1] = '\0';

static Nse_err	tfs_file_move();
static Nse_err	tfs_file_read();
static Nse_err	do_file_read();
static Nse_err	tfs_file_write();
static Nse_err	write_tfs_file_contents();
static Nse_err	tfs_fgets();


/*
 * Returns the searchlink of 'directory' in the string 'name' (which should
 * be MAXPATHLEN chars long.)  If there is no searchlink for the directory,
 * ENOENT is returned as the error code.
 */
Nse_err
nse_get_searchlink(directory, name)
	char		*directory;
	char		*name;
{
	Nse_err		result;

	result = tfs_file_read(directory, name,
				(int *) NULL, (Nse_whiteout *) NULL,
				(Nse_whiteout *) NULL, (FILE **) NULL);
	if ((result == NULL) && (name[0] == '\0')) {
		errno = ENOENT;
		result = nse_err_format_errno("Tfs read of searchlink in directory \"%s\"", 
		    directory);
	}
	return (result);
}


/*
 * Set searchlink of 'directory' to 'name', regardless of whether or not a
 * searchlink already exists for 'directory'.
 */
Nse_err
nse_set_searchlink(directory, name)
	char		*directory;
	char		*name;
{
	FILE		*file;
	char		old_link[MAXPATHLEN];
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	wo;
	Nse_err		result;

	if (result = tfs_file_read(directory, old_link, &blct, &bl,
				   &wo, &file)) {
		return (result);
	}
	result = tfs_file_write(directory, name, blct, bl, wo, file);
	nse_dispose_whiteout(bl);
	nse_dispose_whiteout(wo);
	return (result);
}


/*
 * Set the searchlink of 'directory' to 'name' only if 'directory' does not
 * already have a searchlink.  An error code is returned in two cases:
 * either the directory already has a searchlink which has a value
 * different from 'name', in which case NSE_SL_ALREADY_SET_DIFF is
 * returned and 'old_name' contains the old searchlink; or an error
 * occurred, in which case the intro(2) error code is returned.
 *
 * The boolean pointed at by `changedp' is set to indicate if the
 * searchlink changed.
 */
Nse_err
nse_set_cond_searchlink(directory, name, old_name, changedp)
	char		*directory;
	char		*name;
	char		*old_name;
	bool_t		*changedp;
{
	FILE		*file;
	Nse_whiteout	wo;
	int		blct;
	Nse_whiteout	bl;
	Nse_err		result;

	*changedp = FALSE;
	if (result = tfs_file_read(directory, old_name, &blct, &bl,
				   &wo, &file)) {
		return (result);
	}
	if (old_name[0] != '\0') {
		fclose(file);
		if (!NSE_STREQ(name, old_name)) {
			result = nse_err_format(NSE_SL_ALREADY_SET_DIFF,
						directory);
		}
	} else {
		*changedp = TRUE;
		result = tfs_file_write(directory, name, blct, bl, wo, file);
	}
	nse_dispose_whiteout(bl);
	nse_dispose_whiteout(wo);
	return (result);
}

/*
 * Returns list of white-out entries in directory.
 */
Nse_err
nse_get_whiteout(directory, wop)
	char		*directory;
	Nse_whiteout	*wop;
{
	char		searchlink[MAXPATHLEN];

	return tfs_file_read(directory, searchlink, (int *) NULL,
				(Nse_whiteout *) NULL, wop, (FILE **) NULL);
}


/*
 * Set a new whiteout entry 'name' in 'directory'.  This function is a
 * no-op if whiteout entry 'name' is already set for the directory.
 */
Nse_err
nse_set_whiteout(directory, name)
	char		*directory;
	char		*name;
{
	FILE		*file;
	char		searchlink[MAXPATHLEN];
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	wo_first;
	Nse_whiteout	wo;
	int		found = 0;
	Nse_err		result;

	if (result = tfs_file_read(directory, searchlink, &blct,
					&bl, &wo_first, &file)) {
		return (result);
	}

	wo = wo_first;
	while (wo != NULL) {
		if (NSE_STREQ(name, wo->name)) {
			found = 1;
			break;
		}
		wo = wo->next;
	}
	if (found) {
		fclose(file);
	} else {
		wo = NSE_NEW(Nse_whiteout);
		wo->name = NSE_STRDUP(name);
		wo->next = wo_first;
		wo_first = wo;
		result = tfs_file_write(directory, searchlink, blct, bl,
						wo_first, file);
	}
	nse_dispose_whiteout(bl);
	nse_dispose_whiteout(wo_first);
	return (result);
}


/*
 * Clear whiteout entry 'name' from the whiteout list of 'directory'.  If
 * 'name' is not in the whiteout list of 'directory', 0 is returned.
 */
Nse_err
nse_clear_whiteout(directory, name)
	char		*directory;
	char		*name;
{
	FILE		*file;
	char		searchlink[MAXPATHLEN];
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	wo_first;
	Nse_whiteout	wo;
	Nse_whiteout	wo_prev = NULL;
	int		found = 0;
	Nse_err		result;

	if (result = tfs_file_read(directory, searchlink, &blct,
					&bl, &wo_first, &file)) {
		return (result);
	}

	wo = wo_first;
	while (wo != NULL) {
		if (NSE_STREQ(name, wo->name)) {
			found = 1;
			break;
		}
		wo_prev = wo;
		wo = wo->next;
	}
	if (found) {
		if (wo_prev == NULL) {
			wo_first = wo->next;
		} else {
			wo_prev->next = wo->next;
		}
		free(wo->name);
		free((char *) wo);
		result = tfs_file_write(directory, searchlink,
					blct, bl, wo_first, file);
	} else {
		fclose(file);
	}
	nse_dispose_whiteout(bl);
	nse_dispose_whiteout(wo_first);
	return (result);
}

/*
 * Purge all whiteout info from a tfs file and replace search link.
 *
 */
Nse_err
nse_purge_whiteout(directory, newlink)
	char		*directory;
	char		*newlink;
{
	FILE		*file;
	char		searchlink[MAXPATHLEN];
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	wo;
	Nse_err		result;

	if (result = tfs_file_read(directory, searchlink, &blct,
					&bl, &wo, &file)) {
		return result;
	}
	nse_dispose_whiteout(wo);
	result = tfs_file_write(directory, newlink,
					blct, bl, (Nse_whiteout) NULL, file);
	nse_dispose_whiteout(bl);
	return result;
}


/*
 * Release the list allocated by nse_get_whiteout.
 */
void
nse_dispose_whiteout(wo)
	Nse_whiteout	wo;
{
	Nse_whiteout	wo_next;

	while (wo != NULL) {
		wo_next = wo->next;
		free(wo->name);
		free((char *) wo);
		wo = wo_next;
	}
}


/*
 * Returns list of backlink entries in directory.
 */
Nse_err
nse_get_backlink(directory, blp)
	char		*directory;
	Nse_whiteout	*blp;
{
	char		searchlink[MAXPATHLEN];
	int		blct;

	return tfs_file_read(directory, searchlink, &blct, blp,
				(Nse_whiteout *) NULL, (FILE **) NULL);
}


/*
 * Set a new backlink entry 'name' in 'directory'.  This function is a
 * no-op if backlink entry 'name' is already set for the directory.
 */
Nse_err
nse_set_backlink(directory, name)
	char		*directory;
	char		*name;
{
	FILE		*file;
	char		searchlink[MAXPATHLEN];
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	bl_first;
	Nse_whiteout	wo;
	int		found = 0;
	Nse_err		result;

	if (result = tfs_file_read(directory, searchlink, &blct,
					&bl_first, &wo, &file)) {
		return (result);
	}

	bl = bl_first;
	while (bl != NULL) {
		if (NSE_STREQ(name, bl->name)) {
			found = 1;
			break;
		}
		bl = bl->next;
	}
	if (found) {
		fclose(file);
	} else {
		bl = NSE_NEW(Nse_whiteout);
		bl->name = NSE_STRDUP(name);
		bl->next = bl_first;
		bl_first = bl;
		blct++;
		result = tfs_file_write(directory, searchlink, blct, bl_first,
						wo, file);
	}
	nse_dispose_whiteout(bl_first);
	nse_dispose_whiteout(wo);
	return (result);
}


/*
 * Clear backlink entry 'name' from the backlink list of 'directory'.  If
 * 'name' is not in the backlink list of 'directory', 0 is returned.
 */
Nse_err
nse_clear_backlink(directory, name)
	char		*directory;
	char		*name;
{
	FILE		*file;
	char		searchlink[MAXPATHLEN];
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	bl_first;
	Nse_whiteout	wo;
	Nse_whiteout	bl_prev = NULL;
	int		found = 0;
	Nse_err		result;

	if (result = tfs_file_read(directory, searchlink, &blct,
					&bl_first, &wo, &file)) {
		return (result);
	}

	bl = bl_first;
	while (bl != NULL) {
		if (NSE_STREQ(name, bl->name)) {
			found = 1;
			break;
		}
		bl_prev = bl;
		bl = bl->next;
	}
	if (found) {
		if (bl_prev == NULL) {
			bl_first = bl->next;
		} else {
			bl_prev->next = bl->next;
		}
		free(bl->name);
		free((char *) bl);
		blct--;
		result = tfs_file_write(directory, searchlink,
					blct, bl_first, wo, file);
	} else {
		fclose(file);
	}
	nse_dispose_whiteout(bl_first);
	nse_dispose_whiteout(wo);
	return (result);
}


static FILE	*tfs_info_file;

/*
 * Return both the searchlink, backlink, and the white-out entries in
 * 'directory'.
 */
Nse_err
nse_get_tfs_info(directory, name, blp, wop, will_write_later)
	char		*directory;
	char		*name;
	Nse_whiteout	*blp;
	Nse_whiteout	*wop;
	bool_t		will_write_later;
{
	FILE		**filep;
	int		blct;

	if (will_write_later) {
		filep = &tfs_info_file;
	} else {
		filep = NULL;
	}
	return tfs_file_read(directory, name, &blct, blp, wop, filep);
}


/*
 * Set all tfs info for 'directory'.
 */
Nse_err
nse_set_tfs_info(directory, name, bl_first, wo)
	char		*directory;
	char		*name;
	Nse_whiteout	bl_first;
	Nse_whiteout	wo;
{
	int		blct = 0;
	Nse_whiteout	bl;
	Nse_err		result;

	for (bl = bl_first; bl != NULL; bl = bl->next) {
		blct++;
	}
	result = tfs_file_write(directory, name, blct, bl_first, wo,
				tfs_info_file);
	nse_dispose_whiteout(bl_first);
	nse_dispose_whiteout(wo);
	return (result);
}

/*
 * Copies all tfs_info except backlinks (searchlink, white-out entries, *and*
 * backfiles info) from 'dir1' to 'dir2'.
 */
Nse_err
nse_shift_tfs_info(dir1, dir2)
	char		*dir1;
	char		*dir2;
{
	char		old_link[MAXPATHLEN];
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	wo;
	Nse_err		result;

	result = tfs_file_read(dir1, old_link, &blct, &bl, &wo,
			       (FILE **) NULL);
	if (result) {
		return result;
	}
	nse_dispose_whiteout(bl);
	result = tfs_file_write(dir2, old_link, 0, (Nse_whiteout) NULL, wo,
				(FILE *) NULL);
	nse_dispose_whiteout(wo);
	if (result) {
		return result;
	}
	result = tfs_file_move(dir1, dir2, NSE_TFS_BACK_FILE, TRUE);
	if ((result != NULL) && (result->code == ENOENT)) {
		result = NULL;
	}
	return result;
}

/*
 * Copies all tfs_info (searchlink, white-out entries, backlinks, *and*
 * backfiles info) from 'dir1' to 'dir2'.
 */
Nse_err
nse_copy_tfs_info(dir1, dir2)
	char		*dir1;
	char		*dir2;
{
	Nse_err		result;

	if (result = tfs_file_move(dir1, dir2, NSE_TFS_FILE, TRUE)) {
		return result;
	}
	result = tfs_file_move(dir1, dir2, NSE_TFS_BACK_FILE, TRUE);
	if (result == NULL || result->code == ENOENT) {
		return NULL;
	} else {
		return result;
	}
}


/*
 * Moves all tfs_info (searchlink, white-out entries, backlinks, *and*
 * backfiles info) from 'dir1' to 'dir2'.
 */
Nse_err
nse_move_tfs_info(dir1, dir2)
	char		*dir1;
	char		*dir2;
{
	Nse_err		result;

	if (result = tfs_file_move(dir1, dir2, NSE_TFS_FILE, FALSE)) {
		return result;
	}
	result = tfs_file_move(dir1, dir2, NSE_TFS_BACK_FILE, FALSE);
	if (result == NULL || result->code == ENOENT) {
		return NULL;
	} else {
		return result;
	}
}

/*
 * Destroy the tfs_info file in 'directory'.
 */
Nse_err
nse_clear_tfs_info(directory)
	char		*directory;
{
	char		pathname[MAXPATHLEN];
	int		r;

	strcpy(pathname, directory);
	nse_pathcat(pathname, NSE_TFS_FILE);
	r = unlink(pathname);
	if ((r == 0) || (errno == ENOENT)) {
		return NULL;
	} else {
		return nse_err_format_errno("Unlink of \"%s\"", pathname);
	}
}

/*
 * Move 'fname' from 'dir1' to 'dir2', or if 'make_copy' is TRUE, copy the
 * file.
 */
static Nse_err
tfs_file_move(dir1, dir2, fname, make_copy)
	char		*dir1;
	char		*dir2;
	char		*fname;
	bool_t		make_copy;
{
	char		src[MAXPATHLEN];
	char		dest[MAXPATHLEN];
	Nse_err		err;

	strcpy(src, dir1);
	nse_pathcat(src, fname);
	strcpy(dest, dir2);
	nse_pathcat(dest, fname);
	if (make_copy) {
		return _nse_copy_file(dest, src, 0);
	}
	if (rename(src, dest) == 0) {
		return NULL;
	}
	if (errno != EXDEV) {
		return nse_err_format_errno("Rename of \"%s\" to \"%s\"",
					    src, dest);
	}
	err = _nse_copy_file(dest, src, 1);
	if (err == NULL && unlink(src) < 0) {
		err = nse_err_format_errno("Unlink of \"%s\"", src);
	}
	return err;
}


static Nse_err
tfs_file_read(directory, searchlink, blctp, blp, whiteoutp, filep)
	char		*directory;
	char		*searchlink;
	int		*blctp;
	Nse_whiteout	*blp;
	Nse_whiteout	*whiteoutp;
	FILE		**filep;
{
	int		ntries = 0;
	Nse_err		err;

	/*
	 * Retry the file read if a stale NFS file handle is encountered.
	 * We have to do this because it's possible for a process on
	 * another machine to remove the file after we've opened the file
	 * for reading but before we've read any data from it.  (This
	 * can only happen if the file is large enough that it is written
	 * to a tmp file before being renamed to the real name.)
	 */
	do {
		err = do_file_read(directory, searchlink, blctp, blp,
				   whiteoutp, filep);
		/* XXX free *blp && *whiteoutp if err = ESTALE? */
		ntries++;
	} while (err && ntries < 10 && err->code == ESTALE);
	return err;
}


static Nse_err
do_file_read(directory, searchlink, blctp, blp, whiteoutp, filep)
	char		*directory;
	char		*searchlink;
	int		*blctp;
	Nse_whiteout	*blp;
	Nse_whiteout	*whiteoutp;
	FILE		**filep;
{
	FILE		*tfs_file;
	char		path[MAXPATHLEN];
	char		name[MAXPATHLEN];
	int		first_ch;
	char		*p;
	Nse_whiteout	wo;
	Nse_whiteout	first_wo;
	Nse_whiteout	prev_wo;
	Nse_whiteout	bl;
	Nse_whiteout	first_bl;
	Nse_whiteout	prev_bl;
	int		blct;
	int		i;
	int		version;
	bool_t		eof;
	Nse_err		err = NULL;

	strcpy(path, directory);
	nse_pathcat(path, NSE_TFS_FILE);
	if (filep) {
		err = nse_file_open(path, "r+", &tfs_file);
	} else {
		err = nse_file_open(path, "r", &tfs_file);
	}
	if (err) {
		return err;
	}
	searchlink[0] = '\0';
	if (whiteoutp != NULL) {
		*whiteoutp = NULL;
	}
	if (blp != NULL) {
		*blp = NULL;
		*blctp = 0;
	}
	if (tfs_file == NULL) {
		if (filep) {
			*filep = NULL;
		}
		return NULL;
	}

	/*
	 * Process the 'VERSION X' line (if it exists.)
	 */
	if (err = nse_fgetc(path, tfs_file, &first_ch)) {
		goto done;
	}
	if (first_ch == NSE_TFS_VERSION_HDR[0]) {
		err = tfs_fgets(path, name, MAXPATHLEN, tfs_file, &eof);
		if (eof) {
			goto done;
		}
		if ((p = rindex(name, ' ')) == NULL) {
			version = 0;
		} else {
			version = atoi(p + 1);
		}
	} else {
		ungetc(first_ch, tfs_file);
		version = 0;
	}
	if (version > NSE_TFS_VERSION) {
		err = nse_err_format(NSE_FILE_BAD_VERS, path, version);
		goto done;
	}
	err = tfs_fgets(path, searchlink, MAXPATHLEN, tfs_file, &eof);
	if (eof) {
		goto done;
	}
	STRIP_EOLN(searchlink);

	if (NSE_TFS_HAS_BACK(version)) {
		err = tfs_fgets(path, name, MAXPATHLEN, tfs_file, &eof);
		if (eof) {
			goto done;
		}
		blct = atoi(name);
		if (blp == NULL) {
			for (i = 0; i < blct; i++) {
				err = tfs_fgets(path, name, MAXPATHLEN,
						tfs_file, &eof);
				if (eof) {
					goto done;
				}
			}
		} else {
			first_bl = NULL;
			bl = NULL;
			prev_bl = NULL;
			for (i = 0; i < blct; i++) {
				err = tfs_fgets(path, name, MAXPATHLEN,
						tfs_file, &eof);
				if (eof) {
					goto done;
				}
				bl = NSE_NEW(Nse_whiteout);
				bl->name = NSE_STRDUP(name);
				STRIP_EOLN(bl->name);
				bl->next = NULL;
				if (prev_bl != NULL) {
					prev_bl->next = bl;
				}
				if (first_bl == NULL) {
					first_bl = bl;
				}
				prev_bl = bl;
			}
			*blp = first_bl;
			*blctp = blct;
		}
	}

	if (whiteoutp == NULL) {
		goto done;
	}
	first_wo = NULL;
	wo = NULL;
	prev_wo = NULL;
	err = tfs_fgets(path, name, MAXPATHLEN, tfs_file, &eof);
	while (!eof) {
		wo = NSE_NEW(Nse_whiteout);
		wo->name = NSE_STRDUP(name);
		STRIP_EOLN(wo->name);
		wo->next = NULL;
		if (prev_wo != NULL) {
			prev_wo->next = wo;
		}
		if (first_wo == NULL) {
			first_wo = wo;
		}
		prev_wo = wo;
		err = tfs_fgets(path, name, MAXPATHLEN, tfs_file, &eof);
	};
	*whiteoutp = first_wo;
done:
	if (!err) {
		if (filep) {
			*filep = tfs_file;
		} else {
			err = nse_fclose(path, tfs_file);
		}
	} else {
		fclose(tfs_file);
	}
	return err;
}


/*
 * Write out the .tfs_info file in 'directory', with the indicated
 * searchlink, back links, and whiteout entries.  This routine is careful
 * to ensure that the integrity of the file is preserved if the file system
 * fills up while trying to write the file.
 */
static Nse_err
tfs_file_write(directory, searchlink, blct, bl, wo, tfs_file)
	char		*directory;
	char		*searchlink;
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	wo;
	FILE		*tfs_file;
{
	char		path[MAXPATHLEN];
	long		old_size;
	long		size;
	Nse_err		err;

	strcpy(path, directory);
	nse_pathcat(path, NSE_TFS_FILE);
	if (err = nse_safe_write_open(path, &tfs_file, &old_size)) {
		return err;
	}
	size = 0;
	if (err = write_tfs_file_contents(path, &size, tfs_file, searchlink,
					  blct, bl, wo)) {
		fclose(tfs_file);
		return err;
	}
	return nse_safe_write_close(path, tfs_file, size, old_size);
}


static Nse_err
write_tfs_file_contents(path, sizep, tfs_file, searchlink, blct, bl, wo)
	char		*path;
	long		*sizep;
	FILE		*tfs_file;
	char		*searchlink;
	int		blct;
	Nse_whiteout	bl;
	Nse_whiteout	wo;
{
	int		count;
	Nse_err		err;

	if (searchlink != NULL) {
		err = nse_fprintf_value(&count, path, tfs_file,
					"%s %d\n%s\n%d\n",
					NSE_TFS_VERSION_HDR, NSE_TFS_VERSION,
					searchlink, blct);
	} else {
		err = nse_fprintf_value(&count, path, tfs_file,
					"%s %d\n\n%d\n",
					NSE_TFS_VERSION_HDR, NSE_TFS_VERSION,
					blct);
	}
	if (err) {
		return err;
	}
	*sizep += count;
	while (bl != NULL) {
		if ((err = nse_fputs(path, bl->name, tfs_file)) ||
		    (err = nse_fputs(path, "\n", tfs_file))) {
			return err;
		}
		*sizep += strlen(bl->name) + 1;
		bl = bl->next;
	}

	while (wo != NULL) {
		if ((err = nse_fputs(path, wo->name, tfs_file)) ||
		    (err = nse_fputs(path, "\n", tfs_file))) {
			return err;
		}
		*sizep += strlen(wo->name) + 1;
		wo = wo->next;
	}
	return NULL;
}


/*
 * fgets from 'stream', checking that the line doesn't start with '\0'.
 * (The line can start with '\0' if a previous write of this file was
 * interrupted.  (See nse_safe_write_close().)
 */
static Nse_err
tfs_fgets(path, buf, n, stream, eofp)
	char		*path;
	char		*buf;
	int		n;
	FILE		*stream;
	bool_t		*eofp;
{
	Nse_err		err;

	err = nse_fgets(path, buf, n, stream, eofp);
	if (!*eofp && buf[0] == '\0') {
		*eofp = TRUE;
	}
	return err;
}
