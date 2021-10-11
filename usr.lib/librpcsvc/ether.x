/* @(#)ether.x 1.1 92/07/30 Copyr 1987 Sun Micro */

/*
 * Gather statistics about the ethernet
 */

const NBUCKETS = 16;
const NPROTOS = 6;
const HASHSIZE = 256;

/*
 * Seconds and microseconds since 0:00, January 1, 1970
 */
struct ethertimeval {
	unsigned int tv_seconds;
	unsigned int tv_useconds;
};

/*
 * all ether stat's except src, dst addresses
 */
struct etherstat {
	ethertimeval	e_time;
	unsigned int e_bytes;
	unsigned int e_packets;
	unsigned int e_bcast;
	unsigned int e_size[NBUCKETS];
	unsigned int e_proto[NPROTOS];
};
/*
 * member of address hash table
 */
struct etherhmem_node {
	int h_addr;
	unsigned int h_cnt;
	etherhmem_node *h_nxt;
};
typedef etherhmem_node *etherhmem;

/*
 * src, dst address info
 */
struct etheraddrs {
	ethertimeval	e_time;
	unsigned int e_bytes;
	unsigned int e_packets;
	unsigned int e_bcast;
	etherhmem e_addrs[HASHSIZE];
};

struct addrmask {
	int a_addr;
	int a_mask;
};


program ETHERPROG {
	version ETHERVERS {
		/*
		 * Get ethernet data
		 */
		etherstat
		ETHERPROC_GETDATA(void) = 1;

		/*
		 * Turn on promiscuous mode
		 */
		void
		ETHERPROC_ON(void) = 2;

		/*
		 * Turn off promiscous mode
		 */
		void
		ETHERPROC_OFF(void) = 3;


		/*
		 * Return source address information
		 */
		etheraddrs
		ETHERPROC_GETSRCDATA(void) = 4;

		/*
		 * Return destination address information
		 */
		etheraddrs
		ETHERPROC_GETDSTDATA(void) = 5;

		/*
		 * Select source address mask
		 */
		void
		ETHERPROC_SELECTSRC(addrmask) = 6;

		/*
		 * Select destination address mask
		 */
		void
		ETHERPROC_SELECTDST(addrmask) = 7;

		/*
		 * Select protocol mask
		 */
		void
		ETHERPROC_SELECTPROTO(addrmask) = 8;

		/*
		 * Select length mask
		 */
		void
		ETHERPROC_SELECTLNTH(addrmask) = 9;
	} = 1;
} = 100010;
