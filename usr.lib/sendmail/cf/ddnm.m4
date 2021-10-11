############################################################
#
#		DDN Mailer specification
#
#	Send mail on the Defense Data Network
#	   (such as Arpanet or Milnet)

Mddn,	P=[TCP], F=msDFMuCX, S=22, R=22, A=TCP $h, E=\r\n

# map containing the inverse of mail.aliases
DZmail.byaddr

S22
R$*<@LOCAL>$*		$:$1
R$-<@$->		$:$>3${Z$1@$2$}			invert aliases
R$*<@$+.$*>$*		$@$1<@$2.$3>$4			already ok
R$+<@$+>$*		$@$1<@$2.$m>$3			tack on our domain
R$+			$@$1<@$m>			tack on our domain
