#include <sys/types.h>
#include <sys/uio.h>

uio
./"iovcnt"16t"offset"16t"seg"8t"resid"n{uio_iovcnt,D}{uio_offset,D}{uio_seg,d}{uio_resid,D}
.>f
{*uio_iov,<f},{*uio_iovcnt,<f}$<iovec
