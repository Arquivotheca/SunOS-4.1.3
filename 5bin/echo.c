#ifndef lint
static	char sccsid[] = "@(#)echo.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

main(argc, argv)
char **argv;
{
	register char	*cp;
	register int	i, wd;
	int	j;

	if(--argc == 0) {
		putchar('\n');
		exit(0);
	}
	for(i = 1; i <= argc; i++) {
		for(cp = argv[i]; *cp; cp++) {
			if(*cp == '\\')
			switch(*++cp) {
				case 'b':
					putchar('\b');
					continue;

				case 'c':
					exit(0);

				case 'f':
					putchar('\f');
					continue;

				case 'n':
					putchar('\n');
					continue;

				case 'r':
					putchar('\r');
					continue;

				case 't':
					putchar('\t');
					continue;

				case 'v':
					putchar('\v');
					continue;

				case '\\':
					putchar('\\');
					continue;
				case '0':
					j = wd = 0;
					while ((*++cp >= '0' && *cp <= '7') && j++ < 3) {
						wd <<= 3;
						wd |= (*cp - '0');
					}
					putchar(wd);
					--cp;
					continue;

				default:
					cp--;
			}
			putchar(*cp);
		}
		putchar(i == argc? '\n': ' ');
	}
	exit(0);
	/* NOTREACHED */
}
