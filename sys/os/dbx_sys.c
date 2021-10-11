#ifndef lint
static	char sccsid[] = "@(#)dbx_sys.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>

#include <sys/acct.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/clist.h>
#include <sys/cmap.h>
#include <sys/conf.h>
#include <sys/core.h>
#include <sys/des.h>
#include <sys/dir.h>
#include <sys/dkbad.h>
#include <sys/dnlc.h>
#include <sys/domain.h>
#include <sys/file.h>
#include <sys/gprof.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mtio.h>
#include <sys/pathname.h>
#include <sys/protosw.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/text.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/tty.h>
#include <sys/ttychars.h>
#include <sys/ttydev.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/unpcb.h>
#include <sys/vfs.h>
#include <sys/vm.h>
#include <sys/vnode.h>
#include <sys/vtimes.h>
#include <sys/wait.h>
