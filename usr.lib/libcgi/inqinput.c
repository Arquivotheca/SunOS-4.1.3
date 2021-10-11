#ifndef lint
static char	sccsid[] = "@(#)inqinput.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
 * Permission to use, copy, modify, and distribute this software for any 
 * purpose and without fee is hereby granted, provided that the above 
 * copyright notice appear in all copies and that both that copyright 
 * notice and this permission notice are retained, and that the name 
 * of Sun Microsystems, Inc., not be used in advertising or publicity 
 * pertaining to this software without specific, written prior permission. 
 * Sun Microsystems, Inc., makes no representations about the suitability 
 * of this software or the interface defined in this software for any 
 * purpose. It is provided "as is" without express or implied warranty.
 */

/*
inquire_input_capabilities
inquire_lid_capabilities
inquire_trigger_capabilities
inquire_lid_state
inquire_lid_state_list
inquire_trigger_state
inquire_event_queue_state
*/

#include "cgipriv.h"

struct locatstr *_cgi_locator[_CGI_LOCATORS];
struct keybstr *_cgi_keybord[_CGI_KEYBORDS];
struct strokstr *_cgi_stroker[_CGI_STROKES];
struct valstr  *_cgi_valuatr[_CGI_VALUATRS];
struct choicestr *_cgi_button[_CGI_CHOICES];
Cqtype _cgi_que_status;		/* event queue status */
Ceqflow _cgi_que_over_status;	/* event queue status */
int             _cgi_trigger[MAXTRIG][MAXASSOC + 1];	/* association list */


/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_input_capabilities		 		    */
/*                                                                          */
/*		Reports overall input capabilities			    */
/****************************************************************************/
Cerror          inquire_input_capabilities(valid, table)
Clogical       *valid;		/* device state */
Ccgidesctab    *table;		/* CGI input description table */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	table->numloc = _CGI_LOCATORS;
	table->numval = _CGI_VALUATRS;
	table->numstrk = _CGI_STROKES;
	table->numchoice = _CGI_CHOICES;
	table->numstr = _CGI_KEYBORDS;
	table->numtrig = MAXTRIG;
	table->event_queue = ALL_NON_REQUIRED_FUNCTIONS;
	table->asynch = ALL_NON_REQUIRED_FUNCTIONS;
	table->coord_map = ALL_NON_REQUIRED_FUNCTIONS;
	table->echo = NONE;	/* WDS added 850823: */
	table->tracking = ALL_NON_REQUIRED_FUNCTIONS;
	table->prompt = NONE;
	table->acknowledgement = ALL_NON_REQUIRED_FUNCTIONS;
	table->trigger_manipulation = ALL_NON_REQUIRED_FUNCTIONS;
	*valid = L_TRUE;
    }
    else
	*valid = L_FALSE;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_lid_capabilities		 	    	    */
/*                                                                          */
/*		Describes the capabilities of a specific device		    */
/****************************************************************************/
Cerror          inquire_lid_capabilities(devclass, devnum, valid, table)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Clogical       *valid;		/* device supported at all */
Cliddescript   *table;		/* table of descriptors */
{
    int             err;
    static Cechotypelst echo_lst;
    static Cechotype e1[1] =
    {
     PRINTERS_FIST
    };
    static Cechotype e2[2] =
    {
     PRINTERS_FIST, STRING_ECHO
    };
    static Cechotype e3[2] =
    {
     PRINTERS_FIST, STRING_ECHO
    };
    static Cechotype e4[5] =
    {
     PRINTERS_FIST, SOLID_LINE, XLINE, YLINE,
     RUBBER_BAND_BOX
    };
    static Cechoav  s1[1] =
    {
     E_TRACK
    };
    static Cechoav  s2[2] =
    {
     E_TRACK, E_TRACK
    };
    static Cechoav  s3[5] =
    {
     E_TRACK, E_TRACK, E_TRACK, E_TRACK, E_TRACK
    };
    static int      t1[1] =
    {
     1
    };
    static int      t2[4] =
    {
     2, 3, 4, 5
    };
    static int      t3[3] =
    {
     2, 3, 4
    };
    static char     class1[] = "None";
    static char     class2[] = "yes,0,0,32767,32767";
    static char     class3[] = "0,1";
    static char     class4[] = "No,No";
    static char     class5[] = "Yes";
    static Cinrep   b0;
    static Cstatelist state1;

    state1.state = RELEASE;
    state1.prompt = PROMPT_OFF;
    state1.acknowledgement = ACK_OFF;
    state1.current = &b0;
    state1.n = 0;
    state1.triggers = (Cint *) 0;
    state1.echotyp = NO_ECHO;
    state1.echosta = ECHO_OFF;
    *valid = L_TRUE;
    err = _cgi_check_state_5();
    if (!err)
    {
	*valid = L_FALSE;
	table->sample = L_TRUE;
	table->change = YES;
	table->prompt = NO_INPUT;
	table->acknowledgement = NO_INPUT;
	table->state = state1;
	switch (devclass)
	{
	case IC_STRING:
	    if (devnum < 1 || devnum > _CGI_KEYBORDS)
		err = 80;
	    else
	    {
		table->numassoc = 1;
		table->trigassoc = t1;
		echo_lst.n = 1;
		echo_lst.echos = e2;
		echo_lst.elements = s1;
		table->echo = &echo_lst;
		table->classdep = class1;
	    }
	    break;
	case IC_LOCATOR:
	    if (devnum < 1 || devnum > _CGI_LOCATORS)
		err = 80;
	    else
	    {
		table->numassoc = 4;
		table->trigassoc = t2;
		echo_lst.n = 1;
		echo_lst.echos = e1;
		echo_lst.elements = s1;
		table->echo = &echo_lst;
		table->classdep = class2;
	    }
	    break;
	case IC_CHOICE:
	    if (devnum < 1 || devnum > _CGI_CHOICES)
		err = 80;
	    else
	    {
		table->numassoc = 3;
		table->trigassoc = t3;
		echo_lst.n = 1;
		echo_lst.echos = e1;
		echo_lst.elements = s1;
		table->echo = &echo_lst;
		table->classdep = class3;
	    }
	    break;
	case IC_VALUATOR:
	    if (devnum < 1 || devnum > _CGI_VALUATRS)
		err = 80;
	    else
	    {
		table->numassoc = 4;
		table->trigassoc = t2;
		echo_lst.n = 2;
		echo_lst.echos = e3;
		echo_lst.elements = s2;
		table->echo = &echo_lst;
		table->classdep = class4;
	    }
	    break;
	case IC_STROKE:
	    if (devnum < 1 || devnum > _CGI_STROKES)
		err = 80;
	    else
	    {
		table->numassoc = 3;
		table->trigassoc = t3;
		echo_lst.n = 5;
		echo_lst.echos = e4;
		echo_lst.elements = s3;
		table->echo = &echo_lst;
		table->classdep = class5;
	    }
	    break;
	default:
	    err = 80;
	    break;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_trigger_capabilities		 		    */
/*                                                                          */
/*		Describes how a particular trigger can be associated.	    */
/****************************************************************************/

Cerror          inquire_trigger_capabilities(trigger, valid, tdis)
Cint            trigger;	/* trigger number */
Clogical       *valid;		/* trigger supported at all */
Ctrigdis       *tdis;		/* trigger description table */
{
    int             err;
    static char     class1[] = "Keyboard";
    static char     class2[] = "Left mouse button";
    static char     class3[] = "Center mouse button";
    static char     class4[] = "Right mouse button";
    static char     class5[] = "Mouse movement";
    static Cdevoff  c1[1] =
    {
     IC_STRING
    };
    static Cdevoff  c2[4] =
    {
     IC_STROKE, IC_LOCATOR, IC_VALUATOR, IC_CHOICE
    };
#define c3 c2
#define c4 c2
    static Cdevoff  c5[2] =
    {
     IC_LOCATOR, IC_VALUATOR
    };
/*     Cassoclid * ptr; */
    static Cassoclid ptr;

    static Cint     a1[1] = 1;
    static Cint     a2[4] =
    {
     3, 4, 4, 3
    };
#define a3 a2
#define a4 a2
    static Cint     a5[2] =
    {
     3, 4
    };				/* 3 Locator devices, 4 valuator devices */

    *valid = L_FALSE;
    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_err_check_86(trigger);
    if (!err)
    {
	*valid = L_TRUE;
/* 	ptr = (Cassoclid *) malloc (sizeof (Cassoclid)); */
	tdis->change = YES;
	tdis->prompt = PROMPT_OFF;
	tdis->acknowledgement = ACK_OFF;
	switch (trigger)
	{
	case 1:
	    tdis->maxassoc = 1;
	    ptr.n = 1;
	    ptr.class = c1;
	    ptr.assoc = a1;
	    tdis->numassoc = &ptr;
	    tdis->name = class1;
	    break;
	case 2:
	    tdis->maxassoc = 4;
	    ptr.n = 4;
	    ptr.class = c2;
	    ptr.assoc = a2;
	    tdis->numassoc = &ptr;
	    tdis->name = class2;
	    break;
	case 3:
	    tdis->maxassoc = 4;
	    ptr.n = 4;
	    ptr.class = c3;
	    ptr.assoc = a3;
	    tdis->numassoc = &ptr;
	    tdis->name = class3;
	    break;
	case 4:
	    tdis->maxassoc = 4;
	    ptr.n = 4;
	    ptr.class = c4;
	    ptr.assoc = a4;
	    tdis->numassoc = &ptr;
	    tdis->name = class4;
	    break;
	case 5:
	    tdis->maxassoc = 2;
	    ptr.n = 2;
	    ptr.class = c5;
	    ptr.assoc = a5;
	    tdis->numassoc = &ptr;
	    tdis->name = class5;
	    break;
	}
    }
    return (_cgi_errhand(err));
#undef a3
#undef a4
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_lid_state					    */
/*                                                                          */
/*                                                                          */
/*		Reports the state of specific input devices.		    */
/****************************************************************************/

Cerror          inquire_lid_state(devclass, devnum, valid, state)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Clogical       *valid;		/* trigger supported at all */
Clidstate      *state;		/* event ability */
{
    int             err;
    err = _cgi_err_check_480(devclass, devnum);
    if (!err)
    {
	switch (devclass)
	{
	case IC_STRING:
	    if (_cgi_keybord[devnum - 1])
		*state = (Clidstate) _cgi_keybord[devnum - 1]->subkey.enable;
	    else
		*state = RELEASE;
	    break;
	case IC_STROKE:
	    if (_cgi_stroker[devnum - 1])
		*state = (Clidstate) _cgi_stroker[devnum - 1]->substroke.enable;
	    else
		*state = RELEASE;
	    break;
	case IC_LOCATOR:
	    if (_cgi_locator[devnum - 1])
		*state = (Clidstate) _cgi_locator[devnum - 1]->subloc.enable;
	    else
		*state = RELEASE;
	    break;
	case IC_VALUATOR:
	    if (_cgi_valuatr[devnum - 1])
		*state = (Clidstate) _cgi_valuatr[devnum - 1]->subval.enable;
	    else
		*state = RELEASE;
	    break;
	case IC_CHOICE:
	    if (_cgi_button[devnum - 1])
		*state = (Clidstate) _cgi_button[devnum - 1]->subchoice.enable;
	    else
		*state = RELEASE;
	}
	*valid = L_TRUE;
    }
    else
	*valid = L_FALSE;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_lid_state_list					    */
/*                                                                          */
/*                                                                          */
/*		Reports the state of specific input devices.		    */
/****************************************************************************/

Cerror          inquire_lid_state_list(devclass, devnum, valid, list)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Clogical       *valid;		/* trigger supported at all */
Cstatelist     *list;		/* event ability */
{
    int             err;
    Clidstate       state;	/* event ability */
    int             cnt, i, j, num, num2, temp[5];
    static int      array[(MAXTRIG) * (MAXASSOC + 1)];
    static Cinrep   irep;
    static Ccoor    pt;
    err = inquire_lid_state(devclass, devnum, valid, &state);
    if (!err)
    {
	if (state == RELEASE)
	{
	    list->state = state;
	    list->prompt = PROMPT_OFF;
	    list->acknowledgement = ACK_OFF;
	}
	else
	{
	    list->state = state;
	    list->prompt = PROMPT_OFF;
	    list->acknowledgement = ACK_OFF;
	    num = _cgi_int_dev_conv(devclass, devnum);
	    cnt = 0;
	    for (i = 0; i < 5; i++)
	    {
		num2 = _cgi_trigger[i][0];
		for (j = 1; j < num2 + 1; j++)
		{
		    if (num == _cgi_trigger[i][j])
		    {
/* 			temp[cnt] = _cgi_trigger[i][j]; */
			temp[cnt] = i + 1;
			cnt++;
		    }

		}
	    }
	    list->n = cnt;
/* 	    list->triggers = (int *) malloc (cnt * sizeof (int)); */
	    list->triggers = array;
	    for (i = 0; i < cnt; i++)
	    {
		list->triggers[i] = temp[i];
	    }
	    list->current =
		&irep;
/* 	        (Cinrep*) malloc (sizeof (Cinrep)); */
	    switch (devclass)
	    {
	    case IC_STRING:
		list->current->string = _cgi_keybord[devnum - 1]->initstring;
		if (_cgi_keybord[devnum - 1]->subkey.echo > 0)
		{
		    list->echotyp = STRING_ECHO;
		    list->echosta = TRACK_ON;
		}
		else
		    list->echosta = ECHO_OFF;
		break;
	    case IC_STROKE:
		list->current->points = _cgi_stroker[devnum - 1]->sarray;
		if (_cgi_stroker[devnum - 1]->substroke.echo > 0)
		{
		    switch (_cgi_stroker[devnum - 1]->substroke.echo)
		    {
		    case 1:
			list->echotyp = PRINTERS_FIST;
			break;
		    case 2:
			list->echotyp = SOLID_LINE;
			break;
		    case 3:
			list->echotyp = SOLID_LINE;
			break;
		    case 4:
			list->echotyp = SOLID_LINE;
			break;
		    case 5:
			list->echotyp = RUBBER_BAND_BOX;
			break;
		    }
		    list->echosta = TRACK_ON;
		}
		else
		    list->echosta = ECHO_OFF;
		break;
	    case IC_LOCATOR:
		list->current->xypt = &pt;
/* 		        (Ccoor*) malloc (sizeof (Ccoor)); */
		list->current->xypt->x = _cgi_locator[devnum - 1]->x;
		list->current->xypt->y = _cgi_locator[devnum - 1]->y;
		if (_cgi_locator[devnum - 1]->subloc.echo)
		{
		    list->echotyp = PRINTERS_FIST;
		    list->echosta = TRACK_ON;
		}
		else
		    list->echosta = ECHO_OFF;
		break;
	    case IC_VALUATOR:
		list->current->val = _cgi_valuatr[devnum - 1]->curval;
		if (_cgi_valuatr[devnum - 1]->subval.echo > 0)
		{
		    switch (_cgi_valuatr[devnum - 1]->subval.echo)
		    {
		    case 1:
			list->echotyp = PRINTERS_FIST;
			break;
		    case 2:
			list->echotyp = STRING_ECHO;
			break;
		    }
		    list->echosta = TRACK_ON;
		}
		else
		    list->echosta = ECHO_OFF;
		break;
	    case IC_CHOICE:
		list->current->choice = _cgi_button[devnum - 1]->curval;
		if (_cgi_button[devnum - 1]->subchoice.echo > 0)
		{
		    list->echotyp = PRINTERS_FIST;
		    list->echosta = TRACK_ON;
		}
		else
		    list->echosta = ECHO_OFF;
		break;
	    }
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_trigger_state	 				    */
/*                                                                          */
/*		Describes the binding between a trigger and input devices.  */
/****************************************************************************/

Cerror          inquire_trigger_state(trigger, valid, list)
Cint            trigger;	/* trigger number */
Clogical       *valid;		/* device state */
Ctrigstate     *list;		/* active or inactive */
{

    int             err;
    Cdevoff         devclass;
    Cint            devnum;	/* device type, device number */
/*    Cassoclid * beep;
    Cdevoff * dc;
    Cint * dn; */
    static Cassoclid beep;
    static Cdevoff  dc[MAXTRIG];
    static Cint     dn[MAXTRIG];
    int             i, tpt;

    err = _cgi_err_check_4();
    if (!err)
	err = _cgi_err_check_86(trigger);
    if (!err)
    {
	*valid = L_TRUE;
	tpt = _cgi_trigger[trigger - 1][0];
	if (tpt)
	{			/* report associations if they exist */
	    list->state = ACTIVE;
/*	    beep = (Cassoclid *) malloc (sizeof (Cassoclid));
	    dc = (Cdevoff *) malloc (tpt * sizeof (Cdevoff));
	    dn = (Cint *) malloc (tpt * sizeof (Cint)); */
	    list->assoc = &beep;
	    list->assoc->n = tpt;
	    list->assoc->class = dc;
	    list->assoc->assoc = dn;
	    for (i = 0; i < tpt; i++)
	    {
		_cgi_int_rev_dev_conv
		    (_cgi_trigger[trigger - 1][i + 1], &devclass, &devnum);
		list->assoc->class[i] = devclass;
		list->assoc->assoc[i] = devnum;
	    }
	}
	else
	    list->state = INACTIVE;
    }
    else
	*valid = L_FALSE;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_event_queue_state 			            */
/*                                                                          */
/****************************************************************************/
Cerror          inquire_event_queue_state(qstat, qflow)
Cqtype         *qstat;
Ceqflow        *qflow;
{
    int             err;
    err = _cgi_err_check_4();
    if (!err)
    {
	*qstat = _cgi_que_status;
	*qflow = _cgi_que_over_status;
    }
    return (_cgi_errhand(err));

}
