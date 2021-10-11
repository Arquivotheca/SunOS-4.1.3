#! /bin/sh
case $1 in
	'')	file=template.stripped
		echo "template: output in template.stripped"
	;;
	*)	file=$1
	;;
esac
sed -e 1,10d -e '/^\.\\"---/d' -e '/;/s/%\\&/%/g' < ./template > $file
exit
.\"--- %Z%%M% %I% %E% SMI; 
.\"---
.\"--- Please read the "Man Page Specification" before using this template.
.\"---
.\"--- Substitute the appropriate text for items beginning and ending with `_'
.\"--- (for example, _TITLE_ and _name_).  \-option_ items begin with
.\"--- `\-' (en-dash) and end with `_'.  Do not delete the `\-' characters.
.\"--- Be sure to use upper or lower case indicated for each item.
.\"---
.\"--- You need only use the parts of this template appropriate for your
.\"--- particular man page.  Delete the parts that aren't pertinent.
.\"---
.\"--- If your man page is copyrighted, please preserve the copyright
.\"--- notice.
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- The following line is an instruction which insures that any
.\"--- preprocessors applied to the man page will be invoked in the proper
.\"--- order.  `t' refers to tbl, `e' refers to eqn and `p' refers to pic.
.\"--- Only include those necessary for your particular man page.
.\"--- If there are none, delete this line.
.\"---
'\" tep
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- The next line is the SCCS ID line, which must appear in any file
.\"--- under the control of SCCS.  It contains extraneous zero-width characters
.\"--- `\&' to prevent SCCS from interpreting it as the SCCS line for this
.\"--- template file.  Executing this template will produce a version of the
.\"--- template with these characters removed, thereby generating a valid
.\"--- SCCS line ID for your man page.
.\"---
.\"--- _source_ is where the page comes from, for example, "UCB 4.3 BSD"
.\"--- or "S5r3".  For Sun-originated pages, the "from" information may
.\"--- omitted.
.\"---
.\" %\&Z%%\&M% %\&I% %\&E% SMI; from _source_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- PAGE HEADING
.\"--- This section provides information for the header and footer of the man
.\"--- page.  _#S_ specifies the manual section in which the page will appear,
.\"--- where # is the number of the section and S (if needed) is the letter
.\"--- of the subsection.  The _Month_ should be spelled out (September,
.\"--- October).
.\"---
.TH _TITLE_ _#S_ "_dd_ _Month_ _19yy_"
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- NAME
.\"--- This section is used by cross-referencing programs.  Hence, do not
.\"--- use any font changes or troff escape sequences in this section.
.\"--- The _summary-line_ is brief, all on one line.
.\"---
.SH NAME
_name_ \- _summary-line_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- CONFIGURATION
.\"--- This section occurs in pages from manual Section 4 only.
.\"--- List lines literally, exactly as they should appear in the
.\"--- configuration file.  Include a brief _explanation-paragraph_ if
.\"--- you feel that the file lines need an explanation.
.\"---
.SH CONFIGURATION
.nf
.ft B
_configuration-file-lines_
.ft
.fi
.LP
_explanation-paragraph_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- SYNOPSIS [SYSTEM V SYNOPSIS]
.\"--- This section is a syntax diagram.  Use the following lines for pages in
.\"--- manual Sections 1, 6, 7 and 8:
.\"---
.SH SYNOPSIS
.SH SYSTEM V SYNOPSIS
.B _command-name_
.\"---
.\"--- For an \-option-list_ of one or more single-character options, use:
.\"---
[
.B \-option-list_
]
.\"---
.\"--- For subcommands, delete the above bracket lines.
.\"---
.if n .ti +5n
.\"---
.\"--- For mutually exclusive \-options_, use:
.\"---
[
.BR \-option_ \||\|\c
.BR \-option_ \||\|\c
.B \-option_
]
.\"---
.\"--- For \-options_ with arguments, use:
.\"---
[
.B \-option_\c
.I _arg_
]
.if t .ti +.5i
.\"---
.\"--- For \-options_ with optional arguments, use:
.\"---
[
.B \-option_\c
[
.I _arg_
]
]
.\"---
.\"--- If a space is required between \-option_ and _arg_ or [ _arg_ ],
.\"--- delete `\c'.
.\"---
.\"--- Use the following lines for pages in manual Sections 2, 3, 4 and 5: 
.\"---
.LP
.nf
.ft B
_#include-statement_
_#define-statement_
.sp .5v
_struct-definition_
.sp .5v
_function-definition_
.ft R
.fi
.LP
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- PROTOCOL
.\"--- This section occurs in pages from manual Section 3R only.  List the
.\"--- _protocol-specification-pathname_ literally, exactly as it should be
.\"--- entered.
.\"---
.SH PROTOCOL
.B _protocol-specification-pathname_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- AVAILABILITY
.\"--- This section describes any conditions or restrictions on the use of the
.\"--- command (function, device or file format).
.\"---
.SH AVAILABILITY
.LP
_description-of-restriction_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- DESCRIPTION [SYSTEM V DESCRIPTION]
.\"--- This section tells concisely what the command (function, device or
.\"--- file format) does.  Do not discuss options or cite examples.
.\"---
.SH DESCRIPTION
.\"---
.\"--- The _1st_index_term_ and _2nd_index_term_ will appear in the manual
.\"--- index.  _format_of_1st_ and _format_of_2nd_ specify formatting for
.\"--- these index entries.
.\"---
.IX "_1st_index_term_" "_2nd_index_term_" "_format_of_1st_" "_format_of_2nd_"
.LP
_description-paragraph_
.SH SYSTEM V DESCRIPTION
.LP
_description-paragraph_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- IOCTLS
.\"--- This section appears in pages from manual Section 4 only.
.\"--- Only devices which supply appropriate parameters to the ioctl(2)
.\"--- system call are _IOCTLS_.
.\"---
.\"--- Within the description of an _IOCTL_, list any _ERROR-VALUES_ that it
.\"--- may generate and explain them.
.\"---
.SH IOCTLS
.TP 15
.SB _IOCTL_
_description_
.RS
.TP 15
.SB _ERROR-VALUE_
_explanation_
.RE
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- OPTIONS [SYSTEM V OPTIONS]
.\"--- This section lists \-options_ and summarizes concisely what each does.
.\"--- List options literally, as they should be entered, in the order they
.\"--- appear in the SYNOPSIS section.
.\"---
.\"--- _summaries_ are in the imperative voice.
.\"---
.SH OPTIONS
.SH SYSTEM V OPTIONS
.TP
.B \-option_
_summary_
.\"---
.\"--- For \-options_ with arguments, use:
.\"---
.TP
.BI \-option_ " _arg_"
_summary_
.\"---
.\"--- If no space is required between \-option_ and _arg_, delete the
.\"--- double quotes `"'.
.\"---
.RS
.TP
.I _arg_
_description_
.RE
.\"---
.\"--- For \-options_ with optional arguments, use:
.\"---
.HP
.B \-option_\c
[
.I _arg_
]
.br
_summary_
.RS
.TP
.I _arg_
_description_
.RE
.\"---
.\"--- If a space is required between \-option_ and [ _arg_ ],
.\"--- delete `\c'.
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- RETURN VALUE
.\"--- This section appears in pages from Sections 2 and 3 only.
.\"--- List the _values_ that the function returns and give _explanations_.
.\"---
.SH "RETURN VALUE"
.TP 15
_value_
_explanation_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- ERRORS
.\"--- This section lists and explains _ERROR-CODES_ that the function may
.\"--- may generate.  List _ERROR-CODES_ alphabetically.
.\"---
.SH ERRORS
.TP 15
.SM _ERROR-CODE_
_explanation_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- USAGE
.\"--- This section describes commands, modifiers, variables, expressions
.\"--- or input grammar used in interactive commands.  The format for commands
.\"--- is give here.  The formats for the others are similar.
.\"---
.SH USAGE
.SS Commands
.TP
.B _command_
_description_
.TP
.BI _command_ " _arg_"
_description_
.\"---
.\"--- If no space is required between _command_ and _arg_, delete the
.\"--- double quotes `"'.
.\"---
.RS
.TP
.I _arg_
_description_
.RE
.HP
.B _command_\c
[
.I _arg_
]
.br
_description_
.RS
.TP
.I _arg_
_description_
.RE
.\"---
.\"--- If a space is required between _command_ and [ _arg_ ],
.\"--- delete `\c'.
.\"---
.SS Modifiers
.SS Variables
.SS Expressions
.SS Input Grammar
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- EXAMPLE(S)
.\"--- This section gives examples of how to use the command (function
.\"--- or file format).  Always preface an example with an _introduction_.
.\"--- If there are multiple examples, use separate subsection headings
.\"--- for each _example-type_.  Otherwise, omit these headings.
.\"---
.SH EXAMPLES
.SS _example-type_
.LP
_introduction_
.IP
_one-line-example_
.LP
.RS
.ft B
.nf
_multi-line_
_example_
.fi
.ft
.RE
.LP
.\"---
.\"--- When a prompt is required in an example, use:
.\"---
.B example%
.\"---
.LP
_explanation_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- ENVIRONMENT
.\"--- This section lists the _ENVARS_ (environment variables) which
.\"--- are used by the command or function and gives brief
.\"--- _descriptions_ of their effects.
.\"---
.SH ENVIRONMENT
.LP
_introduction_
.TP 15
.SB _ENVAR_
_description_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- FILES
.\"--- This section lists all _filenames_ refered to on the page,
.\"--- each followed by a _descriptive-summary_.
.\"---
.SH FILES
.PD 0
.TP 20
.B _filename_ 
_descriptive-summary_
.PD
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- SEE ALSO
.\"--- This section lists references to other man pages, in-house
.\"--- documents and other publications.
.\"---
.SH "SEE ALSO"
.BR _man-page_ (_#S_),
.BR _man-page_ (_#S_),
.\"---
.\"--- Use _TX-macro-abbreviations_ to list in-house documents
.\"--- (for example, "SVBG" for "SunView 1 Beginner's Guide").
.\"---
.LP
.TX _TX-macro-abbreviation_
.LP
.\"---
.\"--- Use this format for listing outside publications:
.\"---
_Author_,
.I "_Outside-Doc-Title_,"
_Year-by-Holder_, _Publisher_.
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- DIAGNOSTICS
.\"--- This section lists diagnostic _messages_ and brief _explanations_
.\"--- of their meanings.  List _messages_ literally, exactly as they
.\"--- will appear.
.\"---
.SH DIAGNOSTICS
.TP
.B _message_
_explanation_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- WARNINGS
.\"--- This section lists warnings about special conditions.
.\"---
.\"--- Warnings are NOT diagnostics.
.\"---
.SH WARNINGS
.LP
_warning_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- NOTES
.\"--- This section lists additional information that does not belong
.\"--- anywhere else on the page.  Do not give critical information
.\"--- in this section.
.\"---
.SH NOTES
.LP
_note_
.\"---
.\"----------------------------------------------------------------------------
.\"---
.\"--- BUGS
.\"--- This section describes any bugs that may exist.  Where possible,
.\"--- suggest workarounds.
.\"---
.SH BUGS
.LP
_description-of-bug_
