#ifndef lint
static	char sccsid[] = "@(#)msgbuf.c 1.1 92/07/30 SMI"; /* from S5R2 1.4 */
#endif

# include	"messages.h"
# include	"lerror.h"

/* msgbuf is an array of indices into the message buffer file
 * each entry corresponds to a message in msgtext
 * if msgbuf[ i ] is 0 then the message i in not buffered
 * if msgbuf[ i ] is n then the message is in section n of the buffer file
 */

short	msgbuf[ NUMMSGS ] = {
	0,	/* [0] "%s evaluation order undefined" */
	0,	/* [1] "%s may be used before set" */
	0,	/* [2] "%s redefinition hides earlier one" */
	0,	/* [3] "%s set but not used in function %s" */
	0,	/* [4] "%s undefined" */
	0,	/* [5] "bad structure offset", */
	0,	/* [6] "%s unused in function %s" */
	0,	/* [7] "& before array or function: ignored" */
	0,	/* [8] "=<%c illegal" */
	0,	/* [9] "=>%c illegal" */
	0,	/* [10] "BCD constant exceeds 6 characters" */
	0,	/* [11] "a function is declared as an argument" */
	0,	/* [12] "ambiguous assignment: assignment op taken" */
	1,	/* [13] "argument %s unused in function %s" */
	0,	/* [14] "array of functions is illegal" */
	12,	/* [15] "assignment of different structures" */
	0,	/* [16] "bad asm construction" */
	0,	/* [17] "bad scalar initialization" */
	18,	/* [18] "cannot take address of %s" */
	0,	/* [19] "cannot initialize extern or union" */
	0,	/* [20] "case not in switch" */
	0,	/* [21] "comparison of unsigned with negative constant" */
	0,	/* [22] "constant argument to NOT" */
	0,	/* [23] "constant expected" */
	0,	/* [24] "constant in conditional context" */
	0,	/* [25] "constant too big for cross-compiler" */
	2,	/* [26] "conversion from long may lose accuracy" */
	3,	/* [27] "conversion to long may sign-extend incorrectly" */
	0,	/* [28] "declared argument %s is missing" */
	0,	/* [29] "default not inside switch" */
	0,	/* [30] "degenerate unsigned comparison" */
	0,	/* [31] "division by 0" */
	0,	/* [32] "division by 0." */
	0,	/* [33] "duplicate case in switch, %d" */
	0,	/* [34] "duplicate default in switch" */
	0,	/* [35] "empty array declaration" */
	0,	/* [36] "empty character constant" */
	5,	/* [37] "enumeration type clash, operator %s" */
	0,	/* [38] "field outside of structure" */
	0,	/* [39] "field too big" */
	0,	/* [40] "fortran declaration must apply to function" */
	0,	/* [41] "fortran function has wrong type" */
	0,	/* [42] "fortran keyword nonportable" */
	0,	/* [43] "function %s has return(e); and return;" */
	0,	/* [44] "function declaration in bad context" */
	0,	/* [45] "function %s has illegal storage class" */
	0,	/* [46] "function illegal in structure or union" */
	0,	/* [47] "function returns illegal type" */
	0,	/* [48] "gcos BCD constant illegal" */
	4,	/* [49] "illegal array size combination" */
	0,	/* [50] "illegal break" */
	0,	/* [51] "illegal character: %03o (octal)" */
	0,	/* [52] "illegal class for %s" */
	8,	/* [53] "illegal combination of pointer and integer, op %s" */
	6,	/* [54] "illegal comparison of enums" */
	0,	/* [55] "illegal continue" */
	0,	/* [56] "illegal field size" */
	0,	/* [57] "illegal field type" */
	0,	/* [58] "illegal function" */
	0,	/* [59] "illegal hex constant" */
	0,	/* [60] "illegal indirection" */
	0,	/* [61] "illegal initialization" */
	0,	/* [62] "illegal lhs of assignment operator" */
	14,	/* [63] "illegal member use: %s" */
	0,	/* [64] "illegal character: '%c'" */
	16,	/* [65] "illegal member use: perhaps %s.%s" */
	7,	/* [66] "illegal pointer combination" */
	0,	/* [67] "illegal pointer subtraction" */
	0,	/* [68] "illegal register declaration %s" */
	10,	/* [69] "illegal structure pointer combination" */
	0,	/* [70] "illegal type combination" */
	0,	/* [71] "illegal types in :" */
	0,	/* [72] "illegal use of field" */
	0,	/* [73] "illegal zero sized structure member: %s" */
	0,	/* [74] "illegal {" */
	0,	/* [75] "loop not entered at top" */
	17,	/* [76] "member of structure or union required" */
	0,	/* [77] "newline in BCD constant" */
	0,	/* [78] "newline in string or char constant" */
	0,	/* [79] "no automatic aggregate initialization" */
	0,	/* [80] "case expression must be an integer constant" */
	0,	/* [81] "non-null byte ignored in string initializer" */
	0,	/* [82] "nonportable character comparison" */
	0,	/* [83] "the only portable field type is unsigned int" */
	13,	/* [84] "nonunique name demands struct/union or struct/union pointer" */
	0,	/* [85] "null dimension" */
	20,	/* [86] "null effect" */
	0,	/* [87] "old-fashioned assignment operator" */
	0,	/* [88] "old-fashioned initialization: use =" */
	0,	/* [89] "operands of %s have incompatible types" */
	0,	/* [90] "pointer required" */
	9,	/* [91] "possible pointer alignment problem" */
	0,	/* [92] "precedence confusion possible: parenthesize!" */
	0,	/* [93] "precision lost in assignment to (possibly sign-extended) field" */
	0,	/* [94] "precision lost in field assignment" */
	0,	/* [95] "questionable conversion of function pointer" */
	0,	/* [96] "redeclaration of %s" */
	0,	/* [97] "redeclaration of formal parameter, %s" */
	22,	/* [98] "pointer casts may be troublesome" */
	0,	/* [99] "sizeof returns 0" */
	21,	/* [100] "statement not reached" */
	0,	/* [101] "static variable %s unused" */
	0,	/* [102] "struct/union %s never defined" */
	11,	/* [103] "struct/union or struct/union pointer required" */
	0,	/* [104] "enum %s never defined" */
	19,	/* [105] "structure reference must be addressable" */
	0,	/* [106] "structure typed union member must be named" */
	0,	/* [107] "too many characters in character constant" */
	0,	/* [108] "too many initializers" */
	0,	/* [109] "type clash in conditional" */
	0,	/* [110] "unacceptable operand of &" */
	0,	/* [111] "undeclared initializer name %s" */
	0,	/* [112] "undefined structure or union" */
	0,	/* [113] "unexpected EOF" */
	0,	/* [114] "unknown size" */
	0,	/* [115] "unsigned comparison with 0?" */
	0,	/* [116] "void function %s cannot return value" */
	0,	/* [117] "void type for %s" */
	0,	/* [118] "void type illegal in expression" */
	0,	/* [119] "zero or negative subscript" */
	0,	/* [120] "zero size field" */
	0,	/* [121] "zero sized structure" */
	0,	/* [122] "} expected" */
	23,	/* [123] "long in case or switch statement may be truncated" */
	0,	/* [124] "illegal octal constant" */
	0,	/* [125] "floating point constant folding causes exception" */
	0,	/* [126] "old style assign-op causes syntax error" */
	0,	/* [127] "main() returns random value to invocation environment" */
	0,	/* [128] "`%s' may be indistinguishable from `%s' due to internal name truncation" */
	0,	/* [129] "Label in expression" */
	0,	/* [130] "switch expression must have integral or enumerated type" */
	0,	/* [131] "undefined enum" */
	0,	/* [132] "block nesting too deep" */
	0,	/* [133] "zero-length array element" */
	0,	/* [134] "cannot take size of a function" */
	0,	/* [135] "%s is not a permitted struct/union operation" */
	0,	/* [136] "negative shift" */
	0,	/* [137] "shift greater than size of object" */
	0,	/* [138] "%s declared as parameter to non-function" */
	0,	/* [139] "declaration of %s hides parameter" */
	0,	/* [140] "cannot declare objects of type void" */
	0,	/* [141] "static function %s unused" */
	0,	/* [142] "illegal pointer conversion" */
	0,	/* [143] "unknown preprocessor directive" */
};


/* msgtotal is a cumulative count of the number of each message buffered
 * msgtotal[ 0 ] is the number of unbuffered messages generated
 *
 * msgtotal is indexed by the buffer index found in msgbuf
 *
 */

/* note that as an extern it is assumed that msgtotal is initialized to 0 */

/* msgtype is an array of codes that indicate how to print a message
 * each entry corresponds to a message in msgtext
 */

short	msgtype[ NUMMSGS ] = {
	STRINGTY,	/* [0] "%s evaluation order undefined" */
	STRINGTY,	/* [1] "%s may be used before set" */
	STRINGTY,	/* [2] "%s redefinition hides earlier one" */
	DBLSTRTY,	/* [3] "%s set but not used in function %s" */
	STRINGTY,	/* [4] "%s undefined" */
	PLAINTY,	/* [5] "bad structure offset", */
	DBLSTRTY,	/* [6] "%s unused in function %s" */
	PLAINTY,	/* [7] "& before array or function: ignored" */
	CHARTY,		/* [8] "=<%c illegal" */
	CHARTY,		/* [9] "=>%c illegal" */
	PLAINTY,	/* [10] "BCD constant exceeds 6 characters" */
	PLAINTY,	/* [11] "a function is declared as an argument" */
	PLAINTY,	/* [12] "ambiguous assignment: assignment op taken" */
	DBLSTRTY,	/* [13] "argument %s unused in function %s" */
	PLAINTY,	/* [14] "array of functions is illegal" */
	PLAINTY,	/* [15] "assignment of different structures" */
	PLAINTY,	/* [16] "bad asm construction" */
	PLAINTY,	/* [17] "bad scalar initialization" */
	STRINGTY | SIMPL,	/* [18] "cannot take address of %s" */
	PLAINTY,	/* [19] "cannot initialize extern or union" */
	PLAINTY,	/* [20] "case not in switch" */
	PLAINTY,	/* [21] "comparison of unsigned with negative constant" */
	PLAINTY,	/* [22] "constant argument to NOT" */
	PLAINTY,	/* [23] "constant expected" */
	PLAINTY,	/* [24] "constant in conditional context" */
	PLAINTY,	/* [25] "constant too big for cross-compiler" */
	PLAINTY,	/* [26] "conversion from long may lose accuracy" */
	PLAINTY,	/* [27] "conversion to long may sign-extend incorrectly" */
	STRINGTY,	/* [28] "declared argument %s is missing" */
	PLAINTY,	/* [29] "default not inside switch" */
	PLAINTY,	/* [30] "degenerate unsigned comparison" */
	PLAINTY,	/* [31] "division by 0" */
	PLAINTY,	/* [32] "division by 0." */
	NUMTY,		/* [33] "duplicate case in switch, %d" */
	PLAINTY,	/* [34] "duplicate default in switch" */
	PLAINTY,	/* [35] "empty array declaration" */
	PLAINTY,	/* [36] "empty character constant" */
	STRINGTY,	/* [37] "enumeration type clash, operator %s" */
	PLAINTY,	/* [38] "field outside of structure" */
	PLAINTY,	/* [39] "field too big" */
	PLAINTY,	/* [40] "fortran declaration must apply to function" */
	PLAINTY,	/* [41] "fortran function has wrong type" */
	PLAINTY,	/* [42] "fortran keyword nonportable" */
	STRINGTY,	/* [43] "function %s has return(e); and return;" */
	PLAINTY,	/* [44] "function declaration in bad context" */
	STRINGTY,	/* [45] "function %s has illegal storage class" */
	PLAINTY,	/* [46] "function illegal in structure or union" */
	PLAINTY,	/* [47] "function returns illegal type" */
	PLAINTY,	/* [48] "gcos BCD constant illegal" */
	PLAINTY,	/* [49] "illegal array size combination" */
	PLAINTY,	/* [50] "illegal break" */
	NUMTY,		/* [51] "illegal character: %03o (octal)" */
	STRINGTY,	/* [52] "illegal class for %s" */
	STRINGTY,	/* [53] "illegal combination of pointer and integer, op %s" */
	PLAINTY,	/* [54] "illegal comparison of enums" */
	PLAINTY,	/* [55] "illegal continue" */
	PLAINTY,	/* [56] "illegal field size" */
	PLAINTY,	/* [57] "illegal field type" */
	PLAINTY,	/* [58] "illegal function" */
	PLAINTY,	/* [59] "illegal hex constant" */
	PLAINTY,	/* [60] "illegal indirection" */
	PLAINTY,	/* [61] "illegal initialization" */
	PLAINTY,	/* [62] "illegal lhs of assignment operator" */
	STRINGTY | SIMPL,	/* [63] "illegal member use: %s" */
	CHARTY,		/* [64] "illegal character: '%c'" */
	DBLSTRTY,	/* [65] "illegal member use: perhaps %s.%s" */
	PLAINTY,	/* [66] "illegal pointer combination" */
	PLAINTY,	/* [67] "illegal pointer subtraction" */
	STRINGTY,	/* [68] "illegal register declaration %s" */
	PLAINTY,	/* [69] "illegal structure pointer combination" */
	PLAINTY,	/* [70] "illegal type combination" */
	PLAINTY,	/* [71] "illegal types in :" */
	PLAINTY,	/* [72] "illegal use of field" */
	STRINGTY,	/* [73] "illegal zero sized structure member: %s" */
	PLAINTY,	/* [74] "illegal {" */
	PLAINTY,	/* [75] "loop not entered at top" */
	PLAINTY,	/* [76] "member of structure or union required" */
	PLAINTY,	/* [77] "newline in BCD constant" */
	PLAINTY,	/* [78] "newline in string or char constant" */
	PLAINTY,	/* [79] "no automatic aggregate initialization" */
	PLAINTY,	/* [80] "case expression must be an integer constant" */
	PLAINTY,	/* [81] "non-null byte ignored in string initializer" */
	PLAINTY,	/* [82] "nonportable character comparison" */
	PLAINTY,	/* [83] "the only portable field type is unsigned int" */
	PLAINTY,	/* [84] "nonunique name demands struct/union or struct/union pointer" */
	PLAINTY,	/* [85] "null dimension" */
	PLAINTY,	/* [86] "null effect" */
	PLAINTY,	/* [87] "old-fashioned assignment operator" */
	PLAINTY,	/* [88] "old-fashioned initialization: use =" */
	STRINGTY,	/* [89] "operands of %s have incompatible types" */
	PLAINTY,	/* [90] "pointer required" */
	PLAINTY,	/* [91] "possible pointer alignment problem" */
	PLAINTY,	/* [92] "precedence confusion possible: parenthesize!" */
	PLAINTY,	/* [93] "precision lost in assignment to (possibly sign-extended) field" */
	PLAINTY,	/* [94] "precision lost in field assignment" */
	PLAINTY,	/* [95] "questionable conversion of function pointer" */
	STRINGTY,	/* [96] "redeclaration of %s" */
	STRINGTY,	/* [97] "redeclaration of formal parameter, %s" */
	PLAINTY,	/* [98] "pointer casts may be troublesome" */
	PLAINTY,	/* [99] "sizeof returns 0" */
	PLAINTY,	/* [100] "statement not reached" */
	STRINGTY,	/* [101] "static variable %s unused" */
	STRINGTY,	/* [102] "struct/union %s never defined" */
	PLAINTY,	/* [103] "struct/union or struct/union pointer required" */
	STRINGTY,	/* [104] "enum %s never defined" */
	PLAINTY,	/* [105] "structure reference must be addressable" */
	PLAINTY,	/* [106] "structure typed union member must be named" */
	PLAINTY,	/* [107] "too many characters in character constant" */
	PLAINTY,	/* [108] "too many initializers" */
	PLAINTY,	/* [109] "type clash in conditional" */
	PLAINTY,	/* [110] "unacceptable operand of &" */
	STRINGTY,	/* [111] "undeclared initializer name %s" */
	PLAINTY,	/* [112] "undefined structure or union" */
	PLAINTY,	/* [113] "unexpected EOF" */
	PLAINTY,	/* [114] "unknown size" */
	PLAINTY,	/* [115] "unsigned comparison with 0?" */
	STRINGTY,	/* [116] "void function %s cannot return value" */
	STRINGTY,	/* [117] "void type for %s" */
	PLAINTY,	/* [118] "void type illegal in expression" */
	PLAINTY,	/* [119] "zero or negative subscript" */
	PLAINTY,	/* [120] "zero size field" */
	PLAINTY,	/* [121] "zero sized structure" */
	PLAINTY,	/* [122] "} expected" */
	PLAINTY,	/* [123] "long in case or switch statement may be truncated" */
	PLAINTY,	/* [124] "illegal octal constant" */
	PLAINTY,	/* [125] "floating point constant folding causes exception" */
	PLAINTY,	/* [126] "old style assign-op causes syntax error" */
	PLAINTY,	/* [127] "main() returns random value to invocation environment" */
	DBLSTRTY,	/* [128] "`%s' may be indistinguishable from `%s' due to internal name truncation" */
	PLAINTY,	/* [129] "Label in expression" */
	PLAINTY,	/* [130] "switch expression must have integral or enumerated type" */
	PLAINTY,	/* [131] "undefined enum" */
	PLAINTY,	/* [132] "block nesting too deep" */
	PLAINTY,	/* [133] "zero-length array element" */
	PLAINTY,	/* [134] "cannot take size of a function" */
	STRINGTY,	/* [135] "%s is not a permitted struct/union operation" */
	PLAINTY,	/* [136] "negative shift" */
	PLAINTY,	/* [137] "shift greater than size of object" */
	STRINGTY,	/* [138] "%s declared as parameter to non-function" */
	STRINGTY,	/* [139] "declaration of %s hides parameter" */
	PLAINTY,	/* [140] "cannot declare objects of type void" */
	STRINGTY,	/* [141] "static function %s unused" */
	PLAINTY,	/* [142] "illegal pointer conversion" */
	PLAINTY,	/* [143] "unknown preprocessor directive" */
};


char		*outmsg[ NUMBUF ] = {
/* [0] */	"",
/* [1] */	"argument unused in function:",
/* [2] */	"conversion from long may lose accuracy",
/* [3] */	"conversion to long may sign-extend incorrectly",
/* [4] */	"illegal array size combination",
/* [5] */	"enumeration type clash:",
/* [6] */	"illegal comparison of enums",
/* [7] */	"illegal pointer combination",
/* [8] */	"illegal combination of pointer and integer:",
/* [9] */	"possible pointer alignment problem",
/* [10] */	"illegal structure pointer combination",
/* [11] */	"struct/union or struct/union pointer required",
/* [12] */	"assignment of different structures",
/* [13] */	"nonunique name demands struct/union or struct/union pointer",
/* [14] */	"illegal member use:",
/* [15] */	"",	/* NOT USED */
/* [16] */	"illegal member use:",
/* [17] */	"member of structure or union required",
/* [18] */	"cannot take address of:",
/* [19] */	"structure reference must be addressable",
/* [20] */	"null effect",
/* [21] */	"statement not reached",
/* [22] */	"pointer casts may be troublesome",
/* [23] */	"long in case or switch statement may be truncated",
};

char		*outformat[ NUMBUF ] = {
	/* [0] */	"",
	/* [1] */	"%s in %s",
	/* [2] */	"",
	/* [3] */	"",
	/* [4] */	"",
	/* [5] */	"operator %s",
	/* [6] */	"",
	/* [7] */	"",
	/* [8] */	"operator %s",
	/* [9] */	"",
	/* [10] */	"",
	/* [11] */	"",
	/* [12] */	"",
	/* [13] */	"",
	/* [14] */	"%s",
	/* [15] */	"",		/* NOT USED */
	/* [16] */	"perhaps %s.%s",
	/* [17] */	"",
	/* [18] */	"%s",
	/* [19] */	"",
	/* [20] */	"",
	/* [21] */	"",
	/* [22] */	"",
	/* [23] */	"",
};

