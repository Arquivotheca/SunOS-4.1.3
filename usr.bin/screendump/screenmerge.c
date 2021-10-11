#ifndef lint
static char sccsid[] = "@(#)screenmerge.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

main(argc, argv)
	int argc;
	char *argv[];
{
	extern screendump_main();
	extern screenload_main();
	extern rastrepl_main();
	extern rasfilter8to1_main();
	extern clear_colormap_main();

	static struct cmdtab {
		char *name;
		int (*main)();
	} cmdtab[] = {
		"screendump",		screendump_main,
		"screenload",		screenload_main,
		"rastrepl",		rastrepl_main,
		"rasfilter8to1",	rasfilter8to1_main,
		"clear_colormap",	clear_colormap_main,
		0
	};

	register struct cmdtab *p;
	char *name;

	extern char *basename();
	extern void error();

	name = basename(argv[0]);

	for (p = cmdtab; p->name; p++)
		if (strcmp(p->name, name) == 0)
			exit((*p->main)(argc, argv));

	error((char *) 0, "Cannot run program \"%s\" in merge file \"%s\"", 
		name, argv[0]);
	/* NOTREACHED */
}
