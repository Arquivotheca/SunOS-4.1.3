/*
 *  hsparam.h.  Header file for sunlink.c.
 *
 * @(#) hsparam.h 1.1 7/30/92 1988 Sun Microsystems Inc.
 */

struct hs_param {
	u_char hp_lback;   /* select mk5025 loopback mode */
	u_char hp_tpval;   /* time poll value */
	u_char hp_fwm;     /* fifo water mark */
	u_char hp_burst;   /* burst length */
	u_char hp_v35;     /* V.35 or RS422 */
    u_char hp_iflags;   /* data and clock invert flags */
	struct syncmode hp_mode;
};

#define TXC_IS_SYSCLK  4
#define RXC_IS_SYSCLK  4
#define TXC_IS_INVERT  5
#define RXC_IS_INVERT  5
#define TRXD_NO_INVERT 0
#define RXD_IS_INVERT  1
#define TXD_IS_INVERT  2
#define TRXD_IS_INVERT 3
