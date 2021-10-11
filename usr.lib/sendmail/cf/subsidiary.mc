###########################################################
#
#	SENDMAIL CONFIGURATION FILE FOR SUBSIDIARY MACHINES
#
#	You should install this file as /etc/sendmail.cf
#	if your machine is a subsidiary machine (that is, some
#	other machine in your domain is the main mail-relaying
#	machine).  Then edit the file to customize it for your
#	network configuration.
#
#	See the manual "System Administration for the Sun Workstation".
#	Look at "Setting Up The Mail Routing System" in the chapter on
#	Communications.  The Sendmail references in the back of the
#	manual are also very useful.
#
#	@(#)subsidiary.mc 1.1 92/07/30 SMI; from UCB arpa.mc 3.25 2/24/83
#

# local UUCP connections -- not forwarded to mailhost
CV

# my official hostname
Dj$w.$m

# major relay mailer
DMether

# major relay host
DRmailhost
CRmailhost

include(sunbase.m4)

include(uucpm.m4)

include(zerobase.m4)

################################################
###  Machine dependent part of ruleset zero  ###
################################################

# resolve names we can handle locally
R<@$=V.uucp>:$+		$:$>9 $1			First clean up, then...
R<@$=V.uucp>:$+		$#uucp  $@$1 $:$2		@host.uucp:...
R$+<@$=V.uucp>		$#uucp  $@$2 $:$1		user@host.uucp

# optimize names of known ethernet hosts
R$*<@$%y.LOCAL>$*	$#ether $@$2 $:$1<@$2>$3	user@host.here

# other non-local names will be kicked upstairs
R$+			$:$>9 $1			Clean up, keep <>
R$*<@$+>$*		$#$M    $@$R $:$1<@$2>$3	user@some.where
R$*@$*			$#$M    $@$R $:$1<@$2>		strangeness with @

# Local names with % are really not local!
R$+%$+			$@$>30$1@$2			turn % => @, retry

# everything else is a local name
R$+			$#local $:$1			local names
