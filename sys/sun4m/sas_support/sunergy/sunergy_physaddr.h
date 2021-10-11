/* @(#)sunergy_physaddr.h	1.1 8/6/90 SMI */

/*
 * sunergy_physaddr.h - this information comes from the OBP
 *
 */

/* SingleSPARC module pagesize */
#define P_SSPARC_NBPG		0x1000

/* SSPARC */
#define P_IOMMU_ADDR		(addr_t)0x10000
#define P_IOMMU_SIZE		1
#define P_IOMMU_NBPG		0x1000

/* 
 * OBIO1 space 
 */
#define P_OBIO1_ADDR		(addr_t)0x10000
#define P_OBIO1_SIZE		0xFFFF

#define P_SBUSCTL_OFFSET	(addr_t)0x00001
#define P_SBUSCTL_SIZE		1

/* 
 * OBIO2 space 
 */
#define P_OBIO2_ADDR		(addr_t)0x70000
#define P_OBIO2_SIZE		0xFFFF

/* SLAVIO */
#define P_EEPROM_OFFSET		(addr_t)0x01900
#define P_EEPROM_SIZE		2

/* MACIO */
#define P_COUNTER_OFFSET	(addr_t)0x09300
#define P_COUNTER_SIZE		1
#define P_SYS_COUNTER_OFFSET	(addr_t)0x09310
#define P_SYS_COUNTER_SIZE	1
#define P_INTS_OFFSET		(addr_t)0x09400
#define P_INTS_SIZE		1
#define P_SYS_INTS_OFFSET	(addr_t)0x09410
#define P_SYS_INTS_SIZE		1

/* fake */
#define P_MODID_OFFSET		(addr_t)0x09300 /* kludge */
#define P_MODID_SIZE		1

/* SBUS Space */
#define P_SBUS_ADDR		(addr_t)0x30000
#define P_SBUS_SIZE		0x4ffff



