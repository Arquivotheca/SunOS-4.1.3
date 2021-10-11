#ident "@(#)ssparc_tree.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

/*
 * fake device tree for SingleSPARC
 *
 */

#include "fakeprom.h"
#include "sunergy_physaddr.h"

extern devnode_t mod0_info;
extern struct property mod0_props[];

devnode_t mod0_info = {
        NULL,     	/* next */
        NULL,   	/* slaves */
        &mod0_props[0], /* props */
};
/*
 * XXX - discuss what these values should really be!
 */
static u_int 	p_mod_pagesize  = P_SSPARC_NBPG;
static u_int 	p_mid0  	= 0;
static u_int 	p_nmmus 	= 1;
static u_int 	p_ncaches 	= 1;	/* should be 1 or 2? */
static u_int 	p_csplit 	= 1;
static u_int 	p_pcache 	= 1;
static u_int 	p_wthru 	= 1;
static u_int 	p_cnlines 	= 128;	/* same number of lines in I-$ & D-$ */
static u_int 	p_iclinesize 	= 32;	/* I-$ linesize is 32 */
static u_int 	p_dclinesize 	= 16;	/* D-$ linesize is 16 */
static u_int 	p_cassocia 	= 1;
static u_int 	p_nctx 		= 64;
static u_int 	p_sparcversion 	= 8;
static u_int 	p_mips_off 	= 3;
static u_int 	p_mips_on  	= 20;

static struct property mod0_props[] = {
        { psname, 		8,  "modi5v0" },
	{ "mid", 		ARRAY(p_mid0)},
        { psdevtype, 		10, "processor" },
	{ "nmmus", 		ARRAY(p_nmmus)},
	{ "mmu-nctx", 		ARRAY(p_nctx)},
	{ "ncaches", 		ARRAY(p_ncaches)},
	{ "cache-splitid?", 	BOOLEAN },
	{ "cache-physical?", 	BOOLEAN },
	{ "cache-wthru?", 	BOOLEAN },
	{ "cache-nlines", 	ARRAY(p_cnlines)},
	{ "cache-ilinesize", 	ARRAY(p_iclinesize)},
	{ "cache-dlinesize", 	ARRAY(p_dclinesize)},
	{ "cache-associativity", ARRAY(p_cassocia)},
	{ pspagesize, 		ARRAY(p_mod_pagesize) },
	{ "sparcversion", 	ARRAY(p_sparcversion)},
        { "mips-off", 		ARRAY(p_mips_off)},
        { "mips-on", 		ARRAY(p_mips_on)},
        { 0, 0, 0, },
        { NULL },
};
