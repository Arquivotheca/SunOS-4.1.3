/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appears in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)rawddsubs.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <strings.h>
#include <stdio.h>

int 
_core_getfb(dev, type, winsys)
    char *dev;
    int type, winsys;
{
    char *buf, *malloc();
    int len;
    register int fd = -1, i;

    if (!dev ||
	!(buf = malloc((unsigned) ((len = strlen(dev)) + 1))))
	return fd;
    bcopy(buf, dev, len);

    for (i = 0; fd < 0; i++) {
	if (type != FBTYPE_SUN2GP) {
	    if (i > 9)
		break;
	    buf[len - 1] = i + '0';
	}
	else {
	    if (i > 15)
		break;
	    buf[len - 2] = (i >> 2) + '0';
	    buf[len - 1] = (i & 3) + 'a';
	}
	fd = _core_chkdev(buf, type, winsys);
    }

    return fd;
}


static char *devchk;
static int devisroot;

int 
_core_chkdev(dev, type, winsys)
    char *dev;
    int type, winsys;
{
    int fbfd, fbtype;
    int chkdeviswindowroot();

    if (!dev)
    	return -1;

    if ((fbfd = open(dev, O_RDWR, 0)) < 0)
	return (-1);
    if (type >= 0 &&
	((fbtype = pr_getfbtype_from_fd(fbfd)) == -1 || fbtype != type)) {
	close(fbfd);
	return (-1);
    }
    close(fbfd);
    if (winsys) {		/* check for already used in window system */
	devchk = dev;
	devisroot = FALSE;
	win_enumall(chkdeviswindowroot);
	if (devisroot)
	    return (-1);
    }
    return (fbfd);
}

static int 
chkdeviswindowroot(windowfd)
    int windowfd;
{
    struct screen windowscreen;

    win_screenget(windowfd, &windowscreen);
    if (strcmp(devchk, windowscreen.scr_fbname) == 0) {
	devisroot = TRUE;
	return (TRUE);
    }
    return (FALSE);
}

static struct screen *nbrptr;
static int foundnbrs, allnbrs;

_core_setadjacent(filename, devscreen)
    char *filename;
    struct screen *devscreen;
{
    FILE *adjfile;
    char devname[SCR_NAMESIZE], dir[SCR_NAMESIZE];
    char c, *cptr;
    int i;
    struct screen neighbors[4];
    int getneighbors();

    if ((adjfile = fopen(filename, "r")) == NULL)
	return (-1);
    for (nbrptr = neighbors; nbrptr < &neighbors[4]; nbrptr++) {
	*nbrptr->scr_fbname = '\0';
	*nbrptr->scr_rootname = '\0';
    }
    allnbrs = 0;
    while (fscanf(adjfile, "%[^/]%s", dir, devname) == 1) {
	if (strcmp(devscreen->scr_fbname, devname)) {
	    while (fscanf(adjfile, "%[^:/]:%s", dir, devname) == 2);
	    continue;
	}
	while (fscanf(adjfile, "%[^:/]:%s", dir, devname) == 2) {
	    for (cptr = dir; (c = *cptr++) != 0 && c <= ' ';);
	    i = 0;
	    switch (c) {
		case 'W':
		case 'L':
		    i++;
		case 'S':
		case 'B':
		    i++;
		case 'E':
		case 'R':
		    i++;
		case 'N':
		case 'T':
		    break;
		default:
		    continue;
	    }
	    strncpy(neighbors[i].scr_fbname, devname, SCR_NAMESIZE);
	    allnbrs |= (1 << i);
	}
	break;
    }
    fclose(adjfile);
    nbrptr = neighbors;
    foundnbrs = 0;
    win_enumall(getneighbors);
    for (; nbrptr < &neighbors[4]; nbrptr++)
	if (nbrptr->scr_rootname == '\0')
	    win_initscreenfromargv(nbrptr, (char **) 0);
    win_screenadj(devscreen, neighbors, TRUE);

    return (0);
}

static int 
getneighbors(windowfd)
    int windowfd;
{
    struct screen windowscreen, *scrptr;
    int i;

    win_screenget(windowfd, &windowscreen);
    scrptr = nbrptr;
    for (i = 0; i < 4; i++) {
	if ((*scrptr->scr_rootname == '\0') &&
	    (strcmp(scrptr->scr_fbname, windowscreen.scr_fbname) == 0)) {
	    *scrptr = windowscreen;
	    foundnbrs |= (1 << i);
	}

	/*
	 * Can't break after finding one adjacency for this windowfd, since
	 * user may specify two screens adjacent on more than one edge. 
	 */
	scrptr++;
    }
    return (foundnbrs == allnbrs);
}
