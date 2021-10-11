#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

socket
./"type"8t"opt"8t"linger"8t"state"8t"pcb"8t8t"proto"n{so_type,x}{so_options,x}{so_linger,d}{so_state,x}{so_pcb,X}{so_proto,X}n"head"16t"q0"16t"q0len"n{so_head,X}{so_q0,X}{so_q0len,d}n"q"16t"qlen"8t"qlimit"n{so_q,X}{so_qlen,d}{so_qlimit,d}n"cc"8t"hi"8t"mbcnt"8t"mbmax"8t"mb"16t"flags"n{so_rcv.sb_cc,d}{so_rcv.sb_hiwat,d}{so_rcv.sb_mbcnt,d}{so_rcv.sb_mbmax,d}{so_rcv.sb_mb,X}{so_rcv.sb_flags,d}n{so_snd.sb_cc,d}{so_snd.sb_hiwat,d}{so_snd.sb_mbcnt,d}{so_snd.sb_mbmax,d}{so_snd.sb_mb,X}{so_snd.sb_flags,d}n"timeo"8t"error"8t"oobmark"8t"pgrp"n{so_timeo,d}{so_error,d}{so_oobmark,d}{so_pgrp,d}
