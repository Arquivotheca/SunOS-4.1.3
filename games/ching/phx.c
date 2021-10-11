#include <stdio.h>

/*
 * Select hexagram(s) from the I Ching database
 * based on coin-toss results of the form NNNNNN
 * where N is 6-9.  
 * Written by Tom Lyon in fond remembrance of Ralph Muha whose
 * source never made it to a distribution tape.
 */
#define HEXAGRAMS "hexagrams"

char *toss;

main(argc, argv)
	char **argv;
{
	if (argc != 2 || strlen(argv[1]) != 6) {
		fprintf(stderr, "Usage: phx NNNNNN\n");
		exit(1);
	}
	toss = argv[1];
	gram();
	if (moving())
		gram();
	exit(0);
	/* NOTREACHED */
}

/*
 * Moving lines: 6->7, 9->8
 */
moving()
{
	int found = 0;
	int i;

	for (i=0; i<6; i++) {
		if (toss[i] == '6') {
			toss[i] = '7';
			found = 1;
		}
		if (toss[i] == '9') {
			toss[i] = '8';
			found = 1;
		}
	}
	return (found);
}

/*
 * This table stolen from binary of old 'phx'
 * It implements the 'proper' hexagram selection
 * based on coin toss.
 */
int chingtab[64] = {
	1,	44,	13,	33,
	10,	6,	25,	12,
	9,	57,	37,	53,
	61,	59,	42,	20,
	14,	50,	30,	56,
	38,	64,	21,	35,
	26,	18,	22,	52,
	41,	4,	27,	23,
	43,	28,	49,	31,
	58,	47,	17,	45,
	5,	48,	63,	39,
	60,	29,	3,	8,
	34,	32,	55,	62,
	54,	40,	51,	16,
	11,	46,	36,	15,
	19,	7,	24,	2,
};

/*
 * Select a hexagram
 */
gram()
{
	int i;
	int val = 0;
	int found=0, print=0;
	char line[200];
	FILE *hf;

	for (i=5; i>=0; i--) switch (toss[i]) {
	case '6': case '8':
		val = val*2 + 1;
		break;
	case '7': case '9':
		val = val*2 + 0;
		break;
	}
	val = chingtab[val];

	if ((hf = fopen(HEXAGRAMS, "r")) == NULL) {
		fprintf(stderr, "phx: cannot open %s\n", HEXAGRAMS);
		exit(1);
	}
	while (fgets(line, 200, hf) != NULL) {
		if (strncmp(line, ".H ", 3) == 0) {
			int h;

			if (found) 
				break;
			sscanf(line, ".H %d", &h);
			if (h == val)
				found = print = 1;
		}
		if (found && strncmp(line, ".L ", 3) == 0) {
			int lno;
			char lval[2];

			sscanf(line, ".L %d %s", &lno, lval);
			if (toss[lno - 1] == lval[0])
				print = 1;
			else
				print = 0;
		}
		if (strncmp(line, ".LA ", 4) == 0) {
			char lval[2];

			sscanf(line, ".LA %s", lval);
			print = 1;
			for (i=0; i<6; i++)
				print = print && (toss[i] == lval[0]);
		}
		if (print)
			fputs(line, stdout);
	}
	fclose(hf);
}
