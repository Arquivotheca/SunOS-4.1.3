#ident	"@(#)ptelus.awk 1.1 92/07/30 SMI"; /* from S5R3 acct:ptelus.awk 1.4 */
BEGIN	{
	MAXCPU = 20.		# report if cpu usage is greater than this
	MAXKCORE = 500.		# report is Kcore usage is greater than this
	MAXCONNECT = 120.	# report if connect time is greater than this
	}
NR == 1	 {
	MAXCPU = MAXCPU + 0
	MAXKCORE = MAXKCORE + 0
	MAXCONNECT = MAXCONNECT + 0
	printf "Logins with exceptional Prime/Non-prime Time Usage\n"
	printf ( "CPU > %d or KCORE > %d or CONNECT > %d\n\n\n", MAXCPU, MAXKCORE, MAXCONNECT)
	printf "\tLogin\t\tCPU (mins)\tKCORE-mins\tCONNECT-mins\tdisk"
	printf "\t# of\t# of\t# Disk\tfee\n"
	printf "UID\tName\t\tPrime\tNprime\tPrime\tNprime\t"
	printf "Prime\tNprime\tBlocks\tProcs\tSess\tSamples\n\n"
	}
$3 > MAXCPU || $4 > MAXCPU || $5 > MAXKCORE || $6 > MAXKCORE || $7 > MAXCONNECT || $8 > MAXCONNECT {
				printf("%d\t%-8.8s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)
				}
