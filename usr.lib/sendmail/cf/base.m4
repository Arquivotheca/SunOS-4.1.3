# Version number of configuration file
include(version.m4)

###   Standard macros

# name used for error messages
DnMailer-Daemon
# UNIX header format
DlFrom $g  $d
# delimiter (operator) characters
Do.:%@!^=/[]
# format of a total name
Dq$g$?x ($x)$.
# SMTP login message
De$j Sendmail $v/$V ready at $b

###   Options

# Remote mode - send through server if mailbox directory is mounted
OR
# location of alias file
OA/etc/aliases
# default delivery mode (deliver in background)
Odbackground
# rebuild the alias file automagically
OD
# temporary file mode -- 0600 for secure mail, 0644 for permissive
OF0600
# default GID
Og1
# location of help file
OH/usr/lib/sendmail.hf
# log level
OL9
# default messages to old style
Oo
# Cc my postmaster on error replies I generate
OPPostmaster
# queue directory
OQ/usr/spool/mqueue
# read timeout for SMTP protocols
Or15m
# status file -- none
OS/etc/sendmail.st
# queue up everything before starting transmission, for safety
Os
# return queued mail after this long
OT3d
# default UID
Ou1

###   Message precedences
Pfirst-class=0
Pspecial-delivery=100
Pjunk=-100

###   Trusted users
T root daemon uucp

###   Format of headers 
H?P?Return-Path: <$g>
HReceived: $?sfrom $s $.by $j ($v/$V)
	id $i; $b
H?D?Resent-Date: $a
H?D?Date: $a
H?F?Resent-From: $q
H?F?From: $q
H?x?Full-Name: $x
HSubject:
H?M?Resent-Message-Id: <$t.$i@$j>
H?M?Message-Id: <$t.$i@$j>
HErrors-To:

###########################
###   Rewriting rules   ###
###########################


#  Sender Field Pre-rewriting
S1
# None needed.

#  Recipient Field Pre-rewriting
S2
# None needed.

# Name Canonicalization

# Internal format of names within the rewriting rules is:
# 	anything<@host.domain.domain...>anything
# We try to get every kind of name into this format, except for local
# names, which have no host part.  The reason for the "<>" stuff is
# that the relevant host name could be on the front of the name (for
# source routing), or on the back (normal form).  We enclose the one that
# we want to route on in the <>'s to make it easy to find.
# 
S3

# handle "from:<>" special case
R$*<>$*			$@@				turn into magic token

# basic textual canonicalization
R$*<$+>$*		$2				basic RFC822 parsing

# make sure <@a,@b,@c:user@d> syntax is easy to parse -- undone later
R@$+,$+:$+		@$1:$2:$3			change all "," to ":"
R@$+:$+			$@$>6<@$1>:$2			src route canonical

R$+:$*;@$+		$@$1:$2;@$3			list syntax
R$+@$+			$:$1<@$2>			focus on domain
R$+<$+@$+>		$1$2<@$3>			move gaze right
R$+<@$+>		$@$>6$1<@$2>			already canonical

# convert old-style names to domain-based names
# All old-style names parse from left to right, without precedence.
R$-!$+			$@$>6$2<@$1.uucp>		uucphost!user
R$-.$+!$+		$@$>6$3<@$1.$2>			host.domain!user
R$+%$+			$@$>3$1@$2			user%host

#  Final Output Post-rewriting 
S4
R$+<@$+.uucp>		$2!$1				u@h.uucp => h!u
R$+			$: $>9 $1			Clean up addr
R$*<$+>$*		$1$2$3				defocus


#  Clean up an name for passing to a mailer
#  (but leave it focused)
S9
R$=w!@			$@$w!$n				
R@			$@$n				handle <> error addr
R$*<$*LOCAL>$*		$1<$2$m>$3			change local info
R<@$+>$*:$+:$+		<@$1>$2,$3:$4			<route-addr> canonical
