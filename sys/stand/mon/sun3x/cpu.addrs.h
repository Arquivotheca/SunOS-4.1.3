/*
 * @(#)cpu.addrs.h 1.1 92/07/30 SMI; from UCB X.X XX/XX/XX
 *
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 *********************************************************************
 * Memory Addresses of Interest in the Sun-3x "Inter-Active Monitor's" 
 * Memory Map.
 *
 * THE VALUE FOR "PROM_BASE" must be changed in several places when it is
 * changed.  A complete list follows:
 *
 *	../conf/Makefile.mon	RELOC= near top of file
 *	../h/sunromvec.h	#define romp near end of file
 *	../* /Makefile		(best to rerun make depend to generate them)
 *
 * The following defines the base addresses for various devices, and
 * other forms of Memory that are mapped in when the PROM Monitor powers up.
 * Ongoing operation of the MONITOR requires that these locations remain
 * mapped as they are here.
 *
 ********************************************************************
 * On-board I/O devices
 */
#define MEM_BASE					0x00000000
#define	KEYBMOUSE_BASE		((struct zscc_device *)	0xFEF00000)
#define	SERIAL0_BASE		((struct zscc_device *)	0xFEF02000)
#define	EEPROM_BASE		((unsigned char *)	0xFEF04000)
#define	CLOCK_BASE		((struct intersil7170 *)0xFEF06000)
#define	MEMORY_ERR_BASE		((struct memreg *)	0xFEF09000)
#define	INTERRUPT_BASE		((unsigned char *)	0xFEF0B400)
#define COLORMAP_BASE					0xFEF0C000
#ifdef HYDRA
#define AMD_ETHER_BASE  	((struct amd_ether *)   0xFEF0E000)
#else
#define	ETHER_BASE		((struct obie_device *) 0xFEF0E000)
#endif HYDRA
#define	SCSI_BASE		((struct scsichip *)	0xFEF10000)
#define	DES_BASE		((struct deschip *)	0xFEF12000)
#define	ECC_ENABLE_BASE		((struct ecc_ctrl *)	0xFEF14000)
#define SYS_ENABLE_BASE		((short *)		0xFEF16000)
#define BUSERR_REG_BASE		((char *)		0xFEF18400)
#define LED_REG_BASE		((char *)		0xFEF1A800)
#define IDPROM_BASE		((char *)		0xFEF1CC00)
#define	ID_EEPROM_BASE          ((char *)       (0xFEF04000+0x7d8))
/*
 * Video Memory (For the P4 Bus interface)
 */
#define P4_REG_BASE		((struct p4reg *)	0xFEF1E000)
#define VIDEOMEM_BASE					0xFEF20000 /* 32 PG's*/
#define OVERLAY_BASE		VIDEOMEM_BASE
#define ENABLE_BASE					0xFEF40000
/*
 * Monitor scratch RAM for stack, trap vectors, and expanded font
 */
#define	STACK_BASE					0xFEF60000
#define	STACK_TOP					0xFEF60C00
#define	TRAPVECTOR_BASE					0xFEF60C00
#define	FONT_BASE					0xFEF61000
/* 
 * RAM page used to hold the Valid main memory pages bitmap if
 * memory fails SelfTest. Pages will be mapped invalid if memory is OK.
 */
#define MAINMEM_BITMAP                                  0xFEF62000
/*
 * Starting Virtual Address of the PMMU Translation Tables 
 */
#define PMMU_BASE					0xFEF64000

/*
 * The Rest of the Montior PAGES are listed here, they fall under 
 * the definition of other stuff.
 */
#define IOMAPPER_BASE					0xFEF66000
#define CP_FLSH_BASE					0xFEF68000
#define RESERVED_1					0xFEF6A000
#define IOTAGS_BASE					0xFEF6C000
#define IODATA_BASE					0xFEF6E000
#define FLSHIOCACHE_BASE				0xFEF70000
/*
 * Roughly 512K for mapping in devices & mainmem during boot.
 * GLOBAL_PAGE Must be changed later for DVMA use.  It should
 * point to the MONSHORTPG.
 */
#define GLOBRAM_BASE					0xFEF72000	
#define PTR_BASE					0xFEF74000
#define VME_STATREG_BASE				0xFEF76000
#define COLOR_ZOOM_BASE					0xFEF78000
#define COLOR_WPAN_BASE					0xFEF7A000
#define COLOR_PPAN_BASE					0xFEF7C000
#define COLOR_VZOOM_BASE				0xFEF7E000
#define COLOR_MASK_BASE					0xFEF80000
/*
 ************************************************************************ 
 * DANGER---------DANGER Added No other #defines below these
 ************************************************************************ 
 */
#define BOOTMAP_BASE                           		0xFEF82000
#define RESERVED_BASE				BOOTMAP_BASE+PAGESIZE
/*
 *  Location of PROM code.  The system executes out of these addresses.
 */
#define CACHETAGS_BASE					0xFEFC0000
#define CACHEDATA_BASE					0xFEFD0000
#define	EPROM_BASE					0xFEFE0000
/*
 * First hardware virtual address where DVMA is possible.
 * The Monitor does not normally use this address range, but it
 * does use it during bootstrap, via the resalloc() routine.
 */
#define	IO_MAP_BASE					0xFF000000
#define	DVMA_BASE				(char *)0xFFF00000
/*
 * One page of scratch RAM for general Monitor global variables.
 * We put it here so it can be accessed with short absolute addresses.
 *
 * ****** SEE NOTE AT TOP OF FILE IF YOU CHANGE THIS VALUE ******.
 */
#define EEPROM    	((struct eeprom *)		0xFEF04000)
/*
 * 	Various constants for mapping in the Sun 3x Architecture
 */
#define ADDRSIZE	(0x100000000)	/* 32-bit addresses */
#define PAGESIZE	(0x2000) 	/* 8k pages */
#define COLORMAP_END    (u_short *)(COLORMAP_BASE) + 0x300 /* Dac's map end */

/*
 ***********************************************************
 *
 *	Physical Addresses of PEGASUS Devices
 *
 ***********************************************************
 */
#define pa_MAINMEM			0x00000000/* Main Memory	     */
#define pa_GRAPHICSACC			0x40000000/* Graphics Accellerator   */
#define pa_P4_COLORMAP			0x50200000/* P4 DAC Color Map	     */
#define pa_P4_REG			0x50300000/* P4 Register 	     */
#define pa_P4_OVERLAY			0x50400000/* P4 Overlay Mono Begin   */
#define pa_P4_ENABLE			0x50600000/* P4 Enable Plane 128K    */
#define pa_COLOR			0x50800000/* P4 Color Plane (1 Meg)  */
#define pa_FPA				0x5C000000/* FPA (On Board)	     */
#define pa_IOMAPPER			0x60000000/* I/O Mapper Long Word    */
#define pa_SYS_ENABLE	((short *)	0x61000000)/* System Enable Register */
#define pa_BUSERR_REG	((char *)	0x61000400)/* Bus Error Register     */
#define pa_LED_REG	((char *)	0x61000800)/* Diag Register 'LEDS'   */
#define pa_IDPROM	((char *)	0x61000C00)/* ID-PROM 		     */
#define pa_MEM_ERR			0x61001000/* Memory Error Register   */
#define pa_INT_REG			0x61001400/* Interrupt Register      */
#define pa_KBM				0x62000000/* Keyboard and Mouse Ports*/
#define pa_SCC				0x62002000/* SCC Cntrl & Status      */
#define pa_EPROM			0x63000000/* E-Prom                  */
#define pa_EEPROM			0x64000000/* EE-PROM                 */
#define pa_CLOCK			0x64002000/* TOD 	 	     */
#define pa_INTEL_ETHER			0x65000000/* Intel Ethernet          */
#define pa_AMD_ETHER			0x65002000/* AMD Ethernet            */
#define pa_DES				0x66002000/* Data Encrypt Processor  */
#define pa_SCSI				0x66000000/* SCSI Interface          */
#define pa_CACHETAGS			0x68000000/* Central Cache Tags      */
#define pa_CACHEDATA			0x69000000/* Central Cache Data      */
#define pa_CACHETAGS_END	pa_CACHETAGS + 0x10000/* End of Tags Physical*/
#define pa_ECC_ENABLE			0x6A1E0000/* Ecc Mem Enable Register */
#define pa_IOTAGS			0x6C000000/* I/O Cache Tags          */
#define pa_IODATA			0x6C002000/* I/O Cache Data          */
#define pa_FLSHIOCACHE			0x6D000000/* Flush I/O Cache Block   */
#define pa_VMEA16D16			0x7C000000/* VME Address 16 Data 16  */
#define pa_VMEA16D32			0x7D000000/* Added for 7053 cntlr    */
#define pa_VMEA24D16			0x7E000000/* VME Address 24 Data 16  */
#define pa_VMEA24D32			0x7F000000/* VME Address 24 Data 32  */
#define pa_VMEA32D32                    0x80000000/* VME Address 32 Data 32  */
/*
 * VME color board physical addresses
 */
#define VME_COLOR_PHYS                  0x7E400000/* Base addr of VME color  */
#define PME_COLOR_STAT                  0x7E709041/* Status Reg         */
#define PME_COLOR_ZOOM                  0x7E70C041/* Zoom Reg           */
#define PME_COLOR_WPAN                  0x7E70A041/* Word Pan           */
#define PME_COLOR_PPAN                  0x7E70C041/* pixel pan          */
#define PME_COLOR_VZOOM                 0x7E70E041/* Zoom Reg           */
#define PME_COLOR_MAPS                  0x7E710041/* DAC's              */
#define PME_COLOR_MASK                  0x7E70A041/* Color Mask Reg     */
