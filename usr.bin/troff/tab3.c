#ifndef lint
static	char sccsid[] = "@(#)tab3.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/7/82 */
#endif lint

#define BYTE 8
#define PAIR(A,B) (A|(B<<BYTE))
/*
character name and code tables
default width tables
modified for BTL special font version 4
and Commercial II
*/

int chtab [] = {
PAIR('h','y'), 0200,	/*hyphen*/
PAIR('b','u'), 0201,	/*bullet*/
PAIR('s','q'), 0202,	/*square*/
PAIR('e','m'), 0203,	/*3/4em*/
PAIR('r','u'), 0204,	/*rule*/
PAIR('1','4'), 0205,	/*1/4*/
PAIR('1','2'), 0206,	/*1/2*/
PAIR('3','4'), 0207,	/*3/4*/
PAIR('m','i'), 0302,	/*equation minus*/
PAIR('f','i'), 0211,	/*fi*/
PAIR('f','l'), 0212,	/*fl*/
PAIR('f','f'), 0213,	/*ff*/
PAIR('F','i'), 0214,	/*ffi*/
PAIR('F','l'), 0215,	/*ffl*/
PAIR('d','e'), 0216,	/*degree*/
PAIR('d','g'), 0217,	/*dagger*/
PAIR('s','c'), 0220,	/*section*/
PAIR('f','m'), 0221,	/*foot mark*/
PAIR('a','a'), 0222,	/*acute accent*/
PAIR('g','a'), 0223,	/*grave accent*/
PAIR('u','l'), 0224,	/*underrule*/
PAIR('s','l'), 0225,	/*slash (longer)*/
PAIR('*','a'), 0230,	/*alpha*/
PAIR('*','b'), 0231,	/*beta*/
PAIR('*','g'), 0232,	/*gamma*/
PAIR('*','d'), 0233,	/*delta*/
PAIR('*','e'), 0234,	/*epsilon*/
PAIR('*','z'), 0235,	/*zeta*/
PAIR('*','y'), 0236,	/*eta*/
PAIR('*','h'), 0237,	/*theta*/
PAIR('*','i'), 0240,	/*iota*/
PAIR('*','k'), 0241,	/*kappa*/
PAIR('*','l'), 0242,	/*lambda*/
PAIR('*','m'), 0243,	/*mu*/
PAIR('*','n'), 0244,	/*nu*/
PAIR('*','c'), 0245,	/*xi*/
PAIR('*','o'), 0246,	/*omicron*/
PAIR('*','p'), 0247,	/*pi*/
PAIR('*','r'), 0250,	/*rho*/
PAIR('*','s'), 0251,	/*sigma*/
PAIR('*','t'), 0252,	/*tau*/
PAIR('*','u'), 0253,	/*upsilon*/
PAIR('*','f'), 0254,	/*phi*/
PAIR('*','x'), 0255,	/*chi*/
PAIR('*','q'), 0256,	/*psi*/
PAIR('*','w'), 0257,	/*omega*/
PAIR('*','A'), 0101,	/*Alpha*/
PAIR('*','B'), 0102,	/*Beta*/
PAIR('*','G'), 0260,	/*Gamma*/
PAIR('*','D'), 0261,	/*Delta*/
PAIR('*','E'), 0105,	/*Epsilon*/
PAIR('*','Z'), 0132,	/*Zeta*/
PAIR('*','Y'), 0110,	/*Eta*/
PAIR('*','H'), 0262,	/*Theta*/
PAIR('*','I'), 0111,	/*Iota*/
PAIR('*','K'), 0113,	/*Kappa*/
PAIR('*','L'), 0263,	/*Lambda*/
PAIR('*','M'), 0115,	/*Mu*/
PAIR('*','N'), 0116,	/*Nu*/
PAIR('*','C'), 0264,	/*Xi*/
PAIR('*','O'), 0117,	/*Omicron*/
PAIR('*','P'), 0265,	/*Pi*/
PAIR('*','R'), 0120,	/*Rho*/
PAIR('*','S'), 0266,	/*Sigma*/
PAIR('*','T'), 0124,	/*Tau*/
PAIR('*','U'), 0270,	/*Upsilon*/
PAIR('*','F'), 0271,	/*Phi*/
PAIR('*','X'), 0130,	/*Chi*/
PAIR('*','Q'), 0272,	/*Psi*/
PAIR('*','W'), 0273,	/*Omega*/
PAIR('s','r'), 0274,	/*square root*/
PAIR('t','s'), 0275,	/*terminal sigma*/
PAIR('r','n'), 0276,	/*root en*/
PAIR('>','='), 0277,	/*>=*/
PAIR('<','='), 0300,	/*<=*/
PAIR('=','='), 0301,	/*identically equal*/
PAIR('~','='), 0303,	/*approx =*/
PAIR('a','p'), 0304,	/*approximates*/
PAIR('!','='), 0305,	/*not equal*/
PAIR('-','>'), 0306,	/*right arrow*/
PAIR('<','-'), 0307,	/*left arrow*/
PAIR('u','a'), 0310,	/*up arrow*/
PAIR('d','a'), 0311,	/*down arrow*/
PAIR('e','q'), 0312,	/*equation equal*/
PAIR('m','u'), 0313,	/*multiply*/
PAIR('d','i'), 0314,	/*divide*/
PAIR('+','-'), 0315,	/*plus-minus*/
PAIR('c','u'), 0316,	/*cup (union)*/
PAIR('c','a'), 0317,	/*cap (intersection)*/
PAIR('s','b'), 0320,	/*subset of*/
PAIR('s','p'), 0321,	/*superset of*/
PAIR('i','b'), 0322,	/*improper subset*/
PAIR('i','p'), 0323,	/*  " superset*/
PAIR('i','f'), 0324,	/*infinity*/
PAIR('p','d'), 0325,	/*partial derivative*/
PAIR('g','r'), 0326,	/*gradient*/
PAIR('n','o'), 0327,	/*not*/
PAIR('i','s'), 0330,	/*integral sign*/
PAIR('p','t'), 0331,	/*proportional to*/
PAIR('e','s'), 0332,	/*empty set*/
PAIR('m','o'), 0333,	/*member of*/
PAIR('p','l'), 0334,	/*equation plus*/
PAIR('r','g'), 0335,	/*registered*/
PAIR('c','o'), 0336,	/*copyright*/
PAIR('b','r'), 0337,	/*box vert rule*/
PAIR('c','t'), 0340,	/*cent sign*/
PAIR('d','d'), 0341,	/*dbl dagger*/
PAIR('r','h'), 0342,	/*right hand*/
PAIR('l','h'), 0343,	/*left hand*/
PAIR('*','*'), 0344,	/*math * */
PAIR('b','s'), 0345,	/*bell system sign*/
PAIR('o','r'), 0346,	/*or*/
PAIR('c','i'), 0347,	/*circle*/
PAIR('l','t'), 0350,	/*left top (of big curly)*/
PAIR('l','b'), 0351,	/*left bottom*/
PAIR('r','t'), 0352,	/*right top*/
PAIR('r','b'), 0353,	/*right bot*/
PAIR('l','k'), 0354,	/*left center of big curly bracket*/
PAIR('r','k'), 0355,	/*right center of big curly bracket*/
PAIR('b','v'), 0356,	/*bold vertical*/
PAIR('l','f'), 0357,	/*left floor (left bot of big sq bract)*/
PAIR('r','f'), 0360,	/*right floor (rb of ")*/
PAIR('l','c'), 0361,	/*left ceiling (lt of ")*/
PAIR('r','c'), 0362,	/*right ceiling (rt of ")*/
0,0};

char codetab[256-32] = {	/*cat codes*/
00,	/*space*/
0145,	/*!*/
0230,	/*"*/
0337,	/*#*/
0155,	/*$*/
053,	/*%*/
050,	/*&*/
032,	/*' close*/
0132,	/*(*/
0133,	/*)*/
0122,	/***/
0143,	/*+*/
047,	/*,*/
040,	/*- hyphen*/
044,	/*.*/
043,	/*/*/
0110,	/*0*/
0111,	/*1*/
0112,	/*2*/
0113,	/*3*/
0114,	/*4*/
0115,	/*5*/
0116,	/*6*/
0117,	/*7*/
0120,	/*8*/
0121,	/*9*/
0142,	/*:*/
023,	/*;*/
0303,	/*<*/
0140,	/*=*/
0301,	/*>*/
0147,	/*?*/
0222,	/*@*/
0103,	/*A*/
075,	/*B*/
070,	/*C*/
074,	/*D*/
072,	/*E*/
0101,	/*F*/
065,	/*G*/
060,	/*H*/
066,	/*I*/
0105,	/*J*/
0107,	/*K*/
063,	/*L*/
062,	/*M*/
061,	/*N*/
057,	/*O*/
067,	/*P*/
055,	/*Q*/
064,	/*R*/
076,	/*S*/
056,	/*T*/
0106,	/*U*/
071,	/*V*/
0104,	/*W*/
0102,	/*X*/
077,	/*Y*/
073,	/*Z*/
0134,	/*[*/
0241,	/*\*/
0135,	/*]*/
0336,	/*^*/
0240,	/*_*/
030,	/*` open*/
025,	/*a*/
012,	/*b*/
027,	/*c*/
011,	/*d*/
031,	/*e*/
014,	/*f*/
045,	/*g*/
001,	/*h*/
006,	/*i*/
015,	/*j*/
017,	/*k*/
005,	/*l*/
004,	/*m*/
003,	/*n*/
033,	/*o*/
021,	/*p*/
042,	/*q*/
035,	/*r*/
010,	/*s*/
002,	/*t*/
016,	/*u*/
037,	/*v*/
041,	/*w*/
013,	/*x*/
051,	/*y*/
007,	/*z*/
0332,	/*{*/
0151,	/*|*/
0333,	/*}*/
0342,	/*~*/
00,	/*narrow space*/
040,	/*hyphen*/
0146,	/*bullet*/
0154,	/*square*/
022,	/*3/4 em*/
026,	/*rule*/
034,	/*1/4*/
036,	/*1/2*/
046,	/*3/4*/
0123,	/*minus*/
0124,	/*fi*/
0125,	/*fl*/
0126,	/*ff*/
0131,	/*ffi*/
0130,	/*ffl*/
0136,	/*degree*/
0137,	/*dagger*/
0355,	/*section*/
0150,	/*foot mark*/
0334,	/*acute accent*/
0335,	/*grave accent*/
0240,	/*underrule*/
0304,	/*slash (longer)*/
00,	/*half nar sp*/
00,	/**/
0225,	/*alpha*/
0212,	/*beta*/
0245,	/*gamma*/
0211,	/*delta*/
0231,	/*epsilon*/
0207,	/*zeta*/
0214,	/*eta*/
0202,	/*theta*/
0206,	/*iota*/
0217,	/*kappa*/
0205,	/*lambda*/
0204,	/*mu*/
0203,	/*nu*/
0213,	/*xi*/
0233,	/*omicron*/
0221,	/*pi*/
0235,	/*rho*/
0210,	/*sigma*/
0237,	/*tau*/
0216,	/*upsilon*/
0215,	/*phi*/
0227,	/*chi*/
0201,	/*psi*/
0251,	/*omega*/
0265,	/*Gamma*/
0274,	/*Delta*/
0256,	/*Theta*/
0263,	/*Lambda*/
0302,	/*Xi*/
0267,	/*Pi*/
0276,	/*Sigma*/
00,	/**/
0306,	/*Upsilon*/
0255,	/*Phi*/
0242,	/*Psi*/
0257,	/*Omega*/
0275,	/*square root*/
0262,	/*terminal sigma (was root em)*/
0261,	/*root en*/
0327,	/*>=*/
0326,	/*<=*/
0330,	/*identically equal*/
0264,	/*equation minus*/
0277,	/*approx =*/
0272,	/*approximates*/
0331,	/*not equal*/
0354,	/*right arrow*/
0234,	/*left arrow*/
0236,	/*up arrow*/
0223,	/*down arrow*/
0232,	/*equation equal*/
0323,	/*multiply*/
0324,	/*divide*/
0325,	/*plus-minus*/
0260,	/*cup (union)*/
0305,	/*cap (intersection)*/
0270,	/*subset of*/
0271,	/*superset of*/
0350,	/*improper subset*/
0246,	/* improper superset*/
0244,	/*infinity*/
0273,	/*partial derivative*/
0253,	/*gradient*/
0307,	/*not*/
0266,	/*integral sign*/
0247,	/*proportional to*/
0343,	/*empty set*/
0341,	/*member of*/
0353,	/*equation plus*/
0141,	/*registered*/
0153,	/*copyright*/
0346,	/*box rule (was parallel sign)*/
0127,	/*cent sign*/
0345,	/*dbl dagger*/
0250,	/*right hand*/
0340,	/*left hand*/
0347,	/*math * */
0243,	/*bell system sign*/
0226,	/*or (was star)*/
0351,	/*circle*/
0311,	/*left top (of big curly)*/
0314,	/*left bottom*/
0315,	/*right top*/
0317,	/*right bot*/
0313,	/*left center of big curly bracket*/
0316,	/*right center of big curly bracket*/
0312,	/*bold vertical*/
0321,	/*left floor (left bot of big sq bract)*/
0320,	/*right floor (rb of ")*/
0322,	/*left ceiling (lt of ")*/
0310};	/*right ceiling (rt of ")*/

/* modified for PostScript fonts */
char W1[256-32] = {	/* Times Roman widths */
12,		/* space */
12,		/* exclamation */
0,		/* NULL */
0,		/* NULL */
18,		/* dollar */
30,		/* percent */
28,		/* ampersand */
12,		/* close quote */
12,		/* left paren */
12,		/* right paren */
18,		/* asterisk */
20,		/* plus */
9,		/* comma */
12,		/* - hyphen */
9,		/* period */
10,		/* slash */
18+0200,	/* 0 */
18+0200,	/* 1 */
18+0200,	/* 2 */
18+0200,	/* 3 */
18+0200,	/* 4 */
18+0200,	/* 5 */
18+0200,	/* 6 */
18+0200,	/* 7 */
18+0200,	/* 8 */
18+0200,	/* 9 */
10,		/* colon */
10,		/* semicolon */
0,		/* NULL */
20,		/* equal */
0,		/* NULL */
16,		/* question */
0,		/* NULL */
26+0200,	/* A */
24+0200,	/* B */
24+0200,	/* C */
26+0200,	/* D */
22+0200,	/* E */
20+0200,	/* F */
26+0200,	/* G */
26+0200,	/* H */
12+0200,	/* I */
14+0200,	/* J */
26+0200,	/* K */
22+0200,	/* L */
32+0200,	/* M */
26+0200,	/* N */
26+0200,	/* O */
20+0200,	/* P */
26+0200,	/* Q */
24+0200,	/* R */
20+0200,	/* S */
22+0200,	/* T */
26+0200,	/* U */
26+0200,	/* V */
34+0200,	/* W */
26+0200,	/* X */
26+0200,	/* Y */
22+0200,	/* Z */
12,		/* left bracket */
0,		/* NULL */
12,		/* right bracket */
0,		/* NULL */
0,		/* NULL */
12,		/* open quote */
16,		/* a */
18+0200,	/* b */
16,		/* c */
18+0200,	/* d */
16,		/* e */
12+0200,	/* f */
18+0100,	/* g */
18+0200,	/* h */
10+0200,	/* i */
10+0300,	/* j */
18+0200,	/* k */
10+0200,	/* l */
28,		/* m */
18,		/* n */
18,		/* o */
18+0100,	/* p */
18+0100,	/* q */
12,		/* r */
14,		/* s */
10+0200,	/* t */
18,		/* u */
18,		/* v */
26,		/* w */
18,		/* x */
18+0100,	/* y */
16,		/* z */
0,		/* NULL */
7,		/* vert bar */
0,		/* NULL */
0,		/* NULL */
6,		/* narrow space */
12,		/* hyphen */
17,		/* bullet */
27,		/* square box */
36,		/* 3/4 em-dash */
18,		/* rule */
30,		/* 1/4 */
30,		/* 1/2 */
30,		/* 3/4 */
18,		/* minus sign(en) */
20,		/* fi */
20,		/* fl */
24,		/* ff */
32,		/* ffi */
32,		/* ffl */
14,		/* degree */
18,		/* dagger */
0,		/* NULL */
9,		/* foot mark */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
3,		/* half narrow space */
0,		/* null */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* null */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
28,		/* registered */
28,		/* copyright */
0,		/* NULL */
18,		/* cent */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
};

char W2[256-32] = {	/* Times Italic widths */
12,		/* ital space */
12,		/* ital exclamation */
0,		/* NULL */
0,		/* NULL */
18,		/* ital dollar */
30,		/* ital percent */
28,		/* ital ampersand */
12,		/* ital close quote */
12,		/* ital left paren */
12,		/* ital right paren */
18,		/* ital asterisk */
24,		/* ital plus */
9,		/* ital comma */
12,		/* ital - hyphen */
9,		/* ital period */
10,		/* ital slash (roman) */
18+0200,	/* ital 0 */
18+0200,	/* ital 1 */
18+0200,	/* ital 2 */
18+0200,	/* ital 3 */
18+0200,	/* ital 4 */
18+0200,	/* ital 5 */
18+0200,	/* ital 6 */
18+0200,	/* ital 7 */
18+0200,	/* ital 8 */
18+0200,	/* ital 9 */
12,		/* ital colon */
12,		/* ital semicolon */
0,		/* NULL */
24,		/* ital equal */
0,		/* NULL */
18,		/* ital question */
0,		/* NULL */
22+0200,	/* ital A */
22+0200,	/* ital B */
24+0200,	/* ital C */
26+0200,	/* ital D */
22+0200,	/* ital E */
22+0200,	/* ital F */
26+0200,	/* ital G */
26+0200,	/* ital H */
12+0200,	/* ital I */
16+0200,	/* ital J */
24+0200,	/* ital K */
20+0200,	/* ital L */
30+0200,	/* ital M */
24+0200,	/* ital N */
26+0200,	/* ital O */
22+0200,	/* ital P */
26+0200,	/* ital Q */
22+0200,	/* ital R */
18+0200,	/* ital S */
20+0200,	/* ital T */
26+0200,	/* ital U */
22+0200,	/* ital V */
30+0200,	/* ital W */
22+0200,	/* ital X */
20+0200,	/* ital Y */
20+0200,	/* ital Z */
14,		/* ital left bracket */
0,		/* NULL */
14,		/* ital right bracket */
0,		/* NULL */
0,		/* NULL */
12,		/* ital open quote */
18,		/* ital a */
18+0200,	/* ital b */
16,		/* ital c */
18+0200,	/* ital d */
16,		/* ital e */
10+0200,	/* ital f */
18+0100,	/* ital g */
18+0200,	/* ital h */
10+0200,	/* ital i */
10+0300,	/* ital j */
16+0200,	/* ital k */
10+0200,	/* ital l */
26,		/* ital m */
18,		/* ital n */
18,		/* ital o */
18+0100,	/* ital p */
18+0100,	/* ital q */
14,		/* ital r */
14,		/* ital s */
10+0200,	/* ital t */
18,		/* ital u */
16,		/* ital v */
24,		/* ital w */
16,		/* ital x */
16+0100,	/* ital y */
14,		/* ital z */
0,		/* NULL */
7,		/* ital vert bar (roman) */
0,		/* NULL */
0,		/* NULL */
6,		/* ital narrow space */
12,		/* ital hyphen */
17,		/* ital bullet */
27,		/* ital square box */
32,		/* ital 3/4 em-dash */
18,		/* ital rule */
30,		/* ital 1/4 */
30,		/* ital 1/2 */
30,		/* ital 3/4 */
18,		/* ital minus sign */
18,		/* ital fi */
18,		/* ital fl */
20,		/* ital ff */
28,		/* ital ffi */
28,		/* ital ffl */
14,		/* ital degree */
18,		/* ital dagger */
0,		/* NULL */
9,		/* ital foot mark */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
3,		/* ital half narrow space */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
28,		/* ital registered */
28,		/* ital copyright */
0,		/* NULL */
18,		/* ital cent */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
};

char W3[256-32] = {	/* Times Bold widths */
12,		/* bold space */
12,		/* bold exclamation */
0,		/* NULL */
0,		/* NULL */
18,		/* bold dollar */
36,		/* bold percent */
30,		/* bold ampersand */
12,		/* bold close quote */
12,		/* bold left paren */
12,		/* bold right paren */
18,		/* bold asterisk */
21,		/* bold plus */
9,		/* bold comma */
12,		/* bold - hyphen */
9,		/* bold period */
10,		/* bold slash */
18+0200,	/* bold 0 */
18+0200,	/* bold 1 */
18+0200,	/* bold 2 */
18+0200,	/* bold 3 */
18+0200,	/* bold 4 */
18+0200,	/* bold 5 */
18+0200,	/* bold 6 */
18+0200,	/* bold 7 */
18+0200,	/* bold 8 */
18+0200,	/* bold 9 */
12,		/* bold colon */
12,		/* bold semicolon */
0,		/* NULL */
21,		/* bold equal */
0,		/* NULL */
18,		/* bold question */
0,		/* NULL */
26+0200,	/* bold A */
24+0200,	/* bold B */
26+0200,	/* bold C */
26+0200,	/* bold D */
24+0200,	/* bold E */
22+0200,	/* bold F */
28+0200,	/* bold G */
28+0200,	/* bold H */
14+0200,	/* bold I */
18+0200,	/* bold J */
28+0200,	/* bold K */
24+0200,	/* bold L */
34+0200,	/* bold M */
26+0200,	/* bold N */
28+0200,	/* bold O */
22+0200,	/* bold P */
28+0200,	/* bold Q */
26+0200,	/* bold R */
20+0200,	/* bold S */
24+0200,	/* bold T */
26+0200,	/* bold U */
26+0200,	/* bold V */
36+0200,	/* bold W */
26+0200,	/* bold X */
26+0200,	/* bold Y */
24+0200,	/* bold Z */
12,		/* bold left bracket */
0,		/* NULL */
12,		/* bold right bracket */
0,		/* NULL */
0,		/* NULL */
12,		/* bold open quote */
18,		/* bold a */
20+0200,	/* bold b */
16,		/* bold c */
20+0200,	/* bold d */
16,		/* bold e */
12+0200,	/* bold f */
18+0100,	/* bold g */
20+0200,	/* bold h */
10+0200,	/* bold i */
12+0300,	/* bold j */
20+0200,	/* bold k */
10+0200,	/* bold l */
30,		/* bold m */
20,		/* bold n */
18,		/* bold o */
20+0100,	/* bold p */
20+0100,	/* bold q */
16,		/* bold r */
14,		/* bold s */
12+0200,	/* bold t */
20,		/* bold u */
18,		/* bold v */
26,		/* bold w */
18,		/* bold x */
18+0100,	/* bold y */
16,		/* bold z */
0,		/* NULL */
8,		/* bold vert bar */
0,		/* NULL */
0,		/* NULL */
6,		/* bold narrow space */
12,		/* bold hyphen */
17,		/* bold bullet */
27,		/* bold square box */
36,		/* bold 3/4 em-dash */
18,		/* bold rule */
30,		/* bold 1/4 */
30,		/* bold 1/2 */
30,		/* bold 3/4 */
18,		/* bold minus sign */
20,		/* bold fi */
20,		/* bold fl */
24,		/* bold ff */
32,		/* bold ffi */
32,		/* bold ffl */
14,		/* bold degree */
18,		/* bold dagger */
0,		/* NULL */
9,		/* bold foot mark */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
3,		/* bold half narrow space */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
28,		/* bold registered */
28,		/* bold copyright */
0,		/* NULL */
18,		/* bold cent */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
};

char W4[256-32] = {	/*Special font widths*/
0,		/* NULL */
0,		/* NULL */
15,		/* double quote */
18,		/* number */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
20,		/* less than */
0,		/* NULL */
20,		/* greater than */
0,		/* NULL */
33,		/* at */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
10,		/* backslash */
0,		/* NULL */
12,		/* circumflex */
18,		/* underscore */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
17,		/* left brace */
0,		/* NULL */
17,		/* right brace */
18,		/* tilde */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
18,		/* section */
0,		/* NULL */
12,		/* acute accent */
12,		/* grave accent */
18,		/* underrule */
10,		/* long slash */
0,		/* NULL */
0,		/* NULL */
23,		/* alpha */
20+0300,	/* beta */
15+0200,	/* gamma */
18+0200,	/* delta */
16,		/* epsilon */
18+0300,	/* zeta */
22+0100,	/* eta */
19+0200,	/* theta */
12,		/* iota */
20,		/* kappa */
20+0200,	/* lambda */
21+0100,	/* mu */
19,		/* nu */
18+0300,	/* xi */
20,		/* omicron */
20,		/* pi */
20+0100,	/* rho */
22,		/* sigma */
16,		/* tau */
21,		/* upsilon */
19+0300,	/* phi */
20+0100,	/* chi */
25+0300,	/* psi */
25,		/* omega */
22+0200,	/* Gamma */
22+0200,	/* Delta */
27+0200,	/* Theta */
25+0200,	/* Lambda */
23+0200,	/* Xi */
28+0200,	/* Pi */
21+0200,	/* Sigma */
0,		/* NULL */
25+0200,	/* Upsilon */
27+0200,	/* Phi */
29+0200,	/* Psi */
28+0200,	/* Omega */
20,		/* square root */
16+0100,	/* terminal sigma */
18,		/* root en */
20,		/* greater or equal */
20,		/* less or equal */
20,		/* identically equal */
20,		/* equasion minus */
20,		/* approx equal */
20,		/* approximates */
20,		/* not equal */
36,		/* right arrow */
36,		/* left arrow */
22,		/* up arrow */
22,		/* down arrow */
20,		/* equation equal */
20,		/* multiply */
20,		/* divide */
20,		/* plus-minus */
28,		/* cup (union) */
28,		/* cap (intersection) */
26,		/* subset of */
26,		/* superset of */
26,		/* improper subset of */
26,		/* improper superset of */
26,		/* infinity */
18,		/* partial derivative */
26+0200,	/* gradient */
26,		/* logical not */
10,		/* integral */
26,		/* proportional to */
30,		/* empty set */
26,		/* member of */
20,		/* equasion plus */
0,		/* NULL */
0,		/* NULL */
3,		/* box rule */
0,		/* NULL */
18,		/* double dagger */
36,		/* right hand (arrow) */
36,		/* left hand (arrow) */
18,		/* math star */
28,		/* ?bell? logo */
7,		/* or */
27,		/* circle */
15,		/* lt of big curly */
15,		/* lb of big curly */
15,		/* rt of big curly */
15,		/* rb of big curly */
15,		/* lc of big curly */
15,		/* rc of big curly */
15,		/* bold vertical */
15,		/* left floor */
15,		/* right floor */
15,		/* left ceiling */
15,		/* right ceiling */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
0,		/* NULL */
};
