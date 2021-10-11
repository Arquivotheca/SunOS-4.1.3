/*      @(#)password.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * password.h  contains information for handling password in PROM.
 */

#define PW_SIZE 8				/* max size of a password entry */

#define NON_SECURE_MODE         0x00            /* Non-Secure Mode */
#define COMMAND_SECURE_MODE	0x01		/* Command Secure Mode */
#define FULLY_SECURE_MODE	0x5E		/* Fully Secure Mode */
						/* 0, 2-0x5D, 0x5F-0xFF Non Secure*/
  struct password_inf {
	unsigned short 	bad_counter;		/* illegal password count */
	char 			pw_mode;	/* Contains mode */
						/* 0x01; Command Secure */
						/* 0x5E; Fully Secure */
	char 			pw_bytes[PW_SIZE];/* password */
	};
#define pw_seen gp->g_pw_seen 			/* Declared in globram.h */
#define MAX_COUNT 65535				/* Maximum number of bad entries */
#define SEC_10  10000 				/* Delay count after each password try*/
#define NULL 0 

#define PW_LOWER_LIMIT        (unsigned char *)EEPROM_BASE+0x490
#define PW_UPPER_LIMIT        (unsigned char *)PW_LOWER_LIMIT+sizeof(struct password_inf)
