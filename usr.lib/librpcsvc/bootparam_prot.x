/* @(#)bootparam_prot.x 1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * RPC for bootparms service.
 * There are two procedures:
 *   WHOAMI takes a net address and returns a client name and also a
 *	likely net address for routing
 *   GETFILE takes a client name and file identifier and returns the
 *	server name, server net address and pathname for the file.
 *   file identifiers typically include root, swap, pub and dump
 */

#ifdef RPC_HDR
%#include <rpc/types.h>
%#include <sys/time.h>
%#include <sys/errno.h>
%#include <nfs/nfs.h>
#endif

const MAX_MACHINE_NAME  = 255;
const MAX_PATH_LEN	= 1024;
const MAX_FILEID	= 32;
const IP_ADDR_TYPE	= 1;

typedef	string	bp_machine_name_t<MAX_MACHINE_NAME>;
typedef	string	bp_path_t<MAX_PATH_LEN>;
typedef	string	bp_fileid_t<MAX_FILEID>;

struct	ip_addr_t {
	char	net;
	char	host;
	char	lh;
	char	impno;
};

union bp_address switch (int address_type) {
	case IP_ADDR_TYPE:
		ip_addr_t	ip_addr;
};

struct bp_whoami_arg {
	bp_address		client_address;
};

struct bp_whoami_res {
	bp_machine_name_t	client_name;
	bp_machine_name_t	domain_name;
	bp_address		router_address;
};

struct bp_getfile_arg {
	bp_machine_name_t	client_name;
	bp_fileid_t		file_id;
};
	
struct bp_getfile_res {
	bp_machine_name_t	server_name;
	bp_address		server_address;
	bp_path_t		server_path;
};

program BOOTPARAMPROG {
	version BOOTPARAMVERS {
		bp_whoami_res	BOOTPARAMPROC_WHOAMI(bp_whoami_arg) = 1;
		bp_getfile_res	BOOTPARAMPROC_GETFILE(bp_getfile_arg) = 2;
	} = 1;
} = 100026;
