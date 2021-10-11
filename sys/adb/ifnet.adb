#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

ifnet
./"unit"8t"mtu"8t"flags"n{if_unit,d}{if_mtu,d}{if_flags,x}n"addrlist"n{if_addrlist,X}n"head"16t"tail"16t"len"16t"drops"n{if_snd.ifq_head,X}{if_snd.ifq_tail,X}{if_snd.ifq_len,D}{if_snd.ifq_drops,D}"ipack"16t"ierr"n{if_ipackets,D}{if_ierrors,D}n"opack"16t"oerr"16t"coll"n{if_opackets,D}{if_oerrors,D}{if_collisions,D}n"next"n{if_next,X}n
