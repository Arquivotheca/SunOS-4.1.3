/*	@(#)gpio.h 1.1 92/07/30 SMI	*/

#ifndef _sun_gpio_h
#define	_sun_gpio_h

/*
 * General purpose structure for passing info between GP/fb/processes
 */
struct gp1fbinfo {
	int	fb_vmeaddr;	/* physical color board address */
	int	fb_hwwidth;	/* fb board width */
	int	fb_hwheight;	/* fb board height */
	int	addrdelta;	/* phys addr diff between fb and gp */
	caddr_t	fb_ropaddr;	/* cg2 va thru kernelmap */
	int	fbunit;		/* fb unit to use  for a, b, c, d */
};


/*
 * The following ioctl commands allow for the transferring of data
 * between the graphics processor and color boards and processes.
 */

/* passes information about fb into driver */
#define	GP1IO_PUT_INFO			_IOW(G, 0, struct gp1fbinfo)

/* hands out a static block  from the GP*/
#define	GP1IO_GET_STATIC_BLOCK		_IOR(G, 1, int)

/* frees a static block from the GP */
#define	GP1IO_FREE_STATIC_BLOCK		_IOW(G, 2, int)

/* see if this gp/fb combo can see the GP */
#define	GP1IO_GET_GBUFFER_STATE		_IOR(G, 3, int)

/* restarts the GP if neccessary */
#define	GP1IO_CHK_GP			_IOW(G, 4, int)

/* returns the number of restarts of a gpunit since power on */
/* needed to differentiate SIGXCPU calls in user land */
#define	GP1IO_GET_RESTART_COUNT		_IOR(G, 5, int)

/* configures /dev/fb to talk to a gpunit */
#define	GP1IO_REDIRECT_DEVFB		_IOW(G, 6, int)

/* returns the requested minor device */
#define	GP1IO_GET_REQDEV		_IOR(G, 7, dev_t)

/* returns the true minor device */
#define	GP1IO_GET_TRUMINORDEV		_IOR(G, 8, char)

/* see if there is a GB attached to this GP */
#define	GP1IO_CHK_FOR_GBUFFER 		_IOR(G, 9, int)

/* set the fb that can use the Graphics BUffer */
#define	GP1IO_SET_USING_GBUFFER		_IOW(G, 10, int)

struct static_block_info
{
    int                 sbi_count;
    char               *sbi_array;
};

/* inform which blocks are owned by the current process. */
#define	GP1IO_INFO_STATIC_BLOCK		_IOWR(G, 11, struct static_block_info)

#define	GP1_UNBIND_FBUNIT  0x08000000

#endif				       /* !_sun_gpio_h */
