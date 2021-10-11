#################################################
#
#	General configuration information

# local domain names
#
# These can now be determined from the domainname system call.
# The first component of the NIS domain name is stripped off unless
# it begins with a dot or a plus sign.
# If your NIS domain is not inside the domain name you would like to have
# appear in your mail headers, add a "Dm" line to define your domain name.
# The Dm value is what is used in outgoing mail.  The Cm values are
# accepted in incoming mail.  By default Cm is set from Dm, but you might
# want to have more than one Cm line to recognize more than one domain 
# name on incoming mail during a transition.
# Example:
# DmCS.Podunk.EDU
# Cm cs cs.Podunk.EDU
#
# known hosts in this domain are obtained from gethostbyname() call

include(base.m4)

#######################
#   Rewriting rules

# special local conversions
S6
R$*<@$*$=m>$*		$1<@$2LOCAL>$4			convert local domain

include(localm.m4)
include(etherm.m4)
