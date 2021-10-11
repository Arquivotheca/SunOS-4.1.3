/* memtestio.h */

#ifndef _memtestio_
#define _memtestio_

/*
 * Header file for memtest loadable driver.
 */

/*
 * memtest ioctl commands
 */
#define MEMTESTBADPAR		_IO(M, 0)
#define MEMTESTBADFETCH		_IO(M, 1)
#define MEMTESTBADLOAD		_IO(M, 2)
#define MEMTESTBADSTORE		_IO(M, 3)
#define MEMTESTBADLOADT		_IO(M, 4)
#define MEMTESTBADFETCHT	_IO(M, 5)
#define MEMTESTBADPAREXP	_IO(M, 6)

struct memtest_peekpoke {
	addr_t	paddr;		/* address to peek/poke */
	int error[6];		/* return values */
				/* The order of the return values is:
				 *	peekc, peek, peekl,
				 *	pokec, poke, pokel
				 */
};
#define MEMTESTPEEKPOKE		_IOWR(M, 6, struct memtest_peekpoke)

#endif _memtestio_
