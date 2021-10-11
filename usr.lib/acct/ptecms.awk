#ident	"@(#)ptecms.awk 1.1 92/07/30 SMI"; /* from S5R3 acct:ptecms.awk 1.3 */
BEGIN {
	MAXCPU = 20.0		# report if cpu usage greater than this
	MAXKCORE = 1000.0	# report if KCORE usage is greater than this
	}
NF == 4		{
			print "\t\t\t\t" $1 " Time Exception Command Usage Summary"
		}

NF == 3		{
			print "\t\t\t\tCommand Exception Usage Summary"
		}

NR == 1		{
			MAXCPU = MAXCPU + 0.0
			MAXKCORE = MAXKCORE + 0.0
			print "\t\t\t\tTotal CPU > " MAXCPU " or Total KCORE > " MAXKCORE
		}

NF <= 4 && length != 0	{
				next
			}

$1 == "COMMAND" || $1 == "NAME"		{
						print
						next
					}

NF == 10 && ( $4 > MAXCPU || $3 > MAXKCORE ) && $1 != "TOTALS"

NF == 13 && ( $5 + $6 > MAXCPU || $4 > MAXKCORE ) && $1 != "TOTALS"

length == 0


