.\" @(#)tmac.s 1.1 92/07/30 SMI; from UCB 4.2
.ds // /usr/lib/ms/
.	\" IZ - initialize (before text begins)
.de IZ
.nr FM 1i
.nr YY -\\n(FMu
.nr XX 0 1
.nr IP 0
.nr PI 5n
.nr QI 5n
.nr FI 2n
.nr I0 \\n(PIu
.if n .nr PD 1v
.if t .nr PD .3v
.if n .nr DD 1v
.if t .nr DD .5v
.nr PS 10
.nr VS 12
.ps \\n(PS
.vs \\n(VSp
.nr ML 3v
.nr IR 0
.nr TB 0
.nr SJ \\n(.j
.nr PO \\n(.o
.nr LL 6i
.ll \\n(LLu
.lt 6i
.ev 1
.nr FL 5.5i
.ll \\n(FLu
.ps 8
.vs 10p
.ev
.ds CH - \\\\n(PN -
.if n .ds CF \\*(DY
.wh 0 NP
.wh -\\n(FMu FO
.ch FO 16i
.wh -\\n(FMu FX
.ch FO -\\n(FMu
.wh -\\n(FMu/2u BT
..
.	\" RT - reset (at new paragraph)
.de RT
.if !\\n(1T .BG
.if !\\n(IK .if !\\n(IF .if !\\n(IX .if !\\n(BE .di
.if \\n(TM .ls 2
.ce 0
.ul 0
.if \\n(QP \{\
.	ll +\\n(QIu
.	in -\\n(QIu
.	nr QP -1
.\}
.if \\n(NX<=1 .if !\\n(AJ .ll \\n(LLu
.if !\\n(IF \{\
.	ps \\n(PS
.	if \\n(VS>=40 .vs \\n(VSu
.	if \\n(VS<=39 .vs \\n(VSp
.\}
.if !\\n(IP .nr I0 \\n(PIu
.if \\n(IP \{\
.	in -\\n(I\\n(IRu
.	nr IP -1
.\}
.ft 1
.TA
.fi
..
.	\" TA - set default tabs
.de TA
.if n .ta 8n 16n 24n 32n 40n 48n 56n 64n 72n 80n
.if t .ta 5n 10n 15n 20n 25n 30n 35n 40n 45n 50n 55n 60n 65n 70n 75n
..
.	\" BG - begin (at first paragraph)
.de BG
.br
.nr YE 1
.di
.ce 0
.nr KI 0
.hy 14
.nr 1T 1
.S\\n(ST
.rm S0 S1 S2 SY TX AX WT RP
.\"redefs
.de TL
.ft 3
.ce 99
.sp
.LG
\\..
.de AU
.ft 2
.if n .ul 0
.ce 99
.sp
.NL
\\..
.de AI
.ft 1
.if n .ul 0
.ce 99
.if n .sp
.if t .sp .5
.NL
\\..
.RA
.rn FJ FS
.rn FK FE
.nf
.ev 1
.ps \\n(PS-2
.vs \\n(.s+2p
.ev
.if !\\n(KG .nr FP 0
.nr KG 0
.if \\n(FP \{\
.	FS
.	FG
.	FE
.\}
.br
.if \\n(TV .if n .sp 2
.if \\n(TV .if t .sp 1
.fi
.ll \\n(LLu
..
.	\" RA - redefine abstract
.de RA
.de AB
.br
.if !\\n(1T .BG
.ce
.sp
.if !\\n(.$ ABSTRACT
.if \\n(.$ .if !\\$1no \\$1
.if !\\n(.$ .sp
.if \\n(.$ .if !\\$1no .sp
.sp
.nr AJ 1
.in +\\n(.lu/12u
.ll -\\n(.lu/12u
.RT
.if \\n(TM .ls 1
\\..
.de AE
.nr AJ 0
.br
.in 0
.ll \\n(LLu
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
.if \\n(TM .ls 2
\\..
..
.	\" RP - released paper format
.de RP
.nr ST 2
.if \\$1no .nr ST 1
.pn 0
.br
..
.	\" TL - source file for cover sheet
.de TL
.rn TL @T
.so \*(//ms.cov
.TL
.rm @T
..
.	\" PP - regular paragraph
.de PP
.RT
.if \\n(1T .sp \\n(PDu
.ne 1.1
.ti +\\n(PIu
..
.	\" LP - left paragraph
.de LP
.RT
.if \\n(1T .sp \\n(PDu
.ne 1.1
.ti \\n(.iu
..
.	\" IP - indented paragraph
.de IP
.RT
.if \\n(1T .sp \\n(PDu
.ne 1.1
.if !\\n(IP .nr IP +1
.if \\n(.$-1 .nr I\\n(IR \\$2n
.in +\\n(I\\n(IRu
.ta \\n(I\\n(IRu
.if \\n(.$ \{\
.ds HT \&\\$1
.ti -\\n(I\\n(IRu
\\*(HT\t\c
.if \w'\\*(HT'u>(\\n(I\\n(IRu+1n) .br
.\}
..
.	\" XP - exdented paragraph
.de XP
.RT
.if \\n(1T .sp \\n(PDu
.ne 1.1
.if !\\n(IP .nr IP +1
.in +\\n(I\\n(IRu
.ti -\\n(I\\n(IRu
..
.	\" QP - quote paragraph
.de QP
.ti \\n(.iu
.RT
.if \\n(1T .sp \\n(PDu
.ne 1.1
.nr QP 1
.in +\\n(QIu
.ll -\\n(QIu
.ti \\n(.iu
.if \\n(TM .ls 1
..
.	\" SH - section header
.de SH
.ti \\n(.iu
.RT
.if \\n(1T .sp
.RT
.ne 3.1
.B
..
.	\" NH - numbered header
.de NH
.SH
.nr NS \\$1
.if !\\n(.$ .nr NS 1
.if !\\n(NS .nr NS 1
.nr H\\n(NS +1
.if !\\n(NS-4 .nr H5 0
.if !\\n(NS-3 .nr H4 0
.if !\\n(NS-2 .nr H3 0
.if !\\n(NS-1 .nr H2 0
.if !\\$1 .if \\n(.$ .nr H1 1
.if \\$1S \{\
.	nr NS \\n(.$-1
.	nr H1 \\$2
.	nr H2 \\$3
.	nr H3 \\$4
.	nr H4 \\$5
.	nr H5 \\$6
.\}
.ds SN \\n(H1.
.if \\n(NS-1 .as SN \\n(H2.
.if \\n(NS-2 .as SN \\n(H3.
.if \\n(NS-3 .as SN \\n(H4.
.if \\n(NS-4 .as SN \\n(H5.
\\*(SN
..
.	\" DS - display with keep (L=left I=indent C=center B=block)
.de DS
.KS
.nf
.\\$1D \\$2 \\$1
.ft 1
.ps \\n(PS
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
..
.de D
.ID \\$1
..
.	\" ID - indented display with no keep
.de ID
.XD
.if t .in +.5i
.if n .in +8
.if \\n(.$ .if !\\$1I .if !\\$1 \{\
.	in \\n(OIu
.	in +\\$1n
.\}
..
.	\" LD - left display with no keep
.de LD
.XD
..
.	\" CD - centered display with no keep
.de CD
.XD
.ce 999
..
.	\" XD - real display macro
.de XD
.nf
.nr OI \\n(.i
.sp \\n(DDu
.if \\n(TM .ls 1
..
.	\" DE - end display of any kind
.de DE
.ce 0
.if \\n(BD .DF
.nr BD 0
.in \\n(OIu
.KE
.if \\n(TM .ls 2
.sp \\n(DDu
.fi
..
.	\" BD - block display: center entire block
.de BD
.XD
.nr BD 1
.nf
.in \\n(OIu
.di DD
..
.	\" DF - finish block display
.de DF
.di
.if \\n(dl>\\n(BD .nr BD \\n(dl
.if \\n(BD<\\n(.l .in (\\n(.lu-\\n(BDu)/2u
.nr EI \\n(.l-\\n(.i
.ta \\n(EIuR
.DD
.in \\n(OIu
..
.	\" KS - begin regular keep
.de KS
.nr KN \\n(.u
.if !\\n(IK .if !\\n(IF .KQ
.nr IK +1
..
.	\" KQ - real keep processor
.de KQ
.br
.nr KI \\n(.i
.ev 2
.TA
.br
.in \\n(KIu
.ps \\n(PS
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
.ll \\n(LLu
.lt \\n(LTu
.if \\n(NX>1 .ll \\n(CWu
.if \\n(NX>1 .lt \\n(CWu
.di KK
.nr TB 0
..
.	\" KF - begin floating keep
.de KF
.nr KN \\n(.u
.if !\\n(IK .FQ
.nr IK +1
..
.	\" FQ - real floating keep processor
.de FQ
.nr KI \\n(.i
.ev 2
.TA
.br
.in \\n(KIu
.ps \\n(PS
.if \\n(VS>=40 .vs \\n(VSu
.if \\n(VS<=39 .vs \\n(VSp
.ll \\n(LLu
.lt \\n(LTu
.if \\n(NX>1 .ll \\n(CWu
.if \\n(NX>1 .lt \\n(CWu
.di KK
.nr TB 1
..
.	\" KE - end keep
.de KE
.if \\n(IK .if !\\n(IK-1 .if !\\n(IF .RQ
.if \\n(IK .nr IK -1
..
.	\" RQ - real keep release
.de RQ
.br
.di
.nr NF 0
.if \\n(dn-\\n(.t .nr NF 1
.if \\n(TC .nr NF 1
.if \\n(NF .if !\\n(TB .sp 200
.if !\\n(NF .if \\n(TB .nr TB 0
.nf
.rs
.nr TC 5
.in 0
.ls 1
.if !\\n(TB \{\
.	ev
.	br
.	ev 2
.	KK
.\}
.ls
.ce 0
.if !\\n(TB .rm KK
.if \\n(TB .da KJ
.if \\n(TB \!.KD \\n(dn
.if \\n(TB .KK
.if \\n(TB .di
.nr TC \\n(TB
.if \\n(KN .fi
.in
.ev
..
.	\" KD - keep redivert
.de KD
.nr KM 0
.if \\n(.zKJ .nr KM 1
.if \\n(KM \!.KD \\$1
.if !\\n(KM .if \\n(.t<\\$1 .di KJ
..
.	\" EM - end macro (process leftover keep)
.de EM
.br
.if !\\n(TB .if t .wh -1p CM
.if \\n(TB \{\
\&\c
'	bp
.	NP
.	ch CM 160
.\}
..
.de XK
.nr TD 1
.nf
.ls 1
.in 0
.rn KJ KL
.KL
.rm KL
.if \\n(.zKJ .di
.nr TB 0
.if \\n(.zKJ .nr TB 1
.br
.in
.ls
.fi
.nr TD 0
..
.	\" NP - new page
.de NP
.if !\\n(LT .nr LT \\n(LLu
.if \\n(FM+\\n(HM>=\\n(.p \{\
.	tm HM + FM longer than page
.	ab
.\}
.if t .CM
.if !\\n(HM .nr HM 1i
.po \\n(POu
.nr PF \\n(.f
.nr PX \\n(.s
.ft 1
.ps \\n(PS
'sp \\n(HMu/2u
.PT
'sp |\\n(HMu
.HD	\"undefined
.ps \\n(PX
.ft \\n(PF
.nr XX 0 1
.nr YY 0-\\n(FMu
.ch FO 16i
.ch FX 17i
.ch FO -\\n(FMu
.ch FX \\n(.pu-\\n(FMu
.if \\n(MF .FV
.nr MF 0
.mk
.os
.ev 1
.if !\\n(TD .if \\n(TC<5 .XK
.nr TC 0
.ev
.nr TQ \\n(.i
.nr TK \\n(.u
.if \\n(IT \{\
.	in 0
.	nf
.	TT
.	in \\n(TQu
.	if \\n(TK .fi
.\}
.ns
.mk #T
..
.	\" PT - page titles
.de PT
.lt \\n(LTu
.pc %
.nr PN \\n%
.nr PT \\n%
.if \\n(P1 .nr PT 2
.if \\n(PT>1 .if !\\n(EH .if !\\n(OH .tl \\*(LH\\*(CH\\*(RH
.if \\n(PT>1 .if \\n(OH .if o .tl \\*(O1
.if \\n(PT>1 .if \\n(EH .if e .tl \\*(E2
.lt \\n(.lu
..
.	\" OH - odd page header
.de OH
.nr OH 1
.if !\\n(.$ .nr OH 0
.ds O1 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" EH - even page header
.de EH
.nr EH 1
.if !\\n(.$ .nr EH 0
.ds E2 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" P1 - PT on 1st page
.de P1
.nr P1 1
..
.	\" FO - footer
.de FO
.rn FO FZ
.if \\n(IT .nr T. 1
.if \\n(IT .if !\\n(FC .T# 1
.if \\n(IT .br
.nr FC +1
.if \\n(NX<2 .nr WF 0
.nr dn 0
.if \\n(FC<=1 .if \\n(XX .XF
.rn FZ FO
.nr MF 0
.if \\n(dn .nr MF 1
.if !\\n(WF .nr YY 0-\\n(FMu
.if !\\n(WF .ch FO \\n(YYu
.if !\\n(dn .nr WF 0
.if \\n(FC<=1 .if !\\n(XX \{\
.	if \\n(NX>1 .RC
.	if \\n(NX<2 'bp
.\}
.nr FC -1
.if \\n(ML .ne \\n(MLu
..
.	\" BT - bottom title
.de BT
.nr PF \\n(.f
.nr PX \\n(.s
.ft 1
.ps \\n(PS
.lt \\n(LTu
.po \\n(POu
.if \\n(TM .if \\n(CT \{\
.	tl ''\\n(PN''
.	nr CT 0
.\}
.if \\n% .if !\\n(EF .if !\\n(OF .tl \\*(LF\\*(CF\\*(RF
.if \\n% .if \\n(OF .if o .tl \\*(O3
.if \\n% .if \\n(EF .if e .tl \\*(E4
.ft \\n(PF
.ps \\n(PX
..
.	\" OF - odd page footer
.de OF
.nr OF 1
.if !\\n(.$ .nr OF 0
.ds O3 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" EF - even page footer
.de EF
.nr EF 1
.if !\\n(.$ .nr EF 0
.ds E4 \\$1 \\$2 \\$3 \\$4 \\$5 \\$6 \\$7 \\$8 \\$9
..
.	\" 2C - double column
.de 2C
.MC
..
.	\" 1C - single column
.de 1C
.MC \\n(LLu
.hy 14
..
.	\" MC - multiple columns, arg is col width
.de MC
.nr L1 \\n(LL*7/15
.if \\n(.$ .nr L1 \\$1n
.nr NQ \\n(LL/\\n(L1
.if \\n(NQ<1 .nr NQ 1
.if \\n(NQ>2 .if (\\n(LL%\\n(L1)=0 .nr NQ -1
.if !\\n(1T \{\
.	BG
.	if n .sp 4
.	if t .sp 2
.\}
.if !\\n(NX .nr NX 1
.if !\\n(NX=\\n(NQ \{\
.	RT
.	if \\n(NX>1 .bp
.	mk
.	nr NC 1
.	po \\n(POu
.\}
.if \\n(NQ>1 .hy 12
.nr NX \\n(NQ
.nr CW \\n(L1
.ll \\n(CWu
.nr FL \\n(CWu*11u/12u
.if \\n(NX>1 .nr GW (\\n(LL-(\\n(NX*\\n(CW))/(\\n(NX-1)
.nr RO \\n(CW+\\n(GW
.ns
..
.de RC
.if \\n(NC>=\\n(NX .C2
.if \\n(NC<\\n(NX .C1
.nr NC \\n(ND
..
.de C1
.rt
.po +\\n(ROu
.nr ND \\n(NC+1
.nr XX 0 1
.if \\n(MF .FV
.ch FX \\n(.pu-\\n(FMu
.ev 1
.if \\n(TB .XK
.nr TC 0
.ev
.nr TQ \\n(.i
.if \\n(IT .in 0
.if \\n(IT .TT
.if \\n(IT .in \\n(TQu
.mk #T
.ns
..
.de C2
.po \\n(POu
'bp
.nr ND 1
..
.	\" RS - right shift
.de RS
.nr IS \\n(IP
.RT
.nr IP \\n(IS
.if \\n(IP .in +\\n(I\\n(IRu
.nr IR +1
.nr I\\n(IR \\n(PIu
.in +\\n(I\\n(IRu
..
.	\" RE - retreat left
.de RE
.nr IS \\n(IP
.RT
.nr IP \\n(IS
.if \\n(IR .nr IR -1
.if \\n(IP<=0 .in -\\n(I\\n(IRu
..
.	\" CM - cut mark
.de CM
.po 0
.lt 7.6i
.ft 1
.ps 10
.vs 4p
.tl '--''--'
.po
.vs
.lt
.ps
.ft
..
.	\" I - italic font
.de I
.nr PQ \\n(.f
.if t .ft 2
.ie \\$1 .if n .ul 999
.el .if n .ul 1
.if t .if !\\$1 \&\\$1\|\f\\n(PQ\\$2
.if n .if \\n(.$=1 \&\\$1
.if n .if \\n(.$>1 \&\\$1\c
.if n .if \\n(.$>1 \&\\$2
..
.	\" B - bold font
.de B
.nr PQ \\n(.f
.if t .ft 3
.ie \\$1 .if n .ul 999
.el .if n .ul 1
.if t .if !\\$1 \&\\$1\f\\n(PQ\\$2
.if n .if \\n(.$=1 \&\\$1
.if n .if \\n(.$>1 \&\\$1\c
.if n .if \\n(.$>1 \&\\$2
..
.	\" R - Roman font
.de R
.if n .ul 0
.ft 1
..
.	\" UL - underline in troff
.de UL
.if t \\$1\l'|0\(ul'\\$2
.if n .I \\$1 \\$2
..
.	\" SM - smaller
.de SM
.ps -2
..
.	\" LG - larger
.de LG
.ps +2
..
.	\" NL - normal
.de NL
.ps \\n(PS
..
.	\" DA - force date
.de DA
.if \\n(.$ .ds DY \\$1 \\$2 \\$3 \\$4
.ds CF \\*(DY
..
.	\" ND - no date or new date
.de ND
.if \\n(.$ .ds DY \\$1 \\$2 \\$3 \\$4
.rm CF
..
.	\" \** - numbered footnote
.ds * \\*([.\\n+*\\*(.]
.	\" FJ - replaces FS after cover
.de FJ
'ce 0
.di
.ev 1
.ll \\n(FLu
.da FF
.br
.if \\n(IF .tm Nested footnote
.nr IF 1
.if !\\n+(XX-1 .FA
.if !\\n(MF .if !\\n(.$ .if \\n* .FP \\n*
.if !\\n(MF .if \\n(.$ .FP \\$1 no
..
.	\" FK - replaces FE after cover
.de FK
.br
.in 0
.nr IF 0
.di
.ev
.if !\\n(XX-1 .nr dn +\\n(.v
.nr YY -\\n(dn
.if !\\n(NX .nr WF 1
.if \\n(dl>\\n(CW .nr WF 1
.if (\\n(nl+\\n(.v)<=(\\n(.p+\\n(YY) .ch FO \\n(YYu
.if (\\n(nl+\\n(.v)>(\\n(.p+\\n(YY) \{\
.	if \\n(nl>(\\n(HM+1.5v) .ch FO \\n(nlu+\\n(.vu
.	if \\n(nl+\\n(FM+1v>\\n(.p .ch FX \\n(.pu-\\n(FMu+2v
.	if \\n(nl<=(\\n(HM+1.5v) .ch FO \\n(HMu+(4u*\\n(.vu)
.\}
..
.	\" FS - begin footnote on cover
.de FS
.ev 1
.br
.ll \\n(FLu
.da FG
.if !\\n(.$ .if \\n* .FP \\n*
.if \\n(.$ .FP \\$1 no
..
.	\" FE - end footnote on cover
.de FE
.br
.di
.nr FP \\n(dn
.if !\\n(1T .nr KG 1
.ev
..
.	\" FA - print line before footnotes
.de FA
.in 0
.if n _________________________
.if t \l'1i'
.br
..
.	\" FP - footnote paragraph
.de FP
.sp \\n(PDu/2u
.if \\n(FF<2 .ti \\n(FIu
.if \\n(FF=3 \{\
.	in \\n(FIu*2u
.	ta \\n(FIu*2u
.	ti 0
.\}
.if !\\n(FF \{\
.	ie "\\$2"no" \\$1\0\c
.	el \\*([.\\$1\\*(.]\0\c
.\}
.if \\n(FF .if \\n(FF<3 \{\
.	ie "\\$2"no" \\$1\0\c
.	el \\$1.\0\c
.\}
.if \\n(FF=3 \{\
.	ie "\\$2"no" \\$1\t\c
.	el \\$1.\t\c
.\}
..
.	\" FV - get leftover footnote from previous page
.de FV
.FS
.nf
.ls 1
.FY
.ls
.fi
.FE
..
.	\" FX - divert leftover footnote for next page
.de FX
.if \\n(XX .di FY
.if \\n(XX .ns
..
.	\" XF - actually print footnote
.de XF
.if \\n(nlu+1v>(\\n(.pu-\\n(FMu) .ch FX \\n(nlu+1.9v
.ev 1
.nf
.ls 1
.FF
.rm FF
.nr XX 0 1
.br
.ls
.di
.fi
.ev
..
.	\" TS - source file for tbl
.de TS
.rn TS @T
.so \*(//ms.tbl
.TS \\$1 \\$2
.rm @T
..
.	\" EQ - source file for eqn
.de EQ
.rn EQ @T
.so \*(//ms.eqn
.EQ \\$1 \\$2
.rm @T
..
.	\" ]- - source file for refer
.de ]-
.rn ]- @T
.so \*(//ms.ref
.]-
.rm @T
..
.	\" [< - for refer -s or -e
.de ]<
.rn ]< @T
.so \*(//ms.ref
.]<
.rm @T
..
.if \n(.V>19 .ds [. \f1[
.if \n(.V>19 .ds .] ]\fP
.if \n(.V<20 .ds [. \f1\s-2\v'-.4m'
.if \n(.V<20 .ds .] \v'.4m'\s+2\fP
.ds <. .
.ds <, ,
.if n .ds Q \&"
.if n .ds U \&"
.if n .ds - \%--
.if t .ds Q ``
.if t .ds U ''
.if t .ds - \(em
.ds ' \h'\w'e'u/5'\z\'\h'-\w'e'u/5'
.ds ` \h'\w'e'u/5'\z\`\h'-\w'e'u/5'
.ds ^ \h'\w'o'u/10'\z^\h'-\w'e'u/10'
.ds , \h'\w'c'u/5'\z,\h'-\w'e'u/5'
.ds : \h'\w'u'u/5'\z"\h'-\w'e'u/5'
.ds ~ \h'\w'n'u/10'\z~\h'-\w'e'u/10'
.ds C \h'\w'c'u/5'\v'-.6m'\s-4\zv\s+4\v'.6m'\h'-\w'c'u/5'
.	\" AM - better accent marks
.de AM
.so \*(//ms.acc
..
.	\" TM - thesis mode
.de TM
.so \*(//ms.ths
..
.	\" BX - word in a box
.de BX
.if t \(br\|\\$1\|\(br\l'|0\(rn'\l'|0\(ul'
.if n \(br\\kA\|\\$1\|\\kB\(br\v'-1v'\h'|\\nBu'\l'|\\nAu'\v'1v'\l'|\\nAu'
..
.	\" B1 - source file for boxed text
.de B1
.rn B1 @T
.so \*(//ms.tbl
.B1 \\$1
.rm @T
..
.	\" XS - table of contents
.de XS
.rn XS @T
.so \*(//ms.toc
.XS \\$1 \\$2
.rm @T
..
.	\" IX - index words to stderr
.de IX
.tm \\$1\t\\$2\t\\$3\t\\$4 ... \\n(PN
..
.	\" UX - UNIX macro
.de UX
.ie \\n(UX \s-1UNIX\s0\\$1
.el \{\
\s-1UNIX\s0\\$1\(dg
.FS
\(dg \s-1UNIX\s0 is a trademark of Bell Laboratories.
.FE
.nr UX 1
.\}
..
.co
.if \n(mo-0 .ds MO January
.if \n(mo-1 .ds MO February
.if \n(mo-2 .ds MO March
.if \n(mo-3 .ds MO April
.if \n(mo-4 .ds MO May
.if \n(mo-5 .ds MO June
.if \n(mo-6 .ds MO July
.if \n(mo-7 .ds MO August
.if \n(mo-8 .ds MO September
.if \n(mo-9 .ds MO October
.if \n(mo-10 .ds MO November
.if \n(mo-11 .ds MO December
.ds DY \*(MO \n(dy, 19\n(yr
.nr * 0 1
.IZ
.em EM
.rm CM
.rm IZ RA //
