/*
 * @(#)ipctest.h 1.1 7/30/92
 *
 * ipctest.h:  header file for ipctest.c and pctool-listener.c.
 */

#define		SYSEXDIR	"\\usr\\pctool\\sysex"
#define		LISTNER		"pctool-listener"
#define		SOCKNAME	"/tmp/ipctestsoc"	/* socket name */
#define		INFOSIZ		80
#define		VERBOS		0
#define		DONE		1
#define		PC_ERROR	2	/* pctool error code		*/
#define		TERMINATED	3
#define		INVALID		4
#define		LISTEN_ERROR	5	/* pctool-listener received no input */
#define		READ_FAILED	-1
#define		LISTEN_ERRMSG	"LERROR"

/* defines for timeout_code.  When ipctest gets alarm signal during an
 * accept() or read() from socket, the handler checks whether pctool is
 * still running or it has exited.
 */
#define		STILL_RUNNING	1	/* indicates pctool is still running */
#define		CONNECT_FAILED	2

/* Time before alarm signal is sent to ipctest during an accept() call.
 * In verbose mode, pctool sends many messages to ipctest via a socket.
 * In normal mode, pctool sends 1 message after all the tests have been
 * completed (or an error occurred).
 */
#define		VERBOSE_TIMELIM	60
#define		NORMAL_TIMELIM	300
