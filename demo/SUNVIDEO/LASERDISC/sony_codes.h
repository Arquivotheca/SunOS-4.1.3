/*	@(#)sony_codes.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * This header file defines all of the Hex codes
 * instructions for the Sony 1550 laser disk player
 */
#ifndef sony_codes_DEFINED
#define	sony_codes_DEFINED

#define	AUDIO_MUTE_ON		0x24	/* Audio mute on */
#define	AUDIO_MUTE_OFF		0x25	/* Audio mute off */
#define	CLEAR_ENTRY		0x41	/* Clear last datum entered */
#define	CH1_ON			0x46	/* Turn channel 1 audio on */
#define	CH1_OFF			0x47	/* Turn channel 1 audio off */
#define	CH2_ON			0x48	/* Turn channel 2 audio on */
#define	CH2_OFF			0x49	/* Turn channel 2 audio off */
#define	CHAPTER_MODE		0x69	/* Set player in current chapter mode */
#define	CLEAR_ALL		0x56	/* Clear mode in SEARCH/REPEAT mode */
#define	CONTINUE		0x61	/* Resume mode prior to STILL */
#define	CX_ON			0x6e	/* Activate CX system */
#define	CX_OFF			0x6f	/* De-activate CX system */
#define	EJECT_ENABLE		0x74	/* Eject enable */
#define	EJECT_DISABLE		0x75	/* Eject disable */
#define	F_FAST			0x3b	/* Fast forward play, CAV only */
#define	R_FAST			0x4b	/* Fast reverse play, CAV only */
#define	F_PLAY			0x3a	/* Normal forward play */
#define	R_PLAY			0x4a	/* Normal reverse play */
#define	F_SCAN			0x3e	/* Scan in forward */
#define	R_SCAN			0x4e	/* Scan in reverse */
#define	F_SLOW			0x3c	/* Slow forward play, CAV only */
#define	R_SLOW			0x4c	/* Slow reverse play, CAV only */
#define	F_STEP_STILL		0x2b	/* Advance 1 frame fwd & still, CAV only */
#define	R_STEP_STILL		0x2c	/* Advance 1 frame bwd & still, CAV only */
#define	FRAME_MODE		0x55	/* Set to frame number mode */
#define	INDEX_ON		0x50	/* Activate index function */
#define	INDEX_OFF		0x51	/* De-activate index function */
#define	MEMORY			0x5a	/* Memorise the current position */
#define	MENU			0x42	/* Search for program area heading */
#define	NON_CF_PLAY		0x71	/* Disengage colour framing */
#define	PSC_ENABLE		0x28	/* Enable picture stop code */
#define	PSC_DISABLE		0x29	/* Disable picture stop code */
#define	STILL			0x4f	/* Still the picture */
#define	STOP			0x3f	/* Stop the playback */
#define	VIDEO_ON		0x27	/* Output video signal */
#define	VIDEO_OFF		0x26	/* Mute video output */

#define	ADDR_INQ		0x60	/* Inquire for current address */
#define	CHAPTER_INQ		0x76	/* Inquire for current chapter number */
#define	EJECT			0x2a	/* Open the disc compartment */
#define	ENTER			0x40	/* Terminate a command */
#define	F_STEP			0x3d	/* Variable forward play, CAV only */
#define	R_STEP			0x4d	/* Varaiable reverse play, CAV only */
#define	F_TRACK_JUMP		0x2d	/* Jump multiple tracks fwd, CAV only */
#define	R_TRACK_JUMP		0x2e	/* Jump multiple tracks bwd, CAV only */
#define	MARK_SET		0x73	/* Set mark position */
#define	M_SEARCH		0x5b	/* Locate the memorised position */
#define	MOTOR_OFF		0x63	/* Stop the motor, returns 2 ACKs  */
#define	MOTOR_ON		0x62	/* Start the motor, returns 2 ACKs */
#define	REPEAT			0x44	/* Playback a sequence */
#define	ROM_VERSION		0x72	/* Inquire which ROM version */
#define	SEARCH			0x43	/* Search for an address */
#define	STATUS_INQ		0x67	/* Inquire status of the player */
#define	USERS_CODE_INQ		0x79	/* Inquire users code */

#define	COMPLETION		0x01	/* Successful operation completion */
#define	ERROR			0x02	/* Error signal */
#define	LID_OPEN		0x03	/* Lid is open */
#define	NOT_TARGET		0x05	/* Target frame not found */
#define	NO_FRAME_NUMBER		0x06	/* Illegal frame number */
#define	MARK_RETURN		0x07	/* Mark set position is found */
#define	ACK			0x0a	/* Acknowledge return signal from disk */
#define	NAK			0x0b	/* No Acknowledge */

#define	CLV_DISK		0x20	/* CLV disk status code */
#define	CAV_DISK		0x00	/* CAV disk status code */
#define MOT_STAT		0x20	/* Motor on off status code */

#define INVALID_SPEED		0x0c	/* Invalid disk speed */

#endif sony_codes_DEFINED
