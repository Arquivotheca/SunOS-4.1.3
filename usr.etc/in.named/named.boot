;
;	boot file for name server
; Note that there should be one primary entry for each SOA record.
;
; type		domain		source file or host
;
domain		BERKELEY.EDU
primary		BERKELEY.EDU	/etc/ucbhosts
cache		.		/etc/named.ca
secondary	CC.BERKELEY.EDU	128.32.07	; UCBVAX
