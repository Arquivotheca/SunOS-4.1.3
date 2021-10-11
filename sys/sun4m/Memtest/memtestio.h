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

#endif _memtestio_
