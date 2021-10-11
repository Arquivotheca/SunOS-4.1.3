#ifndef lint
static  char sccsid[] = "@(#)eeprom.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifdef	sun386

/******************************************************************************
* Notify procedure for the "EEPROM" button.
*
******************************************************************************/
/*ARGSUSED*/
eeprom_proc()
{
	(void)confirm("The EEPROM feature is not available on SunX86!", 1);
}

#else	sun386

#include <stdio.h>
#include <sys/types.h>

#ifdef sun3
#include <sun3/eeprom.h>
#endif
#ifdef sun4
#include <sun4/eeprom.h>
#endif

#include <sys/time.h>

#include "sundiag.h"
#include "../../lib/include/libonline.h"

#define	EE_TRUE	0x12

#define	eeoff	 (char *)&eeprom + (int)&((struct eeprom *)0)

char	*eeprom_dev = "/dev/eeprom";
char buffer[100];	/* Global storage buffer */
char *bufp;		/* Storage buffer pointer */
void i_byte();
char *o_byte();
void i_bool();
char *o_bool();
void i_date();
char *o_date();
void i_bootdev(); 
char *o_bootdev();
void i_console();
char *o_console();
void i_scrsize();
char *o_scrsize(); 	
void i_banner();
char *o_banner();
void i_diagpath();
char *o_diagpath();
void i_baud();
char *o_baud();
char *o_string();

void eefix_chksum();

static Frame eeprom_frame=NULL;
static Panel eeprom_panel;

Panel_item	hwupdate_item;
Panel_item	memsize_item;
Panel_item	memtest_item;
Panel_item	scrsize_item;
Panel_item	watchdog_reboot_item;
Panel_item	default_boot_item;
Panel_item	bootdev_item;	
Panel_item	kbdtype_item;
Panel_item	keyclick_item;
Panel_item	console_item;	
Panel_item	custom_banner_item;
Panel_item	banner_item;
Panel_item	diagdev_item;
Panel_item	diagpath_item;
Panel_item	ttya_rtsdtr_item;
Panel_item	ttyb_rtsdtr_item;
Panel_item	columns_item;
Panel_item	rows_item;


struct	eeprom eeprom;
int	errors = 0;

extern	u_char chksum();

extern	char *ctime(), *strncpy();
extern  long lseek();

/******************************************************************************
 * Notify procedure for the "EEPROM" button.				      *
 ******************************************************************************/

/***** global flag(switch) variables. *****/

int watchdog_reboot_file = 0;	/* Watchdog reboot */
int default_boot_file = 0;	/* Default boot */
int custom_banner_file = 0;	/* Custom logo */
int ttya_rtsdtr_file = 0;	/* Assert rst/dtr on port A ? */
int ttyb_rtsdtr_file = 0;	/* Assert rst/dtr on port B ? */
int keyclick_file = 0;		/* keyboard click */
int memsize_file = 0;		/* Memory size */
int memtest_file = 0;		/* Megabytes of memory to test */
int kbdtype_file = 0;		/* Type of Keyboard ( O for all SUN keyboards ) */
int columns_file = 0;		/* Number of columns on screen */
int rows_file    = 0;		/* Number of rows on screen */
int scrsize_file = 0;		/* screen size */
int console_file = 0;		/* console type */
char bootdev_file[15];		/* Boot device */
char banner_file[81];		/* Banner string */
char diagdev_file[15];		/* Diagnostic boot device */
char diagpath_file[41];		/* Diagnostic boot path */
char hwupdate_file[41];		/* Last EEPROM update */


/***** forward references *****/

int eeprom_done_proc();
static eeprom_default_proc();
static eeprom_cancel_proc();


/*ARGSUSED*/
eeprom_proc()
{
	char tmp1[6];
	int which_row=0;

#ifdef sun4	/* Check for Sun4c */
	if ( cpu_is_sun4c() )
		return;
#endif sun4

	if (running == GO) return;
	/* repaint the eeprom option popup */
	if (eeprom_frame != NULL)
	  frame_destroy_proc(eeprom_frame);

	eeprom_get_proc();	/* read contents of eeprom */
	eeprom_frame = window_create(sundiag_frame, FRAME,
	    FRAME_SHOW_LABEL,	TRUE,
	    FRAME_LABEL,	"EEPROM Option Menu",
	    WIN_X,	(int)((STATUS_WIDTH+PERFMON_WIDTH)*frame_width)+15,
	    WIN_Y,	20,
            FRAME_DONE_PROC, frame_destroy_proc, 0);

	eeprom_panel = window_create(eeprom_frame, PANEL, 0);
	(void)panel_create_item(eeprom_panel, PANEL_MESSAGE,
            PANEL_LABEL_STRING,         "LAST UPDATE:",
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row),
            0);
	(void)panel_create_item(eeprom_panel, PANEL_MESSAGE,
            PANEL_LABEL_STRING,         hwupdate_file,
            PANEL_ITEM_X,               ATTR_COL(18),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	ttya_rtsdtr_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Assert SCC Port A DTR/RTS:    ",
	    PANEL_CHOICE_STRINGS,	"yes ", "no", 0,
	    PANEL_VALUE,		ttya_rtsdtr_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	ttyb_rtsdtr_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Assert SCC Port B DTR/RTS:    ",
	    PANEL_CHOICE_STRINGS,	"yes ", "no", 0,
	    PANEL_VALUE,		ttyb_rtsdtr_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	console_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Console type:                 ",
	    PANEL_CHOICE_STRINGS,	"on-board fb","ttya", "ttyb",
					"VME fb","P4 fb", 0,
	    PANEL_VALUE,		console_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	scrsize_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Screen size:                  ",
	    PANEL_CHOICE_STRINGS,	"1024x1024","1152x900", 
	    				"1600x1280","1440x1440", 0,
	    PANEL_VALUE,		scrsize_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	keyclick_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Keyboard click:               ",
	    PANEL_CHOICE_STRINGS,	"no ", "yes", 0,
	    PANEL_VALUE,		keyclick_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	watchdog_reboot_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Watchdog reboot:              ",
	    PANEL_CHOICE_STRINGS,	"no ", "yes", 0,
	    PANEL_VALUE,		watchdog_reboot_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	default_boot_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Unix boot path:               ",
	    PANEL_CHOICE_STRINGS,	"poll", "eeprom", 0,
	    PANEL_VALUE,		default_boot_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	bootdev_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Unix boot device:             ",
	    PANEL_VALUE,		bootdev_file,
	    PANEL_VALUE_DISPLAY_LENGTH,	9,
	    PANEL_VALUE_STORED_LENGTH,	9,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	custom_banner_item = panel_create_item(eeprom_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Custom banner:                ",
	    PANEL_CHOICE_STRINGS,	"no ", "yes", 0,
	    PANEL_VALUE,		custom_banner_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	banner_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Banner string:                ",
	    PANEL_VALUE,		banner_file,
	    PANEL_VALUE_DISPLAY_LENGTH,	8,
	    PANEL_VALUE_STORED_LENGTH,	80,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	diagpath_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Diagnostic boot path:         ",
	    PANEL_VALUE,		diagpath_file,
	    PANEL_VALUE_DISPLAY_LENGTH,	8,
	    PANEL_VALUE_STORED_LENGTH,	40,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	diagdev_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Diagnostic boot device:       ",
	    PANEL_VALUE,		diagdev_file,
	    PANEL_VALUE_DISPLAY_LENGTH,	9,
	    PANEL_VALUE_STORED_LENGTH,	9,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	(void)sprintf(tmp1,"%u", kbdtype_file);
	kbdtype_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Keyboard type:                ",
	    PANEL_VALUE,		tmp1,
	    PANEL_VALUE_DISPLAY_LENGTH,	1,
	    PANEL_VALUE_STORED_LENGTH,	1,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	(void)sprintf(tmp1,"%u", memsize_file);
	memsize_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Memory size (Mb):             ",
	    PANEL_VALUE,		tmp1,
	    PANEL_VALUE_DISPLAY_LENGTH,	3,
	    PANEL_VALUE_STORED_LENGTH,	3,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	(void)sprintf(tmp1,"%u", memtest_file);
	memtest_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "Memory test size (Mb):        ",
	    PANEL_VALUE,		tmp1,
	    PANEL_VALUE_DISPLAY_LENGTH,	3,
	    PANEL_VALUE_STORED_LENGTH,	3,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	(void)sprintf(tmp1,"%u", columns_file);
	columns_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "# of cols on screen:          ",
	    PANEL_VALUE,		tmp1,
	    PANEL_VALUE_DISPLAY_LENGTH,	3,
	    PANEL_VALUE_STORED_LENGTH,	3,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	(void)sprintf(tmp1,"%u", rows_file);
	rows_item = panel_create_item(eeprom_panel, PANEL_TEXT,
            PANEL_LABEL_STRING,         "# of rows on screen:          ",
	    PANEL_VALUE,		tmp1,
	    PANEL_VALUE_DISPLAY_LENGTH,	2,
	    PANEL_VALUE_STORED_LENGTH,	2,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);
	(void)panel_create_item(eeprom_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,		panel_button_image(eeprom_panel,
					"Default", 7, (Pixfont *)NULL),
	    PANEL_ITEM_X,		ATTR_COL(1),
	    PANEL_ITEM_Y,		ATTR_ROW(which_row),
	    PANEL_NOTIFY_PROC,		eeprom_default_proc,
	    0);

	(void)panel_create_item(eeprom_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,		panel_button_image(eeprom_panel,
					"Done", 7, (Pixfont *)NULL),
	    PANEL_NOTIFY_PROC,		eeprom_done_proc,
	    0);

	(void)panel_create_item(eeprom_panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,		panel_button_image(eeprom_panel,
					"Cancel", 7, (Pixfont *)NULL),
	    PANEL_NOTIFY_PROC,		eeprom_cancel_proc,
	    0);

	window_fit(eeprom_panel);
	window_fit(eeprom_frame);
        (void)window_set(eeprom_frame, WIN_SHOW, TRUE, 0);
	
}

/******************************************************************************
 * Panel notify procedure for the "default" button item in "EEPROM Options"   *
 * popup subwindow.							      *
 ******************************************************************************/

static eeprom_default_proc()
{

/*
 *	Note:	Only these items are changed to default 
 *		values, when the default button is pressed.
 *		The other items in the eeprom menu remain 
 *		the same as the previous eeprom settings.
 */

  (void)panel_set(keyclick_item, PANEL_VALUE, 0, 0);
  (void)panel_set(watchdog_reboot_item, PANEL_VALUE, 0, 0);
  (void)panel_set(default_boot_item, PANEL_VALUE, 0, 0);
  (void)panel_set(custom_banner_item, PANEL_VALUE, 0, 0);
  (void)panel_set(kbdtype_item, PANEL_VALUE, "0", 0);

}


eeprom_get_proc()
{
#ifdef sun4	/* Check for sun4c */
	if ( cpu_is_sun4c() )
		return;
#endif 
	read_eeprom();		/* Read the eeprom contents */
	fix_chksum();		/* Fix checksums if they are wrong */
  	sprintf(hwupdate_file,"%s",o_date(eeoff->ee_diag.eed_hwupdate));
  	sprintf(bootdev_file,"%s", o_bootdev(eeoff->ee_diag.eed_bootdev[0]));
  	sprintf(banner_file,"%s",o_banner(eeoff->ee_diag.eed_banner[0]));
  	sprintf(diagdev_file,"%s", o_bootdev(eeoff->ee_diag.eed_diagdev[0]));
  	sprintf(diagpath_file,"%s", o_diagpath(eeoff->ee_diag.eed_diagpath[0]));
  	watchdog_reboot_file = atoi(o_bool(eeoff->ee_diag.eed_dogaction));
  	default_boot_file = atoi(o_bool(eeoff->ee_diag.eed_defboot));
  	keyclick_file = atoi(o_bool(eeoff->ee_diag.eed_keyclick));
  	custom_banner_file = atoi(o_bool(eeoff->ee_diag.eed_showlogo));
  	ttya_rtsdtr_file =atoi(o_bool(eeoff->ee_diag.eed_ttya_def.eet_rtsdtr));
  	ttyb_rtsdtr_file =atoi(o_bool(eeoff->ee_diag.eed_ttyb_def.eet_rtsdtr));
	memsize_file = (*(u_char *)o_byte(eeoff->ee_diag.eed_memsize));
	memtest_file = (*(u_char *)o_byte(eeoff->ee_diag.eed_memtest));
	kbdtype_file = (*(u_char *)o_byte(eeoff->ee_diag.eed_kbdtype));
	columns_file = (*(u_char *)o_byte(eeoff->ee_diag.eed_colsize));
	rows_file = (*(u_char *)o_byte(eeoff->ee_diag.eed_rowsize));
  	scrsize_file =atoi(o_scrsize(eeoff->ee_diag.eed_scrsize));
  	console_file =atoi(o_console(eeoff->ee_diag.eed_console));
}

/******************************************************************************
 * Panel notify procedure for the "Done" button item in "EEPROM Options"      *
 * popup subwindow.							      *
 ******************************************************************************/

eeprom_done_proc()
{
	(void)strcpy(bootdev_file , (char *)panel_get_value(bootdev_item)) ;
	(void)strcpy(banner_file , (char *)panel_get_value(banner_item)) ;
	(void)strcpy(diagdev_file ,(char *)panel_get_value(diagdev_item)) ;
	(void)strcpy(diagpath_file , (char *)panel_get_value(diagpath_item)) ;
	watchdog_reboot_file = (int)panel_get_value(watchdog_reboot_item) ;
	default_boot_file = (int)panel_get_value(default_boot_item) ;
	keyclick_file = (int)panel_get_value(keyclick_item) ;
	custom_banner_file = (int)panel_get_value(custom_banner_item) ;
	ttya_rtsdtr_file = (int)panel_get_value(ttya_rtsdtr_item) ;
	ttyb_rtsdtr_file = (int)panel_get_value(ttyb_rtsdtr_item) ;
  	memsize_file = atoi(panel_get_value(memsize_item));
  	memtest_file = atoi(panel_get_value(memtest_item));
  	kbdtype_file = atoi(panel_get_value(kbdtype_item));
  	columns_file = atoi(panel_get_value(columns_item));
  	rows_file = atoi(panel_get_value(rows_item));
	scrsize_file = (int)panel_get_value(scrsize_item);
	console_file = (int)panel_get_value(console_item);

	write_eeprom();			/* Write new settings to eeprom. */
	fix_chksum();			/* Fix checksums if wrong. */
		
  (void)window_set(eeprom_frame, FRAME_NO_CONFIRM, TRUE, 0);
  (void)window_destroy(eeprom_frame);
  eeprom_frame = NULL;
}

/* 
 * Do the write to the eeprom structure.
 */
write_eestruct()
{
	i_bootdev(bootdev_file, eeoff->ee_diag.eed_bootdev[0]);
	i_banner(banner_file, eeoff->ee_diag.eed_banner[0]);
	i_bootdev(diagdev_file, eeoff->ee_diag.eed_diagdev[0]);
	i_diagpath(diagpath_file, eeoff->ee_diag.eed_diagpath[0]);
	i_bool(watchdog_reboot_file, eeoff->ee_diag.eed_dogaction);
	i_bool(default_boot_file, eeoff->ee_diag.eed_defboot);
	i_bool(keyclick_file, eeoff->ee_diag.eed_keyclick);
	i_bool(custom_banner_file, eeoff->ee_diag.eed_showlogo);
	i_bool(ttya_rtsdtr_file, eeoff->ee_diag.eed_ttya_def.eet_rtsdtr);
	i_bool(ttyb_rtsdtr_file, eeoff->ee_diag.eed_ttyb_def.eet_rtsdtr);
	i_byte(memsize_file, eeoff->ee_diag.eed_memsize);
	i_byte(memtest_file, eeoff->ee_diag.eed_memtest);
	i_byte(kbdtype_file, eeoff->ee_diag.eed_kbdtype);
	i_byte(columns_file, eeoff->ee_diag.eed_colsize);
	i_byte(rows_file, eeoff->ee_diag.eed_rowsize);
	i_scrsize(scrsize_file, eeoff->ee_diag.eed_scrsize);
	i_console(console_file, eeoff->ee_diag.eed_console);
	i_date(eeoff->ee_diag.eed_hwupdate);
}

/******************************************************************************
 * Panel notify procedure for the "Cancel" button item in "EEPROM Options"      *
 * popup subwindow.							      *
 ******************************************************************************/

static eeprom_cancel_proc()
{
  (void)window_set(eeprom_frame, FRAME_NO_CONFIRM, TRUE, 0);
  (void)window_destroy(eeprom_frame);
  eeprom_frame = NULL;
}

/*
 * Read the EEPROM into *ep.
 */
eeread(ep)
	struct eeprom *ep;
{
	int fd;

	if ((fd = open(eeprom_dev, 0)) < 0) {
		fprintf(stderr, "%s: open error.", eeprom_dev);
		perror(eeprom_dev);
		exit(1);
	}
	if (read(fd, (char *)ep, sizeof (*ep)) != sizeof (*ep)) {
		fprintf(stderr, "%s: read error.", eeprom_dev);
		perror(eeprom_dev);
		exit(1);
	}
	close(fd);
}

/*
 * Write out the new EEPROM *ep if there were any changes.
 * Update the checksums and write counts.
 */

eewrite(ep)
	struct eeprom *ep;
{
	int fd;
	struct eeprom eeorig;
	register char *op, *np;		/* old and new update dates */
	int written = 0;

	if ((fd = open(eeprom_dev, 2)) < 0) {
		fprintf(stderr, "%s: open error.", eeprom_dev);
		perror(eeprom_dev);
		exit(1);
	}
	if (read(fd, (char *)&eeorig, sizeof (eeorig)) != sizeof (eeorig)) {
		fprintf(stderr, "%s: read error.", eeprom_dev);
		perror(eeprom_dev);
		exit(1);
	}
	op = (char *)&eeorig.ee_diag.eed_hwupdate;
	np = (char *)&ep->ee_diag.eed_hwupdate;

	/*
	 * Write diagnostic section.
	 */
	while (op < (char *)&eeorig.ee_resv) {
		if (*np != *op) {
			lseek(fd, (long) (np - (char *)ep), 0);
			write(fd, np, 1);
			written = 1;
		}
		op++;
		np++;
	}
	if (written) {
		/*
		 * Recalculate checksum.
		 */
		u_char *cp;
		u_short *wp;

		cp = &ep->ee_diag.eed_chksum[0];
		cp[0] = cp[1] = cp[2] =
		    chksum((u_char *)&ep->ee_diag.eed_hwupdate,
			sizeof (ep->ee_diag) -
			((char *)(&ep->ee_diag.eed_hwupdate) -
			    (char *)(&ep->ee_diag)));
		lseek(fd, (long) ((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;
		lseek(fd, (long) ((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}
	close(fd);
}

/*
 * Check the write counts and checksums in *ep.
 * Return 0 on success, 1 if write count or checksum is wrong.
 */

eecheck(ep, ign)
	register struct eeprom *ep;
	int ign;
{
	register u_short *wp;
	register u_char *cp;

	/* check diagnostic area */
	wp = ep->ee_diag.eed_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "eeprom: diagnostic area write count mismatch\n");
		if (!ign)
			return (1);
	}
	cp = ep->ee_diag.eed_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "eeprom: diagnostic area checksum mismatch\n");
		if (!ign)
			return (1);
	}
	if (chksum((u_char *)&ep->ee_diag.eed_hwupdate, sizeof (ep->ee_diag) -
	    ((char *)(&ep->ee_diag.eed_hwupdate) - (char *)(&ep->ee_diag))) !=
	    cp[0]) {
		fprintf(stderr, "eeprom: diagnostic area checksum wrong\n");
		if (!ign)
			return (1);
	}

	/* check reserved area */
	wp = ep->ee_resv.eev_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "eeprom: reserved area write count mismatch\n");
		if (!ign)
			return (1);
	}
	cp = ep->ee_resv.eev_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "eeprom: reserved area checksum mismatch\n");
		if (!ign)
			return (1);
	}
	if (chksum((u_char *)&ep->ee_resv.eev_resv[0], sizeof (ep->ee_resv) -
	    ((char *)(&ep->ee_resv.eev_resv[0]) - (char *)(&ep->ee_resv))) !=
	    cp[0]) {
		fprintf(stderr, "eeprom: reserved area checksum wrong\n");
		if (!ign)
			return (1);
	}

	/* check rom area */
	wp = ep->ee_rom.eer_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "eeprom: rom area write count mismatch\n");
		if (!ign)
			return (1);
	}
	cp = ep->ee_rom.eer_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "eeprom: rom area checksum mismatch\n");
		if (!ign)
			return (1);
	}
	if (chksum((u_char *)&ep->ee_rom.eer_resv[0], sizeof (ep->ee_rom) -
	    ((char *)(&ep->ee_rom.eer_resv[0]) - (char *)(&ep->ee_rom))) !=
	    cp[0]) {
		fprintf(stderr, "eeprom: rom area checksum wrong\n");
		if (!ign)
			return (1);
	}

	/* check software area */
	wp = ep->ee_soft.ees_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "eeprom: software area write count mismatch\n");
		if (!ign)
			return (1);
	}
	cp = ep->ee_soft.ees_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "eeprom: software area checksum mismatch\n");
		if (!ign)
			return (1);
	}
	if (chksum((u_char *)&ep->ee_soft.ees_resv[0], sizeof (ep->ee_soft) -
	    ((char *)(&ep->ee_soft.ees_resv[0]) - (char *)(&ep->ee_soft))) !=
	    cp[0]) {
		fprintf(stderr, "eeprom: software area checksum wrong\n");
		if (!ign)
			return (1);
	}
	return (0);
}

/*
 * Fix up the checksums if they are wrong.
 */

void
eefix_chksum(ep)
	register struct eeprom *ep;
{
	register u_short *wp;
	register u_char *cp;
	register u_char calc_sum;		/* calculated checksum */
	int fd;

	if ((fd = open(eeprom_dev, 2)) < 0) {
		fprintf(stderr, "%s: open error.", eeprom_dev);
		perror(eeprom_dev);
		exit(1);
	}

	/* check diag area */
	cp = ep->ee_diag.eed_chksum;
	calc_sum =  chksum((u_char *)&ep->ee_diag.eed_hwupdate, 
	    sizeof (ep->ee_diag) -
	    ((char *)(&ep->ee_diag.eed_hwupdate) - (char *)(&ep->ee_diag)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}

	/* check reserved area */
	cp = ep->ee_resv.eev_chksum;
	calc_sum = chksum((u_char *)&ep->ee_resv.eev_resv[0], 
	    sizeof (ep->ee_resv) -
	    ((char *)(&ep->ee_resv.eev_resv[0]) - (char *)(&ep->ee_resv)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}

	/* check rom area */
	cp = ep->ee_rom.eer_chksum;
	calc_sum = chksum((u_char *)&ep->ee_rom.eer_resv[0], 
	    sizeof (ep->ee_rom) -
	    ((char *)(&ep->ee_rom.eer_resv[0]) - (char *)(&ep->ee_rom)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}

	/* check software area */
	cp = ep->ee_soft.ees_chksum;
	calc_sum = chksum((u_char *)&ep->ee_soft.ees_resv[0],
	    sizeof (ep->ee_soft) -
	    ((char *)(&ep->ee_soft.ees_resv[0]) - (char *)(&ep->ee_soft)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}
	close(fd);
}

u_char
chksum(p, n)
	register u_char *p;
	register int n;
{
	register u_char s = 0;

	while (n--)
		s += *p++;
	return (256 - s);
}

void
i_byte(i, p)
	int i;
	char *p;
{
	*(u_char *)p = i;
}

char *
o_byte(addr)
	char *addr;
{
	bufp=buffer;

	sprintf(buffer,"%s", addr);
	return(bufp);
}

void
i_banner(s, p)
	char *s, *p;
{

	strncpy(p, s, sizeof (eeprom.ee_diag.eed_banner));
}

char *
o_banner(addr)
	char *addr;
{

	return(o_string(addr, sizeof (eeprom.ee_diag.eed_banner)));
}

void
i_diagpath(s, p)
	char *s, *p;
{

	strncpy(p, s, sizeof (eeprom.ee_diag.eed_diagpath));
}

char *
o_diagpath(addr)
	char *addr;
{

	return(o_string(addr, sizeof (eeprom.ee_diag.eed_diagpath)));
}

char *
o_string(addr, len)
	char *addr;
	int len;
{
	bufp=buffer;

	sprintf(buffer,"%s", (addr[0] == '\377' ? "" : addr));
	return(bufp);
}

void
i_bool(i, p)
	int i;
	char *p;
{
	*(u_char *)p = i ? EE_TRUE : 0;
}

char *
o_bool(addr)
	char *addr;
{
	bufp=buffer;

	sprintf(buffer,"%s", *addr == EE_TRUE ? "1" : "0");
	return(bufp);
}

void
i_date(p)
	char *p;
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	*(u_int *)p = tv.tv_sec;
}

char *
o_date(addr)
	char *addr;
{
	bufp=buffer;

	sprintf(buffer,"%s", ctime((time_t *)addr));
	return(bufp);
}

void
i_bootdev(s, p)
	char *s, *p;
{
	char c1, c2;
	int ctlr, unit, part;

	if (sscanf(s, "%c%c(%x,%x,%x)", &c1, &c2, &ctlr, &unit, &part) != 5 &&
	    sscanf(s, "%c%c(0x%x,%x,%x)", &c1, &c2, &ctlr, &unit, &part) != 5) {
		fprintf(stderr, "eeprom: %s: bad boot device\n", s);
		return;
	}
	p[0] = c1;
	p[1] = c2;
	p[2] = ctlr;
	p[3] = unit;
	p[4] = part;
}

char *
o_bootdev(addr)
	char *addr;
{

	/*
	 * Note that we are passed the starting address here, and rely on
	 * knowledge of the adjacency of the fields.
	 */
	bufp=buffer;
	sprintf(buffer,"%c%c(%x,%x,%x)",
	    (addr[0] == '\377' ? '?' : addr[0]), 
	    (addr[1] == '\377' ? '?' : addr[1]), 
	    addr[2], addr[3], addr[4]);
	return(bufp);
}

void
i_console(i, p)
	int i;
	char *p;
{
	int val;

	if ( i == 0 )
		val = EED_CONS_BW;
	else if ( i == 1 )
		val = EED_CONS_TTYA;
	else if ( i == 2 )
		val = EED_CONS_TTYB;
	else if ( i == 3 )
		val = EED_CONS_COLOR;
	else if ( i == 4 )
		val = EED_CONS_P4;
	*(u_char *)p = val;
}

char *
o_console(addr)
	char *addr;
{
	char *val;
	bufp=buffer;

	switch (*addr) {
	case EED_CONS_BW:
	default:
		val = "0";
		break;
	case EED_CONS_TTYA:
		val = "1";
		break;
	case EED_CONS_TTYB:
		val = "2";
		break;
	case EED_CONS_COLOR:
		val = "3";
		break;
	case EED_CONS_P4:
		val = "4";
		break;
	}
	sprintf(buffer,"%s", val);
	return(bufp);
}

void
i_scrsize(i, p)
	int i;
	char *p;
{
	u_char val;

	if ( i == 0 )
		val = EED_SCR_1024X1024;
	else if ( i == 2 )
		val = EED_SCR_1600X1280;
	else if ( i == 3 )
		val = EED_SCR_1440X1440;
	else
		val = EED_SCR_1152X900;
	*(u_char *)p = val;
}

char *
o_scrsize(addr)
	char *addr;
{
	char *s;
	bufp=buffer;

	switch (*addr) {
	case EED_SCR_1024X1024:
		s = "0";
		break;
	case EED_SCR_1152X900:
	default:
		s = "1";
		break;
	case EED_SCR_1600X1280:
		s = "2";
		break;
	case EED_SCR_1440X1440:
		s = "3";
		break;
	}
	sprintf(buffer,"%s", s);
	return(bufp);
}

void
i_baud(s, p)
	char *s, *p;
{
	int val;

	(void) sscanf(s, "%d", &val);
	*((u_char *)p)++ = (val >> 8) & 0xff;
	*(u_char *)p = val & 0xff;
}

char *
o_baud(name, addr)
	char *name, *addr;
{
	int val = 0;
	bufp=buffer;

	val = *((u_char *)addr)++ << 8;
	val += *(u_char *)addr;
	sprintf(buffer,"%s", val);
	return(bufp);
}

read_eeprom()
{
	eeread(&eeprom);
}


write_eeprom()
{
	write_eestruct();	/* write the values to the eeprom structure */
	eewrite(&eeprom);
}

fix_chksum()
{
	eefix_chksum(&eeprom);
}
#endif	sun386

#ifdef sun4
/*
 * Checks if the cpuid matches that of sun4c series machines.
 * Returns 1 if its a sun4c,  0 otherwise.
 */
int
cpu_is_sun4c()
{
	unsigned machine_id;

	machine_id = sun_arch();
	if (machine_id == ARCH4C)	/* If it is a Sun4c */
	{
	  (void)confirm("The EEPROM feature is not available on Sun4c!", TRUE);
	  return(1);
	}
        if (machine_id == ARCH4M)   /* If it is a Sun4m */
        {
          (void)confirm("The EEPROM feature is not available on Sun4m!", TRUE);
          return(1);
        }
	return(0);
}
#endif
