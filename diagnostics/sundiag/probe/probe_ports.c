#ifndef lint
static  char sccsid[] = "@(#)probe_ports.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#ifndef SVR4
#include <sys/dir.h>
#endif SVR4

#ifdef SVR4
#include <sys/fcntl.h>
#endif SVR4

#include "probe.h"
#include "sdrtns.h"		/* sundiag standard header */
#include "../../lib/include/probe_sundiag.h"

#ifndef SVR4
extern char *sprintf();
#endif SVR4

#define HSS_IFDDEVS     4

/*
 * check_hss_dev(makedevs, dunit) - checks sunlink HSS device dunit files
 * (see sunlink HSS documentation and MAKEDEV.sunlink)
 */
check_hss_dev(makedevs, dunit)
    int makedevs;
    int dunit;
{
    int fmajor = 45;
    char *mode = "0600";
    char name[MAXNAMLEN];
    int nunit;
    int num_devfiles = 0;	/* number of /dev/ifd# files.  2 files if 1 hss
				   board; 4 files if 2 boards. */

    func_name = "check_hss_dev";
    TRACE_IN
    if (dunit == 1)
	num_devfiles = HSS_IFDDEVS;
    else
	num_devfiles = HSS_IFDDEVS/2;
    for (nunit = 0; nunit < num_devfiles; nunit++) {
	(void) sprintf(name, "/dev/ifd%d", nunit);
        if ((check_dev(makedevs, name, character, fmajor, nunit, mode, 
	    no)) != 1)
	{
	    TRACE_OUT
            return(0);
	}
    }
    TRACE_OUT
    return(1);
}

#define MCP_TTYDEVS     16
#define MCP_IFDDEVS     16

/*
 * check_mcp_dev(makedevs, dunit) - checks sunlink MCP device dunit files
 * (see sunlink MCP documentation and MAKEDEV)
 */
check_mcp_dev(makedevs, dunit)
    int makedevs;
    int dunit;
{
    int fmajor = 44;
    char *mode = "0666";
    char *ifdmode = "0644";
    char name[MAXNAMLEN];
    char *ch;
    int  fminor, nunit;
    int	 pp_major;
    int  state = 0;

    func_name = "check_mcp_dev";
    TRACE_IN

    switch(dunit) {
    case 0:
        ch = "h";
        break;  
    case 1:  
        ch = "i";
        break;  
    case 2:  
        ch = "j";
        break;  
    case 3:  
        ch = "k";
        break;  
    case 4:  
        ch = "l";
        break;  
    case 5:  
        ch = "m";
        break;
    case 6:
        ch = "n";
        break;
    case 7:
        ch = "o";
        break;
    default:
        ch = "h";
        break;
    }

    (void) sprintf(name, "/dev/tty%s0", ch);
    if (access(name, F_OK) != 0) {      /* found no ALM2 tty device file  */
        state = 1;	/* assume IFD exising for MCP sunlink test */
       for (nunit = 0; nunit < MCP_IFDDEVS; nunit++) {
           (void) sprintf(name, "/dev/ifd%d", nunit);
           if ((check_dev(makedevs, name, character, 0, nunit, ifdmode,
              mcp)) != 1) {
  	      state = -1;			/* NO IFD device exists */
	      break;
           }
       }
    } /* end of if */

    if (state == 1) {
	TRACE_OUT
	return(IFDDEVONLY);	/* ask sundiag kernel to test sunlink only */
    } 

    for (nunit = 0; nunit < MCP_TTYDEVS; nunit++) {
        (void) sprintf(name, "/dev/tty%s%x", ch, nunit);
        fminor = nunit + (16 * dunit);
        if ((check_dev(makedevs, name, character, fmajor, fminor, 
	    mode, no)) != 1)
 	{
	    TRACE_OUT
            return(0);  	/* ALM2 devices not exist, not just tty */
	}
    }

    (void) sprintf(name, "/dev/mcpp%d", dunit);
    pp_major = fmajor;
    fminor = dunit + 64; 
#ifndef sun386
    pp_major = 66;
    fminor = dunit;
#endif   
    check_dev(makedevs, name, character, pp_major, fminor, mode, no);    

    TRACE_OUT
    return(MCPOK);		/* TTY and PRINTER both exists on MCP */
}

#define MTI_TTYDEVS     16

/*
 * check_mti_dev(makedevs, dunit) - checks Systech MTI device dunit files
 * (see mti(4s) and MAKEDEV)
 */
check_mti_dev(makedevs, dunit)
    int makedevs;
    int dunit;
{
    int fmajor = 10;
    char *mode = "0600";
    char name[MAXNAMLEN];
    int fminor, nunit;

    func_name = "check_mti_dev";
    TRACE_IN
    for (nunit = 0; nunit < MTI_TTYDEVS; nunit++) {
        (void) sprintf(name, "/dev/tty%d%x", dunit, nunit);
        fminor = nunit + (16 * dunit);
        if ((check_dev(makedevs, name, character, fmajor, fminor, 
	    mode, no)) != 1)
	{
	    TRACE_OUT
            return(0);
	}
    }
    TRACE_OUT
    return(1);

}

#define SCP_DCPDEVS     4

/*
 * check_dcp_dev(makedevs, dunit) - checks sunlink SCP device dunit files
 * (see sunlink SCP documentation and MAKEDEV.dcp)
 */
check_dcp_dev(makedevs, dunit)
    int makedevs;
    int dunit;
{
    char *mode = "0666";
    char dname[MAXNAMLEN];
    char memname[MAXNAMLEN];
    char consname[MAXNAMLEN];
    char name[MAXNAMLEN];
    int fminor, nunit;
    int memminor, consminor;

    func_name = "check_dcp_dev";
    TRACE_IN
    switch(dunit) {
    case 0:
        (void) sprintf(dname, "/dev/dcpa");
        (void) sprintf(memname, "/dev/dcpamem");
        (void) sprintf(consname, "/dev/dcpacons");
        break;
    case 1:
        (void) sprintf(dname, "/dev/dcpb");
        (void) sprintf(memname, "/dev/dcpbmem");
        (void) sprintf(consname, "/dev/dcpbcons");
        break;
    case 2:
        (void) sprintf(dname, "/dev/dcpc");
        (void) sprintf(memname, "/dev/dcpcmem");
        (void) sprintf(consname, "/dev/dcpccons");
        break;
    case 3:
        (void) sprintf(dname, "/dev/dcpd");
        (void) sprintf(memname, "/dev/dcpdmem");
        (void) sprintf(consname, "/dev/dcpdcons");
        break;
    }
    memminor = 32 + (dunit * 8);
    consminor = 33 + (dunit * 8);
    if ((check_dev(makedevs, memname, character, 0, memminor, mode, 
	scp)) != 1)
    {
	TRACE_OUT
        return(0);
    }
    if ((check_dev(makedevs, consname, character, 0, consminor, mode, 
	scp)) != 1)
    {
	TRACE_OUT
        return(0);
    }
    for (nunit = 0; nunit < SCP_DCPDEVS; nunit++) {
	(void) sprintf(name, "%s%d", dname, nunit);
	fminor =  nunit + (dunit * 8);
        if ((check_dev(makedevs, name, character, 0, fminor, mode, 
	    scp)) != 1)
        {
	    TRACE_OUT
            return(0);
	}
    }
    TRACE_OUT
    return(1);
}

#ifndef	sun386
#define ZS_TTYDEVS     2
#else
#define ZS_TTYDEVS     1
#endif

/*
 * check_sp_dev(makedevs, dunit) - checks Zilog 8530 SCC device dunit files
 * (see zs(4s) and MAKEDEV)
 */
check_sp_dev(makedevs, dunit)
    int makedevs;
    int dunit;
{
    char *mode;
    char *name;
    int nunit, fmajor, fminor;

    func_name = "check_sp_dev";
    TRACE_IN
    for (nunit = 0; nunit < ZS_TTYDEVS; nunit++) {
        switch (dunit) {
    	case 0:
    	    mode = "0666";
	    if (nunit == 0) {
                name = "/dev/ttya";
                fmajor = 12;
                fminor = 0;
	    } else {
                name = "/dev/ttyb";
                fmajor = 12;
                fminor = 1;
	    }
            break;
    	case 1:
    	    mode = "0666";
	    if (nunit == 0) {
                name = "/dev/mouse";
                fmajor = 13;
                fminor = 0;
	    } else {
            	name = "/dev/kbd";
            	fmajor = 29;
            	fminor = 0;
	    }
            break;
        case 2:
    	    mode = "0600";
	    if (nunit == 0) {
                name = "/dev/ttyc";
            	fmajor = 12;
            	fminor = 4;
	    } else {
            	name = "/dev/ttyd";
            	fmajor = 12;
            	fminor = 5;
	    }
            break;
        case 3:
    	    mode = "0600";
	    if (nunit == 0) {
            	name = "/dev/ttys2";
            	fmajor = 12;
            	fminor = 6;
	    } else {
            	name = "/dev/ttys3";
            	fmajor = 12;
            	fminor = 7;
	    }
            break;
        case 4:
    	    mode = "0600";
	    if (nunit == 0) {
            	name = "/dev/ttyt0";
            	fmajor = 12;
            	fminor = 8;
	    } else {
            	name = "/dev/ttyt1";
            	fmajor = 12;
            	fminor = 9;
	    }
            break;
        case 5:
    	    mode = "0600";
	    if (nunit == 0) {
            	name = "/dev/ttyt2";
            	fmajor = 12;
            	fminor = 10;
	    } else {
            	name = "/dev/ttyt3";
            	fmajor = 12;
            	fminor = 11;
	    }
            break;
        }
        if ((check_dev(makedevs, name, character, fmajor, fminor, 
	    mode, no)) != 1)
 	{
	    TRACE_OUT
            return(0);
	}
    }
    TRACE_OUT
    return(1);
}

#define SPIF_TTYDEVS     8

/*
 * check_spif_dev(makedevs, dunit) - checks Spif device dunit files
 */
check_spif_dev(makedevs, dunit)
    int makedevs;
    int dunit;
{
    int fmajor = 59;
    char *mode = "0666";
    char name[MAXNAMLEN];
    int fminor, nunit, T;

    func_name = "check_spif_dev";
    TRACE_IN
    for (nunit = 0; nunit < SPIF_TTYDEVS; nunit++) {
	T = (dunit * 7 + nunit);
	if (T < 16) {
            (void) sprintf(name, "/dev/ttyz0%x", T);
	}
	else 
            (void) sprintf(name, "/dev/ttyz%x", T);

        fminor = 128 + nunit + (7 * dunit);
        if ((check_dev(makedevs, name, character, fmajor, fminor, 
	    mode, no)) != 1)
	{
            TRACE_OUT
            return(0);
	}
    }
    for (nunit = 0; nunit < dunit + 1; nunit++) {
	(void) sprintf(name, "/dev/stclp%d", nunit);
        fminor = (nunit + 8) * 8; 
	if ((check_dev(makedevs, name, character, fmajor, fminor,
            mode, no)) != 1)
        {
            TRACE_OUT
            return(0);
        }
    }	
    TRACE_OUT
    return(1);
}
