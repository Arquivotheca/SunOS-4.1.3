#ifndef lint
static char	sccsid[] = "@(#)inqinput77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Input Inquiry functions
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
#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqinpcaps                                                 */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfqinpcaps_ (valid,numloc,numval,numstrk,numchoice,numstr,numtrig,
evqueue,asynch, coordmap,echo,tracking,prompt,acknowledgement,trigman)
int * valid;
int * numloc;
int * numval;
int * numstrk;
int * numchoice;
int * numstr;
int * numtrig;
int * evqueue;
int * asynch;
int * coordmap;
int * echo;
int * tracking;
int * prompt;
int * acknowledgement;
int * trigman;
{
Clogical val;	/* device state */
Ccgidesctab  table;	/* CGI input description table */
    int err;
    
    err = inquire_input_capabilities (&val, &table);
     *numloc = table.numloc;
     *numval = table.numval;
     *numstrk = table.numstrk;
     *numchoice = table.numchoice;
     *numstr = table.numstr;
     *numtrig = table.numtrig;
     *valid = (int) val;
     *evqueue = (int) table.event_queue;
     *asynch = (int) table.asynch;
     *coordmap = (int) table.coord_map;
     *echo = (int) table.echo;
     *tracking = (int) table.tracking;
     *prompt = (int) table.prompt;
     *acknowledgement = (int) table.acknowledgement;
     *trigman = (int) table.trigger_manipulation;
        return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqlidcaps                                                 */
/*                                                                          */
/****************************************************************************/
int cfqlidcaps_ (devclass,devnum,valid,sample,change,numassoc,trigassoc,
		promptinp,ackinp,necho,echo,echotype,classdep,state,prompt,
		acknowledgement, x,y,xlist,ylist,n,val,choice,string,
		segid,pickid, ntrig,triggers,echotyp,echosta,echodat,
		f77strlen1,f77strlen2)
int *devclass;
int *devnum;			/* device type, device number */
int *valid;		/* device supported at all */
int *sample;
int *change;
int *numassoc;
int trigassoc[];
int *promptinp;
int *ackinp;
int *necho;
int echo[];
int echotype[];
char *classdep;		/* f77strlen1 is length */
int *state;
int *prompt;
int *acknowledgement;
int *x, *y;
int xlist[], ylist[];
int *n;
float *val;
int *choice;
char *string;		/* f77strlen2 is length */
int *segid;
int *pickid;
int *ntrig;
int triggers[];
int *echotyp, *echosta, *echodat;
{
    Cliddescript table;	/* table of descriptors */
    int err;

    err = inquire_lid_capabilities ((Cdevoff) *devclass, *devnum, 
	(Clogical*)valid, &table);

    /* Pass back all non-structure members of Cliddescript */
    *sample = (int)table.sample;
    *change = (int)table.change;
    *numassoc = (int)table.numassoc;
    RETURN_1ARRAY(trigassoc, table.trigassoc, *numassoc);
    *promptinp = (int)table.prompt;
    *ackinp = (int) table.acknowledgement;
    RETURN_STRING(classdep, table.classdep, f77strlen1);

    /* Pass back the Cechostatelst sub-structure */
    *necho = (int)table.echo->n;
    RETURN_1ARRAY(echo, table.echo->elements, *necho);
    RETURN_1ARRAY(echotype, table.echo->echos, *necho);

    /* Pass back the Cstatelist sub-structure */
    *state = (int)table.state.state;
    *prompt = (int)table.state.prompt;
    *acknowledgement = (int)table.state.acknowledgement;
    RETURN_INREP(*devclass,x,y,xlist,ylist,n,val,choice,string,f77strlen2,
		segid,pickid,table.state.current);
    *ntrig = table.state.n;
    RETURN_1ARRAY(triggers, table.state.triggers, table.state.n);
    *echotyp = (int)table.state.echotyp;
    *echosta = (int)table.state.echosta;
    *echodat = table.state.echodat;

    return (err);
    }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqtrigcaps                                                */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfqtrigcaps_ (trigger,valid,change,n,class,assoc,maxassoc,
	        prompt,acknowledgement,name,description,f77strlen1,f77strlen2)
int    *trigger;		/* trigger number */
int * valid;		/* trigger supported at all */
int * change;
int *   n;
int  class[];
int  assoc[];
int *  maxassoc;
int *     prompt;
int *     acknowledgement;
char   *name;
char   *description;
int f77strlen1;
int f77strlen2;
{
    Ctrigdis tdis;		/* trigger description table */
    int err;
    
    err = inquire_trigger_capabilities (*trigger, (Clogical*) valid, &tdis) ;

    *change = (int) tdis.change;
    *n = (int) tdis.numassoc->n;
    RETURN_1ARRAY(class, tdis.numassoc->class, tdis.numassoc->n);
    RETURN_1ARRAY(assoc, tdis.numassoc->assoc, tdis.numassoc->n);
    *maxassoc = (int) tdis.maxassoc;
    *prompt = (int) tdis.prompt;
    *acknowledgement = (int) tdis.acknowledgement;
    RETURN_STRING(name, tdis.name, f77strlen1);
    RETURN_STRING(description, tdis.description, f77strlen2);
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqlidstate                                                */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

cfqlidstate_ (devclass, devnum, valid, state)
int *devclass;
int *devnum;			/* device type, device number */
int * valid;		/* trigger supported at all */
int * state;		/* event ability */
{
    return(inquire_lid_state (*devclass, *devnum, (Clogical*)valid,
	(Clidstate*)state));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqlidstatelis                                             */
/*                                                                          */
/*		Reports the state of specific input devices.		    */
/****************************************************************************/

int cfqlidstatelis_(devclass, devnum, valid, state, prompt, acknowledgement,
		    x, y, xlist, ylist, n, val, choice, string, segid, pickid,
		    ntrig, triggers, echotyp, echosta, echodat,f77strlen)
int *devclass;
int *devnum;			/* device type, device number */
int * valid;		/* trigger supported at all */
int *state;
int *prompt;
int *acknowledgement;
int *x, *y;
int xlist[], ylist[];
int *n;
float *val;
int *choice;
char *string;		/* f77strlen is length */
int *segid;
int *pickid;
int *ntrig;
int triggers[];
int *echotyp, *echosta, *echodat;
int f77strlen;
{
    Cstatelist value;
    int err;

    err = inquire_lid_state_list (*devclass, *devnum, (Clogical*)valid, &value);

    /* Pass back the Cstatelist structure */
    *state = (int)value.state;
    *prompt = (int)value.prompt;
    *acknowledgement = (int)value.acknowledgement;
    RETURN_INREP(*devclass,x,y,xlist,ylist,n,val,choice,string,f77strlen,
		segid,pickid,value.current);
    *ntrig = value.n;
    RETURN_1ARRAY(triggers, value.triggers, value.n);
    *echotyp = (int)value.echotyp;
    *echosta = (int)value.echosta;
    *echodat = value.echodat;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqtrigstate 						    */
/*                                                                          */
/****************************************************************************/

int     cfqtrigstate_ (trigger, valid, state,n,class,assoc)
int trigger;
int * valid;		/* device state */
int * state;
int * n;
int class[];
int assoc[];
{
    Ctrigstate trig;		/* active or inactive */
    int err;

    err = inquire_trigger_state (trigger, (Clogical*)valid, &trig);
    *state = (int) trig.state;
    *n = trig.assoc->n;
    RETURN_1ARRAY(class, trig.assoc->class, trig.assoc->n);
    RETURN_1ARRAY(assoc, trig.assoc->assoc, trig.assoc->n);
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqevque 						    */
/*                                                                          */
/****************************************************************************/

int     cfqevque_ (qstate, qoflow)
int * qstate;
int * qoflow;
{
    return (inquire_event_queue_state ( (Ceqflow *)qstate, (Cqtype *)qoflow));
}
