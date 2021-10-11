#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)audit_args.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>

audit_args(event, argc, argv)
	int event;
	int argc;
	char *argv[];
{
	return (audit_text(event, -1, -1, argc, argv));
}

audit_text(event, error, returns, argc, argv)
	int event;
	int error;
	int returns;
	int argc;
	char *argv[];
{
	audit_record_t *ap;
	int size = sizeof (audit_record_t) + sizeof (short) * (argc + 1);
	int len;
	int vsize = 0;
	int i;
	int groups[NGROUPS];
	short *sp;
	char *cp;
	char *malloc();
	char *realloc();

	/*
	 * read the group list
	 */
	if ((vsize = getgroups(NGROUPS, groups)) < 0)
		return (-1);
	vsize *= sizeof (int);
	/*
	 * allocate memory for the header and length fields
	 */
	if (!(ap = (audit_record_t *)malloc(size)))
		return (-1);
	sp = (short *)(ap + 1);
	*sp++ = vsize;
	/*
	 * gather up the parameters into a hunk of memory
	 * start with the groups field
	 */
	cp = malloc(vsize);
	bcopy((char *)groups, cp, vsize);
	for ( i = 0; i < argc; i++ ) {
		len = strlen( *argv ) + 1;
		if (len & 1)
			len++;
		if (!(cp = realloc(cp, vsize + len)))
			return (-1);
		strcpy(cp + vsize, *argv);
		vsize += len;
		*sp++ = len;
		argv++;
	}
	/*
	 * build the final structure
	 */
	if (!(ap = (audit_record_t *) realloc(ap, size + vsize)))
		return (-1);
	bcopy(cp, ((char *)(ap+1)) + (sizeof (short) * (argc + 1)), vsize);
	free(cp);
	ap->au_param_count = argc + 1; /* + 1 because of group list */
	ap->au_record_size = size + vsize;
	ap->au_record_type = AUR_TEXT;
	ap->au_event = event;
	ap->au_auid = getauid();
	ap->au_uid = getuid();
	ap->au_euid = geteuid();
	ap->au_gid = getgid();
	ap->au_pid = getpid();
	ap->au_errno = error;
	ap->au_return = returns;
	getplabel(&ap->au_label);

	audit(ap);
	free(ap);
	return (0);
}

/*
 * Processes don't have significant labels, so this zeroes that field
 * We will have real labels one day.
 */
getplabel(b)
	blabel_t *b;
{
	bzero((char *)b, sizeof(blabel_t));
}

