#ifndef lint
static char sccsid[] = "@(#)stdio.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/stdio.h>
#include <nse/util.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/dir.h>

static Nse_err	write_error();
static Nse_err	file_size();

/*
 * Wrapper routines which call the standard stdio library routines and
 * return an Nse_err struct if a read/write error occurs.
 */

Nse_err
nse_fclose(path, stream)
	char		*path;
	FILE		*stream;
{
	if (EOF == fclose(stream)) {
		/*
		 * fclose should only fail for streams that have been
		 * written to (because the close will force the kernel to
		 * flush all modified buffers for the file, and this delayed
		 * write may fail.)
		 */
#ifdef SUN_OS_4
		/*
		 * XXX Get around SunOS 4.0 bug which causes spurious error
		 * return by close(2).  (Bug ID 1010171)
		 */
		if (errno == -98) {
			return NULL;
		}
#endif SUN_OS_4
		return nse_err_format_errno("Write (fclose) to \"%s\"", path);
	} else {
		return NULL;
	}
}


Nse_err
nse_fflush(path, stream)
	char		*path;
	FILE		*stream;
{
	if (EOF == fflush(stream)) {
		return write_error(path, stream, "fflush");
	} else {
		return NULL;
	}
}


/*
 * The character read is returned in '*cp'.  EOF is returned at
 * end-of-file.
 */
Nse_err
nse_fgetc(path, stream, cp)
	char		*path;
	FILE		*stream;
	int		*cp;
{
	*cp = fgetc(stream);
	if (*cp == EOF && ferror(stream)) {
		return nse_err_format_errno("Read (fgetc) from \"%s\"", path);
	}
	return NULL;
}


/*
 * '*eofp' is returned as TRUE when end-of-file reached.
 */
Nse_err
nse_fgets(path, buf, n, stream, eofp)
	char		*path;
	char		*buf;
	int		n;
	FILE		*stream;
	bool_t		*eofp;
{
	if (fgets(buf, n, stream) == NULL) {
		*eofp = TRUE;
		if (ferror(stream)) {
			return nse_err_format_errno("Read (fgets) from \"%s\"",
						    path);
		}
	} else {
		*eofp = FALSE;
	}
	return NULL;
}


Nse_err
nse_fopen(path, mode, filep)
	char		*path;
	char		*mode;
	FILE		**filep;
{
	*filep = fopen(path, mode);
	if (*filep) {
		return NULL;
	}
	if (NSE_STREQ(mode, "w") || NSE_STREQ(mode, "a")) {
		return nse_err_format_errno("Open of \"%s\" for writing",
					    path);
	} else if (errno == ENOENT) {
		/*
		 * Not an error if the file doesn't exist when
		 * opening for read or update.
		 */
		return NULL;
	} else if (NSE_STREQ(mode, "r")) {
		return nse_err_format_errno("Open of \"%s\" for reading",
					    path);
	} else {
		return nse_err_format_errno(
				"Open of \"%s\" for reading and writing",
					    path);
	}
}


/* VARARGS3 */
Nse_err
nse_fprintf(path, stream, format, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	char		*path;
	FILE		*stream;
	char		*format;
	int		a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	if (EOF == fprintf(stream, format, a1, a2, a3, a4, a5, a6, a7,
			   a8, a9)) {
		return write_error(path, stream, "fprintf");
	} else {
		return NULL;
	}
}


/*
 * Returns, in *countp, the value returned by fprintf (the number of
 * characters printed.)
 */
/* VARARGS4 */
Nse_err
nse_fprintf_value(countp, path, stream, format, a1, a2, a3, a4, a5, a6, a7, a8,
		  a9)
	int		*countp;
	char		*path;
	FILE		*stream;
	char		*format;
	int		a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	int		cc;

	cc = fprintf(stream, format, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	if (cc == EOF) {
		return write_error(path, stream, "fprintf");
	} else {
		*countp = cc;
		return NULL;
	}
}


Nse_err
nse_fputc(path, c, stream)
	char		*path;
	char		c;
	FILE		*stream;
{
	if (EOF == fputc(c, stream)) {
		return write_error(path, stream, "fputc");
	} else {
		return NULL;
	}
}


Nse_err
nse_fputs(path, buf, stream)
	char		*path;
	char		*buf;
	FILE		*stream;
{
	if (EOF == fputs(buf, stream)) {
		return write_error(path, stream, "fputs");
	} else {
		return NULL;
	}
}


/*
 * Returns, in *countp, the value returned by fscanf (the number of items
 * read in.)  *countp will be returned as EOF when end-of-file is reached.
 */
/* VARARGS4 */
Nse_err
nse_fscanf(path, countp, stream, format, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	char		*path;
	int		*countp;
	FILE		*stream;
	char		*format;
	int		a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	*countp = fscanf(stream, format, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	if (*countp == EOF && ferror(stream)) {
		return nse_err_format_errno("Read (fscanf) from \"%s\"", path);
	} else {
		return NULL;
	}
}


/*
 * This routine is called when a write routine returns EOF.  Standard
 * stdio routines can return EOF with ferror() FALSE (and errno = 0)
 * if the stream wasn't opened for writing, so handle that condition
 * here.
 */
static Nse_err
write_error(path, stream, type)
	char		*path;
	FILE		*stream;
	char		*type;
{
	if (!ferror(stream)) {
		/*
		 * Return the error code that write(2) returns when called
		 * on an fd that wasn't opened for writing.
		 */
		errno = EBADF;
	}
	return nse_err_format_errno("Write (%s) to \"%s\"", type, path);
}


Nse_err
nse_readdir(path, dirp, dpp)
	char		*path;
	DIR		*dirp;
	struct direct	**dpp;
{
	errno = 0;
	*dpp = readdir(dirp);
	if (*dpp == NULL && errno != 0) {
		return nse_err_format_errno("Readdir of \"%s\"", path);
	}
	return NULL;
}


/*
 * Routines to safely rewrite small ( < 2048 bytes) files in place, which
 * avoid the overhead of opening a tmp file and then renaming it to the
 * real file.  These routines ensure that the integrity of the file is
 * preserved if the file system fills up while trying to write the file.
 * The safe way to do this is to write the file to a tmp file, then rename
 * the tmp file to the real file only after the tmp file has been written
 * successfully.  If the file to be written is a short file, we can do
 * better than this.  If the whole contents of the file fit within one
 * buffer, then the write can be done in place.  This is because only one
 * write will be necessary, and if this write fails, the original file will
 * be left intact.  (This is faster than writing to a tmp file, and also
 * has less likelihood of failing when the filesystem fills up.)
 *
 * These routines are useful in cases where the file must be rewritten
 * (instead of appended to.)  The client must keep track of the number of
 * characters written to the file, and pass the new size into
 * nse_safe_file_close().  Additionally, any routines which read files
 * created with these routines must check for NULL characters in the file
 * -- a NULL character indicates EOF (see comment in
 * nse_safe_file_close().)
 */

#define BUF_LEN		2048

/*
 * Will file be too big to fit within one buffer?  Assume here that at most
 * one line will be added to the file, and that no line in the file will be
 * longer than 200 characters.
 */
#define FILE_TOO_BIG(size)	(size > BUF_LEN - 200)


/*
 * If '*filep' is NULL, opens the file 'path' and returns the file pointer
 * in '*filep'; otherwise uses the pre-opened file.  The size of the file
 * (for use in nse_safe_write_close()) is returned in 'old_sizep'.
 */
Nse_err
nse_safe_write_open(path, filep, old_sizep)
	char		*path;
	FILE		**filep;
	long		*old_sizep;
{
	Nse_err		err;

	if (err = file_size(path, *filep, old_sizep)) {
		return err;
	}
	if (!FILE_TOO_BIG(*old_sizep)) {
		if (!*filep) {
			if (err = nse_file_open(path, "a", filep)) {
				return err;
			}
		}
		rewind(*filep);
		return NULL;
	} else {
		if (*filep && (err = nse_fclose(path, *filep))) {
			return err;
		}
		strcat(path, ".tmp");
		err = nse_file_open(path, "w", filep);
		*rindex(path, '.') = '\0';
		return err;
	}
}


Nse_err
nse_safe_write_close(path, file, size, old_size)
	char		*path;
	FILE		*file;
	long		size;
	long		old_size;
{
	Nse_err		err;

	if (!FILE_TOO_BIG(old_size)) {
		if (size < old_size) {
			/*
			 * If the file will be shorter than it used to be,
			 * it needs to be truncated to its new length.  To
			 * always leave the file in a consistent state, we
			 * flush the buffer before truncating the file.
			 * Since we stick a NULL character at the end of
			 * the buffer before flushing it, we can recover
			 * if the write happens but the ftruncate doesn't.
			 * In such a case, the file can have some junk
			 * at the end after the NULL.  This will not be
			 * a problem as long as input routines check for
			 * NULL's in the file, as tfs_fgets() does.
			 */
			if ((err = nse_fputc(path, '\0', file)) ||
			    (err = nse_fflush(path, file))) {
				fclose(file);
				return err;
			}
			if (ftruncate(fileno(file), size) < 0) {
				err = nse_err_format_errno(
					"Write to \"%s\" (ftruncate)", path);
				fclose(file);
				return err;
			}
		}
		if (err = nse_fclose(path, file)) {
			return err;
		}
	} else {
		char		path_tmp[MAXPATHLEN];

		strcpy(path_tmp, path);
		strcat(path_tmp, ".tmp");
		if (err = nse_fclose(path_tmp, file)) {
			(void) unlink(path_tmp);
			return err;
		}
		if (rename(path_tmp, path) < 0) {
			return nse_err_format_errno(
						"Rename of \"%s\" to \"%s\"",
						    path_tmp, path);
		}
	}
	return NULL;
}


/*
 * Open a file with a stdio buffer of size BUF_LEN.
 */
Nse_err
nse_file_open(path, mode, filep)
	char		*path;
	char		*mode;
	FILE		**filep;
{
	static char	*buf;
	Nse_err		err;

	if (err = nse_fopen(path, mode, filep)) {
		return err;
	} else if (*filep) {
		if (buf == NULL) {
			buf = malloc((unsigned) BUF_LEN);
		}
		setbuffer(*filep, buf, BUF_LEN);
	}
	return NULL;
}


static Nse_err
file_size(path, file, sizep)
	char		*path;
	FILE		*file;
	long		*sizep;
{
	struct stat	statb;
	int		result;

	if (file) {
		result = fstat(fileno(file), &statb);
	} else {
		result = lstat(path, &statb);
	}
	if (result == 0) {
		*sizep = statb.st_size;
	} else if (errno == ENOENT) {
		*sizep = 0;
	} else {
		return nse_err_format_errno("Stat of \"%s\"", path);
	}
	return NULL;
}
