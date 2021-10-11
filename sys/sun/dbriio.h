/*
 * @(#) dbriio.h 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc.
 */

/*
 * DBRI specific ioctl descriptions
 */

/* 
 * NOTE: ISDN functionality is not currently supported in s/w.
 */

#define DBRI_STATUS	_IOWR(A, 101, isdn_info_t) /* get DBRI status */

typedef struct dbri_stat {
	isdn_port_t	port;
	unsigned int	dma_underflow;
	unsigned int	dma_overflow;
	unsigned int	abort_error;
	unsigned int	crc_error;
	unsigned int	badbyte_error;
	unsigned int	crc_error_soft;
	unsigned int	recv_eol;
	unsigned int	recv_error_octets;
} dbri_stat_t;

/*
 * DBRI specific parameters for ISDN_SET_PH_PARAM ioctl.
 */

	/*
	 * Set required applition service interval. If application does
	 * not service within this interval, then the device driver
	 * will assume that the upper layer software is no longer
	 * functioning and will behave as if power has been removed from
	 * the interface.
	 *
	 * The duration is expressed in milliseconds.
	 *
	 * A duration of zero milliseconds disables the sanity timer.
	 * When the sanity timer is disabled, the application is assumed
	 * to be sane whenever the interface's D-channel is open.
	 *
	 * The sanity timer is disabled by default.
	 */
#define ISDN_PH_PARAM_SANITY_INTERVAL	ISDN_PH_PARAM_VENDOR(1)
#define 	ISDN_PH_PARAM_SANITY_INTERVAL_DISABLE	(0)

	/*
	 * Application executes this ioctl at lease once
	 * every sanity timer interval
	 * in order to assure driver of working upper layers.
	 */
#define ISDN_PH_PARAM_SANITY_PING	ISDN_PH_PARAM_VENDOR(2)

#ifdef DBRI_UNSUPPORTED
	/*
	 * Force a momentary loss of sync, arg is number of usec to disable
	 * interface .
	 */
#define ISDN_PH_PARAM_GLITCH		ISDN_PH_PARAM_VENDOR(101)

	/*
	 * NORESTART is a hack to test DBRI failure to restart after loss
	 * of sync
	 */
#define ISDN_PH_PARAM_NORESTART		ISDN_PH_PARAM_VENDOR(102)

#endif /* DBRI_UNSUPPORTED */
