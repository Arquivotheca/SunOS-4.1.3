#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)main.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool
 */

#include <stdio.h>
#include <locale.h>
#include <sys/syscall.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sunwindow/window_hs.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/walkmenu.h>

#include "glob.h"
#include "tool.h"

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
        (dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

char    *mktemp(), *strcpy(), *sprintf();

int	mt_aborting;		/* aborting, don't save messages */
char	*mt_cmdname;		/* our name */
int	mt_mailclient;		/* client handle */

char	mt_hdrfile[32];
char	mt_msgfile[32];
char	mt_scratchfile[32];
char	mt_printfile[32];
char	mt_dummybox[32];
char    mt_clipboard[32];

static 	void usage();


#ifdef STANDALONE
main(argc, argv)
#else
mailtool_main(argc, argv)
#endif
	int	argc;
	char	**argv;
{
	int	i;
	char	*p;
	extern char    *index();

	(void) setlocale(LC_CTYPE, "");

	mt_init_mail_storage();
	mt_init_tool_storage();
	for (i = GETDTABLESIZE(); i > 2; i--)
		(void)close(i);
	mt_cmdname = argv[0];
	argc--;
	argv++;

	/*
	 * Determine user's mailbox.
	 */
	if (getenv("MAIL"))
		(void)strcpy(mt_mailbox, getenv("MAIL"));
	else
		(void)sprintf(mt_mailbox, "/usr/spool/mail/%s", getenv("USER"));
		/* XXX */

	if (mt_init_tmpfiles() < 0)
		return;
	if (!mt_start_mail())
		EXIT(1);
	mt_get_vars(1);
		
	if (!mt_mail_seln_exists())
		EXIT(1);
	mt_start_tool(argc, argv);
	mt_done(0);
	mt_release_mail_storage();
	
	EXIT(0);
}

void
mt_parse_tool_args (argc, argv)
	int	argc;
	char	**argv;
{
	char	*iv = NULL;
	int	expert = 0;

	while (argc > 0) {
		if (argv[0][0] == '-') {
			switch (argv[0][1]) {
			case 'M':
				/* all mailtool switches will begin with -M */
				if (argv[0][3] != '\0')
					goto error;
				switch (argv[0][2]) {
				case 'x':
					expert++;
					break;
				case 'i':
					if (argc < 2)
						goto error;
					iv = argv[1];
					argc--;
					argv++;
					break;
/*				case 'f':
					if (argc < 2)
						goto error;
					mt_load_from_folder = argv[1];
					argc--;
					argv++;
					break;
				case 'l':
					if (argc < 2)
						goto error;
					else if (strcmp(argv[1], "opened") == 0)
						mt_load_messages =
							mt_When_Opened;
					else if (strcmp(argv[1], "startup") == 0)
						mt_load_messages =
							mt_At_Startup;
					else  if (strcmp(argv[1], "asked") == 0)
						mt_load_messages =
							mt_When_Asked;
					else
						goto error;
					argc--;
					argv++;
					break;
*/				case 'd':
					mt_debugging = TRUE;
					break;
				default: goto error;
				}
				break;
			case 'x':	/* for backwards compatibility */
				if (argv[0][2] != '\0')
					goto error;
				expert++;
				break;
			case 'i':	/* for backwards compatibility */
				if (argv[0][2] != '\0')
					goto error;
				else if (argc < 2)
					goto error;
				iv = argv[1];
				argc--;
				argv++;
				break;
			default:
			error:
				usage(mt_cmdname);
			}
		} else {
			usage(mt_cmdname);
		}
		argc--;
		argv++;
	}
	if (iv)
		mt_assign("interval", iv);
	if (expert)
		mt_assign("expert", "");

}

static void
usage(name)
	char	*name;
{
	(void)fprintf(stderr, "Usage: %s [-Mx] [-Mi interval] [tool args]\n", name);
	exit(1);
}

static int
mt_init_tmpfiles()
{
	int	f;

	(void) strcpy(mt_hdrfile, 	"/tmp/MThXXXXXX");
	(void) strcpy(mt_msgfile, 	"/tmp/MTmXXXXXX");
	(void) strcpy(mt_scratchfile, 	"/tmp/MTsXXXXXX");
	(void) strcpy(mt_printfile, 	"/tmp/MTpXXXXXX");
	(void) strcpy(mt_dummybox, 	"/tmp/MTdXXXXXX");
	(void) mktemp(mt_hdrfile);
	(void) mktemp(mt_msgfile);
	(void) mktemp(mt_scratchfile);
	(void) mktemp(mt_printfile);
	(void) mktemp(mt_dummybox);

	(void) strcpy(mt_clipboard,     "/tmp/MTcXXXXXX");
	if ( ((f = mkstemp(mt_clipboard)) == -1) ||
	     (close(f) == -1)
	   )
 	  (void)fprintf(stderr, "Unable to create temporary clipboard file.\n");

	return(0);
}

void
mt_done(i)
	int	i;
{
	struct reply_panel_data *ptr;

	(void) unlink(mt_hdrfile);
	(void) unlink(mt_msgfile);
	if (mt_cmdpanel)
		ptr = (struct reply_panel_data *)panel_get(
			mt_cmdpanel, PANEL_CLIENT_DATA);
	while (ptr) {
		if (ptr->replysw_file)
			unlink(ptr->replysw_file);
		ptr=ptr->next_ptr;
	}
	(void) unlink(mt_scratchfile);
	(void) unlink(mt_printfile);
	(void) unlink(mt_dummybox);
	(void) unlink(mt_clipboard);
	
	exit(i);
}
