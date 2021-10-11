#ifndef lint
static	char sccsid[] = "@(#) mkglue.c 1.1 92/07/30"; /* from UCB 1.10 83/07/09 */
#endif

/*
 * Make the uba interrupt file ubglue.s or mb interrupt file mbglue.s
 */
#include <stdio.h>
#include "config.h"
#include "y.tab.h"
#include <sun/autoconf.h>

/*
 * print an interrupt handler for unibus
 */
dump_ub_handler(fp, vec, number, name)
	register FILE *fp;
	register struct vlst *vec;
	int number;
	char *name;
{
	char nbuf[80];
	register char *v = nbuf;

#ifdef lint
	name = name;
#endif
	(void) sprintf(v, "%s%d", vec->v_id, number);
	fprintf(fp, "\t.globl\t_X%s\n\t.align\t2\n_X%s:\n\tpushr\t$0x3f\n",
	    v, v);
	if (strncmp(vec->v_id, "dzx", 3) == 0)
		fprintf(fp, "\tmovl\t$%d, r0\n\tjmp\tdzdma\n\n", number);
	else {
		if (strncmp(vec->v_id, "uur", 3) == 0) {
			fprintf(fp, "#ifdef UUDMA\n");
			fprintf(fp, "\tmovl\t$%d, r0\n\tjsb\tuudma\n", number);
			fprintf(fp, "#endif\n");
		}
		fprintf(fp, "\tpushl\t$%d\n", number);
		fprintf(fp, "\tcalls\t$1, _%s\n\tpopr\t$0x3f\n", vec->v_id);
		fprintf(fp, "#if defined(VAX750) || defined(VAX730)\n");
		fprintf(fp, "\tincl\t_cnt+V_INTR\n#endif\n\trei\n\n");
	}
}

char *vec_intrnames[100];
int vec_intrcnt = 0;

/*
 * print an interrupt handler for mainbus
 */
dump_mb_handler(fp, vec, number, name)
	register FILE *fp;
	register struct vlst *vec;
	int number;
	char *name;
{
	if (machine == MACHINE_SUN4) {
		fprintf(fp, "\t.word\t%s%d, %d\n",
			vec->v_id, number, number);
	} else {
		fprintf(fp, "\tVECINTR(_X%s%d, _%s, _V%s%d, %d)\n", vec->v_id,
		    number, vec->v_id, vec->v_id, number, vec_intrcnt);

		/* keep track of interrupt counts and device names for vmstat */
		vec_intrnames[vec_intrcnt] = (char *)malloc(strlen(name) + 1);
		strcpy(vec_intrnames[vec_intrcnt], name);
		vec_intrcnt++;
	}
}

mbglue()
{
	register FILE *fp;
	char *name = "mbglue.s";
	register int i;

	fp = fopen(path(name), "w");
	if (fp == 0) {
		perror(path(name));
		exit(1);
	}
	fprintf(fp, "#include <machine/asm_linkage.h>\n\n");
	glue(fp, dump_mb_handler);

	/* make device name table */
	fprintf(fp, "\n.globl _intrnames\n.globl _eintrnames\n");
	fprintf(fp, ".data\n");
	fprintf(fp, "_intrnames:\n");
	for (i = 0; i < vec_intrcnt; i++) {
		fprintf(fp, "\t.asciz \"%s\"\n", vec_intrnames[i]);
	}
	fprintf(fp, "_eintrnames:\n");

	/* make space to hold interrupt counts */
	if (vec_intrcnt > 0) {
		fprintf(fp, "\n.bss\n.align 4\n");
		fprintf(fp, ".globl _intrcnt\n.globl _eintrcnt\n");
		fprintf(fp, "_intrcnt:\n");
		fprintf(fp, "\t.skip %d * 4\n", vec_intrcnt);
		fprintf(fp, "_eintrcnt:\n");
	}
	(void) fclose(fp);
}

ubglue()
{
	register FILE *fp;
	char *name = "ubglue.s";

	fp = fopen(path(name), "w");
	if (fp == 0) {
		perror(path(name));
		exit(1);
	}
	glue(fp, dump_ub_handler);
	(void) fclose(fp);
}

#define	NUNIQUE	512

int
glue(fp, dump_handler)
	register FILE *fp;
	register int (*dump_handler)();
{
	register i;
	auto struct device *unique[NUNIQUE];
	struct vlst *vd, *vd2;
	register struct device *dp, *mp;
	char name[100];

	if (machine == MACHINE_SUN4) {
		fprintf(fp, "\t.global	vme_vector\n");
		fprintf(fp, "vme_vector:\n");
	}

	bzero((caddr_t) unique, (sizeof (struct unique *)) * NUNIQUE);
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp == NULL || mp == TO_NEXUS || eq(mp->d_name, "mba"))
			continue;

		for (i = 0; unique[i] != NULL && i < NUNIQUE; i++) {
			if (eq(dp->d_name, unique[i]->d_name) &&
			    dp->d_unit == unique[i]->d_unit)
				break;
		}
		if (i >= NUNIQUE) {
			printf("Too many unique controllers\n");
			exit (2);
		}
		if (unique[i] || (vd = dp->d_vec) == 0)
			continue;
		unique[i] = dp;
		for (vd = dp->d_vec; vd; vd = vd->v_next) {
			for (vd2 = dp->d_vec; vd2; vd2 = vd2->v_next) {
				if (vd2 == vd) {
					(void) sprintf(name, "%s%d",
					    dp->d_name, dp->d_unit);
					(void) (*dump_handler)
					    (fp, vd, dp->d_unit, name);
					break;
				}
				if (!strcmp(vd->v_id, vd2->v_id))
					break;
			}
		}
	}
}
