#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>

rtentry
./"hash"16t{rt_hash,X}n"dst"16t{rt_dst,3X}n"gate"16t{rt_gateway,3X}n"flags"8t"refcnt"8t"use"8t"ifp"n{rt_flags,x}{rt_refcnt,d}{rt_use,D}{rt_ifp,X}
