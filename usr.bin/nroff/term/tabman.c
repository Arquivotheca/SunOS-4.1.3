/*	@(#)tabman.c 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#define INCH 240
/*
 * LPR or CRT 10 Pitch
 * nroff driving table
 * with reverse or half line feeds
 */
struct {
	int bset;
	int breset;
	int Hor;
	int Vert;
	int Newline;
	int Char;
	int Em;
	int Halfline;
	int Adj;
	char *twinit;
	char *twrest;
	char *twnl;
	char *hlr;
	char *hlf;
	char *flr;
	char *bdon;
	char *bdoff;
	char *ploton;
	char *plotoff;
	char *up;
	char *down;
	char *right;
	char *left;
	char *codetab[256-32];
	int zzz;
	} t = {
/*bset*/	0,
/*breset*/	0,
/*Hor*/		INCH/10,
/*Vert*/	INCH/12,
/*Newline*/	INCH/6,
/*Char*/	INCH/10,
/*Em*/		INCH/10,
/*Halfline*/	INCH/12,
/*Adj*/		INCH/10,
/*twinit*/	"",
/*twrest*/	"",
/*twnl*/	"\n",
/*hlr*/		"\0338",
/*hlf*/		"\0339",
/*flr*/		"\0337",
/*bdon*/	"",
/*bdoff*/	"",
/*ploton*/	"",
/*plotoff*/	"",
/*up*/		"",
/*down*/	"",
/*right*/	"",
/*left*/	"",
/*codetab*/
"\001 ",	/*space*/
"\001!",	/*!*/
"\001\"",	/*"*/
"\001#",	/*#*/
"\001$",	/*$*/
"\001%",	/*%*/
"\001&",	/*&*/
"\001'",	/*' close*/
"\001(",	/*(*/
"\001)",	/*)*/
"\001*",	/***/
"\001+",	/*+*/
"\001,",	/*,*/
"\001-",	/*-*/
"\001.",	/*.*/
"\001/",	/*/*/
"\2010",	/*0*/
"\2011",	/*1*/
"\2012",	/*2*/
"\2013",	/*3*/
"\2014",	/*4*/
"\2015",	/*5*/
"\2016",	/*6*/
"\2017",	/*7*/
"\2018",	/*8*/
"\2019",	/*9*/
"\001:",	/*:*/
"\001;",	/*;*/
"\001<",	/*<*/
"\001=",	/*=*/
"\001>",	/*>*/
"\001?",	/*?*/
"\001@",	/*@*/
"\201A",	/*A*/
"\201B",	/*B*/
"\201C",	/*C*/
"\201D",	/*D*/
"\201E",	/*E*/
"\201F",	/*F*/
"\201G",	/*G*/
"\201H",	/*H*/
"\201I",	/*I*/
"\201J",	/*J*/
"\201K",	/*K*/
"\201L",	/*L*/
"\201M",	/*M*/
"\201N",	/*N*/
"\201O",	/*O*/
"\201P",	/*P*/
"\201Q",	/*Q*/
"\201R",	/*R*/
"\201S",	/*S*/
"\201T",	/*T*/
"\201U",	/*U*/
"\201V",	/*V*/
"\201W",	/*W*/
"\201X",	/*X*/
"\201Y",	/*Y*/
"\201Z",	/*Z*/
"\001[",	/*[*/
"\001\\",	/*\*/
"\001]",	/*]*/
"\001^",	/*^*/
"\001_",	/*_*/
"\001`",	/*` open*/
"\201a",	/*a*/
"\201b",	/*b*/
"\201c",	/*c*/
"\201d",	/*d*/
"\201e",	/*e*/
"\201f",	/*f*/
"\201g",	/*g*/
"\201h",	/*h*/
"\201i",	/*i*/
"\201j",	/*j*/
"\201k",	/*k*/
"\201l",	/*l*/
"\201m",	/*m*/
"\201n",	/*n*/
"\201o",	/*o*/
"\201p",	/*p*/
"\201q",	/*q*/
"\201r",	/*r*/
"\201s",	/*s*/
"\201t",	/*t*/
"\201u",	/*u*/
"\201v",	/*v*/
"\201w",	/*w*/
"\201x",	/*x*/
"\201y",	/*y*/
"\201z",	/*z*/
"\001{",	/*{*/
"\001|",	/*|*/
"\001}",	/*}*/
"\001~",	/*~*/
"\000",		/*nar sp*/
"\001-",	/*hyphen*/
"\001o\b+",	/*bullet*/
"\002[]",	/*square*/
"\001-",	/*3/4 em*/
"\001_",	/*rule*/
"\0031/4",	/*1/4*/
"\0031/2",	/*1/2*/
"\0033/4",	/*3/4*/
"\001-",	/*minus*/
"\202fi",	/*fi*/
"\202fl",	/*fl*/
"\202ff",	/*ff*/
"\203ffi",	/*ffi*/
"\203ffl",	/*ffl*/
"\000\0",	/*degree*/
"\001|\b-",	/*dagger*/
"\001s\bS",	/*section*/
"\001'",	/*foot mark*/
"\001'",	/*acute accent*/
"\001`",	/*grave accent*/
"\001_",	/*underrule*/
"\001/",	/*slash (longer)*/
"\000",		/*half narrow space*/
"\001 ",	/*unpaddable space*/
"\201a", 	/*alpha*/
"\201b", 	/*beta*/
"\201g", 	/*gamma*/
"\201d", 	/*delta*/
"\201e", 	/*epsilon*/
"\201z", 	/*zeta*/
"\201h",	/*eta*/
"\202th", 	/*theta*/
"\201i",	/*iota*/
"\201k",	/*kappa*/
"\201l", 	/*lambda*/
"\201m",	/*mu*/
"\201n",	/*nu*/
"\201x", 	/*xi*/
"\201o",	/*omicron*/
"\201p",	/*pi*/
"\201r",	/*rho*/
"\201s", 	/*sigma*/
"\201t", 	/*tau*/
"\201u",	/*upsilon*/
"\202ph", 	/*phi*/
"\202ch",	/*chi*/
"\202ps", 	/*psi*/
"\201w", 	/*omega*/
"\201G", 	/*Gamma*/
"\201D", 	/*Delta*/
"\202TH",	/*Theta*/
"\201L",	/*Lambda*/
"\201X",	/*Xi*/
"\201P",	/*Pi*/
"\201S", 	/*Sigma*/
"\000",		/**/
"\201U",	/*Upsilon*/
"\202PH",	/*Phi*/
"\202PS",	/*Psi*/
"\201W",	/*Omega*/
"\001v\b/",	/*square root*/
"\000\0",	/*terminal sigma*/
"\001~",	/*root en*/
"\002>=",	/*>=*/
"\002<=",	/*<=*/
"\001_\b=",	/*identically equal*/
"\001-",	/*equation minus*/
"\002~=",	/*approx =*/
"\001~",	/*approximates*/
"\002!=",	/*not equal*/
"\002->",	/*right arrow*/
"\002<-",	/*left arrow*/
"\001|\b^",	/*up arrow*/
"\001|\bv",	/*down arrow*/
"\001=",	/*equation equal*/
"\001x",	/*multiply*/
"\001/",	/*divide*/
"\002+-",	/*plus-minus*/
"\002(\b~)\b~",	/*cup (union)*/
"\002(\b_)\b_",	/*cap (intersection)*/
"\002(=",	/*subset of*/
"\002=)",	/*superset of*/
"\002(=\b_",	/*improper subset*/
"\002=\b_)",	/*improper superset*/
"\002oo",	/*infinity*/
"\001o\b`",	/*partial derivative*/
"\002\\\b~/\b~", /*gradient*/
"\000\0",	/*not*/
"\000\0",	/*integral sign*/
"\002oc",	/*proportional to*/
"\002{}",	/*empty set*/
"\001E",	/*member of*/
"\001+",	/*equation plus*/
"\003(R)",	/*registered*/
"\003(C)",	/*copyright*/
"\001|",	/*box rule */
"\001/\bc",	/*cent sign*/
"\001|\b=",	/*dbl dagger*/
"\002=>",	/*right hand*/
"\002<=",	/*left hand*/
"\001*",	/*math * */
"\000\0",	/*bell system sign*/
"\001|",	/*or (was star)*/
"\001O",	/*circle*/
"\001|",	/*left top of big brace*/
"\001|",	/*left bot of big brace*/
"\001|",	/*right top of big brace*/
"\001|",	/*right bot of big brace*/
"\001|",	/*left center of big brace*/
"\001|",	/*right center of big brace*/
"\001|",	/*bold vertical*/
"\001|",	/*left floor (lb of big bracket)*/
"\001|",	/*right floor (rb of big bracket)*/
"\001|",	/*left ceiling (lt of big bracket)*/
"\001|"		/*right ceiling (rt of big bracket)*/
};
