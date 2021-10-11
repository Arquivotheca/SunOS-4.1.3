/*
 * @(#)cpu.map.c 1.1 92/07/30
 *
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 **********************************************************************
 * cpu_map.c -  Routines used to initialize the PMMU's page tables, and
 *		set up the CPU's external registers.
 *
 *	getpgmap()   - Extract page entries from the PMMU's page table.
 *	get_tia()    - Get the TIA value from the PMMU's page table.
 *	get_tib()    - Get the TIB value from the PMMU's page table.
 *	setpgmap()   - Create page entries in the PMMU's page table.
 *	set_tia()    - Create TIA Entries in the PMMU's page table.
 *	set_tib()    - Create TIB Entries in the PMMU's page table.
 *	get_enable() - Get the contents of the System enable register.
 *	set_enable() - Set the value of the System enable register.
 *	set_leds()   - Write a value to the Diagnostic LED Register.
 *	getidprom()  - Get the value of the IDPORM.
 *	adjust_tbl_addr - Adjust the value of the table address.
 *
 **********************************************************************
 */
#include "../h/systypes.h"
#include "pmmu.h"
#include "cpu.addrs.h"

extern int	get_crp(), get_srp(), get_tc(), set_tc();

/**********************************************************************
 * Description: Get a value from the Page Table pointed to by the 
 *		virtual address passed in by the caller.
 * Synopsis:    status = getpgmap(virt_addr)
 *              status  : (int) 0 = pgtable_addr
 *			: (int) 1 =
 *		virt_addr:(u_long) 32 bit virtual address of the pgmap entry.
 *
 * Routines:    get_tib(), 
 **********************************************************************
 */
getpgmap(virt_addr) 
	u_long	virt_addr;
{
	u_long	*pgtable_addr;  /* Ptr to begining of the page table */
	int	index;		/* Index into that page table */
	/*
	 * If this is an I/O Mapper address, use I/O Mapper Tables
	 */
	if ((get_tib(virt_addr) & 0xFFFFE000) == pa_IOMAPPER) {
		return(get_iomapent(virt_addr));
	} else {
		index = ((virt_addr & PAGEMASK) >> PAGESHIFT) * TBL_ADDR_OFFSET;
		(u_long)pgtable_addr = (get_tib(virt_addr)&0xFFFFFFF0) + index;
		return(*pgtable_addr);
	}
}

/**********************************************************************
 * Description: Use the virtual address to determine which location should
 *		be read from the TIA table.  Set the value of that location
 *		in the TIA struct, and return with the value set and ready 
 *		to be used.
 * Synopsis:    status = get_tia(virt_addr)
 *              status  : (int) 0 = 
 *			: (int) 1 =
 *		virt_addr:(u_long) 32 bit virtual address of the pgmap entry.
 *
 * Routines:	_get_crp(),
 **********************************************************************
 */
u_long
get_tia(virt_addr)
	u_long   virt_addr;
{
	int	index;			/* Index to the tia table entry */
	u_long  *tib_tbl_addr;		/* Ptr to the TIB table address field*/
	struct	crp_reg crp_val;	/* 8 bytes reserverd for CRP value */ 

	index = ((virt_addr & TIAMASK) >> TIASHIFT) * STATUS_OFFSET;
	get_crp(&crp_val);
	(u_long)tib_tbl_addr = adjust_crp_addr(crp_val.crp_tbl_addr) + index 
						+ TBL_ADDR_OFFSET;
	return(adjust_tbl_addr(*tib_tbl_addr));
}

/********************************************************************** 
 * Description: Get TIB value from the Level #2 table, using the virtual
 *		address passed in by the caller. 
 * Synopsis:    status = get_tib(virt_addr)
 *              status  : (int) 0 = *pg_tbl_addr
 *			: (int) 1 =
 *		virt_addr:(u_long) 32 bit virtual address of the TIB entry.
 *
 * Routines:	get_tia(), 
 **********************************************************************
 */ 
get_tib(virt_addr)
	u_long   virt_addr;
{
	u_long	*pg_tbl_addr;		/* Ptr to start of the Page Table */
	int	index;			/* TIB Table entry index */

	index = ((virt_addr & TIBMASK) >> TIBSHIFT) * TBL_ADDR_OFFSET;
	(u_long)pg_tbl_addr = get_tia(virt_addr) + index;

	return(adjust_tbl_addr(*pg_tbl_addr));/* Contents of Page tbl entry */
}

 
/**********************************************************************
 * Description: Set a page value using the appropriate virtual address
 * Synopsis:    status = setpgmap(virt_addr,val) 
 *              status  : (int) 0 = 
 *			: (int) 1 =
 *		virt_addr: (u_long) 32 bit virtual address of the pgmap entry.
 *		val	: (ulong) page map entry value.
 *
 * Routines:    get_tib(),
 **********************************************************************
 */
setpgmap(virt_addr, val) 
	u_long	virt_addr, val;
{
	u_long	*pg_tbl_entry;		/* Ptr to the PG table entry */
	int    	index;			/* index into Page Table */

	if ((get_tib(virt_addr) & 0xFFFFE000) == pa_IOMAPPER) {
                set_iomapent(virt_addr, val);
        } else {
		index = ((virt_addr & PAGEMASK) >> PAGESHIFT) * TBL_ADDR_OFFSET;
		(u_long)pg_tbl_entry = (get_tib(virt_addr)&0xFFFFFFF0) + index;
		*pg_tbl_entry = val;            /* Set this page entry = val */
	}
}

/********************************************************************** 
 * Description: Set an 8 byte value in the TIA Table.  Enter routine with 
 *		the value for the table entry aready set in the struct.
 *		Use set_tia to get an index into the TIA table, then write
 *		the value of the Status and Table Address to the index/address
 *		that is calculated.
 * Synopsis:    status = set_tia(virt_addr, status, tib_addr)
 *              status : (int) 0 = 
 *                     : (int) 1 = 
 *		virt_addr:(u_long) 32 bit virtual address of the pgmap entry.
 *		status	:(u_long) status field of the TIA entry
 *		tib_addr:(u_long) address field of the TIA entry
 *
 * Routines:	get_crp(), adjust_tbl_addr(),
 **********************************************************************
 */ 
set_tia(virt_addr, status, tib_addr) 
	u_long	virt_addr, status, tib_addr;
{
	u_long	*tib_ptr;		/* Ptr to this entry in the TIA table*/
	struct  crp_reg crp_val;	/* 8 bytes reserverd for crp value */

	get_crp(&crp_val);		/* Get the value of the root ptr */
	/*
	 * Ptr to the TIB table entry is = the root ptr + a TIA Table index
	 */
	(u_long)tib_ptr = (((virt_addr & TIAMASK) >> TIASHIFT)*STATUS_OFFSET) +
			  adjust_crp_addr(crp_val.crp_tbl_addr); 
					/* Value set in crp struct */

	*tib_ptr++ = status; 		/* Set Status field */
	*tib_ptr   = tib_addr;		/* Set address field */
}

/********************************************************************** 
 * Description: Set a 4 byte TIB value.  Mask off and shift the Virtual
 *		Address to get an index into the TIB table.  Get the
 *		start of the TIB table from the function get_TIA().
 * Synopsis:    status = set_tib(virt_addr, val)
 *              status  : (int) 0 = 
 *                     	: (int) 1 = 
 *		virt_addr:(u_long) 32 bit Virtual Address
 *		val	: (u_long) value to set the TIB entry to.
 *
 * Routines:	get_tia(),
 **********************************************************************
 */ 
set_tib(virt_addr, val) 
	u_long   virt_addr, val;
{
	u_long	*tib_tbl_addr;
	int	index;

	index = ((virt_addr & TIBMASK) >> TIBSHIFT) * TBL_ADDR_OFFSET;
	(u_long)tib_tbl_addr = get_tia(virt_addr) + index;/* get tbl start */

	*tib_tbl_addr = val;		/* WR value to tbl */
}

/**********************************************************************
 * Description: Set the value of the Sun3x System Enable Register.  The
 *		size of the System Enable Register has been expaned to
 *		16 bits for the Sun3x architecture.             
 * Synopsis:    status = set_enable(val)
 *              status  : (int) 0 = 
 *                      : (int) 1 = 
 *              val     : (u_long) 16 bit value for the Enable Register.
 *
 * Routines:    get_tc(),
 **********************************************************************
 */
set_enable(val)
	short val;
{
	u_long	tc_val;
	short	*enable_ptr;
/*
 * Determine if the PMMU is on, set the registers address accordingly.
 */
	if (get_tc(&tc_val) & PMMU_ENABLE)
		enable_ptr = SYS_ENABLE_BASE;	/* PMMU ON, Use Virtual Addr */
	else 
		enable_ptr = pa_SYS_ENABLE;	/* PMMU Off, Use Physical    */

	*enable_ptr = val;
}

/**********************************************************************
 * Description: Get the value of the System Enable Register.  
 * Synopsis:    status = get_enable()
 *              status  : (int) 0 = (short)*enable_ptr
 *                      : (int) 1 = 
 *
 * Routines:    get_tc(),
 **********************************************************************
 */
short
get_enable()
{
	u_long	tc_val;				/* Address to put return val */
	short   *enable_ptr;
/* 
 * Determine if the PMMU is on, set the registers address accordingly. 
 */ 
        if (get_tc(&tc_val) & PMMU_ENABLE) 	/* Read the TC Reg's contents*/
                enable_ptr = SYS_ENABLE_BASE;   /* PMMU ON, Use Virtual Addr */
        else 
                enable_ptr = pa_SYS_ENABLE;     /* PMMU Off, Use Physical    */

        return(*enable_ptr);			/* Return Enable Reg Contents*/
}

/**********************************************************************
 * Description: Write a value to the LED Diagnostic Register.
 * Synopsis:    status = set_leds()
 *              status  : (int) 0 = 
 *                      : (int) 1 =
 *
 * Routines:    get_tc(),
 **********************************************************************
 */
set_leds(val)
	char	val;
{
	u_long	tc_val;			/* Address to put return val */
	char	*led_ptr;
/*
 * Determine if the PMMU is on, set the registers address accordingly.
 */
        if (get_tc(&tc_val) & PMMU_ENABLE)	/* Read the TC Reg's contents*/
                led_ptr = LED_REG_BASE;   	/* PMMU ON, Use Virtual Addr */
        else
                led_ptr = pa_LED_REG;     	/* PMMU Off, Use Physical    */

        *led_ptr = ~val;/* Set Led's to the ones complement of val passed in */
}

/********************************************************************** 
 * Description: Write a value to the LED Diagnostic Register.
 * Synopsis:    status = getidprom() 
 *              status  : (int) 0 = 
 *                      : (int) 1 =
 * 
 * Routines:    get_tc(), 
 ********************************************************************** 
 */
getidprom(addr_ptr, size)
	char	*addr_ptr;
	int	size;
{
	u_long	tc_val;				/* Address to put return val */
        char    *idprom_ptr;
	int	i; 
/* 
 * Determine if the PMMU is on, set the registers address accordingly. 
 */
        if (get_tc(&tc_val) & PMMU_ENABLE)	/* Read the TC Reg's contents*/
                idprom_ptr = IDPROM_BASE;	/* PMMU ON, Use Virtual Addr */
        else
                idprom_ptr = pa_IDPROM;		/* PMMU Off, Use Physical    */

	for (i = 0; i < size; i++)
        	*addr_ptr++ = *idprom_ptr++;
}

/**********************************************************************
 * Description: Adjust the Table Address, if the PMMU is On, by adding
 *		the virtual address of PMMU_BASE to the lower bits of
 *		the tbl_address;
 * Synopsis:    status = adjust_tbl_addr(tbl_addr)
 *              status  : (int) 0 =
 *                      : (int) 1 =
 *              tbl_addr: (u_long) Location of the PMMU table.
 *
 * Routines:    get_tc(),
 **********************************************************************
 */
adjust_tbl_addr(tbl_addr)
        u_long  tbl_addr;       /* Location of Table */
{
	u_long	tc_val;

   	return((get_tc(&tc_val) & PMMU_ENABLE) ?
		PMMU_BASE + (tbl_addr & 0x1fff) : tbl_addr);
}

adjust_crp_addr(tbl_addr)
        u_long  tbl_addr;       /* Location of Table */
{
	u_long	tc_val;

   	return((get_tc(&tc_val) & PMMU_ENABLE) ? PMMU_BASE : tbl_addr);
}


/**********************************************************************
 * Description: Generic Delay routine.  
 * Synopsis:    status = delay(duration)
 *              status  : (int) 0 =
 *                      : (int) 1 =
 *		duration: (u_long) Value of required delay
 *
 * Routines:    get_tc(),
 **********************************************************************
 */
delay(duration)
	u_long	duration;	/* Value of required delay */
{
	while (duration)
		--duration;
}

