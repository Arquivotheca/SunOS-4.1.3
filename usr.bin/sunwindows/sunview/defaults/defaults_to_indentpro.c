#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)defaults_to_indentpro.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
/* last edited on December 27, 1985, 17:00:43 */

#include <strings.h>
#include <sunwindow/string_utils.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <strings.h>
#include <ctype.h>
#include <sunwindow/defaults.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

static char    *bap  = "Blank_line_after_proc_body";
static char    *bad  = "Blank_line_after_decl_block";
static char    *bbb  = "Blank_line_before_block_comment";
static char    *bc   = "Multiple_declarations";
static char    *br   = "Left_brace";
static char    *cdb  = "Comment_delimiters";
static char    *ce   = "Right_brace_and_else";
static char    *fc1  = "Format_comments_in_column_1";
static char    *ip   = "Parameter_declarations";
static char    *lp   = "Align_parenthesized_code";
static char    *pcs  = "Space_after_proc_call";
static char    *psl  = "Return_type_of_procedure";
static char    *sc   = "Asterisk_at_left_edge";
static char    *sob  = "Swallow_optional_blank_lines";
static char    *clin = "Case_labels";
static char    *din  = "Indentation_in_declarations";
static char    *ln   = "Line_length";
static char    *in   = "Indentation";
static char    *dn   = "Indent_block_comments";
static char    *cn   = "Trailing_comments_start_in_column";

static char     options[256];
static char     typedefs[256];
static char    *delimited_prefix = "/Indent/";
static char     scratch[256];

static void compute_string() ;
static char *get_string();
static Bool get_boolean();

/* ARGSUSED */
#ifdef STANDALONE
main(argc, argv)
#else
defaults_to_indentpro_main(argc, argv)
#endif
	int             argc;
	char          **argv;
{
	FILE           *stream;
	char	msg[128];

	compute_string();
	(void)sprintf(scratch, "%s/.indent.pro", getpwuid(getuid())->pw_dir);
	/* getpwuid expression computes home directory */
	if ((stream = fopen(scratch, "w")) == NULL) {
		(void)sprintf(msg, "defaults_to_indentpro: %s", scratch);
		perror(msg);
		exit(1);
	}
	fputs(options, stream);
	(void)fclose(stream);
	
	EXIT(0);
}

static void
compute_string()
{
	char           *s;
	int		i;
	

	options[0] = '\0';
	typedefs[0] = '\0';	/* Put -T in front of each typedef */
	for (i = 0, s = get_string("Typedefs");
		string_get_token(s, &i, scratch, white_space); ) {
		(void)strcat(typedefs, " -T");
		(void)strcat(typedefs, scratch);
	}
	(void)sprintf(options, " -%s -%s -%s -%s -%s -%s -%s -%s -%s -%s -%s -%s -%s -%s -%s -di%d -l%d -i%s  -%s -c%d %s %s",
		(get_boolean(bap) ? "bap" : "nbap"),
		(get_boolean(bad) ? "bad" : "nbad"),
		(get_boolean(bbb) ? "bbb" : "nbbb"),
		(strequal(get_string(bc), "On_separate_lines") ? "bc" : "nbc"),
		(strequal(get_string(br), "On_same_line") ? "br" : "bl"),
		(strequal(get_string(cdb), "On_separate_line") ? "cdb" : "ncdb"),
		(strequal(get_string(ce), "On_same_line") ? "ce" : "nce"),
		(get_boolean(fc1) ? "fc1" : "nfc1"),
		(strequal(get_string(ip), "Indented") ? "ip" : "nip"),
		(get_boolean(lp) ? "lp" : "nlp"),
		(get_boolean(pcs) ? "pcs" : "npcs"),
		(strequal(get_string(psl), "On_previous_line") ? "psl" : "npsl"),
		(get_boolean(sc) ? "sc" : "nsc"),
		(get_boolean(sob) ? "sob" : "nsob"),
		(strequal(get_string(clin), "Align_with_switch") ? "cli0" : "cli0.5"),
		get_integer(din),
		get_integer(ln),
		get_string(in),	/* indendation is an enumeration consisting of either "8" or "4" */	
		(strequal(get_string(dn), "Same_as_commented_code") ? "d0" : "d1"),
		get_integer(cn),
		typedefs,
		get_string("Other_options")
		);
}

static char *
get_string(name)
	char           *name;
{
	(void)sprintf(scratch, "%s%s", delimited_prefix, name);
	return (defaults_get_string(scratch, "", (int *)NULL));
}

static Bool
get_boolean(name)
	char           *name;
{
	(void)sprintf(scratch, "%s%s", delimited_prefix, name);
	return (defaults_get_boolean(scratch, (Bool) NULL, (int *)NULL)); 
}

static int
get_integer(name)
	char           *name;
{
	(void)sprintf(scratch, "%s%s", delimited_prefix, name);
	return (defaults_get_integer(scratch, (int)NULL, (int *)NULL));
}
