/* @(#)spiftest.h 1.1 7/30/92 Copyright 1989 Sun Microsystems. */

/*
 * spiftest.h -
 *
 * Header file for spiftest.
 */

/* Error numbers - Sundiag gets -1, 1, 2, 94..99 */
#define	BAD_DEVICE_NAME		3
#define TIMEOUT_ERROR		79
#define SPWN_ERROR		80
#define	OPEN_ERROR		81
#define CLOSE_ERROR		82
#define UNEXP_ERROR		83
#define	WRITE_ERROR		84
#define READ_ERROR		85
#define CMP_ERROR		86
#define PROBE_ERROR		87
#define IOCTL_ERROR		88
#define KILL_ERROR		89

/* Sub-test constants */
#define 	RETRIES		8
#define		TIMEOUT		240
#define 	MAX_TESTS	5
#define		INTERNAL	0
#define		PRINT		1
#define		SP_96		2
#define		DB_25		3
#define		ECHO_TTY	4

/* Other SPIF test constants */
#define		BUF_SIZE     512
#define		MAX_BOARDS	4
#define		MAX_TERMS	8	/* maximum # ports per board */
#define 	LOCK_FILE	"/tmp/spif.lock"
#define		RD_PGM		"./sp_read"
#define		WR_PGM		"./sp_write"

#define		MAX_PRTS	MAX_BOARDS
#define		MAX_TASK_NO	MAX_BOARDS*MAX_TERMS
#define		TOTAL_IO_PORTS	MAX_BOARDS*MAX_TERMS
#define		MAX_BYTES	100
#define		MAX_RUNS	100
#define		WORD		4

#define		READY		1
#define		START		2
#define 	PASS		0
#define 	FAIL		-1

#define		ON		1
#define		OFF		0

#define		SAME		0

#define		MAXLINE_LEN	80
#define		NAME_LEN	30
#define		ROOT_ID		0

/* CD180 related */
#define CD180_REV	0x81
#define LLOOP		0x10
#define COR2_CHNG	0x42

/* Printer Test related */
#define PP_LINE		64
#define PSIZE		24	/* lines per page of test */
#define ETX		3

typedef	struct {
	int	rpid;
	int	wpid;
	int	rstatus;
	int	wstatus;
	int	rport;
	int	wport;
}	sp_child_t;


typedef	struct {
	int	fd;
	int	selected;
	struct	termios	termios;
}	sp_config;

/* typedef	struct	termios	termios; */
struct termios termios;

/* global declarations */
static	sp_config  sp_ports[TOTAL_IO_PORTS];
static sp_child_t rw_child_pid[MAX_TASK_NO];
static char dev_name[NAME_LEN];
struct stc_defaults_t sd;
struct ppc_params_t pd;
int spif[MAX_BOARDS];
int dev_num;
int testid[MAX_TESTS];
int parallel;
int current_test = 0;
unsigned long data;
int baud, csize, sbit, parity, flow;
char parity_str[NAME_LEN], flow_str[NAME_LEN];
