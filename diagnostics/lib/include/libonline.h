/*      @(#)libonline.h 1.1 92/07/30 SMI          */

/********************************************************************
  libonline.h
  This include file is used for general libonline.a library.
*********************************************************************/
 
#ifndef FALSE
#define FALSE   0 
#endif

#ifndef TRUE
#define TRUE    ~FALSE
#endif

#define MSG_INDENT	2
#define MSG_LINELEN     76
/* architecture code */
#define ARCH2			0
#define ARCH3			1
#define ARCH4			2
#define ARCH386			3
#define ARCH3X			4
#define ARCH4C			5
#define ARCH4M			7

#ifndef IDM_ARCH_SUN4C
#define IDM_ARCH_SUN4C   0x50
#endif

#ifndef IDM_ARCH_SUN386
#define IDM_ARCH_SUN386   0x30
#endif

#ifndef IDM_ARCH_SUN4
#define IDM_ARCH_SUN4   0x20    	/* arch value for Sun-4 */
#endif

#ifndef IDM_ARCH_SUN3X
#define IDM_ARCH_SUN3X   0x40            /* arch value for Sun-3x */
#endif

#ifndef IDM_ARCH_SUN4M
#define IDM_ARCH_SUN4M   0x70            /* arch value for Sun-4m */
#endif

/* common diagnostics error message header for log files */
extern  char    *versionid;
extern  int     test_id;                /* test id for test_name */
extern  int     version_id;             /* test version number (SCCS)*/
extern  int     subtest_id;
extern  int     error_code;
extern  int     error_base;             /* priority of error severity */

/* macro to test sun architecture and unix level, see sdutil.c */
#define is_arch(a) ((sun_arch()==(a))? TRUE : FALSE)
#define is_unix(a) ((strcmp(sun_unix(),(a)))? FALSE : TRUE)

#ifndef MIOCSPAM
#define MIOCSPAM _IOWR(M, 2, unsigned int) /* set processor affinity mask */
#define MIOCGPAM _IOWR(M, 3, unsigned int) /* set processor affinity mask */
#endif

extern int sun_arch();			/* functions defined in sdutil.c */
extern char *sun_unix();
extern void check_superuser();
extern void format_line();
extern char *errmsg();
extern int  get_test_id();
extern int  get_version_id();
extern int get_processors_mask();
extern int get_number_processors();
