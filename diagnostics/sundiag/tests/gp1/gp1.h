/* @(#)gp1.h 1.1 92/07/30 SMI      */

#define VME24_NOT_OPEN          3
#define NO_MALLOC               4
#define NO_MAP                  5
#define UFILE_NOT_OPEN          6
#define TEST1_VERIFY_ERROR      7
#define TEST1_PATH_ERROR        8
#define TEST2_VERIFY_ERROR      9
#define TEST2_PATH_ERROR        10
#define TEST3_VERIFY_ERROR      11
#define TEST3_PATH_ERROR        12
#define TEST4_VERIFY_ERROR      13
#define TEST4_HUNG_ERROR        14
#define TEST4_PATH_ERROR        15
#define CHECKSUM_ERROR          16
#define STATUS_ERROR            17
#define TEST5_VERIFY_ERROR      18
#define TEST5_HUNG_ERROR        19
#define TEST5_PATH_ERROR        20
#define TEST6_VERIFY_ERROR      21
#define TEST6_HUNG_ERROR        22
#define TEST6_PATH_ERROR        23
#define INT1_ERROR              24
#define TEST7_VERIFY_ERROR      25
#define TEST7_HUNG_ERROR        26
#define INT2_ERROR              27
#define TEST7_PATH_ERROR        28
#define END_ERROR               29


#define	VME_GP1BASE 0x210000		       /* VME bus address and size */
#define	VME_GP1SIZE 0x10000

#define GP1_BOARD_IDENT_REG 0		       /* Microstore Interface
					        * Registers */
#define GP1_CONTROL_REG 1
#define GP1_STATUS_REG 1
#define GP1_UCODE_ADDR_REG 2
#define GP1_UCODE_DATA_REG 3

#define GP1_SHMEM_OFFSET 0x8000		       /* Start of shared memory with
					        * respect to gp1 base address */

#define GP1_CR_CLRIF 0x8000		       /* Control Register Bit Fields */
#define GP1_CR_IENBLE 0x0300
#define GP1_CR_RESET 0x0040
#define GP1_CR_VP_CONTROL 0x0038
#define GP1_CR_VP_STRT0 0x0020
#define GP1_CR_VP_HLT 0x0010
#define GP1_CR_VP_CONT 0x0008
#define GP1_CR_PP_CONTROL 0x0007
#define GP1_CR_PP_STRT0 0x0004
#define GP1_CR_PP_HLT 0x0002
#define GP1_CR_PP_CONT 0x0001

#define GP1_CR_INT_NOCHANGE 0x0000	       /* Values for GP1_CR_IENBLE
					        * field */
#define GP1_CR_INT_ENABLE 0x0100
#define GP1_CR_INT_DISABLE 0x0200
#define GP1_CR_INT_TOGGLE 0x0300

#define GP1_SR_IFLG 0x8000		       /* Status Register Bit Fields */
#define GP1_SR_IEN 0x4000
#define GP1_SR_RESET 0x0400
#define GP1_SR_VP_STATE 0x0200
#define GP1_SR_PP_STATE 0x0100
#define GP1_SR_VP_STATUS 0x00F0
#define GP1_SR_PP_STATUS 0x000F
