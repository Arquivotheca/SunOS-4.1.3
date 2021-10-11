/*static char     fpamfsccsid[] = "@(#)fpa.def.h 1.2 2/28/86 Copyright Sun Microsystems"; 
*/ 
/*
 * ======================================================
 * defines for FPA register structure 
 * ======================================================
 */
#define FPA_BASE 			0xE0000000

#define FPA_HANG_DP_1			0xE0000004

#define MODE_WRITE_REGISTER		0xE00008D0
#define WSTATUS_WRITE_REGISTER          0xE0000958
#define LOOP_DIAG_ADDR			0x968
#define DIAG_INIT_CMD                   0xE0000978

#define REGISTER_ZERO_MSW		0xE0000C00
#define REGISTER_ZERO_LSW		0xE0000C04
#define REGISTER_ONE_MSW		0xE0000C08
#define REGISTER_ONE_LSW		0xE0000C0C
#define REGISTER_TWO_MSW		0xE0000C10
#define REGISTER_TWO_LSW		0xE0000C14
#define REGISTER_THREE_MSW		0xE0000C18
#define REGISTER_THREE_LSW		0xE0000C1C
#define REGISTER_FOUR_MSW		0xE0000C20
#define REGISTER_FOUR_LSW		0xE0000C24
#define REGISTER_FIVE_MSW               0xE0000C28
#define REGISTER_FIVE_LSW               0xE0000C2C
#define REGISTER_SIX_MSW                0xE0000C30
#define REGISTER_SIX_LSW                0xE0000C34
#define REGISTER_SEVEN_MSW              0xE0000C38
#define REGISTER_SEVEN_LSW              0xE0000C3C
#define REGISTER_EIGHT_MSW              0xE0000C40
#define REGISTER_EIGHT_LSW		0xE0000C44
#define REGISTER_NINE_MSW               0xE0000C48
#define REGISTER_NINE_LSW               0xE0000C4C
#define REGISTER_TEN_MSW                0xE0000C50
#define REGISTER_TEN_LSW                0xE0000C54
#define REGISTER_ELEVEN_MSW             0xE0000C58
#define REGISTER_ELEVEN_LSW             0xE0000C5C
#define REGISTER_TWELVE_MSW             0xE0000C60
#define REGISTER_TWELVE_LSW             0xE0000C64
#define REGISTER_THIRTEEN_MSW		0xE0000C68
#define REGISTER_THIRTEEN_LSW		0xE0000C6C
#define REGISTER_FOURTEEN_MSW           0xE0000C70
#define REGISTER_FOURTEEN_LSW           0xE0000C74
#define REGISTER_FIFTEEN_MSW            0xE0000C78
#define REGISTER_FIFTEEN_LSW            0xE0000C7C
#define REGISTER_SIXTEEN_MSW            0xE0000C80
#define REGISTER_SIXTEEN_LSW            0xE0000C84

#define SHADOW_RAM_START		0xE0000E00
#define FPA_HANG_DP_2			0xE0001000

#define FPA_CONTEXT_NUM 		0xEC0

#define FPA_STATE 			0xF10
#define         FPA_STATE_ACCENB 	0x0040
					/* Regram enable bit - bit 6    */
		                        /*   0 - disables access to reg ram via pointer */
                     	                /*   1 - enables access */

#define         FPA_STATE_LODENB 	0x0080
				        /* Load enable bit  - bit 7        */
		                        /*   0 - disables access to microstore */
                   		        /*   1 - enables access to microstore */
#define FPA_IMASK 			0xF14
#define         FPA_IMASK_INXACT 	0x0001
					/* inexact error mask - bit 0 */
                                        /*   0 - errors are disabled */
                                        /*   1 - errors are enables  */

#define FPA_LOAD_PTR 			0xF18
#define         FPA_LOAD_PTR_BITMSK 	0x0003
					/* segment select  - bits 1:0 */
                                        /*    00 - bits 71:64 of microstore */
                                        /*    01 - bits 63:32 of microstore */
                                        /*    10 - bits 31:0  of microstore */
                                        /*    11 - bits 23:0  of microstore */

#define         FPA_LOAD_PTR_LOWBIT 	0x0000
#define         FPA_LOAD_PTR_MIDBIT 	0x0001
#define         FPA_LOAD_PTR_HIGBIT 	0x0002
#define         FPA_LOAD_PTR_MAPRAM 	0x0003
#define         FPA_LOAD_PTR_ADDMSK 	0x3FFC
					/* Ram word addrress  - bits 13:2  */
                                        /*    Mapping ram is 4k X 24 bits */
                                        /*    Microstore  is 4k X 72 bits */
                                        /*    Register ram   2k X 64 bits */

#define         FPA_LOAD_PTR_UCDMAP 	0x8000

#define	FPA_IERR 			0xF1C
#define 	FPA_ILL_CONACC          0x80 
					/* illegal control register address - bit 23 */
/*#define 	FPA_RETRY               0x40 
*/				        /* 256th retry - bit 22 */
/*#define 	FPA_HUNG_PIPE           0x20
*/				        /* hung pipe - pipe access, shadow access, */
                                        /*             control access - bit 21 */
/*#define 	FPA_ILLEGAL_SEQ         0x10 
*/				        /* Illegal access sequence - bit 20 */
/*#define 	FPA_EXEC_UCODE          0x08 
*/				        /* attempt to executte via microcode when the */
                                        /*   load enable bit is set - bit 19 */
/*#define 	FPA_ILLEGAL_ACCESS      0x04
*/				        /* illegal acces - bit 18 */
                                        /*  reading a write only addrress */
                                        /*  writing a read only address */
/*#define 	FPA_PROTECTION          0x02
*/				        /* protection violation - bit 17 */
                                        /*  user write to supervisor space */
                                        /*  attempt to write reg/ram without */
                                        /*    regram-accfess enable bit set */
                                        /*  attempt to access ustore/map ram */
                                        /*    without load enable bit set */
                                        /*  attempt to access illegal address */
/*#define FPA_NON32_ACCESS                0x01 
*/				        /* non - 32 bit access - bit 16 */
/* **************

#define		FPA_IERR_USRACC 	0x0001
#define		FPA_IERR_WRTREG 	0x0002
#define		FPA_IERR_WRTUCD 	0x0004
#define		FPA_IERR_ILLRED 	0x0008
#define		FPA_IERR_ILLWRT 	0x0010
#define		FPA_IERR_ILLSEQ 	0x0020
#define		FPA_IERR_NON32B 	0x0001
#define		FPA_IERR_PARERR 	0x0080
#define		FPA_IERR_ILLEXC 	0x0100
#define		FPA_IERR_TIMOUT 	0x0200

*****************/
#define FPA_PIPE_ACT_INS 		0xF20
#define FPA_PIPE_NXT_INS 		0xF24
#define FPA_PIPE_ACT_D1 		0xF28
#define FPA_PIPE_ACT_D2 		0xF2C
#define FPA_PIPE_NXT_D1 		0xF30
#define FPA_PIPE_NXT_D2 		0xF34

/*
 * 					For PIPE_ACT_INS, PIPE_NXT_INS registers (read only )
 */
/*#define 	FPA_FIRST_V             0x80000000 
*/					/* bits 15 and 31 (V) of these registers */
/*#define 	FPA_FIRST_HALF          0x7FFF0000 
*/					/* indicate whether that instruction is valid */
/*#define 	FPA_SECOND_V            0x00008000 
*/					/* instruction previously issued by the processor */
/*#define 	FPA_SECOND_HALF   	0x00007FFF
*/					/* |V| = 1 instruction was not valid and should */
                                        /*         not be rretransmitted. */
                                        /* |V| = 0 instruction was valid and should be */
                                        /*         reexecuted or retransmitted.        */
     					/* Each instruction half is the address to */
					/* which that access was originally */
     					/* written (the |V| bit is appended).  To retransmit*/
					/* an instruction a PIPE_XXX_Dy  */
     					/* register is written to the address specified */
					/* by the corresponding half of the */
     					/* corresponding PIPE_XXX_INS register.  */
					/* This address should be offset by   */
     					/* 0xE0000000.  There is no need to mask off the */
					/* |V| bit because it will be */
    					/* ignored by the hardware.  */

#define FPA_MODE3_0S 			0xF38

#define FPA_WSTATUSS 			0xF3C
/***************
#define		FPA_WSTATUS_WTKERR      0x8000
#define		FPA_WSTATUS_PARMSK 	0x0F00
#define		FPA_WSTATUS_PARBNK 	0x0800
#define		FPA_WSTATUS_WTSTAT 	0x0F00
#define		FPA_WSTATUS_WTDCOD 	0x001F
****************/
/*
 * 					For WSTATUS Register 
 *					       (register can be read from two address)
 *                          		1. requires clear pipe before data is returned
 *                          		2. requires stable pipe only
 */ 
/*#define 	FPA_WEITEK_ERROR        0x08000   
*/					/* Weitek error (excluding unimpl error) - bit 15 */
                                        /*   0 - error,   1 - no error */
/*#define 	FPA_NONEXIST_INSTR      0x04000    
*/					/* unimplemented instruction - bit 14 */
/*#define 	FPA_STATUS_VALID        0x02000   
*/					/* status valid : bit - 13 */
                                        /*  On power up and after a clear pipe tthe */
                                        /*   weitek chip is invalid until this bit is set */
                                        /*   The bit is set every time WSTATUS register is */
                                        /*   updatted by the microcode.   */

/*#define 	FPA_WEITEK_STATUS       0x00F00    
*/					/* status taken directly from weitek chip - bits 11:8 */
/*#define 	FPA_DECODED_STATUS      0x0001F   
*/					/* decoded status :   bits 4:0    */
        				/*
         				* Weitek comparision conditions 
         				*/
/*#define 	FPA_EQ                  0x00004     equal */ 
/*#define 	FPA_LT                  0x00019     less than */
/*#define 	FPA_GT                  0x00000     greater than */
/*#define 	FPA_UNORDERED           0x00002     unorrdered */
/*#define 	FPA_OTHERS              0x00000     for other values  */

#define FPA_PIPE_STATUS 		0xF48
/*
 *
 * 					For PIPE_STATUS register (read only)
 *
 */
/*#define 	FPA_SECOND_V_NXT        0x80  
*/					/* |V| bit from PIPE_NXT_INS - 2nd half , bit 23 */
/*#define 	FPA_FIRST_V_NXT         0x40    
/*					/*    ditto                  - 1st half , bit 22 */
/*#define 	FPA_SECOND_V_ACT        0x20    
*/					/* |V| bit from PIPE_ACT_INS - 2nd half , bit 21 */
/*#define 	FPA_FIRST_V_ACT         0x10    
*/					/*     ditto                 - 1st half , bit 20 */
/*#define 	FPA_IDLE2               0x08    
*/					/* idle2 bit from command register , bit 19 */
/*#define 	FPA_IDLE1               0x04    
*/					/* idle1 bit from command register , bit 18 */
/*#define 	FPA_HUNG_BIT            0x02    
*/					/* hung bit from command register , bit 17 */
#define 	FPA_STABLE_BIT          0x01    
					/* stable bit - bit 16 */
                                        /*  0 - pipe may change the state */
                                        /*  1 - pipe is :           */
                                        /*       clear (no pending instructions) */
                                        /*       hung  (a Werr has occured)  */
                                        /*   	 waiting for 2nd half of instr  */ 

#define FPA_CMMND_REG_1 		0xF49
#define FPA_CMMND_REG_2 		0xF4A
#define FPA_CMMND_REG_3 		0xF4B

#define FPA_REG_RAM_ADDR 		0xF4D
#define FPA_READ_REG 			0xF60
#define FPA_UST_ADDR 			0xF64
/*
 * 					For REG_UST_ADDR register (read only) 
 *
 */
#define 	FPA_REGRAM_MUX          0xE0000000  
					/* current regram address mux select bits */
                                        /*   (from ustore)  , bits 31:29 */
/*#define 	FPA_REGRAM_ADDR         0x0FFF0000  
*/					/* current regram address (from mux) - bits 27:16 */
/*#define 	FPA_USTORE_ADDR         0x00003FFF  
*/					/* current ustore address (from mux) - bits 13:00 */

#define FPA_STABLE_PIPE_STATUS          0xF68
#define FPA_CLR_PIPEHARD 		0xF80
#define FPA_CLEAR_PIPE 			0xF84
#define FPA_MODE3_0C 			0xFB8
#define FPA_WSTATUSC 			0xFBC
#define FPA_LD_RAM                      0xFC0
#define FPA_ID_REG 			0xFC1



