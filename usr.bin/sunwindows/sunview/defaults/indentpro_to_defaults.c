#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)indentpro_to_defaults.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
/* last edited on December 27, 1985, 16:47:00 */

#include <strings.h>
#include <sunwindow/string_utils.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <strings.h>
#include <ctype.h>
#include <sunwindow/defaults.h>

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
static char     other_options[128];

static char     options[128];
static char     typedefs[256];
static char    *delimited_prefix = "/Indent/";
static char     scratch[256];

static void parse(), flush_defaults(), set_string(), set_enumeration(), set_boolean(), set_integer();

/* ARGSUSED */
#ifdef STANDALONE
main(argc, argv)
#else
indentpro_to_defaults_main(argc, argv)
#endif
	int             argc;
	char          **argv;
{
	FILE           *stream;
	char	msg[128];

	(void)sprintf(scratch, "%s/.indent.pro", getpwuid(getuid())->pw_dir);
	/* getpwuid expression computes home directory */
	if ((stream = fopen(scratch, "a+")) == NULL) {
		(void)sprintf(msg, "indentpro_to_defaults: %s", scratch);
		perror(msg);
		_exit(1);	/* no .indent.pro file */
	}
	/* Because of the fopen append we must reposition the file */
        fseek(scratch, (long)0, 0);

	(void)fgets(options, 1000, stream);
	(void)fclose(stream);
	parse(options);
	flush_defaults();
	exit(0);
}


/*
 * parses str and sets variables accordingly. Used to parse both .indent.pro
 * as well as the contents of the others_options. 
 */
static void
parse(str)
	char           *str;
{
	int             i = 0;

	while (string_get_token(str, &i, scratch, white_space)) {
/* check choice items (in order of indent man page) */
		if (strequal(scratch, "-bap"))
			set_boolean(bap, (int)True);
		else if (strequal(scratch, "-nbap"))
			set_boolean(bap, (int)False);
		else if (strequal(scratch, "-bad"))
			set_boolean(bad, (int)True);
		else if (strequal(scratch, "-nbad"))
			set_boolean(bad, (int)False);
		else if (strequal(scratch, "-bbb"))
			set_boolean(bbb, (int)True);
		else if (strequal(scratch, "-nbbb"))
			set_boolean(bbb, (int)False);
		else if (strequal(scratch, "-bc"))
			set_enumeration(bc, "On_separate_lines");
		else if (strequal(scratch, "-nbc"))
			set_enumeration(bc, "On_same_lines");
		else if (strequal(scratch, "-br"))
			set_enumeration(br, "On_same_line");
		else if (strequal(scratch, "-bl"))
			set_enumeration(br, "On_separate_line");
		else if (strequal(scratch, "-cdb"))
			set_enumeration(cdb, "On_separate_line");
		else if (strequal(scratch, "-ncdb"))
			set_enumeration(cdb, "On_same_line");
		else if (strequal(scratch, "-ce"))
			set_enumeration(ce, "On_same_line");
		else if (strequal(scratch, "-nce"))
			set_enumeration(ce, "On_separate_line");
		else if (strequal(scratch, "-fc1"))
			set_boolean(fc1, (int)True);
		else if (strequal(scratch, "-nfc1"))
			set_boolean(fc1, (int)False);
		else if (strequal(scratch, "-ip"))
			set_enumeration(ip, "Indented");
		else if (strequal(scratch, "-nip"))
			set_enumeration(ip, "In_column_1");
		else if (strequal(scratch, "-lp"))
			set_boolean(lp, (int)True);
		else if (strequal(scratch, "-nlp"))
			set_boolean(lp, (int)False);
		else if (strequal(scratch, "-pcs"))
			set_boolean(pcs, (int)True);
		else if (strequal(scratch, "-npcs"))
			set_boolean(pcs, (int)False);
		else if (strequal(scratch, "-psl"))
			set_enumeration(psl, "On_previous_line");
		else if (strequal(scratch, "-npsl"))
			set_enumeration(psl, "On_same_line");
		else if (strequal(scratch, "-sc"))
			set_boolean(sc, (int)True);
		else if (strequal(scratch, "-nsc"))
			set_boolean(sc, (int)False);
		else if (strequal(scratch, "-sob"))
			set_boolean(sob, (int)True);
		else if (strequal(scratch, "-nsob"))
			set_boolean(sob, (int)False);

/* this ends the booleans. Now check for options that specify parameters */

		else if (substrequal(scratch, 0, "-T", 0, 2, True)) {
			if (string_find(typedefs, scratch + 2, True) == -1) {
				(void)strcat(typedefs, " ");
				(void)strcat(typedefs, scratch + 2);
			/*
			 * there may be more than one -T. gather them up and
			 * make a single call to set_string after processing
			 * entire options string 
			 */
			}
		} else if (substrequal(scratch, 0, "-cli", 0, 4, True))
			switch ((int)(2 * atof(scratch + 4))) {
			    case 0:
				set_enumeration(clin, "Align_with_switch");
				break; 
			    case 1:	/* i.e. -cli0.5 */
				set_enumeration(clin, "Indent_half_tab");
				break; 
			    default:
				/* doesn't map into a choice provided by item in Indents.d; put this option into other_options */
				(void)strcat(other_options, scratch);
				(void)strcat(other_options, " ");
			    	break;
			 }
		else if (substrequal(scratch, 0, "-di", 0, 3, True))
			set_integer(din, atoi(scratch + 3));
		else if (substrequal(scratch, 0, "-d", 0, 2, True))
			switch (atoi(scratch + 2)) {
			    case 0:
				set_enumeration(dn, "Same_as_commented_code");
				break; 
			    case 1:	
				set_enumeration(dn, "Indent_LEFT_one_level");
				break; 
			    default:
				/* doesn't map into a choice provided by item in Indents.d; put this option into other_options */
				(void)strcat(other_options, scratch);
				(void)strcat(other_options, " ");
			    	break;
			 }
		else if (substrequal(scratch, 0, "-i", 0, 2, True)) {
			switch (atoi(scratch + 2)) {
			case 8:
				set_enumeration(in, "8");
				break;
			case 4:
				set_enumeration(in, "4");
				break;
			default:
				/*
				 * doesn't map into a choice provided by item
				 * in Indents.d; put this option into
				 * other_options 
				 */
				(void)strcat(other_options, scratch);
				(void)strcat(other_options, " ");
				break;
			}
		} else if (substrequal(scratch, 0, "-l", 0, 2, True))
			set_integer(ln, atoi(scratch + 2));
		else if (substrequal(scratch, 0, "-c", 0, 2, True))
			set_integer(cn, atoi(scratch + 2));
		else  {
			/* e.g. -v, -bs, or new options added to indent */			(void)strcat(other_options, scratch);
			(void)strcat(other_options, " ");
		}
	}
	set_string("Typedefs", typedefs);
	set_string("Other_options", other_options);

}


static void
flush_defaults()
{
	defaults_write_changed((char *)NULL, (int *)NULL);
}

static void
set_string(name, value)
	char           *name;
	char           *value;
{
	(void)sprintf(scratch, "%s%s", delimited_prefix, name);
	defaults_set_string(scratch, value, (int *)NULL);
}

static void
set_enumeration(name, value)
	char           *name;
	char           *value;
{
	(void)sprintf(scratch, "%s%s", delimited_prefix, name);
	defaults_set_enumeration(scratch, value, (int *)NULL);
}

static void
set_boolean(name, value)
	char           *name;
	int             value;
{
	set_enumeration(name, value ? "Yes" : "No");
}

static void
set_integer(name, value)
	char           *name;
	int             value;
{
	(void)sprintf(scratch, "%s%s", delimited_prefix, name);
	defaults_set_integer(scratch, value, (int *)NULL);
}
