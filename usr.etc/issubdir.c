/* @(#)issubdir.c 1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * Subdirectory detection: needed by exportfs and rpc.mountd.
 * The above programs call issubdir() frequently, so we make
 * it fast by caching the results of stat().
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>

#define MAXSTATS 20	/* maximum number of stat()'s to save */

#define inoeq(ino1, ino2)	((ino1) == (ino2))
#define deveq(dev1, dev2)	((dev1) == (dev2))

/*
 * dir1 is a subdirectory of dir2 within the same filesystem if
 *     (a) dir1 is identical to dir2
 *     (b) dir1's parent is dir2
 */
issubdir(dir1, dir2)
	char *dir1;
	char *dir2;
{
	struct stat st;
	char *p;
	int index;
	dev_t parent_dev;
	ino_t parent_ino;

	static dev_t child_dev;
	static ino_t child_ino[MAXSTATS];
	static int valid;
	static char childdir[MAXPATHLEN];

	/*
	 * Get parent directory info
	 */
	if (stat(dir2, &st) < 0) {
		return(0);
	}
	parent_dev = st.st_dev;
	parent_ino = st.st_ino;

	if (strcmp(childdir, dir1) != 0) {
		/*
	 	 * Not in cache: get child directory info
		 */
		p = strcpy(childdir, dir1) + strlen(dir1);
		index = 0;
		valid = 0;
		for (;;) {
			if (stat(childdir, &st) < 0) {
				childdir[0] = 0;	/* invalidate cache */
				return (0);
			}
			if (index == 0) {
				child_dev = st.st_dev;
			}
			if (index > 0 && 
			    (child_dev != st.st_dev ||
			     inoeq(child_ino[index - 1], st.st_ino))) {
				/*
				 * Hit root: done
				 */
				break;
			}
			child_ino[index++] = st.st_ino;
			if (st.st_mode & S_IFDIR) {
				p = strcpy(p, "/..") + 3;
			} else {
				p = strrchr(childdir, '/');
				if (p == NULL) {
					p = strcpy(childdir, ".") + 1;
				} else {
					while (&p[1] >childdir && p[1] == '/') {
						p--;
					}
					*p = 0;
				}
			}
		}
		valid = index;
		(void) strcpy(childdir, dir1);
	}

	/*
	 * Perform the test
	 */
	if (!deveq(parent_dev, child_dev)) {
		return (0);
	}
	for (index = 0; index < valid; index++) {
		if (inoeq(child_ino[index], parent_ino)) {
			return (1);
		}
	}
	return (0);
}
