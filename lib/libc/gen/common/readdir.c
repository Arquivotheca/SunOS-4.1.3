#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)readdir.c 1.1 92/07/30 SMI";
#endif

#include <sys/param.h>
#include <dirent.h>

/*
 * get next entry in a directory.
 */
struct dirent *
readdir(dirp)
	register DIR *dirp;
{
	register struct dirent *dp;
	int saveloc = 0;

next:
        if (dirp->dd_size != 0) {
                dp = (struct dirent *)&dirp->dd_buf[dirp->dd_loc];
                saveloc = dirp->dd_loc;   /* save for possible EOF */
                dirp->dd_loc += dp->d_reclen;
        }
        if (dirp->dd_loc >= dirp->dd_size)
                dirp->dd_loc = dirp->dd_size = 0;

        if (dirp->dd_size == 0  /* refill buffer */
          && (dirp->dd_size = getdents(dirp->dd_fd, dirp->dd_buf, dirp->dd_bsize)
             ) <= 0
           ) {
                if (dirp->dd_size == 0) /* This means EOF */
                        dirp->dd_loc = saveloc;  /* EOF so save for telldir */
                return (NULL);    /* error or EOF */
        }

        dp = (struct dirent *)&dirp->dd_buf[dirp->dd_loc];
	if (dp->d_reclen <= 0)
		return (NULL);
	if (dp->d_fileno == 0)
		goto next;
	dirp->dd_off = dp->d_off;
        return(dp);
}
