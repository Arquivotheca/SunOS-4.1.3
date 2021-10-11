;
;       static  char sccsid[] = "@(#)ewsi_cmds.h 1.2 90/03/09 SMI";
;

;************************************************************************
;	Edge walker commands
; 	These commands are passed thru from the front end to the
;	edge walker.
;*************************************************************************

#define HK_EW_FB		0x20100001
#define HK_EW_NOP		0x24100000
#define HK_EW_PSI		0x050c0801
#define HK_EW_DOT		0x26100003
#define HK_EW_RGBDOT		0x27100006
#define HK_EW_X_VEC		0x28100006
#define HK_EW_Y_VEC		0x29100006
#define HK_EW_X_RGBVEC		0x2a10000c
#define HK_EW_Y_RGBVEC		0x2b10000c
#define HK_EW_TRI		0x2c100010
#define HK_EW_GTRI		0x2e100016
#define HK_EW_GTRI_AA		0x2f100019
#define HK_EW_SET_AA            0x30100000
#define HK_EW_CLR_AA            0x31100000
#define HK_EW_SET_COLOR		0x32100003
#define HK_EW_SEM_FLAG		0x33100000
#define HK_EWSI_SET_COLOR       0x32100003
#define HK_EWSI_SET_DOT         0x261cfc03
#define HK_EWSI_X_MAJOR         0x28106006
#define HK_EWSI_Y_MAJOR         0x29106006
#define HK_EWSI_X_RGBVEC        0x2a10600c
#define HK_EWSI_Y_RGBVEC        0x2b10600c
#define HK_EWSI_VEC_AA          0x28108006
#define HK_EWSI_TRI             0x2c106010
#define HK_EWSI_GTRI		0x2e106016
#define HK_EWSI_GTRI_AA		0x2f106019
#define HK_EW_TST_MD		0x34100000
#define HK_EW_RD_START		0x3c10000a
#define HK_EW_RD_DDX		0x3d100008
#define HK_EW_RD_DDY		0x3e100008
#define HK_EXIT_TSTMD           0x24100000


/*
 *******************************************************************
 *	SI commands
 *******************************************************************
 */
#define HK_SI_NOP		0x050c0000
#define HK_SI_GRD		0x0508a001	/* si gouraud */
#define HK_SI_GRD_AA		0x0508c001	/* si gouraud aa */
#define HK_SI_DOT		0x0508e801	/* si dot, load y */
#define HK_SI_DOT_AA		0x05090801	/* si dot AA, load y */
#define HK_SI_PASS_FB		0x050e9001	/* pass to fb, load x */
#define HK_SI_SET_SEM		0x050ea001	/* set sem flag */
#define HK_SI_VERT_WAIT		0x050ac001	/* vertical wait */
#define HK_SI_TST_MODE		0x050f2001	/* tst mode */
#define HK_SI_READ_ALL		0x050f2001	/* read all accumulator */
#define HK_SI_SET_DELTA		0x05094801	/* load y ,set delta's*/
#define HK_SI_SET_DELTAL        0x05095001      /* load x ,set delta's*/

#define HK_SI_SET_DY		0x05000801	/* load y */
#define HK_SI_SET_DR		0x05000201	/* load r */
#define HK_SI_SET_DG		0x05000101	/* load g */
#define HK_SI_SET_DB		0x05000081	/* load b */
#define HK_SI_SET_DZ		0x05000401	/* load z */
#define HK_SI_SET_DA		0x05000041	/* load a */
#define HK_SI_SET_DX		0x05041001	/* load x , end*/
#define HK_SI_SET_DBE		0x05040081      /* load b, end */
#define HK_SI_SET_DELTAX	0x05096801	/* set deltax,y,z, load y*/
#define HK_SI_SET_DA_AA		0x05098040	/* set delta a, load a */
#define HK_SI_SET_DA_AA_L	0x05040040	/* load aa */
#define HK_SI_SET_SMAJOR	0x251da001	/* set starting major */
#define HK_SI_SET_EMAJOR	0x251dc001	/* ending major */
#define HK_SI_SET_SLOPE		0x251de001	/* slope */
#define HK_SI_SET_LP		0x251e0001	/* set line position */
#define	HK_SI_SET_XMFLAG	0x251a2001	/* set x major flag */
#define HK_SI_CLR_XMFLAG	0x251e4001	/* clear x major flag */
#define HK_SI_SET_ADDR		0x251e7801	/* set address space */
#define HK_SI_SET_COLOR		0x32100003	/* set color */
#define HK_SI_SET_INDEXCL	0x251ee001	/* set index color   */
#define HK_SI_CLR_INDEXCL	0x251f0001	/* clear index color */
#define HK_SI_GEN		0x05082001	/* si general command */
#define HK_SI_LD_Y		0x05000801	/* load y */
#define HK_SI_LD_X		0x05001001	/* load x */
#define HK_SI_LD_A		0x05000041	/* load alpha */
#define HK_SI_LD_ZE		0x05040401	/* load z, end */
#define HK_SI_VEC		0x05086001      /* si vector command */
#define HK_SI_VEC_AA            0x05088001      /* si vector aa command
*/
#define HK_SI_GEN_AA		0x05084001      /* si general AA command */
#define HK_SI_VEC_ALPHA		0x050b4001	/* si vec alpha cmd */

/*
 ******************************************************************
 *	the following commands are for the loading up the fifo's
 *	to be able to test the fifo's with the read accumulator
 *	command.
 ******************************************************************
 */
#define HK_SI_LD_YL             0x050c0801      /* load y */
#define HK_SI_LD_XL             0x050c1001      /* load x */
#define HK_SI_LD_ZL             0x050c0401      /* load z */
#define HK_SI_LD_RL             0x050c0201      /* load r */
#define HK_SI_LD_GL             0x050c0101      /* load g */
#define HK_SI_LD_BL             0x050c0081      /* load b */
#define HK_SI_LD_AL             0x050c0041      /* load alpha */
