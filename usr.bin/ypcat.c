#ifndef lint
static	char sccsid[] = "@(#)ypcat.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * This is a user command which dumps each entry in a NIS data base.  It gets
 * the stuff using the normal ypclnt package; the user doesn't get to choose
 * which server gives him the input.  Usage is:
 * ypcat [-k] [-d domain] [-t] map
 * ypcat -x
 * where the -k switch will dump keys followed by a single blank space
 * before the value, and the -d switch can be used to specify a domain other
 * than the default domain.
 * 
 * Normally, passwd gets converted to passwd.byname, and similarly for the
 * other standard files.  -t inhibits this translation.
 * 
 * The -x switch will dump the map nickname translation table.  
 */
#ifdef NULL
#undef NULL
#endif
#define NULL 0
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>

int dumpkeys = FALSE;
int translate = TRUE;
int dodump = FALSE;
char *domain = NULL;
char default_domain_name[YPMAXDOMAIN];
char *map = NULL;
char nullstring[] = "";
char err_usage[] =
"Usage:\n\
	ypcat [-k] [-d domainname] [-t] mapname\n\
	ypcat -x\n";
char err_bad_args[] =
	"ypcat:  %s argument is bad.\n";
char err_cant_get_kname[] =
	"ypcat:  can't get %s back from system call.\n";
char err_null_kname[] =
	"ypcat:  the %s hasn't been set on this machine.\n";
char err_bad_mapname[] = "mapname";
char err_bad_domainname[] = "domainname";
char err_cant_bind[] =
	"ypcat:  can't bind to NIS server for domain %s.  Reason:  %s.\n";
char err_first_failed[] =
	"ypcat:  can't get first record from NIS.  Reason:  %s.\n";
char err_next_failed[] =
	"ypcat:  can't get next record from NIS.  Reason:  %s.\n";
char *transtable[] = {
	"passwd", "passwd.byname",
	"group", "group.byname",
	"networks", "networks.byaddr",
	"hosts", "hosts.byaddr",
	"protocols","protocols.bynumber",
	"services","services.byname",
	"aliases", "mail.aliases",
	"ethers", "ethers.byname",
	NULL
};

void get_command_line_args();
void dumptable();
int callback();
void one_by_one_all();

/*
 * This is the mainline for the ypcat process.  It pulls whatever arguments
 * have been passed from the command line, and uses defaults for the rest.
 */

void
main (argc, argv)
	int argc;
	char **argv;
	
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;
	int fail=0;
	int i;
	struct ypall_callback cbinfo;
	
	get_command_line_args(argc, argv);

	if (dodump) {
		dumptable();
		exit(0);
	}

	if (!domain) {
		
		if (!getdomainname(default_domain_name, YPMAXDOMAIN) ) {
			domain = default_domain_name;
		} else {
			fprintf(stderr, err_cant_get_kname, err_bad_domainname);
			exit(1);
		}

		if (strlen(domain) == 0) {
			fprintf(stderr, err_null_kname, err_bad_domainname);
			exit(1);
		}
	}

	if (err = yp_bind(domain) ) {
		fprintf(stderr, err_cant_bind, domain,
		    yperr_string(err) );
		exit(1);
	}

	key = nullstring;
	keylen = 0;
	val = nullstring;
	vallen = 0;
	
	if (translate) {
		for (i = 0; transtable[i]; i+=2)
			if (strcmp(map, transtable[i]) == 0) {
			    map = transtable[i+1];
			    break;
			}
	}

	cbinfo.foreach = callback;
	cbinfo.data = (char *) &fail;
	err = yp_all(domain, map, &cbinfo);

	if (err == YPERR_VERS) {
		one_by_one_all(domain, map);
	}
	
	exit(fail);
	/* NOTREACHED */
}

/*
 * This does the command line argument processing.
 */
static void
get_command_line_args(argc, argv)
	int argc;
	char **argv;
	
{

	argv++;
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 't': 
				translate = FALSE;
				argv++;
				break;

			case 'k': 
				dumpkeys = TRUE;
				argv++;
				break;
				
			case 'x': 
				dodump = TRUE;
				argv++;
				break;

			case 'd': 

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						fprintf(stderr, err_bad_args,
						    err_bad_domainname);
						exit(1);
					}
					
				} else {
					fprintf(stderr, err_usage);
					exit(1);
				}
				
				break;
				
			default: 
				fprintf(stderr, err_usage);
				exit(1);
			}
			
		} else {
			
			if (!map) {
				map = *argv;
				argv++;
			} else {
				fprintf(stderr, err_usage);
				exit(1);
			}
		}
	}

	if (!map && !dodump) {
		fprintf(stderr, err_usage);
		exit(1);
	}
}
/*
 * This will print out the map nickname translation table.
 */
static void
dumptable()
{
	int i;

	for (i = 0; transtable[i]; i += 2) {
		printf("Use \"%s\" for map \"%s\"\n", transtable[i],
		    transtable[i + 1]);
	}
}

/*
 * This dumps out the value, optionally the key, and perhaps an error message.
 */
static int
callback(status, key, kl, val, vl, fail)
	int status;
	char *key;
	int kl;
	char *val;
	int vl;
	int *fail;
{
	int e;
	
	if (status == YP_TRUE) {

		if (dumpkeys)
			(void) printf("%.*s ", kl, key);

		(void) printf("%.*s\n", vl, val);
		return (FALSE);
	} else {

		e = ypprot_err(status);

		if (e != YPERR_NOMORE) {
			(void) fprintf(stderr, "%s\n", yperr_string(e));
			*fail = TRUE;
		}
		
		return (TRUE);
	}
}

/*
 * This cats the map out by using the old one-by-one enumeration interface.
 * As such, it is prey to the old-style problems of rebinding to different
 * servers during the enumeration.
 */
static void
one_by_one_all(domain, map)
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;

	key = nullstring;
	keylen = 0;
	val = nullstring;
	vallen = 0;
	
	if (err = yp_first(domain, map, &outkey, &outkeylen, &val, &vallen) ) {

		if (err == YPERR_NOMORE) {
			exit(0);
		} else {
			fprintf(stderr, err_first_failed,
			    yperr_string(err) );
			exit(1);
		}
	}

	while (TRUE) {

		if (dumpkeys) {
			printf("%.*s ", outkeylen, outkey);
		}

		printf("%.*s\n", vallen, val);
		free(val);
		key = outkey;
		keylen = outkeylen;
		
		if (err = yp_next(domain, map, key, keylen, &outkey, &outkeylen,
		    &val, &vallen) ) {

			if (err == YPERR_NOMORE) {
				break;
			} else {
				fprintf(stderr, err_next_failed,
				    yperr_string(err) );
				exit(1);
			}
		}

		free(key);
	}
}
