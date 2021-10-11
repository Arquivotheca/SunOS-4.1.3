#ifndef	lint
static  char sccsid[] = "@(#)opfile.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "sundiag.h"
#include "../../lib/include/probe_sundiag.h"
#include "struct.h"
#ifndef	sun386
#include <sys/mtio.h>
#else
#include "mtio.h"
#endif
#include <sys/file.h>
#include "sundiag_msg.h"

extern char run_time_file[];
extern long runtime;
extern char *vmem_waittime[];
extern char audbri_ref_file[];

extern	char	*strtok(), *fgets(), *malloc();
extern	char	*strcpy(), *sprintf();

static	char	*enable="enable";
static	char	*disable="disable";

/******************************************************************************
 * system_select(), selects the tests according to specification.	      *
 * Input: value, could be SEL_DEF, SEL_NON, or SEL_ALL.			      *
 * Output: none.							      *
 ******************************************************************************/
static	void	system_select(value)
int	value;
{
  int	test_id, group_id;

  if (!tty_mode)
    (void)panel_set(select_item, PANEL_VALUE, value, 0);

  switch (value)
  {
    case SEL_DEF:
	/* disable all the tests first */
	group_id = -1;		/* initialize the id(to flag the first group) */
	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	    if (tests[test_id]->type == 2)
		continue;	/* skip this */

	    if (group_id != tests[test_id]->group)  /* clear the group too */
	    {
		group_id = tests[test_id]->group;	/* save it for next */
		groups[group_id].enable = FALSE;
	    }

	    tests[test_id]->enable = FALSE;
	    if (tests[test_id]->test_no > 1)
	    {
		tests[test_id]->dev_enable = FALSE;
		if (tests[test_id]->which_test > 1) continue;
	    }
	}

	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	  if (tests[test_id]->type == 0)	/* this is a default test */
	  {
	    group_id = tests[test_id]->group;
	    if (!groups[group_id].enable)
		groups[group_id].enable = TRUE;
	    tests[test_id]->enable = TRUE;

	    if (tests[test_id]->test_no > 1)
		tests[test_id]->dev_enable = TRUE;
	  }
	}

	break;

    case SEL_NON:
	group_id = -1;		/* initialize the id(to flag the first group) */
	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	    if (tests[test_id]->type == 2)
		continue;	/* skip this */

	    if (group_id != tests[test_id]->group)  /* clear the group too */
	    {
		group_id = tests[test_id]->group;	/* save it for next */
		groups[group_id].enable = FALSE;
	    }
	    tests[test_id]->enable = FALSE;

	    if (tests[test_id]->test_no > 1)
		tests[test_id]->dev_enable = DISABLE;
	}
	break;

    case SEL_ALL:
	group_id = -1;		/* initialize the id(to flag the first group) */
	for (test_id=0; test_id != exist_tests; ++test_id)
	{
	    if (tests[test_id]->type == 2)
		continue;	/* skip this */

	    if (group_id != tests[test_id]->group)	/* set the group too */
	    {
		group_id = tests[test_id]->group;	/* save it for next */
		groups[group_id].enable = TRUE;
	    }
	    tests[test_id]->enable = TRUE;

	    if (tests[test_id]->test_no > 1)
		tests[test_id]->dev_enable = ENABLE;
	}
	break;

    default:
	break;
  }
}

/******************************************************************************
 * store_system_options(), store the system-wide options to the specified     *
 *	file.								      *
 * Input: opt_fp, the file pointer of the file to be stored to.		      *
 * Output: none.							      *
 ******************************************************************************/
static void store_system_options(opt_fp)
FILE	*opt_fp;
{
  char	*temp;

  (void)fprintf(opt_fp, "\n#Followings are system options:\n");

  switch (select_value)
  {
    case 1: temp = "none"; break;
    case 2: temp = "all"; break;
    default: temp = "default";
  }

  (void)fprintf(opt_fp, "option test_selections %s\n", temp);
  (void)fprintf(opt_fp, "option selection_flag %s\n",
				selection_flag?enable:disable);
  (void)fprintf(opt_fp, "option intervention %s\n",
				intervention?enable:disable);
  (void)fprintf(opt_fp, "option core_files %s\n",
				core_file?enable:disable);
  (void)fprintf(opt_fp, "option single_pass %s\n",
				single_pass?enable:disable);
  (void)fprintf(opt_fp, "option quick_test %s\n",
				quick_test?enable:disable);
  (void)fprintf(opt_fp, "option verbose %s\n",
				verbose?enable:disable);
  (void)fprintf(opt_fp, "option trace %s\n",
				trace?enable:disable);
  (void)fprintf(opt_fp, "option auto_start %s\n",
				auto_start?enable:disable);
  (void)fprintf(opt_fp, "option schedule_file %s\n",
				schedule_file?enable:disable);
  (void)fprintf(opt_fp, "option runtime %s\n", run_time_file); 
  (void)fprintf(opt_fp, "option run_on_error %s\n",
				run_error?enable:disable);
  (void)fprintf(opt_fp, "option max_passes %d\n", max_sys_pass); 
  (void)fprintf(opt_fp, "option max_errors %d\n", max_errors); 
  (void)fprintf(opt_fp, "option concurrent_tests %d\n", max_tests);
  (void)fprintf(opt_fp, "option send_email %s\n",
				send_email==1?"on_error":
				(send_email==2?"periodically":
				(send_email==3?"both":disable)));
  (void)fprintf(opt_fp, "option log_period %d\n", log_period);
  (void)fprintf(opt_fp, "option email_address %s\n", eaddress);
  (void)fprintf(opt_fp, "option printer_name %s\n", printer_name);
}

/******************************************************************************
 * load_system_options(), load the system-wide options from the specified     *
 *	file.								      *
 * Input: param, parsed option tokens.					      *
 * Output: none.							      *
 ******************************************************************************/
load_system_options(param)
char	*param[];
{
  int	endis=9;
  int	i;

  if (strcmp(param[2], enable) == 0) endis = 1;
  else if (strcmp(param[2], disable) == 0) endis = 0;

  if (strcmp(param[1], "intervention") == 0)
  {
    if (endis < 2 && intervention != endis)
    {
      intervention = endis;
      if (intervention)
      {
	for (i=0 ; i != exist_tests; ++i)
	  if (tests[i]->type == 2) tests[i]->type = 12;
	  /* enable intervention */
      }
      else
      {
	for (i=0 ; i != exist_tests; ++i)
	  if (tests[i]->type == 12) tests[i]->type = 2;
	  /* disable intervention */
      }
      if (!tty_mode)
        (void)panel_set(mode_item, PANEL_VALUE, intervention, 0);
    }
  }
  else if (strcmp(param[1], "test_selections") == 0)
  {
    if (strcmp(param[2], "default") == 0) select_value = 0;
    else if (strcmp(param[2], "all") == 0) select_value = 2;
    else select_value = 1;

    system_select(select_value);
  }
  else if (strcmp(param[1], "selection_flag") == 0)
    selection_flag = (endis<2 ? endis : selection_flag);
  else if (strcmp(param[1], "core_files") == 0)
    core_file = (endis<2 ? endis : core_file);
  else if (strcmp(param[1], "single_pass") == 0)
    single_pass = (endis<2 ? endis : single_pass);
  else if (strcmp(param[1], "quick_test") == 0)
    quick_test = (endis<2 ? endis : quick_test);
  else if (strcmp(param[1], "verbose") == 0)
    verbose = (endis<2 ? endis : verbose);
  else if (strcmp(param[1], "trace") == 0)
    trace = (endis<2 ? endis : trace);
  else if (strcmp(param[1], "auto_start") == 0)
    auto_start = (endis<2 ? endis : auto_start);
  else if (strcmp(param[1], "schedule_file") == 0)
    schedule_file = (endis<2 ? endis : schedule_file);
  else if (strcmp(param[1], "runtime") == 0)
  { /* Since delimiter interfears with our format, rebuild it */
	  strcpy(run_time_file, param[2]);
	  strcat(run_time_file, ":");
	  strcat(run_time_file, param[3]);
	  strcat(run_time_file, ":");
	  strcat(run_time_file, param[4]);
	  runtime = get_run_time();
  }
  else if (strcmp(param[1], "run_on_error") == 0)
    run_error = (endis<2 ? endis : run_error);
  else if (strcmp(param[1], "max_passes") == 0)
    max_sys_pass = atoi(param[2]);
  else if (strcmp(param[1], "max_errors") == 0)
    max_errors = atoi(param[2]);
  else if (strcmp(param[1], "concurrent_tests") == 0)
  {
    max_tests = atoi(param[2]);
    set_max_tests(max_tests);
  }
  else if (strcmp(param[1], "send_email") == 0)
  {
    if (strcmp(param[2], "on_error") == 0)
      send_email = 1;
    else if (strcmp(param[2], "periodically") == 0)
      send_email = 2;
    else if (strcmp(param[2], "both") == 0)
      send_email = 3;
    else
      send_email = 0;
  }
  else if (strcmp(param[1], "log_period") == 0)
    log_period = atoi(param[2]);
  else if (strcmp(param[1], "email_address") == 0)
    strcpy(eaddress, param[2]);
  else if (strcmp(param[1], "printer_name") == 0)
    (void)strcpy(printer_name, param[2]);
}

/******************************************************************************
 * store_test_options(), store the individual test options to the specified   *
 *	file.								      *
 * Input: opt_fp, the file pointer of the file to be stored to.		      *
 * Output: none.							      *
 ******************************************************************************/
static void store_test_options(opt_fp)
FILE	*opt_fp;
{
  int	i, j, num, b;
  char	*temp;

  (void)fprintf(opt_fp, "\n#Followings are individual test options:\n");

  for (i=0; i != exist_tests; ++i)  /* go through the entire test structure */
  {
    (void)fprintf(opt_fp, "%s %s %s %s", tests[i]->devname, tests[i]->testname,
	tests[i]->enable?"on":"off", tests[i]->dev_enable?"on":"off");

    switch (tests[i]->id)		/* depend on which test it is */
    {
      case VMEM:
	(void)fprintf(opt_fp, " wait_time:%s", 
		   vmem_waittime[(int)tests[i]->data]);
	(void)fprintf(opt_fp, " reserve:%d\n", (int)tests[i]->special);
	break;

      case AUDIO:
        if (tests[i]->conf->uval.devinfo.status == 0) {
                (void)fprintf(opt_fp, " audio_output:%s",
                                ((int)tests[i]->data&1)?"jack":"speaker");
                (void)fprintf(opt_fp, " volume:%d\n", (int)tests[i]->special);
        }
        else {
                (void)fprintf(opt_fp,
                  " audbri_loopback:%s audbri_type:%d audbri_calib:%s audbri_crystal:%s audbri_controls:%s audbri_audio:%s audbri_ref_file:%s\n",
                        (int)tests[i]->data&0x4?enable:disable,
                        (int)tests[i]->data&0x1,
                        (int)tests[i]->data&0x8?enable:disable,
                        (int)tests[i]->data&0x10?enable:disable,
                        (int)tests[i]->data&0x20?enable:disable,
                        (int)tests[i]->data&0x40?enable:disable,
                        (int)tests[i]->special);
        }

        break;

      case ENET0:
      case ENET1:
      case ENET2:
      case OMNI_NET:
      case FDDI:
      case TRNET:
	(void)fprintf(opt_fp, " spray:%s delay:%d warning:%s\n",
			((int)tests[i]->data&1)?enable:disable,
			(int)tests[i]->special,
			((int)tests[i]->data&2)?enable:disable);
	break;

      case CPU_SP:
	switch ((int)tests[i]->data)
	{
	  case 0: temp = "a"; break;
	  case 1: temp = "b"; break;
	  case 2: temp = "ab"; break;
	  default: temp = "a-b";
	}

	(void)fprintf(opt_fp, " loopback:%s\n", temp);
	break;

      case CPU_SP1:
	switch ((int)tests[i]->data)
	{
	  case 0: temp = "c"; break;
	  case 1: temp = "d"; break;
	  case 2: temp = "cd"; break;
	  default: temp = "c-d";
	}

	(void)fprintf(opt_fp, " loopback:%s\n", temp);
	break;

      case CDROM:
	switch ((int)tests[i]->data & 7)
	{
	   case 1:  temp = "sony2"; break;
	   case 2:  temp = "hitachi4"; break;
	   case 3:  temp = "pdo"; break;
	   case 4:  temp = "other"; break;
	   default: temp = "cdassist";
	}

	(void)fprintf(opt_fp,
	  " cdtype:%s data1:%s data2:%s audiotest:%s volume:%d datatrack:%d readmode:%s\n",
		temp, (int)tests[i]->data&8?disable:enable,
		      (int)tests[i]->data&0x40?disable:enable,
		      (int)tests[i]->data&0x10?disable:enable,
		      (int)tests[i]->special,
		      (int)tests[i]->data>>8,
		      (int)tests[i]->data&0x20?"sequential":"random");
	break;

      case SCSIDISK1:
      case XYDISK1:
      case XDDISK1:
      case IPIDISK1:
      case IDDISK1:
      case SFDISK1:
      case OBFDISK1:

	(void)fprintf(opt_fp, " rawtest_mode:%s",
			(int)tests[i]->data&0x8?"write_read" : "readonly" );
	switch ((int)tests[i]->data & 0x7)
	{
		case  0: temp = "a"; break;
		case  1: temp = "b"; break;
		case  2: temp = "c"; break;
		case  3: temp = "d"; break;
		case  4: temp = "e"; break;
		case  5: temp = "f"; break;
		case  6: temp = "g"; break;
		case  7: temp = "h"; break;
	}
	(void)fprintf(opt_fp, " rawtest_part:%s", temp);
	switch (((int)tests[i]->data & 0x70) >> 4)
	{
		case  0: temp = "100"; break;
		case  1: temp = "200"; break;
		case  2: temp = "400"; break;
		case  3: temp = "600"; break;
		case  4: temp = "800"; break;
		case  5: temp = "1000"; break;
		case  6: temp = "1200"; break;
	}
	(void)fprintf(opt_fp, " rawtest_size:%s\n", temp);
	break;
      case MAGTAPE1:
      case MAGTAPE2:
      case SCSITAPE:

	if (tests[i]->conf->uval.tapeinfo.t_type != MT_ISVIPER1 &&
	    tests[i]->conf->uval.tapeinfo.t_type != MT_ISWANGTEK1)
	{
	  if (tests[i]->conf->uval.tapeinfo.t_type == MT_ISHP || 
	    tests[i]->conf->uval.tapeinfo.t_type == MT_ISKENNEDY)
	  {
	    switch ((int)tests[i]->data & 7)
	    {
	      case 1: temp = "1600-BPI"; break;
	      case 2: temp = "6250-BPI"; break;
	      case 3: temp = "all"; break;
	      case 4: temp = "compression"; break;
	      default: temp = "800-BPI";
	    }
	    (void)fprintf(opt_fp," density:%s", temp);
	  }
	  else if (tests[i]->conf->uval.tapeinfo.t_type == MT_ISXY || 
	    tests[i]->conf->uval.tapeinfo.t_type == MT_ISCDC)
	  {
	    switch ((int)tests[i]->data & 3)
	    {
	      case 1: temp = "1600-BPI"; break;
	      case 2: temp = "6250-BPI"; break;
	      default: temp = "both";
	    }
	    (void)fprintf(opt_fp," density:%s", temp);
	  }
#ifdef NEW
	  else if (tests[i]->conf->uval.tapeinfo.t_type == MT_ISEXB8500)
	  {
	    if (tests[i]->conf->uval.tapeinfo.status == FLT_COMP)
		    switch ((int)tests[i]->data & 7)
		    {
		      case 1: temp = "EXB-8500"; break;
		      case 2: temp = "all"; break;
		      case 3: temp = "compression"; break;
		      default: temp = "EXB-8200";
		    }
	    else
		    switch ((int)tests[i]->data & 3)
		    {
		      case 1: temp = "EXB-8500"; break;
		      case 2: temp = "both"; break;
		      default: temp = "EXB-8200";
		    }
		
	    (void)fprintf(opt_fp," density:%s", temp);
	  }
#endif NEW
	  else
	  {
	    switch ((int)tests[i]->data & 3)
	    {
	      case 1: temp = "QIC-24"; break;
	      case 2: temp = "QIC-11&QIC-24"; break;
	      default: temp = "QIC-11";
	    }
	    (void)fprintf(opt_fp," format:%s", temp);
	  }
	}

	(void)fprintf(opt_fp," mode:%s",
		(int)tests[i]->data&0x100?"readonly":"write_read");

	switch (((int)tests[i]->data & 0x18) >> 3)
	{
	    case 1: temp = "specified"; break;
	    case 2: temp = "long"; break;
	    case 3: temp = "short"; break;
	    default: temp = "eot";
	}
	(void)fprintf(opt_fp," length:%s", temp);

	(void)fprintf(opt_fp," block:%u",
				(unsigned)tests[i]->special);
	(void)fprintf(opt_fp," file_test:%s",
				(int)tests[i]->data&0x20?disable:enable);
	(void)fprintf(opt_fp," streaming:%s",
				(int)tests[i]->data&0x40?enable:disable);
	(void)fprintf(opt_fp," recon:%s",
				(int)tests[i]->data&0x80?enable:disable);
	(void)fprintf(opt_fp," retension:%s",
				(int)tests[i]->data&0x200?disable:enable);
	(void)fprintf(opt_fp," clean_head:%s",
				(int)tests[i]->data&0x400?disable:enable);
	(void)fprintf(opt_fp," pass:%u\n",
				(unsigned)tests[i]->data>>16);
	break;

      case TV1:
	(void)fprintf(opt_fp," ntsc:%s",
				(int)tests[i]->data&1?enable:disable);
	(void)fprintf(opt_fp," yc:%s",
				(int)tests[i]->data&2?enable:disable);
	(void)fprintf(opt_fp," yuv:%s",
				(int)tests[i]->data&4?enable:disable);
	(void)fprintf(opt_fp," rgb:%s\n",
				(int)tests[i]->data&8?enable:disable);
	break;

      case IPC:
	(void)fprintf(opt_fp," floppy:%s",
				(int)tests[i]->data&1?enable:disable);
	(void)fprintf(opt_fp," printer:%s\n",
				(int)tests[i]->data&2?enable:disable);
	break;

      case MCP:
      case MTI:
      case SCP:
      case SCP2:
      case SCSISP1:
      case SCSISP2:
	(void)fprintf(opt_fp, " from:%s$ to:%s$\n",
		((struct loopback *)(tests[i]->data))->from,
		((struct loopback *)(tests[i]->data))->to);
	break;

      case SBUS_HSI:
        if ((int)tests[i]->data & 1) temp = "External";
        else temp = "Baud (on-board)";
        (void)fprintf(opt_fp, " clock_source:%s", temp);

        (void)fprintf(opt_fp," internal_loopback:%s",
                                (int)tests[i]->data & 0x2? enable:disable);
        if (tests[i]->unit == 0)                        /* first board */
            switch (((int)tests[i]->data & 0x070) >> 4)
            {
                case 0 : temp = "0"; break;
                case 1 : temp = "1"; break;
                case 2 : temp = "2"; break;
                case 3 : temp = "3"; break;
                case 4 : temp = "0,1,2,3"; break;
                case 5 : temp = "0-1,2-3"; break;
                case 6 : temp = "0-2,1-3"; break;
                case 7 : temp = "0-3,1-2"; break;
                default: temp = "0,1,2,3"; break;
            }
        else if (tests[i]->unit == 1)                   /* second board */
            switch (((int)tests[i]->data & 0x070) >> 4)
            {
                case 0: temp = "4"; break;
                case 1: temp = "5"; break;
                case 2: temp = "6"; break;
                case 3: temp = "7"; break;
                case 4: temp = "4,5,6,7"; break;
                case 5: temp = "4-5,6-7"; break;
                case 6: temp = "4-6,5-7"; break;
                case 7: temp = "4-7,5-6"; break;
                default: temp = "4,5,6,7"; break;
            }
        else                                            /* third board */
            switch (((int)tests[i]->data & 0x070) >> 4)
            {
                case 0: temp = "8"; break;
                case 1: temp = "9"; break;
                case 2: temp = "10"; break;
                case 3: temp = "11"; break;
                case 4: temp = "8,9,10,11"; break;
                case 5: temp = "8-9,10-11"; break;
                case 6: temp = "8-10,9-11"; break;
                case 7: temp = "8,11,9,10"; break;
                default: temp = "8-9, 10-11"; break;
            }
        (void)fprintf(opt_fp, " loopback:%s\n", temp);

        break;

      case HSI:
	if ((int)tests[i]->data & 16) temp = "external";
	else temp = "on-board";
	(void)fprintf(opt_fp, " clock_source:%s", temp);

	switch (((int)tests[i]->data & 12) >> 2)
	{
	  case 1: temp = "V.35"; break;
	  case 2: temp = "RS449&V.35"; break;
	  case 3: temp = "RS449-V.35"; break;
	  default: temp = "RS449"; break;
	}
	(void)fprintf(opt_fp, " port_type:%s", temp);

	if (tests[i]->unit == 0)
	  switch ((int)tests[i]->data & 3)
	  {
	    case 0: temp = "0"; break;
	    case 1: temp = "1"; break;
	    case 2: temp = "01"; break;
	    default: temp = "0-1";
	  }
	else
	  switch ((int)tests[i]->data & 3)
	  {
	    case 0: temp = "2"; break;
	    case 1: temp = "3"; break;
	    case 2: temp = "23"; break;
	    default: temp = "2-3";
	  }
	(void)fprintf(opt_fp, " loopback:%s\n", temp);

	break;

      case GP:
	(void)fprintf(opt_fp," graphics_buffer:%s\n",
				(int)tests[i]->data&1?enable:disable);
	break;

      case PRESTO:
        (void)fprintf(opt_fp," presto_perf_ratio:%s\n",
                                (int)tests[i]->data&1?enable:disable);
        break;

      case CG12:
	(void)fprintf(opt_fp,
	  " dsp:%s sram&dram:%s video_memories:%s lookup_tables:%s\
 vectors_generation:%s polygons_generation:%s transformations:%s\
 clipping&hidden:%s depth_cueing:%s lighting&shading:%s arbitration:%s\
 loops_per_function:%d loops_per_board:%d\n",
		(int)tests[i]->data&0x1?disable:enable,
		(int)tests[i]->data&0x2?disable:enable,
		(int)tests[i]->data&0x4?disable:enable,
		(int)tests[i]->data&0x8?disable:enable,
		(int)tests[i]->data&0x10?disable:enable,
		(int)tests[i]->data&0x20?disable:enable,
		(int)tests[i]->data&0x40?disable:enable,
		(int)tests[i]->data&0x80?disable:enable,
		(int)tests[i]->data&0x100?disable:enable,
		(int)tests[i]->data&0x200?disable:enable,
		(int)tests[i]->data&0x400?disable:enable,
		(int)tests[i]->data>>12,
		(int)tests[i]->special);
        break;

      case GT:
        (void)fprintf(opt_fp,
          " video_memory:%s cluts&wlut:%s fe_local_data_memory:%s\
 su_shared_ram:%s rendering_pipeline:%s acc_video_memory:%s\
 fp_output_section:%s vectors:%s triangles:%s spline_curves:%s\
 viewport_clipping:%s hidden_surface_removal:%s polygon_edges:%s\
 transparency:%s depth_cueing:%s lighting&shading:%s text:%s\
 picking:%s arbitration:%s stereo:%s lightpen:%s\
 loops_per_function:%d loops_per_board:%d\n",
                (int)tests[i]->data&0x1?    enable:disable,
                (int)tests[i]->data&0x2?    enable:disable,
                (int)tests[i]->data&0x4?    enable:disable,
                (int)tests[i]->data&0x8?    enable:disable,
                (int)tests[i]->data&0x10?   enable:disable,
                (int)tests[i]->data&0x20?   enable:disable,
                (int)tests[i]->data&0x40?   enable:disable,
                (int)tests[i]->data&0x80?   enable:disable,
                (int)tests[i]->data&0x100?  enable:disable,
                (int)tests[i]->data&0x200?  enable:disable,
                (int)tests[i]->data&0x400?  enable:disable,
                (int)tests[i]->data&0x800?  enable:disable,
                (int)tests[i]->data&0x1000? enable:disable,
                (int)tests[i]->data&0x2000? enable:disable,
                (int)tests[i]->data&0x4000? enable:disable,
                (int)tests[i]->data&0x8000? enable:disable,
                (int)tests[i]->data&0x10000?enable:disable,
                (int)tests[i]->data&0x20000?enable:disable,
                (int)tests[i]->data&0x40000?enable:disable,
                (int)tests[i]->data&0x80000?enable:disable,
                (int)tests[i]->data&0x100000?enable:disable,
                (int)tests[i]->data>>21,
                (int)tests[i]->special);
        break;

      case MP4:
        (void)fprintf(opt_fp, " lock_unlock:%s data_io:%s FPU_check:%s cache_consistency:%s",
                (int)tests[i]->data&0x1?disable:enable,
                (int)tests[i]->data&0x2?disable:enable,
                (int)tests[i]->data&0x4?disable:enable,
                (int)tests[i]->data&0x8?disable:enable);
	for (b = 1, j = 0, num = 0; num < number_processors; b <<= 1, j++)
	{
	    if (processors_mask & 1)
	    {
		fprintf(opt_fp, " processor_%d:%s", j, 
                              (int)tests[i]->data&(0x10<<j)?disable:enable);
		num++;
	    }
	}
	fprintf(opt_fp, "\n");
        break;

      case ZEBRA1:
        switch (((int)tests[i]->data & 0xC) >> 2) 
        { 
            case 0: temp = "fast"; break; 
            case 1: temp = "medium"; break; 
            default: temp = "extended"; 
        }
	(void)fprintf(opt_fp, " access:%s mode:%s\n",
                (int)tests[i]->data&0x1?"readonly":"writeonly",
		 temp);
 
	break;

      case ZEBRA2:
        (void)fprintf(opt_fp," access:%s", 
                (int)tests[i]->data&0x1?"readonly":"writeonly"); 
  
        switch (((int)tests[i]->data & 0xC) >> 2) 
        { 
            case 0: temp = "fast"; break; 
            case 1: temp = "medium"; break; 
            default: temp = "extended"; 
        } 
	(void)fprintf(opt_fp, " mode:%s", temp);

        switch (((int)tests[i]->data & 0x30) >> 4)  
        { 
            case 1: temp = "57fonts"; break;  
	    case 2: temp = "user_defined"; break;
            default: temp = "default";  
        } 
        (void)fprintf(opt_fp, " image:%s", temp);

        (void)fprintf(opt_fp," resolution:%s\n",  
                (int)tests[i]->data&0xC0?"300":"400");

        break;

      case SPIF:
	(void)fprintf(opt_fp," internal:%s",
				(int)tests[i]->data&0x1?enable:disable);
	(void)fprintf(opt_fp," print:%s",
				(int)tests[i]->data&0x2?enable:disable);
	(void)fprintf(opt_fp," sp_96:%s",
				(int)tests[i]->data&0x4?enable:disable);
	(void)fprintf(opt_fp," db_25:%s",
				(int)tests[i]->data&0x8?enable:disable);
	(void)fprintf(opt_fp," echo_tty:%s",
				(int)tests[i]->data&0x10?enable:disable);

	switch (((int)tests[i]->data & 0xf00) >> 8)
	{
	    case 0: temp = "110"; break;
	    case 1: temp = "300"; break;
	    case 2: temp = "600"; break;
	    case 3: temp = "1200"; break;
	    case 4: temp = "2400"; break;
	    case 5: temp = "4800"; break;
	    case 6: temp = "9600"; break;
	    case 7: temp = "19200"; break;
	    case 8: temp = "38400"; break;
	    default: temp = "9600";
	}
	(void)fprintf(opt_fp," baud_rate:%s", temp);

	switch (((int)tests[i]->data & 0xf000) >> 12)
	{
	    case 0: temp = "5"; break;
	    case 1: temp = "6"; break;
	    case 2: temp = "7"; break;
	    case 3: temp = "8"; break;
	    default: temp = "8";
	}
	(void)fprintf(opt_fp," char_size:%s", temp);

	(void)fprintf(opt_fp," stop_bit:%s",
				(int)tests[i]->data&32?"2":"1");

	switch (((int)tests[i]->data & 0xf0000) >> 16)
	{
	    case 0: temp = "none"; break;
	    case 1: temp = "odd"; break;
	    case 2: temp = "even"; break;
	    default: temp = "none";
	}
	(void)fprintf(opt_fp," parity:%s", temp);

	switch (((int)tests[i]->data & 0xf00000) >> 20)
	{
	    case 0: temp = "xonoff"; break;
	    case 1: temp = "rtscts"; break;
	    case 2: temp = "both"; break;
	    default: temp = "rtscts";
	}
	(void)fprintf(opt_fp," flow_control:%s", temp);

	switch (((int)tests[i]->data & 0xf000000) >> 24)
	{
	    case 0: (void)sprintf(temp, "%d", (int)tests[i]->unit); break;
	    case 1: (void)sprintf(temp, "%d", (int)tests[i]->unit + 1); break;
	    case 2: (void)sprintf(temp, "%d", (int)tests[i]->unit + 2); break;
	    case 3: (void)sprintf(temp, "%d", (int)tests[i]->unit + 3); break;
	    case 4: (void)sprintf(temp, "%d", (int)tests[i]->unit + 4); break;
	    case 5: (void)sprintf(temp, "%d", (int)tests[i]->unit + 5); break;
	    case 6: (void)sprintf(temp, "%d", (int)tests[i]->unit + 6); break;
	    case 7: (void)sprintf(temp, "%d", (int)tests[i]->unit + 7); break;
	    case 8: (void)sprintf(temp, "all"); break;
	    default: temp = "0";
	}
  	if (strcmp(temp, "all") == 0) 
	    (void)fprintf(opt_fp," serial_port:all");
	else
	    (void)fprintf(opt_fp," serial_port:/dev/ttyz%s", temp);

	switch (((int)tests[i]->data & 0xf0000000) >> 28)
	{
	    case 0: temp = "0x55"; break;
	    case 1: temp = "0xaa"; break;
	    case 2: temp = "random"; break;
	    default: temp = "0x55";
	}
	(void)fprintf(opt_fp," data_type:%s", temp);

	switch (((int)tests[i]->data & 64) >> 6)
	{
	    case 0: (void)fprintf(opt_fp, " parallel_port:disable \n");  break;
	    case 1: (void)fprintf(opt_fp, " parallel_port:enable: \n");
	    	break;
	}

	break;

      default:
	(void)fprintf(opt_fp, "\n");
    }
  }    
}

/******************************************************************************
 * load_test_options(), load the individual test options from the specified   *
 *	file.								      *
 * Input: param, parsed option tokens.					      *
 * Output: none.							      *
 ******************************************************************************/
load_test_options(param)
char	*param[];
{
  int	test_id, temp, i, processor_num;
  int	onoff=1;
  int	index=4;  /* to keep track of where the options start(default to 4) */
  char	*temp1[10];

  /* go through all existing devices */
  for (test_id=0; test_id != exist_tests; ++test_id)
    if (strcmp(param[0], tests[test_id]->devname) == 0 &&
	strcmp(param[1], tests[test_id]->testname) == 0) break;

  if (test_id == exist_tests)	/* unknown test, format error(ignored) */
    return;

  if (strcmp(param[2], "off") == 0) tests[test_id]->enable = 0;
  else if (strcmp(param[2], "on") == 0) tests[test_id]->enable = 1;
  else
  {
    onoff = 0;		/* device/test enable/disable were not specified */
    index = 2;		/* options start from 3rd argument */
  }

  if (onoff && (param[3] != NULL))	/* device dis/enable was specified */
  {
    if (strcmp(param[3], "off") == 0) tests[test_id]->dev_enable = 0;
    else if (strcmp(param[3], "on") == 0) tests[test_id]->dev_enable = 1;
    else index = 3;	/* options start from 4th argument */
  }

  switch (tests[test_id]->id)		/* depend on which test it is */
  {
      case VMEM:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (strcmp(param[index], "wait_time") == 0)
	  {
	    if (param[++index] != NULL)
		for (i=0; i<=7; i++) {
		   if(!strcmp(param[index], vmem_waittime[i])) {
			(int)tests[test_id]->data = i;
			break;
		   }
		}
	    else break;
	  }
	  else if (strcmp(param[index], "reserve") == 0)
	  {
	    if (param[++index] != NULL)
		(int)tests[test_id]->special = atoi(param[index]);
	    else break;
	  }

	  ++index;
	}
	break;

      case AUDIO:
        if (tests[test_id]->conf->uval.devinfo.status == 0) {
                while (param[index] != NULL)    /* still more to be checked */
                {
		  if ( param[index+1] == NULL ) break;
                  if (strcmp(param[index], "audio_output") == 0)
                  {
                    if (param[++index] != NULL)
                    {
                      if (strcmp(param[index], "speaker") == 0)
                        tests[test_id]->data = (caddr_t)0;
                      else if (strcmp(param[index], "jack") == 0)
                        tests[test_id]->data = (caddr_t)1;
                    }   
                    else break;
                  } 
                  else if (strcmp(param[index], "volume") == 0)
                  {
                    if (param[++index] != NULL)
                        tests[test_id]->special = (caddr_t)(atoi(param[index]));
                    else break;
                  } 

                  ++index;
                } 
        }
        else {
                while (param[index] != NULL)    /* still more to be checked */
                {
		  if ( param[index+1] == NULL ) break;
                  if (strcmp(param[index], "audbri_loopback") == 0)
                  {
                        ++index;
                        if (strcmp(param[index], disable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~4);
                        else if (strcmp(param[index], enable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 4);
                  }
                  else if (strcmp(param[index], "audbri_calib") == 0)
                  {
                        ++index;
                        if (strcmp(param[index], disable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~8);
                        else if (strcmp(param[index], enable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 8);
                  }
                  else if (strcmp(param[index], "audbri_crystal") == 0)
                  {
                        ++index;
                        if (strcmp(param[index], disable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x10);
                        else if (strcmp(param[index], enable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 0x10);
                  }
                  else if (strcmp(param[index], "audbri_controls") == 0)
                  {
                        ++index;
                        if (strcmp(param[index], disable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x20);
                        else if (strcmp(param[index], enable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 0x20);
                  }
                  else if (strcmp(param[index], "audbri_audio") == 0)
                  {
                        ++index;
                        if (strcmp(param[index], disable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x40);
                        else if (strcmp(param[index], enable) == 0)
                          tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 0x40);
                  }
                  else if (strcmp(param[index], "audbri_type") == 0)
                  {
                        ++index;
 
                        temp = atoi(param[index]);
                        tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x01);
                        tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | temp);
                  }
                  else if (strcmp(param[index], "audbri_ref_file") == 0)
                  {
                        ++index;
			if (param[index] == NULL ) break;
                        strcpy(audbri_ref_file, param[index]);
                        tests[test_id]->special = (char *)audbri_ref_file;
                  }
                  ++index;
                }/* end of while */
        }         
        break;
 
      case ENET0:
      case ENET1:
      case ENET2:
      case OMNI_NET:
      case FDDI:
      case TRNET:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "spray") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 1;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~1;
	  }
	  else if (strcmp(param[index], "delay") == 0)
	  {
		++index;
		(int)tests[test_id]->special = atoi(param[index]);
	  }
	  else if (strcmp(param[index], "warning") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 2;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~2;
	  }

	  ++index;
	}
	break;

      case CPU_SP:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (strcmp(param[index], "loopback") == 0)
	    if (param[++index] != NULL)
	    {
		if (strcmp(param[index], "a") == 0) temp = 0;
		else if (strcmp(param[index], "b") == 0) temp = 1;
		else if (strcmp(param[index], "ab") == 0) temp = 2;
		else if (strcmp(param[index], "a-b") == 0) temp = 3;
		else
		  temp = (int)tests[test_id]->data;

		(int)tests[test_id]->data = temp;
	    }
	    else break;

	  ++index;
	}
	break;

      case CPU_SP1:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (strcmp(param[index], "loopback") == 0)
	    if (param[++index] != NULL)
	    {
		if (strcmp(param[index], "c") == 0) temp = 0;
		else if (strcmp(param[index], "d") == 0) temp = 1;
		else if (strcmp(param[index], "cd") == 0) temp = 2;
		else if (strcmp(param[index], "c-d") == 0) temp = 3;
		else
		  temp = (int)tests[test_id]->data;

		(int)tests[test_id]->data = temp;
	    }
	    else break;

	  ++index;
	}
	break;

      case CDROM:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "cdtype") == 0)
	  {
		++index;
		if (strcmp(param[index], "cdassist") == 0) temp = 0;
		else if (strcmp(param[index], "sony2") == 0) temp = 1;
		else if (strcmp(param[index], "hitachi4") == 0) temp = 2;
		else if (strcmp(param[index], "pdo") == 0) temp = 3;
		else if (strcmp(param[index], "other") == 0) temp = 4;
		else
		  temp = (int)tests[test_id]->data;

		(int)tests[test_id]->data = ((int)tests[test_id]->data&~7)|temp;
	  }
	  else if (strcmp(param[index], "data1") == 0)
	  {
		++index;
		if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data |= 8;
		else if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data &= ~8;
	  }
	  else if (strcmp(param[index], "data2") == 0)
	  {
		++index;
		if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data |= 0x40;
		else if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data &= ~0x40;
	  }
	  else if (strcmp(param[index], "audiotest") == 0)
	  {
		++index;
		if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data |= 0x10;
		else if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data &= ~0x10;
	  }
	  else if (strcmp(param[index], "volume") == 0)
	  {
		++index;
		(int)tests[test_id]->special = atoi(param[index]);
	  }
	  else if (strcmp(param[index], "datatrack") == 0)
	  {
		++index;
		(int)tests[test_id]->data = ((int)tests[test_id]->data&~0xFF00)
					| atoi(param[index]) << 8;
	  }
	  else if (strcmp(param[index], "readmode") == 0)
	  {
		++index;
		if (strcmp(param[index], "sequential") == 0)
		  (int)tests[test_id]->data |= 0x20;
		else if (strcmp(param[index], "random") == 0)
		  (int)tests[test_id]->data &= ~0x20;
	  }

	  ++index;
	}
	break;

      case SCSIDISK1:
      case XYDISK1:
      case XDDISK1:
      case IPIDISK1:
      case IDDISK1:
      case SFDISK1:
      case OBFDISK1:

	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (strcmp(param[index], "rawtest_mode") == 0)
	  {
	    if (param[++index] != NULL)
	    {
	      if (strcmp(param[index], "readonly") == 0) {
		(int)tests[test_id]->data &= ~0x8; /* reset the mode bit */
	      }
	      else if (strcmp(param[index], "write_read") == 0) {
		(int)tests[test_id]->data |= 0x8;  /* set the mode bit */
	      }
	    }
	    else break;
	  }
          else if (strcmp(param[index], "rawtest_part") == 0)
          {
            ++index;
            if (strcmp(param[index], "a") == 0) temp = 0;
            else if (strcmp(param[index], "b") == 0) temp = 1;
            else if (strcmp(param[index], "c") == 0) temp = 2;
            else if (strcmp(param[index], "d") == 0) temp = 3;
            else if (strcmp(param[index], "e") == 0) temp = 4;
            else if (strcmp(param[index], "f") == 0) temp = 5;
            else if (strcmp(param[index], "g") == 0) temp = 6;
            else if (strcmp(param[index], "h") == 0) temp = 7;

            (int)tests[test_id]->data &= ~0x7;
            (int)tests[test_id]->data |= temp;
          }
          else if (strcmp(param[index], "rawtest_size") == 0)
          {
            ++index;
            if (strcmp(param[index], "100") == 0) temp = 0;
            else if (strcmp(param[index], "200") == 0) temp = 1;
            else if (strcmp(param[index], "400") == 0) temp = 2;
            else if (strcmp(param[index], "600") == 0) temp = 3;
            else if (strcmp(param[index], "800") == 0) temp = 4;
            else if (strcmp(param[index], "1000") == 0) temp = 5;
            else if (strcmp(param[index], "1200") == 0) temp = 6;

            (int)tests[test_id]->data &= ~0x70;
            (int)tests[test_id]->data |= temp << 4;
          }
	  ++index;
	}
	break;

      case MAGTAPE1:
      case MAGTAPE2:
      case SCSITAPE:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "format") == 0)
	  {
		++index;
		(int)tests[test_id]->data &= ~3;
		if (strcmp(param[index], "QIC-24") == 0)
		  (int)tests[test_id]->data |= 1;
		else if (strcmp(param[index], "QIC-11&QIC-24") == 0)
		  (int)tests[test_id]->data |= 2;
	  }
	  else if (strcmp(param[index], "density") == 0)
	  {
		++index;
		(int)tests[test_id]->data &= ~7;
		if (strcmp(param[index], "1600-BPI") == 0)
		  (int)tests[test_id]->data |= 1;
		else if (strcmp(param[index], "6250-BPI") == 0)
		  (int)tests[test_id]->data |= 2;
#ifdef NEW
		else if (strcmp(param[index], "EXB-8500") == 0) {
		  if (tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISEXB8500)
		  	(int)tests[test_id]->data |= 1;
		}
#endif NEW
		else if (strcmp(param[index], "compression") == 0)
		{
		  if ((tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISHP || 
		    tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISKENNEDY)
		    && tests[test_id]->conf->uval.tapeinfo.status == FLT_COMP)
		      (int)tests[test_id]->data |= 4;
#ifdef NEW
		  else if ((tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISEXB8500)
		    && tests[test_id]->conf->uval.tapeinfo.status == FLT_COMP)
		      (int)tests[test_id]->data |= 3;
#endif NEW
		}
		else if (strcmp(param[index], "all") == 0)
		{
		  if (tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISHP || 
		    tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISKENNEDY)
		      (int)tests[test_id]->data |= 3;
#ifdef NEW
		  else if (tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISEXB8500)
		      (int)tests[test_id]->data |= 2;
#endif NEW
		}
#ifdef NEW
		else if (strcmp(param[index], "both") == 0) {
		  if ((tests[test_id]->conf->uval.tapeinfo.t_type == MT_ISEXB8500)
		    && tests[test_id]->conf->uval.tapeinfo.status != FLT_COMP)
			(int)tests[test_id]->data |= 2;
		}
#endif NEW
	  }
	  else if (strcmp(param[index], "length") == 0)
	  {
		++index;
		(int)tests[test_id]->data &= ~0x18;
		if (strcmp(param[index], "specified") == 0)
		  (int)tests[test_id]->data |= 1 << 3;
		else if (strcmp(param[index], "long") == 0)
		  (int)tests[test_id]->data |= 2 << 3;
		else if (strcmp(param[index], "short") == 0)
		  (int)tests[test_id]->data |= 3 << 3;
	  }
	  else if (strcmp(param[index], "mode") == 0)
	  {
		++index;
		if (strcmp(param[index], "readonly") == 0)
		  (int)tests[test_id]->data |= 0x100;
		else if (strcmp(param[index], "write_read") == 0)
		  (int)tests[test_id]->data &= ~0x100;
	  }
	  else if (strcmp(param[index], "file_test") == 0)
	  {
		++index;
		if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data |= 0x20;
		else if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data &= ~0x20;
	  }
	  else if (strcmp(param[index], "streaming") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data |= 0x40;
		else if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data &= ~0x40;
	  }
	  else if (strcmp(param[index], "recon") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data |= 0x80;
		else if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data &= ~0x80;
	  }
	  else if (strcmp(param[index], "retension") == 0)
	  {
		++index;
		if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data |= 0x200;
		else if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data &= ~0x200;
	  }
	  else if (strcmp(param[index], "clean_head") == 0)
	  {
		++index;
		if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data |= 0x400;
		else if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data &= ~0x400;
	  }
	  else if (strcmp(param[index], "block") == 0)
	  {
		++index;
		(unsigned)tests[test_id]->special= (unsigned)atoi(param[index]);
	  }
	  else if (strcmp(param[index], "pass") == 0)
	  {
		++index;
		(unsigned)tests[test_id]->data =
		((unsigned)tests[test_id]->data&0xffff) |
		(unsigned)atoi(param[index])<<16;
	  }

	  ++index;
	}
	break;

      case TV1:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "ntsc") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 1;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~1;
	  }
	  else if (strcmp(param[index], "yc") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 2;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~2;
	  }
	  else if (strcmp(param[index], "yuv") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 4;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~4;
	  }
	  else if (strcmp(param[index], "rgb") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 8;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~8;
	  }

	  ++index;
	}
	break;

      case IPC:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "floppy") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 1;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~1;
	  }
	  else if (strcmp(param[index], "printer") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data |= 2;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data &= ~2;
	  }

	  ++index;
	}
	break;

      case MCP:
      case MTI:
      case SCP:
      case SCP2:
      case SCSISP1:
      case SCSISP2:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "from") == 0)
	  {
		++index;
		temp = strlen(param[index]);
		*(param[index]+temp-1) = '\0';		/* get rid of '$' */
		(void)strcpy(((struct loopback *)(tests[test_id]->data))->from,
			param[index]);
	  }
	  else if (strcmp(param[index], "to") == 0)
	  {
		++index;
		temp = strlen(param[index]);
		*(param[index]+temp-1) = '\0';		/* get rid of '$' */
		(void)strcpy(((struct loopback *)(tests[test_id]->data))->to,
			param[index]);
	  }

	  ++index;
	}
	break;

      case HSI:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "clock_source") == 0)
	  {
	    ++index;
	    if (strcmp(param[index], "external") == 0) temp = 1;
	    else temp = 0;

	    (int)tests[test_id]->data &= ~16;
	    (int)tests[test_id]->data |= temp << 4;
	  }
	  else if (strcmp(param[index], "port_type") == 0)
	  {
	    ++index;
	    if (strcmp(param[index], "V.35") == 0) temp = 1;
	    else if (strcmp(param[index], "RS449&V.35") == 0) temp = 2;
	    else if (strcmp(param[index], "RS449-V.35") == 0) temp = 3;
	    else temp = 0;

	    (int)tests[test_id]->data &= ~12;
	    (int)tests[test_id]->data |= temp << 2;
	  }
	  else if (strcmp(param[index], "loopback") == 0)
	  {
	    ++index;
	    if (strcmp(param[index], "1") == 0 ||
			strcmp(param[index], "3") == 0) temp = 1;
	    else if (strcmp(param[index], "01") == 0 ||
			strcmp(param[index], "23") == 0) temp = 2;
	    else if (strcmp(param[index], "0-1") == 0 ||
			strcmp(param[index], "2-3") == 0) temp = 3;
	    else
	      temp = 0;

	    if ((((int)tests[test_id]->data & 12) >> 2) == 3) temp = 3;
	    /* force 0-1/2-3 loopback */

	    (int)tests[test_id]->data &= ~3;
	    (int)tests[test_id]->data |= temp;
	  }

	  ++index;
	}
	break;

      case SBUS_HSI:
        while (param[index] != NULL)    /* still more to be checked */
        {
          if (param[index+1] == NULL) break;
          if (strcmp(param[index], "clock_source") == 0)
          {
            ++index;
            if (strcmp(param[index], "External") == 0) temp = 1;
            else temp = 0;

            (int)tests[test_id]->data &= ~1;
            (int)tests[test_id]->data |= temp;
          }
          else if (strcmp(param[index], "internal_loopback") == 0)
          {
                ++index;
                if (strcmp(param[index], "enable") == 0) temp = 1;
                else temp =0;

                (int)tests[test_id]->data &= ~2;
                (int)tests[test_id]->data |= temp << 1;
          }
          else if (strcmp(param[index], "loopback") == 0)
          {
            ++index;
            if (strcmp(param[index], "1") == 0 ||
                        strcmp(param[index], "5") == 0 ||
                        strcmp(param[index], "9") == 0) temp = 1;
            else if (strcmp(param[index], "2") == 0 ||
                        strcmp(param[index], "6") == 0 ||
                        strcmp(param[index], "10") == 0) temp = 2;
            else if (strcmp(param[index], "3") == 0 ||
                        strcmp(param[index], "7") == 0 ||
                        strcmp(param[index], "11") == 0) temp = 3;
            else if (strcmp(param[index], "0,1,2,3") == 0 ||
                        strcmp(param[index], "4,5,6,7") == 0 ||
                        strcmp(param[index], "8,9,10,11") == 0) temp = 4;
            else if (strcmp(param[index], "0-1,2-3") == 0 ||
                        strcmp(param[index], "4-5,6-7") == 0 ||
                        strcmp(param[index], "8-9,10-11") == 0) temp = 5;
            else if (strcmp(param[index], "0-2,1-3") == 0 ||
                        strcmp(param[index], "4-6,5-7") == 0 ||
                        strcmp(param[index], "8-10,9-11") == 0) temp = 6;
            else if (strcmp(param[index], "0-3,1-2") == 0 ||
                        strcmp(param[index], "4-7,5-6") == 0 ||
                        strcmp(param[index], "8-11,9-10") == 0) temp = 7;
            else
              temp = 0;

            /* use 0x0F0 instead of 0x070 to clear out all bits */
            (int)tests[test_id]->data &= ~0x0F0;
            (int)tests[test_id]->data |= temp << 4;
          }

          ++index;
        }
        break;

      case GP:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;
	  if (strcmp(param[index], "graphics_buffer") == 0)
	  {
		++index;
		if (strcmp(param[index] , enable) == 0)
		  (int)tests[test_id]->data = 1;
		else if (strcmp(param[index] , disable) == 0)
		  (int)tests[test_id]->data = 0;
	  }

	  ++index;
	}
	break;

      case PRESTO:
        while (param[index] != NULL)    /* still more to be checked */
        {  
          if (param[index+1] == NULL) break;
          if (strcmp(param[index], "presto_perf_ratio") == 0)
          {
                ++index;
                if (strcmp(param[index] , enable) == 0)
                  tests[test_id]->data = (caddr_t)1;
                else if (strcmp(param[index] , disable) == 0)
                  tests[test_id]->data = (caddr_t)0;
          }    

          ++index;
        }  
        break;

      case CG12:
        while (param[index] != NULL)    /* still more to be checked */
        {
          if (param[index+1] == NULL) break;
          if (strcmp(param[index], "dsp") == 0)
          {
                ++index;
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x1; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x1; 
	  }
          else if (strcmp(param[index], "sram&dram") == 0)
          {
                ++index;
                if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data |= 0x2;
                else if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data &= ~0x2;
          }
          else if (strcmp(param[index], "video_memories") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x4; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x4; 
          }
          else if (strcmp(param[index], "lookup_tables") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x8; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x8; 
          }
          else if (strcmp(param[index], "vectors_generation") == 0)
          {
                ++index;
                if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data |= 0x10;
                else if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data &= ~0x10;
          }
          else if (strcmp(param[index], "polygons_generation") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x20; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x20; 
          }
          else if (strcmp(param[index], "transformations") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x40; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x40; 
          }
          else if (strcmp(param[index], "clipping&hidden") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x80; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x80; 
          }
          else if (strcmp(param[index], "depth_cueing") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x100; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x100; 
          }
          else if (strcmp(param[index], "lighting&shading") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x200; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x200; 
          }
          else if (strcmp(param[index], "arbitration") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], disable) == 0) 
                  (int)tests[test_id]->data |= 0x400; 
                else if (strcmp(param[index], enable) == 0) 
                  (int)tests[test_id]->data &= ~0x400; 
          }
          else if (strcmp(param[index], "loops_per_function") == 0) 
          { 
                ++index; 
		(int)tests[test_id]->data = 
			((int)tests[test_id]->data&~0xFFFFF000)
					| atoi(param[index]) << 12;
          }
          else if (strcmp(param[index], "loops_per_board") == 0)
          {
                ++index;
                (int)tests[test_id]->special = atoi(param[index]);
          }

	  ++index;
	}/* end of while */
	break;

      case GT:
        while (param[index] != NULL)    /* still more to be checked */
        {
          if (param[index+1] == NULL) break;
          if (strcmp(param[index], "video_memory") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x1;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x1;
          }
          else if (strcmp(param[index], "cluts&wlut") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x2;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x2;
          }
          else if (strcmp(param[index], "fe_local_data_memory") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x4;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x4;
          }
          else if (strcmp(param[index], "su_shared_ram") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x8;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x8;
          }
          else if (strcmp(param[index], "rendering_pipeline") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x10;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x10;
          }
          else if (strcmp(param[index], "acc_video_memory") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x20;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x20;
          }
          else if (strcmp(param[index], "fp_output_section") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x40;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x40;
          }
          else if (strcmp(param[index], "vectors") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x80;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x80;
          }
          else if (strcmp(param[index], "triangles") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x100;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x100;
          }
          else if (strcmp(param[index], "spline_curves") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x200;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x200;
          }
          else if (strcmp(param[index], "viewport_clipping") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x400;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x400;
          }
          else if (strcmp(param[index], "hidden_surface_removal") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x800;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x800;
          }
          else if (strcmp(param[index], "polygon_edges") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x1000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x1000;
          }
          else if (strcmp(param[index], "transparency") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x2000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x2000;
          }
          else if (strcmp(param[index], "depth_cueing") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x4000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x4000;
          }
          else if (strcmp(param[index], "lighting&shading") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x8000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x8000;
          }
          else if (strcmp(param[index], "text") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x10000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x10000;
          }
          else if (strcmp(param[index], "picking") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x20000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x20000;
          }
          else if (strcmp(param[index], "arbitration") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x40000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x40000;
          }
          else if (strcmp(param[index], "stereo") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x80000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x80000;
          }
          else if (strcmp(param[index], "lightpen") == 0)
          {
                ++index;
                if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data |= 0x100000;
                else if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data &= ~0x100000;
          }
          else if (strcmp(param[index], "loops_per_function") == 0)
          {
                ++index;
                (int)tests[test_id]->data =
                        ((int)tests[test_id]->data&= ~0xFFE00000)
                                        | atoi(param[index]) << 21;
          }
          else if (strcmp(param[index], "loops_per_board") == 0)
          {
                ++index;
                (int)tests[test_id]->special = atoi(param[index]);
          }

          ++index;
        }/* end of while */
        break;

      case MP4:
        while (param[index] != NULL)    /* still more to be checked */
        {
          if (param[index+1] == NULL) break;
          if (strcmp(param[index], "lock_unlock") == 0)
          {
                ++index;
                if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data |= 0x1;
                else if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data &= ~0x1;
          }
          else if (strcmp(param[index], "data_io") == 0)
          {
                ++index;
                if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data |= 0x2;
                else if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data &= ~0x2;
          }
          else if (strcmp(param[index], "FPU_check") == 0)
          {
                ++index;
                if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data |= 0x4;
                else if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data &= ~0x4;
          }
          else if (strcmp(param[index], "cache_consistency") == 0)
          {
                ++index;
                if (strcmp(param[index], disable) == 0)
                  (int)tests[test_id]->data |= 0x8;
                else if (strcmp(param[index], enable) == 0)
                  (int)tests[test_id]->data &= ~0x8;
          }
	  else if (strncmp(param[index], "processor_", 10) == 0)
	  {
	        processor_num = parse_str(param[index]);
		++index;
		if (strcmp(param[index], disable) == 0)
		  (int)tests[test_id]->data |= (0x10 << processor_num);
		else if (strcmp(param[index], enable) == 0)
		  (int)tests[test_id]->data &= ~(0x10 << processor_num);
	  }
            ++index;
        } /* end of while */
        break;

      case ZEBRA1:
        while (param[index] != NULL)    /* still more to be checked */
        {
          if (param[index+1] == NULL) break;


          if (strcmp(param[index], "mode") == 0)
          {
                ++index;
                (int)tests[test_id]->data &= ~0xC;
                if (strcmp(param[index], "medium") == 0)
                  (int)tests[test_id]->data |= 1 << 2;
                else if (strcmp(param[index], "extended") == 0)
                  (int)tests[test_id]->data |= 2 << 2;
                else if (strcmp(param[index], "fast") == 0)
                  (int)tests[test_id]->data |= 0 << 2;  /* do nothing */
          }
          else if (strcmp(param[index], "access") == 0)
          {
                ++index;
                if (strcmp(param[index], "readonly") == 0)
                  (int)tests[test_id]->data |= 0x1;
                else if (strcmp(param[index], "writeonly") == 0)
                  (int)tests[test_id]->data &= ~0x3;
          }
          ++index;
        }/* end of while */
        break;

     case ZEBRA2:
        while (param[index] != NULL)    /* still more to be checked */
        {
          if (param[index+1] == NULL) break;
 
          if (strcmp(param[index], "mode") == 0)
          {
                ++index;
                (int)tests[test_id]->data &= ~0xC; 
                if (strcmp(param[index], "medium") == 0) 
                  (int)tests[test_id]->data |= 1 << 2; 
                else if (strcmp(param[index], "extended") == 0) 
                  (int)tests[test_id]->data |= 2 << 2; 
                else if (strcmp(param[index], "fast") == 0) 
                  (int)tests[test_id]->data |= 0 << 2;  /* do nothing */ 
          } 
          else if (strcmp(param[index], "access") == 0) 
          { 
                ++index; 
                if (strcmp(param[index], "readonly") == 0) 
                  (int)tests[test_id]->data |= 0x1;
                else if (strcmp(param[index], "writeonly") == 0)
                  (int)tests[test_id]->data &= ~0x3;
          }

          else if (strcmp(param[index], "image") == 0)
          {
                ++index;
                (int)tests[test_id]->data &= ~0x30;
                if (strcmp(param[index], "57fonts") == 0)
                  (int)tests[test_id]->data |= 1 << 4;
                else if (strcmp(param[index], "user_defined") == 0) 
                  (int)tests[test_id]->data |= 2 << 4; 
                else if (strcmp(param[index], "default") == 0)
                  (int)tests[test_id]->data |= 0 << 4;  /* do nothing */
          }
          else if (strcmp(param[index], "resolution") == 0)
          {
                ++index;
                if (strcmp(param[index], "300") == 0)
                  (int)tests[test_id]->data |= 0x1 << 6;
                else if (strcmp(param[index], "writeonly") == 0)
                  (int)tests[test_id]->data &= ~0xC0;
          }

          ++index; 
        }/* end of while */ 
        break;

      case SPIF:
	while (param[index] != NULL)	/* still more to be checked */
	{
	  if (param[index+1] == NULL) break;

	  if (strcmp(param[index], "internal") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 1);
		else if (strcmp(param[index], disable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~1);
	  }
	  else if (strcmp(param[index], "print") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 2);
		else if (strcmp(param[index], disable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~2);
	  }
	  else if (strcmp(param[index], "sp_96") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 0x04);
		else if (strcmp(param[index], disable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x04);
	  }
	  else if (strcmp(param[index], "db_25") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 0x08);
		else if (strcmp(param[index], disable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x08);
	  }
	  else if (strcmp(param[index], "echo_tty") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0)
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 0x10);
		else if (strcmp(param[index], disable) == 0)
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x10);
	  }
	  else if (strcmp(param[index], "baud_rate") == 0)
	  {
		++index;
		if (strcmp(param[index], "110") == 0) temp = 0;
		else if (strcmp(param[index], "300") == 0) temp = 1;
		else if (strcmp(param[index], "600") == 0) temp = 2;
		else if (strcmp(param[index], "1200") == 0) temp = 3;
		else if (strcmp(param[index], "2400") == 0) temp = 4;
		else if (strcmp(param[index], "4800") == 0) temp = 5;
		else if (strcmp(param[index], "9600") == 0) temp = 6;
		else if (strcmp(param[index], "19200") == 0) temp = 7;
		else if (strcmp(param[index], "38400") == 0) temp = 8;
		else
		  temp = 6;

		tests[test_id]->data = (caddr_t)(((int)tests[test_id]->data&~0xf00)|temp << 8);
	  }

	  else if (strcmp(param[index], "char_size") == 0)
	  {
		++index;
		if (strcmp(param[index], "5") == 0) temp = 0;
		else if (strcmp(param[index], "6") == 0) temp = 1;
		else if (strcmp(param[index], "7") == 0) temp = 2;
		else if (strcmp(param[index], "8") == 0) temp = 3;
		else
		  temp = 6;

		tests[test_id]->data = (caddr_t)(((int)tests[test_id]->data&~0xf000)|temp << 12);

	  }
	  else if (strcmp(param[index], "stop_bit") == 0)
	  {
		++index;
		if (strcmp(param[index], "2") == 0)
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 0x20);
		else if (strcmp(param[index], "1") == 0)
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~0x20);
	  }
	  else if (strcmp(param[index], "parity") == 0)
	  {
		++index;
		if (strcmp(param[index], "none") == 0) temp = 0;
		else if (strcmp(param[index], "odd") == 0) temp = 1;
		else if (strcmp(param[index], "even") == 0) temp = 2;
		else
		  temp = 0;

		tests[test_id]->data = (caddr_t)(((int)tests[test_id]->data&~0xf0000)|temp << 16);
	  }
	  else if (strcmp(param[index], "flow_control") == 0)
	  {
		++index;
		if (strcmp(param[index], "xonoff") == 0) temp = 0;
		else if (strcmp(param[index], "rtscts") == 0) temp = 1;
		else if (strcmp(param[index], "both") == 0) temp = 2;
		else
		  temp = 1;

		tests[test_id]->data = (caddr_t)(((int)tests[test_id]->data&~0xf00000)|temp << 20);
	  }
	  else if (strcmp(param[index], "serial_port") == 0)
	  {
		++index;
		if (strcmp(param[index], "all") == 0) {
		    temp = 8;
		}
		else {
		    for (i=0; i < 7 ; i++) {
		        sprintf(temp1, "/dev/ttyz%x", (int)tests[test_id]->unit + i);
		        if (strcmp(param[index], temp1) == 0) {
		            temp = i;
			    break;
		        }
		    }
		}

		tests[test_id]->data = (caddr_t)(((int)tests[test_id]->data&~0xf000000)|temp << 24);
	  }
	  else if (strcmp(param[index], "data_type") == 0)
	  {
		++index;
		if (strcmp(param[index], "0x55") == 0) temp = 0;
		else if (strcmp(param[index], "0xaa") == 0) temp = 1;
		else if (strcmp(param[index], "random") == 0) temp = 2;
		else
		  temp = 0;

		tests[test_id]->data = (caddr_t)(((int)tests[test_id]->data&~0xf0000000)|temp << 28);
	  }
	  else if (strcmp(param[index], "parallel_port") == 0)
	  {
		++index;
		if (strcmp(param[index], enable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data | 64);
		else if (strcmp(param[index], disable) == 0) 
		  tests[test_id]->data = (caddr_t)((int)tests[test_id]->data & ~64);
	  }
	  ++index;
	}/* end of while */
	break;

    } /* end of main switch */

}

int
parse_str(string)
char *string;
{
    char option_name[30];
    char *ptr;
    int number;

    strcpy(option_name, string);
    ptr = (char *)rindex(option_name, '_');
    ptr++;
    number = atoi(ptr);
    return(number);
 
}

/******************************************************************************
 * optfile_update, updates the information on the screen, after loading the   *
 *		option file.						      *
 ******************************************************************************/
void	optfile_update()
{
  int	group_id, test_id;

  /* clear all the test check marks first */
  group_id = -1;		/* initialize the id(to flag the first group) */
  for (test_id=0; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->which_test > 1) continue;  /* no check mark for them */

    if (group_id != tests[test_id]->group)  /* clear the group too */
    {
	group_id = tests[test_id]->group;	/* save it for next */
	groups[group_id].enable = FALSE;
	if (!tty_mode)
	  (void)panel_set(groups[group_id].select,
				PANEL_TOGGLE_VALUE, 0, FALSE, 0);
    }

    if (!tty_mode)
      (void)panel_set(tests[test_id]->select, PANEL_TOGGLE_VALUE, 0, FALSE, 0);
    else
      display_enable(test_id, FALSE);
  }

  /* mark all the enabled tests with check marks */
  for (test_id=0; test_id != exist_tests; ++test_id)
  {
    if (tests[test_id]->dev_enable &&	/* this is an enabled test */
	(tests[test_id]->enable || tests[test_id]->test_no > 1) &&
	tests[test_id]->type != 2)
    {
      group_id = tests[test_id]->group;
      if (!groups[group_id].enable)
      {
	groups[group_id].enable = TRUE;
	if (!tty_mode)
	  (void)panel_set(groups[group_id].select,
				PANEL_TOGGLE_VALUE, 0, TRUE, 0);
      }

      if (tests[test_id]->which_test == 1)
      {
	if (!tty_mode)
	  (void)panel_set(tests[test_id]->select,
				PANEL_TOGGLE_VALUE, 0, TRUE, 0);
	else
	  display_enable(test_id, TRUE);
      }
    }
  }

  print_status();			/* update the status window too */
}

/******************************************************************************
 * store_option(), stores entire options to the specified option file.	      *
 * Input: opt_file, option file name to be stored into.			      *
 * output: True, if successful; False, otherwise.			      *
 ******************************************************************************/
#define	OPT_VERSION	"#!@#2"
int	store_option(opt_file)
char	*opt_file;
{
  char	temp[82];
  FILE	*opt_fp;

  if (access(OPTION_DIR, F_OK) != 0)
    if (mkdir(OPTION_DIR, 0755) != 0)
    {
	(void)sprintf(temp, "mkdir %s", OPTION_DIR);
	perror(temp);
	return(FALSE);
    }

  (void)sprintf(temp, "%s/%s", OPTION_DIR, opt_file);

  if ((opt_fp=fopen(temp, "w")) == NULL) return(FALSE);

  (void)fprintf(opt_fp, "%s - Sundiag Option File Version 2\n", OPT_VERSION);
  store_system_options(opt_fp);	/* store system-wide options first */
  store_test_options(opt_fp);	/* followed by individual test options */

  (void)fclose(opt_fp);
  return(TRUE);
}

/******************************************************************************
 * load_option(), load entire options from the specified option file.	      *
 * Input: opt_file, option file name to be load from.			      *
 *	  update, != 0, if need to update the screen after loading.  	      *
 * output: TRUE, if successful; FALSE, otherwise.			      *
 ******************************************************************************/
#define	delimit	" :\t\n"	/* delimitter in the option file */
#define TOKEN_NUM  100		/* number of token (label:value) at a time */
#define RDBUF_SIZE 512          /* buffer size for read */

int	load_option(opt_file, update)
char	*opt_file;
int	update;
{
  char	buff[RDBUF_SIZE+2], *temp;
  FILE	*opt_fp;
  char	*param[TOKEN_NUM+1];	/* max. to 100 tokens(plus one to store NULL) */
  int	i, flag=0;

  (void)sprintf(buff, "%s/%s", OPTION_DIR, opt_file);

  if ((opt_fp=fopen(buff, "r")) == NULL)
  {
    if (update == 1)			/* needs to prompt */
      (void)confirm(
        "Can't load! The specified option file does not exist.", TRUE);

    return(FALSE);		/* the option file does not exist */
  }

  if (fgets(buff, RDBUF_SIZE, opt_fp) == NULL ||
      strncmp(buff, OPT_VERSION, strlen(OPT_VERSION)) != 0)
  {
    if (update == 1)			/* needs to prompt */
      (void)confirm(
      "Can't load! The format of the specified option file is incorrect.",TRUE);

    (void)fclose(opt_fp);
    return(FALSE);
  }
  sprintf(buff, loaded_option_file, opt_file);
  logmsg(buff, 0, LOADED_OPTION_FILE);
    
  selection_flag = FALSE;

  while (fgets(buff, RDBUF_SIZE, opt_fp) != NULL)
  {
    if (buff[0] == '#') continue;	/* skip comment */
    
    i = 0;
    if ((temp=strtok(buff, delimit)) != NULL)
    {
      do
	param[i++] = temp;		/* parse out the tokens */
      while ((temp=strtok((char *)NULL, delimit)) != NULL && i < TOKEN_NUM);
      param[i] = NULL;

      if (i < 3) continue;		/* format error, ignored for now */

      if (strcmp(param[0], "option")==0)/* this is an entry for system option */
        load_system_options(param);	/* load system-wide options */
      else
      {
        load_test_options(param);  /* otherwise, individual test options */
	flag = 1;		/* there is at least one test option executed */
      }
    }
  }

  (void)fclose(opt_fp);

  if (!tty_mode)
  {
    if (!selection_flag)		/* at least one test was modified */
      (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_NONE, 0);
    else 
      (void)panel_set(select_item, PANEL_FEEDBACK, PANEL_INVERTED, 0);

    if (option_frame != NULL)	/* destroy the popup test option frame */
      frame_destroy_proc(option_frame);
  }

  if (update != 0)		/* needs to update the screen */
  {
    if (tty_mode)
      tty_int_sel();
    optfile_update();		/* update the screen if needed */
  }

  return(TRUE);
}

