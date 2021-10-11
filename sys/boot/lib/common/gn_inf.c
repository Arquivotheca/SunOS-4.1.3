/*      @(#)gn_inf.c 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* 
 * gn_inf.c - Generic Interface
 *
 * Routine to initialize the devinfo structure and
 * associated data from the EEPROM.
 */

/*
 *  To set-up the EEPROM for the new generic network
 *	data info fields of address and memory type, use 
 *  the following addresses
 *
 *  BC is the first byte signifying generic net type of slot (0x13)
 *  BD is the second type byte to signify shared or dma interface (0x0 - shared mem)
 *  BE-C1 are the 4 bytes representing the address with the most significant bytes first
 *
 *  This pattern of 6 bytes repeats itself at addresses BC, CC, DC, and EC for the
 *  four possible board addresses for the generic network
 */


#include <sys/types.h>
#include <mon/eeprom.h>
#include <mon/sunromvec.h>
#include <mon/cpu.addrs.h>
#include <stand/saio.h>


#ifdef sun4
#define READ_SHORT(short_number, short_addr) {\
     register unsigned char *cptr = short_addr;\
     short_number = (0xff00&((*(cptr))<< 8)) | (0xff&(*(cptr+1)));\
	 short_number = short_number<<16;\
 	 short_number = short_number&0xffff0000 | ((0xff00&((*(cptr+2))<< 8))\
	 | (0xff&(*(cptr+3))));\
}
#endif sun4


/*
 * Name:	gn_inf()
 * 
 * Description:	gn_inf provides the information needed 
 * for the Generic Network (FDDI) driver. It extracts them from EEPROM.
 * 
 * Synopsis:	status = gn_inf()
 * 
 * status:	None
 * 
 * Routines:	None
 */


/*
 * lists of external declarations found in individual drivers. 
 */
extern u_char gnmemtype[];
extern u_long gnaddrs[];
extern struct devinfo gninfo;
extern int gnctlrnum;

gn_inf(sip)
register struct saioreq *sip;
{
	register int slot,i;
	char ctlra[EEC_TYPE_END];
	register struct eed_conf *conf_ptr = EEPROM->ee_diag.eed_conf;
	register struct devinfo *devp;
	register u_long high;
	register u_char *temp_ptr;

	/* clear out number of controllers array */
	for (i = 0; i < EEC_TYPE_END; ctlra[i++] = 0);

	/* loop through all slot info and assign board addresses */
	for (slot = 0; slot < MAX_SLOTS+1; slot++) {
		switch ( conf_ptr[slot].eec_un.eec_gen.eec_type ) {
		case EEC_TYPE_GENERIC_NET:
			devp = &gninfo;
#ifndef sun4
			devp->d_stdaddrs[ctlra[EEC_TYPE_GENERIC_NET]]= 
			    (long) conf_ptr[slot].eec_un.eec_generic_net.dev_reg_ptr;
#else sun4
			temp_ptr = (u_char *) conf_ptr[slot].eec_un.eec_generic_net.dev_reg_ptr;
			READ_SHORT(high, temp_ptr);
			devp->d_stdaddrs[ctlra[EEC_TYPE_GENERIC_NET]] = high;
#endif sun4
			gnmemtype[ctlra[EEC_TYPE_GENERIC_NET]] = (long) conf_ptr[slot] .eec_un.eec_generic_net.eec_mem_type;
			devp->d_stdcount = ++ctlra[EEC_TYPE_GENERIC_NET];
			gnctlrnum = sip->si_ctlr;		/* save controller number */
			/* as this is trashed out */
			/* by the devopen routine */
			break;

			/* ADD NEW DRIVERS THAT UTILIZE EEPROM BOARD ADDRESSES HERE */

		default:
			break;
		} /* switch */
	} /* for */

} /* gn_inf */

