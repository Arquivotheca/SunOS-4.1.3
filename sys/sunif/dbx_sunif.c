#ifndef lint
static	char sccsid[] = "@(#)dbx_sunif.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <sunif/if_en.h>

#include "ec.h"
#if NEC > 0
#include <sunif/if_ecreg.h>
#endif NEC > 0

#include "en.h"
#if NEN > 0
#include <sunif/if_enreg.h>
#endif NEN > 0

#include "ie.h"
#if NIE > 0
#include <sunif/if_iereg.h>
#include <sunif/if_mie.h>
#include <sunif/if_obie.h>
#endif NIE > 0

#include "le.h"
#if NLE > 0
#include <sunif/if_lereg.h>
#endif NLE > 0
