#ifndef lint
static char sccsid[] = "@(#)loadlogo.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include <stdio.h>

int _error();
#define	NO_PERROR	((char *) 0)

/*
 * Icon loader for eeprom command.
 *
 * Based on libsuntool/icon/icon_load.c 10.10 88/02/08
 * See <suntool/icon_load.h> for icon file format.
 */

loadlogo(name, w, h, logo)
	char *name;
	int w, h;
	char *logo;
{
	FILE *f;
	int c = 0;
	int val;
	int icw = 64, ich = 64, bits = 16;
	int count;

	if (!(f = fopen(name, "r")))
		return _error("cannot open %s", name);

	do {
		int val;
		char slash;

		if ((c = fscanf(f, "%*[^DFHVW*]")) == EOF)
			break;

		switch (c = getc(f)) {
		case 'D':
			if ((c = fscanf(f, "epth=%d", &val)) == 1 &&
				val != 1)
				goto badform;
			break;
		case 'F':
			if ((c = fscanf(f, "ormat_version=%d", &val)) == 1 &&
				val != 1)
				goto badform;
			break;
		case 'H':
			c = fscanf(f, "eight=%d", &ich);
			break;
		case 'V':
			c = fscanf(f, "alid_bits_per_item=%d", &bits);
			break;
		case 'W':
			c = fscanf(f, "idth=%d", &icw);
			break;
		case '*':
			c = fscanf(f, "%c", &slash);
			if (slash == '/')
				goto eoh; /* end of header */
			else
				(void) ungetc(slash, f);
			break;
		}
	} while (c != EOF);

eoh:
	if (c == EOF ||
		icw != w || ich != h ||
		bits != 16 && bits != 32) {
badform:
 		(void) fclose(f);
		return _error(NO_PERROR, "header format error in %s", name);
	}

	for (count = ((w + (bits - 1)) / bits) * h; count > 0; count--) {
		c = fscanf(f, " 0x%x,", &val);
		if (c == 0 || c == EOF)
			break;

		switch (bits) {
		case 32:
			*logo++ = val >> 24;
			*logo++ = val >> 16;
			/* fall through */
		case 16:
			*logo++ = val >> 8;
			*logo++ = val;
			break;
		}
	}

	(void) fclose(f);

	if (count > 0)
		return _error(NO_PERROR, "data format error in %s", name);

	return 0;
}
