#include <sys/param.h>
#include <sys/stream.h>
#include <sys/ttycom.h>
#include "../sys/tty.h"

tty_common
./n"flags"16t"readq"16t"writeq"n{t_flags,X}{t_readq,X}{t_writeq,X}n"iflag"16t"cflag"16t"stopc"8t"startc"n{t_iflag,X}{t_cflag,X}{t_stopc,b}{t_startc,b}n"rows"8t"cols"8t"xpixels"8t"ypixels"8t"ioc_pend"n{t_size.ws_row,d}{t_size.ws_col,d}{t_size.ws_xpixel,d}{t_size.ws_ypixel,d}{t_iocpending,X}
