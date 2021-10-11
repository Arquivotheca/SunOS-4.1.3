#ifndef lint
static  char sccsid[] = "@(#)getidprom.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <mon/cpu.addrs.h>
#include <mon/idprom.h>

getidprom(addr, size)
	char *addr;
	int size;
{
	int i;
	char *idprom;

	if (peek((char *) IDPROM_BASE) != -1)
		idprom = IDPROM_BASE;
	else
		idprom = ID_EEPROM_BASE;
	for (i = 0; i < size; i++)
		*addr++ = *idprom++;
}
