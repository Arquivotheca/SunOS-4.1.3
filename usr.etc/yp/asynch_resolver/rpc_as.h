#ifndef _rpc_as_h
/*static	char sccsid[] = "@(#)rpc_as.h 1.1 92/07/30 Copyright Sun Micro 1989";*/
#define _rpc_as_h
#include <rpc/types.h>
struct rpc_as {
	char   *as_userptr;	/*anything you like*/
	struct timeval   as_timeout_remain;
	int	as_fd;
	bool_t	as_timeout_flag; /*set if timeouts wanted*/
	void (*as_timeout)();  /*routine to call if timeouts wanted*/
	void (*as_recv)();     /*routine to call if data is present*/
};
typedef struct rpc_as rpc_as;

struct timeval rpc_as_get_timeout(); /*get timeval to wait for*/
struct fd_set rpc_as_get_fdset(); /*get fdset to wait for*/
void	rpc_as_timeout();	  /*process timeout if select times out*/
void	rpc_as_rcvreqset(/*fdset*/);/*process read on bits returned by select*/

int	rpc_as_rcv(); /*process read or timeout signal -- never wait
			returns select status*/

int	rpc_as_register(/* &rpc_as*/);
int	rpc_as_unregister(/* &rpc_as*/);

rpc_as *rpc_as_fd_to_as(/*fd*/); 

#endif
