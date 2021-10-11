/*	@(#)praudit.h 1.1 92/07/30 SMI; c2 secure	*/
/*
 * praudit.c defines, globals
 */

/* DEFINES */

/* 
 * output value types 
 */
#define INT 0
#define UINT 1
#define LONG 2
#define ULONG 3
#define SHORT 4
#define USHORT 5
#define CHAR 6
#define STRING 7
#define HEX 8
#define SHEX 9
#define OCT 10 
#define OUTREC 11

/* 
 * output mode - set with values of command line flags 
 */
#define DEFAULTM 0
#define RAWM  1 
#define SHORTM  2

/* 
 * source of audit file names 
 */
#define PIPEMODE 0
#define FILEMODE 1 


/* 
 * record header field types 
 */
#define RECORD_TYPE 0
#define RECORD_EVENT 1
#define RECORD_TIME 2
#define RECORD_UID 3
#define RECORD_AUID 4
#define RECORD_EUID 5
#define RECORD_GID 6
#define RECORD_PID 7
#define RECORD_ERROR 8
#define RECORD_RETURN 9
#define RECORD_LABEL 10 

/* 
 * used with access() 
 */
#define R_OK  4  /* test for read permission */

/* 
 * used to indicate one line output per item 
 */
#define NOINDEX -1

/* 
 * max. number of audit file names entered on command line 
 */
#define MAXFILENAMES 100 

/* 
 * max. number of variable length audit record data fields 
 */
#define MAXFIELDS 30

/* 
 * max. size of event strings short or long 
 */
#define MAXEVENTSTR 156 

/* 
 * max. size of file name
 */
#define MAXFILELEN MAXPATHLEN+MAXNAMLEN+1 

/* 
 * max. event 
 */ 
#define    MAXEVENT 11 

/* 
 * GLOBALS 
 */
char *sprintf();

/* 
 * indicates source of audit file names 
 */
int file_mode;

/* 
 * initial value for output mode 
 */
int format = DEFAULTM;

/* 
 * output value type 
 */
int uvaltype;

/* 
 * one line output or formatted output 
 */
int ONELINE = 0; 

/* 
 * field seperator for one line per record output 
 */
char SEPERATOR[4] = ",";

/* 
 * used to store value to be output 
 */
union u_tag {
		int int_val;
		u_int uint_val;
		long long_val;
		u_long ulong_val;
		short short_val;
		u_short ushort_val;
		char char_val;
		char *string_val;
} uval;

/* 
 * contains format information for record header fields 
 */
struct outp {
	int pos;
	int printit;
	char *width;
} rec_display[] ={
	0, 1, "15",
	0, 1, "80",
	0, 1, "30",
	0, 0, "15",
	15, 0, "15",
	30, 1, "30",
	0, 0, "15",
	15, 1, "45",
	0, 0, "15",
	15, 1, "45",
	0, 1, "60"
};

#define	PR_MINRECTYPE		1
#define	PR_MAXRECTYPE		63

char *rec_types[] ={
	"invalid record type",
	"access",
	"chmod",
	"chown",
	"creat",
	"fchmod",
	"fchown",
	"ftruncate",
	"link",
	"mkdir",
	"mknod",
	"open",
	"rmdir",
	"rename",
	"stat",
	"symlink",
	"truncate",
	"unlink",
	"utimes",
	"execv",
	"msgconv",
	"msgctl",
	"msgget",
	"msgrcv",
	"msgsnd",
	"semctl",
	"semget",
	"semop",
	"shmat",
	"shmctl",
	"shmdt",
	"shmget",
	"socket",
	"ptrace",
	"kill",
	"killpg",
	"execve",
	"core",
	"adjtime",
	"settimeofday",
	"sethostname",
	"setdomainname",
	"reboot",
	"rebootfail",
	"sysacct",
	"mount_ufs",
	"mount_nfs",
	"mount",
	"unmount",
	"readlink",
	"quota_on",
	"quota_off",
	"quota_set",
	"quota_lim",
	"quota_sync",
	"quota",
	"statfs",
	"chroot",
	"text",
	"chdir",
	"msgctlrmid",
	"semctl3",
	"semctlall",
	"shmctlrmid"
};
