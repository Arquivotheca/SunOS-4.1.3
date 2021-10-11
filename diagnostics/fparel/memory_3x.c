static char sccsid[] = "@(#)memory_3x.c 1.1 7/30/92 Copyright Sun Microsystems"; 

/*
 * ****************************************************************************
 * Source File     : memory_3x.c
 * Original Engr   : Nancy Chow
 * Date            : 10/29/88
 * Function        : This file contains the routines used to verify the contents
 *		   : of the Ustore, Map Ram, and Register Ram on the FPA-3X.
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

/* ***************
 * Include Files
 * ***************/

#include <sys/types.h>
#include "fpa3x.h"

/* *************
 * map_ram_sel
 * *************/

/* Stub to select Map Ram test for FPA or FPA-3X */

map_ram_sel()
{
	u_long version;

	version = *(u_long *)REG_IMASK & VERSION_MASK;
 
	if (version == VERSION_FPA)
		return(map_ram());
	else
		return(0);	/* Map Ram is incorporated into ustore */
}

/* ****************
 * ustore_ram_sel
 * ****************/

/* Stub to select Ustore Ram test for FPA or FPA-3X */

ustore_ram_sel()
{
        u_long version; 
 
        version = *(u_long *)REG_IMASK & VERSION_MASK;
 
	if (version == VERSION_FPA)
		return(ustore_ram());
	else
		return(ustore_ram_3x());
}

/* ****************
 * reg_uh_ram_sel
 * ****************/

/* Stub to select Reg Ram test for FPA or FPA-3X */

reg_uh_ram_sel()
{
        u_long version; 
 
        version = *(u_long *)REG_IMASK & VERSION_MASK;
 
	if (version == VERSION_FPA)
		return(reg_uh_ram());
	else
		return(reg_uh_ram_3x());
}

/* ***************
 * ustore_ram_3x
 * ***************/

/* Calcualate and verify checksum for Ustore Ram (FPA-3X). */

ustore_ram_3x()
{

	u_long checksum_h;	/* MSH Map Ram checksum */
	u_long checksum_l;	/* LSH Map Ram checksum */
	u_long lo_cksum;	/* expected LSH checksum from Ram */
	u_long hi_cksum;	/* expected MSH checksum from Ram */
	u_long ram_data_h;	/* MSH Map Ram data */
	u_long ram_data_l;	/* LSH Map Ram data */
	u_long no_data_flag = TRUE;
	int    i;

	checksum_l = 0;
	checksum_h = 0;

							/* search for ustore checksum; */
							/* first non-zero location     */
							/* from end of ustore          */
	for (i=MAX_USTORE-1; (no_data_flag == TRUE) && (i>=MIN_MAP_RAM); i--) {
		*(u_long *)REG_LOADPTR = LD_PTR_LSH(i); /* addr LSH */
                ram_data_l = *(u_long *)REG_LDRAM;      /* LSH value */
                *(u_long *)REG_LOADPTR = LD_PTR_MSH(i); /* addr MSH */
                ram_data_h = *(u_long *)REG_LDRAM;      /* MSH value */

		if ((ram_data_l) || (ram_data_h)) {                  
                        no_data_flag = FALSE;           /* data is non zero */
			lo_cksum = ram_data_l;		/* set expected LSH checksum */
			hi_cksum = ram_data_h;		/* set expected MSH checksum */
		}
	}

	if (no_data_flag)       			/* ram filled with zero */
                return(-1);

	for (; i >= MIN_MAP_RAM; i--) {			/* calc checksum */
		*(u_long *)REG_LOADPTR = LD_PTR_LSH(i);	/* addr LSH */
		ram_data_l = *(u_long *)REG_LDRAM;	/* LSH value */
		*(u_long *)REG_LOADPTR = LD_PTR_MSH(i);	/* addr MSH */
		ram_data_h = *(u_long *)REG_LDRAM;      /* MSH value */
		
		checksum_l ^= ram_data_l;		/* calc LSH checksum */
		checksum_h ^= ram_data_h;		/* calc MSH checksum */
	}

				/* if calc checksum == expected */
	if (((checksum_l^CKSUM_XOR) == lo_cksum) && ((checksum_h^CKSUM_XOR) == hi_cksum))
		return(0);
	else
		return(-1);
}

/* ***************
 * reg_uh_ram_3x
 * ***************/

/* Calculate and verify checksum for register ram (FPA-3X). */

reg_uh_ram_3x()
{

	u_long checksum_h;
	u_long checksum_l;
	u_long ram_data_h;
	u_long ram_data_l;
	u_long no_data_flag = TRUE;
	int    i;
 
	checksum_l = 0;
	checksum_h = 0;
 
        for (i=RR_CKSUM_START; i < RR_CKSUM_END; i++) { /* for each ram loc */
                *(u_long *)REG_LOADPTR = LD_PTR_ADDR(i);/* addr LSH */
                ram_data_l = *(u_long *)INS_REGFLP_L;   /* LSH value */
                ram_data_h = *(u_long *)INS_REGFLP_M;   /* MSH value */

                if ((ram_data_l) || (ram_data_h))                       
                        no_data_flag = FALSE;           /* data is non zero */
 
                checksum_l ^= ram_data_l;               /* calc LSH checksum */
                checksum_h ^= ram_data_h;               /* calc MSH checksum */
        }
 
        if (no_data_flag)       /* ram filled with zero */
                return(-1);
 
				/* get expected cksum */
	*(u_long *)REG_LOADPTR = LD_PTR_ADDR(CKSUM_LOC);
                                /* if calc checksum == expected */
	if ((checksum_l ^ CKSUM_XOR) != *(u_long *)INS_REGFLP_L)
		return(-1);
	if ((checksum_h ^ CKSUM_XOR) != *(u_long *)INS_REGFLP_M)
		return(-1);

        return(0);
}
 


