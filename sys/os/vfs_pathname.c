/*	@(#)vfs_pathname.c 1.1 92/07/30 SMI 	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/pathname.h>
#include <sys/dirent.h>

/*
 * Pathname utilities.
 *
 * In translating file names we copy each argument file
 * name into a pathname structure where we operate on it.
 * Each pathname structure can hold MAXPATHLEN characters
 * including a terminating null, and operations here support
 * allocating and freeing pathname structures, fetching
 * strings from user space, getting the next character from
 * a pathname, combining two pathnames (used in symbolic
 * link processing), and peeling off the first component
 * of a pathname.
 */

static char *pn_freelist;
static u_int pn_freelist_size;
static u_int pn_freelist_max = 2;

/*
 * Allocate contents of pathname structure.
 * Structure itself is typically automatic
 * variable in calling routine for convenience.
 */
pn_alloc(pnp)
	register struct pathname *pnp;
{
	if (pn_freelist != NULL) {
		pnp->pn_buf = pn_freelist;
		pn_freelist = *(char **)pnp->pn_buf;
		--pn_freelist_size;
	} else {
		pnp->pn_buf = (char *)new_kmem_alloc(
					(u_int)MAXPATHLEN, KMEM_SLEEP);
	}
	pnp->pn_path = (char *)pnp->pn_buf;
	pnp->pn_pathlen = 0;
}

/*
 * Pull a pathname from user user or kernel space
 */
int
pn_get(str, seg, pnp)
	register char *str;
	int seg;
	register struct pathname *pnp;
{
	register int error;

	pn_alloc(pnp);
	if (seg == UIO_USERSPACE) {
		error =
		    copyinstr(str, pnp->pn_path, MAXPATHLEN, &pnp->pn_pathlen);
	} else {
		error =
		    copystr(str, pnp->pn_path, MAXPATHLEN, &pnp->pn_pathlen);
	}
	pnp->pn_pathlen--;		/* don't count null byte */
	if (error)
		pn_free(pnp);
	return (error);
}

#ifdef notneeded
/*
 * Get next character from a path.
 * Return null at end forever.
 */
pn_getchar(pnp)
	register struct pathname *pnp;
{

	if (pnp->pn_pathlen == 0)
		return (0);
	pnp->pn_pathlen--;
	return (*pnp->pn_path++);
}
#endif notneeded

/*
 * Set pathname to argument string.
 */
pn_set(pnp, path)
	register struct pathname *pnp;
	register char *path;
{
	register int error;

	pnp->pn_path = pnp->pn_buf;
	error = copystr(path, pnp->pn_path, MAXPATHLEN, &pnp->pn_pathlen);
	pnp->pn_pathlen--;		/* don't count null byte */
	return (error);
}

/*
 * Combine two argument pathnames by putting
 * second argument before first in first's buffer,
 * and freeing second argument.
 * This isn't very general: it is designed specifically
 * for symbolic link processing.
 */
pn_combine(pnp, sympnp)
	register struct pathname *pnp;
	register struct pathname *sympnp;
{

	if (pnp->pn_pathlen + sympnp->pn_pathlen >= MAXPATHLEN)
		return (ENAMETOOLONG);
	ovbcopy(pnp->pn_path, pnp->pn_buf + sympnp->pn_pathlen,
	    (u_int)pnp->pn_pathlen);
	bcopy(sympnp->pn_path, pnp->pn_buf, (u_int)sympnp->pn_pathlen);
	pnp->pn_pathlen += sympnp->pn_pathlen;
	pnp->pn_path = pnp->pn_buf;
	return (0);
}

/*
 * Get next component off a pathname and leave in
 * buffer comoponent which should have room for
 * MAXNAMLEN bytes and a null terminator character.
 * If PEEK is set in flags, just peek at the component,
 * i.e., don't strip it out of pnp.
 */
pn_getcomponent(pnp, component, flags)
	register struct pathname *pnp;
	register char *component;
	int flags;
{
	register char *cp;
	register int l;
	register int n;

	cp = pnp->pn_path;
	l = pnp->pn_pathlen;
	n = MAXNAMLEN;
	while ((l > 0) && (*cp != '/')) {
		if (--n < 0)
			return (ENAMETOOLONG);
		*component++ = *cp++;
		--l;
	}
	if (!(flags & PN_PEEK)) {
		pnp->pn_path = cp;
		pnp->pn_pathlen = l;
	}
	*component = 0;
	return (0);
}

/*
 * skip over consecutive slashes in the pathname
 */
void
pn_skipslash(pnp)
	register struct pathname *pnp;
{
	while ((pnp->pn_pathlen != 0) && (*pnp->pn_path == '/')) {
		pnp->pn_path++;
		pnp->pn_pathlen--;
	}
}

/*
 * Free pathname resources.
 */
void
pn_free(pnp)
	register struct pathname *pnp;
{

	if (pn_freelist_size >= pn_freelist_max)
		kmem_free((caddr_t)pnp->pn_buf, (u_int)MAXPATHLEN);
	else {
		*(char **)pnp->pn_buf = pn_freelist;
		pn_freelist = pnp->pn_buf;
		pnp->pn_buf = 0;
		++pn_freelist_size;
	}
}

/*
 * Append the given path string to the given pathname structure.
 */
int
pn_append(pnp, sp)
register struct pathname *pnp;
register char *sp;
{
	u_int alen;
	register char *pp = &pnp->pn_path[pnp->pn_pathlen];
	int error = 0;

	if (*sp == '\0')
		return (0);
	if (pnp->pn_pathlen == 0)
		return (pn_set(pnp, sp));
	if ((pp - pnp->pn_buf) >= MAXPATHLEN)
		return (ENAMETOOLONG);
	if (*(pp-1) != '/' && *sp != '/') {
		*pp++ = '/';
		*pp = '\0';
		pnp->pn_pathlen++;
	}
	error = copystr(sp, pp, (u_int)(MAXPATHLEN -(pp - pnp->pn_buf)), &alen);
	if (error)
		return (error);
	pnp->pn_pathlen += alen - 1;
	return (0);
}

/*
 * Remove the last component of the given pathname and return it in comp
 */
int
pn_getlast(pnp, comp)
struct pathname *pnp;
char *comp;
{
	register char *pp = &pnp->pn_path[pnp->pn_pathlen];
	int error;

	while (pp >= pnp->pn_path && *pp == '/')
		*pp-- = '\0';
	while (pp >= pnp->pn_path && *pp != '/')
		pp--;
	pp++;
	if (error = copystr(pp, comp, MAXNAMLEN, (u_int *) NULL))
		return (error);
	while (pp >= pnp->pn_path && *pp != '/')
		pp--;
	while (pp >= pnp->pn_path && *pp == '/')
		*pp-- = '\0';
	*++pp = '\0';
	pnp->pn_pathlen = pp - pnp->pn_path;
	return (0);
}
