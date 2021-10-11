#ident	"@(#)st_conf.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990, 1991 by Sun Microsystems, Inc.
 */

#include "st.h"
#if	NST > 0

#include <scsi/scsi.h>
#include <scsi/targets/stdef.h>

/*
 * Tunable parameters. See <scsi/targets/stdef.h>
 * for what the initial values are.
 */

int st_retry_count	= ST_RETRY_COUNT;
int st_io_time		= ST_IO_TIME;
int st_space_time	= ST_SPACE_TIME;
int st_error_level	= STERR_RETRYABLE;

/*
 * Drive Tables.
 *
 * The structure and option definitions can be
 * found in <scsi/targets/stdef.h>.
 *
 * Note: that blocksize should be a power of two
 * for fixed-length recording devices.
 *
 * Note: the speed codes are unused at present.
 * The driver turns around whatever is reported
 * from the drive via the mode sense.
 *
 * Note: the read retry and write retry counts
 * are there to provide a limit until warning
 * messages are printed.
 *
 *
 * Note: For drives that are not in this table....
 *
 * The first open of the device will cause a MODE SENSE command
 * to be sent. From that we can determine block size. If block
 * size is zero, than this drive is in variable-record length
 * mode. The driver uses the SCSI-2 specification density codes in
 * order to attempt to determine what kind of sequential access
 * device this is. This will allow determination of 1/4" cartridge,
 * 1/2" cartridge, some helical scan (3.81 && 8 mm cartridge) and
 * 1/2" reel tape devices. The driver will print what it finds and is
 * assuming to the console. If the device you have hooked up returns
 * the default density code (0) after power up, the drive cannot
 * determine what kind of drive it might be, so it will assume that
 * it is an unknown 1/4" cartridge tape (QIC).
 *
 * If the drive is determined in this way to be a 1/2" 9-track reel
 * type device, an attempt will be mode to put it in Variable
 * record length mode.
 *
 * Generic drives are assumed to support only the long erase option
 * and will not to be run in buffered mode.
 *
 */

struct st_drivetype st_drivetypes[] = {

/* Emulex MT-02 controller for 1/4" cartridge */
/*
 * The EMULEX MT-02 adheres to CCS level 0, and thus
 * returns nothing of interest for the Inquiry command
 * past the 'response data format' field (which will be
 * zero). The driver will recognize this and assume that
 * a drive that so responds is actually an MT-02 (there
 * is no other way to really do this, ungracious as it
 * may seem).
 *
 */
{
	"Emulex MT02 QIC-11/QIC-24",
	8, "\0\0\0\0\0\0\0\0", ST_TYPE_EMULEX, 512,
	ST_QIC,
	130, 130,
	/*
	 * Note that low density is a vendor unique density code.
	 * This gives us 9 Track QIC-11. Supposedly the MT02 can
	 * read 4 Track QIC-11 while in this mode. If that doesn't
	 * work, change one of the duplicated QIC-24 fields to 0x4.
	 *
	 */
	{ 0x84,  0x05, 0x05, 0x05 },
	{ 0, 0, 0, 0 }
},
/* Archive QIC-150 1/4" cartridge */
{
	"Archive QIC-150", 13, "ARCHIVE VIPER", ST_TYPE_ARCHIVE, 512,
	/*
	 * The manual for the Archive drive claims that this drive
	 * can backspace filemarks. In practice this doens't always
	 * seem to be the case.
	 *
	 */
	(ST_QIC | ST_AUTODEN_OVERRIDE),
	400, 400,
	{ 0x00, 0x00, 0x00, 0x00},
	{  0, 0, 0, 0 }
},
/* HP 1/2" reel */
{
	"HP-88780 1/2\" Reel", 13, "HP      88780", ST_TYPE_HP, 10240,
	(ST_REEL | ST_VARIABLE | ST_BSF | ST_BSR),
	400, 400,
	/*
	 * Note the vendor unique density '0xC3':
	 * this is compressed 6250 mode. Beware
	 * that using large data sets consisting
	 * of repeated data compresses *too* well
	 * and one can run into the unix 2 gb file
	 * offset limit this way.
	 */
	{ 0x01, 0x02, 0x03, 0xC3},
	{  0, 0, 0, 0 }
},
/* Exabyte 8mm 5GB cartridge */
{
	"Exabyte EXB-8500 8mm Helical Scan", 16, "EXABYTE EXB-8500",
	ST_TYPE_EXB8500, 1024,
	(ST_VARIABLE | ST_BSF | ST_BSR | ST_LONG_ERASE),
	5000, 5000,
	{ 0x14, 0x00, 0x8C, 0x8C },
	{  0, 0, 0, 0 }
},
/* Exabyte 8mm 2GB cartridge */
{
	"Exabyte EXB-8200 8mm Helical Scan", 16, "EXABYTE EXB-8200",
	ST_TYPE_EXABYTE, 1024,
	(ST_VARIABLE | ST_BSF | ST_BSR | ST_LONG_ERASE | ST_AUTODEN_OVERRIDE),
	5000, 5000,
	{ 0x00, 0x00, 0x00, 0x00 },
	{  0, 0, 0, 0 }
},
/*
 * The drives below have not been qualified, and are
 * not supported by Sun Microsystems. However, many
 * customers have stated a strong desire for them,
 * so our best guess as to their capabilities is
 * included herein.
 */
/* Wangtek QIC-150 1/4" cartridge */ {
	"Wangtek QIC-150", 14, "WANGTEK 5150ES", ST_TYPE_WANGTEK, 512,
	(ST_QIC | ST_AUTODEN_OVERRIDE),
	400, 400,
	{ 0x00, 0x00, 0x00, 0x00},
	{  0, 0, 0, 0 }
},
/* Kennedy 1/2" reel */
{
	"Kennedy 1/2\" Reel", 4, "KENNEDY", ST_TYPE_KENNEDY, 10240,
	(ST_REEL | ST_VARIABLE | ST_BSF | ST_BSR),
	400, 400,
	{ 0x01, 0x02, 0x03, 0x03 },
	{ 0, 0, 0, 0 }
},
/* CDC 1/2" cartridge */
{
	"CDC 1/2\" Cartridge", 3, "LMS", ST_TYPE_CDC, 1024,
	(ST_QIC | ST_VARIABLE | ST_BSF | ST_LONG_ERASE | ST_AUTODEN_OVERRIDE),
	300, 300,
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0, 0, 0, 0 }
},
/* Fujitsu 1/2" cartridge */
{
	"Fujitsu 1/2\" Cartridge", 2, "\076\000", ST_TYPE_FUJI, 1024,
	(ST_QIC | ST_VARIABLE | ST_BSF | ST_BSR | ST_LONG_ERASE),
	300, 300,
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0, 0, 0, 0 }
},
/* M4 Data Systems 9303 transport with 9700 512k i/f */
{
	"M4-Data 1/2\" Reel", 19, "M4 DATA 123107 SCSI", ST_TYPE_REEL, 10240,
	/*
	 * This is in non-buffered mode because it doesn't flush the
	 * buffer at end of tape writes. If you don't care about end
	 * of tape conditions (e.g., you use dump(8) which cannot
	 * handle end-of-tape anyhow), take out the ST_NOBUF.
	 */
	(ST_REEL | ST_VARIABLE | ST_BSF | ST_BSR | ST_NOBUF),
	500, 500,
	{ 0x01, 0x02, 0x06, 0x06},
	{ 0, 0, 0, 0 }
}
};
int st_ndrivetypes = (sizeof (st_drivetypes)/sizeof (st_drivetypes[0]));

#endif	/* NST > 0 */
