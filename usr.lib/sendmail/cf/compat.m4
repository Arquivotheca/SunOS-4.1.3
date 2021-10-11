##########################################################
#  General code to convert back to old style UUCP names
S5
R$+<@LOCAL>		$@ $w!$1			name@LOCAL => sun!name
R$+<@$-.LOCAL>		$@ $2!$1			u@h.LOCAL => h!u
R$+<@$+.uucp>		$@ $2!$1			u@h.uucp => h!u
R$+<@$*>		$@ $2!$1			u@h => h!u
# Route-addrs do not work here.  Punt til uucp-mail comes up with something.
R<@$+>$*		$@ @$1$2			just defocus and punt
R$*<$*>$*		$@ $1$2$3			Defocus strange stuff
