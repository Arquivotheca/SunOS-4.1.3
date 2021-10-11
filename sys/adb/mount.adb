#include <sys/types.h>
#include <sys/vfs.h>
#include <ufs/mount.h>

mount
./"nxt"16t"vfsp"16t"devvp"16t"bufp"n{m_nxt,X}{m_vfsp,X}{m_devvp,X}{m_bufp,X}
+/"maj"8t"min"8t"qflags"8t"qinod"n{m_dev,2b}{m_qflags,x}{m_qinod,X}
