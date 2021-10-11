/*	@(#)led.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This file defines the values used in the Sun-2 LED's by the monitor.
 *
 * Unix might use other values, we don't define them here (yet?).
 *
 * It is desirable that these values match those used by Manufacturing's
 * diagnostic PROMs.
 *
 * Values with the high-order bit set are defined in sign-extended form,
 * so our assembler will assemble them into 'moveq' instructions.
 */

#ifndef _mon_led_h
#define _mon_led_h

#define	L_RESET		0xFFFFFFFF	
				/* **** **** Resets set LEDs to this
				   PROM instructions are not being fetched. */
#define	L_INITIAL	0x01	
				/* ---- ---* Initial state on power-up
				   Local memory is not working.	*/
#define	L_GOTMEM	0x03	
				/* ---- --** After local mem verified 
				   Assorted low-level failures? */
#define	L_AFTERDIAG	0x07
				/* ---- -*** After diags, while setting up
				   Bus problems seem to hang here. */
#define	L_RUNNING	0x00	
				/* ---- ---- After reset processing in monitor()
				   System seems to be running OK */
#ifdef sun2 
#define L_HEARTBEAT     0x08
#endif 
#if defined(sun3)  || defined(sun4) || defined(sun3x)
#define L_HEARTBEAT     0x20
#endif 
				/* ---- *--- (OR) ---- --*- Blinks off and on
				   while NMI is OK, System running OK, 
				   monitor taking and handling NMI's */
#define	L_USERDOG	0x02	
				/* ---- --*- Entering user watchdog routine
				   User watchdog routine failed */
#define	L_CONTEXT	0x11
				/* ---* ---* Testing context registers
				   Context register(s) failing test */
#define	L_SM_CONST	0x21
				/* --*- ---* Testing constant data in segmap
				   Seg map data wires shorted or bits bad */
#define	L_SM_DATA	0x23
				/* --*- --** Testing data lines in segmap
				   Seg map data wires shorted */
#define	L_SM_ADDR	0x22
				/* --*- --*- Testing addr-dependency in segmap
				   Seg map addr lines shorted or miswired */
#define	L_PM_CONST	0x31
				/* --** ---* Testing constant data in pgmap
				   Page map data wires shorted or bad chips */
#define	L_PM_DATA	0x33
				/* --** --** Testing data lines in pgmap
				   Page map data wires shorted */
#define	L_PM_ADDR	0x32
				/* --** --*- Testing addr-dependency in pgmap
				   Page map addr lines shorted or miswired */
#define L_PROM		0x40	
				/* -*-- ---- Testing PROM contents
				   Prom decoding broken, pins bent, bad cksm */
#define	L_UART		0x50	
				/* -*-* ---- Testing UART chip(s)
				   Uart chip bad, or decoding bad */
#define	L_M_MAP		0x70
				/* -*** ---- Sizing memory before const test
				   No mem, or mem giving berr or wierdness */
#define	L_M_CONST	0x71
				/* -*** ---* Testing constant data in memory
				   */
#define	L_M_ADDR	0x72
				/* -*** --*- Testing address-dependency in mem
				   */
#define	L_PARITY	0x7F
				/* -*** **** Testing parity circuitry
				   Parity gen or checking messed up */
#define	L_TIMER		0x81
				/* *--- ---* Testing timer chip
				   Timer clk, bus, or interrupts not wkg */

#define	L_DES		0x82
				/* *--- --*- Testing DES chip
				   DES chip failure */

#define L_SETUP_MEM	0xFFFFFFF1
				/* **** ---* Setting up memory after diags
				   Memory failure that hangs CPU */

#define	L_SETUP_MAP	0xFFFFFFF2
				/* **** --*- Setting up maps after diags
				   Map failure, pretty unlikely */

#define	L_SETUP_FB	0xFFFFFFF3
				/* **** --** Setting up frame buffer
				   Frame buffer accesses hang */

#define	L_SETUP_KEYB	0xFFFFFFF4
				/* **** -*-- Setting up NMI or keyboard
				   Timer chip or keyboard bad */
#define L_INIT_MEM      0x15 
                                /* ---* -*-* Initializing Memory before
                                   attempting to boot in UNIX. */

/* Please define and implement other values! */

#endif /*!_mon_led_h*/
