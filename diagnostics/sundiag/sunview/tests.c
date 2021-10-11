#ifndef	lint
static	char sccsid[] = "@(#)tests.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#include "sundiag.h"
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <sun/fbio.h>
#include <vfork.h>
#include "../../lib/include/libonline.h"
#include "../../lib/include/probe_sundiag.h"
#include "struct.h"
#include "procs.h"
#include "sundiag_ats.h"
#include "sundiag_msg.h"


#ifndef	sun386
#include <sys/mtio.h>
#else
#include "mtio.h"
#endif

#define MAX_ARG		17		/* max. # of arg's for child + 2 */

#define	GROUP_COL	1		/* group toggle starting column */
#define	SEL_COL		4		/* test select toggle starting column */
#define	OPT_COL		32		/* option panel item starting column */
#define	FB_WAIT		30
/* seconds to be wait before restart the frame buffer tests */

struct	test_info	*tests[MAXTESTS];	/* test's data base */
int	exist_tests=0;			/* total # of existing tests */
char	*cpuname;			/* CPU type of the tested machine */
char    *vmem_waittime[] = {"0","5","10","15","30","60","90","random"};
int	vmem_wait=0;			/* vmem wait time counter */
int	fb_delay=0;   /* # of seconds to be delayed between fb tests */
char	*sp_src= "0,1,3,5,8,9,b,d";	/* default ALM loopback source */
char	*sp_des= "7,2,4,6,f,a,c,e";	/* default ALM loopback destination */
char	*sp2_src= "0,2,4,6,8,a,c,e";	/* default ALM2 loopback source */
char	*sp2_des= "1,3,5,7,9,b,d,f";	/* default ALM2 loopback destination */
char	*scsisp_src= "0,2";	/* default SCSI serial port loopback source */
char	*scsisp_des= "1,3";/* default SCSI serial port loopback destination */
char	*sunlink_src[]=		/* default SunLink loopback source */
	{"0,2","4,6","8,10","12,14"};
char	*sunlink_des[]=		/* default SunLink loopback destination */
	{"1,3","5,7","9,11","13,15"};
char    *bpp_mode[] = {"fast", "medium", "extended"};
char    *lpvi_mode[] = {"fast", "medium", "extended"};
char    *lpvi_image[] = {"default", "57fonts", "u_image"};
char    *lpvi_resolution[] = {"400", "300"};
Frame	option_frame=NULL;	/* used to keep the frame of the option popup */
int	taac=0;			/* flag for whether a taac board or not */
int	zebra=0;		/* flag for whether a zebra board or not */
FILE	*conf_fp;

extern	select_test();			/* forward external declaration */
extern	Panel	init_opt_panel();	/* declared in option.c */
extern	frame_destroy_proc();		/* declared in panelprocs.c */
extern	char	*malloc();
extern	char	*strtok();
extern	char	*strcpy();
extern	char	*strcat();
extern	char	*sprintf();
extern	char	*mktemp();
extern	FILE	*fopen();
extern	char	*getenv();
extern	multi_tests();		/* forward declaration */
extern	Pixrect	*pr_open();
extern	Pixrect	*start_button, *reset_button;		/* defined in panel.c */
extern	char	*remotehost;
extern	int	config_file_only;	/* defined in sundiag.c */

char	toni_msg[82];
static	char	*child_arg[MAX_ARG];	/* argument array of pointers */
static	int	start_row=7;	/* beginning row of the test selection items */
static	int	ipcs=0;		/* keep # of IPC's */
static	int	gp2=0;		/* flag for whether there is GP2 board */
static	int	cg2=0;		/* flag for whether there is CG2 board */
static	int	cg4=0;		/* flag for whether there is CG4 board */
static	int	cg5=0;		/* flag for whether there is CG5 board */
static  int     cg12=0;		/* flag for whether there is CG12 board */ 
static  int	gttest=0;	/* flag for whether there is GT board */
static	int	ibis=0;		/* flag for whether there is IBIS board */
static	int	fb_inuse=0;   /* flag to tell whether frame buffer is in use */
static	int	fb_id= -1;    /* to keep track of who is using the fb */
static	int	dcp_kernel[4]={0,0,0,0}; /* dcp kernel(ucode) been loaded? */
static	int	mcp_flag[8]={0,0,0,0,0,0,0,0};
				/* up to 8 mcp boards in a machine */
static	int	net_flag=0;		 /* only run one nettest at a time */
static	int	next_net=0;		 /* only run one nettest at a time */
static	int	last_group;		 /* the group id of lastly-run test */

/******************************************************************************
 * check_dblbuf(), returns TRUE if the frame device is double buffered.       *
 *	This code was extracted from cg5test.c.				      *
 * Note: this function(checking double frame buffer) should be moved to probe.*
 ******************************************************************************/
static	int	check_dblbuf()
{
  Pixrect *prfd;
  struct  fbtype  fb_type;
  int tmpfd;

  /* first test for console type */
  if ((tmpfd=open("/dev/fb", O_RDONLY)) != -1)
  {
    (void)ioctl(tmpfd, FBIOGTYPE, &fb_type);
    (void)close(tmpfd);
  }
  else
    return(FALSE);

  /* check if cg driver is there */
  if (access("/dev/cgtwo0", 0) == 0)
  {
    /* first check if going through gp2 */
    if (fb_type.fb_type == FBTYPE_SUN2GP )
    {
	if (access("/dev/gpone0a", 0) == 0)
	{
	  if ((prfd=pr_open("/dev/gpone0a")) == (Pixrect *)0)
	    return(FALSE);
	}
	else
	  return(FALSE);
    }
    else if (fb_type.fb_type == FBTYPE_SUN2COLOR)
    {
	if ((prfd=pr_open("/dev/fb")) == (Pixrect *)0)
	  return(FALSE);
    }
    else
    {
	if ((prfd=pr_open("/dev/cgtwo0")) == (Pixrect *)0)
	  return(FALSE);
    }

    /* We have a device.  Check for single or double buffer */
    (void)pr_dbl_set(prfd, PR_DBL_WRITE, PR_DBL_B, 0);
    (void)pr_put(prfd, 0, 0, 0x55);

    (void)pr_dbl_set(prfd, PR_DBL_WRITE, PR_DBL_A, 0);
    (void)pr_put(prfd, 0, 0, 0xAA);

    (void)pr_dbl_set(prfd, PR_DBL_READ, PR_DBL_B, 0);
    if (pr_get(prfd, 0, 0) == 0x55)
    {
	(void)pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
		   	PR_DBL_READ, PR_DBL_A, PR_DBL_WRITE, PR_DBL_BOTH, 0);
	(void)pr_destroy(prfd);
	return(TRUE);
    }
    else
      (void)pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
		   	PR_DBL_READ, PR_DBL_A, PR_DBL_WRITE, PR_DBL_BOTH, 0);

    (void)pr_destroy(prfd);
  }

  return(FALSE);
}

/******************************************************************************
 * check_probe() checks the device name againt the test data base to determine*
 * which test should be run on this device.				      *
 * Input: dev_ptr, the info. regarding the found device.		      *
 * Output: the test no; -1 if no test supports this device.		      *
 ******************************************************************************/
static int	check_probe(dev_ptr)
struct found_dev	*dev_ptr;
{
  int	i;

  if (dev_ptr->unit > 0)		/* if 2nd(and higher) device */
  {
    if (strcmp(dev_ptr->device_name, "ie") == 0) return(ENET2);
	/* at most 2 ethernet boards!! */

    if (strcmp(dev_ptr->device_name, "zs") == 0)
    {
      if (dev_ptr->unit == 2)		/* zs3 is dummy */
	return(CPU_SP1);		/* ttyc and ttyd */
      else if (dev_ptr->unit == 4)	/* zs5 is dummy */
	return(SCSISP2);
      else
	return(-1);			/* not implemented yet */
    }
  }

  for (i=0; i != TEST_NO; ++i)
    if (strcmp(dev_ptr->device_name, tests_base[i].devname) == 0)
    {
      if (i == GP)			/* check whether it is CG2 */
      {
	if (dev_ptr->u_tag.uval.devinfo.status == 2) return(GP2);
      }
      else if (i == COLOR2)		/* check whether it's double buffered */
      {
	if (check_dblbuf()) return(COLOR5);
      }
      else if (i == FPA)		/* check for fpa-3x board */
      {
        if (dev_ptr->u_tag.uval.devinfo.status == FPA3X_EXIST) return(FPA3X);
      }
      else if (i == FPUTEST)
      {
	if (dev_ptr->u_tag.uval.devinfo.status == FPU2_EXIST) return(FPU2);
      }
      return(i);
    }

  return(-1);
}

/******************************************************************************
 * make_test() creates a test information data structure to be inserted into  *
 * the tests[].								      *
 * Input: test_no, test number; unit, device unit number.		      *
 * Output: a pointer to created data structure.				      *
 ******************************************************************************/
static struct test_info	*make_test(test_no, unit)
int	test_no;		/* test no(index to tests_base[]) */
int	unit;			/* device unit number */
{
  struct test_info *tmp;

    if (unit != 0)		/* if multiple devices */
    {
      if (tests_base[test_no].unit == -1)
      /* devices with single unit, but with a non-zero UNIX device number */
      {
	tests_base[test_no].unit = unit;   /* change the unit number here */
	return(&tests_base[test_no]);
      }

      tmp = (struct test_info *)malloc(sizeof(struct test_info));
      bcopy((char *)(&tests_base[test_no]), (char *)tmp,
						sizeof(struct test_info));
      tmp->unit = unit;

      tmp->devname = malloc((unsigned)strlen(tmp->devname)+5);
      (void)strcpy(tmp->devname, tests_base[test_no].devname);
    }
    else
      tmp = &tests_base[test_no];

    return(tmp);
}

/******************************************************************************
 * build_user_test(), appends the user-defined tests at the end of the test   *
 * data structure, tests[].						      *
 * Input: global test data structure, tests[].				      * 
 * Output: None, but the test data structure, tests[],  will be modified.     *
 ******************************************************************************/
build_user_tests()
{
  FILE	*user_fp;
  int	i, k;
  struct test_info *tmp;
  char	buff[82], *temp;
  char	*param[21];		/* max. to 20 tokens(plus one to store NULL) */
  char	*delimit;

  if ((user_fp=fopen(USER_FILE, "r")) == NULL) return;

  k = 0;			/* initialize the user-defined test number */
  while (fgets(buff, 81, user_fp) != NULL)
  {
    if (buff[0] == '#') continue;	/* skip comment */
    
    i = 0;
    delimit = ",\n";
    if ((temp=strtok(buff, delimit)) != NULL)
    {
      do
      {
	param[i++] = temp;		/* parse out the tokens */
	if (i == 2)
	  delimit = "\n";		/* ignore the comma from here */
      }
      while ((temp=strtok((char *)NULL, delimit)) != NULL && i < 20);
      param[i] = NULL;
    }
    if (i < 2) continue;		/* format error, skipped for now */

    tmp = (struct test_info *)malloc(sizeof(struct test_info));
    (void)bcopy((char *)(&tests_base[USER]), (char *)tmp,
						sizeof(struct test_info));

    temp = malloc((unsigned)strlen(tmp->devname)+5);
    (void)sprintf(temp, "%s%d", tmp->devname, k);
    tmp->devname = temp;		/* get the device name, userx */

    temp = malloc((unsigned)strlen(param[0])+5);
    (void)strcpy(temp, param[0]);	/* get the label */
    tmp->label = temp;

    temp = malloc((unsigned)strlen(param[1])+2);
    (void)strcpy(temp, param[1]);	/* get the testname */
    while (*temp == ' ' || *temp == '\t') ++temp;	/* skip white spaces */
    tmp->testname = temp;

    if (i > 2)				/* there are command tails */
    {
      temp = malloc((unsigned)strlen(param[2])+2);
      (void)strcpy(temp, param[2]);	/* ignore the rest of parameters */
      tmp->env = temp;
    }

    tests[exist_tests++] = tmp;
    ++k;
  }

  (void)fclose(user_fp);
}

/******************************************************************************
 * Initilaize the test's data base according to whatever the probing routine  *
 * discovered.								      *
 * Input: f_dev_ptr, pointer to the table of information regarding the	      *
 *	  "found" device(built by probing routine).			      *
 * Output: None.							      *
 *									      *
 * Note: The found devices will also be sorted by the group.		      *
 ******************************************************************************/
init_tests(f_dev_ptr)
struct f_devs *f_dev_ptr;		/* info. of found devices */
{
  int	i, j, k, temp, test_no;
  char	*tmp, buff[82];
  struct found_dev	*dev_ptr;
  struct test_info	*tmp_tests[MAXTESTS];	/* temporary array */
  int	already_log=0;

  last_group = ngroups - 1;

  if (!ats_nohost || config_file_only == TRUE)
    if ((conf_fp=fopen(conf_file, "w")) == NULL)
    {
      (void)fprintf(stderr, "Sundiag: Can't open %s", conf_file);
      sundiag_exit(1);
    }

  cpuname = malloc((unsigned)strlen(f_dev_ptr->cpuname)+2);
  (void)strcpy(cpuname, f_dev_ptr->cpuname); 

  if (!ats_nohost || config_file_only == TRUE)
    (void)fprintf(conf_fp, "cpu	\"%s\"\n", cpuname);

  exist_tests = f_dev_ptr->num;
  dev_ptr = f_dev_ptr->found_dev;

  for (i=0, j= -1; i < exist_tests; ++i)
  {
    if ((test_no=check_probe(dev_ptr)) != -1)
    {
      tmp_tests[++j] = make_test(test_no, dev_ptr->unit);

      /* copy the device information into test's data base */
      tmp_tests[j]->conf = (struct u_tag *)malloc(sizeof(struct u_tag));
      bcopy((char *)(&(dev_ptr->u_tag)), (char *)(tmp_tests[j]->conf),
							sizeof(struct u_tag));
      if (dev_ptr->u_tag.utype == DISK_DEV)
      {
	tmp = malloc((unsigned)strlen(dev_ptr->u_tag.uval.diskinfo.ctlr)+2);
	(void)strcpy(tmp, dev_ptr->u_tag.uval.diskinfo.ctlr);
	tmp_tests[j]->conf->uval.diskinfo.ctlr = tmp;
      }
      else if (dev_ptr->u_tag.utype == TAPE_DEV)
      	{
	  tmp = malloc((unsigned)strlen(dev_ptr->u_tag.uval.tapeinfo.ctlr)+2);
	  (void)strcpy(tmp, dev_ptr->u_tag.uval.tapeinfo.ctlr);
	  tmp_tests[j]->conf->uval.tapeinfo.ctlr = tmp;
        }

      switch (tmp_tests[j]->id)
      {
	case AUDIO:
          if ( dev_ptr->u_tag.uval.devinfo.status == 1 )
          {
          tmp_tests[j]->type = 2;
          tmp_tests[j]->enable = DISABLE;
          tmp_tests[j]->data = (caddr_t)0x40;
          tmp_tests[j]->testname = "audbri";
          tmp_tests[j]->special = (caddr_t)0;
          }
          break;

        case PMEM:
        case VMEM:
	  temp = dev_ptr->u_tag.uval.meminfo.amt/1024;
	  if (dev_ptr->u_tag.uval.meminfo.amt%1024 > 0)  
	    ++temp; 
	  tmp_tests[j]->conf->uval.meminfo.amt = temp;	/* save it */
	  if (!ats_nohost || config_file_only == TRUE)
	  {
	    (void)fprintf(conf_fp, "%s\t%d MB\n", tmp_tests[j]->devname, temp);
	    already_log = 1;
	  }
	  break;

        case SCSIDISK1:
        case XYDISK1:
        case XDDISK1:
        case IPIDISK1:
	case IDDISK1:
	case SFDISK1:
	case OBFDISK1:
	  temp = dev_ptr->u_tag.uval.diskinfo.amt;	/* in the unit of MB */
	  if (!ats_nohost || config_file_only == TRUE)
	  {
	    if (tmp_tests[j]->id != IDDISK1)
	      (void)fprintf(conf_fp, "%s%d\t%d MB\n", tmp_tests[j]->devname,
						tmp_tests[j]->unit, temp);
	    else
	      (void)fprintf(conf_fp, "%s%03x\t%d MB\n", tmp_tests[j]->devname,
						tmp_tests[j]->unit, temp);
	    already_log = 1;
	  }

	  /* Make sure the default rawtest option runs only one copy */
	  if (temp <= 100) temp = 0;
	  else
	 	 temp = ((temp -1)/200) + 1;
	  if (temp > 6) temp = 6;
          (int)tmp_tests[j]->data &= ~0x70;
          (int)tmp_tests[j]->data |= temp << 4;

	  tmp_tests[++j] = make_test(test_no+1, dev_ptr->unit);
			/* enable the read/write test too */
	 
	  break;

	case SCSITAPE:
	  temp = dev_ptr->u_tag.uval.tapeinfo.t_type;	/* get the tape type */
	  if (temp == MT_ISEXABYTE)
	    (int)tmp_tests[j]->data = 0x80060;
#ifdef NEW
	  else if (temp == MT_ISEXB8500)
	    (int)tmp_tests[j]->data = 0x80060;
#endif NEW
	  else if (temp == MT_ISHP || temp == MT_ISKENNEDY)
	    (int)tmp_tests[j]->data = 0x190043;
	  else if (temp == MT_ISVIPER1 || temp == MT_ISWANGTEK1)
	    (int)tmp_tests[j]->data = 0xa0040;		/* QIC-150 format */
	  else (int)tmp_tests[j]->data = 0xa0042;	/* to be sure */
	  break;

        case MTI:
        case MCP:
	case SCSISP1:
	case SCSISP2:
	  tmp = malloc(sizeof(struct loopback));
	  if (tmp_tests[j]->id == MCP)
	  {
	    (void)strcpy(((struct loopback *)tmp)->from, sp2_src);
	    (void)strcpy(((struct loopback *)tmp)->to, sp2_des);
	  }
	  else if (tmp_tests[j]->id == MTI)
	  {
	    (void)strcpy(((struct loopback *)tmp)->from, sp_src);
	    (void)strcpy(((struct loopback *)tmp)->to, sp_des);
	  }
	  else
	  {
	    (void)strcpy(((struct loopback *)tmp)->from, scsisp_src);
	    (void)strcpy(((struct loopback *)tmp)->to, scsisp_des);
	  }

	  tmp_tests[j]->data = (caddr_t)tmp;

	  if (tmp_tests[j]->id == MCP)
	  {
	    if (dev_ptr->u_tag.uval.devinfo.status == IFDDEVONLY)
	    /* SunLink was installed */
	    {
	      tmp_tests[j] = make_test(test_no+2, dev_ptr->unit);
	        /* SCP2 SunLink test, overwrite tty test*/
	      tmp = malloc(sizeof(struct loopback));
	      (void)strcpy(((struct loopback *)tmp)->from,
				sunlink_src[tmp_tests[j]->unit]);
	      (void)strcpy(((struct loopback *)tmp)->to,
				sunlink_des[tmp_tests[j]->unit]);
	      tmp_tests[j]->data = (caddr_t)tmp;
	    }
            if (dev_ptr->u_tag.uval.devinfo.status == MCPOK)
              tmp_tests[++j] = make_test(test_no+1, dev_ptr->unit);
                        /* also enable the MCP printer test */
	  }
	  break;

        case SCP:
	  tmp = malloc(sizeof(struct loopback));
	  (void)strcpy(((struct loopback *)tmp)->from, sunlink_src[0]);
	  (void)strcpy(((struct loopback *)tmp)->to, sunlink_des[0]);
	  tmp_tests[j]->data = (caddr_t)tmp;
	  break;

	case COLOR2:
	case TV1:		/* need 1MB of swap */
	  cg2 = 1;
	  break;

	case COLOR3:
	case COLOR4:
	  cg4 = 1;
	  break;

	case COLOR5:
#ifdef	sun386
	  cg2 = 1;		/* using color test(not cg5test) */
#else	sun386
	  cg5 = 1;		/* double frame buffer boards */
#endif	sun386
	  break;

	case COLOR6:		/* LEGO */
	case COLOR8:		/* IBIS */
	case COLOR9:		/* CRANE */
	  ibis = 1;		/* IBIS board */
	  break;

	case GP2:
	  gp2 = 1;
	  break;

	case CG12:
	  cg12 = 1;
	  break;

	case GT:
	  gttest = 1;
	  break;

	case ZEBRA2:		/* lpvi device (zebra board) */
	  ++zebra;
	  break;

	case TAAC:
	  taac = 1;		/* there's a taac board */
	  break;

	case IPC:
	  if (ipcs == 0)			/* do followings only once */
	  {
	    (void)gethostname(buff, 80);		/* get the host name */
	    tmp = malloc((unsigned)strlen(buff)+20);
		/* make sure it is long enough */
	    (void)sprintf(tmp, "SYSDIAG_HOST=%s", buff);
	    (void)putenv(tmp);
	    (void)sprintf(buff, "IPC_MSG_DIR=%s", LOG_DIR);
	    tmp = malloc((unsigned)strlen(buff)+2);
	    (void)strcpy(tmp, buff);
	    (void)putenv(tmp);
	  }
	  ++ipcs;
	  break;

        default:
	  break;
      }

      if ( (!ats_nohost && already_log == 0) || (config_file_only == TRUE && already_log == 0) )
      {
            if (tmp_tests[j]->unit == -1)
	      (void)fprintf(conf_fp, "%s\t0\n", tmp_tests[j]->devname);
            else
	      (void)fprintf(conf_fp, "%s%d\t0\n", tmp_tests[j]->devname,
							tmp_tests[j]->unit);
      }
      else
	already_log = 0;
    }

    ++dev_ptr;
  }

  exist_tests = j+1;			/* total number of tests */

  if (!ats_nohost || config_file_only == TRUE)
    (void)fclose(conf_fp);

  temp = 0;			/* used to keep track of # of "mt" devices */
  for (i=0, k=0; i != TEST_NO; ++i)	/* sort the tests according to group */
  {
    if (tests_base[i].which_test > 1) continue;	/* skip "non-1st" tests */

    for (j=0; j != exist_tests; ++j)
      if (tmp_tests[j]->id == i)
      {
	if (i == MAGTAPE1) ++temp;
	else if (i == MAGTAPE2) tmp_tests[j]->unit += temp;

	tests[k++] = tmp_tests[j];		/* right spot for it */

	if (tmp_tests[j]->test_no != 1)
	/* should be followed by all tests for this device */
	{
	  do
	  {
		++j;
		tests[k++] = tmp_tests[j];
	  }
	  while (tmp_tests[j]->which_test != tmp_tests[j]->test_no);
	}
      }
  }

  for (i=0 ; i != exist_tests; ++i)	/* build the unit # into device name */
    if (tests[i]->unit != -1)
      if (tests[i]->id != IDDISK1 && tests[i]->id != IDDISK2)
        (void)sprintf(tests[i]->devname,
			"%s%d", tests[i]->devname, tests[i]->unit);
      else
        (void)sprintf(tests[i]->devname,
			"%s%03x", tests[i]->devname, tests[i]->unit);

  if (!ats_nohost)
  {
    intervention = ENABLE;		/* defaults to enabling intervention */
    for (i=0 ; i != exist_tests; ++i)	/* disable all tests */
    {
      tests[i]->enable = DISABLE;
      if (tests[i]->test_no > 1)      /* more than one tests for this device */
        tests[i]->dev_enable = DISABLE;
      if (tests[i]->type == 2) tests[i]->type = 12;   /* enable intervention */
    }
  }

  build_user_tests();

  for (i=0; i != MAX_ARG; ++i)
    child_arg[i] = NULL;		/* initialize the test arg's array */
}

/******************************************************************************
 * Build the device label(english name + info.) for the device, which is to   *
 * be displayed in Control subwindow.					      *
 * Input: test_id, the internal test number of the test label to be built.    *
 * Output: none.							      *
 ******************************************************************************/
char *build_sel_label(test_id)
int	test_id;
{
  static char	label_buf[82];

  switch (tests[test_id]->id)		/* depend on which test it is */
  {
    case SCSIDISK1:			/* multi-unit devices */
    case XYDISK1:
    case XDDISK1:
    case IPIDISK1:
    case SFDISK1:
    case OBFDISK1:
    case IPC:
    case MCP:
    case MAGTAPE1:
    case MAGTAPE2:
    case MTI:
    case SCSITAPE:
    case SCP:
    case SCP2:
    case HSI:
    case SBUS_HSI:
    case GT:
    case PP:
    case CDROM:
	(void)sprintf(label_buf, "(%s) %s%d ",  tests[test_id]->devname,
		tests[test_id]->label, tests[test_id]->unit);
	break;

    case IDDISK1:
	(void)sprintf(label_buf, "(%s) %s%03x ",  tests[test_id]->devname,
		tests[test_id]->label, tests[test_id]->unit);
	break;

    case SPIF:
	(void)sprintf(label_buf, "(stc%d) %s%d ", tests[test_id]->unit, 
		tests[test_id]->label, tests[test_id]->unit + 1); 
	break;

    default:
	if (tests[test_id]->devname != NULL)
	  (void)sprintf(label_buf, "(%s) %s",
			tests[test_id]->devname, tests[test_id]->label);
	else
	  (void)strcpy(label_buf, tests[test_id]->label);

	break;
  }

  return((char *)label_buf);
}

/******************************************************************************
 * group_sel_proc, the panel notify procedure for the group selection toggle. *
 ******************************************************************************/
static group_sel_proc(item, value)
Panel_item item;
int	value;
{
  int	group_id, test_id;
  int	flag=0, iflag=0;

  group_id = (int)panel_get(item, PANEL_CLIENT_DATA);
  /* get the internal test number */

  groups[group_id].enable = (value & 1);

  test_id = groups[group_id].first;
  /* get the first test number in the group */

  while (test_id != exist_tests)	/* check the entire group */
  {
    if (tests[test_id]->group == group_id)
    {
      if (tests[test_id]->type != 2)/* if not the disabled intervention tests */
      {
        if (value & 1)		/* if the entire group is to be enabled */
	{
	  tests[test_id]->enable = TRUE;

	  flag = 1;

	  if (running == GO || running == SUSPEND)  /* if tests are running */
	    start_log(test_id);
	}
        else			/* if the entire group is to be disabled */
        {
	  tests[test_id]->enable = FALSE;

	  if (tests[test_id]->pid != 0)		/* test is currently running */
	    (void)kill(tests[test_id]->pid, SIGINT);

	  if (running == GO || running == SUSPEND)  /* if tests are running */
	    stop_log(test_id);
        }

	if (tests[test_id]->which_test == 1)
	/* the toggle item was only kept in the first test for the device */
          (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE,
					0, tests[test_id]->enable, 0);
	if (tests[test_id]->test_no > 1)
	/* for device with more than one tests */
	  tests[test_id]->dev_enable = tests[test_id]->enable;
      }
      else if (value & 1)	/* and to enable the entire group */
	iflag = 1;

      ++test_id;	/* next test in the group */
    }
    else
	break;		/* done */
  }

  if ((value & 1) && flag == 0)		/* nothing got enabled */
  {
	(void)panel_set(groups[group_id].select,
				PANEL_TOGGLE_VALUE, 0, FALSE, 0);
	groups[group_id].enable = DISABLE;
	if (iflag == 1) (void)confirm(
	"The 'Intervention' mode was not enabled, no tests selected.", TRUE);
  }
  else
  {
    (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_NONE, 0);
    /* no test slections mark */
    selection_flag = FALSE;
    print_status();		/* update the status */
  }
}

/******************************************************************************
 * test_sel_proc, the panel notify procedure for the test selection toggle.   *
 ******************************************************************************/
static test_sel_proc(item, value)
Panel_item item;
int	value;
{
  int	test_id;
  int	group_id;

  test_id = (int)panel_get(item, PANEL_CLIENT_DATA);
  /* get the internal test number */

  if (tests[test_id]->type == 2)
  {
	(void)panel_set(tests[test_id]->select,
				PANEL_TOGGLE_VALUE, 0, FALSE, 0);
	(void)confirm(
	"Please enable the 'Intervention' mode before select this test.", TRUE);
	return;	/* return if disabled intervention/manufacturing tests */
  }

  (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_NONE, 0);
  /* no test slections mark */
  selection_flag = FALSE;

  if (tests[test_id]->test_no > 1)
    multi_tests(test_id, value);	/* handle multiple tests for a device */
  else
  {
    tests[test_id]->enable = (value & 1);

    if (tests[test_id]->enable)		/* if test is to be enabled */
    {
      print_status();			/* display it on status subwindow */

      if (running == GO || running == SUSPEND)	/* if tests are running */
        start_log(test_id);		/* log to information file */
    }
    else				/* if test is to be disabled */
    {
      if (tests[test_id]->pid == 0)	/* if test is not currently running */
	print_status();			/* just remove it */
      else
	(void)kill(tests[test_id]->pid, SIGINT);

      if (running == GO || running == SUSPEND)	/* if tests are running */
        stop_log(test_id);		/* log to information file */
    }
  }

  group_id = tests[test_id]->group;	/* get the group number of the test */
  test_id = groups[group_id].first;	/* get the first test in the group */
  for (; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->group != group_id) break;	/* none was enabled */

    if (tests[test_id]->dev_enable && (tests[test_id]->enable ||
		tests[test_id]->test_no > 1) && tests[test_id]->type != 2)
    {
	if (!groups[group_id].enable)
	{
	  groups[group_id].enable = TRUE;
	  (void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE,
					0, TRUE, 0);
	}
	return;				/* done */
    }
  }

  if (groups[group_id].enable)
  {
    groups[group_id].enable = FALSE;
    (void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE,
					0, FALSE, 0);
  }
}

/******************************************************************************
 * ats_start_test was called when ATS tried to select a test by passing in    *
 * the desired test name and device name.				      *
 * Input: testname, test name; devname, device name.			      *
 * Output: none.							      *
 ******************************************************************************/
ats_start_test(testname, devname)
char	*testname, *devname;
{
  int	test_id;
  int	group_id;
  int	id;

  for (test_id=0; test_id != exist_tests; ++test_id)
					/* gone through all existing devices */
    if (strcmp(devname, tests[test_id]->devname) == 0 &&
	strcmp(testname, tests[test_id]->testname) == 0) break;

  if (test_id == exist_tests)		/* unknown test */
  {
    send_test_msg(SUNDIAG_ATS_ERROR, 0, testname, devname, "unknown test");
    return;
  }

  if (tests[test_id]->type == 2)
  {
	send_test_msg(SUNDIAG_ATS_ERROR, 0, testname, devname, 
			"disabled intervention test");
	return;	/* return if disabled intervention/manufacturing tests */
  }

  if (!tty_mode)			/* remove test slections mark */
    (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_NONE, 0);

  selection_flag = FALSE;  

  if (tests[test_id]->enable == ENABLE) return;	/* already enabled */

  tests[test_id]->enable = ENABLE;

  if (tests[test_id]->test_no > 1)		/* multiple tests */
  {
    for (id=test_id; tests[id]->which_test != 1; --id);
	/* find the first test for this device */
    if (!tty_mode)
      (void)panel_set(tests[id]->select, PANEL_TOGGLE_VALUE, 0, TRUE, 0);
    else
      display_enable(id, TRUE);
    for (; tests[id]->which_test != tests[id]->test_no; ++id)
      tests[id]->dev_enable = ENABLE;
    tests[id]->dev_enable = ENABLE;
  }
  else
  {
    if (!tty_mode)
      (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE, 0, TRUE, 0);
    else
      display_enable(test_id, TRUE);
  }

  if (running == GO || running == SUSPEND)	/* tests are running */
    start_log(test_id);			/* log to information file */

  print_status();			/* display it */

  group_id = tests[test_id]->group;	/* update the group toggle too */
  if (!groups[group_id].enable)
  {
    groups[group_id].enable = ENABLE;
    if (!tty_mode)
      (void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE, 0, TRUE, 0);
  }
}

/******************************************************************************
 * ats_stop_test was called when ATS tried to disselect a test by passing in  *
 * the desired test name and device name.				      *
 * Input: testname, test name; devname, device name.			      *
 * Output: none.							      *
 ******************************************************************************/
ats_stop_test(testname, devname)
char	*testname, *devname;
{
  int	test_id;
  int	group_id;
  int	id, tmp;

  for (test_id=0; test_id != exist_tests; ++test_id)
					/* gone through all existing devices */
    if (strcmp(devname, tests[test_id]->devname) == 0 &&
	strcmp(testname, tests[test_id]->testname) == 0) break;

  if (test_id == exist_tests)		/* unknown test */
  {
    send_test_msg(SUNDIAG_ATS_ERROR, 0, testname, devname, "unknown test");
    return;
  }

  if (!tty_mode)			/* remove test slections mark */
    (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_NONE, 0);

  selection_flag = FALSE;

  if (tests[test_id]->enable == DISABLE) return;	/* already disabled */

  tests[test_id]->enable = DISABLE;

  if (tests[test_id]->test_no > 1)		/* multiple tests */
  {
    for (id=test_id; tests[id]->which_test != 1; --id);
	/* find the first test for this device */

    for (tmp=id; tests[tmp]->which_test != tests[tmp]->test_no; ++tmp)
	if (tests[tmp]->enable) break;	/* at least one is still enabled */

    if (!tests[tmp]->enable)		/* none was enabled now */
    {
      if (!tty_mode)
        (void)panel_set(tests[id]->select, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
      else
	display_enable(id, FALSE);
      for (; tests[id]->which_test != tests[id]->test_no; ++id)
        tests[id]->dev_enable = DISABLE;
      tests[id]->dev_enable = DISABLE;
    }
  }
  else
  {
    if (!tty_mode)
      (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
    else
      display_enable(test_id, FALSE);
  }

  if (tests[test_id]->pid == 0)	/* test is not currently running */
    print_status();		/* just remove it */
  else
    (void)kill(tests[test_id]->pid, SIGINT);

  if (running == GO || running == SUSPEND)	/* tests are running */
    stop_log(test_id);		/* log to information file */

  group_id = tests[test_id]->group;
  test_id = groups[group_id].first;
  for (; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->group != group_id) break;	/* none was enabled */

    if (tests[test_id]->dev_enable && (tests[test_id]->enable ||
		tests[test_id]->test_no > 1) && tests[test_id]->type != 2)
	return;		/* at least one is still enabled */
  }

  if (groups[group_id].enable)
  {
    groups[group_id].enable = DISABLE;
    if (!tty_mode)
      (void)panel_set(groups[group_id].select, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
  }
}

/******************************************************************************
 * Initialize the group select toggle on the control subwindow.		      *
 * Input: group_id, the internal group number of the group to be initialized. *
 * Output: none.							      *
 ******************************************************************************/
static	init_group_tog(group_id)
int		group_id;		/* group number, index to groups[] */
{
  Panel_item	handle;

  handle = panel_create_item(sundiag_control, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,	groups[group_id].c_label, 0,
		PANEL_TOGGLE_VALUE,	0, groups[group_id].enable,
		PANEL_NOTIFY_PROC,	group_sel_proc,
		PANEL_CLIENT_DATA,	group_id,
		PANEL_ITEM_X,		ATTR_COL(GROUP_COL),
		PANEL_ITEM_Y,		ATTR_ROW(start_row++),
		PANEL_SHOW_MENU,	FALSE,
		0);
  groups[group_id].select = handle;	/* keep the panel item handle */
}

/******************************************************************************
 * Initialize the test select toggle on the control subwindow.		      *
 * Input: test_id, the internal test number of the group to be initialized.   *
 * Output: none.							      *
 ******************************************************************************/
static	init_test_tog(test_id)
int		test_id;		/* test number, index to tests[] */
{
  Panel_item	handle;
  int		toggle_value=0;

  if (tests[test_id]->test_no > 1)	/* multiple tests */
  {
    if (tests[test_id]->type != 2) /* it is not a disabled intervention tests */
      toggle_value = tests[test_id]->dev_enable;
    else
      toggle_value = FALSE;
  }
  else
    toggle_value = (tests[test_id]->enable && tests[test_id]->type != 2);

  handle = panel_create_item(sundiag_control, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS,	build_sel_label(test_id), 0,
		PANEL_TOGGLE_VALUE,	0, toggle_value,
		PANEL_NOTIFY_PROC,	test_sel_proc,
		PANEL_CLIENT_DATA,	test_id,
		PANEL_ITEM_X,		ATTR_COL(SEL_COL),
		PANEL_ITEM_Y,		ATTR_ROW(start_row++),
		PANEL_SHOW_MENU,	FALSE,
		0);
  tests[test_id]->select = handle;	/* keep the panel item handle */
}

/******************************************************************************
 * opt_sel_proc, option button notify procedure.			      *
 ******************************************************************************/
/*ARGSUSED*/
static opt_sel_proc(item, value, event)
Panel_item item;
int	value;
Event	*event;
{
  int	test_id;
  Panel	panel_handle;

  test_id = (int)panel_get(item, PANEL_CLIENT_DATA);
  /* get the internal test number of the test */

  if (tests[test_id]->popup)		/* option popup is really needed */
  {
	if (option_frame != NULL)  /* destroy the old one, if there is one */
	  frame_destroy_proc(option_frame);

	option_frame = window_create(sundiag_frame, FRAME,
	    FRAME_SHOW_LABEL,	TRUE,
	    WIN_X,	(int)((STATUS_WIDTH+PERFMON_WIDTH)*frame_width)+15,
	    WIN_Y,	20,
            FRAME_DONE_PROC, frame_destroy_proc, 0);

	panel_handle = init_opt_panel(test_id);
	/* initialize rest of the control subwindow */
	/* return popup panel's handle */

	window_fit(panel_handle);
        window_fit(option_frame);
        (void)window_set(option_frame, WIN_SHOW, TRUE, 0);
  }
}

/******************************************************************************
 * Initialize the test option button on the control subwindow.		      *
 * Input: test_id, the internal test number of the option button to be	      *
 *	initialized.							      *
 * Output: none.							      *
 ******************************************************************************/
static	init_opt_button(test_id)
int	test_id;			/* index to tests[] */
{
  Panel_item	handle;
  int		cur_row;

  cur_row = (int)panel_get(tests[test_id]->select, PANEL_ITEM_Y) + 2;
	/* get the row to display the option button on */

  if (tests[test_id]->popup)	/* check whether the option popup is needed */
  {
	handle = panel_create_item(sundiag_control, PANEL_BUTTON,
		PANEL_LABEL_IMAGE,
		  panel_button_image(sundiag_control, "Option",
							6, (Pixfont *)NULL),
		PANEL_NOTIFY_PROC,      opt_sel_proc,
		PANEL_ITEM_X,		ATTR_COL(OPT_COL),
		PANEL_ITEM_Y,		cur_row,
		PANEL_CLIENT_DATA,	test_id,
		0);
	tests[test_id]->option = handle;
  }
}

/******************************************************************************
 * test_items() creates and initializes all of the select toggles on control  *
 * panel.								      *
 ******************************************************************************/
test_items()
{
  int	group, i;			/* current group id(index to groups[] */
  int	cur_test_i;			/* current test index(to tests[]) */

  for (i=0; i != ngroups; ++i)
    groups[i].enable = DISABLE;

  for (cur_test_i=0; cur_test_i != exist_tests; ++cur_test_i)
					/* gone through all existing devices */
  {
    group = tests[cur_test_i]->group;	/* get the current group */

    if (tests[cur_test_i]->dev_enable &&
	(tests[cur_test_i]->enable || tests[cur_test_i]->test_no > 1) &&
	tests[cur_test_i]->type != 2)
	  groups[group].enable = ENABLE;
  }

  if (!tty_mode)
  {
    for (cur_test_i=0; cur_test_i != exist_tests;)
    /* gone through all existing devices */
    {
      group = tests[cur_test_i]->group;	/* get the current group */
      groups[group].first = cur_test_i;	/* keep the first test in the group */

      init_group_tog(group);		/* display the group's toggle item */

      while (tests[cur_test_i]->group == group)
      {
        if (tests[cur_test_i]->which_test == 1)
        {
	  init_test_tog(cur_test_i);
		/* display the test's select toggle item */
		/* also initialize the selection panel item handle */
	  init_opt_button(cur_test_i);
		/* display the test's option item(e.g. vmem wait time) if any */
		/* also initialize the option panel item handle if needed */
        }

        if (++cur_test_i == exist_tests) break;		/* no more */
      }
    }

    (void)scrollbar_paint(control_bar);		/* update the bubble */
  }
}

/******************************************************************************
 * before_test(), checks to see whether the specified test can be run at this *
 * time. If not, return 1, otherwise, build the command line arguments(tail)  *
 * according to the user-specified options.				      *
 * Input: test_id, internal test number.				      *
 * Output: 1, if failed; 0, if succeed.					      *
 *	   Also, command line arguments will be built in tests[]->tail.	      *
 ******************************************************************************/
static	int	before_test(test_id)
{
  static	int	done_clean[48]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			/* up to 16 tape drives for st, mt and xt */
  static	char	opt_code[122];
  char		unit, *loopback, clock, rawtest_part, rawtest_size[10];
  int		type, T, i;
  static	int	vmem_waitvar;
  char		dev_name[8], tmp_buf[82];
  int		sdlc;   	     /* Sbus HSI test:  sdlc or hdlc protocol */
  int		internal_loopback;   /* interal loopback for Sbus HSI: 3-7 */
				     /* 3 = None
				      * 4 = Simple
				      * 5 = Clockless
				      * 6 = Silent
				      * 7 = Silent & Clockless
				      */

  /* stop testing if exceeded error limit/finished one pass for all tests */
  if ((max_errors != 0 && sys_error >= max_errors) ||
				(single_pass && sys_pass >= 1))
  {
    stop_proc();			/* stop all tests */
    return(1);				/* do not start anymore tests */
  }

  if (!run_error && tests[test_id]->error >= 1) return(1);
  /* this test has failed at least once and run-on-error is not enabled */

  if (single_pass && tests[test_id]->pass >= 1) return(1);
  /* this test has been run more than once */

  tests[test_id]->tail = opt_code;

  (void)strcpy(opt_code, "s ");		/* pass "s" to tests as default */

  switch (tests[test_id]->id)		/* check which test it is */
  {
    case MAGTAPE1:
    case MAGTAPE2:
    case SCSITAPE:
	if (tests[test_id]->id == MAGTAPE1) type = 1;
	else if (tests[test_id]->id == MAGTAPE2) type = 2;
	else type = 0;
	type = type*16;			/* up to 16 drives per tape type */
	if (!((int)tests[test_id]->data & 0x400) &&
			((unsigned)tests[test_id]->data>>16) != 0)
	if (tests[test_id]->pass != 0 && 
	    tests[test_id]->pass%((unsigned)tests[test_id]->data>>16) == 0)
	{				/* it's time to clean the head */
	  if (!done_clean[tests[test_id]->unit+type])
	  /* user hasn't responded yet */
	  {
	    if (!tty_mode)
	    {
	      sprintf(tmp_buf,
		"Clean the head of %s, then click \"Done\" to continue.",
		tests[test_id]->devname);
	      (void)popup_info(tmp_buf, &done_clean[tests[test_id]->unit+type]);
	      return(1);			/* can't run it yet */
	    }
	    else
	    {
	      sprintf(tmp_buf, "Clean the head of %s, then reselect the test.",
				tests[test_id]->devname);
	      tty_message(tmp_buf);
	      /* disable the test here */
	      tty_test_sel_proc(tests[test_id]->devname);
	      done_clean[tests[test_id]->unit+type] = TRUE;
	      return(1);			/* can't run it yet */
	    }
	  }
	  else
	    done_clean[tests[test_id]->unit+type] = FALSE;	/* reset */
	}

	if (tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISEXABYTE)
	{
	  (int)tests[test_id]->data |= 0x200;	/* always skip retension */
	  (int)tests[test_id]->data &= 0xffffff7f;  /* always disable recon */
	}
#ifdef NEW
	if (tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISEXB8500)
	{
	  (int)tests[test_id]->data |= 0x200;	/* always skip retension */
	  (int)tests[test_id]->data &= 0xffffff7f;  /* always disable recon */
	}
#endif NEW
	(void)sprintf(opt_code, "s D=/dev/r%s op=%d", tests[test_id]->devname,
					(int)tests[test_id]->data);
	if ((((int)tests[test_id]->data & 0x18) >> 3) == 1)	/* specified */
	  (void)sprintf(opt_code, "%s b=%u",
			opt_code, (unsigned)(tests[test_id]->special));
	break;

    case CDROM:			/* CD-ROM test */
	(void)sprintf(opt_code, "s D=/dev/r%s V=%d P=%d",
		tests[test_id]->devname, (int)tests[test_id]->special,
		(int)tests[test_id]->data >> 8);
	type = (int)tests[test_id]->data & 7;
	if (type == 1) strcat(opt_code, " T=sony2");
	else if (type == 2) strcat(opt_code, " T=hitachi4");
	else if (type == 3) strcat(opt_code, " T=pdo");
	else if (type == 4) strcat(opt_code, " T=other");
	else strcat(opt_code, " T=cdassist");

	if ((int)tests[test_id]->data & 8) strcat(opt_code, " S=data1");
	if ((int)tests[test_id]->data & 0x40) strcat(opt_code, " S=data2");
	if ((int)tests[test_id]->data & 0x10) strcat(opt_code, " S=audio");
	if ((int)tests[test_id]->data & 0x20) strcat(opt_code, " R=sequential");
	break;

    case SCSIDISK1:	/* standard format for device entry */
    case XYDISK1:
    case XDDISK1:
    case IPIDISK1:
    case IDDISK1:	/* Check for a Write/Read  raw disk test */
    case SFDISK1:
    case OBFDISK1:
	rawtest_part = (unsigned char)'a' + ((int)tests[test_id]->data & 0x7);
	/* NO.. this is wrong(Hai 1/30/92)
	type = ((int)tests[test_id]->data & 0x70);
	*/
	type = ((int)tests[test_id]->data & 0xf0) >> 4;
	if ( type == 8 ) /* 'Entire is choosed */
	    sprintf(opt_code, "s D=%s P=%c", tests[test_id]->devname, rawtest_part);
	else
	{
	    sprintf( rawtest_size, "%d", type == 0 ? 100 : type*200);
	    sprintf(opt_code, "s D=%s P=%c S=%s", tests[test_id]->devname, rawtest_part, rawtest_size);
	}
	if ((int)tests[test_id]->data & 0x8) strcat(opt_code, " W");
	break;
    case SCSIDISK2:
#ifdef sun386
        (void)sprintf(opt_code, "s D=roota");
	break;
#endif   
    case XYDISK2:
    case XDDISK2:
    case IPIDISK2:
    case IDDISK2:
    case SFDISK2:
    case OBFDISK2:
	(void)sprintf(opt_code, "s D=%s", tests[test_id]->devname);
	break;

    case VMEM:				/* take care of vmem wait time here */
	if (vmem_wait == 0 ||		/* first time around */
		elapse_count - vmem_wait >= vmem_waitvar * 60)
	{
		srand(elapse_count);
		if(!strcmp( (vmem_waittime[(int)tests[test_id]->data]), "random") )
		  vmem_waitvar =  (rand() >> 12) % 90;
		else
		  vmem_waitvar = atoi(vmem_waittime[(int)tests[test_id]->data]);

	  /* take care of margins here */

	  (void)sprintf(opt_code, "s R=%d M=%d",
		(int)tests[test_id]->special, (int)(tests[test_id]->pass+1));

	  if (gp2)		/* gp2, cg2, cg12, and cg5 are exclusive */
	    (void)strcat(opt_code, " gp2");
	  else if (cg5)
	    (void)strcat(opt_code, " cg5");
	  else if (cg2)
	    (void)strcat(opt_code, " cg2");
	  else if (cg12)
	    (void)strcat(opt_code, " cg12");

	  if (ibis)		/* ibis and cg4 are exclusive */
	    (void)strcat(opt_code, " ibis");
	  else if (cg4)
	    (void)strcat(opt_code, " cg4");

	  if (ipcs != 0)
	    (void)sprintf(opt_code, "%s ipcs=%d", opt_code, ipcs);
	  if (taac == 1)
	    (void)strcat(opt_code, " taac");
	  if (zebra != 0)
	    (void)sprintf(opt_code, "%s zebra=%d", opt_code, zebra);
	  if (gttest == 1)
	    (void)strcat(opt_code, " gt");

	  break;
	}
	else
	  return(1);			/* can't run it yet */

    case AUDIO:
        if (tests[test_id]->conf->uval.devinfo.status == 0) {
                (void)sprintf(opt_code, "s O=%s V=%d",
                        ((int)tests[test_id]->data&1)?"jack":"speaker",
                        (int)tests[test_id]->special);
        } else {
		i = (int)tests[test_id]->data&0x1;
                (void)sprintf(opt_code, "s T=%d D=/dev/%s F=%s",
                        i+1,
                        (int)tests[test_id]->devname,
                        (int)tests[test_id]->special);
                if ( (((int)tests[test_id]->data & 0x4) >> 2) != 0 )
                        strcat(opt_code, " L");
                if ( (((int)tests[test_id]->data & 0x8) >> 3) != 0 )
                        strcat(opt_code, " C");
                if ( (((int)tests[test_id]->data & 0x10) >> 4) != 0 )
                        strcat(opt_code, " X");
                if ( (((int)tests[test_id]->data & 0x20) >> 5) != 0 )
                        strcat(opt_code, " S");
                if ( (((int)tests[test_id]->data & 0x40) >> 6) != 0 )
                        strcat(opt_code, " M");
        }
        break;

    case CPU_SP:		/* ttya and ttyb */
	switch ((int)tests[test_id]->data)
	{
	  case 0:
		loopback = "/dev/ttya";
		break;
	  case 1:
		loopback = "/dev/ttyb";
		break;
	  case 2:
		loopback = "/dev/ttya,b";
		break;
	  case 3:
	  default:
		loopback = "/dev/ttya /dev/ttyb";
		break;
	}

	(void)sprintf(opt_code, "s D=%s %s", tests[test_id]->devname, loopback);
	break;

    case CPU_SP1:		/* ttyc and ttyd */
	switch ((int)tests[test_id]->data)
	{
	  case 0:
		loopback = "/dev/ttyc";
		break;
	  case 1:
		loopback = "/dev/ttyd";
		break;
	  case 2:
		loopback = "/dev/ttyc,d";
		break;
	  case 3:
	  default:
		loopback = "/dev/ttyc /dev/ttyd";
		break;
	}

	(void)sprintf(opt_code, "s D=%s %s", tests[test_id]->devname, loopback);
	break;

    case ENET0:
    case ENET1:
    case ENET2:
    case OMNI_NET:
    case FDDI:
    case TRNET:
	if (net_flag == 0 && (next_net == 0 || next_net == test_id))
	{
	  net_flag = test_id;	/* turn on the flag */
	  next_net = 0;
	  (void)sprintf(opt_code, "s net_%s", tests[test_id]->devname);
	  if (!((int)tests[test_id]->data & 1))	/* whether to run spray test */
	    (void)strcat(opt_code, " S");
	  (void)sprintf(opt_code, "%s D=%d", opt_code,
				(int)tests[test_id]->special);
	  if ((int)tests[test_id]->data & 2)	/* driver warning messages */
	    (void)strcat(opt_code, " W");
	  break;
	}

	if (next_net == 0)	/* registered so it will be started next */
	  next_net = test_id;
	else if (!tests[next_net]->enable)
	  next_net = 0;				/* to prevent deadlock */

	if (net_flag != 0)
	  if (tests[net_flag]->pid == 0)	/* to prevent deadlock */
	    net_flag = 0;

	return(1);				/* can't run it yet */

    case IPC:
	(void)sprintf(opt_code, "s ipc%d", tests[test_id]->unit);
	if ((int)tests[test_id]->data & 1)	/* test floppy drive too */
	  (void)strcat(opt_code, " D");
	if ((int)tests[test_id]->data & 2)	/* test printer port too */
	  (void)strcat(opt_code, " P");
	break;

    case BW2:
	(void)sprintf(opt_code, "s /dev/%s", tests[test_id]->devname);
	break;

    case COLOR3:
	(void)sprintf(opt_code, "s /dev/%s", tests[test_id]->devname);
	if (fb_delay != 0) return(1);
	break;

    case COLOR4:
	(void)strcat(opt_code, " /dev/cgfour0");
	if (fb_delay != 0) return(1);
	break;

    case COLOR6:
	(void)sprintf(opt_code, "s D=/dev/%s", tests[test_id]->devname);
    case COLOR8:
	if (fb_delay != 0) return(1);
	break;

#ifdef	sun386
    case COLOR5:
	(void)strcat(opt_code, " /dev/cgfive0");
#else	sun386
    case COLOR2:
	(void)strcat(opt_code, " /dev/cgtwo0");
    case COLOR5:
    case COLOR9:
    case GP2:
#endif	sun386
	if (fb_inuse != 0) return(1);		/* can't run it now */
	if (fb_delay != 0 && fb_id == tests[test_id]->id) return(1);
	if (fb_delay > FB_WAIT/2 && fb_id > tests[test_id]->id) return(1);
	/* give other fb tests a chance to start */

	++fb_inuse;
	fb_id = tests[test_id]->id;		/* keep track who is using it */

	if (tests[test_id]->id == COLOR9)
	  (void)strcat(opt_code, " 9");
	break;

    case TV1:
	if ((int)tests[test_id]->data & 1)	/* NTSC loopback mode */
	  (void)strcat(opt_code, " ntsc");
	if ((int)tests[test_id]->data & 2)	/* YC loopback mode */
	  (void)strcat(opt_code, " yc");
	if ((int)tests[test_id]->data & 4)	/* YUV loopback mode */
	  (void)strcat(opt_code, " yuv");
	if ((int)tests[test_id]->data & 8)	/* RGB loopback mode */
	  (void)strcat(opt_code, " rgb");
	break;

    case GP:
	if ((int)tests[test_id]->data & 1)	/* run the gb test too */
	  (void)strcat(opt_code, " gb");
	break;

    case PRESTO:
        if ((int)tests[test_id]->data & 1)      /* run the presto test too */
          (void)strcat(opt_code, " e");
        break;

    case MTI:
    case MCP:
    case SCSISP1:
    case SCSISP2:
	if (tests[test_id]->id == MCP)
	{
	  if (mcp_flag[tests[test_id]->unit] != 0) return(1);	/* can't run */
	  mcp_flag[tests[test_id]->unit] = 1;
	  unit = (unsigned int)'h' + tests[test_id]->unit;
	}
	else if (tests[test_id]->id == MTI)
	  unit = (unsigned int)'0' + tests[test_id]->unit;
	else if (tests[test_id]->id == SCSISP1)
	  unit = 's';				/* first SCSI serial ports */
	else
	  unit = 't';				/* second SCSI serial ports */
	
	(void)sprintf(opt_code, "s D=%s /dev/tty%c%s /dev/tty%c%s",
		tests[test_id]->devname,
		unit, ((struct loopback *)(tests[test_id]->data))->from,
		unit, ((struct loopback *)(tests[test_id]->data))->to);
	break;

    case SCP:
    case SCP2:
	if (tests[test_id]->id == SCP)
	{
	  unit = (unsigned int)'a' + tests[test_id]->unit;
	  (void)sprintf(dev_name, "dcp%c", unit);
	}
	else
	{
	  if (mcp_flag[tests[test_id]->unit] != 0) return(1);	/* can't run */
	  mcp_flag[tests[test_id]->unit] = 1;
	  (void)strcpy(dev_name, "mcp");
	}

	(void)sprintf(opt_code, "s %s%s %s%s",
		dev_name, ((struct loopback *)(tests[test_id]->data))->from,
		dev_name, ((struct loopback *)(tests[test_id]->data))->to);

	if (tests[test_id]->id == SCP && dcp_kernel[tests[test_id]->unit] == 0)
	  (void)strcat(opt_code, " k");		/* load the dcp kernel too */
	break;

    case SPIF:
	(void)sprintf(opt_code, "s ");
	if (type = (( (int)tests[test_id]->data & 0xf000000) >> 24) )
	for (i = 0; i < 8; i++) {
	    if (type == i) {
		if ( (int)tests[test_id]->unit < 2 ) {
	    	    sprintf(tmp_buf, " D=/dev/ttyz0%x ", 
			(int)tests[test_id]->unit * 8 + i);
		} else {
	    	    sprintf(tmp_buf, " D=/dev/ttyz%x ", 
			(int)tests[test_id]->unit * 8 + i);
		} 
	    }
	}
	if (type == 0) 
	{
	  if ( (int)tests[test_id]->unit < 2 ) {
		sprintf(tmp_buf, " D=/dev/ttyz0%x ", 
			(int)tests[test_id]->unit * 8);
	  } else {
		sprintf(tmp_buf, " D=/dev/ttyz%x ", 
			(int)tests[test_id]->unit * 8);
	  } 
	}
	else if (type == 8) 
	{
	        sprintf(tmp_buf, " D=sb%d ", (int)tests[test_id]->unit + 1);
	}
	else
	{
	  if (type = (( (int)tests[test_id]->data & 64) >> 6) ) 
		sprintf(tmp_buf, "D=/dev/stclp%d ", (int)tests[test_id]->unit);
	}

	/* build in the device arguement */ 
	strcat(opt_code, tmp_buf);

	/* Initialize T and Set the test selected */
	T = 0 ;
	if (type = ((int)tests[test_id]->data & 1) ) 
		T=1;
	if (type = (( (int)tests[test_id]->data & 2) >> 1) ) 
		T += 2;
	if (type = (( (int)tests[test_id]->data & 4) >> 2 ) )
		T += 4;
	if (type = (( (int)tests[test_id]->data & 8) >> 3) ) 
		T += 8;
	if (type = (( (int)tests[test_id]->data & 0x10) >> 4) )
		T += 16;
	if (T == 0)
	    strcat(opt_code, " T=1 ");
	else {
	    sprintf(tmp_buf, "T=%d ", T);
	    strcat(opt_code, tmp_buf);
	}

	type = (( (int)tests[test_id]->data & 0xf00) >> 8);
	switch (type)
	{
	  case 0:
		strcat(opt_code, " B=110");
		break;
	  case 1:
		strcat(opt_code, " B=300");
		break;
	  case 2:
		strcat(opt_code, " B=600");
		break;
	  case 3:
		strcat(opt_code, " B=1200");
		break;
	  case 4:
		strcat(opt_code, " B=2400");
		break;
	  case 5:
		strcat(opt_code, " B=4800");
		break;
	  case 6:
		strcat(opt_code, " B=9600");
		break;
	  case 7:
		strcat(opt_code, " B=19200");
		break;
	  case 8:
		strcat(opt_code, " B=38400");
		break;
	  default:
		strcat(opt_code, " B=9600");
		break;
	}

	type = (( (int)tests[test_id]->data & 0xf000) >> 12);
	switch (type)
	{
	  case 0:
		strcat(opt_code, " C=5");
		break;
	  case 1:
		strcat(opt_code, " C=6");
		break;
	  case 2:
		strcat(opt_code, " C=7");
		break;
	  case 3:
		strcat(opt_code, " C=8");
		break;
	  default:
		strcat(opt_code, " C=8");
		break;
	}

	type = (( (int)tests[test_id]->data & 0x20) >> 5); /* stop bit */
	switch (type)
	{
	  case 0:
		strcat(opt_code, " S=1");
		break;
	  case 1:
		strcat(opt_code, " S=2");
		break;
	  default:
		strcat(opt_code, "S=1");
		break;
	}

	type = (( (int)tests[test_id]->data & 0xf0000) >> 16); /* Parity */
	switch (type)
	{
	  case 0:
		strcat(opt_code, " P=none"); 	/* none */
		break;
	  case 1:
		strcat(opt_code, " P=odd"); 	/* odd */
		break;
	  case 2:
		strcat(opt_code, " P=even"); 	/* even */
		break;
	  default:
		strcat(opt_code, " P=none");
		break;
	}

	type = (( (int)tests[test_id]->data & 0xf00000) >> 20); /* flow ctl */
	switch (type)
	{
	  case 0:
		strcat(opt_code, " F=xonoff");
		break;
	  case 1:
		strcat(opt_code, " F=rtscts");
		break;
	  case 2:
		strcat(opt_code, " F=both");
		break;
	  default:
		strcat(opt_code, " F=rtscts");
		break;
	}

	type = (( (int)tests[test_id]->data & 0xf0000000) >> 28); /* pattern */
	switch (type)
	{
	  case 0:
		strcat(opt_code, " I=5");
		break;
	  case 1:
		strcat(opt_code, " I=a");
		break;
	  case 2:
		strcat(opt_code, " I=r");
		break;
	  default:
		strcat(opt_code, " I=5");
		break;
	}

	/*
	printf("spiftest Command args= %s\n", opt_code);
	*/
	break;

    case HSI:
	if ((int)tests[test_id]->data & 16) clock = 'e';
	else clock = 'o';
	unit = (int)tests[test_id]->data & 3;
	type = ((int)tests[test_id]->data & 12) >> 2;

	switch (unit)
	{
	  case 0:			/* 0, 2 */
	  case 1:			/* 1, 3 */
		unit += tests[test_id]->unit*2;
		switch (type)
		{
		  case 1:
			(void)sprintf(opt_code, "s hss%c%dv", clock, unit);
			break;
		  case 2:
			(void)sprintf(opt_code, "s hss%c%dr,%c%dv",
					clock, unit, clock, unit);
			break;
		  default:
			(void)sprintf(opt_code, "s hss%c%dr", clock, unit);
			break;
		}
		break;

	  case 2:			/* 0 1, 2 3 */
		unit = tests[test_id]->unit*2;
		switch (type)
		{
		  case 1:
			(void)sprintf(opt_code, "s hss%c%dv,%c%dv",
					clock, unit, clock, unit+1);
			break;
		  case 2:
			(void)sprintf(opt_code,
				"s hss%c%dr,%c%dr,%c%dv,%c%dv",
				clock, unit, clock, unit+1,
				clock, unit, clock, unit+1);
			break;
		  default:
			(void)sprintf(opt_code, "s hss%c%dr,%c%dr",
					clock, unit, clock, unit+1);
			break;
		}
		break;

	  case 3:			/* 0-1, 2-3 */
	  default:
		unit = tests[test_id]->unit*2;
		switch (type)
		{
		  case 1:
			(void)sprintf(opt_code, "s hss%c%dv hss%c%dv",
					clock, unit, clock, unit+1);
			break;
		  case 2:
			(void)sprintf(opt_code,
				"s hss%c%dr,%c%dv hss%c%dr,%c%dv",
				clock, unit, clock, unit,
				clock, unit+1, clock, unit+1);
			break;
		  case 3:
			(void)sprintf(opt_code,
				"s hss%c%dr,%c%dv hss%c%dv,%c%dr",
				clock, unit, clock, unit,
				clock, unit+1, clock, unit+1);
			break;
		  default:
			(void)sprintf(opt_code, "s hss%c%dr hss%c%dr",
					clock, unit, clock, unit+1);
			break;
		}
		break;
	}
	break;

    case SBUS_HSI:
        if ((int)tests[test_id]->data & 1)
            clock = 'e';  /* in order to use external clock source, the user */
        else              /* must physically loopback xmit & recv pins and
*/
            clock = 'o';  /* provide external clock source */

        unit = ((int)tests[test_id]->data & 0x070) >> 4;   /* loopback type */

        internal_loopback = (((int)tests[test_id]->data & 2) >> 1);

        switch (unit)
        {
          case 0:                       /* 0, 4, 8  */
          case 1:                       /* 1, 5, 9  */
          case 2:                       /* 2, 6, 10 */
          case 3:                       /* 3, 7, 11 */
                unit += tests[test_id]->unit*4;
                        (void)sprintf(opt_code, "s hih%c%d", clock, unit);
                break;

          case 4:                       /* 0 1 2 3 */
                unit = tests[test_id]->unit*4;
                        (void)sprintf(opt_code, "s hih%c%d,%c%d,%c%d,%c%d",
                              clock, unit  , clock, unit+1,
                              clock, unit+2, clock, unit+3);
                break;

          case 5:                       /* 0-1, 2-3 */
          default:
                unit = tests[test_id]->unit*4;
                        (void)sprintf(opt_code,"s i%d hih%c%d,%c%d %s%c%d,%c%d",
                              clock, unit  , clock, unit+2,
                              clock, unit+1, clock, unit+3);
                break;

          case 6:                       /* 0-2, 1-3 */
                unit = tests[test_id]->unit*4;
                        (void)sprintf(opt_code,"s i%d hih%c%d,%c%d %s%c%d,%c%d",
                              clock, unit  , clock, unit+1,
                              clock, unit+2, clock, unit+3);
                break;

          case 7:                       /* 0-3, 1-2 */
                unit = tests[test_id]->unit*4;
                        (void)sprintf(opt_code,"s i%d hih%c%d,%c%d %s%c%d,%c%d",
                              clock, unit  , clock, unit+1,
                              clock, unit+3, clock, unit+2);
                break;
        }
        if (internal_loopback)
                    (void)strcat(opt_code, " I");
        break;

    case PRINTER:
	(void)sprintf(opt_code, "s p%d", tests[test_id]->unit);
	break;

    case CG12:                 /* Egret graphic accelerator test */
        (void)sprintf(opt_code, "s D=/dev/%s F=%d B=%d S=%d",
                tests[test_id]->devname, (int)tests[test_id]->data >> 12,
                (int)tests[test_id]->special, 
		~(int)tests[test_id]->data & 0x7FF); /* bit 1/0 == on/off */ 
	break;

    case GT:                 /* Hawk graphic accelerator test */
        (void)sprintf(opt_code, "s D=/dev/%s S=%d F=%d B=%d",
                tests[test_id]->devname, (int)tests[test_id]->data & 0x1FFFFF,
                (int)tests[test_id]->data >> 21,
                (int)tests[test_id]->special); /* bit 1/0 == on/off */
        break;

    case MP4:                   /* Multiple processors (Galaxy) */
        (void)sprintf(opt_code, "s T=%d C=%d", 
              ~((int)tests[test_id]->data) & 0xf,
              (~(((int)tests[test_id]->data & 0xf0) >> 4)) & processors_mask);
        break;

    case ZEBRA1:		/* Sbus bidirectional parallel (printer) port */
	(void)sprintf(opt_code, "s D=/dev/%s W M=%s",
		tests[test_id]->devname, 
		bpp_mode[(int)tests[test_id]->data >> 2]);
	break;
    case ZEBRA2:		/* SBus serial video port */
	(void)sprintf(opt_code, "s D=/dev/%s W M=%s I=%s R=%s",
		tests[test_id]->devname,
		lpvi_mode[((int)tests[test_id]->data & 0xC)>> 2],
		lpvi_image[((int)tests[test_id]->data & 0x30)>> 4],
		lpvi_resolution[((int)tests[test_id]->data & 0xC0)>> 6]);
	break;
    case USER:
	if (tests[test_id]->env != NULL)
	  (void)strcat(opt_code, tests[test_id]->env);	/* get command tail */
	break;

    default:
	break;
  }

  if (core_file) (void)strcat(opt_code, " c");	/* generate core files */
  if (single_pass) (void)strcat(opt_code, " p");/* single pass mode */
  if (verbose) (void)strcat(opt_code, " v");	/* verbose mode */
  if (trace) (void)strcat(opt_code, " t");	/* trace mode */
  if (quick_test) (void)strcat(opt_code, " q"); /* quick test mode */
  if (run_error) (void)strcat(opt_code, " r");  /* run_on_error mode */

  return(0);				/* the test is O.K. to be run */
}

/******************************************************************************
 * after_test(), checks to see whether there is anything needs to be done to  *
 * clean up after the specified test is done.				      *
 * Input: test_id, internal test number.				      *
 * Output: none.							      *
 ******************************************************************************/
static	int	after_test(test_id)
{
  switch (tests[test_id]->id)		/* check which test it is */
  {
    case VMEM:
	vmem_wait = elapse_count;	/* remember when did VMEM exit */
	break;

    case COLOR3:
    case COLOR4:
    case COLOR6:
    case COLOR8:
	fb_delay = FB_WAIT;
	/* delay at least FB_WAIT seconds before it can be restarted */
	break;

    case COLOR2:
    case COLOR5:
    case COLOR9:
    case GP2:
	if (fb_inuse > 0)
	  if (--fb_inuse == 0)
	    fb_delay = FB_WAIT;
	    /* delay at least FB_WAIT seconds before it can be restarted */
	break;

    case ENET0:
    case ENET1:
    case ENET2:
    case OMNI_NET:
    case FDDI:
    case TRNET:
	net_flag = 0;				/* allow others to start */
	break;

    case MCP:
    case SCP2:
	mcp_flag[tests[test_id]->unit] = 0;	/* reset the occupied flag */
	break;

    case SCP:
	dcp_kernel[tests[test_id]->unit] = 1;
	/* don't need to load kernel next time */
	break;

    default:				/* nothing to be done */
	break;
  }
}

/******************************************************************************
 * The child process termination notify procedure, to be called when one of   *
 * the forked tests exits.						      *
 ******************************************************************************/
/*ARGSUSED*/
static	Notify_value wait_child(me, pid, status, rusage)
int *me;
int pid;
union wait *status;
struct rusage *rusage;
{
  int	idx;
  char	tmp_msg[82];
  char	corename[82];			/* new name of the core file */

	if (sundiag_done) return(NOTIFY_IGNORED);	/* quitting sundiag */

        /* Note: Could handle child stopping differently from child death */
        /* Break out of notification loop */

        if (WIFSTOPPED(*status)) return(NOTIFY_IGNORED);

        if ((idx = get_pid(pid)) == -1)
	{
		(void)sprintf(tmp_msg, pid_valid_error, pid);
		logmsg(tmp_msg, 2, PID_VALID_ERROR); /* "pid %d is not valid" */
                return(NOTIFY_IGNORED);
	}
	else
	{
		if (status->w_coredump)
		{
		  (void)sprintf(corename,"core.%s.XXXXXX",tests[idx]->testname);
		  (void)mktemp(corename);  /* make temporary file's name */
		  (void)sprintf(tmp_msg, core_dump_error,
                        tests[idx]->devname, tests[idx]->testname, pid);
					/* "(%s) %s(pid %d) dumped core"*/
		  if (rename("core", corename) == 0)
		    (void)sprintf(tmp_msg,"%s Core file= %s",tmp_msg,corename);
		  logmsg(tmp_msg, 2, CORE_DUMP_ERROR);
		  if (!ats_nohost)
		    send_test_msg(SUNDIAG_ATS_ERROR, 0,
		    tests[idx]->testname, tests[idx]->devname,
		    tmp_msg);
		  ++tests[idx]->error;		/* increase the error count */
		  ++sys_error;
		  error_display();		/* refresh total errors */
		  failed_log(idx);		/* log pass and error counts */
		}
                else if (status->w_termsig)
		{
		  if (status->w_termsig != SIGTERM &&
		      status->w_termsig != SIGINT &&
		      status->w_termsig != SIGHUP)
		  {
		    (void)sprintf(tmp_msg,
			term_signal_error,
				/*"(%s) %s(pid %d) died due to signal %d."*/
			tests[idx]->devname, tests[idx]->testname,
			pid, status->w_termsig);
		    logmsg(tmp_msg, 2, TERM_SIGNAL_ERROR);
		    if (!ats_nohost)
			send_test_msg(SUNDIAG_ATS_ERROR, 0,
			tests[idx]->testname, tests[idx]->devname,
			tmp_msg);
		    ++tests[idx]->error;	/* increase the error count */
		    ++sys_error;
		    error_display();		/* refresh total errors */
		    failed_log(idx);		/* log pass and error counts */
		  }
		}
        	else if (status->w_retcode)
		{
		  if (status->w_retcode != 98)	/* == 98, if been killed */
		  {
		    ++tests[idx]->error;	/* increase the error count */
		    ++sys_error;
		    error_display();		/* refresh total errors */

		    if (status->w_retcode == 97)/* RPC send_log_msg() failed */
		    {
			(void)sprintf(tmp_msg,
			  send_message_error,
			  	/*"(%s) %s failed sending message to sundiag."*/
			  tests[idx]->devname, tests[idx]->testname);
	  		logmsg(tmp_msg, 2, SEND_MESSAGE_ERROR);
			/* log to INFO & ERROR files */
			if (!ats_nohost)
			  send_test_msg(SUNDIAG_ATS_ERROR, 0,
				tests[idx]->testname, tests[idx]->devname,
				tmp_msg);
		    }
                    else if (status->w_retcode == 94) /* RPC init failed */
                    {    
                        (void)sprintf(tmp_msg, rpcinit_error, 
                                /*"(%s) %s failed to initialize RPC."*/
			  tests[idx]->devname, tests[idx]->testname);
                        logmsg(tmp_msg, 2, RPCINIT_ERROR);
			/* log to INFO & ERROR files */
			if (!ats_nohost)
			  send_test_msg(SUNDIAG_ATS_ERROR, 0,
				tests[idx]->testname, tests[idx]->devname,
				tmp_msg);
                    }
		    else if (status->w_retcode == 255)
		    /* error in forking(lower 8 bits of -1) */
		    {
			(void)sprintf(tmp_msg, fork_error,
				tests[idx]->testname, tests[idx]->devname);
				/* "Failed forking %s for device (%s)" */
	  		logmsg(tmp_msg, 2, FORK_ERROR);	
				/* log to INFO & ERROR files */

			if (!ats_nohost)
			  send_test_msg(SUNDIAG_ATS_ERROR, 0,
				tests[idx]->testname, tests[idx]->devname,
				tmp_msg);
		    }

		    failed_log(idx);	/* log pass and error counts */
		  }
		}
                else
		{
		  ++tests[idx]->pass;		/* increase the pass count */
		  if (!ats_nohost)
		    send_start_stop(SUNDIAG_ATS_STOP, tests[idx]->testname,
				tests[idx]->devname, tests[idx]->pass);
		}

		tests[idx]->pid = 0;

		--groups[tests[idx]->group].tests_no;

		after_test(idx);		/* clean up after testing */

		if (tests[idx]->dev_enable && tests[idx]->enable &&
			tests[idx]->type != 2)
		  update_status(idx);  	/* update the status on screen */
		else			/* killed while running */
		  print_status();	/* redisplay the entire status screen */

		return(NOTIFY_IGNORED);
        }
}

/******************************************************************************
 * build_env() sets the environmental variables(if any) to be used by the     *
 *	to-be-forked test.	     					      *
 * Input: test_id, the internal test number.				      *
 * Output: none.							      *
 ******************************************************************************/
static	build_env(test_id)
int	test_id;
{
  char	*tmp;
  char	*env;

  if ((tmp = tests[test_id]->environ) == NULL) return;
				/* no environmental variables to be set */
  env = tests[test_id]->env;

  (void)strcpy(env, tmp);  /* make a new copy of it from tests[]->environ */

  if ((tmp=strtok(env, ",")) != NULL)
  {
    do
	(void)putenv(tmp);
    while ((tmp=strtok((char *)NULL, ",")) != NULL);
  }
}

static char  tmp4[82];
/******************************************************************************
 * build_arg() builds the argument array to be used by execvp().	      *
 * Input: test_id, the internal test number.				      *
 * Output: child_arg[] was filled with proper arguments.		      *
 ******************************************************************************/
static	build_arg(test_id)
int	test_id;
{
  int	i=1;
  char	*tmp, *tail;

  child_arg[0]= tests[test_id]->priority;	/* pass the priority first */

  if (remotehost != NULL)		/* remote execution */
  {
    child_arg[i++]= remotehost;		/* remote host name */
    sprintf(tmp4,"%s", tests[test_id]->testname);
  }
  else
  {
    child_arg[i++]= "localhost";	/* local execution  */
    sprintf(tmp4,"./%s", tests[test_id]->testname);
  }
  child_arg[i++]= tmp4; 		/* followed by test name */

  tail = tests[test_id]->tail;

  if (tail == NULL)			/* no command line tail at all */
  {
    child_arg[i] = NULL;
    return;
  }

  if ((tmp=strtok(tail, " \t")) != NULL)
  {
    do
      child_arg[i++] = tmp;
    while (i != MAX_ARG-1 && (tmp=strtok((char *)NULL, " \t")) != NULL);
  }

  child_arg[i] = NULL;			/* NULL-terminated after the last one */
}

/******************************************************************************
 * fork_test(), forks "dispatcher", which will, in turn, fork the passed test.*
 * Input: test_id, internal test number.				      *
 * Output: 1, if failed; 0, if succeed.					      *
 ******************************************************************************/
static	int	fork_object;
static	int	*fork_handle = &fork_object;

static	int	fork_test(test_id)
int	test_id;		/* the test to be forked, index to tests[] */
{
  int	childpid;
  struct rusage rusage;
  int memfd;

	if (before_test(test_id)) return(1);
	/* check the "runnability" of the test, also build the command tail */

	mark_close_on_exec();

	if (!ats_nohost)
	  send_start_stop(SUNDIAG_ATS_START, tests[test_id]->testname,
			tests[test_id]->devname, tests[test_id]->pass);


	childpid = vfork();

	if(childpid == 0)	/* initialize the environment for the tests */
	{
	  childpid = getpid();

	  (void)dup2(tty_fd, 1);    /* redirect stdout & stderr to /dev/null */
	  (void)dup2(tty_fd, 2);

	  build_env(test_id);	/* build the environmental variables(if any) */
	  build_arg(test_id);	/* build the arguments for child */

          /* attach the system processors to run other tests except mptest */
          memfd = open("/dev/null", 0);
	  ioctl(memfd, MIOCSPAM, &system_processors_mask);
          close(memfd);

          execvp("./dispatcher", child_arg);
	  perror("Failed EXECVPing dispatcher");	/* execvp() failed */
	  _exit(-1);
    	}


	(void)notify_set_wait3_func((Notify_client)fork_handle,
						wait_child, childpid);

	tests[test_id]->pid = childpid;		/* keep the child's pid */
	update_status(test_id);
	/* indicate the test is running now, on status subwindow */


	if (running == SUSPEND)			/* timing problem */
	  (void)kill(childpid, SIGSTOP);
	else if (running != GO)
	  (void)kill(childpid, SIGINT);
	
	return(0);
}

/******************************************************************************
 * mark_close_on_exec() marks all open files(except 0, 1, and 2) to be closed *
 *	when a child is forked.						      *
 ******************************************************************************/
static	int mark_close_on_exec()
{
  register i;

        /* Mark all fds (other than stds) as close on exec */
        for (i = 3; i < NOFILE; i++)
	  (void)fcntl(i, F_SETFD, 1);	/* -JCH- ignore the error */
}

/******************************************************************************
 * get_pid() returns the internal test number, given the pid.		      *
 * Input: pid, pid of the running test.					      *
 * Output: internal test number of the test; -1, if not a valid pid.	      *
 ******************************************************************************/
static	int get_pid(pid)
int	pid;
{
  int	index;

  for (index=0; index != exist_tests; ++index)
    if (tests[index]->pid == pid) return(index);

  return(-1);				/* not a valid pid */
}

/******************************************************************************
 * invoke_tests() checks the test data base to see if there is any test that  *
 * is ready to be forked. It will be called once every second when system     *
 * status was set to "Testing"(by hitting "Start Tests" button).	      *
 ******************************************************************************/
invoke_tests()
{
  int	max_no;			/* max # of tests can be run in a group */
  int	index;			/* the test index currently being checking */
  int	last;			/* the test index, which was lastly forked */
  int	first;			/* the first test index in this group */
  int	i;			/* loop count(the group id too) */

  if (fb_delay != 0) --fb_delay;	/* count down once every instance */

  i = last_group+1;

  do
  /* check through the group table */
  {
    if (i >= ngroups) i = 0;
    if (!groups[i].enable) continue;	/* the whole group is not enabled */

    max_no = groups[i].max_no;
    if (groups[i].tests_no >= max_no) continue;	/* already full */

    first = groups[i].first;
    last = groups[i].last;

    if (last == -1)			/* the first time since started */
    {
	for (last=first; tests[last]->group == i;)
	  if (++last == exist_tests) break;

	--last;	/* set the last invoked test to the last test in the group */
    }

    index = last;

    while (TRUE)		/* until break */
    {
	if (++index == exist_tests) index = first;
	if (tests[index]->group != i) index = first;  /* hit the end of group */

	if (tests[index]->enable && tests[index]->dev_enable &&
		tests[index]->pid == 0 && tests[index]->type != 2)
	{
	  groups[i].last = index;

	  if (!fork_test(index))	/* start this test */
	  {
	    ++groups[i].tests_no;
	    last_group = i;
	    return;			/* one test forked at a time */
	  }
	  else if (running == GO)
	    break;			/* skip to next group */
	  else
	    return;			/* stopped by exceeding error count */
	}
	if (index == last) break;	/* one cycle alreday */
    }
  }
  while (i++ != last_group);
}

/******************************************************************************
 * kill_all_tests() kills all currently running tests(by SIGINT).	      *
 ******************************************************************************/
kill_all_tests()
{
  int	test_id;
  int	tmp;

  if (running == SUSPEND)			/* restart them first */
    for (test_id=0; test_id != exist_tests; ++test_id)
    {
      if (tests[test_id]->pid != 0)	/* it is loaded */
	(void)kill(tests[test_id]->pid, SIGCONT);
    }

  tmp = running;			/* save running before changing */
  running = KILL;

  for (test_id=0; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->pid != 0)	/* it is still running */
      if (kill(tests[test_id]->pid, SIGINT) != 0)	/* can't kill it */
	if (tmp == KILL) tests[test_id]->pid = 0;	/* assumed killed */
  }
}

/******************************************************************************
 * check_term() checks to see whether all the killed tests are really 	      *
 *	terminated. It will also change the system status when all tests have *
 * 	exited.								      *
 ******************************************************************************/
check_term()
{
  int	test_id;

  for (test_id=0; test_id != exist_tests; ++test_id)
    if (tests[test_id]->pid != 0)
	break;				  /* at least one is still running */

  if (test_id == exist_tests)		/* none is running now */
  {
	running = IDLE;			/* idle */
	status_display();
	log_status();	/* also log current status to information file */

	if (tty_mode) return;	/* that's it for TTY mode */

	(void)panel_set(test_item,
	  PANEL_LABEL_IMAGE, start_button,
	  PANEL_NOTIFY_PROC, start_proc, 0);

	(void)panel_set(reset_item,
	  PANEL_LABEL_IMAGE, reset_button,
	  PANEL_NOTIFY_PROC, reset_proc, 0);

	light_all_buttons();		/* un-gray the unaccessible buttons */
  }
}

/******************************************************************************
 * select_test: enable/disable the passed test.			      	      *
 * Input: internal test number, enable/disable flag.			      *
 * Output: none.							      *
 ******************************************************************************/
select_test(test_id, enable)
int	test_id;			/* internal test number */
int	enable;				/* enable/disable flag */
{
  if (enable)				/* test is to be enabled */
  {
      if (running == GO || running == SUSPEND)	/* tests are running */
        start_log(test_id);		/* log to information file */
  }
  else					/* test is to be killed */
  {
      if (tests[test_id]->pid != 0)	/* test is currently running */
	(void)kill(tests[test_id]->pid, SIGINT);

      if (running == GO || running == SUSPEND)	/* tests are running */
        stop_log(test_id);		/* log to information file */
  }
}

/******************************************************************************
 * multi_tests: enable/disable the tests for a device which has more than one *
 *		tests.							      *
 * Input: internal test number, disable/enable flag.			      *
 * output: none.							      *
 ******************************************************************************/
multi_tests(test_id, enable)
int	test_id;			/* internal test number */
int	enable;				/* enable/disable flag */
{
  --test_id;

  if (enable)		/* to enable the tests for this device */
  {
    do
    {
      ++test_id;    
      tests[test_id]->dev_enable = ENABLE;

      if (tests[test_id]->enable)
        select_test(test_id, ENABLE);
    }
    while (tests[test_id]->which_test != tests[test_id]->test_no);
  }
  else			/* to disable the tests for this device */
  {
    do
    {
      ++test_id;
      tests[test_id]->dev_enable = DISABLE;

      select_test(test_id, DISABLE);
    }
    while (tests[test_id]->which_test != tests[test_id]->test_no);
  }

  print_status();			/* display it */
}
