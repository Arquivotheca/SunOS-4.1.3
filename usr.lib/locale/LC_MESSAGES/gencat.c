/*	@(#)gencat.c 1.1 92/07/30 SMI	*/

#include <stdio.h>
#include <ctype.h>
#include <nl_types.h>
#include <locale.h>
#include <signal.h>

/*
 * gencat : take a message source file and produce a message catalog.
 * This generates a tmp file that can be used as input to the 
 * installtxt utility that really does the work. We always name the 
 * tmpffile ".Xgencat", so that it can always be merged later
 *
 */

#define CATNAME	"/tmp/.Xgencat"

char 	buff[512];
char 	linebuf[512];
int 	set_no;
char	tname[2*L_tmpnam];

FILE *tempfile;

main(argc, argv)
	int argc;
	char **argv;
{
	char *catf;
	FILE *fd;
	register int i;
	int merge;
	int ret;
	
	if (argc == 1)
		usage(argv[0]);
		
	/*
	 * Create a tempfile for installtxt. Call it CATNAME
	 */

	strcpy(tname, CATNAME);
	(void) unlink(CATNAME);
	tempfile = fopen(tname,"w+");
	if (tempfile == NULL) {
		perror(tname);
		exit(1);
	}
	
	/*
	 * Check to see if catalogue is there, if it is we will merge it
	 * later
	 */

	catf = argv[1];
	
	if (access(catf, 4) == 0)
		merge = 1;

	/*
	 * Open msgfile(s) or use stdin and call handling proc
	 */

	if (argc == 2)
		make_inter(stdin);
	else {
		for (i = 2 ; i < argc ; i++){
			if ((fd = fopen(argv[i], "r")) == 0){
			   perror(argv[i]);
			   continue;
			}
			make_inter(fd);
			fclose(fd);
		}
	}
	
	/* Do the installtxt cv catd tempfile
	 * then close fds
	 * need to use merge flag if necc
	 */
	fclose(tempfile);
	i = fork();
	if(i == -1) {
		perror(NULL);
	      	done(1);
	}

	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGTSTP, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);

	if (i == 0) {
	   if (merge) {
		execl("/usr/etc/installtxt", "installtxt", "r", catf, tname, (char *) 0);
		execl("/etc/installtxt", "installtxt", "r", catf, tname, (char *) 0);
	   } else {
		execl("/usr/etc/installtxt", "installtxt", "c", catf, tname, (char *) 0);
		execl("/etc/installtxt", "installtxt", "c", catf, tname, (char *) 0);
	   }	
	      fprintf(stderr, "could not exec installtxt\n");
	      exit(1);
	}
	if ((ret = wait((int *) 0))== -1) {
		perror(NULL);
		done(1);
	}
	(void) unlink(tname);  
	exit(0);

}


usage(name)
	char *name;
{
	fprintf(stderr, "Usage: %s object [source]\n", name);
	exit(0);
}



make_inter(fd)
	FILE *fd;
{
	char quote = 0;
	register int t1, t2;
	char c;
	int msg_nr;
	int line_count;

	
	fprintf(tempfile, "$quote \"\n");
	set_no = 0; line_count=0;
	while (fgets(linebuf, BUFSIZ, fd) != 0){
		line_count++;
		while (strcmp(&linebuf[strlen(linebuf)-2],"\\\n") == 0) {
			line_count++;
			if (fgets(&linebuf[strlen(linebuf)-2], BUFSIZ, fd) == 0) {
				fprintf(stderr, "line continuation expected on line %d\n",
					line_count);
				done(1);
			}


			if (strlen(linebuf) > BUFSIZ) {
				fprintf(stderr, "line too long: line %d\n", line_count);
				done(1);
			}
		}

		if ((t1 = skip_blanks(linebuf, 0)) == -1)
			continue;

		if (linebuf[t1] == '$'){
			t1 += 1;
			/*
			 * Handle commands or comments
			 */
			if (strncmp(&linebuf[t1], "set", 3) == 0){
				t1 += 3;

				/*
				 * Change current set
				 */
				if ((t1 = skip_blanks(linebuf, t1)) == -1){
					fprintf(stderr, "Syntax error on line %d\n", line_count);
					continue;
				}
				set_no = atoi(&linebuf[t1]);
				continue;
			}
			if (strncmp(&linebuf[t1], "delset", 6) == 0){
				t1 += 6;

				/*
				 * Delete named set
				 */
				if ((t1 = skip_blanks(linebuf, t1)) == -1){
					fprintf(stderr, "Syntax error on line %d (Ignored)\n", line_count);
					continue;
				}

				/* not implemented */
				continue;

			}
			if (strncmp(&linebuf[t1], "quote", 5) == 0){
				t1 += 5;
				if ((t1 = skip_blanks(linebuf, t1)) == -1)
					quote = 0;
				else
					quote = linebuf[t1];
				continue;
			}
			/*
			 * Everything else is a comment
			 */
			continue;
		}

		/* A real message id */

		if (isdigit(linebuf[t1])){
			msg_nr = 0;
			while(isdigit(c = linebuf[t1])){
				msg_nr *= 10;
				msg_nr += c - '0';
				t1++;
			}
			if ((t1 = skip_blanks(linebuf, t1)) == -1) {
				fprintf(stderr, 
					"Syntax error on line %d (ignored)\n", 
					line_count);
				linebuf[0] = 0;
			}	
			else {
				/* null out the trailing newline */
				if (linebuf[t1] == quote) {
					/* test for unbalanced quote pairs */
					if(linebuf[strlen(linebuf)-2] != '\"')
					 strcpy(&linebuf[strlen(linebuf)-1],
						"\"");
					else
					  linebuf[strlen(linebuf)-1] = '\0';
					fprintf(tempfile,"%d:%d %s\n",
						set_no, msg_nr, linebuf+t1);
				}
				else {
					linebuf[strlen(linebuf)-1] = '\0';
					if (strcmp(&linebuf[strlen(linebuf)-2],
						"\\\"") == 0)
					  fprintf(tempfile,"%d:%d \"%s\n", 
						set_no, msg_nr, linebuf+t1);
					else
					  fprintf(tempfile,"%d:%d \"%s\"\n", 
						set_no, msg_nr, linebuf+t1);
                                                
				}
			}
		}

		continue;
	}
		
}

  /*
   * Skip blanks in the input stream
   */


skip_blanks(lineptr, i)
  char *lineptr;
  int i;
{
	while (lineptr[i] && isspace(lineptr[i]) && !iscntrl(lineptr[i]))
		i++;
	if (!lineptr[i] || lineptr[i] == '\n')
		return -1;
	return i;
}

done(code)
int code;
{

	(void) fclose(tempfile);
	(void) unlink(tname);  
	exit(code);

}
