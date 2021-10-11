SunDefaults_Version 2
;	@(#)Indent.d	1.1	92/07/30 SMI
/Indent			""
//$Specialformat_to_defaults "indentpro_to_defaults"
//$Defaults_to_specialformat "defaults_to_indentpro"

//Indentation	"8"
; -in. Default = 8.
	$Help		"Specifies the number of spaces for one indentation level."
	$Enumeration	""
	8		""
	8/$Help		"Indent 8 spaces for each level of indentation."
	4		""
	4/$Help		"Indent 4 spaces for each level of indentation."
	
//Indentation_in_declarations	"16"
	$Help		"Indentation from the beginning of a declaration keyword to the next identifier."
; -din. Default = 16.

//Line_length	"75"
	$Help		"Maximum length of an output line."
; -ln. Default = 75.

//Case_labels	"Align_with_switch"
; -clin. Default = -cli0.
	$Enumeration
	$Help		"Controls placement of case labels relative to switch."
	Align_with_switch	""
	Align_with_switch/$Help	"Align case labels with switch."
	Indent_half_tab		""
	Indent_half_tab/$Help	"Indent case labels 1/2 tab stop from switch."

//Trailing_comments_start_in_column	"33"
	$Help		"The column in which comments other than block comments start."
; -cn. Default = 33.

//Typedefs	""
	$Help		"Defines a list of type keywords, e.g., Panel Menu Textsw."


//Mess1/$Message	""

//Mess4/$Message	"    ~~~~~~ declarations ~~~~~~"

//Parameter_declarations	"Indented"
; -ip, -nip. Default is -ip.
        $Enumeration	""
        $Help           "Specifies the indentation of parameter declarations from the left margin."
	Indented		""
        Indented/$Help           "Parameter declarations are indented one level from the left margin."
	In_column_1		""
	In_column_1/$Help           "Parameter declarations start in column 1."

//Multiple_declarations		"On_same_lines"
; -bc, -nbc. Default = -nbc.
        $Enumeration	""
        $Help           "Enables (disables) multiple declarations of the same type on the same line."
	On_same_lines		""
        On_same_lines/$Help           "Multiple declarations of same type can apear on same line, e.g. int x, y, z;"
	On_separate_lines		""
	On_separate_lines/$Help           "Force multiple declarations onto separate lines."


//Return_type_of_procedure		"On_previous_line"
; -psl, -npsl. Default = -psl.
        $Enumeration	""
        $Help           "Controls placement of the return type of a procedure relative to the procedure name."
	On_previous_line		""
        On_previous_line/$Help           "Place return type of procedure on previous line from name of procedure."
	On_same_line		""
	On_same_line/$Help           "Place return type of procedure on same line as name of procedure."


//Mess5/$Message	""
//Mess6/$Message	"    ~~~~~~ block comments ~~~~~~"

//Comment_delimiters	"On_separate_line"
; -cdb, -ncdb, Default = -cdb.
        $Enumeration	""
        $Help           "Enables/disables the placement of block comment delimiters (/* and */) on separate lines."
	On_separate_line		""
        On_separate_line/$Help           "Comment delimiters for block comments will always be on separate lines."
	On_same_line		""
	On_same_line/$Help           "Comment delimiters for block comments will be place on the same line as the comment."

//Asterisk_at_left_edge	"Yes"
; -sc, -nsc, Default = -sc.
        $Enumeration	""
        $Help           "Enables (disables) the placement of *'s at the left edge of each line of a block comment."
	Yes		""
	Yes/$Help	"A * will appear at the left edge of each line of a block comment."
	No
	No/$Help	"A * will NOT appear at the left edge of each line of a block comment."

//Indent_block_comments		"Same_as_commented_code"
; -dn. Default = -d0.
        $Enumeration	""
        $Help           "Controls placement of block comments."
	Same_as_commented_code		""
	Same_as_commented_code/$Help	"Block comments line up with code."
	Indent_LEFT_one_level
	Indent_LEFT_one_level/$Help	"BLock comments are placed one indentation level to the LEFT of the code."

//Format_comments_in_column_1	"Yes"
; -fc1, -nfc1. Default = -fc1.
	$Help	"Enables (disables) the formatting of comments that start in column 1."
	$Enumeration
	Yes
	Yes/$Help	"Comments that start in column 1 are formatted."
	No
	No/$Help	"Comments that start in column 1 are left alone."


//Mess7/$Message	""
//Mess8/$Message	"    ~~~~~~ blank lines ~~~~~~"

//Blank_line_after_proc_body	 "Yes"
; -bap, nbap. Default = -bap.
	$Help	"Controls insertion of blank lines following procedure bodies."
	$Enumeration
	Yes
	Yes/$Help	"Blank line inserted after every procedure body."
	No
	No/$Help	"Blank line is NOT inserted after procedure body."

//Blank_line_after_decl_block	 "Yes"
; -bad, -nbad. Default = -bad.
	$Help	"Controls insertion of blank lines following block of declarations."
	$Enumeration
	Yes
	Yes/$Help	"Blank line inserted after every block of declarations."
	No
	No/$Help	"Blank line is NOT inserted after block of declarations."

//Blank_line_before_block_comment	 "No"
; -bbb,-nbbb. Default = -nbbb.
	$Help	"Controls insertion of blank lines before block comments."
	$Enumeration
	Yes
	Yes/$Help	"Blank line inserted before every block commented."
	No
	No/$Help	"Blank line is NOT inserted before block comments."

//Swallow_optional_blank_lines	"No"
; -sob, -nsob. Default = -nsob.
	$Help	"Enables (disables) swallowing of optional blank lines, e.g., after declarations."
	$Enumeration
	Yes
	Yes/$Help	"Indent will swallow optional blank lines, e.g., after declarations."
	No
	No/$Help	"Blank lines are not swallowed."


//Mess9/$Message	""
//Mess10/$Message	"    ~~~~~~  braces ~~~~~~"

//Left_brace	"On_same_line"
; -br, -bl. Default = -br.
	$Help	"Controls placement of {'s in compound statements."
	$Enumeration	""
	On_same_line		""
        On_same_line/$Help	"Left brace appears on same line as preceding code, e.g., if (...) {"
	On_separate_line	""
	On_separate_line/$Help           "Left brace always appears on a line by itself."

//Right_brace_and_else	"On_same_line"
; -ce, -nce. Default = -ce.
        $Enumeration	""
        $Help           "Enables (disables) forcing else's to cuddle up to the immediately preceding }"
	On_same_line		""
        On_same_line/$Help           "else appears on same line as preceding }, e.g. } else {"
	On_separate_line		""
	On_separate_line/$Help		"else appears on line after }"


//Mess11/$Message	""
//Mess12/$Message	"    ~~~~~~ miscellaneous  ~~~~~~"

	
//Space_after_proc_call		"No"
; -pcs, -npcs. Default = -npcs.
        $Enumeration	""
        $Help           "Controls insertion of space between procedure name and '(' in procedure call."
	Yes		""
        Yes/$Help           "Insert space between procedure name and '(', e.g., foo (args)"
	No		""
	No/$Help		"Don't insert space between procedure name and '(', e.g., foo(args)"

//Align_parenthesized_code		"Yes"
; -lp, -nlp. Default = -lp.
        $Enumeration	""
        $Help           "Controls formatting of continuation lines."
	Yes		""
        Yes/$Help           "When inside parentheses, align continuation lines at position of '(' + 1"
	No		""
	No/$Help		"Being inside parentheses does not affect formatting of continuation lines."


//Other_options		""
	$Help		"A string of options to be passed in to indent, e.g. -nbs -troff."
