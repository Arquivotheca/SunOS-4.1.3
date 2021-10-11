/*	@(#)fcntl.c 1.1 92/07/30 Copyright SMI 1992 */


#include <sys/types.h>
#include <unistd.h>
#include "fcntlcom.h"
#include <fcntl.h>

fcntl(fd, cmd, arg) 
int fd, cmd;
struct flock *arg; 
{ 
	int hold;

	hold = posix_fcntl(fd, cmd, arg); 
	if (cmd == F_GETLK) {  
		arg->l_pid >>= 16;
		arg->l_whence = SEEK_SET;
	}        
	return (hold);
} 
