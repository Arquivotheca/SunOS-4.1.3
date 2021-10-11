#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)openchild.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * openchild.c, Copyright (C) 1986 Sun Microsystems, Inc.
 *
 * Open two pipes to a child process, one for reading, one for writing.
 * The pipes are accessed by FILE pointers. This is NOT a public
 * interface, but for internal use only!
 */
#include <stdio.h>
#include <vfork.h>

extern char *malloc();
extern char *strrchr();
#ifndef	S5EMUL
extern char *sprintf();
#endif

static char *basename();
static char *SHELL = "/bin/sh";


/*
 * returns pid, or -1 for failure
 */
_openchild(command, fto, ffrom)
	char *command;
	FILE **fto;
	FILE **ffrom;
{
	int i;
	int pid;
	int pdto[2];
	int pdfrom[2];
	char *com;


	if (pipe(pdto) < 0) {
		goto error1;
	}
	if (pipe(pdfrom) < 0) {
		goto error2;
	}
	switch (pid = vfork()) {
	case -1:
		goto error3;

	case 0:
		/*
		 * child: read from pdto[0], write into pdfrom[1]
		 */
		(void) close(0);
		(void) dup(pdto[0]);
		(void) close(1);
		(void) dup(pdfrom[1]);
		for (i = _rpc_dtablesize() - 1; i >= 3; i--) {
			(void) close(i);
		}
		com = malloc((unsigned) strlen(command) + 6);
		if (com == NULL) {
			_exit(~0);
		}
		(void) sprintf(com, "exec %s", command);
		execl(SHELL, basename(SHELL), "-c", com, NULL);
		_exit(~0);

	default:
		/*
		 * parent: write into pdto[1], read from pdfrom[0]
		 */
		*fto = fdopen(pdto[1], "w");
		(void) close(pdto[0]);
		*ffrom = fdopen(pdfrom[0], "r");
		(void) close(pdfrom[1]);
		break;
	}
	return (pid);

	/*
	 * error cleanup and return
	 */
error3:
	(void) close(pdfrom[0]);
	(void) close(pdfrom[1]);
error2:
	(void) close(pdto[0]);
	(void) close(pdto[1]);
error1:
	return (-1);
}

static char *
basename(path)
	char *path;
{
	char *p;

	p = strrchr(path, '/');
	if (p == NULL) {
		return (path);
	} else {
		return (p + 1);
	}
}
