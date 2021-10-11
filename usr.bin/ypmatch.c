#ifndef lint
static  char sccsid[] = "@(#)ypmatch.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This is a user command which looks up the value of a key in a map
 *
 * Usage is:
 *	ypmatch [-d domain] [-t] [-k] key [key ...] mname 
 *	ypmatch -x
 *
 * where:  the -d switch can be used to specify a domain other than the
 * default domain.  mname may be either a mapname, or a nickname which will
 * be translated into a mapname according to the translation table at
 * transtable.  The  -t switch inhibits this translation. The -k switch
 * prints keys as well as values.  The -x switch may be used alone to
 * dump the translation table.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

void get_command_line_args();
void getdomain();
bool match_list();
bool match_one();
void print_one();
void dumptable();

#define TIMEOUT 30			/* Total seconds for timeout */
#define INTER_TRY 10			/* Seconds between tries */

int translate = TRUE;
int printkeys = FALSE;
int dodump = FALSE;
char *domain = NULL;
char default_domain_name[YPMAXDOMAIN];
char *map = NULL;
char **keys = NULL;
int nkeys;
struct timeval udp_intertry = {
	INTER_TRY,			/* Seconds */
	0				/* Microseconds */
	};
struct timeval udp_timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};
char *transtable[] = {
	"passwd", "passwd.byname",
	"group", "group.byname",
	"networks", "networks.byaddr",
	"hosts", "hosts.byname",
	"protocols","protocols.bynumber",
	"services","services.byname",
	"aliases", "mail.aliases",
	"ethers", "ethers.byname",
	NULL
};
char err_usage[] =
"Usage:\n\
	ypmatch [-d domain] [-t] [-k] key [key ...] mname\n\
	ypmatch -x\n\
where\n\
	mname may be either a mapname or a nickname for a map\n\
	-t inhibits map nickname translation\n\
	-k prints keys as well as values.\n\
	-x dumps the map nickname translation table.\n";
char err_bad_args[] =
	"ypmatch:  %s argument is bad.\n";
char err_cant_get_kname[] =
	"ypmatch:  can't get %s back from system call.\n";
char err_null_kname[] =
	"ypmatch:  the %s hasn't been set on this machine.\n";
char err_bad_mapname[] = "mapname";
char err_bad_domainname[] = "domainname";

/*
 * This is the main line for the ypmatch process.
 */
main(argc, argv)
	char **argv;
{
	int i;

	get_command_line_args(argc, argv);

	if (dodump) {
		dumptable();
		exit(0);
	}

	if (!domain) {
		getdomain();
	}

	if (translate) {
						
		for (i = 0; transtable[i]; i += 2)

			if (strcmp(map, transtable[i]) == 0) {
				map = transtable[i + 1];
			}
	}

	if (match_list()) {
		exit(0);
	} else {
		exit(1);
	}

	/* NOTREACHED */
}

/*
 * This does the command line argument processing.
 */
void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{
		
	if (argc < 2) {
		(void) fprintf(stderr, err_usage);
		exit(1);
	}
	argv++;

	while (--argc > 0 && (*argv)[0] == '-') {

		switch ((*argv)[1]) {

		case 't':
			translate = FALSE;
			break;

		case 'k':
			printkeys = TRUE;
			break;

		case 'x':
			dodump = TRUE;
			break;

		case 'd':

			if (argc > 1) {
				argv++;
				argc--;
				domain = *argv;

				if (strlen(domain) > YPMAXDOMAIN) {
					(void) fprintf(stderr, err_bad_args,
					    err_bad_domainname);
					exit(1);
				}
				
			} else {
				(void) fprintf(stderr, err_usage);
				exit(1);
			}
				
			break;
				
		default:
			(void) fprintf(stderr, err_usage);
			exit(1);
		}

		argv++;
	}

	if (!dodump) {
		
		if (argc < 2) {
			(void) fprintf(stderr, err_usage);
			exit(1);
		}

		keys = argv;
		nkeys = argc -1;
		map = argv[argc -1];

		if (strlen(map) > YPMAXMAP) {
			(void) fprintf(stderr, err_bad_args, err_bad_mapname);
			exit(1);
		}
	}
}

/*
 * This gets the local default domainname, and makes sure that it's set
 * to something reasonable.  domain is set here.
 */
void
getdomain()		
{
	if (!getdomainname(default_domain_name, YPMAXDOMAIN) ) {
		domain = default_domain_name;
	} else {
		(void) fprintf(stderr, err_cant_get_kname, err_bad_domainname);
		exit(1);
	}

	if (strlen(domain) == 0) {
		(void) fprintf(stderr, err_null_kname, err_bad_domainname);
		exit(1);
	}
}

/*
 * This traverses the list of argument keys.
 */
bool
match_list()
{
	bool error;
	bool errors = FALSE;
	char *val;
	int len;
	int n = 0;

	while (n < nkeys) {
		error = match_one(keys[n], &val, &len);

		if (!error) {
			print_one(keys[n], val, len);
			free(val);
		} else {
			errors = TRUE;
		}

		n++;
	}
	
	return (!errors);
}

/*
 * This fires off a "match" request to any old NIS server, using the vanilla
 * NIS client interface.  To cover the case in which trailing NULLs are included
 * in the keys, this retrys the match request including the NULL if the key
 * isn't in the map.
 */
bool
match_one(key, val, len)
	char *key;
	char **val;
	int *len;
{
	int err;
	bool error = FALSE;

	*val = NULL;
	*len = 0;
	err = yp_match(domain, map, key, strlen(key), val, len);
	

	if (err == YPERR_KEY) {
		err = yp_match(domain, map, key, (strlen(key) + 1),
		    val, len);
	}
		
	if (err) {
		(void) fprintf(stderr,
		    "Can't match key %s in map %s.  Reason: %s.\n", key, map,
		    yperr_string(err) );
		error = TRUE;
	}
	
	return (error);
}

/*
 * This prints the value, (and optionally, the key) after first checking that
 * the last char in the value isn't a NULL.  If the last char is a NULL, the
 * \n\0 sequence which the NIS client layer has given to us is shuffled back
 * one byte.
 */
void
print_one(key, val, len)
	char *key;
	char *val;
	int len;
{
	if (printkeys) {
		(void) printf("%s: ", key);
	}

	(void) printf("%.*s\n", len, val);
}

/*
 * This will print out the map nickname translation table.
 */
void
dumptable()
{
	int i;

	for (i = 0; transtable[i]; i += 2) {
		printf("Use \"%s\" for map \"%s\"\n", transtable[i],
		    transtable[i + 1]);
	}
}
