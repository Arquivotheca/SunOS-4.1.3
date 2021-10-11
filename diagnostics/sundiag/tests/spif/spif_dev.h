/* @(#)spif_dev.h 1.1 7/30/92 Copyright 1986 Sun Microsystems,Inc. */

/*
 * spif_dev.h -
 *
 * SPIFF serial port and parallel port device names
 *
 */


char	*spif_dev_name[] = {
	/* 0 */		"/dev/stc0",
	/* 1 */		"/dev/stc1",
	/* 2 */		"/dev/stc2",
	/* 3 */		"/dev/stc3",
};


char 	*sp_dev_name[] = {
#ifdef IN [
    /* 	
     * Serial port dial-in tty lines -
     * Can be opened only when CD is asserted.  Once
     * opened, corresponding ttyyn lines cannot be opened.
     */
	/* SBUS board #1 */
	/* 0 */		"/dev/ttyy00",
	/* 1 */		"/dev/ttyy01",
	/* 2 */		"/dev/ttyy02",
	/* 3 */		"/dev/ttyy03",
	/* 4 */		"/dev/ttyy04",
	/* 5 */		"/dev/ttyy05",
	/* 6 */		"/dev/ttyy06",
	/* 7 */		"/dev/ttyy07",
	/* SBUS board #2 */
	/* 8 */		"/dev/ttyy08",
	/* 9 */		"/dev/ttyy09",
	/* 10 */	"/dev/ttyy0a",
	/* 11 */	"/dev/ttyy0b",
	/* 12 */	"/dev/ttyy0c",
	/* 13 */	"/dev/ttyy0d",
	/* 14 */	"/dev/ttyy0e",
	/* 15 */	"/dev/ttyy0f",
	/* SBUS board #3 */
	/* 16 */	"/dev/ttyy10",
	/* 17 */	"/dev/ttyy11",
	/* 18 */	"/dev/ttyy12",
	/* 19 */	"/dev/ttyy13",
	/* 20 */	"/dev/ttyy14",
	/* 21 */	"/dev/ttyy15",
	/* 22 */	"/dev/ttyy16",
	/* 23 */	"/dev/ttyy17",
	/* SBUS board #4 */
	/* 24 */	"/dev/ttyy18",
	/* 25 */	"/dev/ttyy19",
	/* 26 */	"/dev/ttyy1a",
	/* 27 */	"/dev/ttyy1b",
	/* 28 */	"/dev/ttyy1c",
	/* 29 */	"/dev/ttyy1d",
	/* 30 */	"/dev/ttyy1e",
	/* 31 */	"/dev/ttyy1f",

#else ][
    /* 
     * Serial port dial-out tty lines -
     * Can be opened when CD is not asserted. Once
     * opened, corresponding ttyyn cannot be opened.
     * Use these lines for echo tty tests.
     */
	/* SBUS board #1 */
	/* 0 */		"/dev/ttyz00",
	/* 1 */		"/dev/ttyz01",
	/* 2 */		"/dev/ttyz02",
	/* 3 */		"/dev/ttyz03",
	/* 4 */		"/dev/ttyz04",
	/* 5 */		"/dev/ttyz05",
	/* 6 */		"/dev/ttyz06",
	/* 7 */		"/dev/ttyz07",
	/* SBUS board #2 */
	/* 8 */		"/dev/ttyz08",
	/* 9 */		"/dev/ttyz09",
	/* 10 */	"/dev/ttyz0a",
	/* 11 */	"/dev/ttyz0b",
	/* 12 */	"/dev/ttyz0c",
	/* 13 */	"/dev/ttyz0d",
	/* 14 */	"/dev/ttyz0e",
	/* 15 */	"/dev/ttyz0f",
	/* SBUS board #3 */
	/* 16 */	"/dev/ttyz10",
	/* 17 */	"/dev/ttyz11",
	/* 18 */	"/dev/ttyz12",
	/* 19 */	"/dev/ttyz13",
	/* 20 */	"/dev/ttyz14",
	/* 21 */	"/dev/ttyz15",
	/* 22 */	"/dev/ttyz16",
	/* 23 */	"/dev/ttyz17",
	/* SBUS board #4 */
	/* 24 */	"/dev/ttyz18",
	/* 25 */	"/dev/ttyz19",
	/* 26 */	"/dev/ttyz1a",
	/* 27 */	"/dev/ttyz1b",
	/* 28 */	"/dev/ttyz1c",
	/* 29 */	"/dev/ttyz1d",
	/* 30 */	"/dev/ttyz1e",
	/* 31 */	"/dev/ttyz1f",
#endif IN ]
};


char	*pp_dev_name[] = {
        /* parallel port for three SBus board */
	/* 0 */		"/dev/stclp0",
	/* 1 */		"/dev/stclp1",
	/* 2 */		"/dev/stclp2",
	/* 3 */		"/dev/stclp3"
};
