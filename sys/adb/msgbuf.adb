#include <sys/msgbuf.h>

msgbuf
msgbuf/"magic"16t"size"16t"bufx"16t"bufr"n{msg_magic,X}{msg_size,X}{msg_bufx,X}{msg_bufr,X}{msg_bufc}
+,({*msg_bufx,msgbuf}-{*msg_bufr,msgbuf})&80000000$<msgbuf.wrap
.+{*msg_bufr,msgbuf},({*msg_bufx,msgbuf}-{*msg_bufr,msgbuf})/c
