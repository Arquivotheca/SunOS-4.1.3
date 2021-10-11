/*	@(#)stdio.h 1.1 92/07/30 SMI; from S5R2 2.7	*/

#ifdef	comment	/* ======================================================= */

"Standard" header file definition.  See the C style paper for
full explanations.  Note that this reorganization has been
forced upon us by ANSI C; please abide by it.  This file, aside
from this comment, is supposed to be a good example.

/* sccs id stuff */

#ifndef	__[dir_]_filename_h
#define	__[dir_]_filename_h

#include	/* all includes come first */

#define		/* all defines other than function like macros */

structs
unions
enums

function prototypes

/*
 * macros that are supplied as C functions have to be defined after
 * the C prototype.
 */
#define		/* all function like macros */

extern variables

#endif	__[dir_]_filename_h

#endif	/* comment ======================================================= */

#ifndef	__stdio_h
#define	__stdio_h

#include <sys/stdtypes.h>	/* for size_t */

#if	pdp11
#define	BUFSIZ	512
#elif	u370
#define	BUFSIZ	4096
#else	/* just about every other UNIX system in existence */
#define	BUFSIZ	1024
#endif
#ifndef	EOF
#define	EOF		(-1)
#endif
#define	L_ctermid	9
#define	L_cuserid	9
#define	L_tmpnam	25		/* (sizeof (_tmpdir) + 15) */
#define	_tmpdir		"/usr/tmp/"
#define	FILENAME_MAX	1025
#define	TMP_MAX		17576

/*
 * ANSI C requires definitions of SEEK_CUR, SEEK_END, and SEEK_SET here.
 * They must be kept in sync with SEEK_* in <sys/unistd.h> (as required
 * by POSIX.1) and L_* in <sys/file.h>.
 * FOPEN_MAX should follow definition of _POSIX_OPEN_MAX in <sys/unistd.h>.
 */

#ifndef SEEK_SET
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

#define	FOPEN_MAX	16

#ifndef	_POSIX_SOURCE
#define	P_tmpdir	_tmpdir
#endif
#ifndef	NULL
#define	NULL		0
#endif
#define	stdin		(&_iob[0])
#define	stdout		(&_iob[1])
#define	stderr		(&_iob[2])
/*
 * _IOLBF means that a file's output will be buffered line by line
 * In addition to being flags, _IONBF, _IOLBF and _IOFBF are possible
 * values for "type" in setvbuf.
 */
#define	_IOFBF		0000
#define	_IOREAD		0001
#define	_IOWRT		0002
#define	_IONBF		0004
#define	_IOMYBUF	0010
#define	_IOEOF		0020
#define	_IOERR		0040
#define	_IOSTRG		0100
#define	_IOLBF		0200
#define	_IORW		0400
/*
 * buffer size for multi-character output to unbuffered files
 */
#define	_SBFSIZ 8

typedef	struct {
#if	pdp11 || u370
	unsigned char	*_ptr;
	int	_cnt;
#else	/* just about every other UNIX system in existence */
	int	_cnt;
	unsigned char	*_ptr;
#endif
	unsigned char	*_base;
	int	_bufsiz;
	short	_flag;
	unsigned char	_file;	/* should be short */
} FILE;

#ifndef	_POSIX_SOURCE
extern char	*ctermid(/* char *s */);	/* unistd.h */
extern char	*cuserid(/* char *s */);	/* unistd.h */
extern FILE	*popen(/* char *prog, char *mode */);
extern char	*tempnam(/* char *d, char *s */);
#endif

extern void	clearerr(/* FILE *stream */);
extern int	fclose(/* FILE *f */);
extern FILE	*fdopen(/* int fd, char *type */);
extern int	feof(/* FILE *f */);
extern int	ferror(/* FILE *f */);
extern int	fflush(/* FILE *f */);
extern int	fgetc(/* FILE *f */);
extern int	fileno(/* FILE *f */);
extern FILE	*fopen(/* const char *path, const char *mode */);
extern char	*fgets(/* char *s, int n, FILE *f */);
extern int	fprintf(/* FILE *f, const char *fmt, ... */);
extern int	fputc(/* int c, FILE *f */);
extern int	fputs(/* const char *s, FILE *f */);
extern size_t	fread(/* void *ptr, size_t size, size_t nmemb, FILE *f */);
extern FILE	*freopen(/* const char *filename, const char *mode, FILE *f */);
extern int	fscanf(/* FILE *f, const char *format, ... */);
extern int	fseek(/* FILE *f, long int offset, int whence */);
extern long	ftell(/* FILE *f */);
extern size_t	fwrite(/* const void *p, size_t size, size_t nmemb, FILE *f */);
extern int	getc(/* FILE *f */);
extern int	getchar(/* void */);
extern char	*gets(/* char *s */);
extern void	perror(/* const char *s */);
extern int	printf(/* const char *fmt, ... */);
extern int	putc(/* int c, FILE *f */);
extern int	putchar(/* int c */);
extern int	puts(/* const char *s */);
extern int	remove(/* const char *filename */);
extern int	rename(/* char *old, char *new */);
extern void	rewind(/* FILE *f */);
extern int	scanf(/* const char *format, ... */);
extern void	setbuf(/* FILE *f, char *b */);
extern int	sprintf(/* char *s, const char *fmt, ... */);
extern int	sscanf(/* const char *s, const char *format, ... */);
extern FILE	*tmpfile(/* void */);
extern char	*tmpnam(/* char *s */);
extern int	ungetc(/* int c, FILE *f */);

#ifndef	lint
#define	getc(p)		(--(p)->_cnt >= 0 ? ((int) *(p)->_ptr++) : _filbuf(p))
#define	putc(x, p)	(--(p)->_cnt >= 0 ?\
	(int)(*(p)->_ptr++ = (unsigned char)(x)) :\
	(((p)->_flag & _IOLBF) && -(p)->_cnt < (p)->_bufsiz ?\
		((*(p)->_ptr = (unsigned char)(x)) != '\n' ?\
			(int)(*(p)->_ptr++) :\
			_flsbuf(*(unsigned char *)(p)->_ptr, p)) :\
		_flsbuf((unsigned char)(x), p)))
#define	getchar()	getc(stdin)
#define	putchar(x)	putc((x), stdout)
#define	clearerr(p)	((void) ((p)->_flag &= ~(_IOERR | _IOEOF)))
#define	feof(p)		(((p)->_flag & _IOEOF) != 0)
#define	ferror(p)	(((p)->_flag & _IOERR) != 0)
#define	fileno(p)	(p)->_file
#endif

extern FILE	_iob[];

#endif	/* !__stdio_h */
