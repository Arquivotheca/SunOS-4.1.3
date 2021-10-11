/*	%Z%%M% %I% %E% SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lsdata.c	1.6"

/*
 *	network listener process global initialized data
 */

#include	<fcntl.h>

#define GLOBAL_DATA

#include	"lserror.h"

char Usage[] = "Use nlsadmin(1M) to start the listener process";

errlist err_list[] = {

/* error message						exit code */

{(char *)0,						0},
{Usage,							1}, /* E_CMDLINE */
{"cannot change directory to home",			2}, /* E_CDHOME  */
{"cannot create a required file",			3}, /* E_CREAT	 */
{"cannot access or execute a file",			4},
{"cannot open a required file",				5},
{"cannot initialize properly (listener can't fork itself)",6},
{"cannot initialize properly (pidfile write)",		7},

{"cannot open channel to network (FD1)",			11}, 
{"cannot open channel to network (FD2)",			12}, 
{"cannot open channel to network (FD3)",			13},
{"uname system call error",				14},
{"caught SIGTERM (exiting)",				15},

{"data base and/or cmd line inconsistency",		99},

{"TLI t_alloc failed",					101},
{"TLI t_bind failed",					102},
{"TLI bound a different name than requested",		103},
{"TLI t_free failed",					104},
{"System call failed while in a TLI routine",		105},
{"TLI t_listen failed",					106},
{"TLI t_accept failed",					107},
{"TLI t_snddis failed",					108},
{"TLI t_rcv failed",					109},
{"TLI t_snd failed",					110},

{"Software bug -- should not happen",			201},

{"Login service request; no intermediary process",	51},
{"Error during fork() to start a service",		52},
{"Error trying to rcv message to start a service",	53},
{"Timed out trying to rcv message to start a service",	54},

{"An error occurred during network initialization",	61},

{"An I/O error occurred while reading the listener data base", 71},

{"ATT service: unknown version",			81},
{"ATT service: bad message format",			82},

{"System error",					91},
{"Cannot allocate enough memory for data base",		92},
{"System error: poll failed",				93},
{"cannot allocate enough memory",			36},
{"TLI t_rcvdis failed",					111},
{"TLI t_look failed",					38},
{"Database file has been corrupted",			39},
{"Database file is not at the current version",		40},
};

