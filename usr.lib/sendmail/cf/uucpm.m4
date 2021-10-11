ifdef(`m4COMPAT',, `include(compat.m4)')
#	UUCP Mailer specification

Muucp,	P=/usr/bin/uux, F=msDFMhuU, S=13, R=23,
	A=uux - -r -a$f $h!rmail ($u)

# Convert uucp sender (From) field
S13
R$+			$:$>5$1				convert to old style
R$=w!$+			$2				strip local name
R$+			$:$w!$1				stick on real host name

# Convert uucp recipient (To, Cc) fields
S23
R$+			$:$>5$1				convert to old style
