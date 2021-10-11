#ifndef lint
static	char sccsid[] = "@(#)conf.c 1.1 92/07/30 SMI";
#endif
/*
 * Copyright (c) 1987-1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/acct.h>
#include <sys/stream.h>

extern int nulldev();
extern int nodev();

#ifdef VDDRV
extern int vd_unuseddev();
extern vdopen(), vdclose(), vdioctl();
#else
#define	vd_unuseddev nodev
#define	vdopen  nodev
#define	vdclose nodev
#define	vdioctl nodev
#endif

#include "xy.h"
#if NXY > 0
extern int xyopen(), xystrategy(), xyread(), xywrite();
extern int xydump(), xyioctl(), xysize();
#else
#define	xyopen		nodev
#define	xystrategy	nodev
#define	xyread		nodev
#define	xywrite		nodev
#define	xydump		nodev
#define	xyioctl		nodev
#define	xysize		0
#endif

#include "hrc.h"
#if NHRC > 0
extern int hrcopen(), hrcclose(), hrcmmap(), hrcioctl(), hrcread();
#else
#define	hrcopen		nodev
#define	hrcclose	nodev
#define	hrcmmap		nodev
#define	hrcioctl 	nodev
#define	hrcread 	nodev
#endif

#include "xd.h"
#if NXD > 0
extern int xdopen(), xdstrategy(), xdread(), xdwrite();
extern int xddump(), xdioctl(), xdsize();
#else
#define	xdopen		nodev
#define	xdstrategy	nodev
#define	xdread		nodev
#define	xdwrite		nodev
#define	xddump		nodev
#define	xdioctl		nodev
#define	xdsize		0
#endif

#include "st.h"
#if NST > 0
extern int stopen(), stclose(), stread(), stwrite(), stioctl(), ststrategy();
#else
#define	stopen		nodev
#define	stclose		nodev
#define	stread		nodev
#define	stwrite		nodev
#define	stioctl		nodev
#define	ststrategy	nodev
#endif

#include "ns.h"
#if NNS > 0
extern int
	nsopen(),
	nsread(),
	nswrite(),
	nssize(),
	nsioctl(),
	nsstrategy();
#else
#define	nsopen		nodev
#define	nsclose		nodev
#define	nsread		nodev
#define	nswrite		nodev
#define	nssize		0
#define	nsioctl		nodev
#define	nsstrategy	nodev
#endif

#include "rd.h"
#if NRD > 0
extern int
	rdopen(),
	rdread(),
	rdwrite(),
	rdsize(),
	rdioctl(),
	rdstrategy();
#else
#define	rdopen		nodev
#define	rdclose		nodev
#define	rdread		nodev
#define	rdwrite		nodev
#define	rdsize		0
#define	rdioctl		nodev
#define	rdstrategy	nodev
#endif

#include "ft.h"
#if NFT > 0
extern int
	ftopen(),
	ftclose(),
	ftread(),
	ftwrite(),
	ftsize(),
	ftioctl(),
	ftstrategy();
#else
#define	ftopen		nodev
#define	ftclose		nodev
#define	ftread		nodev
#define	ftwrite		nodev
#define	ftsize		0
#define	ftioctl		nodev
#define	ftstrategy	nodev
#endif

#include "mt.h"
#if NMT > 0
extern int tmopen(), tmclose(), tmstrategy(), tmread(), tmwrite();
extern int tmdump(), tmioctl();
#else
#define	tmopen		nodev
#define	tmclose		nodev
#define	tmstrategy	nodev
#define	tmread		nodev
#define	tmwrite		nodev
#define	tmdump		nodev
#define	tmioctl		nodev
#endif

#include "xt.h"
#if NXT > 0
extern int xtopen(), xtclose(), xtstrategy(), xtread(), xtwrite(), xtioctl();
#else
#define	xtopen		nodev
#define	xtclose		nodev
#define	xtstrategy	nodev
#define	xtread		nodev
#define	xtwrite		nodev
#define	xtioctl		nodev
#endif

#include "ar.h"
#if NAR > 0
extern int aropen(), arclose(), arstrategy(), arread(), arwrite(), arioctl();
#else
#define	aropen		nodev
#define	arclose		nodev
#define	arstrategy	nodev
#define	arread		nodev
#define	arwrite		nodev
#define	arioctl		nodev
#endif

#include "sd.h"
#if NSD > 0
extern int sdopen(), sdclose(), sdstrategy(), sdread(), sdwrite();
extern int sddump(), sdioctl(), sdsize();
#else
#define	sdopen		nodev
#define	sdclose		nodev
#define	sdstrategy	nodev
#define	sdread		nodev
#define	sdwrite		nodev
#define	sddump		nodev
#define	sdioctl		nodev
#define	sdsize		0
#endif

#include "hd.h"
#if NHD > 0
extern int hdopen(), hdstrategy(), hdread(), hdwrite();
extern int hddump(), hdioctl(), hdsize();
#else
#define	hdopen		nodev
#define	hdstrategy	nodev
#define	hdread		nodev
#define	hdwrite		nodev
#define	hddump		nodev
#define	hdioctl		nodev
#define	hdsize		0
#endif

#include "fd.h"
#if NFD > 0
extern int fdopen(), fdclose(), fdread(), fdstrategy(), fdwrite();
extern int fddump(), fdioctl(), fdsize();
#else
#define	fdopen		nodev
#define	fdclose		nodev
#define	fdstrategy	nodev
#define	fdread		nodev
#define	fdwrite		nodev
#define	fddump		nodev
#define	fdioctl		nodev
#define	fdsize		0
#endif

#include "id.h"
#if NID > 0
extern int idcopen(), idbopen(), idstrategy(), idread(), idwrite();
extern int iddump(), idioctl(), idsize();
#else
#define	idbopen		nodev
#define	idcopen		nodev
#define	idstrategy	nodev
#define	idread		nodev
#define	idwrite		nodev
#define	iddump		nodev
#define	idioctl		nodev
#define	idsize		0
#endif

#include "tvone.h"
#if NTVONE > 0
extern int tvoneopen(), tvoneclose(), tvonemmap(), tvoneioctl();
#else
#define	tvoneopen   nodev
#define	tvoneclose  nodev
#define	tvoneioctl  nodev
#define	tvonemmap   nodev
#endif

#include "sr.h"
#if NSR > 0
int	sropen(), srclose(), srread(), srioctl(), srstrategy(), srsize();
#else
#define	sropen		nodev
#define	srclose		nodev
#define	srread		nodev
#define	srioctl		nodev
#define	srstrategy	nodev
#define	srsize		nodev
#endif

#include "vx.h"
#if NVX > 0
extern int vxopen(), vxclose();
extern int vxread(), vxwrite();
extern int vxioctl(), vxmmap();
extern int vxsegmap();
#else
#define	vxopen		nodev
#define	vxclose		nodev
#define	vxread		nodev
#define	vxwrite		nodev
#define	vxioctl		nodev
#define	vxmmap		nodev
#define	vxsegmap	nodev
#endif


struct bdevsw	bdevsw[] =
{
	{ nodev,	nodev,		nodev,		nodev,		/*0*/
	  0,		0 },					/* was ip */
	{ tmopen,	tmclose,	tmstrategy,	tmdump,		/*1*/
	  0,		B_TAPE },
	{ nodev,	nodev,		nodev,		nodev,		/*2*/
	  0,		B_TAPE },				/* was ar */
	{ xyopen,	nulldev,	xystrategy,	xydump,		/*3*/
	  xysize,	0 },
	{ nodev,	nodev,		nodev,		nodev,		/*4*/
	  0,		0 },					/* was sw */
	{ nodev,	nodev,		nodev,		nodev,		/*5*/
	  nodev,	0 },					/* was nd */
	{ nodev,	nodev,		nodev,		nodev,		/*6*/
	  0,		0 },
	{ sdopen,	sdclose,	sdstrategy,	sddump,		/*7*/
	  sdsize,	0 },
	{ xtopen,	xtclose,	xtstrategy,	nodev,		/*8*/
	  0,		B_TAPE },
	{ nodev,	nodev,		nodev,		nodev,		/*9*/
	  nodev,	0 },					/* was sf */
	{ xdopen,	nulldev,	xdstrategy,	xddump,		/*10*/
	  xdsize,	0 },
	{ stopen,	stclose,	ststrategy,	nodev,		/*11*/
	  0,		B_TAPE },
	{ nsopen,	nodev,		nsstrategy,	nodev,		/*12*/
	  nssize,	0 },
	{ rdopen,	nodev,		rdstrategy,	nodev,		/*13*/
	  rdsize,	0 },
	{ ftopen,	ftclose,	ftstrategy,	nodev,		/*14*/
	  ftsize,	0 },
	{ hdopen,	nulldev,	hdstrategy,	hddump,		/*15*/
	  hdsize,	0 },
	{ fdopen,	fdclose,	fdstrategy,	fddump,		/*16*/
	  fdsize,	0 },
	{ vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	/*17*/
	  0,		0 },
	{ sropen,	srclose,	srstrategy,	nodev,		/*18*/
	  srsize,	0 },
	{ vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	/*19*/
	  0,		0 },
	{ vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	/*20*/
	  0,		0 },
	{ vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	/*21*/
	  0,		0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*22*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*23*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*24*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*25*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*26*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*27*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*28*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*29*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*30*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*31*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*32*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*33*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*34*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*35*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*36*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*37*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*38*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*39*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*40*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*41*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*42*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*43*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*44*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*45*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*46*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*47*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*48*/
	  idsize,	0 },
	{ idbopen,	nulldev,	idstrategy,	iddump,		/*49*/
	  idsize,	0 },
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

extern int swread(), swwrite();

extern int cnopen(), cnclose(), cnread(), cnwrite(), cnioctl(), cnselect();

extern struct streamtab wcinfo;

extern struct streamtab conskbd_info;

extern int consfbopen(), consfbclose(), consfbioctl(), consfbmmap();

extern int spec_segmap();

#include "ms.h"
#if NMS > 0
extern struct streamtab consms_info;
#define	consmstab	&consms_info
#else
#define	consmstab	0
#endif

extern int syopen(), syread(), sywrite(), syioctl(), syselect();

extern int mmopen(), mmread(), mmwrite(), mmioctl(), mmmmap(), mmsegmap();
#define	mmselect	seltrue

#include "vp.h"
#if NVP > 0
extern int vpopen(), vpclose(), vpwrite(), vpioctl();
#else
#define	vpopen		nodev
#define	vpclose		nodev
#define	vpwrite		nodev
#define	vpioctl		nodev
#endif

#include "vpc.h"
#if NVPC > 0
extern int vpcopen(), vpcclose(), vpcwrite(), vpcioctl();
#else
#define	vpcopen		nodev
#define	vpcclose	nodev
#define	vpcwrite	nodev
#define	vpcioctl	nodev
#endif

#include "zs.h"
#if NZS > 0
extern struct streamtab zsstab;
#define	zstab	&zsstab
#else
#define	zstab	0
#endif

#include "pty.h"
#if NPTY > 0
extern struct streamtab ptsinfo;
extern int ptcopen(), ptcclose(), ptcread(), ptcwrite(), ptcioctl();
extern int ptcselect();
#define	ptstab	&ptsinfo
#else
#define	ptstab	0
#define	ptcopen		nodev
#define	ptcclose	nodev
#define	ptcread		nodev
#define	ptcwrite	nodev
#define	ptcioctl	nodev
#define	ptcselect	nodev
#endif

#include "mti.h"
#if NMTI > 0
extern int mtireset();
extern struct streamtab mtistab;
#define	mtitab		&mtistab
#else
#define	mtireset	nodev
#define	mtitab		0
#endif

#include "mcpa.h"
#if NMCPA > 0
extern struct streamtab mcpstab;
extern struct streamtab mcppstab;
#define	mcptab		&mcpstab
#define	mcpptab		&mcppstab
#else
#define	mcptab		0
#define	mcpptab		0
#endif

#include "cgtwo.h"
#if NCGTWO > 0
extern int cgtwoopen(), cgtwommap(), cgtwoioctl();
extern int cgtwoclose();
#else
#define	cgtwoopen	nodev
#define	cgtwommap	nodev
#define	cgtwoioctl	nodev
#define	cgtwoclose	nodev
#endif


#include "cgfour.h"
#if NCGFOUR > 0
extern int cgfouropen(), cgfourclose(), cgfourioctl(), cgfourmmap();
#else
#define	cgfouropen	nodev
#define	cgfourclose	nodev
#define	cgfourioctl	nodev
#define	cgfourmmap	nodev
#endif

#include "gpone.h"
#if NGPONE > 0
extern int gponeopen(), gponemmap(), gponeioctl();
extern int gponeclose();
#else
#define	gponeopen	nodev
#define	gponemmap	nodev
#define	gponeioctl	nodev
#define	gponeclose	nodev
#endif

#include "win.h"
#if NWIN > 0
extern int winopen(), winclose(), winread(), winwrite();
extern int winioctl(), winmmap(), winselect(), winsegmap();
#else
#define	winopen		nodev
#define	winclose	nodev
#define	winread		nodev
#define	winwrite	nodev
#define	winioctl	nodev
#define	winmmap		nodev
#define	winselect	nodev
#define	winsegmap	0
#endif

int	logopen(), logclose(), logread(), logioctl(), logselect();

#include "pi.h"
#if NPI > 0
extern struct streamtab piinfo;
#define	pitab	&piinfo
#else
#define	pitab	0
#endif

#include "bwtwo.h"
#if NBWTWO > 0
extern int bwtwoopen(), bwtwommap(), bwtwoioctl();
extern int bwtwoclose();
#else
#define	bwtwoopen	nodev
#define	bwtwommap	nodev
#define	bwtwoioctl	nodev
#define	bwtwoclose	nodev
#endif

#include "des.h"
#if NDES > 0
extern int desopen(), desclose(), desioctl();
#else
#define	desopen		nodev
#define	desclose	nodev
#define	desioctl	nodev
#endif

#ifdef FPU
#include "fpa.h"
#endif FPU
#if NFPA > 0
extern int fpaopen(), fpaclose(), fpaioctl();
#else
#define	fpaopen		nodev
#define	fpaclose	nodev
#define	fpaioctl	nodev
#endif

#include "sp.h"
#if NSP > 0
extern struct streamtab spinfo;
#define	sptab	&spinfo
#else
#define	sptab	0
#endif

#include "clone.h"
#if NCLONE > 0
extern int cloneopen();
#else
#define	cloneopen	nodev
#endif

#include "pc.h"
#if NPC > 0
extern int pcopen(), pcclose(), pcioctl(), pcselect(), pcmmap();
#else
#define	pcopen		nodev
#define	pcclose		nodev
#define	pcioctl		nodev
#define	pcselect	nodev
#define	pcmmap		nodev
#endif

extern int dumpopen(), dumpread(), dumpwrite();

/* Sunlink device driver entries */
#include "ifd.h"
#if NIFD > 0
int ifdopen(), ifdclose(), ifdread(), ifdwrite(), ifdioctl(), ifdselect();
#else
#define	ifdopen		nodev
#define	ifdclose	nodev
#define	ifdread		nodev
#define	ifdwrite	nodev
#define	ifdioctl	nodev
#define	ifdselect	nodev
#endif

#include "dcp.h"
#if NDCP > 0
int dcpopen(), dcpclose(), dcpread(), dcpwrite(), dcpdioctl(), dcpselect();
#else
#define	dcpopen		nodev
#define	dcpclose	nodev
#define	dcpread		nodev
#define	dcpwrite	nodev
#define	dcpdioctl	nodev
#define	dcpselect	nodev
#endif

#include "dnalink.h"
#if NDNALINK > 0
int dnaopen(), dnaclose(), dnaread(), dnawrite(), dnaioctl(), dnaselect();
#else
#define	dnaopen		nodev
#define	dnaclose	nodev
#define	dnaread		nodev
#define	dnawrite	nodev
#define	dnaioctl	nodev
#define	dnaselect	nodev
#endif

#include "tbi.h"
#if NTBI > 0
int tbiopen(), tbiclose(), tbiread(), tbiwrite(), tbidioctl();
#else
#define	tbiopen		nodev
#define	tbiclose	nodev
#define	tbiread		nodev
#define	tbiwrite	nodev
#define	tbidioctl	nodev
#endif

#include "chat.h"
#if NCHAT > 0
int chatopen(), chatclose(), chatread(), chatwrite();
int chatioctl(), chatselect();
int chutopen(), chutclose(), chutread(), chutwrite();
int chutioctl(), chutselect();
#else
#define	chatopen	nodev
#define	chatclose	nodev
#define	chatread	nodev
#define	chatwrite	nodev
#define	chatioctl	nodev
#define	chatselect	nodev
#define	chutopen	nodev
#define	chutclose	nodev
#define	chutread	nodev
#define	chutwrite	nodev
#define	chutioctl	nodev
#define	chutselect	nodev
#endif

/* bpp device */
#include "bpp.h"
#if NBPP > 0
extern int bpp_open(), bpp_close(), bpp_read(), bpp_write();
extern int bpp_ioctl();
#else
#define bpp_open        nodev
#define bpp_close       nodev
#define bpp_read        nodev
#define bpp_write       nodev
#define bpp_ioctl       nodev
#endif

/* streams NIT device */
#include "snit.h"
#if	NSNIT > 0
extern struct streamtab	snit_info;
#define	snittab	&snit_info
#else	NSNIT > 0
#define	snittab	0
#endif	NSNIT > 0

#include "cgthree.h"
#if NCGTHREE > 0
extern int cgthreeopen(), cgthreeclose(), cgthreemmap(), cgthreeioctl();
#else
#define	cgthreeopen	nodev
#define	cgthreeclose	nodev
#define	cgthreeioctl	nodev
#define	cgthreemmap	nodev
#endif

#include "cgeight.h"
#if NCGEIGHT > 0
extern int cgeightopen(), cgeightclose(), cgeightmmap(), cgeightioctl();
#else
#define	cgeightopen	nodev
#define	cgeightclose	nodev
#define	cgeightioctl	nodev
#define	cgeightmmap	nodev
#endif

#include "cgsix.h"
#if NCGSIX > 0
extern int cgsixopen(), cgsixclose(), cgsixmmap(), cgsixioctl();
extern int cgsixsegmap();
#else
#define	cgsixopen	nodev
#define	cgsixclose	nodev
#define	cgsixioctl	nodev
#define	cgsixmmap	nodev
#define	cgsixsegmap	nodev
#endif

#include "cgnine.h"
#if NCGNINE > 0
extern int cgnineopen(), cgnineclose(), cgninemmap(), cgnineioctl();
#else
#define	cgnineopen	nodev
#define	cgnineclose	nodev
#define	cgnineioctl	nodev
#define	cgninemmap	nodev
#endif

#include "pp.h"
#if NPP > 0
extern int ppopen(), ppclose(), ppwrite(), ppioctl();
#else
#define	ppopen		nodev
#define	ppclose		nodev
#define	ppwrite		nodev
#define	ppioctl		nodev
#endif

#include "taac.h"
#if NTAAC > 0
extern int taacopen(), taacmmap();
extern int taacclose(), taacioctl(), taacread(), taacwrite();
#else
#define	taacopen	nodev
#define	taacmmap	nodev
#define	taacclose	nodev
#define	taacioctl	nodev
#define	taacread	nodev
#define	taacwrite	nodev
#endif

/* TCP stream head */
#include "tcptli.h"
#if NTCPTLI > 0
extern struct streamtab tcptli_info;
#define	tcptlitab   &tcptli_info
#else  NTCPTLI > 0
#define	tcptlitab   0
#endif NTCPTLI > 0

#include "audioamd.h"
#if NAUDIOAMD > 0
extern struct streamtab audio_79C30_info;
#define	audio_79C30tab	&audio_79C30_info
#else
#define	audio_79C30tab	0
#endif

#include "dbri.h"
#if NDBRI > 0
extern struct streamtab dbri_info;
#define	dbri_tab	&dbri_info
#else
#define	dbri_tab	0
#endif

#if NDBRI > 0 || NAUDIOAMD > 0
extern int acloneopen();
#else
#define acloneopen      nodev
#endif NDBRI || NAUDIOAMD

#ifdef  OPENPROMS
#include "openeepr.h"
#else
#define	NOPENEEPR 0
#endif
#if NOPENEEPR > 0
extern int opromopen(), opromioctl();
#else
#define	opromopen	nodev
#define	opromioctl	nodev
#endif

#include "sg.h"
#if	NSG > 0
int sgopen(), sgclose(), sgioctl(), sgread(), sgwrite();
#else	NSG > 0
#define	sgopen	nodev
#define	sgclose	nodev
#define	sgioctl	nodev
#define	sgread	nodev
#define	sgwrite	nodev
#endif

#include "cgtwelve.h"
#if NCGTWELVE > 0
extern int cgtwelveopen(),  cgtwelveclose(), cgtwelvemmap();
extern int cgtwelveioctl(), cgtwelvesegmap();
#else
#define	cgtwelveopen   nodev
#define	cgtwelveclose  nodev
#define	cgtwelvemmap   nodev
#define	cgtwelveioctl  nodev
#define	cgtwelvesegmap nodev
#endif

#include "gt.h" 
#if NGT > 0
extern int gtopen(),  gtclose(), gtmmap();
extern int gtioctl(), gtsegmap();
extern struct streamtab lightpenptab;
#define lightpentab   &lightpenptab
#else
#define	gtopen   nodev
#define	gtclose  nodev
#define	gtmmap   nodev
#define	gtioctl  nodev
#define	gtsegmap nodev
#define lightpentab   0
#endif

extern int seltrue();

struct cdevsw	cdevsw[] =
{
    {
	cnopen,		cnclose,	cnread,		cnwrite,	/*0*/
	cnioctl,	nulldev,	cnselect,	0,
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*1*/
	nodev,		nodev,		nodev,		0,
	&wcinfo,	0,
    },
    {
	syopen,		nulldev,	syread,		sywrite,	/*2*/
	syioctl,	nulldev,	syselect,	0,
	0,		0,
    },
    {
	mmopen,		nulldev,	mmread,		mmwrite,	/*3*/
	mmioctl,	nulldev,	mmselect,	mmmmap,
	0,		mmsegmap,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*4*/
	nodev,		nodev,		nodev,		0,	/* was ip */
	0,		0,
    },
    {
	tmopen,		tmclose,	tmread,		tmwrite,	/*5*/
	tmioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	vpopen,		vpclose,	nodev,		vpwrite,	/*6*/
	vpioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	nulldev,	nulldev,	swread,		swwrite,	/*7*/
	nodev,		nulldev,	nodev,		0,
	0,		0,
    },
    {
	aropen,		arclose,	arread,		arwrite,	/*8*/
	arioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	xyopen,		nulldev,	xyread,		xywrite,	/*9*/
	xyioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*10*/
	nodev,		mtireset,	nodev,		0,
	mtitab,		0,
    },
    {
	desopen,	desclose,	nodev,		nodev,		/*11*/
	desioctl,	nulldev,	nodev,		0,
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*12*/
	nodev,		nodev,		nodev,		0,
	zstab,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*13*/
	nodev,		nulldev,	nodev,		0,
	consmstab,	0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*14*/
	nodev,		nodev,		nodev,		0,	/* was cgone */
	0,		0,
    },
    {
	winopen,	winclose,	winread,	winwrite,	/*15*/
	winioctl,	nodev,		winselect,	winmmap,
	0,		winsegmap,
    },
    {
	logopen,	logclose,	logread,	nodev,		/*16*/
	logioctl,	nulldev,	logselect,	0,
	0,		0,
    },
    {
	sdopen,		sdclose,	sdread,		sdwrite,	/*17*/
	sdioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	stopen,		stclose,	stread,		stwrite,	/*18*/
	stioctl,	nodev,		nodev,		0,
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*19*/
	nodev,		nodev,		nodev,		0,	/* was nd */
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*20*/
	nodev,		nodev,		nodev,		0,
	ptstab,		0,
    },
    {
	ptcopen,	ptcclose,	ptcread,	ptcwrite,	/*21*/
	ptcioctl,	nulldev,	ptcselect,	0,
	0,		0,
    },
    {
	consfbopen,	consfbclose,	nodev,		nodev,		/*22*/
	consfbioctl,	nodev,		nodev,		consfbmmap,
	0,		spec_segmap,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*23*/
	nodev,		nodev,		nodev,		0,	/* was ropc */
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*24*/
	nodev,		nodev,		nodev,		0,	/* was sky */
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*25*/
	nodev,		nodev,		nodev,		0,
	pitab,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*26*/
	nodev,		nodev,		nodev,		0,	/* was bwone */
	0,		0,
    },
    {
	bwtwoopen,	bwtwoclose,	nodev,		nodev,		/*27*/
	bwtwoioctl,	nodev,		seltrue,	bwtwommap,
	0,		spec_segmap,
    },
    {
	vpcopen,	vpcclose,	nodev,		vpcwrite,	/*28*/
	vpcioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*29*/
	nodev,		nulldev,	nodev,		0,
	&conskbd_info,	0,
    },
    {
	xtopen,		xtclose,	xtread,		xtwrite,	/*30*/
	xtioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	cgtwoopen,	cgtwoclose,	nodev,		nodev,		/*31*/
	cgtwoioctl,	nodev,		seltrue,	cgtwommap,
	0,		spec_segmap,
    },
    {
	gponeopen,	gponeclose,	nodev,		nodev,		/*32*/
	gponeioctl,	nodev,		seltrue,	gponemmap,
	0,		spec_segmap,
    },
    {
	nodev,		nodev,		nodev,		nodev,	/*33*/
	nodev,		nodev,		nodev,		0, /* was sf */
	0,		0,
    },
    {
	fpaopen,	fpaclose,	nodev,		nodev,		/*34*/
	fpaioctl,	nodev,		nodev,		0,
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*35*/
	nodev,		nodev,		nodev,		0,
	sptab,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*36*/
	nodev,		nodev,		nodev,		0,
	0,		0,
    },
    {
	cloneopen,	nodev,		nodev,		nodev,		/*37*/
	nodev,		nodev,		nodev,		0,
	0,		0,
    },
    {
	pcopen,		pcclose,	nodev,		nodev,		/*38*/
	pcioctl,	nodev,		pcselect,	pcmmap,
	0,		spec_segmap,
    },
    {
	cgfouropen,	cgfourclose,	nodev,		nodev,		/*39*/
	cgfourioctl,	nodev,		seltrue,	cgfourmmap,
	0,		spec_segmap,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*40*/
	nodev,		nodev,		nodev,		0,
	snittab,	0,
    },
    {
	dumpopen,	nulldev,	dumpread,	dumpwrite,	/*41*/
	nodev,		nulldev,	nodev,		0,
	0,		0,
    },
    {
	xdopen,		nulldev,	xdread,		xdwrite,	/*42*/
	xdioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	hrcopen,	hrcclose,	hrcread,	nodev,		/*43*/
	hrcioctl,	nulldev, 	nodev,		hrcmmap,
	0,		spec_segmap,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*44*/
	nodev,		nodev,		nodev,		0,
	mcptab,		0,
    },
    {
	ifdopen,	ifdclose,	ifdread,	ifdwrite,	/*45*/
	ifdioctl,	nulldev,	ifdselect,	0,
	0,		0,
    },
    {
	dcpopen,	dcpclose,	dcpread,	dcpwrite,	/*46*/
	dcpdioctl,	nulldev,	dcpselect,	0,
	0,		0,
    },
    {
	dnaopen,	dnaclose,	dnaread,	dnawrite,	/*47*/
	dnaioctl,	nulldev,	dnaselect,	0,
	0,		0,
    },
    {
	tbiopen,	tbiclose,	tbiread,	tbiwrite,	/*48*/
	tbidioctl,	nulldev,	nulldev,	0,
	0,		0,
    },
    {
	chatopen,	chatclose,	chatread,	chatwrite,	/*49*/
	chatioctl,	nulldev,	chatselect,	0,
	0,		0,
    },
    {
	chutopen,	chutclose,	chutread,	chutwrite,	/*50*/
	chutioctl,	nulldev,	chutselect,	0,
	0,		0,
    },
    {
	chutopen,	chutclose,	chutread,	chutwrite,	/*51*/
	chutioctl,	nulldev,	chutselect,	0,
	0,		0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*52*/
	nodev,		nodev,		nodev,		0,
	0,		0,
    },
    {
	hdopen,		nulldev,	hdread,		hdwrite,	/*53*/
	hdioctl,	nulldev,	seltrue, 	0,
	0,		0,
    },
    {
	fdopen,		fdclose,	fdread,		fdwrite,	/*54*/
	fdioctl,	nulldev,	seltrue, 	0,
	0,		0,
    },
    {
	cgthreeopen,	cgthreeclose,	nodev,		nodev,		/*55*/
	cgthreeioctl,	nodev,		seltrue,	cgthreemmap,
	0,		spec_segmap,
    },
    {
	ppopen,		ppclose,	nodev,		ppwrite,	/*56*/
	ppioctl,	nodev,		nodev, 		0,
	0,		0,
    },
    {
	vdopen,		vdclose,	nodev,		nodev,		/*57*/
	vdioctl,	nodev,		nodev, 		0,
	0,		0,
    },
    {
	sropen,		srclose,	srread,		nodev,		/*58*/
	srioctl,	nulldev,	nulldev,	0,
	0,		0,
    },
    {
	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	/*59*/
	vd_unuseddev,	vd_unuseddev,	vd_unuseddev, 	0,
	0,		0,
    },
    {
	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	/*60*/
	vd_unuseddev,	vd_unuseddev,	vd_unuseddev, 	0,
	0,		0,
    },
    {
	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	vd_unuseddev,	/*61*/
	vd_unuseddev,	vd_unuseddev,	vd_unuseddev, 	0,
	0,		0,
    },
    {
	taacopen,	taacclose,	taacread,	taacwrite,	/*62*/
	taacioctl,	nodev,		seltrue,	taacmmap,
	0,		spec_segmap,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*63*/
	nodev,		nodev,		nodev,		0,
	tcptlitab,	0,
    },
    {
	cgeightopen,	cgeightclose,	nodev,		nodev,		/*64*/
	cgeightioctl,	nodev,		seltrue,	cgeightmmap,
	0,		spec_segmap,
    },
    /* unused major number (old IPI driver) */
    {
	nodev,		nodev,		nodev,		nodev,		/*65*/
	nodev,		nodev,		nodev,		0,
	0,		0,
    },
    /* mcp parallel printer port */
    {
	nodev,		nodev,		nodev,		nodev,		/*66*/
	nodev,		nodev,		nodev,		0,
	mcpptab,	0,
    },
    {
	cgsixopen,	cgsixclose,	nodev,		nodev,		/*67*/
	cgsixioctl,	nodev,		nodev,		cgsixmmap,
	0,		cgsixsegmap,
    },
    {
	cgnineopen,	cgnineclose,	nodev,		nodev,		/*68*/
	cgnineioctl,	nodev,		nodev,		cgninemmap,
	0,		spec_segmap,
    },
    {
	acloneopen,	nodev,		nodev,		nodev,		/*69*/
	nodev,		nodev,		nodev,		0,
	0,		0,
    },
    {
	opromopen,	nulldev,	nodev,		nodev,		/*70*/
	opromioctl,	nodev,		nodev,		0,
	0,		0,
    },
    {
	sgopen,		sgclose,	sgread,		sgwrite,	/*71*/
	sgioctl,	nodev,		nodev,		0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*72*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*73*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*74*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },

    {
	idcopen,	nulldev,	idread,		idwrite,	/*75*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*76*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*77*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*78*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*79*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*80*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*81*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*82*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*83*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*84*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*85*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*86*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*87*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*88*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*89*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*90*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*91*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*92*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*93*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*94*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*95*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*96*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*97*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*98*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	idcopen,	nulldev,	idread,		idwrite,	/*99*/
	idioctl,	nulldev,	seltrue,	0,
	0,		0,
    },
    {
	tvoneopen,	tvoneclose,	nodev,		nodev,		/*100*/
	tvoneioctl,	nodev,		seltrue,	tvonemmap,
	0,		0,
    },
    {
	vxopen,		vxclose,	vxread,		vxwrite,	/*101*/
	vxioctl,	nodev,		seltrue,	vxmmap,
	0,		vxsegmap,
    },
    {
	cgtwelveopen,	cgtwelveclose,	nodev,		nodev,		/*102*/
	cgtwelveioctl,	nodev,		nodev,		cgtwelvemmap,
	0,		cgtwelvesegmap,
    },
    {
	gtopen,		gtclose,	nodev,		nodev,		/*103*/
	gtioctl,	nodev,		nodev,		gtmmap,
	0,		gtsegmap,
    },
    {
        nodev,          nodev,          nodev,          nodev,	        /*104*/
        nodev,          nodev,          nodev,          0,
        lightpentab,          0,
    },
    {
	nodev,		nodev,		nodev,		nodev,		/*105*/
	nodev,		nodev,		nodev,		0,
	audio_79C30tab,	0,
    },
    { 
	nodev,          nodev,          nodev,          nodev,          /*106*/
	nodev,          nodev,          nodev,          0,
	dbri_tab,       0,
    },
    {
	bpp_open,	bpp_close,	bpp_read,	bpp_write,	/*107*/
	bpp_ioctl,	nulldev,	seltrue,	0,
	0,		0,
    }
};

int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 3;	/* major device number of memory special file */
int	dump_no = 41;	/* major device number of dump special file */
