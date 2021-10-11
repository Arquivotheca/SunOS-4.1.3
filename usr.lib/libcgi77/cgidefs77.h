c
c       @(#)cgidefs77.h 1.1 92/07/30 Copyr 1985-9 Sun Micro
c
c Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
c Permission to use, copy, modify, and distribute this software for any 
c purpose and without fee is hereby granted, provided that the above 
c copyright notice appear in all copies and that both that copyright 
c notice and this permission notice are retained, and that the name 
c of Sun Microsystems, Inc., not be used in advertising or publicity 
c pertaining to this software without specific, written prior permission. 
c Sun Microsystems, Inc., makes no representations about the suitability 
c of this software or the interface defined in this software for any 
c purpose. It is provided "as is" without express or implied warranty.
c 
	integer NOERROR
	parameter (NOERROR = 0)
	integer ENOTCGCL
	parameter (ENOTCGCL = 1)
	integer ENOTCGOP
	parameter (ENOTCGOP = 2)
	integer ENOTVSOP
	parameter (ENOTVSOP = 3)
	integer ENOTVSAC
	parameter (ENOTVSAC = 4)
	integer ENOTOPOP
	parameter (ENOTOPOP = 5)
	integer EVSIDINV
	parameter (EVSIDINV = 10)
	integer ENOWSTYP
	parameter (ENOWSTYP = 11)
	integer EVSISOPN
	parameter (EVSISOPN = 12)
	integer EVSNOTOP
	parameter (EVSNOTOP = 13)
	integer EVSISACT
	parameter (EVSISACT = 14)
	integer EVSNTACT
	parameter (EVSNTACT = 15)
	integer EINQALTL
	parameter (EINQALTL = 16)
	integer EBADRCTD
	parameter (EBADRCTD = 20)
	integer EBDVIEWP
	parameter (EBDVIEWP = 21)
	integer ECLIPTOL
	parameter (ECLIPTOL = 22)
	integer ECLIPTOS
	parameter (ECLIPTOS = 23)
	integer EVDCSDIL
	parameter (EVDCSDIL = 24)
	integer EBTBUNDL
	parameter (EBTBUNDL = 30)
	integer EBBDTBDI
	parameter (EBBDTBDI = 31)
	integer EBTUNDEF
	parameter (EBTUNDEF = 32)
	integer EBADLINX
	parameter (EBADLINX = 33)
	integer EBDWIDTH
	parameter (EBDWIDTH = 34)
	integer ECINDXLZ
	parameter (ECINDXLZ = 35)
	integer EBADCOLX
	parameter (EBADCOLX = 36)
	integer EBADMRKX
	parameter (EBADMRKX = 37)
	integer EBADSIZE
	parameter (EBADSIZE = 38)
	integer EBADFABX
	parameter (EBADFABX = 39)
	integer EPATARTL
	parameter (EPATARTL = 40)
	integer EPATSZTS
	parameter (EPATSZTS = 41)
	integer ESTYLLEZ
	parameter (ESTYLLEZ = 42)
	integer ENOPATNX
	parameter (ENOPATNX = 43)
	integer EPATITOL
	parameter (EPATITOL = 44)
	integer EBADTXTX
	parameter (EBADTXTX = 45)
	integer EBDCHRIX
	parameter (EBDCHRIX = 46)
	integer ETXTFLIN
	parameter (ETXTFLIN = 47)
	integer ECEXFOOR
	parameter (ECEXFOOR = 48)
	integer ECHHTLEZ
	parameter (ECHHTLEZ = 49)
	integer ECHRUPVZ
	parameter (ECHRUPVZ = 50)
	integer ECOLRNGE
	parameter (ECOLRNGE = 51)
	integer ENMPTSTL
	parameter (ENMPTSTL = 60)
	integer EPLMTWPT
	parameter (EPLMTWPT = 61)
	integer EPLMTHPT
	parameter (EPLMTHPT = 62)
	integer EGPLISFL
	parameter (EGPLISFL = 63)
	integer EARCPNCI
	parameter (EARCPNCI = 64)
	integer EARCPNEL
	parameter (EARCPNEL = 65)
	integer ECELLATS
	parameter (ECELLATS = 66)
	integer ECELLPOS
	parameter (ECELLPOS = 67)
	integer ECELLTLS
	parameter (ECELLTLS = 68)
	integer EVALOVWS
	parameter (EVALOVWS = 69)
	integer EPXNOTCR
	parameter (EPXNOTCR = 70)
	integer EINDNOEX
	parameter (EINDNOEX = 80)
	integer EINDINIT
	parameter (EINDINIT = 81)
	integer EINDALIN
	parameter (EINDALIN = 82)
	integer EINASAEX
	parameter (EINASAEX = 83)
	integer EINAIIMP
	parameter (EINAIIMP = 84)
	integer EINNTASD
	parameter (EINNTASD = 85)
	integer EINTRNEX
	parameter (EINTRNEX = 86)
	integer EINNECHO
	parameter (EINNECHO = 87)
	integer EINECHON
	parameter (EINECHON = 88)
	integer EINEINCP
	parameter (EINEINCP = 89)
	integer EINERVWS
	parameter (EINERVWS = 90)
	integer EINETNSU
	parameter (EINETNSU = 91)
	integer EINENOTO
	parameter (EINENOTO = 92)
	integer EIAEVNEN
	parameter (EIAEVNEN = 93)
	integer EINEVNEN
	parameter (EINEVNEN = 94)
	integer EBADDATA
	parameter (EBADDATA = 95)
	integer ESTRSIZE
	parameter (ESTRSIZE = 96)
	integer EINQOVFL
	parameter (EINQOVFL = 97)
	integer EINNTRQE
	parameter (EINNTRQE = 98)
	integer EINNTRSE
	parameter (EINNTRSE = 99)
	integer EINNTQUE
	parameter (EINNTQUE = 100)
	integer EMEMSPAC
	parameter (EMEMSPAC = 110)
	integer ENOTCSTD
	parameter (ENOTCSTD = 111)
	integer ENOTCCPW
	parameter (ENOTCCPW = 112)
	integer EFILACC
	parameter (EFILACC = 113)
	integer ECGIWIN
	parameter (ECGIWIN = 114)


	integer BW1DD
	parameter (BW1DD = 1)
	integer BW2DD
	parameter (BW2DD = 2)
	integer CG1DD
	parameter (CG1DD = 3)
	integer BWPIXWINDD
	parameter (BWPIXWINDD = 4)
	integer CGPIXWINDD
	parameter (CGPIXWINDD = 5)
	integer GP1DD
	parameter (GP1DD = 6)
	integer CG2DD
	parameter (CG2DD = 7)
	integer CG4DD
	parameter (CG4DD = 8)
	integer PIXWINDD
	parameter (PIXWINDD = 9)
	integer CG3DD
	parameter (CG3DD = 10)
	integer CG6DD
	parameter (CG6DD = 11)

	integer VWSURFNEWFLG
	parameter (VWSURFNEWFLG = 1)
	integer MAXVWS
	parameter (MAXVWS = 5)
	integer MAXTRIG
	parameter (MAXTRIG = 6)
	integer MAXASSOC
	parameter (MAXASSOC = 5)
	integer MAXEVENTS
	parameter (MAXEVENTS = 1024)
	
	
	integer MAXAESSIZE
	parameter (MAXAESSIZE = 10)
	integer MAXNUMPATS
	parameter (MAXNUMPATS = 50)
	integer MAXPATSIZE
	parameter (MAXPATSIZE = 256)
	integer MAXPTS
	parameter (MAXPTS = 1024)
	
	integer ROMAN
	parameter (ROMAN = 0)
	integer GREEK
	parameter (GREEK = 1)
	integer SCRIPT
	parameter (SCRIPT = 2)
	integer OLDENGLISH
	parameter (OLDENGLISH = 3)
	integer STICK
	parameter (STICK = 4)
	integer SYMBOLS
	parameter (SYMBOLS = 5)
	integer DEVNAMESIZE
	parameter (DEVNAMESIZE = 20)
