#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)mount.c 1.1 92/07/30 SMI";
#endif

#include <sys/types.h>
#include <sys/mount.h>

int
mount(spec, dir, flags)
	char *spec;
	char *dir;
	int flags;
{
	struct ufs_args args;

	args.fspec = spec;
	return(_mount("4.2", dir, flags|M_NEWTYPE|M_SYS5, (caddr_t) &args));
}
