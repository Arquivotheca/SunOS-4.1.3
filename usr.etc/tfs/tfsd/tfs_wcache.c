#ifndef lint
static char sccsid[] = "@(#)tfs_wcache.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/util.h>
#include "headers.h"
#include "tfs.h"
#include "vnode.h"
#include "subr.h"

/*
 * Routines to maintain a write cache, so that individual tfs_writes of
 * size less than one block (8K) are bundled together and written as 8K
 * chunks.  This cache will no longer be necessary when the TFS can have a
 * UDP buffer size greater than 9K, in which case the write size to the TFS
 * can be set to 8K.  This cache is necessary for performance in the
 * meantime because each tfs_write of 2K, say, is flushed from the buffer
 * cache due to a bug in the NFS client code.
 *
 * The write cache sets the mtime of a file to the current time on the local
 * machine.  Consequently, other routines which set the mtime of a file (e.g.
 * tfs_create() when no subsequent writes are done, and tfs_setattr() for
 * truncate) must also use the write cache to ensure that mtimes are always
 * updated consistently.
 */

#define MAX_CACHE_ELEM	5		/* Max.  # of elements in cache.
					 * Should be >= the number of
					 * biod's running on the system.
					 */
#define WRITE_TIMEOUT 5			/* Time that a write buffer allowed
					 * to stay in the cache. */

#define	BLOCK_MASK	(NFS_MAXDATA - 1)
#define ANY_BLOCK	-1


typedef struct Write_cache {
	struct pnode	*pnode;
	long		block;		/* Starting offset of block */
	int		offset;		/* Offset within block */
	int		count;		/* # of chars in buffer */
	char		*buffer;
	long		sync_time;	/* When this block should be
					 * sync'ed with disk */
} *Write_cache, Write_cache_rec;


static Write_cache_rec write_cache[MAX_CACHE_ELEM];
static int	write_timeout;


int		write_wcache();
int		read_wcache();
bool_t		timeout_wcache();
void		sync_entire_wcache();
void		sync_wcache();
void		flush_wcache();
static void	init_wcache();
static Write_cache find_unused_wcache_block();
static int	write_wcache_block();
static void	set_mtime();
static Write_cache find_wcache_block();


static bool_t	wcache_on;

int
write_wcache(pp, buf, length, offset)
	struct pnode	*pp;
	char		*buf;
	int		length;
	long		offset;
{
	Write_cache	wc;
	int		block;
	int		block_offset;

	if (!wcache_on) {
		init_wcache();
		wcache_on = TRUE;
	}
	/*
	 * Always make sure that there is an open fd for a file in the
	 * write wcache.  (Some routines rely on close_fd() to clean up
	 * the write cache.)
	 */
	if (pp->p_fd == 0 && open_fd(pp, O_RDWR, 0) < 0) {
		return (errno);
	}
	block = (offset & ~BLOCK_MASK);
	block_offset = (offset & BLOCK_MASK);
	wc = find_wcache_block(pp, block);
	if (wc == NULL) {
		wc = find_unused_wcache_block();
		wc->pnode = pp;
		wc->block = block;
		wc->offset = block_offset;
		/*
		 * Note that wc->offset isn't always 0 because
		 * find_unused_block() may flush a buffer before it has
		 * been completely filled.
		 */
		wc->count = 0;
		wc->sync_time = 0;
		pp->p_needs_write = 1;
	} else if (block_offset < wc->offset) {
		wc->offset = block_offset;
	}
	if (length > 0) {
		BCOPY(buf, wc->buffer + block_offset, length);
	}
	if (block_offset >= wc->count) {
		wc->count += length;
	} else {
		/*
		 * Receiving a write request that is overwriting previous
		 * request.  Assuming here that writes arrive sequentially
		 * within an 8K block.
		 */
		wc->count = block_offset + length;
	}
	pp->p_write_time = get_current_time(TRUE);
	if (wc->offset + wc->count == NFS_MAXDATA) {
		/*
		 * Sync the block with disk immediately if the block is
		 * full.
		 */
		length = write_wcache_block(wc, FALSE);
	} else {
		wc->sync_time = pp->p_write_time + write_timeout;
	}
	return (length);
}


/*
 * Try to read data from write cache.  Returns -1 if the data is not in the
 * write cache.
 */
int
read_wcache(pp, buf, length, offset)
	struct pnode	*pp;
	char		*buf;
	int		length;
	long		offset;
{
	Write_cache     wc;
	int             block;
	int		block_offset;
	int		len;

	block = (offset & ~BLOCK_MASK);
	block_offset = (offset & BLOCK_MASK);
	wc = find_wcache_block(pp, block);
	if (wc == NULL) {
		return (-1);
	}
	if (wc->offset > 0) {
		(void) write_wcache_block(wc, TRUE);
		return (-1);
	}
	len = MIN(wc->count - block_offset, length);
	BCOPY(wc->buffer + block_offset, buf, len);
	return (len);
}


/*
 * Called from alarm handler.  Sync's all blocks in the write cache which
 * have been there too long.
 */
bool_t
timeout_wcache(now)
	long		now;
{
	struct pnode	*pp;
	int		i;
	bool_t		more_to_sync = FALSE;

	if (!wcache_on) {
		return (FALSE);
	}
	for (i = 0; i < MAX_CACHE_ELEM; i++) {
		if (write_cache[i].pnode != NULL) {
			if (write_cache[i].sync_time < now) {
				pp = write_cache[i].pnode;
#ifdef TFSDEBUG
{
	char		pn[MAXPATHLEN];

	ptopn(pp, pn);
	dprint(tfsdebug, 3, "syncing block %u of %s (sync_time %u)\n",
		write_cache[i].block, pn, write_cache[i].sync_time);
}
#endif TFSDEBUG
				(void) write_wcache_block(&write_cache[i],
							  TRUE);
				if (!pp->p_needs_write && pp->p_fd != 0) {
					close_fd(pp);
				}
			} else {
				more_to_sync = TRUE;
			}
		}
	}
	return (more_to_sync);
}


void
sync_entire_wcache()
{
	int		i;

	for (i = 0; i < MAX_CACHE_ELEM; i++) {
		if (write_cache[i].pnode != NULL) {
			(void) write_wcache_block(&write_cache[i], TRUE);
		}
	}
}


/*
 * Synchronize file 'pp's in-core state with that on disk.  Write out all
 * blocks for 'pp' which are in the write cache.
 */
void
sync_wcache(pp)
	struct pnode	*pp;
{
	Write_cache	wc;

	while ((wc = find_wcache_block(pp, ANY_BLOCK)) != NULL) {
		(void) write_wcache_block(wc, TRUE);
	}
}


/*
 * Flush all blocks for the file with pnode 'pp' from the write cache
 * (don't write them to the disk.)
 */
void
flush_wcache(pp)
	struct pnode	*pp;
{
	Write_cache	wc;

	while ((wc = find_wcache_block(pp, ANY_BLOCK)) != NULL) {
		wc->pnode = NULL;
	}
	pp->p_needs_write = 0;
}


static void
init_wcache()
{
	int		i;

	write_timeout = WRITE_TIMEOUT;
	for (i = 0; i < MAX_CACHE_ELEM; i++) {
		write_cache[i].pnode = NULL;
		write_cache[i].buffer = nse_malloc(NFS_MAXDATA);
	}
}


static Write_cache
find_unused_wcache_block()
{
	static int	block_to_sync;
	int		block_num = -1;
	int		i;

	/*
	 * Find an unused wcache block to use, or use one which has an uneven
	 * count (a block with (size mod 2K != 0) will probably not receive any
	 * more write requests.)
	 */
	for (i = 0; i < MAX_CACHE_ELEM; i++) {
		if (write_cache[i].pnode == NULL) {
			block_num = i;
			break;
		}
		if ((write_cache[i].count & 03777) != 0) {
			(void) write_wcache_block(&write_cache[i], TRUE);
			block_num = i;
			break;
		}
	}
	if (block_num == -1) {
#ifdef TFSDEBUG
dprint(tfsdebug, 3, "find_unused_wcache_block: no buffers free!\n");
print_wcache();
#endif TFSDEBUG
		block_num = block_to_sync;
		(void) write_wcache_block(&write_cache[block_to_sync], TRUE);
		block_to_sync++;
		if (block_to_sync == MAX_CACHE_ELEM) {
			block_to_sync = 0;
		}
	}
	return (&write_cache[block_num]);
}


#ifdef TFSDEBUG
print_wcache()
{
	Write_cache	wc;
	int		i;

	for (i = 0; i < MAX_CACHE_ELEM; i++) {
		wc = &write_cache[i];
		if (wc->pnode != NULL) {
			dprint(tfsdebug, 3,
				"fd %d  block %d  offset %d  count %d\n",
				wc->pnode->p_fd, wc->block, wc->offset,
				wc->count);
		}
	}
}
#endif TFSDEBUG


static int
write_wcache_block(wc, delayed)
	Write_cache	wc;
	bool_t		delayed;
{
	struct pnode	*pp;
	int		count;

	pp = wc->pnode;
	if (wc->count > 0) {
		count = rdwr_fd(pp, wc->buffer + wc->offset, wc->count,
				wc->block + wc->offset, UIO_WRITE);
	} else {
		count = 0;
	}
	wc->pnode = NULL;
	if (delayed && count == -1) {
		pp->p_write_error = errno;
		flush_wcache(pp);
		return (-1);
	}
	if (find_wcache_block(pp, ANY_BLOCK) == NULL) {
		set_mtime(pp);
		pp->p_needs_write = 0;
		flush_cached_attrs(pp);
	}
	return (count);
}


/*
 * This routine is called to set the mtime of file 'pp' to the time that
 * we've given as the mtime in the file's attributes.  This routine is
 * usually called when write caching causes a delayed write to occur, but
 * can also be called when there is no data to be written, but the mtime
 * of the file needs to be updated.
 */
static void
set_mtime(pp)
	struct pnode	*pp;
{
	struct pnode	*parentp;
	struct stat	statb;
	struct timeval	file_time[2];
	char		name[MAXNAMLEN];
	int		result;

	if (fstat((int) pp->p_fd, &statb) < 0) {
		print_warning_msg(pp, "set_mtime: fstat");
		return;
	}
	if (statb.st_mtime == pp->p_write_time) {
		return;
	}
	file_time[0].tv_sec = statb.st_atime;
	file_time[0].tv_usec = 0;
	file_time[1].tv_sec = pp->p_write_time;
	file_time[1].tv_usec = 0;
	/*
	 * Since close_fd() may call this routine, we can't open any fd's
	 * here (we would start recursing between open_fd() and close_fd().)
	 * So fchdir to an fd only if one is available.
	 */
	parentp = PARENT_PNODE(pp);
	if (parentp->p_fd != 0 && change_to_dir(parentp) == 0) {
		strcpy(name, pp->p_name);
	} else {
		ptopn(pp, name);
	}
	result = utimes_as_owner(name, file_time, (int) statb.st_uid);
	if (result) {
		errno = result;
		print_warning_msg(pp, "set_mtime: utimes");
	}
}


/*
 * Find entry in write cache with pnode 'pp' and block number 'block'.  If
 * 'block' is ANY_BLOCK, then find any entry for pnode 'pp'.
 */
static Write_cache
find_wcache_block(pp, block)
	struct pnode	*pp;
	int		block;
{
	int		i;

	for (i = 0; i < MAX_CACHE_ELEM; i++) {
		if (write_cache[i].pnode == pp) {
			if ((block == ANY_BLOCK) ||
			    (write_cache[i].block == block)) {
				return (&write_cache[i]);
			}
		}
	}
	return (NULL);
}
